#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include <sys/resource.h>

#include <google/protobuf/arena.h>

#include <Generics/AppUtils.hpp>

#include <UserInfoSvcs/UserBindServer/UserBindServerGrpc.pb.h>

namespace
{
  using GetUserIdResponse = adserver::user_info_svcs::user_bind::GetUserIdResponse;

  constexpr unsigned long DEFAULT_COUNT = 1000000;

  struct Options
  {
    unsigned long count = DEFAULT_COUNT;
  };

  struct CpuTimes
  {
    double user = 0.0;
    double sys = 0.0;
  };

  struct MeasureResult
  {
    CpuTimes cpu;
    std::uint64_t checksum = 0;
  };

  void
  print_usage()
  {
    std::cerr
      << "Usage: UserBindArenaFillTest [OPTIONS]\n"
      << "Options:\n"
      << "  --count <N>  GetUserIdResponse fill count (default: " << DEFAULT_COUNT << ")\n";
  }

  Options
  parse_options(int argc, char** argv)
  {
    using namespace Generics::AppUtils;

    Option<unsigned long> opt_count(DEFAULT_COUNT);
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("count"), opt_count);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      print_usage();
      std::exit(0);
    }

    Options options;
    options.count = *opt_count;

    if (options.count == 0)
    {
      throw std::runtime_error("--count must be > 0");
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

  CpuTimes
  diff_cpu_times(const CpuTimes& finish, const CpuTimes& start)
  {
    return {
      finish.user - start.user,
      finish.sys - start.sys
    };
  }

  std::string
  format_float(double value)
  {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
  }

  void
  fill_response(GetUserIdResponse& response, unsigned long index)
  {
    static constexpr char USER_ID[] = "0123456789abcdef";
    static constexpr char HOSTNAME[] = "user-bind-arena-fill-test";

    response.set_user_id(USER_ID, sizeof(USER_ID) - 1);
    response.set_min_age_reached((index & 1) == 0);
    response.set_created((index & 2) != 0);
    response.set_invalid_operation(false);
    response.set_user_found(true);
    response.set_hostname(HOSTNAME);
  }

  std::uint64_t
  response_checksum(const GetUserIdResponse& response)
  {
    return
      response.user_id().size() +
      response.hostname().size() +
      (response.min_age_reached() ? 1 : 0) +
      (response.created() ? 3 : 0) +
      (response.invalid_operation() ? 5 : 0) +
      (response.user_found() ? 7 : 0);
  }

  std::uint64_t
  run_without_arena(unsigned long count)
  {
    std::uint64_t checksum = 0;
    for (unsigned long i = 0; i < count; ++i)
    {
      auto response = std::make_unique<GetUserIdResponse>();
      fill_response(*response, i);
      checksum += response_checksum(*response);
    }

    return checksum;
  }

  std::uint64_t
  run_with_arena(unsigned long count)
  {
    google::protobuf::Arena arena;
    std::uint64_t checksum = 0;
    for (unsigned long i = 0; i < count; ++i)
    {
      auto* response = google::protobuf::Arena::CreateMessage<GetUserIdResponse>(&arena);
      fill_response(*response, i);
      checksum += response_checksum(*response);
    }

    return checksum;
  }

  template<typename Fun>
  MeasureResult
  measure(Fun&& fun)
  {
    const CpuTimes start = current_cpu_times();
    const std::uint64_t checksum = fun();
    const CpuTimes finish = current_cpu_times();

    return {
      diff_cpu_times(finish, start),
      checksum
    };
  }

  void
  print_result(const char* name, const MeasureResult& result)
  {
    const double total = result.cpu.user + result.cpu.sys;
    std::cout
      << name
      << "_cpu_sec=" << format_float(total)
      << "\n" << name
      << "_user_cpu_sec=" << format_float(result.cpu.user)
      << "\n" << name
      << "_sys_cpu_sec=" << format_float(result.cpu.sys)
      << "\n" << name << "_checksum=" << result.checksum << '\n';
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);

    std::cout << "count=" << options.count << '\n';

    const MeasureResult without_arena = measure(
      [&options] ()
      {
        return run_without_arena(options.count);
      });

    const MeasureResult with_arena = measure(
      [&options] ()
      {
        return run_with_arena(options.count);
      });

    print_result("without_arena", without_arena);
    print_result("with_arena", with_arena);

    return 0;
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
  }

  return 1;
}
