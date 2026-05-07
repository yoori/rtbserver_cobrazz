#include "UserInfoDistributedGrpcClient.hpp"

#include <algorithm>
#include <chrono>
#include <set>
#include <utility>

#include <grpcpp/grpcpp.h>

#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <UserInfoSvcs/UserInfoController2/UserInfoControllerGrpc.grpc.pb.h>

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
        Response());
    }
  }

  struct UserInfoDistributedGrpcClient::ClientHolder
  {
    ClientHolder(
      std::string endpoint_val,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      AdServer::Grpc::BatchingOptions batching_options)
      : endpoint(std::move(endpoint_val)),
        client(new Client(endpoint, std::move(grpc_executor), std::move(batching_options)))
    {
      client->activate_object();
    }

    ~ClientHolder() noexcept
    {
      try
      {
        if (client->active())
        {
          client->deactivate_object();
          client->wait_object();
        }
      }
      catch (...)
      {
      }
    }

    const std::string endpoint;
    Client_var client;
  };

  struct UserInfoDistributedGrpcClient::RefHolder
  {
    explicit RefHolder(ClientHolderPtr client_holder_val)
      : client_holder(std::move(client_holder_val))
    {}

    AdServer::Grpc::Stats stats() const noexcept
    {
      return static_cast<UserInfoManagerGrpcAsyncClient*>(
        client_holder->client.in())->stats();
    }

    ClientHolderPtr client_holder;
  };

  class UserInfoDistributedGrpcClient::ResolveRefsTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    explicit ResolveRefsTask(UserInfoDistributedGrpcClient* client) noexcept
      : client_(client)
    {}

    void execute() noexcept override
    {
      while (!client_->deactivated_.load(std::memory_order_acquire))
      {
        client_->resolve_refs_();
        client_->wait_next_resolve_();
      }
    }

  private:
    ~ResolveRefsTask() noexcept override = default;

    UserInfoDistributedGrpcClient* client_;
  };

  UserInfoDistributedGrpcClient::UserInfoDistributedGrpcClient(
    const UserInfoControllerRefs& user_info_controller_refs,
    AdServer::Grpc::BatchingOptions batching_options,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    Logging::Logger* logger)
    : user_info_controller_refs_(user_info_controller_refs),
      batching_options_(std::move(batching_options)),
      grpc_executor_(std::move(grpc_executor)),
      pool_timeout_(DEFAULT_POOL_TIMEOUT),
      resolve_period_(DEFAULT_RESOLVE_PERIOD),
      callback_(new Logging::ActiveObjectCallbackImpl(
        logger,
        "UserInfoDistributedGrpcClient",
        "UserInfo")),
      task_runner_(new Generics::TaskRunner(callback_, 1)),
      logger_(ReferenceCounting::add_ref(logger))
  {
    if (user_info_controller_refs_.empty())
    {
      throw Exception("UserInfoDistributedGrpcClient: empty UserInfoController2 refs");
    }

    add_child_object(task_runner_);
  }

  void
  UserInfoDistributedGrpcClient::activate_object_()
  {
    deactivated_.store(false, std::memory_order_release);
    Generics::CompositeActiveObject::activate_object_();
    resolve_refs_();
    task_runner_->enqueue_task(Generics::Task_var(new ResolveRefsTask(this)));
  }

  void
  UserInfoDistributedGrpcClient::deactivate_object_()
  {
    deactivated_.store(true, std::memory_order_release);
    resolve_cond_.notify_all();

    Generics::CompositeActiveObject::deactivate_object_();

    std::vector<PoolPtr> old_pools;
    {
      std::unique_lock<std::shared_mutex> lock(pool_lock_);
      for (auto& chunk_pool : chunk_pools_)
      {
        old_pools.emplace_back(std::move(chunk_pool.second));
      }
      if (any_pool_)
      {
        old_pools.emplace_back(std::move(any_pool_));
      }
      chunk_pools_.clear();
      chunks_number_ = 0;
      refs_state_.clear();
      current_client_holders_.clear();
      client_holders_.clear();
    }

    std::set<Pool*> seen_pools;
    for (auto& pool : old_pools)
    {
      if (pool && seen_pools.insert(pool.get()).second)
      {
        pool->deactivate_object();
        pool->wait_object();
      }
    }
  }

  AdServer::Grpc::Stats
  UserInfoDistributedGrpcClient::stats() const noexcept
  {
    std::vector<ClientHolderPtr> client_holders;
    {
      std::shared_lock<std::shared_mutex> lock(pool_lock_);
      client_holders = current_client_holders_;
    }

    AdServer::Grpc::Stats result;
    std::set<const ClientHolder*> seen_clients;
    for (const auto& client_holder : client_holders)
    {
      if (client_holder && seen_clients.insert(client_holder.get()).second)
      {
        merge_stats_(
          result,
          static_cast<UserInfoManagerGrpcAsyncClient*>(
            client_holder->client.in())->stats());
      }
    }
    return result;
  }

#define USER_INFO_ROUTE_USER(method_name, request_type, response_type, callback_type, user_expr) \
  void \
  UserInfoDistributedGrpcClient::method_name( \
    const request_type& request, \
    callback_type callback) \
  { \
    auto ref = get_ref_(user_expr); \
    if (!ref) \
    { \
      finish_with_unavailable_<response_type>( \
        std::move(callback), \
        "no available UserInfoManager grpc client"); \
      return; \
    } \
    auto pool_ref = std::move(*ref); \
    pool_ref->client_holder->client->method_name( \
      request, \
      [ \
        pool_ref = std::move(pool_ref), \
        callback = std::move(callback), \
        pool_timeout = pool_timeout_ \
      ](const grpc::Status& status, const response_type& response) mutable \
      { \
        if (!status.ok()) \
        { \
          pool_ref.mark_as_bad( \
            Generics::Time::get_time_of_day() + pool_timeout); \
        } \
        callback(status, response); \
      }); \
  }

#define USER_INFO_ROUTE_ANY(method_name, request_type, response_type, callback_type) \
  void \
  UserInfoDistributedGrpcClient::method_name( \
    const request_type& request, \
    callback_type callback) \
  { \
    auto ref = get_any_ref_(); \
    if (!ref) \
    { \
      finish_with_unavailable_<response_type>( \
        std::move(callback), \
        "no available UserInfoManager grpc client"); \
      return; \
    } \
    auto pool_ref = std::move(*ref); \
    pool_ref->client_holder->client->method_name( \
      request, \
      [ \
        pool_ref = std::move(pool_ref), \
        callback = std::move(callback), \
        pool_timeout = pool_timeout_ \
      ](const grpc::Status& status, const response_type& response) mutable \
      { \
        if (!status.ok()) \
        { \
          pool_ref.mark_as_bad( \
            Generics::Time::get_time_of_day() + pool_timeout); \
        } \
        callback(status, response); \
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
  UserInfoDistributedGrpcClient::get_ref_(const std::string& user_id) const noexcept
  {
    try
    {
      if (user_id.empty())
      {
        return get_any_ref_();
      }

      std::shared_lock<std::shared_mutex> lock(pool_lock_);
      if (!chunks_number_)
      {
        return std::nullopt;
      }

      const auto chunk_id = chunk_index_(user_id, chunks_number_);
      const auto it = chunk_pools_.find(chunk_id);
      if (it == chunk_pools_.end() || !it->second)
      {
        return std::nullopt;
      }

      return it->second->get_object();
    }
    catch (...)
    {
      return std::nullopt;
    }
  }

  std::optional<UserInfoDistributedGrpcClient::Pool::Ref>
  UserInfoDistributedGrpcClient::get_any_ref_() const noexcept
  {
    std::shared_lock<std::shared_mutex> lock(pool_lock_);
    return any_pool_ ? any_pool_->get_object() : std::nullopt;
  }

  void
  UserInfoDistributedGrpcClient::resolve_refs_() noexcept
  {
    ControllerRefsState refs_state;
    if (fill_refs_state_(refs_state))
    {
      update_pools_if_changed_(std::move(refs_state));
    }
  }

  bool
  UserInfoDistributedGrpcClient::fill_refs_state_(
    ControllerRefsState& refs_state) noexcept
  {
    bool ok = false;

    for (const auto& controller_ref : user_info_controller_refs_)
    {
      std::vector<std::string> endpoints;
      try
      {
        auto stub = adserver::user_info_svcs::user_info_controller::
          UserInfoControllerGrpc::NewStub(
            grpc::CreateChannel(
              controller_ref,
              grpc::InsecureChannelCredentials()));

        grpc::ClientContext context;
        set_deadline_(context);
        adserver::user_info_svcs::user_info_controller::
          GetSessionDescriptionRequest request;
        adserver::user_info_svcs::user_info_controller::
          GetSessionDescriptionResponse response;

        const auto status = stub->get_session_description(
          &context,
          request,
          &response);

        if (!status.ok())
        {
          Stream::Error ostr;
          ostr << "get_session_description failed: " <<
            status.error_message();
          throw Exception(ostr);
        }

        for (const auto& manager : response.user_info_managers())
        {
          endpoints.emplace_back(manager.user_info_manager_endpoint());
        }
        std::sort(endpoints.begin(), endpoints.end());
        endpoints.erase(
          std::unique(endpoints.begin(), endpoints.end()),
          endpoints.end());
        ok = true;
      }
      catch (const eh::Exception& ex)
      {
        if (logger_)
        {
          logger_->sstream(
            Logging::Logger::ERROR,
            "UserInfoDistributedGrpcClient",
            "ADS-IMPL-72") <<
            "Can't resolve UserInfoController2 '" << controller_ref <<
            "': " << ex.what();
        }
      }

      refs_state.emplace_back(controller_ref, std::move(endpoints));
    }

    return ok;
  }

  bool
  UserInfoDistributedGrpcClient::update_pools_if_changed_(
    ControllerRefsState refs_state) noexcept
  {
    {
      std::shared_lock<std::shared_mutex> lock(pool_lock_);
      if (refs_state == refs_state_)
      {
        return false;
      }
    }

    std::map<unsigned long, std::vector<std::shared_ptr<RefHolder>>> chunk_refs;
    std::vector<std::shared_ptr<RefHolder>> any_refs;
    std::vector<ClientHolderPtr> client_holders;
    unsigned long chunks_number = 0;

    for (const auto& controller_ref : user_info_controller_refs_)
    {
      try
      {
        auto stub = adserver::user_info_svcs::user_info_controller::
          UserInfoControllerGrpc::NewStub(
            grpc::CreateChannel(
              controller_ref,
              grpc::InsecureChannelCredentials()));

        grpc::ClientContext context;
        set_deadline_(context);
        adserver::user_info_svcs::user_info_controller::
          GetSessionDescriptionRequest request;
        adserver::user_info_svcs::user_info_controller::
          GetSessionDescriptionResponse response;

        const auto status = stub->get_session_description(
          &context,
          request,
          &response);

        if (!status.ok())
        {
          continue;
        }

        for (const auto& manager : response.user_info_managers())
        {
          auto client_holder =
            get_or_create_client_holder_(manager.user_info_manager_endpoint());
          client_holders.emplace_back(client_holder);
          auto ref_holder = std::make_shared<RefHolder>(client_holder);
          any_refs.emplace_back(ref_holder);

          for (const auto chunk_id : manager.chunk_ids())
          {
            chunk_refs[chunk_id].emplace_back(ref_holder);
            chunks_number = std::max<unsigned long>(
              chunks_number,
              static_cast<unsigned long>(chunk_id) + 1);
          }
        }
      }
      catch (...)
      {
      }
    }

    if (chunk_refs.empty() || chunks_number == 0)
    {
      return false;
    }

    std::map<unsigned long, PoolPtr> new_chunk_pools;
    std::vector<PoolPtr> new_pools;
    for (auto& chunk_ref : chunk_refs)
    {
      auto pool = std::make_shared<Pool>();
      pool->set_refs(chunk_ref.second);
      pool->activate_object();
      new_chunk_pools.emplace(chunk_ref.first, pool);
      new_pools.emplace_back(pool);
    }

    auto new_any_pool = std::make_shared<Pool>();
    new_any_pool->set_refs(any_refs);
    new_any_pool->activate_object();

    std::vector<PoolPtr> old_pools;
    {
      std::unique_lock<std::shared_mutex> lock(pool_lock_);
      for (auto& chunk_pool : chunk_pools_)
      {
        old_pools.emplace_back(std::move(chunk_pool.second));
      }
      if (any_pool_)
      {
        old_pools.emplace_back(std::move(any_pool_));
      }

      chunk_pools_.swap(new_chunk_pools);
      any_pool_.swap(new_any_pool);
      chunks_number_ = chunks_number;
      refs_state_ = std::move(refs_state);
      current_client_holders_ = std::move(client_holders);
    }

    std::set<Pool*> seen_pools;
    for (auto& pool : old_pools)
    {
      if (pool && seen_pools.insert(pool.get()).second)
      {
        pool->deactivate_object();
        pool->wait_object();
      }
    }

    return true;
  }

  UserInfoDistributedGrpcClient::ClientHolderPtr
  UserInfoDistributedGrpcClient::get_or_create_client_holder_(
    const std::string& endpoint)
  {
    auto it = client_holders_.find(endpoint);
    if (it != client_holders_.end())
    {
      if (auto client_holder = it->second.lock())
      {
        return client_holder;
      }
    }

    auto client_holder = std::make_shared<ClientHolder>(
      endpoint,
      grpc_executor_,
      batching_options_);
    client_holders_[endpoint] = client_holder;
    return client_holder;
  }

  void
  UserInfoDistributedGrpcClient::wait_next_resolve_() noexcept
  {
    std::unique_lock<std::mutex> lock(resolve_lock_);
    resolve_cond_.wait_for(
      lock,
      std::chrono::microseconds(resolve_period_.microseconds()),
      [this]() noexcept
      {
        return deactivated_.load(std::memory_order_acquire);
      });
  }

  unsigned long
  UserInfoDistributedGrpcClient::chunk_index_(
    const std::string& user_id,
    unsigned long chunks_number)
  {
    return AdServer::Commons::uuid_distribution_hash(
      GrpcAlgs::unpack_user_id(user_id)) % chunks_number;
  }

  void
  UserInfoDistributedGrpcClient::merge_stats_(
    AdServer::Grpc::Stats& result,
    const AdServer::Grpc::Stats& source) noexcept
  {
    result.write_batches += source.write_batches;
    result.write_items += source.write_items;
    result.queue_wait_count += source.queue_wait_count;
    result.queue_wait_sum_us += source.queue_wait_sum_us;
    result.queue_wait_max_us =
      std::max(result.queue_wait_max_us, source.queue_wait_max_us);
    result.response_wait_count += source.response_wait_count;
    result.response_wait_sum_us += source.response_wait_sum_us;
    result.response_wait_max_us =
      std::max(result.response_wait_max_us, source.response_wait_max_us);
    result.max_streams = std::max(result.max_streams, source.max_streams);
    if (source.consumer_stream_write.has_value())
    {
      if (!result.consumer_stream_write.has_value())
      {
        result.consumer_stream_write = AdServer::Grpc::Stats::ConsumerStreamWrite();
      }
      result.consumer_stream_write->count += source.consumer_stream_write->count;
      result.consumer_stream_write->sum_us += source.consumer_stream_write->sum_us;
      result.consumer_stream_write->max_us = std::max(
        result.consumer_stream_write->max_us,
        source.consumer_stream_write->max_us);
    }
  }
}
