#pragma once

#include <cstdint>
#include <istream>
#include <set>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <eh/Exception.hpp>

namespace AdServer::Predictor
{
  class FeatureCorrelationPruner
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using FeatureIndex = std::uint32_t;
    using FeatureIndexes = std::set<FeatureIndex>;
    using FeatureValue = std::pair<FeatureIndex, float>;
    using FeatureValues = std::vector<FeatureValue>;

    struct Drop
    {
      FeatureIndex index;
      FeatureIndex representative;
      double correlation;
    };

    struct Result
    {
      FeatureIndexes kept;
      FeatureIndexes dropped;
      std::vector<Drop> drops;
      std::uint64_t rows = 0;
    };

    FeatureCorrelationPruner(FeatureIndexes feature_indexes, double correlation_threshold);

    void add_row(const FeatureValues& values);

    void add_svm(std::istream& input, std::string_view source_name = "<stream>");

    Result result() const;

    std::uint64_t rows() const noexcept;

  private:
    struct Column
    {
      bool binary = true;
      std::vector<std::uint64_t> bits;
      std::vector<float> values;
    };

    struct ColumnStatistics
    {
      double sum = 0;
      double square_sum = 0;
    };

    using Position = std::uint32_t;

    static constexpr Position INVALID_POSITION = static_cast<Position>(-1);
    static constexpr FeatureIndex DIRECT_INDEX_LIMIT = 1U << 20;

    Position position_(FeatureIndex index) const noexcept;

    void add_parsed_row_(const std::vector<Position>& positions, const std::vector<float>& values);

    void convert_to_numeric_(Column& column);

    std::vector<ColumnStatistics> calculate_statistics_() const;

    double correlation_(
      Position left,
      Position right,
      const std::vector<ColumnStatistics>& statistics) const noexcept;

    static double binary_dot_(const Column& left, const Column& right) noexcept;

    static double binary_numeric_dot_(const Column& binary, const Column& numeric) noexcept;

    static double numeric_dot_(const Column& left, const Column& right) noexcept;

    FeatureIndexes feature_indexes_;
    std::vector<FeatureIndex> indexes_;
    std::vector<Column> columns_;
    std::vector<Position> direct_positions_;
    std::unordered_map<FeatureIndex, Position> sparse_positions_;
    double correlation_threshold_;
    std::uint64_t rows_ = 0;
  };
}
