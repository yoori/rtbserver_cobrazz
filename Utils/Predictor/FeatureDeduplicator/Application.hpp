#pragma once

#include <cstdint>
#include <map>
#include <set>
#include <string>

#include <eh/Exception.hpp>

class Application_
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  Application_() noexcept = default;
  ~Application_() noexcept = default;

  void main(int& argc, char** argv);

private:
  struct Fingerprint
  {
    std::uint64_t first = 0;
    std::uint64_t second = 0;

    bool operator<(const Fingerprint& right) const noexcept
    {
      return first < right.first || (first == right.first && second < right.second);
    }
  };

  struct FeatureState
  {
    Fingerprint fingerprint;
  };

  using FeatureIndexes = std::set<unsigned long>;
  using FeatureStates = std::map<unsigned long, FeatureState>;
  using FeatureGroups = std::map<Fingerprint, FeatureIndexes>;

  static FeatureIndexes read_feature_indexes_(const char* file_name);

  static std::uint32_t parse_index_(const std::string& token);

  static std::uint32_t parse_value_bits_(const std::string& token);

  static std::uint64_t mix_(std::uint64_t value) noexcept;

  static void update_fingerprint_(
    FeatureState& state,
    std::uint64_t row,
    std::uint32_t value_bits) noexcept;

  static std::map<unsigned long, std::uint32_t> parse_row_(
    const std::string& line,
    const FeatureIndexes& feature_indexes);

  static FeatureStates calculate_fingerprints_(
    const char* svm_file,
    const FeatureIndexes& feature_indexes,
    std::uint64_t& rows);

  static FeatureIndexes choose_dropped_features_(
    const FeatureIndexes& group,
    const FeatureIndexes& early_dropped_features);

  static FeatureIndexes verify_groups_and_choose_dropped_(
    const char* svm_file,
    const FeatureGroups& groups,
    const FeatureIndexes& early_dropped_features,
    std::uint64_t rows);

  static void write_feature_indexes_(const char* file_name, const FeatureIndexes& indexes);

  static void write_dropped_features_(const char* file_name, const FeatureIndexes& indexes);
};
