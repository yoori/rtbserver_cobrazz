#include "Application.hpp"

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>

#include <Generics/AppUtils.hpp>

namespace
{
  const char USAGE[] =
    "Usage: FeatureDeduplicator --svm-file FILE "
    "--feature-indexes-file FILE "
    "[--output-feature-indexes-file FILE] "
    "[--dropped-features-file FILE] "
    "[--early-dropped-features-file FILE]";

  const std::uint64_t HASH_SEED_1 = 0x243f6a8885a308d3ULL;
  const std::uint64_t HASH_SEED_2 = 0x13198a2e03707344ULL;
}

Application_::FeatureIndexes
Application_::read_feature_indexes_(const char* file_name)
{
  std::ifstream input(file_name);
  if (!input)
  {
    Stream::Error ostr;
    ostr << "Can't open feature indexes file '" << file_name << "'";
    throw Exception(ostr);
  }

  FeatureIndexes result;
  std::string line;
  while (std::getline(input, line))
  {
    const auto first = line.find_first_not_of(" \t\r");
    if (first == std::string::npos || line[first] == '#')
    {
      continue;
    }
    const auto last = line.find_last_not_of(" \t\r");
    const std::string value = line.substr(first, last - first + 1);
    std::uint32_t index = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), index);
    if (parsed.ec != std::errc() || parsed.ptr != value.data() + value.size() || index == 0)
    {
      Stream::Error ostr;
      ostr << "Invalid feature index '" << value << "' in '" << file_name << "'";
      throw Exception(ostr);
    }
    result.insert(index);
  }
  return result;
}

std::uint32_t
Application_::parse_index_(const std::string& token)
{
  const auto separator = token.find(':');
  if (separator == std::string::npos || separator == 0)
  {
    Stream::Error ostr;
    ostr << "Invalid LibSVM feature token '" << token << "'";
    throw Exception(ostr);
  }
  const std::string value = token.substr(0, separator);
  std::uint32_t index = 0;
  const auto parsed = std::from_chars(value.data(), value.data() + value.size(), index);
  if (parsed.ec != std::errc() || parsed.ptr != value.data() + value.size() || index == 0)
  {
    Stream::Error ostr;
    ostr << "Invalid LibSVM feature index '" << value << "'";
    throw Exception(ostr);
  }
  return index;
}

std::uint32_t
Application_::parse_value_bits_(const std::string& token)
{
  const auto separator = token.find(':');
  const std::string value = token.substr(separator + 1);
  char* end = nullptr;
  errno = 0;
  const float parsed = std::strtof(value.c_str(), &end);
  if (end == value.c_str() || *end != '\0' || errno == ERANGE || !std::isfinite(parsed))
  {
    Stream::Error ostr;
    ostr << "Invalid LibSVM feature value '" << value << "'";
    throw Exception(ostr);
  }
  if (parsed == 0.0F)
  {
    return 0;
  }
  std::uint32_t bits = 0;
  std::memcpy(&bits, &parsed, sizeof(bits));
  return bits;
}

std::uint64_t
Application_::mix_(std::uint64_t value) noexcept
{
  value ^= value >> 30;
  value *= 0xbf58476d1ce4e5b9ULL;
  value ^= value >> 27;
  value *= 0x94d049bb133111ebULL;
  return value ^ (value >> 31);
}

void
Application_::update_fingerprint_(
  FeatureState& state,
  std::uint64_t row,
  std::uint32_t value_bits) noexcept
{
  const std::uint64_t value = mix_(
    row * 0x9e3779b97f4a7c15ULL + value_bits +
    0x517cc1b727220a95ULL);
  state.fingerprint.first = mix_(state.fingerprint.first ^ value);
  state.fingerprint.second = mix_(state.fingerprint.second + value + 0x6eed0e9da4d94a4fULL);
}

std::map<unsigned long, std::uint32_t>
Application_::parse_row_(const std::string& line, const FeatureIndexes& feature_indexes)
{
  std::istringstream input(line);
  std::string token;
  if (!(input >> token))
  {
    return {};
  }

  std::map<unsigned long, std::uint32_t> result;
  while (input >> token)
  {
    const std::uint32_t index = parse_index_(token);
    if (feature_indexes.find(index) != feature_indexes.end())
    {
      result[index] = parse_value_bits_(token);
    }
  }
  return result;
}

Application_::FeatureStates
Application_::calculate_fingerprints_(
  const char* svm_file,
  const FeatureIndexes& feature_indexes,
  std::uint64_t& rows)
{
  std::ifstream input(svm_file);
  if (!input)
  {
    Stream::Error ostr;
    ostr << "Can't open LibSVM file '" << svm_file << "'";
    throw Exception(ostr);
  }

  FeatureStates states;
  for (const unsigned long index : feature_indexes)
  {
    states[index].fingerprint = {HASH_SEED_1, HASH_SEED_2};
  }

  rows = 0;
  std::string line;
  while (std::getline(input, line))
  {
    if (line.find_first_not_of(" \t\r") == std::string::npos)
    {
      continue;
    }
    const auto values = parse_row_(line, feature_indexes);
    for (const auto& item : values)
    {
      if (item.second != 0)
      {
        update_fingerprint_(states[item.first], rows, item.second);
      }
    }
    ++rows;
  }
  for (auto& item : states)
  {
    item.second.fingerprint.first = mix_(item.second.fingerprint.first ^ rows);
    item.second.fingerprint.second = mix_(item.second.fingerprint.second + rows);
  }
  return states;
}

Application_::FeatureIndexes
Application_::choose_dropped_features_(
  const FeatureIndexes& group,
  const FeatureIndexes& early_dropped_features)
{
  if (group.size() < 2)
  {
    return {};
  }

  FeatureIndexes non_early;
  for (const unsigned long index : group)
  {
    if (early_dropped_features.find(index) == early_dropped_features.end())
    {
      non_early.insert(index);
    }
  }

  const unsigned long canonical = non_early.empty() ? *group.begin() : *non_early.begin();
  FeatureIndexes result;
  for (const unsigned long index : group)
  {
    if (index != canonical)
    {
      result.insert(index);
    }
  }
  return result;
}

Application_::FeatureIndexes
Application_::verify_groups_and_choose_dropped_(
  const char* svm_file,
  const FeatureGroups& groups,
  const FeatureIndexes& early_dropped_features,
  std::uint64_t rows)
{
  std::ifstream input(svm_file);
  if (!input)
  {
    Stream::Error ostr;
    ostr << "Can't open LibSVM file '" << svm_file << "'";
    throw Exception(ostr);
  }

  FeatureIndexes dropped;
  std::map<unsigned long, unsigned long> canonical_by_member;
  FeatureIndexes verification_indexes;
  for (const auto& group_item : groups)
  {
    const FeatureIndexes group_dropped = choose_dropped_features_(
      group_item.second, early_dropped_features);
    unsigned long canonical = 0;
    for (const unsigned long index : group_item.second)
    {
      if (group_dropped.find(index) == group_dropped.end())
      {
        canonical = index;
        break;
      }
    }
    if (canonical == 0)
    {
      // choose_dropped_features_ always leaves one representative for a
      // duplicate group, but keep this guard if that policy changes later.
      continue;
    }
    for (const unsigned long index : group_item.second)
    {
      canonical_by_member[index] = canonical;
      verification_indexes.insert(index);
    }
    dropped.insert(group_dropped.begin(), group_dropped.end());
  }

  // Hash matches are only candidates. Re-read the dataset and confirm every
  // candidate against its selected representative, treating absent values as 0.
  std::map<unsigned long, bool> equal;
  for (const auto& item : canonical_by_member)
  {
    if (item.first != item.second)
    {
      equal[item.first] = true;
    }
  }
  std::uint64_t current_row = 0;
  std::string line;
  while (std::getline(input, line))
  {
    if (line.find_first_not_of(" \t\r") == std::string::npos)
    {
      continue;
    }
    const auto values = parse_row_(line, verification_indexes);
    for (auto& item : equal)
    {
      if (!item.second)
      {
        continue;
      }
      const auto member = values.find(item.first);
      const auto canonical = values.find(canonical_by_member[item.first]);
      const std::uint32_t member_value = member == values.end() ? 0 : member->second;
      const std::uint32_t canonical_value = canonical == values.end() ? 0 : canonical->second;
      if (member_value != canonical_value)
      {
        item.second = false;
        dropped.erase(item.first);
      }
    }
    ++current_row;
  }
  if (current_row != rows)
  {
    Stream::Error ostr;
    ostr << "LibSVM row count changed between passes: " << current_row << " != " << rows;
    throw Exception(ostr);
  }
  return dropped;
}

void
Application_::write_feature_indexes_(const char* file_name, const FeatureIndexes& indexes)
{
  std::ofstream output(file_name);
  if (!output)
  {
    Stream::Error ostr;
    ostr << "Can't open output feature indexes file '" << file_name << "'";
    throw Exception(ostr);
  }
  for (const unsigned long index : indexes)
  {
    output << index << '\n';
  }
}

void
Application_::write_dropped_features_(const char* file_name, const FeatureIndexes& indexes)
{
  if (file_name == nullptr || file_name[0] == '\0')
  {
    return;
  }
  write_feature_indexes_(file_name, indexes);
}

void
Application_::main(int& argc, char** argv)
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::StringOption opt_svm_file;
  Generics::AppUtils::StringOption opt_feature_indexes_file;
  Generics::AppUtils::StringOption opt_output_feature_indexes_file;
  Generics::AppUtils::StringOption opt_dropped_features_file;
  Generics::AppUtils::StringOption opt_early_dropped_features_file;

  Generics::AppUtils::Args args(-1);
  args.add(Generics::AppUtils::equal_name("help") || Generics::AppUtils::short_name("h"), opt_help);
  args.add(Generics::AppUtils::equal_name("svm-file"), opt_svm_file);
  args.add(Generics::AppUtils::equal_name("feature-indexes-file"), opt_feature_indexes_file);
  args.add(
    Generics::AppUtils::equal_name("output-feature-indexes-file"),
    opt_output_feature_indexes_file);
  args.add(Generics::AppUtils::equal_name("dropped-features-file"), opt_dropped_features_file);
  args.add(
    Generics::AppUtils::equal_name("early-dropped-features-file"),
    opt_early_dropped_features_file);
  args.parse(argc - 1, argv + 1);

  if (opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return;
  }
  if (!opt_svm_file.installed() || opt_svm_file->empty() ||
      !opt_feature_indexes_file.installed() ||
      opt_feature_indexes_file->empty())
  {
    Stream::Error ostr;
    ostr << USAGE;
    throw Exception(ostr);
  }

  const FeatureIndexes feature_indexes = read_feature_indexes_(opt_feature_indexes_file->c_str());
  const FeatureIndexes early_dropped_features =
    opt_early_dropped_features_file.installed() &&
    !opt_early_dropped_features_file->empty()
      ? read_feature_indexes_(opt_early_dropped_features_file->c_str())
      : FeatureIndexes();

  std::uint64_t rows = 0;
  const FeatureStates states = calculate_fingerprints_(
    opt_svm_file->c_str(), feature_indexes, rows);
  FeatureIndexes dropped;
  // With no observations there is no evidence that two features are equal;
  // preserve the input list rather than treating all empty sequences as one
  // duplicate group.
  if (rows != 0)
  {
    FeatureGroups groups;
    for (const auto& item : states)
    {
      groups[item.second.fingerprint].insert(item.first);
    }
    dropped = verify_groups_and_choose_dropped_(
      opt_svm_file->c_str(), groups, early_dropped_features, rows);
  }

  FeatureIndexes kept = feature_indexes;
  for (const unsigned long index : dropped)
  {
    kept.erase(index);
  }
  if (opt_output_feature_indexes_file.installed() && !opt_output_feature_indexes_file->empty())
  {
    write_feature_indexes_(opt_output_feature_indexes_file->c_str(), kept);
  }
  else
  {
    for (const unsigned long index : kept)
    {
      std::cout << index << '\n';
    }
  }
  write_dropped_features_(
    opt_dropped_features_file.installed() &&
    !opt_dropped_features_file->empty()
      ? opt_dropped_features_file->c_str() : nullptr,
    dropped);
}

int main(int argc, char** argv)
{
  Application_ app;
  try
  {
    app.main(argc, argv);
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
