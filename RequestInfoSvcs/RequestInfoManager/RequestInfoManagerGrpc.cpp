#include "RequestInfoManagerGrpc.hpp"

#include <utility>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/GrpcServer.hpp>

#include "RequestInfoManagerGrpc.grpc.pb.h"

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
    constexpr char request_info_manager_grpc_aspect[] = "RequestInfoManagerGrpc";

    namespace Proto = adserver::request_info_svcs::request_info_manager;

    void
    pack_profile(const Generics::ConstSmartMemBuf* profile, Proto::ProfileResponse& response)
    {
      response.set_found(profile != nullptr);
      if (profile)
      {
        response.set_profile(profile->membuf().data(), profile->membuf().size());
      }
    }
  }

  class RequestInfoManagerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      RequestInfoManagerGrpc::ServiceImpl,
      Proto::RequestInfoManagerGrpc,
      Proto::RequestInfoManagerGrpc::AsyncService>
  {
    using AsyncService = Proto::RequestInfoManagerGrpc::AsyncService;

  public:
    explicit ServiceImpl(RequestInfoManagerImpl* request_info_manager)
      : request_info_manager_(ReferenceCounting::add_ref(request_info_manager))
    {}

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_GRPC_CORO_CALL(
          Proto::ProfileRequest,
          Proto::ProfileResponse,
          get_profile,
          co_get_profile),
        MAKE_GRPC_CORO_CALL(
          Proto::ProfileRequest,
          Proto::ProfileResponse,
          get_user_campaign_reach_profile,
          co_get_user_campaign_reach_profile),
        MAKE_GRPC_CORO_CALL(
          Proto::ProfileRequest,
          Proto::ProfileResponse,
          get_user_action_profile,
          co_get_user_action_profile),
        MAKE_GRPC_CORO_CALL(
          Proto::ProfileRequest,
          Proto::ProfileResponse,
          get_passback_profile,
          co_get_passback_profile),
        MAKE_GRPC_CORO_CALL(
          Proto::ProfileRequest,
          Proto::ProfileResponse,
          get_user_fraud_protection_profile,
          co_get_user_fraud_protection_profile),
        MAKE_GRPC_CORO_CALL(
          Proto::ProfileRequest,
          Proto::ProfileResponse,
          get_user_site_reach_profile,
          co_get_user_site_reach_profile),
        MAKE_GRPC_CORO_CALL(
          Proto::ProfileRequest,
          Proto::ProfileResponse,
          get_user_tag_request_group_profile,
          co_get_user_tag_request_group_profile),
        MAKE_GRPC_CORO_CALL(
          Proto::ClearExpiredRequest,
          Proto::ClearExpiredResponse,
          clear_expired,
          co_clear_expired));
    }

    AdServer::Commons::StartableAwaitable<void>
    co_get_profile(
      Proto::ProfileRequest&& request,
      Proto::ProfileResponse& response,
      grpc::Status& status) const
    {
      co_await co_execute_profile_request_(
        request_info_manager_->co_get_profile(AdServer::Commons::RequestId(request.id())),
        response,
        status);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_get_user_campaign_reach_profile(
      Proto::ProfileRequest&& request,
      Proto::ProfileResponse& response,
      grpc::Status& status) const
    {
      co_await co_execute_profile_request_(
        request_info_manager_->co_get_user_campaign_reach_profile(
          AdServer::Commons::UserId(request.id())),
        response,
        status);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_get_user_action_profile(
      Proto::ProfileRequest&& request,
      Proto::ProfileResponse& response,
      grpc::Status& status) const
    {
      co_await co_execute_profile_request_(
        request_info_manager_->co_get_user_action_profile(AdServer::Commons::UserId(request.id())),
        response,
        status);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_get_passback_profile(
      Proto::ProfileRequest&& request,
      Proto::ProfileResponse& response,
      grpc::Status& status) const
    {
      co_await co_execute_profile_request_(
        request_info_manager_->co_get_passback_profile(AdServer::Commons::RequestId(request.id())),
        response,
        status);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_get_user_fraud_protection_profile(
      Proto::ProfileRequest&& request,
      Proto::ProfileResponse& response,
      grpc::Status& status) const
    {
      co_await co_execute_profile_request_(
        request_info_manager_->co_get_user_fraud_protection_profile(
          AdServer::Commons::UserId(request.id())),
        response,
        status);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_get_user_site_reach_profile(
      Proto::ProfileRequest&& request,
      Proto::ProfileResponse& response,
      grpc::Status& status) const
    {
      co_await co_execute_profile_request_(
        request_info_manager_->co_get_user_site_reach_profile(
          AdServer::Commons::UserId(request.id())),
        response,
        status);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_get_user_tag_request_group_profile(
      Proto::ProfileRequest&& request,
      Proto::ProfileResponse& response,
      grpc::Status& status) const
    {
      co_await co_execute_profile_request_(
        request_info_manager_->co_get_user_tag_request_group_profile(
          AdServer::Commons::UserId(request.id())),
        response,
        status);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_clear_expired(
      Proto::ClearExpiredRequest&& request,
      Proto::ClearExpiredResponse&,
      grpc::Status& status) const
    {
      try
      {
        co_await request_info_manager_->co_clear_expired(request.synchronous());
        status = grpc::Status::OK;
      }
      catch(const eh::Exception& ex)
      {
        status = grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
      }
    }

  private:
    static AdServer::Commons::StartableAwaitable<void>
    co_execute_profile_request_(
      AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var> operation,
      Proto::ProfileResponse& response,
      grpc::Status& status)
    {
      try
      {
        const Generics::ConstSmartMemBuf_var profile = co_await std::move(operation);
        pack_profile(profile.in(), response);
        status = grpc::Status::OK;
      }
      catch(const RequestInfoManagerImpl::NotReady& ex)
      {
        status = grpc::Status(grpc::StatusCode::UNAVAILABLE, ex.what());
      }
      catch(const eh::Exception& ex)
      {
        status = grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
      }
    }

    RequestInfoManagerImpl_var request_info_manager_;
  };

  RequestInfoManagerGrpc::RequestInfoManagerGrpc(
    RequestInfoManagerImpl* request_info_manager,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port,
    std::size_t grpc_threads)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        request_info_manager_grpc_aspect,
        bind_address_,
        grpc_threads,
        std::make_unique<ServiceImpl>(request_info_manager)))
  {
    add_child_object(impl_);
  }

  RequestInfoManagerGrpc::~RequestInfoManagerGrpc() noexcept = default;
}
