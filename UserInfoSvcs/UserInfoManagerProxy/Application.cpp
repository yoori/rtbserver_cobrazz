#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include <grpc/impl/channel_arg_names.h>
#include <grpcpp/grpcpp.h>

#include <Generics/AppUtils.hpp>

#include <Commons/Grpc/Batch.grpc.pb.h>
#include <Commons/Grpc/GrpcClient.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoManagerGrpc.grpc.pb.h>

#include "UserInfoManagerDump.hpp"

namespace
{
  namespace batch = adserver::grpc;
  namespace uim = adserver::user_info_svcs::user_info_manager;
  namespace dump = AdServer::UserInfoSvcs::UserInfoManagerDump;

  constexpr const char* GET_SOURCE_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/get_source";
  constexpr const char* GET_MASTER_STAMP_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/get_master_stamp";
  constexpr const char* GET_USER_PROFILE_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/get_user_profile";
  constexpr const char* MATCH_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/match";
  constexpr const char* UPDATE_USER_FREQ_CAPS_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/update_user_freq_caps";
  constexpr const char* CONFIRM_USER_FREQ_CAPS_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/confirm_user_freq_caps";
  constexpr const char* FRAUD_USER_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/fraud_user";
  constexpr const char* REMOVE_USER_PROFILE_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/remove_user_profile";
  constexpr const char* MERGE_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/merge";
  constexpr const char* CONSIDER_PUBLISHERS_OPTIN_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/consider_publishers_optin";
  constexpr const char* UIM_READY_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/uim_ready";
  constexpr const char* GET_PROGRESS_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/get_progress";
  constexpr const char* CLEAR_EXPIRED_FULL_METHOD =
    "/adserver.user_info_svcs.user_info_manager.UserInfoManagerGrpc/clear_expired";

  int usage()
  {
    std::cerr <<
      "usage: UserInfoManagerProxy "
      "--listen-port <port> "
      "--send-endpoint <host:port> "
      "--save-dump <path>\n";
    return 2;
  }

  struct Config
  {
    std::string listen_port;
    std::string send_endpoint;
    std::string save_dump;
  };

  bool parse_args(int argc, char** argv, Config& config)
  {
    Generics::AppUtils::CheckOption opt_help;
    Generics::AppUtils::StringOption opt_listen_port;
    Generics::AppUtils::StringOption opt_send_endpoint;
    Generics::AppUtils::StringOption opt_save_dump;
    Generics::AppUtils::Args args(-1);

    args.add(
      Generics::AppUtils::equal_name("help") ||
      Generics::AppUtils::short_name("h"),
      opt_help);
    args.add(Generics::AppUtils::equal_name("listen-port"), opt_listen_port);
    args.add(Generics::AppUtils::equal_name("send-endpoint"), opt_send_endpoint);
    args.add(Generics::AppUtils::equal_name("save-dump"), opt_save_dump);
    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled() ||
      !args.commands().empty() ||
      !opt_listen_port.installed() ||
      !opt_send_endpoint.installed() ||
      !opt_save_dump.installed())
    {
      return false;
    }

    config.listen_port = *opt_listen_port;
    config.send_endpoint = *opt_send_endpoint;
    config.save_dump = *opt_save_dump;

    return true;
  }

  class ProxyService final : public uim::UserInfoManagerGrpc::Service
  {
  public:
    ProxyService(
      std::shared_ptr<uim::UserInfoManagerGrpc::Stub> stub,
      std::shared_ptr<dump::Writer> dump_writer)
      : stub_(std::move(stub)),
        dump_writer_(std::move(dump_writer))
    {}

    grpc::Status get_source(
      grpc::ServerContext* context,
      const uim::GetSourceRequest* request,
      uim::GetSourceResponse* response) override
    {
      return forward_(context, "get_source", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->get_source(ctx, req, resp);
        });
    }

    grpc::Status get_master_stamp(
      grpc::ServerContext* context,
      const uim::GetMasterStampRequest* request,
      uim::GetMasterStampResponse* response) override
    {
      return forward_(context, "get_master_stamp", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->get_master_stamp(ctx, req, resp);
        });
    }

    grpc::Status get_user_profile(
      grpc::ServerContext* context,
      const uim::GetUserProfileRequest* request,
      uim::GetUserProfileResponse* response) override
    {
      return forward_(context, "get_user_profile", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->get_user_profile(ctx, req, resp);
        });
    }

    grpc::Status match(
      grpc::ServerContext* context,
      const uim::MatchRequest* request,
      uim::MatchResponse* response) override
    {
      return forward_(context, "match", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->match(ctx, req, resp);
        });
    }

    grpc::Status update_user_freq_caps(
      grpc::ServerContext* context,
      const uim::UpdateUserFreqCapsRequest* request,
      uim::UpdateUserFreqCapsResponse* response) override
    {
      return forward_(context, "update_user_freq_caps", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->update_user_freq_caps(ctx, req, resp);
        });
    }

    grpc::Status confirm_user_freq_caps(
      grpc::ServerContext* context,
      const uim::ConfirmUserFreqCapsRequest* request,
      uim::ConfirmUserFreqCapsResponse* response) override
    {
      return forward_(context, "confirm_user_freq_caps", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->confirm_user_freq_caps(ctx, req, resp);
        });
    }

    grpc::Status fraud_user(
      grpc::ServerContext* context,
      const uim::FraudUserRequest* request,
      uim::FraudUserResponse* response) override
    {
      return forward_(context, "fraud_user", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->fraud_user(ctx, req, resp);
        });
    }

    grpc::Status remove_user_profile(
      grpc::ServerContext* context,
      const uim::RemoveUserProfileRequest* request,
      uim::RemoveUserProfileResponse* response) override
    {
      return forward_(context, "remove_user_profile", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->remove_user_profile(ctx, req, resp);
        });
    }

    grpc::Status merge(
      grpc::ServerContext* context,
      const uim::MergeRequest* request,
      uim::MergeResponse* response) override
    {
      return forward_(context, "merge", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->merge(ctx, req, resp);
        });
    }

    grpc::Status consider_publishers_optin(
      grpc::ServerContext* context,
      const uim::ConsiderPublishersOptinRequest* request,
      uim::ConsiderPublishersOptinResponse* response) override
    {
      return forward_(context, "consider_publishers_optin", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->consider_publishers_optin(ctx, req, resp);
        });
    }

    grpc::Status uim_ready(
      grpc::ServerContext* context,
      const uim::UimReadyRequest* request,
      uim::UimReadyResponse* response) override
    {
      return forward_(context, "uim_ready", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->uim_ready(ctx, req, resp);
        });
    }

    grpc::Status get_progress(
      grpc::ServerContext* context,
      const uim::GetProgressRequest* request,
      uim::GetProgressResponse* response) override
    {
      return forward_(context, "get_progress", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->get_progress(ctx, req, resp);
        });
    }

    grpc::Status clear_expired(
      grpc::ServerContext* context,
      const uim::ClearExpiredRequest* request,
      uim::ClearExpiredResponse* response) override
    {
      return forward_(context, "clear_expired", request, response,
        [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
          return stub_->clear_expired(ctx, req, resp);
        });
    }

  private:
    template<typename Request, typename Response, typename Caller>
    grpc::Status forward_(
      grpc::ServerContext* server_context,
      const char* method,
      const Request* request,
      Response* response,
      Caller caller)
    {
      const auto request_time = dump::now_us();

      grpc::ClientContext client_context;
      client_context.set_deadline(server_context->deadline());

      const grpc::Status status = caller(&client_context, *request, response);
      const auto response_time = dump::now_us();

      dump::DumpRecord record;
      record.request_time_us = request_time;
      record.method = method;
      request->SerializeToString(&record.request_bytes);
      record.response_time_us = response_time;
      response->SerializeToString(&record.response_bytes);
      dump_writer_->write(record);

      return status;
    }

  private:
    std::shared_ptr<uim::UserInfoManagerGrpc::Stub> stub_;
    std::shared_ptr<dump::Writer> dump_writer_;
  };

  class BatchProxyService final : public batch::BatchTransport::Service
  {
  public:
    BatchProxyService(
      std::shared_ptr<uim::UserInfoManagerGrpc::Stub> stub,
      std::shared_ptr<dump::Writer> dump_writer)
      : stub_(std::move(stub)),
        dump_writer_(std::move(dump_writer))
    {}

    grpc::Status stream_batches(
      grpc::ServerContext* server_context,
      grpc::ServerReaderWriter<batch::BatchResponse, batch::BatchRequest>* stream) override
    {
      batch::BatchRequest request;
      while (stream->Read(&request))
      {
        batch::BatchResponse response;
        for (const auto& item : request.items())
        {
          auto* response_item = response.add_items();
          response_item->set_request_id(item.request_id());
          forward_batch_item_(server_context, item, *response_item);
        }

        if (!stream->Write(response))
        {
          break;
        }
      }

      return grpc::Status::OK;
    }

  private:
    template<typename Request, typename Response, typename Caller>
    void forward_payload_(
      grpc::ServerContext* server_context,
      const batch::BatchRequestItem& request_item,
      batch::BatchResponseItem& response_item,
      const char* dump_method,
      Caller caller)
    {
      const auto request_time = dump::now_us();

      Request request;
      if (!request.ParseFromString(request_item.payload()))
      {
        response_item.set_status_code(grpc::StatusCode::INVALID_ARGUMENT);
        response_item.set_status_message("failed to parse batch request payload");
        return;
      }

      Response response;
      grpc::ClientContext client_context;
      client_context.set_deadline(server_context->deadline());

      const grpc::Status status = caller(&client_context, request, &response);
      const auto response_time = dump::now_us();

      response_item.set_status_code(status.error_code());
      response_item.set_status_message(status.error_message());
      response.SerializeToString(response_item.mutable_payload());

      dump::DumpRecord record;
      record.request_time_us = request_time;
      record.method = dump_method;
      record.request_bytes = request_item.payload();
      record.response_time_us = response_time;
      record.response_bytes = response_item.payload();
      dump_writer_->write(record);
    }

    void forward_batch_item_(
      grpc::ServerContext* server_context,
      const batch::BatchRequestItem& request_item,
      batch::BatchResponseItem& response_item)
    {
      const std::string& method = request_item.full_method();
      if (method == GET_SOURCE_FULL_METHOD)
      {
        forward_payload_<uim::GetSourceRequest, uim::GetSourceResponse>(
          server_context, request_item, response_item, "get_source",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->get_source(ctx, req, resp);
          });
      }
      else if (method == GET_MASTER_STAMP_FULL_METHOD)
      {
        forward_payload_<uim::GetMasterStampRequest, uim::GetMasterStampResponse>(
          server_context, request_item, response_item, "get_master_stamp",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->get_master_stamp(ctx, req, resp);
          });
      }
      else if (method == GET_USER_PROFILE_FULL_METHOD)
      {
        forward_payload_<uim::GetUserProfileRequest, uim::GetUserProfileResponse>(
          server_context, request_item, response_item, "get_user_profile",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->get_user_profile(ctx, req, resp);
          });
      }
      else if (method == MATCH_FULL_METHOD)
      {
        forward_payload_<uim::MatchRequest, uim::MatchResponse>(
          server_context, request_item, response_item, "match",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->match(ctx, req, resp);
          });
      }
      else if (method == UPDATE_USER_FREQ_CAPS_FULL_METHOD)
      {
        forward_payload_<uim::UpdateUserFreqCapsRequest, uim::UpdateUserFreqCapsResponse>(
          server_context, request_item, response_item, "update_user_freq_caps",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->update_user_freq_caps(ctx, req, resp);
          });
      }
      else if (method == CONFIRM_USER_FREQ_CAPS_FULL_METHOD)
      {
        forward_payload_<uim::ConfirmUserFreqCapsRequest, uim::ConfirmUserFreqCapsResponse>(
          server_context, request_item, response_item, "confirm_user_freq_caps",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->confirm_user_freq_caps(ctx, req, resp);
          });
      }
      else if (method == FRAUD_USER_FULL_METHOD)
      {
        forward_payload_<uim::FraudUserRequest, uim::FraudUserResponse>(
          server_context, request_item, response_item, "fraud_user",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->fraud_user(ctx, req, resp);
          });
      }
      else if (method == REMOVE_USER_PROFILE_FULL_METHOD)
      {
        forward_payload_<uim::RemoveUserProfileRequest, uim::RemoveUserProfileResponse>(
          server_context, request_item, response_item, "remove_user_profile",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->remove_user_profile(ctx, req, resp);
          });
      }
      else if (method == MERGE_FULL_METHOD)
      {
        forward_payload_<uim::MergeRequest, uim::MergeResponse>(
          server_context, request_item, response_item, "merge",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->merge(ctx, req, resp);
          });
      }
      else if (method == CONSIDER_PUBLISHERS_OPTIN_FULL_METHOD)
      {
        forward_payload_<uim::ConsiderPublishersOptinRequest, uim::ConsiderPublishersOptinResponse>(
          server_context, request_item, response_item, "consider_publishers_optin",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->consider_publishers_optin(ctx, req, resp);
          });
      }
      else if (method == UIM_READY_FULL_METHOD)
      {
        forward_payload_<uim::UimReadyRequest, uim::UimReadyResponse>(
          server_context, request_item, response_item, "uim_ready",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->uim_ready(ctx, req, resp);
          });
      }
      else if (method == GET_PROGRESS_FULL_METHOD)
      {
        forward_payload_<uim::GetProgressRequest, uim::GetProgressResponse>(
          server_context, request_item, response_item, "get_progress",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->get_progress(ctx, req, resp);
          });
      }
      else if (method == CLEAR_EXPIRED_FULL_METHOD)
      {
        forward_payload_<uim::ClearExpiredRequest, uim::ClearExpiredResponse>(
          server_context, request_item, response_item, "clear_expired",
          [this](grpc::ClientContext* ctx, const auto& req, auto* resp) {
            return stub_->clear_expired(ctx, req, resp);
          });
      }
      else
      {
        response_item.set_status_code(grpc::StatusCode::UNIMPLEMENTED);
        response_item.set_status_message("Unknown method");
      }
    }

  private:
    std::shared_ptr<uim::UserInfoManagerGrpc::Stub> stub_;
    std::shared_ptr<dump::Writer> dump_writer_;
  };
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

    grpc::ChannelArguments channel_args;
    channel_args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
    auto channel = AdServer::Grpc::create_custom_channel(
      config.send_endpoint,
      grpc::InsecureChannelCredentials(),
      channel_args);

    auto dump_writer = std::make_shared<dump::Writer>(config.save_dump);
    auto stub = std::shared_ptr<uim::UserInfoManagerGrpc::Stub>(
      uim::UserInfoManagerGrpc::NewStub(channel).release());
    ProxyService service(stub, dump_writer);
    BatchProxyService batch_service(stub, dump_writer);

    grpc::ServerBuilder builder;
    builder.SetMaxReceiveMessageSize(-1);
    builder.SetMaxSendMessageSize(-1);
    builder.AddListeningPort("0.0.0.0:" + config.listen_port, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    builder.RegisterService(&batch_service);

    auto server = builder.BuildAndStart();
    if (!server)
    {
      throw std::runtime_error("failed to listen on gRPC port: " + config.listen_port);
    }

    std::cout << "UserInfoManagerProxy listens on port " <<
      config.listen_port << ", forwards to " << config.send_endpoint <<
      ", dump file " << config.save_dump << std::endl;
    server->Wait();
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "UserInfoManagerProxy: " << ex.what() << std::endl;
    return 1;
  }
}
