#include "UserInfoDistributedGrpcClient.hpp"

#include <chrono>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include <grpcpp/grpcpp.h>

#include <Commons/GrpcAlgs.hpp>
#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/ResponseHolder.hpp>
#include <Commons/UserInfoManip.hpp>
#include <UserInfoSvcs/UserInfoController/UserInfoControllerGrpc.grpc.pb.h>

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    const Generics::Time DEFAULT_POOL_TIMEOUT = Generics::Time::ONE_SECOND;
    const Generics::Time DEFAULT_RESOLVE_PERIOD = Generics::Time(10);
    const std::chrono::seconds DEFAULT_RPC_TIMEOUT(5);

    void set_deadline_(grpc::ClientContext& context)
    {
      context.set_deadline(std::chrono::system_clock::now() + DEFAULT_RPC_TIMEOUT);
    }

    template<typename Response, typename Callback>
    void finish_with_unavailable_(Callback callback, const char* message)
    {
      callback(
        grpc::Status(grpc::StatusCode::UNAVAILABLE, message),
        AdServer::Grpc::ResponseHolder<Response>::make_value(Response()));
    }
  }

  struct UserInfoDistributedGrpcClient::ControllerClient
  {
    using ControllerGrpc =
      adserver::user_info_svcs::user_info_controller::UserInfoControllerGrpc;

    explicit ControllerClient(
      const std::string& endpoint,
      const std::size_t partition_index)
      : endpoint(endpoint),
        name(endpoint),
        partition_index(partition_index),
        channel(AdServer::Grpc::create_channel(
          this->endpoint,
          grpc::InsecureChannelCredentials())),
        stub(ControllerGrpc::NewStub(channel))
    {}

    void reset()
    {
      channel = AdServer::Grpc::create_channel(
        endpoint,
        grpc::InsecureChannelCredentials());
      stub = ControllerGrpc::NewStub(channel);
    }

    const std::string endpoint;
    const std::string name;
    const std::size_t partition_index;
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<ControllerGrpc::Stub> stub;
  };

  UserInfoDistributedGrpcClient::UserInfoDistributedGrpcClient(
    const UserInfoControllerRefs& user_info_controller_refs,
    AdServer::Grpc::BatchingOptions batching_options,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    Logging::Logger* logger,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner)
  {
    pool_ = std::make_shared<Pool>(
      "UserInfoDistributedGrpcClient",
      "UserInfo",
      user_info_controller_refs,
      std::move(batching_options),
      std::move(grpc_executor),
      std::move(coalesce_runner),
      logger,
      [this](ControllerClient& controller_client)
      {
        return resolve_partition_(controller_client);
      },
      &partition_hash_,
      &chunk_index_,
      DEFAULT_POOL_TIMEOUT,
      DEFAULT_RESOLVE_PERIOD);
    add_child_object(pool_);
  }

  UserInfoDistributedGrpcClient::~UserInfoDistributedGrpcClient() noexcept =
    default;

  AdServer::Grpc::Stats
  UserInfoDistributedGrpcClient::stats() const noexcept
  {
    return pool_->stats();
  }

  AdServer::Grpc::Client::EndpointStats
  UserInfoDistributedGrpcClient::endpoint_stats() const noexcept
  {
    return pool_->endpoint_stats();
  }

  std::string
  UserInfoDistributedGrpcClient::endpoint_for_user(
    const std::string& user_id) noexcept
  {
    auto ref = get_ref_(user_id);
    return ref ? (*ref)->endpoint : std::string();
  }

#define USER_INFO_ROUTE_USER(method_name, request_type, response_type, callback_type, user_expr) \
  void \
  UserInfoDistributedGrpcClient::method_name( \
    const request_type& request, \
    callback_type callback) \
  { \
    if (!active()) \
    { \
      finish_with_unavailable_<response_type>( \
        std::move(callback), \
        "inactive"); \
      return; \
    } \
    auto ref = get_ref_(user_expr); \
    if (!ref) \
    { \
      finish_with_unavailable_<response_type>( \
        std::move(callback), \
        "no available UserInfoManager grpc client"); \
      return; \
    } \
    auto pool_ref = std::move(*ref); \
    const auto endpoint = pool_ref->endpoint; \
    pool_ref->client->method_name( \
      request, \
      [ \
        pool_ref = std::move(pool_ref), \
        endpoint, \
        callback = std::move(callback) \
      ]( \
        const grpc::Status& status, \
        AdServer::Grpc::ResponseHolder<response_type>&& response_holder) \
      mutable \
      { \
        if (!status.ok() && !AdServer::Grpc::is_request_specific_error(status)) \
        { \
          pool_ref.mark_as_bad( \
            Generics::Time::get_time_of_day() + DEFAULT_POOL_TIMEOUT); \
        } \
        callback( \
          AdServer::Grpc::status_with_endpoint(status, endpoint), \
          std::move(response_holder)); \
      }); \
  }

#define USER_INFO_ROUTE_ANY(method_name, request_type, response_type, callback_type) \
  void \
  UserInfoDistributedGrpcClient::method_name( \
    const request_type& request, \
    callback_type callback) \
  { \
    if (!active()) \
    { \
      finish_with_unavailable_<response_type>( \
        std::move(callback), \
        "inactive"); \
      return; \
    } \
    auto ref = get_any_ref_(); \
    if (!ref) \
    { \
      finish_with_unavailable_<response_type>( \
        std::move(callback), \
        "no available UserInfoManager grpc client"); \
      return; \
    } \
    auto pool_ref = std::move(*ref); \
    const auto endpoint = pool_ref->endpoint; \
    pool_ref->client->method_name( \
      request, \
      [ \
        pool_ref = std::move(pool_ref), \
        endpoint, \
        callback = std::move(callback) \
      ]( \
        const grpc::Status& status, \
        AdServer::Grpc::ResponseHolder<response_type>&& response_holder) \
      mutable \
      { \
        if (!status.ok() && !AdServer::Grpc::is_request_specific_error(status)) \
        { \
          pool_ref.mark_as_bad( \
            Generics::Time::get_time_of_day() + DEFAULT_POOL_TIMEOUT); \
        } \
        callback( \
          AdServer::Grpc::status_with_endpoint(status, endpoint), \
          std::move(response_holder)); \
      }); \
  }

  USER_INFO_ROUTE_ANY(
    get_source,
    adserver::user_info_svcs::user_info_manager::GetSourceRequest,
    adserver::user_info_svcs::user_info_manager::GetSourceResponse,
    UserInfoManagerGrpcAsyncClient::GetSourceCallback)

  USER_INFO_ROUTE_ANY(
    get_master_stamp,
    adserver::user_info_svcs::user_info_manager::GetMasterStampRequest,
    adserver::user_info_svcs::user_info_manager::GetMasterStampResponse,
    UserInfoManagerGrpcAsyncClient::GetMasterStampCallback)

  USER_INFO_ROUTE_USER(
    get_user_profile,
    adserver::user_info_svcs::user_info_manager::GetUserProfileRequest,
    adserver::user_info_svcs::user_info_manager::GetUserProfileResponse,
    UserInfoManagerGrpcAsyncClient::GetUserProfileCallback,
    request.user_id())

  USER_INFO_ROUTE_USER(
    match,
    adserver::user_info_svcs::user_info_manager::MatchRequest,
    adserver::user_info_svcs::user_info_manager::MatchResponse,
    UserInfoManagerGrpcAsyncClient::MatchCallback,
    request.user_info().user_id())

  USER_INFO_ROUTE_USER(
    update_user_freq_caps,
    adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsRequest,
    adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsResponse,
    UserInfoManagerGrpcAsyncClient::UpdateUserFreqCapsCallback,
    request.user_id())

  USER_INFO_ROUTE_USER(
    confirm_user_freq_caps,
    adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsRequest,
    adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsResponse,
    UserInfoManagerGrpcAsyncClient::ConfirmUserFreqCapsCallback,
    request.user_id())

  USER_INFO_ROUTE_USER(
    fraud_user,
    adserver::user_info_svcs::user_info_manager::FraudUserRequest,
    adserver::user_info_svcs::user_info_manager::FraudUserResponse,
    UserInfoManagerGrpcAsyncClient::FraudUserCallback,
    request.user_id())

  USER_INFO_ROUTE_USER(
    remove_user_profile,
    adserver::user_info_svcs::user_info_manager::RemoveUserProfileRequest,
    adserver::user_info_svcs::user_info_manager::RemoveUserProfileResponse,
    UserInfoManagerGrpcAsyncClient::RemoveUserProfileCallback,
    request.user_id())

  USER_INFO_ROUTE_USER(
    merge,
    adserver::user_info_svcs::user_info_manager::MergeRequest,
    adserver::user_info_svcs::user_info_manager::MergeResponse,
    UserInfoManagerGrpcAsyncClient::MergeCallback,
    request.user_info().user_id())

  USER_INFO_ROUTE_USER(
    consider_publishers_optin,
    adserver::user_info_svcs::user_info_manager::ConsiderPublishersOptinRequest,
    adserver::user_info_svcs::user_info_manager::ConsiderPublishersOptinResponse,
    UserInfoManagerGrpcAsyncClient::ConsiderPublishersOptinCallback,
    request.user_id())

  USER_INFO_ROUTE_ANY(
    uim_ready,
    adserver::user_info_svcs::user_info_manager::UimReadyRequest,
    adserver::user_info_svcs::user_info_manager::UimReadyResponse,
    UserInfoManagerGrpcAsyncClient::UimReadyCallback)

  USER_INFO_ROUTE_ANY(
    get_progress,
    adserver::user_info_svcs::user_info_manager::GetProgressRequest,
    adserver::user_info_svcs::user_info_manager::GetProgressResponse,
    UserInfoManagerGrpcAsyncClient::GetProgressCallback)

  USER_INFO_ROUTE_ANY(
    clear_expired,
    adserver::user_info_svcs::user_info_manager::ClearExpiredRequest,
    adserver::user_info_svcs::user_info_manager::ClearExpiredResponse,
    UserInfoManagerGrpcAsyncClient::ClearExpiredCallback)

#undef USER_INFO_ROUTE_USER
#undef USER_INFO_ROUTE_ANY

  std::optional<UserInfoDistributedGrpcClient::Pool::Ref>
  UserInfoDistributedGrpcClient::get_ref_(const std::string& user_id) noexcept
  {
    return pool_->get_ref(user_id);
  }

  std::optional<UserInfoDistributedGrpcClient::Pool::Ref>
  UserInfoDistributedGrpcClient::get_any_ref_() noexcept
  {
    return pool_->get_any_ref();
  }

  std::optional<UserInfoDistributedGrpcClient::Pool::EndpointChunksArray>
  UserInfoDistributedGrpcClient::resolve_partition_(
    ControllerClient& controller_client)
  {
    grpc::ClientContext context;
    set_deadline_(context);
    adserver::user_info_svcs::user_info_controller::
      GetSessionDescriptionRequest request;
    adserver::user_info_svcs::user_info_controller::
      GetSessionDescriptionResponse response;

    const auto status = controller_client.stub->get_session_description(
      &context,
      request,
      &response);

    if (!status.ok())
    {
      controller_client.reset();
      std::ostringstream ostr;
      ostr << "UserInfoController '" << controller_client.endpoint
        << "': get_session_description failed: code="
        << static_cast<int>(status.error_code())
        << ", message=" << status.error_message();
      throw Exception(ostr.str());
    }

    Pool::EndpointChunksArray refs;
    refs.reserve(response.user_info_managers_size());
    for (const auto& manager : response.user_info_managers())
    {
      Pool::EndpointChunks endpoint_chunks;
      endpoint_chunks.endpoint = manager.user_info_manager_endpoint();
      endpoint_chunks.chunk_ids.reserve(manager.chunk_ids_size());
      for (const auto chunk_id : manager.chunk_ids())
      {
        endpoint_chunks.chunk_ids.emplace_back(chunk_id);
      }
      refs.emplace_back(std::move(endpoint_chunks));
    }
    if (refs.empty())
    {
      std::ostringstream ostr;
      ostr << "UserInfoController '" << controller_client.endpoint
        << "': get_session_description returned no UserInfoManager refs";
      throw Exception(ostr.str());
    }
    return refs;
  }

  unsigned long
  UserInfoDistributedGrpcClient::chunk_index_(
    const std::string& user_id,
    unsigned long chunks_number)
  {
    return AdServer::Commons::uuid_distribution_hash(
      GrpcAlgs::unpack_user_id(user_id)) % chunks_number;
  }

  std::uint64_t
  UserInfoDistributedGrpcClient::partition_hash_(const std::string& user_id)
  {
    return GrpcAlgs::unpack_user_id(user_id).hash();
  }
}
