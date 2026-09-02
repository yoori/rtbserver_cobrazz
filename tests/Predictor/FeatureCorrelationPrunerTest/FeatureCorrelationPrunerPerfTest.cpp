#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>

#include <sys/resource.h>

#include <Utils/Predictor/FeatureCorrelationPruner/CorrelationPruner.hpp>

namespace
{
  using Pruner = AdServer::Predictor::FeatureCorrelationPruner;

  struct Options
  {
    std::uint64_t rows = 1000000;
    std::uint32_t features = 300;
    std::uint32_t active_features = 297;
    double correlation_threshold = 0.98;
  };

  std::uint64_t
  parse_unsigned(const char* value, const char* option)
  {
    std::uint64_t result = 0;
    const std::string_view text(value);
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() || result == 0)
    {
      throw std::runtime_error(std::string("Invalid ") + option + " value");
    }
    return result;
  }

  double
  parse_double(const char* value, const char* option)
  {
    char* end = nullptr;
    const double result = std::strtod(value, &end);
    if (end == value || *end != '\0')
    {
      throw std::runtime_error(std::string("Invalid ") + option + " value");
    }
    return result;
  }

  void
  print_usage()
  {
    std::cout
      << "Usage: FeatureCorrelationPrunerPerfTest [OPTIONS]\n"
      << "  --rows N                 generated rows (default: 1000000)\n"
      << "  --features N             candidate features (default: 300)\n"
      << "  --active-features N      regular active features per row (default: 297)\n"
      << "  --correlation-threshold  absolute Pearson threshold (default: 0.98)\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    Options result;
    for (int index = 1; index < argc; ++index)
    {
      const std::string_view option(argv[index]);
      if (option == "--help" || option == "-h")
      {
        print_usage();
        std::exit(0);
      }

      if (++index == argc)
      {
        throw std::runtime_error(std::string(option) + " requires a value");
      }
      if (option == "--rows")
      {
        result.rows = parse_unsigned(argv[index], "--rows");
      }
      else if (option == "--features")
      {
        result.features = parse_unsigned(argv[index], "--features");
      }
      else if (option == "--active-features")
      {
        result.active_features = parse_unsigned(argv[index], "--active-features");
      }
      else if (option == "--correlation-threshold")
      {
        result.correlation_threshold = parse_double(argv[index], option.data());
      }
      else
      {
        throw std::runtime_error("Unexpected option '" + std::string(option) + "'");
      }
    }

    if (result.features < 3)
    {
      throw std::runtime_error("--features must be at least 3");
    }
    if (result.active_features == 0 || result.active_features > result.features - 2)
    {
      throw std::runtime_error("--active-features must be in [1, features - 2]");
    }
    return result;
  }

  class GeneratedSvmBuffer final: public std::streambuf
  {
  public:
    explicit GeneratedSvmBuffer(const Options& options)
      : options_(options)
    {
      line_.reserve(options.active_features * 10 + 32);
    }

  protected:
    int_type underflow() override
    {
      if (gptr() != nullptr && gptr() < egptr())
      {
        return traits_type::to_int_type(*gptr());
      }
      if (row_ == options_.rows)
      {
        return traits_type::eof();
      }

      line_.clear();
      line_ = "0";
      const std::uint32_t regular_features = options_.features - 2;
      const std::uint32_t start = (row_ * 17) % regular_features;
      for (std::uint32_t offset = 0; offset < options_.active_features; ++offset)
      {
        append_feature(1 + (start + offset * 37) % regular_features);
      }

      const bool base = row_ % 2 == 0;
      const bool flip = row_ % 400 == 0 || row_ % 400 == 1;
      if (base)
      {
        append_feature(options_.features - 1);
      }
      if (base != flip)
      {
        append_feature(options_.features);
      }
      line_ += '\n';
      ++row_;
      setg(line_.data(), line_.data(), line_.data() + line_.size());
      return traits_type::to_int_type(*gptr());
    }

  private:
    void append_feature(std::uint32_t feature)
    {
      char buffer[32];
      const auto converted = std::to_chars(buffer, buffer + sizeof(buffer), feature);
      line_ += ' ';
      line_.append(buffer, converted.ptr);
      line_ += ":1";
    }

    Options options_;
    std::uint64_t row_ = 0;
    std::string line_;
  };

  const Pruner::Drop*
  find_drop(const Pruner::Result& result, Pruner::FeatureIndex index)
  {
    for (const auto& drop : result.drops)
    {
      if (drop.index == index)
      {
        return &drop;
      }
    }
    return nullptr;
  }
}

int main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    Pruner::FeatureIndexes feature_indexes;
    for (std::uint32_t index = 1; index <= options.features; ++index)
    {
      feature_indexes.insert(index);
    }

    Pruner pruner(std::move(feature_indexes), options.correlation_threshold);
    GeneratedSvmBuffer buffer(options);
    std::istream input(&buffer);
    const auto started = std::chrono::steady_clock::now();
    pruner.add_svm(input, "generated.libsvm");
    const Pruner::Result result = pruner.result();
    const double elapsed = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - started).count();

    if (result.rows != options.rows)
    {
      throw std::runtime_error("Processed row count mismatch");
    }
    const Pruner::Drop* correlated = find_drop(result, options.features);
    if (correlated == nullptr || correlated->representative != options.features - 1)
    {
      throw std::runtime_error("Expected correlated feature pair was not pruned");
    }

    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
      throw std::runtime_error("getrusage failed");
    }
    std::cout
      << std::fixed << std::setprecision(6)
      << "rows=" << options.rows << '\n'
      << "features=" << options.features << '\n'
      << "regular_active_features_per_row=" << options.active_features << '\n'
      << "elapsed_sec=" << elapsed << '\n'
      << "rows_per_sec=" << static_cast<double>(options.rows) / elapsed << '\n'
      << "peak_rss_bytes=" << static_cast<std::uint64_t>(usage.ru_maxrss) * 1024 << '\n'
      << "kept_features=" << result.kept.size() << '\n'
      << "dropped_features=" << result.dropped.size() << '\n'
      << "expected_pair_correlation=" << correlated->correlation << std::endl;
    return 0;
  }
  catch(const std::exception& exception)
  {
    std::cerr << "FeatureCorrelationPrunerPerfTest failed: " << exception.what() << std::endl;
  }
  return 1;
}
