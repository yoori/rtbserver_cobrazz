#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <sys/resource.h>

#include <boost/unordered/unordered_flat_map.hpp>

#include <Generics/AppUtils.hpp>

namespace
{
  struct Options
  {
    unsigned long count = 100000;
    unsigned long items = 1024;
    unsigned long miss_percent = 0;
    std::string key_type = "uint64";
  };

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  struct MeasureResult
  {
    double elapsed = 0.0;
    double user_cpu = 0.0;
    double sys_cpu = 0.0;
    std::uint64_t checksum = 0;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: BiddingFrontendHashMapPerfTest [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>          iterations count (default: 100000)\n"
      << "  --items <N>          keys per iteration (default: 1024)\n"
      << "  --miss-percent <N>   lookup misses percent, 0..100 (default: 0)\n"
      << "  --key-type <TYPE>    uint64|string (default: uint64)\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> opt_count(100000);
    Option<unsigned long> opt_items(1024);
    Option<unsigned long> opt_miss_percent(0);
    StringOption opt_key_type("uint64");
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("items"), opt_items);
    args.add(equal_name("miss-percent"), opt_miss_percent);
    args.add(equal_name("key-type"), opt_key_type);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.count = *opt_count;
    options.items = *opt_items;
    options.miss_percent = *opt_miss_percent;
    options.key_type = *opt_key_type;

    if (options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }

    if (options.items == 0)
    {
      throw std::runtime_error("--items must be > 0");
    }

    if (options.miss_percent > 100)
    {
      throw std::runtime_error("--miss-percent must be <= 100");
    }

    if (options.key_type != "uint64" && options.key_type != "string")
    {
      throw std::runtime_error("--key-type must be uint64 or string");
    }

    return options;
  }

  CpuTimes
  current_cpu_times()
  {
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
      throw std::runtime_error("getrusage failed");
    }

    return {
      usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0,
      usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0
    };
  }

  std::string
  format_float(double value)
  {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
  }

  std::vector<std::uint64_t>
  make_uint64_keys(std::size_t count)
  {
    std::vector<std::uint64_t> keys;
    keys.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
      keys.emplace_back(i * 2654435761ULL + 17);
    }

    return keys;
  }

  std::vector<std::string>
  make_string_keys(std::size_t count)
  {
    std::vector<std::string> keys;
    keys.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
      keys.emplace_back("bidding_frontend_key/" + std::to_string(i * 2654435761ULL + 17));
    }

    return keys;
  }

  std::vector<std::uint64_t>
  make_uint64_lookup_keys(const std::vector<std::uint64_t>& keys, unsigned long miss_percent)
  {
    std::vector<std::uint64_t> lookup_keys;
    lookup_keys.reserve(keys.size());

    for (std::size_t i = 0; i < keys.size(); ++i)
    {
      if (miss_percent != 0 && (i * 100 / keys.size()) < miss_percent)
      {
        lookup_keys.emplace_back(keys[i] ^ 0x8000000000000000ULL);
      }
      else
      {
        lookup_keys.emplace_back(keys[i]);
      }
    }

    return lookup_keys;
  }

  std::vector<std::string>
  make_string_lookup_keys(const std::vector<std::string>& keys, unsigned long miss_percent)
  {
    std::vector<std::string> lookup_keys;
    lookup_keys.reserve(keys.size());

    for (std::size_t i = 0; i < keys.size(); ++i)
    {
      if (miss_percent != 0 && (i * 100 / keys.size()) < miss_percent)
      {
        lookup_keys.emplace_back(keys[i] + "/miss");
      }
      else
      {
        lookup_keys.emplace_back(keys[i]);
      }
    }

    return lookup_keys;
  }

  template<typename Fun>
  MeasureResult
  measure(Fun fun)
  {
    const auto cpu_start = current_cpu_times();
    const auto started = std::chrono::steady_clock::now();
    const std::uint64_t checksum = fun();
    const auto finished = std::chrono::steady_clock::now();
    const auto cpu_finish = current_cpu_times();

    return {
      std::chrono::duration<double>(finished - started).count(),
      cpu_finish.user - cpu_start.user,
      cpu_finish.sys - cpu_start.sys,
      checksum
    };
  }

  template<typename Map>
  MeasureResult
  measure_lookup(
    const std::vector<typename Map::key_type>& keys,
    const std::vector<typename Map::key_type>& lookup_keys,
    unsigned long count)
  {
    return measure([&]() {
      Map map;
      map.reserve(keys.size());
      for (std::size_t i = 0; i < keys.size(); ++i)
      {
        map.emplace(keys[i], i + 1);
      }

      std::uint64_t checksum = 0;
      for (unsigned long i = 0; i < count; ++i)
      {
        for (const auto& key : lookup_keys)
        {
          const auto it = map.find(key);
          checksum += it != map.end() ? it->second : 1;
        }
      }

      return checksum;
    });
  }

  template<typename Map>
  MeasureResult
  measure_update_or_insert(const std::vector<typename Map::key_type>& keys, unsigned long count)
  {
    return measure([&]() {
      Map map;
      map.reserve(keys.size());

      std::uint64_t checksum = 0;
      for (unsigned long i = 0; i < count; ++i)
      {
        for (const auto& key : keys)
        {
          auto it = map.find(key);
          if (it == map.end())
          {
            it = map.emplace(key, 0).first;
          }

          checksum += ++it->second;
        }
      }

      return checksum + map.size();
    });
  }

  void
  print_result(
    const char* name,
    const MeasureResult& result,
    unsigned long count,
    unsigned long items)
  {
    const double cpu = result.user_cpu + result.sys_cpu;
    const double ops = static_cast<double>(count) * items;

    std::cout
      << name << ":\n"
      << "  elapsed_sec=" << format_float(result.elapsed) << '\n'
      << "  cpu_sec=" << format_float(cpu) << '\n'
      << "  user_cpu_sec=" << format_float(result.user_cpu) << '\n'
      << "  sys_cpu_sec=" << format_float(result.sys_cpu) << '\n'
      << "  mops_per_cpu_sec=" << format_float(ops / cpu / 1000000.0) << '\n'
      << "  checksum=" << result.checksum << '\n';
  }

  template<typename Key>
  void
  run_tests(
    const Options& options,
    const std::vector<Key>& keys,
    const std::vector<Key>& lookup_keys)
  {
    using StdMap = std::unordered_map<Key, std::uint64_t>;
    using BoostFlatMap = boost::unordered_flat_map<Key, std::uint64_t>;

    print_result(
      "std_unordered_map_lookup",
      measure_lookup<StdMap>(keys, lookup_keys, options.count),
      options.count,
      options.items);
    print_result(
      "boost_unordered_flat_map_lookup",
      measure_lookup<BoostFlatMap>(keys, lookup_keys, options.count),
      options.count,
      options.items);
    print_result(
      "std_unordered_map_update_or_insert",
      measure_update_or_insert<StdMap>(keys, options.count),
      options.count,
      options.items);
    print_result(
      "boost_unordered_flat_map_update_or_insert",
      measure_update_or_insert<BoostFlatMap>(keys, options.count),
      options.count,
      options.items);
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);

    std::cout
      << "count=" << options.count << '\n'
      << "items=" << options.items << '\n'
      << "miss_percent=" << options.miss_percent << '\n' << "key_type=" << options.key_type << '\n';

    if (options.key_type == "string")
    {
      const auto keys = make_string_keys(options.items);
      const auto lookup_keys = make_string_lookup_keys(keys, options.miss_percent);
      run_tests(options, keys, lookup_keys);
    }
    else
    {
      const auto keys = make_uint64_keys(options.items);
      const auto lookup_keys = make_uint64_lookup_keys(keys, options.miss_percent);
      run_tests(options, keys, lookup_keys);
    }

    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Caught std::exception: " << ex.what() << '\n';
  }

  return 1;
}
