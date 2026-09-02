#include "CorrelationPruner.hpp"

#include <charconv>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <Generics/AppUtils.hpp>
#include <Stream/MemoryStream.hpp>

namespace
{
  const char USAGE[] =
    "Usage: FeatureCorrelationPruner --svm-file FILE [--svm-file FILE ...] "
    "--feature-indexes-file FILE --correlation-threshold VALUE "
    "[--output-feature-indexes-file FILE] [--dropped-features-file FILE] "
    "[--correlations-file FILE]";

  using Pruner = AdServer::Predictor::FeatureCorrelationPruner;

  Pruner::FeatureIndexes
  read_feature_indexes(const char* file_name)
  {
    std::ifstream input(file_name);
    if (!input)
    {
      Stream::Error error;
      error << "Can't open feature indexes file '" << file_name << "'";
      throw Pruner::Exception(error);
    }

    Pruner::FeatureIndexes result;
    std::string line;
    std::uint64_t line_number = 0;
    while (std::getline(input, line))
    {
      ++line_number;
      const std::size_t first = line.find_first_not_of(" \t\r");
      if (first == std::string::npos || line[first] == '#')
      {
        continue;
      }

      const std::size_t last = line.find_last_not_of(" \t\r");
      const std::string_view value(line.data() + first, last - first + 1);
      Pruner::FeatureIndex index = 0;
      const auto parsed = std::from_chars(value.data(), value.data() + value.size(), index);
      if (
        parsed.ec != std::errc() ||
        parsed.ptr != value.data() + value.size() ||
        index == 0)
      {
        Stream::Error error;
        error << file_name << ':' << line_number << ": invalid feature index '" << value << "'";
        throw Pruner::Exception(error);
      }
      result.insert(index);
    }
    return result;
  }

  void
  write_feature_indexes(const char* file_name, const Pruner::FeatureIndexes& indexes)
  {
    std::ofstream output(file_name);
    if (!output)
    {
      Stream::Error error;
      error << "Can't open output feature indexes file '" << file_name << "'";
      throw Pruner::Exception(error);
    }
    for (const auto index : indexes)
    {
      output << index << '\n';
    }
  }

  void
  write_correlations(const char* file_name, const std::vector<Pruner::Drop>& drops)
  {
    std::ofstream output(file_name);
    if (!output)
    {
      Stream::Error error;
      error << "Can't open correlations file '" << file_name << "'";
      throw Pruner::Exception(error);
    }
    output << "dropped_index,representative_index,correlation\n" << std::setprecision(17);
    for (const auto& drop : drops)
    {
      output << drop.index << ',' << drop.representative << ',' << drop.correlation << '\n';
    }
  }
}

int main(int argc, char** argv)
{
  try
  {
    Generics::AppUtils::CheckOption opt_help;
    Generics::AppUtils::OptionsSet<std::vector<std::string>> opt_svm_files;
    Generics::AppUtils::StringOption opt_feature_indexes_file;
    Generics::AppUtils::Option<double> opt_correlation_threshold;
    Generics::AppUtils::StringOption opt_output_feature_indexes_file;
    Generics::AppUtils::StringOption opt_dropped_features_file;
    Generics::AppUtils::StringOption opt_correlations_file;

    Generics::AppUtils::Args args(-1);
    args.add(
      Generics::AppUtils::equal_name("help") || Generics::AppUtils::short_name("h"),
      opt_help);
    args.add(Generics::AppUtils::equal_name("svm-file"), opt_svm_files);
    args.add(
      Generics::AppUtils::equal_name("feature-indexes-file"),
      opt_feature_indexes_file);
    args.add(
      Generics::AppUtils::equal_name("correlation-threshold"),
      opt_correlation_threshold);
    args.add(
      Generics::AppUtils::equal_name("output-feature-indexes-file"),
      opt_output_feature_indexes_file);
    args.add(
      Generics::AppUtils::equal_name("dropped-features-file"),
      opt_dropped_features_file);
    args.add(Generics::AppUtils::equal_name("correlations-file"), opt_correlations_file);
    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    if (
      !opt_svm_files.installed() ||
      opt_svm_files->empty() ||
      !opt_feature_indexes_file.installed() ||
      opt_feature_indexes_file->empty() ||
      !opt_correlation_threshold.installed())
    {
      Stream::Error error;
      error << USAGE;
      throw Pruner::Exception(error);
    }

    Pruner pruner(
      read_feature_indexes(opt_feature_indexes_file->c_str()),
      *opt_correlation_threshold);
    for (const std::string& file_name : *opt_svm_files)
    {
      std::ifstream input(file_name);
      if (!input)
      {
        Stream::Error error;
        error << "Can't open LibSVM file '" << file_name << "'";
        throw Pruner::Exception(error);
      }
      pruner.add_svm(input, file_name);
    }

    const Pruner::Result result = pruner.result();
    if (opt_output_feature_indexes_file.installed())
    {
      write_feature_indexes(opt_output_feature_indexes_file->c_str(), result.kept);
    }
    else
    {
      for (const auto index : result.kept)
      {
        std::cout << index << '\n';
      }
    }

    if (opt_dropped_features_file.installed())
    {
      write_feature_indexes(opt_dropped_features_file->c_str(), result.dropped);
    }

    if (opt_correlations_file.installed())
    {
      write_correlations(opt_correlations_file->c_str(), result.drops);
    }

    std::cerr
      << "Feature correlation pruning processed " << result.rows << " rows, dropped "
      << result.dropped.size() << " indexes, kept " << result.kept.size() << std::endl;
    return 0;
  }
  catch(const std::exception& exception)
  {
    std::cerr << exception.what() << std::endl;
  }
  return 1;
}
