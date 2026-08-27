#include "ChannelDistributedGrpcClient.hpp"

#include <algorithm>
#include <chrono>
#include <map>
#include <set>
#include <utility>

#include <grpcpp/grpcpp.h>

#include <Logger/ActiveObjectCallback.hpp>
#include <Stream/MemoryStream.hpp>
#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/ResponseHolder.hpp>
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

    std::string group_name_(const std::vector<std::string>& group)
    {
      std::string result;
      for (const auto& endpoint : group)
      {
        if (!result.empty())
        {
          result += ",";
        }
        result += endpoint;
      }
      return result;
    }

    void append_channel_atoms_(
      google::protobuf::RepeatedPtrField<
        adserver::channel_svcs::channel_server::ChannelAtom>* target,
      const google::protobuf::RepeatedPtrField<
        adserver::channel_svcs::channel_server::ChannelAtom>& source)
    {
      for (const auto& atom : source)
      {
        *target->Add() = atom;
      }
    }

    void merge_match_response_(
      adserver::channel_svcs::channel_server::MatchResponse& target,
      const adserver::channel_svcs::channel_server::MatchResponse& source)
    {
      auto* target_channels = target.mutable_matched_channels();
      const auto& source_channels = source.matched_channels();

      append_channel_atoms_(
        target_channels->mutable_page_channels(),
        source_channels.page_channels());
      append_channel_atoms_(
        target_channels->mutable_search_channels(),
        source_channels.search_channels());
      append_channel_atoms_(
        target_channels->mutable_url_channels(),
        source_channels.url_channels());
      append_channel_atoms_(
        target_channels->mutable_url_keyword_channels(),
        source_channels.url_keyword_channels());

      for (const auto channel_id : source_channels.uid_channels())
      {
        target_channels->add_uid_channels(channel_id);
      }

      std::map<std::uint32_t, std::uint32_t> content_weights;
      for (const auto& channel : target.content_channels())
      {
        content_weights[channel.id()] += channel.weight();
      }
      for (const auto& channel : source.content_channels())
      {
        content_weights[channel.id()] += channel.weight();
      }
      target.clear_content_channels();
      for (const auto& [channel_id, weight] : content_weights)
      {
        auto* channel = target.add_content_channels();
        channel->set_id(channel_id);
        channel->set_weight(weight);
      }

      target.set_no_adv(target.no_adv() || source.no_adv());
      target.set_no_track(target.no_track() || source.no_track());
      target.set_full_loaded(target.full_loaded() && source.full_loaded());
      if (!source.hostname().empty())
      {
        if (target.hostname().empty())
        {
          target.set_hostname(source.hostname());
        }
        else
        {
          target.set_hostname(target.hostname() + "," + source.hostname());
        }
      }

      if (!source.match_time().empty())
      {
        if (target.match_time().empty())
        {
          target.set_match_time(source.match_time());
        }
      }
    }

    void compose_traits_response_(
      adserver::channel_svcs::channel_server::GetCcgTraitsResponse& target,
      const std::vector<
        adserver::channel_svcs::channel_server::GetCcgTraitsResponse>& sources)
    {
      std::set<std::uint64_t> negative_ccgs;
      for (const auto& source : sources)
      {
        for (const auto ccg_id : source.neg_ccg())
        {
          if (negative_ccgs.insert(ccg_id).second)
          {
            target.add_neg_ccg(ccg_id);
          }
        }
      }

      for (const auto& source : sources)
      {
        for (const auto& keyword : source.ccg_keywords())
        {
          if (!negative_ccgs.count(keyword.ccg_id()))
          {
            *target.add_ccg_keywords() = keyword;
          }
        }

        if (!source.hostname().empty())
        {
          if (target.hostname().empty())
          {
            target.set_hostname(source.hostname());
          }
          else
          {
            target.set_hostname(target.hostname() + "," + source.hostname());
          }
        }
      }
    }
  }

  struct ChannelDistributedGrpcClient::ControllerClient
  {
    using ControllerGrpc = adserver::channel_svcs::channel_controller::ChannelControllerGrpc;

    explicit ControllerClient(const std::string& endpoint)
      : endpoint(endpoint),
        name(endpoint),
        channel(AdServer::Grpc::create_channel(endpoint, grpc::InsecureChannelCredentials())),
        stub(ControllerGrpc::NewStub(channel))
    {}

    void reset()
    {
      channel = AdServer::Grpc::create_channel(endpoint, grpc::InsecureChannelCredentials());
      stub = ControllerGrpc::NewStub(channel);
    }

    const std::string endpoint;
    const std::string name;
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<ControllerGrpc::Stub> stub;
  };

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
    explicit RefHolder(std::vector<ClientHolderPtr> client_holders_val)
      : client_holders(std::move(client_holders_val))
    {
      for (const auto& client_holder : client_holders)
      {
        if (!name_value.empty())
        {
          name_value += ",";
        }
        name_value += client_holder->endpoint;
      }
    }

    void match(
      const adserver::channel_svcs::channel_server::MatchRequest& request,
      MatchCallback callback)
    {
      if (client_holders.empty())
      {
        callback(
          grpc::Status(grpc::StatusCode::UNAVAILABLE, "empty ChannelServer grpc group"),
          AdServer::Grpc::ResponseHolder<
            adserver::channel_svcs::channel_server::MatchResponse>::
              make_value(adserver::channel_svcs::channel_server::MatchResponse()));
        return;
      }

      if (client_holders.size() == 1)
      {
        const auto client_holder = client_holders.front();
        client_holder->client->match(
          request,
          [
            callback = std::move(callback)
          ](
            const grpc::Status& status,
            AdServer::Grpc::ResponseHolder<
              adserver::channel_svcs::channel_server::MatchResponse>&&
                response_holder) mutable
          {
            callback(status, std::move(response_holder));
          });
        return;
      }

      struct MatchState
      {
        explicit MatchState(std::size_t remaining_val)
          : remaining(remaining_val)
        {
          response.set_full_loaded(true);
        }

        std::mutex lock;
        std::size_t remaining;
        std::size_t errors = 0;
        grpc::Status last_error;
        adserver::channel_svcs::channel_server::MatchResponse response;
        MatchCallback callback;
      };

      const auto group_size = client_holders.size();
      auto state = std::make_shared<MatchState>(group_size);
      state->callback = std::move(callback);
      for (const auto& client_holder : client_holders)
      {
        client_holder->client->match(
          request,
          [state, endpoint = client_holder->endpoint, group_size](
            const grpc::Status& status,
            AdServer::Grpc::ResponseHolder<
              adserver::channel_svcs::channel_server::MatchResponse>&&
                response_holder)
          {
            MatchCallback callback;
            grpc::Status result_status = grpc::Status::OK;
            adserver::channel_svcs::channel_server::MatchResponse
              result_response;
            {
              std::lock_guard<std::mutex> guard(state->lock);
              if (status.ok())
              {
                merge_match_response_(state->response, response_holder.get());
              }
              else
              {
                ++state->errors;
                state->last_error = AdServer::Grpc::status_with_endpoint(status, endpoint);
              }

              if (--state->remaining != 0)
              {
                return;
              }

              if (state->errors == 0 || state->errors < group_size)
              {
                result_response = std::move(state->response);
              }
              else
              {
                result_status = state->last_error;
              }
              callback = std::move(state->callback);
            }

            callback(
              result_status,
              AdServer::Grpc::ResponseHolder<
                adserver::channel_svcs::channel_server::MatchResponse>::
                  make_value(std::move(result_response)));
          });
      }
    }

    void get_ccg_traits(
      const adserver::channel_svcs::channel_server::GetCcgTraitsRequest& request,
      GetCcgTraitsCallback callback)
    {
      if (client_holders.empty())
      {
        callback(
          grpc::Status(grpc::StatusCode::UNAVAILABLE, "empty ChannelServer grpc group"),
          AdServer::Grpc::ResponseHolder<
            adserver::channel_svcs::channel_server::GetCcgTraitsResponse>::
              make_value(adserver::channel_svcs::channel_server::GetCcgTraitsResponse()));
        return;
      }

      struct TraitsState
      {
        explicit TraitsState(std::size_t remaining_val)
          : remaining(remaining_val)
        {}

        std::mutex lock;
        std::size_t remaining;
        std::size_t errors = 0;
        grpc::Status last_error;
        std::vector<
          adserver::channel_svcs::channel_server::GetCcgTraitsResponse>
            responses;
        GetCcgTraitsCallback callback;
      };

      const auto group_size = client_holders.size();
      auto state = std::make_shared<TraitsState>(group_size);
      state->callback = std::move(callback);
      state->responses.reserve(group_size);
      for (const auto& client_holder : client_holders)
      {
        client_holder->client->get_ccg_traits(
          request,
          [state, endpoint = client_holder->endpoint, group_size](
            const grpc::Status& status,
            AdServer::Grpc::ResponseHolder<
              adserver::channel_svcs::channel_server::GetCcgTraitsResponse>&&
                response_holder)
          {
            GetCcgTraitsCallback callback;
            grpc::Status result_status = grpc::Status::OK;
            adserver::channel_svcs::channel_server::GetCcgTraitsResponse
              result_response;
            {
              std::lock_guard<std::mutex> guard(state->lock);
              if (status.ok())
              {
                state->responses.emplace_back(response_holder.get());
              }
              else
              {
                ++state->errors;
                state->last_error = AdServer::Grpc::status_with_endpoint(status, endpoint);
              }

              if (--state->remaining != 0)
              {
                return;
              }

              if (state->errors == 0 || state->errors < group_size)
              {
                compose_traits_response_(result_response, state->responses);
              }
              else
              {
                result_status = state->last_error;
              }
              callback = std::move(state->callback);
            }

            callback(
              result_status,
              AdServer::Grpc::ResponseHolder<
                adserver::channel_svcs::channel_server::GetCcgTraitsResponse>::
                  make_value(std::move(result_response)));
          });
      }
    }

    void check_configuration(
      const adserver::channel_svcs::channel_server::CheckConfigurationRequest&
        request,
      CheckConfigurationCallback callback)
    {
      client_holders.front()->client->check_configuration(request, std::move(callback));
    }

    void set_sources(
      const adserver::channel_svcs::channel_server::SetSourcesRequest& request,
      SetSourcesCallback callback)
    {
      client_holders.front()->client->set_sources(request, std::move(callback));
    }

    void set_proxy_sources(
      const adserver::channel_svcs::channel_server::SetProxySourcesRequest&
        request,
      SetProxySourcesCallback callback)
    {
      client_holders.front()->client->set_proxy_sources(request, std::move(callback));
    }

    AdServer::Grpc::Stats stats() const noexcept
    {
      return static_cast<ChannelServerGrpcAsyncClient*>(
          client_holders.front()->client.get())->stats();
    }

    const std::string& name() const noexcept
    {
      return name_value;
    }

    std::string name_value;
    std::vector<ClientHolderPtr> client_holders;
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
      coalesce_runner,
    Logging::Logger* logger)
    : channel_controller_refs_(channel_controller_refs),
      batching_options_(std::move(batching_options)),
      grpc_executor_(std::move(grpc_executor)),
      coalesce_runner_(std::move(coalesce_runner)),
      pool_timeout_(DEFAULT_POOL_TIMEOUT),
      resolve_period_(DEFAULT_RESOLVE_PERIOD),
      callback_(new Logging::ActiveObjectCallbackImpl(
        logger,
        "ChannelDistributedGrpcClient",
        "ChannelSvcs")),
      task_runner_(new Generics::TaskRunner(callback_, 1))
  {
    if (channel_controller_refs_.empty())
    {
      throw Exception("ChannelDistributedGrpcClient: empty ChannelController refs");
    }
    for (const auto& controller_group : channel_controller_refs_)
    {
      if (controller_group.empty())
      {
        throw Exception("ChannelDistributedGrpcClient: empty ChannelController ref group");
      }
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
    controller_pools_.reserve(channel_controller_refs_.size());
    for (const auto& controller_group : channel_controller_refs_)
    {
      std::vector<ControllerClientPtr> controller_clients;
      controller_clients.reserve(controller_group.size());
      for (const auto& endpoint : controller_group)
      {
        controller_clients.emplace_back(std::make_shared<ControllerClient>(endpoint));
      }
      auto controller_pool = std::make_shared<ControllerPool>(
        std::move(controller_clients),
        coalesce_runner_);
      add_child_object(controller_pool);
      controller_pools_.emplace_back(std::move(controller_pool));
    }
  }

  void
  ChannelDistributedGrpcClient::activate_object_()
  {
    deactivated_.store(false, std::memory_order_release);
    Generics::CompositeActiveObject::activate_object_();
    resolve_refs_();
    task_runner_->enqueue_task(Generics::Task_var(new ResolveRefsTask(this)));
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
      if (!seen_clients.insert(client_holder.get()).second)
      {
        continue;
      }

      merge_stats_(
        result,
        static_cast<ChannelServerGrpcAsyncClient*>(client_holder->client.get())->stats());
    }
    return result;
  }

  AdServer::Grpc::Client::EndpointStats
  ChannelDistributedGrpcClient::endpoint_stats() const noexcept
  {
    std::vector<ClientHolderPtr> client_holders;
    {
      std::shared_lock<std::shared_mutex> lock(pool_lock_);
      client_holders.reserve(current_client_holders_.size() + shutdown_client_holders_.size());
      client_holders.insert(
        client_holders.end(),
        current_client_holders_.begin(),
        current_client_holders_.end());
      client_holders.insert(
        client_holders.end(),
        shutdown_client_holders_.begin(),
        shutdown_client_holders_.end());
    }

    AdServer::Grpc::Client::EndpointStats result;
    result.reserve(client_holders.size());
    std::set<const ClientHolder*> seen_clients;
    for (const auto& client_holder : client_holders)
    {
      if (!seen_clients.insert(client_holder.get()).second)
      {
        continue;
      }

      result.emplace_back(
        client_holder->endpoint,
        static_cast<ChannelServerGrpcAsyncClient*>(client_holder->client.get())->stats());
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
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::MatchResponse>::
            make_value(adserver::channel_svcs::channel_server::MatchResponse()));
      return;
    }

    auto ref = get_ref_();
    if (!ref)
    {
      add_completed_stats(true);
      callback(
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "no available ChannelServer grpc client"),
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::MatchResponse>::
            make_value(adserver::channel_svcs::channel_server::MatchResponse()));
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
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::MatchResponse>&&
            response_holder
      )
      mutable
      {
        if (!status.ok() && !AdServer::Grpc::is_request_specific_error(status))
        {
          ref.mark_as_bad(Generics::Time::get_time_of_day() + pool_timeout);
        }
        callback(
          AdServer::Grpc::status_with_endpoint(status, ref->name()),
          std::move(response_holder));
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
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::GetCcgTraitsResponse>::
            make_value(adserver::channel_svcs::channel_server::GetCcgTraitsResponse()));
      return;
    }

    auto ref = get_ref_();
    if (!ref)
    {
      add_completed_stats(true);
      callback(
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "no available ChannelServer grpc client"),
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::GetCcgTraitsResponse>::
            make_value(adserver::channel_svcs::channel_server::GetCcgTraitsResponse()));
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
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::GetCcgTraitsResponse>&&
            response_holder)
      mutable
      {
        if (!status.ok() && !AdServer::Grpc::is_request_specific_error(status))
        {
          ref.mark_as_bad(Generics::Time::get_time_of_day() + pool_timeout);
        }
        callback(
          AdServer::Grpc::status_with_endpoint(status, ref->name()),
          std::move(response_holder));
      });
  }

  void
  ChannelDistributedGrpcClient::check_configuration(
    const adserver::channel_svcs::channel_server::CheckConfigurationRequest& request,
    CheckConfigurationCallback callback)
  {
    add_input_stats(1);

    if (!active())
    {
      add_completed_stats(true);
      callback(
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "inactive"),
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::CheckConfigurationResponse>::
            make_value(adserver::channel_svcs::channel_server::CheckConfigurationResponse()));
      return;
    }

    auto ref = get_ref_();
    if (!ref)
    {
      add_completed_stats(true);
      callback(
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "no available ChannelServer grpc client"),
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::CheckConfigurationResponse>::
            make_value(adserver::channel_svcs::channel_server::CheckConfigurationResponse()));
      return;
    }

    (*ref)->check_configuration(
      request,
      [
        ref = std::move(*ref),
        callback = std::move(callback),
        pool_timeout = pool_timeout_
      ](
        const grpc::Status& status,
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::CheckConfigurationResponse>&&
            response_holder)
      mutable
      {
        if (!status.ok() && !AdServer::Grpc::is_request_specific_error(status))
        {
          ref.mark_as_bad(Generics::Time::get_time_of_day() + pool_timeout);
        }
        callback(status, std::move(response_holder));
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
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::SetSourcesResponse>::
            make_value(adserver::channel_svcs::channel_server::SetSourcesResponse()));
      return;
    }

    auto ref = get_ref_();
    if (!ref)
    {
      add_completed_stats(true);
      callback(
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "no available ChannelServer grpc client"),
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::SetSourcesResponse>::
            make_value(adserver::channel_svcs::channel_server::SetSourcesResponse()));
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
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::SetSourcesResponse>&&
            response_holder)
      mutable
      {
        if (!status.ok() && !AdServer::Grpc::is_request_specific_error(status))
        {
          ref.mark_as_bad(Generics::Time::get_time_of_day() + pool_timeout);
        }
        callback(status, std::move(response_holder));
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
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::SetProxySourcesResponse>::
            make_value(adserver::channel_svcs::channel_server::SetProxySourcesResponse()));
      return;
    }

    auto ref = get_ref_();
    if (!ref)
    {
      add_completed_stats(true);
      callback(
        grpc::Status(grpc::StatusCode::UNAVAILABLE, "no available ChannelServer grpc client"),
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::SetProxySourcesResponse>::
            make_value(adserver::channel_svcs::channel_server::SetProxySourcesResponse()));
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
        AdServer::Grpc::ResponseHolder<
          adserver::channel_svcs::channel_server::SetProxySourcesResponse>&&
            response_holder)
      mutable
      {
        if (!status.ok() && !AdServer::Grpc::is_request_specific_error(status))
        {
          ref.mark_as_bad(Generics::Time::get_time_of_day() + pool_timeout);
        }
        callback(status, std::move(response_holder));
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
  ChannelDistributedGrpcClient::fill_refs_state_(ControllerRefsState& refs_state) noexcept
  {
    namespace ControllerProto = adserver::channel_svcs::channel_controller;

    refs_state.clear();
    refs_state.reserve(channel_controller_refs_.size());

    for (std::size_t i = 0; i < channel_controller_refs_.size(); ++i)
    {
      try
      {
        auto controller_ref = controller_pools_[i]->get_object();
        if (!controller_ref)
        {
          Stream::Error ostr;
          ostr << "ChannelController group '" <<
            group_name_(channel_controller_refs_[i]) <<
            "': no available refs: " << controller_pools_[i]->unavailable_description();
          callback_->report_error(
            Generics::ActiveObjectCallback::WARNING,
            ostr.str(),
            "ADS-IMPL-117");
          continue;
        }
        auto& ref = *controller_ref;

        grpc::ClientContext context;
        set_deadline_(context);
        ControllerProto::GetSessionDescriptionRequest request;
        ControllerProto::GetSessionDescriptionResponse response;

        const auto status = ref->stub->get_session_description(&context, request, &response);
        if (!status.ok())
        {
          ref->reset();
          ref.mark_as_bad(
            Generics::Time::get_time_of_day() + pool_timeout_,
            status.error_message());
          Stream::Error ostr;
          ostr << "ChannelController '" << ref->endpoint
            << "': get_session_description failed: code="
            << static_cast<int>(status.error_code())
            << ", message=" << status.error_message();
          callback_->report_error(
            Generics::ActiveObjectCallback::WARNING,
            ostr.str(),
            "ADS-IMPL-117");
          continue;
        }

        std::vector<ChannelServerGroup> server_groups;
        for (const auto& group : response.channel_server_groups())
        {
          ChannelServerGroup server_refs;
          for (const auto& server : group.channel_servers())
          {
            server_refs.emplace_back(server.channel_server_endpoint());
          }

          if (!server_refs.empty())
          {
            std::sort(server_refs.begin(), server_refs.end());
            server_groups.emplace_back(std::move(server_refs));
          }
        }

        if (server_groups.empty())
        {
          ref.mark_as_bad(
            Generics::Time::get_time_of_day() + pool_timeout_,
            "get_session_description returned no ChannelServer groups");
          Stream::Error ostr;
          ostr << "ChannelController '" << ref->endpoint
            << "': get_session_description returned no ChannelServer groups";
          callback_->report_error(
            Generics::ActiveObjectCallback::WARNING,
            ostr.str(),
            "ADS-IMPL-117");
          continue;
        }

        std::sort(server_groups.begin(), server_groups.end());
        refs_state.emplace_back(group_name_(channel_controller_refs_[i]), std::move(server_groups));
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "ChannelController group '" <<
          group_name_(channel_controller_refs_[i])
          << "': can't resolve refs: " << ex.what();
        callback_->report_error(
          Generics::ActiveObjectCallback::WARNING,
          ostr.str(),
          "ADS-IMPL-117");
      }
      catch (...)
      {
        Stream::Error ostr;
        ostr << "ChannelController group '" <<
          group_name_(channel_controller_refs_[i])
          << "': can't resolve refs: unknown exception";
        callback_->report_error(
          Generics::ActiveObjectCallback::WARNING,
          ostr.str(),
          "ADS-IMPL-117");
      }
    }

    return !refs_state.empty();
  }

  bool
  ChannelDistributedGrpcClient::update_pool_if_changed_(ControllerRefsState refs_state) noexcept
  {
    try
    {
      std::vector<std::shared_ptr<RefHolder>> refs;
      std::vector<ClientHolderPtr> client_holders;
      for (const auto& controller_refs : refs_state)
      {
        for (const auto& server_group : controller_refs.second)
        {
          std::vector<ClientHolderPtr> group_client_holders;
          group_client_holders.reserve(server_group.size());
          for (const auto& endpoint : server_group)
          {
            auto client_holder = get_or_create_client_holder_(endpoint);
            if (!client_holder)
            {
              return false;
            }

            group_client_holders.emplace_back(client_holder);
            client_holders.emplace_back(std::move(client_holder));
          }

          refs.emplace_back(std::make_shared<RefHolder>(std::move(group_client_holders)));
        }
      }

      PoolPtr new_pool;
      PoolPtr old_pool;
      {
        std::unique_lock<std::shared_mutex> lock(pool_lock_);
        if (deactivated_.load(std::memory_order_acquire) || refs_state_ == refs_state)
        {
          return false;
        }

        new_pool = std::make_shared<Pool>(std::move(refs), coalesce_runner_);
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
  ChannelDistributedGrpcClient::get_or_create_client_holder_(const std::string& endpoint)
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
    result.queue_wait_max_us = std::max(result.queue_wait_max_us, source.queue_wait_max_us);
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
    AdServer::Grpc::merge_last_error(result, source);
  }
}
