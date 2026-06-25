#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include <Commons/Gason.hpp>

#include "../JsonParserPerfUtils.hpp"

namespace
{
  struct Options
  {
    std::uint64_t count = 1000000;
    std::string request_file;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: GasonPerfTest [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>         parse calls count (default: 1000000)\n"
      << "  --request-file <P>  file with OpenRTB request body\n"
      << "  --help, -h          print this help\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    Options options;

    for(int i = 1; i < argc; ++i)
    {
      const std::string arg(argv[i]);
      if(arg == "--help" || arg == "-h")
      {
        print_usage();
        std::exit(0);
      }
      else if(arg == "--count")
      {
        if(++i == argc)
        {
          throw std::runtime_error("--count requires value");
        }
        options.count = AdServer::Tests::Frontends::parse_uint64(
          argv[i],
          "--count");
      }
      else if(arg == "--request-file")
      {
        if(++i == argc)
        {
          throw std::runtime_error("--request-file requires value");
        }
        options.request_file = argv[i];
      }
      else
      {
        throw std::runtime_error("unexpected option '" + arg + "'");
      }
    }

    return options;
  }
}

extern "C"
{
  __attribute__((noinline))
  std::uint64_t
  gason_run_benchmark(
    std::uint64_t count,
    const std::string& request_body)
  {
    std::uint64_t checksum = 0;
    std::string buffer;
    buffer.resize(request_body.size() + 1);

    for(std::uint64_t i = 0; i < count; ++i)
    {
      std::memcpy(buffer.data(), request_body.data(), request_body.size());
      buffer[request_body.size()] = '\0';

      JsonAllocator allocator;
      JsonValue value;
      char* endptr = nullptr;
      const JsonParseStatus status = json_parse(
        buffer.data(),
        &endptr,
        &value,
        allocator);
      if(status != JSON_PARSE_OK)
      {
        throw std::runtime_error(json_parse_error(status));
      }

      checksum += value.getTag();
      checksum += static_cast<std::uint64_t>(endptr - buffer.data());
    }

    return checksum;
  }
}

int
main(int argc, char** argv)
{
  try
  {
    using namespace AdServer::Tests::Frontends;

    const Options options = parse_options(argc, argv);
    const std::string request_body = read_request_body(options.request_file);

    const auto started_at = std::chrono::steady_clock::now();
    const CpuTimes cpu_started = current_cpu_times();

    const std::uint64_t checksum = gason_run_benchmark(
      options.count,
      request_body);

    const CpuTimes cpu_finished = current_cpu_times();
    const auto finished_at = std::chrono::steady_clock::now();

    const double elapsed = std::chrono::duration<double>(
      finished_at - started_at).count();
    const double user_cpu = cpu_finished.user - cpu_started.user;
    const double sys_cpu = cpu_finished.sys - cpu_started.sys;
    const double rate = static_cast<double>(options.count) / elapsed;

    std::cout
      << "count=" << options.count << '\n'
      << "elapsed_sec=" << format_float(elapsed) << '\n'
      << "rate_per_sec=" << format_float(rate) << '\n'
      << "user_cpu_sec=" << format_float(user_cpu) << '\n'
      << "sys_cpu_sec=" << format_float(sys_cpu) << '\n'
      << "checksum=" << checksum << '\n';

    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "std::exception: " << ex.what() << std::endl;
  }

  return 1;
}
