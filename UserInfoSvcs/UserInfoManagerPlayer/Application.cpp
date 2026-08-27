#include <cstdint>
#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <grpc/impl/channel_arg_names.h>
#include <grpcpp/grpcpp.h>

#include <Generics/AppUtils.hpp>

#include <Commons/Grpc/GrpcClient.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoManagerGrpc.grpc.pb.h>

#include "UserInfoManagerDump.hpp"

namespace
{
  namespace uim = adserver::user_info_svcs::user_info_manager;
  namespace dump = AdServer::UserInfoSvcs::UserInfoManagerDump;

  int usage()
  {
    std::cerr <<
      "usage: UserInfoManagerPlayer play --send-endpoint=<host:port> --dump=<path>\n"
      "       UserInfoManagerPlayer print --dump=<path>\n";
    return 2;
  }

  enum class Command
  {
    Play,
    Print
  };

  struct Config
  {
    Command command = Command::Play;
    std::string send_endpoint;
    std::string dump_path;
  };

  bool parse_args(int argc, char** argv, Config& config)
  {
    Generics::AppUtils::CheckOption opt_help;
    Generics::AppUtils::StringOption opt_send_endpoint;
    Generics::AppUtils::StringOption opt_dump;
    Generics::AppUtils::Args args(-1);

    args.add(
      Generics::AppUtils::equal_name("help") ||
      Generics::AppUtils::short_name("h"),
      opt_help);
    args.add(Generics::AppUtils::equal_name("send-endpoint"), opt_send_endpoint);
    args.add(Generics::AppUtils::equal_name("dump"), opt_dump);
    args.parse(argc - 1, argv + 1);

    auto commands = args.commands();
    if (opt_help.enabled() || commands.size() != 1 || !opt_dump.installed())
    {
      return false;
    }

    if (commands.front() == "play")
    {
      if (!opt_send_endpoint.installed())
      {
        return false;
      }

      config.command = Command::Play;
      config.send_endpoint = *opt_send_endpoint;
    }
    else if (commands.front() == "print")
    {
      if (opt_send_endpoint.installed())
      {
        return false;
      }

      config.command = Command::Print;
    }
    else
    {
      return false;
    }

    config.dump_path = *opt_dump;

    return true;
  }

  std::string status_description(const grpc::Status& status)
  {
    std::string result = status.error_message();
    if (result.empty())
    {
      result = std::to_string(static_cast<int>(status.error_code()));
    }

    if (!status.error_details().empty())
    {
      result += ": ";
      result += status.error_details();
    }
    return result;
  }

  template<typename Request, typename Response, typename Caller>
  grpc::Status call_(const std::string& request_bytes, Caller caller)
  {
    Request request;
    if (!request.ParseFromString(request_bytes))
    {
      return grpc::Status(grpc::INVALID_ARGUMENT, "failed to parse request");
    }

    Response response;
    grpc::ClientContext context;
    return caller(&context, request, &response);
  }

  grpc::Status replay_record(uim::UserInfoManagerGrpc::Stub* stub, const dump::DumpRecord& record)
  {
    if (record.method == "get_source")
    {
      return call_<uim::GetSourceRequest, uim::GetSourceResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->get_source(ctx, req, resp);
        });
    }

    if (record.method == "get_master_stamp")
    {
      return call_<uim::GetMasterStampRequest, uim::GetMasterStampResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->get_master_stamp(ctx, req, resp);
        });
    }

    if (record.method == "get_user_profile")
    {
      return call_<uim::GetUserProfileRequest, uim::GetUserProfileResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->get_user_profile(ctx, req, resp);
        });
    }

    if (record.method == "match")
    {
      return call_<uim::MatchRequest, uim::MatchResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->match(ctx, req, resp);
        });
    }

    if (record.method == "update_user_freq_caps")
    {
      return call_<uim::UpdateUserFreqCapsRequest, uim::UpdateUserFreqCapsResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->update_user_freq_caps(ctx, req, resp);
        });
    }

    if (record.method == "confirm_user_freq_caps")
    {
      return call_<uim::ConfirmUserFreqCapsRequest, uim::ConfirmUserFreqCapsResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->confirm_user_freq_caps(ctx, req, resp);
        });
    }

    if (record.method == "fraud_user")
    {
      return call_<uim::FraudUserRequest, uim::FraudUserResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->fraud_user(ctx, req, resp);
        });
    }

    if (record.method == "remove_user_profile")
    {
      return call_<uim::RemoveUserProfileRequest, uim::RemoveUserProfileResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->remove_user_profile(ctx, req, resp);
        });
    }

    if (record.method == "merge")
    {
      return call_<uim::MergeRequest, uim::MergeResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->merge(ctx, req, resp);
        });
    }

    if (record.method == "consider_publishers_optin")
    {
      return call_<uim::ConsiderPublishersOptinRequest, uim::ConsiderPublishersOptinResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->consider_publishers_optin(ctx, req, resp);
        });
    }

    if (record.method == "uim_ready")
    {
      return call_<uim::UimReadyRequest, uim::UimReadyResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->uim_ready(ctx, req, resp);
        });
    }

    if (record.method == "get_progress")
    {
      return call_<uim::GetProgressRequest, uim::GetProgressResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->get_progress(ctx, req, resp);
        });
    }

    if (record.method == "clear_expired")
    {
      return call_<uim::ClearExpiredRequest, uim::ClearExpiredResponse>(
        record.request_bytes,
        [stub](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub->clear_expired(ctx, req, resp);
        });
    }

    return grpc::Status(grpc::INVALID_ARGUMENT, "unknown UserInfoManager method: " + record.method);
  }

  std::string format_time_us(std::uint64_t time_us)
  {
    const std::time_t seconds = static_cast<std::time_t>(time_us / 1000000);
    const std::uint64_t microseconds = time_us % 1000000;

    std::tm tm{};
    gmtime_r(&seconds, &tm);

    std::ostringstream out;
    out << std::put_time(&tm, "%F %T") << '.' <<
      std::setw(6) << std::setfill('0') << microseconds;
    return out.str();
  }

  int print_dump(const std::string& dump_path)
  {
    dump::Reader reader(dump_path);
    dump::DumpRecord record;
    while (reader.read(record))
    {
      std::cout << format_time_us(record.request_time_us) << ' ' << record.method << '\n';
    }

    return 0;
  }

  int play_dump(const Config& config)
  {
    grpc::ChannelArguments channel_args;
    channel_args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
    auto channel = AdServer::Grpc::create_custom_channel(
      config.send_endpoint,
      grpc::InsecureChannelCredentials(),
      channel_args);
    auto stub = uim::UserInfoManagerGrpc::NewStub(channel);

    dump::Reader reader(config.dump_path);
    dump::DumpRecord record;
    std::uint64_t sent = 0;
    std::uint64_t failed = 0;
    while (reader.read(record))
    {
      const grpc::Status status = replay_record(stub.get(), record);
      ++sent;
      if (!status.ok())
      {
        ++failed;
        std::cerr << "record #" << sent << " " << record.method <<
          " failed: " << status_description(status) << std::endl;
      }
    }

    std::cout << "UserInfoManagerPlayer sent " << sent <<
      " requests, failed " << failed << std::endl;
    return failed == 0 ? 0 : 1;
  }
}

int main(int argc, char** argv)
{
  try
  {
    Config config;
    if (!parse_args(argc, argv, config))
    {
      return usage();
    }

    if (config.command == Command::Print)
    {
      return print_dump(config.dump_path);
    }

    return play_dump(config);
  }
  catch (const std::exception& ex)
  {
    std::cerr << "UserInfoManagerPlayer: " << ex.what() << std::endl;
    return 1;
  }
}
