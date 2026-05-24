#include "ChannelDistributedGrpcClient.hpp"

#include <algorithm>
#include <chrono>
#include <set>
#include <utility>

#include <grpcpp/grpcpp.h>

#include <Logger/ActiveObjectCallback.hpp>
#include <ChannelSvcs/ChannelController/ChannelControllerGrpc.grpc.pb.h>

namespace AdServer::ChannelSvcs
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
  }

  struct ChannelDistributedGrpcClient::ClientHolder
  {
    ClientHolder(
      std::string endpoint_val,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
        coalesce_runner,
      AdServer::Grpc::BatchingOptions batching_options)
      : endpoint(std::move(endpoint_val)),
        client(std::make_shared<Client>(
          endpoint,
          std::move(grpc_executor),
          std::move(coalesce_runner),
          std::move(batching_options)))
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
    ClientPtr client;
  };

  struct ChannelDistributedGrpcClient::RefHolder
  {
    explicit RefHolder(ClientHolderPtr client_holder_val)
      : client_holder(std::move(client_holder_val))
    {}

    void match(
      const adserver::channel_svcs::channel_server::MatchRequest& request,
      MatchCallback callback)
    {
      client_holder->client->match(request, std::move(callback));
    }

    void get_ccg_traits(
      const adserver::channel_svcs::channel_server::GetCcgTraitsRequest& request,
      GetCcgTraitsCallback callback)
    {
      client_holder->client->get_ccg_traits(request, std::move(callback));
    }

    void set_sources(
      const adserver::channel_svcs::channel_server::SetSourcesRequest& request,
      SetSourcesCallback callback)
    {
      client_holder->client->set_sources(request, std::move(callback));
    }

    void set_proxy_sources(
      const adserver::channel_svcs::channel_server::SetProxySourcesRequest&
        request,
      SetProxySourcesCallback callback)
    {
      client_holder->client->set_proxy_sources(request, std::move(callback));
    }

    AdServer::Grpc::Stats stats() const noexcept
    {
      return static_cast<ChannelServerGrpcAsyncClient*>(
        client_holder->client.get())->stats();
    }

    const std::string& name() const noexcept
    {
      return client_holder->endpoint;
    }

    ClientHolderPtr client_holder;
  };

  class ChannelDistributedGrpcClient::ResolveRefsTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    explicit ResolveRefsTask(ChannelDistributedGrpcClient* client) noexcept
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

    ChannelDistributedGrpcClient* client_;
  };

  ChannelDistributedGrpcClient::ChannelDistributedGrpcClient(
    const ChannelControllerRefs& channel_controller_refs,
    AdServer::Grpc::BatchingOptions batching_options,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner)
    : channel_controller_refs_(channel_controller_refs),
      batching_options_(std::move(batching_options)),
      grpc_executor_(std::move(grpc_executor)),
      coalesce_runner_(std::move(coalesce_runner)),
      pool_timeout_(DEFAULT_POOL_TIMEOUT),
      resolve_period_(DEFAULT_RESOLVE_PERIOD),
      callback_(new Logging::ActiveObjectCallbackImpl(
        nullptr,
        "ChannelDistributedGrpcClient",
        "ChannelSvcs")),
      task_runner_(new Generics::TaskRunner(callback_, 1))
  {
    if (channel_controller_refs_.empty())
    {
      throw Exception(
        "ChannelDistributedGrpcClient: empty ChannelController refs");
    }

    if (!coalesce_runner_)
    {
      coalesce_runner_ =
        std::make_shared<AdServer::Commons::BoostAsioContextRunActiveObject>(
          callback_,
          std::make_shared<boost::asio::io_service>(),
          std::max<unsigned long>(1, batching_options_.workers_number));
      add_child_object(coalesce_runner_);
    }

    add_child_object(task_runner_);
  }

  void
  ChannelDistributedGrpcClient::activate_object_()
  {
    deactivated_.store(false, std::memory_order_release);
    Generics::CompositeActiveObject::activate_object_();
    resolve_refs_();
    task_runner_->enqueue_task(
      Generics::Task_var(new ResolveRefsTask(this)));
  }

  void
  ChannelDistributedGrpcClient::deactivate_object_()
  {
    deactivated_.store(true, std::memory_order_release);
    resolve_cond_.notify_all();

    Generics::CompositeActiveObject::deactivate_object_();

    {
      std::unique_lock<std::shared_mutex> lock(pool_lock_);
      shutdown_pool_ = std::move(pool_);
      shutdown_client_holders_ = std::move(current_client_holders_);
      refs_state_.clear();
      client_holders_.clear();
    }

    for (const auto& client_holder : shutdown_client_holders_)
    {
      client_holder->client->deactivate_object();
    }

    if (shutdown_pool_)
    {
      shutdown_pool_->deactivate_object();
    }
  }

  void
  ChannelDistributedGrpcClient::wait_object_()
  {
    Generics::CompositeActiveObject::wait_object_();

    if (shutdown_pool_)
    {
      shutdown_pool_->wait_object();
      shutdown_pool_.reset();
    }

    for (const auto& client_holder : shutdown_client_holders_)
    {
      client_holder->client->wait_object();
    }
    shutdown_client_holders_.clear();
  }

  AdServer::Grpc::Stats
  ChannelDistributedGrpcClient::stats() const noexcept
  {
    std::vector<ClientHolderPtr> client_holders;
    {
      std::shared_lock<std::shared_mutex> lock(pool_lock_);
      client_holders = current_client_holders_;
    }

    AdServer::Grpc::Stats result = AdServer::Grpc::Client::stats();
    std::set<const ClientHolder*> seen_clients;
    for (const auto& client_holder : client_holders)
    {
      if (!client_holder || !seen_clients.insert(client_holder.get()).second)
      {
        continue;
      }

      merge_stats_(
        result,
        static_cast<ChannelServerGrpcAsyncClient*>(
          client_holder->client.get())->stats());
    }
    return result;
  }

  void
  ChannelDistributedGrpcClient::match(
    const adserver::channel_svcs::channel_server::MatchRequest& request,
    MatchCallback callback)
  {
    add_input_stats(1);

    if (!active())
    {
      add_completed_stats(true);
      callback(
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "inactive"),
        adserver::channel_svcs::channel_server::MatchResponse());
      return;
    }

    auto ref = get_ref_();
    if (!ref)
    {
      add_completed_stats(true);
      callback(
        grpc::Status(
          grpc::StatusCode::UNAVAILABLE,
          "no available ChannelServer grpc client"),
        adserver::channel_svcs::channel_server::MatchResponse());
      return;
    }

    (*ref)->match(
      request,
      [
        ref = std::move(*ref),
        callback = std::move(callback),
        pool_timeout = pool_timeout_
      ]
      (
        const grpc::Status& status,
        const adserver::channel_svcs::channel_server::MatchResponse& response
      )
      mutable
      {
        if (!status.ok())
        {
          ref.mark_as_bad(
            Generics::Time::get_time_of_day() + pool_timeout);
        }
        callback(status, response);
      });
  }

  void
  ChannelDistributedGrpcClient::get_ccg_traits(
    const adserver::channel_svcs::channel_server::GetCcgTraitsRequest& request,
    GetCcgTraitsCallback callback)
  {
    add_input_stats(1);

    if (!active())
    {
      add_completed_stats(true);
      callback(
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "inactive"),
        adserver::channel_svcs::channel_server::GetCcgTraitsResponse());
      return;
    }

    auto ref = get_ref_();
    if (!ref)
    {
      add_completed_stats(true);
      callback(
        grpc::Status(
          grpc::StatusCode::UNAVAILABLE,
          "no available ChannelServer grpc client"),
        adserver::channel_svcs::channel_server::GetCcgTraitsResponse());
      return;
    }

    (*ref)->get_ccg_traits(
      request,
      [
        ref = std::move(*ref),
        callback = std::move(callback),
        pool_timeout = pool_timeout_
      ](
        const grpc::Status& status,
        const adserver::channel_svcs::channel_server::GetCcgTraitsResponse&
          response)
      mutable
      {
        if (!status.ok())
        {
          ref.mark_as_bad(
            Generics::Time::get_time_of_day() + pool_timeout);
        }
        callback(status, response);
      });
  }

  void
  ChannelDistributedGrpcClient::set_sources(
    const adserver::channel_svcs::channel_server::SetSourcesRequest& request,
    SetSourcesCallback callback)
  {
    add_input_stats(1);

    if (!active())
    {
      add_completed_stats(true);
      callback(
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "inactive"),
        adserver::channel_svcs::channel_server::SetSourcesResponse());
      return;
    }

    auto ref = get_ref_();
    if (!ref)
    {
      add_completed_stats(true);
      callback(
        grpc::Status(
          grpc::StatusCode::UNAVAILABLE,
          "no available ChannelServer grpc client"),
        adserver::channel_svcs::channel_server::SetSourcesResponse());
      return;
    }

    (*ref)->set_sources(
      request,
      [
        ref = std::move(*ref),
        callback = std::move(callback),
        pool_timeout = pool_timeout_
      ](
        const grpc::Status& status,
        const adserver::channel_svcs::channel_server::SetSourcesResponse&
          response)
      mutable
      {
        if (!status.ok())
        {
          ref.mark_as_bad(
            Generics::Time::get_time_of_day() + pool_timeout);
        }
        callback(status, response);
      });
  }

  void
  ChannelDistributedGrpcClient::set_proxy_sources(
    const adserver::channel_svcs::channel_server::SetProxySourcesRequest&
      request,
    SetProxySourcesCallback callback)
  {
    add_input_stats(1);

    if (!active())
    {
      add_completed_stats(true);
      callback(
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "inactive"),
        adserver::channel_svcs::channel_server::SetProxySourcesResponse());
      return;
    }

    auto ref = get_ref_();
    if (!ref)
    {
      add_completed_stats(true);
      callback(
        grpc::Status(
          grpc::StatusCode::UNAVAILABLE,
          "no available ChannelServer grpc client"),
        adserver::channel_svcs::channel_server::SetProxySourcesResponse());
      return;
    }

    (*ref)->set_proxy_sources(
      request,
      [
        ref = std::move(*ref),
        callback = std::move(callback),
        pool_timeout = pool_timeout_
      ](
        const grpc::Status& status,
        const adserver::channel_svcs::channel_server::SetProxySourcesResponse&
          response)
      mutable
      {
        if (!status.ok())
        {
          ref.mark_as_bad(
            Generics::Time::get_time_of_day() + pool_timeout);
        }
        callback(status, response);
      });
  }

  std::optional<ChannelDistributedGrpcClient::Pool::Ref>
  ChannelDistributedGrpcClient::get_ref_() const noexcept
  {
    PoolPtr pool;
    {
      std::shared_lock<std::shared_mutex> lock(pool_lock_);
      pool = pool_;
    }

    return pool ? pool->get_object() : std::nullopt;
  }

  void
  ChannelDistributedGrpcClient::resolve_refs_() noexcept
  {
    ControllerRefsState refs_state;
    if (fill_refs_state_(refs_state))
    {
      update_pool_if_changed_(std::move(refs_state));
    }
  }

  bool
  ChannelDistributedGrpcClient::fill_refs_state_(
    ControllerRefsState& refs_state) noexcept
  {
    namespace ControllerProto =
      adserver::channel_svcs::channel_controller;

    refs_state.clear();
    refs_state.reserve(channel_controller_refs_.size());

    try
    {
      for (const auto& controller_ref : channel_controller_refs_)
      {
        auto channel = grpc::CreateChannel(
          controller_ref,
          grpc::InsecureChannelCredentials());
        auto stub = ControllerProto::ChannelControllerGrpc::NewStub(channel);

        grpc::ClientContext context;
        set_deadline_(context);
        ControllerProto::GetSessionDescriptionRequest request;
        ControllerProto::GetSessionDescriptionResponse response;

        const auto status =
          stub->get_session_description(&context, request, &response);
        if (!status.ok())
        {
          return false;
        }

        std::vector<std::string> server_refs;
        for (const auto& group : response.channel_server_groups())
        {
          for (const auto& server : group.channel_servers())
          {
            server_refs.emplace_back(server.channel_server_endpoint());
          }
        }
        std::sort(server_refs.begin(), server_refs.end());
        refs_state.emplace_back(controller_ref, std::move(server_refs));
      }
    }
    catch (...)
    {
      return false;
    }

    return true;
  }

  bool
  ChannelDistributedGrpcClient::update_pool_if_changed_(
    ControllerRefsState refs_state) noexcept
  {
    try
    {
      std::vector<std::shared_ptr<RefHolder>> refs;
      std::vector<ClientHolderPtr> client_holders;
      for (const auto& controller_refs : refs_state)
      {
        for (const auto& endpoint : controller_refs.second)
        {
          auto client_holder = get_or_create_client_holder_(endpoint);
          if (!client_holder)
          {
            return false;
          }

          refs.emplace_back(std::make_shared<RefHolder>(client_holder));
          client_holders.emplace_back(std::move(client_holder));
        }
      }

      PoolPtr new_pool;
      PoolPtr old_pool;
      {
        std::unique_lock<std::shared_mutex> lock(pool_lock_);
        if (deactivated_.load(std::memory_order_acquire) ||
          refs_state_ == refs_state)
        {
          return false;
        }

        new_pool = std::make_shared<Pool>(
          std::move(refs),
          coalesce_runner_);
        new_pool->activate_object();

        old_pool = std::move(pool_);
        pool_ = std::move(new_pool);
        refs_state_ = std::move(refs_state);
        current_client_holders_ = std::move(client_holders);
      }

      if (old_pool)
      {
        old_pool->deactivate_object();
        old_pool->wait_object();
      }
      return true;
    }
    catch (...)
    {
      return false;
    }
  }

  ChannelDistributedGrpcClient::ClientHolderPtr
  ChannelDistributedGrpcClient::get_or_create_client_holder_(
    const std::string& endpoint)
  {
    if (deactivated_.load(std::memory_order_acquire))
    {
      return ClientHolderPtr();
    }

    std::unique_lock<std::shared_mutex> lock(pool_lock_);
    auto& weak_client_holder = client_holders_[endpoint];
    auto client_holder = weak_client_holder.lock();
    if (!client_holder)
    {
      client_holder = std::make_shared<ClientHolder>(
        endpoint,
        grpc_executor_,
        coalesce_runner_,
        batching_options_);
      weak_client_holder = client_holder;
    }
    return client_holder;
  }

  void
  ChannelDistributedGrpcClient::wait_next_resolve_() noexcept
  {
    std::unique_lock<std::mutex> lock(resolve_lock_);
    resolve_cond_.wait_for(
      lock,
      std::chrono::microseconds(resolve_period_.microseconds()),
      [this]() {
        return deactivated_.load(std::memory_order_acquire);
      });
  }

  void
  ChannelDistributedGrpcClient::merge_stats_(
    AdServer::Grpc::Stats& result,
    const AdServer::Grpc::Stats& source) noexcept
  {
    result.write_batches += source.write_batches;
    result.write_items += source.write_items;
    result.read_batches += source.read_batches;
    result.read_items += source.read_items;
    result.completed_items += source.completed_items;
    result.completed_error_items += source.completed_error_items;
    result.queue_wait_count += source.queue_wait_count;
    result.queue_wait_sum_us += source.queue_wait_sum_us;
    result.queue_wait_max_us =
      std::max(result.queue_wait_max_us, source.queue_wait_max_us);
    result.queue_timeout_count += source.queue_timeout_count;
    result.response_wait_count += source.response_wait_count;
    result.response_wait_sum_us += source.response_wait_sum_us;
    result.response_wait_max_us =
      std::max(result.response_wait_max_us, source.response_wait_max_us);
    result.timing_coalesce_items += source.timing_coalesce_items;
    result.max_streams = std::max(result.max_streams, source.max_streams);
    result.inflight_items += source.inflight_items;
    result.stream_inflight_items += source.stream_inflight_items;
    result.queue_items += source.queue_items;
    result.pending_batches += source.pending_batches;
    result.pending_batch_items += source.pending_batch_items;
    result.active_streams += source.active_streams;
    result.available_streams += source.available_streams;
    result.connecting_streams += source.connecting_streams;
    result.draining_streams += source.draining_streams;
    result.deferred_streams += source.deferred_streams;
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
