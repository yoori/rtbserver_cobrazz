#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <memory_resource>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include <sys/resource.h>

#include <Generics/AppUtils.hpp>

namespace
{
  struct Options
  {
    unsigned long count = 100000;
    unsigned long items = 500;
    std::size_t arena_initial_size = 64 * 1024;
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
    std::size_t checksum = 0;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: PmrTests [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>                iterations count (default: 100000)\n"
      << "  --items <N>                ids per unordered_set (default: 500)\n"
      << "  --arena-initial-size <N>   monotonic_buffer_resource initial size (default: 65536)\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> opt_count(100000);
    Option<unsigned long> opt_items(500);
    Option<unsigned long> opt_arena_initial_size(64 * 1024);
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("items"), opt_items);
    args.add(equal_name("arena-initial-size"), opt_arena_initial_size);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if(opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.count = *opt_count;
    options.items = *opt_items;
    options.arena_initial_size = *opt_arena_initial_size;

    if(options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
    }

    if(options.items == 0)
    {
      throw std::runtime_error("--items must be > 0");
    }

    if(options.arena_initial_size == 0)
    {
      throw std::runtime_error("--arena-initial-size must be > 0");
    }

    return options;
  }

  CpuTimes
  current_cpu_times()
  {
    rusage usage{};
    if(getrusage(RUSAGE_SELF, &usage) != 0)
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

  std::vector<unsigned long>
  make_ids(std::size_t count)
  {
    std::vector<unsigned long> ids;
    ids.reserve(count);
    for(std::size_t i = 0; i < count; ++i)
    {
      ids.push_back(static_cast<unsigned long>(i * 2654435761UL + 17));
    }
    return ids;
  }

  template<typename Fun>
  MeasureResult
  measure(Fun fun)
  {
    const auto cpu_start = current_cpu_times();
    const auto started = std::chrono::steady_clock::now();
    const std::size_t checksum = fun();
    const auto finished = std::chrono::steady_clock::now();
    const auto cpu_finish = current_cpu_times();

    return {
      std::chrono::duration<double>(finished - started).count(),
      cpu_finish.user - cpu_start.user,
      cpu_finish.sys - cpu_start.sys,
      checksum
    };
  }

  MeasureResult
  measure_pmr_unordered_set(
    const std::vector<unsigned long>& ids,
    unsigned long count,
    std::size_t arena_initial_size)
  {
    return measure([&]() {
      std::size_t checksum = 0;

      for(unsigned long i = 0; i < count; ++i)
      {
        std::pmr::monotonic_buffer_resource arena(arena_initial_size);
        std::pmr::unordered_set<unsigned long> values(&arena);
        values.reserve(ids.size());
        for(const auto id : ids)
        {
          values.emplace(id);
        }
        checksum += values.size();
      }

      return checksum;
    });
  }

  MeasureResult
  measure_std_unordered_set(
    const std::vector<unsigned long>& ids,
    unsigned long count)
  {
    return measure([&]() {
      std::size_t checksum = 0;

      for(unsigned long i = 0; i < count; ++i)
      {
        std::unordered_set<unsigned long> values;
        values.reserve(ids.size());
        for(const auto id : ids)
        {
          values.emplace(id);
        }
        checksum += values.size();
      }

      return checksum;
    });
  }

  void
  print_result(const char* name, const MeasureResult& result)
  {
    const double cpu = result.user_cpu + result.sys_cpu;

    std::cout
      << name << ":\n"
      << "  elapsed_sec=" << format_float(result.elapsed) << '\n'
      << "  cpu_sec=" << format_float(cpu) << '\n'
      << "  user_cpu_sec=" << format_float(result.user_cpu) << '\n'
      << "  sys_cpu_sec=" << format_float(result.sys_cpu) << '\n'
      << "  checksum=" << result.checksum << '\n';
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);
    const auto ids = make_ids(options.items);

    std::cout
      << "count=" << options.count << '\n'
      << "items=" << options.items << '\n'
      << "arena_initial_size=" << options.arena_initial_size << '\n';

    const auto pmr_result = measure_pmr_unordered_set(
      ids,
      options.count,
      options.arena_initial_size);
    const auto std_result = measure_std_unordered_set(ids, options.count);

    print_result("pmr_unordered_set", pmr_result);
    print_result("std_unordered_set", std_result);

    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Caught std::exception: " << ex.what() << '\n';
  }

  return 1;
}
