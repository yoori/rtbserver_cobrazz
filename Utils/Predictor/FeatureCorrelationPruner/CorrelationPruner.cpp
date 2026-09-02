#include "CorrelationPruner.hpp"

#include <algorithm>
#include <bit>
#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

#include <Stream/MemoryStream.hpp>

namespace AdServer::Predictor
{
  namespace
  {
    std::uint64_t
    popcount_portable(std::uint64_t value) noexcept
    {
      value -= (value >> 1) & 0x5555555555555555ULL;
      value =
        (value & 0x3333333333333333ULL) +
        ((value >> 2) & 0x3333333333333333ULL);
      value = (value + (value >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
      return (value * 0x0101010101010101ULL) >> 56;
    }

#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    __attribute__((target("popcnt")))
    std::uint64_t
    binary_dot_popcnt(
      const std::vector<std::uint64_t>& left,
      const std::vector<std::uint64_t>& right) noexcept
    {
      std::uint64_t result = 0;
      for (std::size_t index = 0; index < left.size(); ++index)
      {
        result += std::popcount(left[index] & right[index]);
      }
      return result;
    }
#endif

    std::uint64_t
    binary_dot_portable(
      const std::vector<std::uint64_t>& left,
      const std::vector<std::uint64_t>& right) noexcept
    {
      std::uint64_t result = 0;
      for (std::size_t index = 0; index < left.size(); ++index)
      {
        result += popcount_portable(left[index] & right[index]);
      }
      return result;
    }

    std::uint64_t
    binary_dot(
      const std::vector<std::uint64_t>& left,
      const std::vector<std::uint64_t>& right) noexcept
    {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
      if (__builtin_cpu_supports("popcnt"))
      {
        return binary_dot_popcnt(left, right);
      }
#endif
      return binary_dot_portable(left, right);
    }

    bool
    next_token(std::string_view line, std::size_t& position, std::string_view& token) noexcept
    {
      while (
        position < line.size() &&
        (line[position] == ' ' || line[position] == '\t' || line[position] == '\r'))
      {
        ++position;
      }

      if (position == line.size())
      {
        return false;
      }

      const std::size_t begin = position;
      while (
        position < line.size() &&
        line[position] != ' ' &&
        line[position] != '\t' &&
        line[position] != '\r')
      {
        ++position;
      }
      token = line.substr(begin, position - begin);
      return true;
    }

    FeatureCorrelationPruner::FeatureIndex
    parse_index(std::string_view token)
    {
      const std::size_t separator = token.find(':');
      if (separator == std::string_view::npos || separator == 0)
      {
        Stream::Error error;
        error << "Invalid LibSVM feature token '" << token << "'";
        throw FeatureCorrelationPruner::Exception(error);
      }

      FeatureCorrelationPruner::FeatureIndex result = 0;
      const auto parsed = std::from_chars(token.data(), token.data() + separator, result);
      if (
        parsed.ec != std::errc() ||
        parsed.ptr != token.data() + separator ||
        result == 0)
      {
        Stream::Error error;
        error << "Invalid LibSVM feature index '" << token.substr(0, separator) << "'";
        throw FeatureCorrelationPruner::Exception(error);
      }
      return result;
    }

    float
    parse_value(std::string_view token)
    {
      const std::size_t separator = token.find(':');
      const char* begin = token.data() + separator + 1;
      const char* end = token.data() + token.size();
      const std::size_t size = end - begin;
      if (size >= 128)
      {
        Stream::Error error;
        error << "Invalid LibSVM feature value '" << token.substr(separator + 1) << "'";
        throw FeatureCorrelationPruner::Exception(error);
      }

      char buffer[128];
      std::memcpy(buffer, begin, size);
      buffer[size] = '\0';
      char* parsed_end = nullptr;
      errno = 0;
      const float result = std::strtof(buffer, &parsed_end);
      if (
        parsed_end == buffer ||
        parsed_end != buffer + size ||
        errno == ERANGE ||
        !std::isfinite(result))
      {
        Stream::Error error;
        error << "Invalid LibSVM feature value '" << token.substr(separator + 1) << "'";
        throw FeatureCorrelationPruner::Exception(error);
      }
      return result;
    }
  }

  FeatureCorrelationPruner::FeatureCorrelationPruner(
    FeatureIndexes feature_indexes,
    double correlation_threshold)
    : feature_indexes_(std::move(feature_indexes)),
      indexes_(feature_indexes_.begin(), feature_indexes_.end()),
      columns_(indexes_.size()),
      correlation_threshold_(correlation_threshold)
  {
    if (feature_indexes_.empty())
    {
      throw Exception("At least one feature index is required");
    }

    if (
      !std::isfinite(correlation_threshold_) ||
      correlation_threshold_ <= 0 ||
      correlation_threshold_ > 1)
    {
      throw Exception("Correlation threshold must be in the range (0, 1]");
    }

    const FeatureIndex max_index = indexes_.back();
    if (max_index <= DIRECT_INDEX_LIMIT)
    {
      direct_positions_.assign(max_index + 1, INVALID_POSITION);
      for (Position position = 0; position < indexes_.size(); ++position)
      {
        direct_positions_[indexes_[position]] = position;
      }
    }
    else
    {
      sparse_positions_.reserve(indexes_.size());
      for (Position position = 0; position < indexes_.size(); ++position)
      {
        sparse_positions_.emplace(indexes_[position], position);
      }
    }
  }

  void FeatureCorrelationPruner::add_row(const FeatureValues& values)
  {
    std::vector<Position> positions;
    std::vector<float> parsed_values(columns_.size());
    std::vector<unsigned char> present(columns_.size());
    positions.reserve(values.size());
    for (const auto& [index, value] : values)
    {
      if (!std::isfinite(value))
      {
        throw Exception("Feature value must be finite");
      }

      const Position position = position_(index);
      if (position == INVALID_POSITION)
      {
        continue;
      }

      if (!present[position])
      {
        present[position] = 1;
        positions.push_back(position);
      }
      parsed_values[position] = value;
    }
    add_parsed_row_(positions, parsed_values);
  }

  void FeatureCorrelationPruner::add_svm(std::istream& input, std::string_view source_name)
  {
    std::vector<Position> positions;
    std::vector<float> values(columns_.size());
    std::vector<std::uint64_t> marks(columns_.size());
    std::uint64_t mark = 0;
    std::uint64_t line_number = 0;
    std::string line;
    while (std::getline(input, line))
    {
      ++line_number;
      if (line.find_first_not_of(" \t\r") == std::string::npos)
      {
        continue;
      }

      ++mark;
      positions.clear();
      std::size_t token_position = 0;
      std::string_view token;
      if (!next_token(line, token_position, token))
      {
        continue;
      }

      try
      {
        while (next_token(line, token_position, token))
        {
          const FeatureIndex index = parse_index(token);
          const Position position = position_(index);
          if (position == INVALID_POSITION)
          {
            continue;
          }

          if (marks[position] != mark)
          {
            marks[position] = mark;
            positions.push_back(position);
          }
          values[position] = parse_value(token);
        }
      }
      catch(const Exception& exception)
      {
        Stream::Error error;
        error << source_name << ':' << line_number << ": " << exception.what();
        throw Exception(error);
      }

      add_parsed_row_(positions, values);
    }

    if (!input.eof())
    {
      Stream::Error error;
      error << "Can't read LibSVM source '" << source_name << "'";
      throw Exception(error);
    }
  }

  FeatureCorrelationPruner::Result FeatureCorrelationPruner::result() const
  {
    Result result;
    result.rows = rows_;
    if (rows_ == 0)
    {
      result.kept = feature_indexes_;
      return result;
    }

    const auto statistics = calculate_statistics_();
    std::vector<unsigned char> dropped(indexes_.size());
    for (Position left = 0; left < indexes_.size(); ++left)
    {
      if (dropped[left])
      {
        continue;
      }

      result.kept.insert(indexes_[left]);
      for (Position right = left + 1; right < indexes_.size(); ++right)
      {
        if (dropped[right])
        {
          continue;
        }

        const double correlation = correlation_(left, right, statistics);
        if (std::abs(correlation) >= correlation_threshold_)
        {
          dropped[right] = 1;
          result.dropped.insert(indexes_[right]);
          result.drops.push_back({indexes_[right], indexes_[left], correlation});
        }
      }
    }
    std::sort(
      result.drops.begin(),
      result.drops.end(),
      [](const Drop& left, const Drop& right)
      {
        return left.index < right.index;
      });
    return result;
  }

  std::uint64_t FeatureCorrelationPruner::rows() const noexcept
  {
    return rows_;
  }

  FeatureCorrelationPruner::Position
  FeatureCorrelationPruner::position_(FeatureIndex index) const noexcept
  {
    if (!direct_positions_.empty())
    {
      return index < direct_positions_.size()
        ? direct_positions_[index]
        : INVALID_POSITION;
    }

    const auto iterator = sparse_positions_.find(index);
    return iterator == sparse_positions_.end() ? INVALID_POSITION : iterator->second;
  }

  void
  FeatureCorrelationPruner::add_parsed_row_(
    const std::vector<Position>& positions,
    const std::vector<float>& values)
  {
    if (rows_ % 64 == 0)
    {
      for (Column& column : columns_)
      {
        if (column.binary)
        {
          column.bits.push_back(0);
        }
      }
    }

    for (Column& column : columns_)
    {
      if (!column.binary)
      {
        column.values.push_back(0);
      }
    }

    const std::uint64_t mask = 1ULL << (rows_ % 64);
    for (const Position position : positions)
    {
      const float value = values[position];
      if (value == 0)
      {
        continue;
      }

      Column& column = columns_[position];
      if (column.binary && value == 1)
      {
        column.bits.back() |= mask;
      }
      else
      {
        if (column.binary)
        {
          convert_to_numeric_(column);
        }
        column.values[rows_] = value;
      }
    }
    ++rows_;
  }

  void FeatureCorrelationPruner::convert_to_numeric_(Column& column)
  {
    column.values.assign(rows_ + 1, 0);
    for (std::size_t word_index = 0; word_index < column.bits.size(); ++word_index)
    {
      std::uint64_t word = column.bits[word_index];
      while (word != 0)
      {
        const unsigned long bit = std::countr_zero(word);
        const std::uint64_t row = word_index * 64 + bit;
        if (row < rows_)
        {
          column.values[row] = 1;
        }
        word &= word - 1;
      }
    }
    column.bits.clear();
    column.bits.shrink_to_fit();
    column.binary = false;
  }

  std::vector<FeatureCorrelationPruner::ColumnStatistics>
  FeatureCorrelationPruner::calculate_statistics_() const
  {
    std::vector<ColumnStatistics> result(columns_.size());
    for (Position position = 0; position < columns_.size(); ++position)
    {
      const Column& column = columns_[position];
      if (column.binary)
      {
        const double count = binary_dot_(column, column);
        result[position] = {count, count};
      }
      else
      {
        for (const float value : column.values)
        {
          result[position].sum += value;
          result[position].square_sum += static_cast<double>(value) * value;
        }
      }
    }
    return result;
  }

  double
  FeatureCorrelationPruner::correlation_(
    Position left,
    Position right,
    const std::vector<ColumnStatistics>& statistics) const noexcept
  {
    const double count = static_cast<double>(rows_);
    const double left_variance =
      count * statistics[left].square_sum -
      statistics[left].sum * statistics[left].sum;
    const double right_variance =
      count * statistics[right].square_sum -
      statistics[right].sum * statistics[right].sum;
    if (left_variance <= 0 || right_variance <= 0)
    {
      return 0;
    }

    const Column& left_column = columns_[left];
    const Column& right_column = columns_[right];
    double product_sum;
    if (left_column.binary && right_column.binary)
    {
      product_sum = binary_dot_(left_column, right_column);
    }
    else if (left_column.binary)
    {
      product_sum = binary_numeric_dot_(left_column, right_column);
    }
    else if (right_column.binary)
    {
      product_sum = binary_numeric_dot_(right_column, left_column);
    }
    else
    {
      product_sum = numeric_dot_(left_column, right_column);
    }

    const double numerator =
      count * product_sum - statistics[left].sum * statistics[right].sum;
    const double denominator = std::sqrt(left_variance * right_variance);
    return std::clamp(numerator / denominator, -1.0, 1.0);
  }

  double
  FeatureCorrelationPruner::binary_dot_(const Column& left, const Column& right) noexcept
  {
    return static_cast<double>(binary_dot(left.bits, right.bits));
  }

  double
  FeatureCorrelationPruner::binary_numeric_dot_(const Column& binary, const Column& numeric)
    noexcept
  {
    double result = 0;
    for (std::size_t word_index = 0; word_index < binary.bits.size(); ++word_index)
    {
      std::uint64_t word = binary.bits[word_index];
      while (word != 0)
      {
        const unsigned long bit = std::countr_zero(word);
        const std::size_t row = word_index * 64 + bit;
        if (row < numeric.values.size())
        {
          result += numeric.values[row];
        }
        word &= word - 1;
      }
    }
    return result;
  }

  double
  FeatureCorrelationPruner::numeric_dot_(const Column& left, const Column& right) noexcept
  {
    double result = 0;
    for (std::size_t row = 0; row < left.values.size(); ++row)
    {
      result += static_cast<double>(left.values[row]) * right.values[row];
    }
    return result;
  }
}
