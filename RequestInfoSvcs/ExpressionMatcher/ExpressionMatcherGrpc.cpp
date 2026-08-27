#include "ExpressionMatcherGrpc.hpp"

#include <utility>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/GrpcServer.hpp>

#include "ExpressionMatcherGrpc.grpc.pb.h"

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
    constexpr char expression_matcher_grpc_aspect[] = "ExpressionMatcherGrpc";

    namespace Proto = adserver::request_info_svcs::expression_matcher;

    void
    pack_profile(const Generics::ConstSmartMemBuf* profile, Proto::ProfileResponse& response)
    {
      response.set_found(profile != nullptr);
      if(profile)
      {
        response.set_profile(profile->membuf().data(), profile->membuf().size());
      }
    }
  }

  class ExpressionMatcherGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      ExpressionMatcherGrpc::ServiceImpl,
      Proto::ExpressionMatcherGrpc,
      Proto::ExpressionMatcherGrpc::AsyncService>
  {
    using AsyncService = Proto::ExpressionMatcherGrpc::AsyncService;

  public:
    explicit ServiceImpl(ExpressionMatcherImpl* expression_matcher)
      : expression_matcher_(ReferenceCounting::add_ref(expression_matcher))
    {}

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_GRPC_CORO_CALL(
          Proto::ProfileRequest,
          Proto::ProfileResponse,
          get_inventory_profile,
          co_get_inventory_profile),
        MAKE_GRPC_CORO_CALL(
          Proto::UserTriggerMatchProfileRequest,
          Proto::ProfileResponse,
          get_user_trigger_match_profile,
          co_get_user_trigger_match_profile),
        MAKE_GRPC_CORO_CALL(
          Proto::ProfileRequest,
          Proto::ProfileResponse,
          get_request_trigger_match_profile,
          co_get_request_trigger_match_profile),
        MAKE_GRPC_CORO_CALL(
          Proto::ProfileRequest,
          Proto::ProfileResponse,
          get_household_colo_reach_profile,
          co_get_household_colo_reach_profile),
        MAKE_GRPC_CORO_CALL(
          Proto::RunDailyProcessingRequest,
          Proto::RunDailyProcessingResponse,
          run_daily_processing,
          co_run_daily_processing));
    }

    AdServer::Commons::StartableAwaitable<void>
    co_get_inventory_profile(
      Proto::ProfileRequest&& request,
      Proto::ProfileResponse& response,
      grpc::Status& status) const
    {
      co_await co_execute_profile_request_(
        expression_matcher_->co_get_inventory_profile(
          AdServer::Commons::UserId(request.id())),
        response,
        status);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_get_user_trigger_match_profile(
      Proto::UserTriggerMatchProfileRequest&& request,
      Proto::ProfileResponse& response,
      grpc::Status& status) const
    {
      co_await co_execute_profile_request_(
        expression_matcher_->co_get_user_trigger_match_profile(
          AdServer::Commons::UserId(request.user_id()),
          request.temporary_user()),
        response,
        status);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_get_request_trigger_match_profile(
      Proto::ProfileRequest&& request,
      Proto::ProfileResponse& response,
      grpc::Status& status) const
    {
      co_await co_execute_profile_request_(
        expression_matcher_->co_get_request_trigger_match_profile(
          AdServer::Commons::RequestId(request.id())),
        response,
        status);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_get_household_colo_reach_profile(
      Proto::ProfileRequest&& request,
      Proto::ProfileResponse& response,
      grpc::Status& status) const
    {
      co_await co_execute_profile_request_(
        expression_matcher_->co_get_household_colo_reach_profile(
          AdServer::Commons::UserId(request.id())),
        response,
        status);
    }

    AdServer::Commons::StartableAwaitable<void>
    co_run_daily_processing(
      Proto::RunDailyProcessingRequest&& request,
      Proto::RunDailyProcessingResponse&,
      grpc::Status& status) const
    {
      try
      {
        co_await expression_matcher_->co_run_daily_processing(request.synchronous());
        status = grpc::Status::OK;
      }
      catch(const ExpressionMatcherImpl::NotReady& ex)
      {
        status = grpc::Status(grpc::StatusCode::UNAVAILABLE, ex.what());
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
      catch(const ExpressionMatcherImpl::NotReady& ex)
      {
        status = grpc::Status(grpc::StatusCode::UNAVAILABLE, ex.what());
      }
      catch(const eh::Exception& ex)
      {
        status = grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
      }
    }

    ExpressionMatcherImpl_var expression_matcher_;
  };

  ExpressionMatcherGrpc::ExpressionMatcherGrpc(
    ExpressionMatcherImpl* expression_matcher,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port,
    std::size_t grpc_threads)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        expression_matcher_grpc_aspect,
        bind_address_,
        grpc_threads,
        std::make_unique<ServiceImpl>(expression_matcher)))
  {
    add_child_object(impl_);
  }

  ExpressionMatcherGrpc::~ExpressionMatcherGrpc() noexcept = default;
}
