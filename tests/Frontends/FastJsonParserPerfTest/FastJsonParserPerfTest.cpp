#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include <Commons/FastJsonParser.hpp>

#include "../JsonParserPerfUtils.hpp"

namespace
{
  struct Options
  {
    std::uint64_t count = 1000000;
    bool non_strict = false;
    AdServer::Commons::FastJsonParserSimdLevel simd_level =
      AdServer::Commons::FastJsonParserSimdLevel::AUTO;
    std::string request_file;
  };

  const char*
  simd_level_name(AdServer::Commons::FastJsonParserSimdLevel level) noexcept
  {
    switch(level)
    {
    case AdServer::Commons::FastJsonParserSimdLevel::AUTO:
      return "auto";
    case AdServer::Commons::FastJsonParserSimdLevel::SCALAR:
      return "scalar";
    case AdServer::Commons::FastJsonParserSimdLevel::SSE2:
      return "sse2";
    case AdServer::Commons::FastJsonParserSimdLevel::AVX2:
      return "avx2";
    case AdServer::Commons::FastJsonParserSimdLevel::AVX512BW:
      return "avx512bw";
    }

    return "unknown";
  }

  AdServer::Commons::FastJsonParserSimdLevel
  parse_simd_level(const std::string& value)
  {
    if(value == "auto")
    {
      return AdServer::Commons::FastJsonParserSimdLevel::AUTO;
    }
    if(value == "scalar")
    {
      return AdServer::Commons::FastJsonParserSimdLevel::SCALAR;
    }
    if(value == "sse2")
    {
      return AdServer::Commons::FastJsonParserSimdLevel::SSE2;
    }
    if(value == "avx2")
    {
      return AdServer::Commons::FastJsonParserSimdLevel::AVX2;
    }
    if(value == "avx512" || value == "avx512bw")
    {
      return AdServer::Commons::FastJsonParserSimdLevel::AVX512BW;
    }

    throw std::runtime_error("unexpected --simd value '" + value + "'");
  }

  void
  print_usage()
  {
    std::cerr
      << "Usage: FastJsonParserPerfTest [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>         parse calls count (default: 1000000)\n"
      << "  --request-file <P>  file with OpenRTB request body\n"
      << "  --simd <MODE>       auto|scalar|sse2|avx2|avx512bw (default: auto)\n"
      << "  --non-strict        use non-strict skipped subtree parsing\n"
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
      else if(arg == "--non-strict")
      {
        options.non_strict = true;
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
      else if(arg == "--simd")
      {
        if(++i == argc)
        {
          throw std::runtime_error("--simd requires value");
        }
        options.simd_level = parse_simd_level(argv[i]);
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
  fast_json_parser_run_benchmark(
    const AdServer::Commons::FastJsonParser<>& parser,
    std::uint64_t count,
    const std::string& request_body)
  {
    std::uint64_t checksum = 0;

    for(std::uint64_t i = 0; i < count; ++i)
    {
      parser.parse(request_body, nullptr);
      checksum += request_body.size();
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
    const AdServer::Commons::FastJsonParser<> parser(
      !options.non_strict,
      options.simd_level);

    const auto started_at = std::chrono::steady_clock::now();
    const CpuTimes cpu_started = current_cpu_times();

    const std::uint64_t checksum = fast_json_parser_run_benchmark(
      parser,
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
      << "strict=" << (options.non_strict ? "false" : "true") << '\n'
      << "simd=" << simd_level_name(parser.simd_level()) << '\n'
      << "available_simd=" << simd_level_name(
        AdServer::Commons::FastJsonParser<>::available_simd_level()) << '\n'
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
