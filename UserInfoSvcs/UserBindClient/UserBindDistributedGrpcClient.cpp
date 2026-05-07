#include "UserBindDistributedGrpcClient.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include <grpcpp/grpcpp.h>

#include <Commons/UserInfoManip.hpp>
#include <Commons/Grpc/RefPool.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <String/StringManip.hpp>
#include <Sync/SyncPolicy.hpp>
#include <UserInfoSvcs/UserBindController2/UserBindControllerGrpc.grpc.pb.h>

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    const Generics::Time DEFAULT_POOL_TIMEOUT = Generics::Time::ONE_SECOND;
    const std::chrono::seconds DEFAULT_RPC_TIMEOUT(5);

    void set_deadline_(grpc::ClientContext& context)
    {
      context.set_deadline(std::chrono::system_clock::now() + DEFAULT_RPC_TIMEOUT);
    }
  }

  class UserBindDistributedGrpcClient::Distributor:
    public virtual ReferenceCounting::AtomicImpl,
    public Generics::CompositeActiveObject,
    public virtual Generics::RefCountableActiveObject,
    public UserBindServerGrpcAsyncClient
  {
  public:
    using ControllerRefList = std::vector<std::string>;
    using ServerClient = UserBindServerGrpcAsyncBatchingClient;
    using ServerClient_var = ReferenceCounting::SmartPtr<ServerClient>;
    using ControllerGrpc =
      adserver::user_info_svcs::user_bind_controller::UserBindControllerGrpc;

    Distributor(
      Logging::Logger* logger,
      ControllerRefList controller_refs,
      AdServer::Grpc::BatchingOptions batching_options,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      const Generics::Time& pool_timeout = DEFAULT_POOL_TIMEOUT)
      : callback_(new Logging::ActiveObjectCallbackImpl(
          logger,
          "UserBindGrpcOperationDistributor",
          "UserInfo")),
        try_count_(validate_controller_refs_(controller_refs)),
        pool_timeout_(pool_timeout),
        controller_refs_(std::move(controller_refs)),
        batching_options_(std::move(batching_options)),
        grpc_executor_(std::move(grpc_executor)),
        task_runner_(new Generics::TaskRunner(callback_, try_count_))
    {
      add_child_object(task_runner_);

      for (std::size_t i = 0; i < controller_refs_.size(); ++i)
      {
        partition_holders_.push_back(std::make_shared<PartitionHolder>());
        resolve_partition_(i);
      }
    }

    void get_bind_request(
      const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
      GetBindRequestCallback callback) override
    {
      auto ref = get_ref_(request.request_id());
      if (!ref)
      {
        finish_with_unavailable_<
          adserver::user_info_svcs::user_bind::GetBindRequestResponse>(
            std::move(callback));
        return;
      }

      auto pool_ref = std::move(*ref);
      pool_ref->client->get_bind_request(
        request,
        [
          pool_ref = std::move(pool_ref),
          callback = std::move(callback),
          pool_timeout = pool_timeout_
        ](
          const grpc::Status& status,
          const adserver::user_info_svcs::user_bind::GetBindRequestResponse&
            response)
        mutable
        {
          if (!status.ok())
          {
            pool_ref.mark_as_bad(
              Generics::Time::get_time_of_day() + pool_timeout);
          }
          callback(status, response);
        });
    }

    void add_bind_request(
      const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
      AddBindRequestCallback callback) override
    {
      auto ref = get_ref_(request.request_id());
      if (!ref)
      {
        finish_with_unavailable_<
          adserver::user_info_svcs::user_bind::AddBindRequestResponse>(
            std::move(callback));
        return;
      }

      auto pool_ref = std::move(*ref);
      pool_ref->client->add_bind_request(
        request,
        [
          pool_ref = std::move(pool_ref),
          callback = std::move(callback),
          pool_timeout = pool_timeout_
        ](
          const grpc::Status& status,
          const adserver::user_info_svcs::user_bind::AddBindRequestResponse&
            response)
        mutable
        {
          if (!status.ok())
          {
            pool_ref.mark_as_bad(
              Generics::Time::get_time_of_day() + pool_timeout);
          }
          callback(status, response);
        });
    }

    void get_user_id(
      const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
      GetUserIdCallback callback) override
    {
      auto ref = get_ref_(request.id());
      if (!ref)
      {
        finish_with_unavailable_<
          adserver::user_info_svcs::user_bind::GetUserIdResponse>(
            std::move(callback));
        return;
      }

      auto pool_ref = std::move(*ref);
      pool_ref->client->get_user_id(
        request,
        [
          pool_ref = std::move(pool_ref),
          callback = std::move(callback),
          pool_timeout = pool_timeout_
        ](
          const grpc::Status& status,
          const adserver::user_info_svcs::user_bind::GetUserIdResponse&
            response)
        mutable
        {
          if (!status.ok())
          {
            pool_ref.mark_as_bad(
              Generics::Time::get_time_of_day() + pool_timeout);
          }
          callback(status, response);
        });
    }

    void add_user_id(
      const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
      AddUserIdCallback callback) override
    {
      auto ref = get_ref_(request.id());
      if (!ref)
      {
        finish_with_unavailable_<
          adserver::user_info_svcs::user_bind::AddUserIdResponse>(
            std::move(callback));
        return;
      }

      auto pool_ref = std::move(*ref);
      pool_ref->client->add_user_id(
        request,
        [
          pool_ref = std::move(pool_ref),
          callback = std::move(callback),
          pool_timeout = pool_timeout_
        ](
          const grpc::Status& status,
          const adserver::user_info_svcs::user_bind::AddUserIdResponse&
            response)
        mutable
        {
          if (!status.ok())
          {
            pool_ref.mark_as_bad(
              Generics::Time::get_time_of_day() + pool_timeout);
          }
          callback(status, response);
        });
    }

    void get_source(
      const adserver::user_info_svcs::user_bind::GetSourceRequest& request,
      GetSourceCallback callback) override
    {
      auto ref = get_ref_(std::string());
      if (!ref)
      {
        finish_with_unavailable_<
          adserver::user_info_svcs::user_bind::GetSourceResponse>(
            std::move(callback));
        return;
      }

      auto pool_ref = std::move(*ref);
      pool_ref->client->get_source(
        request,
        [
          pool_ref = std::move(pool_ref),
          callback = std::move(callback),
          pool_timeout = pool_timeout_
        ](
          const grpc::Status& status,
          const adserver::user_info_svcs::user_bind::GetSourceResponse&
            response)
        mutable
        {
          if (!status.ok())
          {
            pool_ref.mark_as_bad(
              Generics::Time::get_time_of_day() + pool_timeout);
          }
          callback(status, response);
        });
    }

    AdServer::Grpc::Stats stats() const noexcept override
    {
      AdServer::Grpc::Stats result;
      std::set<const RefHolder*> seen_refs;
      for (const auto& holder : partition_holders_)
      {
        Partition_var partition;
        {
          Sync::Policy::PosixThread::WriteGuard lock(holder->lock);
          partition = holder->partition;
        }

        if (!partition)
        {
          continue;
        }

        for (const auto& ref_holder : partition->ref_holders)
        {
          if (!ref_holder || !seen_refs.insert(ref_holder.get()).second)
          {
            continue;
          }

          merge_stats_(
            result,
            static_cast<UserBindServerGrpcAsyncClient*>(
              ref_holder->client.in())->stats());
        }
      }
      return result;
    }

  private:
    class ResolvePartitionTask:
      public Generics::Task,
      public ReferenceCounting::AtomicImpl
    {
    public:
      ResolvePartitionTask(Distributor* distributor, unsigned int partition_num)
        noexcept
        : distributor_(distributor),
          partition_num_(partition_num)
      {}

      void execute() noexcept override
      {
        distributor_->resolve_partition_(partition_num_);
      }

    private:
      ~ResolvePartitionTask() noexcept override = default;

      Distributor* distributor_;
      const unsigned int partition_num_;
    };

    struct RefHolder
    {
      RefHolder(
        std::string endpoint,
        std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
        AdServer::Grpc::BatchingOptions batching_options)
        : endpoint(std::move(endpoint)),
          client(new ServerClient(
            this->endpoint,
            std::move(grpc_executor),
            std::move(batching_options)))
      {
        client->activate_object();
      }

      const std::string endpoint;
      ReferenceCounting::SmartPtr<ServerClient> client;

      ~RefHolder() noexcept
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
    };

    using RefHolder_var = std::shared_ptr<RefHolder>;
    using Pool = AdServer::Grpc::RefPool<RefHolder>;
    using PoolPtr = std::shared_ptr<Pool>;

    class Partition
    {
    public:
      unsigned long chunk_index(const std::string& id) const noexcept
      {
        if (max_chunk_number == 0)
        {
          return 0;
        }

        return AdServer::Commons::external_id_distribution_hash(
          String::SubString(id)) % max_chunk_number;
      }

      std::map<unsigned long, PoolPtr> chunks_ref_pool_map;
      std::vector<RefHolder_var> ref_holders;
      unsigned int max_chunk_number = 0;
    };

    using Partition_var = std::shared_ptr<const Partition>;

    class PartitionHolder
    {
    public:
      mutable Sync::Policy::PosixThread::Mutex lock;
      Partition_var partition;
      Generics::Time last_try_to_resolve;
      bool resolve_in_progress = false;
    };

    using PartitionHolder_var = std::shared_ptr<PartitionHolder>;
    using PartitionHolderArray = std::vector<PartitionHolder_var>;

    void deactivate_object_() override
    {
      Generics::CompositeActiveObject::deactivate_object_();
      deactivate_partitions_();
    }

    void deactivate_partitions_() noexcept
    {
      std::vector<Partition_var> partitions;
      std::vector<RefHolder_var> ref_holders;
      std::vector<PoolPtr> pools;
      std::set<const RefHolder*> seen_refs;
      std::set<const Pool*> seen_pools;

      deactivated_.store(true, std::memory_order_release);

      for (auto& holder : partition_holders_)
      {
        try
        {
          Sync::Policy::PosixThread::WriteGuard lock(holder->lock);
          if (holder->partition)
          {
            partitions.emplace_back(holder->partition);
          }
          holder->partition.reset();
        }
        catch (...)
        {
        }
      }

      for (const auto& partition : partitions)
      {
        if (!partition)
        {
          continue;
        }

        for (const auto& chunk_pool : partition->chunks_ref_pool_map)
        {
          if (chunk_pool.second && seen_pools.insert(chunk_pool.second.get()).second)
          {
            pools.emplace_back(chunk_pool.second);
          }
        }

        for (const auto& ref_holder : partition->ref_holders)
        {
          if (ref_holder && seen_refs.insert(ref_holder.get()).second)
          {
            ref_holders.emplace_back(ref_holder);
          }
        }
      }

      for (const auto& pool : pools)
      {
        try
        {
          pool->deactivate_object();
        }
        catch (...)
        {
        }
      }

      for (const auto& pool : pools)
      {
        try
        {
          pool->wait_object();
        }
        catch (...)
        {
        }
      }

      for (const auto& ref_holder : ref_holders)
      {
        try
        {
          if (ref_holder->client->active())
          {
            ref_holder->client->deactivate_object();
          }
        }
        catch (...)
        {
        }
      }

      for (const auto& ref_holder : ref_holders)
      {
        try
        {
          ref_holder->client->wait_object();
        }
        catch (...)
        {
        }
      }
    }

    void deactivate_partition_pools_(const Partition_var& partition) noexcept
    {
      if (!partition)
      {
        return;
      }

      std::vector<PoolPtr> pools;
      std::set<const Pool*> seen_pools;
      for (const auto& chunk_pool : partition->chunks_ref_pool_map)
      {
        if (chunk_pool.second && seen_pools.insert(chunk_pool.second.get()).second)
        {
          pools.emplace_back(chunk_pool.second);
        }
      }

      for (const auto& pool : pools)
      {
        try
        {
          pool->deactivate_object();
        }
        catch (...)
        {
        }
      }

      for (const auto& pool : pools)
      {
        try
        {
          pool->wait_object();
        }
        catch (...)
        {
        }
      }
    }

    RefHolder_var get_or_create_ref_holder_(
      const std::string& endpoint)
    {
      std::lock_guard<std::mutex> lock(ref_holders_lock_);
      if (deactivated_.load(std::memory_order_acquire))
      {
        return RefHolder_var();
      }

      auto& weak_ref_holder = ref_holders_[endpoint];
      auto ref_holder = weak_ref_holder.lock();
      if (!ref_holder)
      {
        ref_holder = std::make_shared<RefHolder>(
          endpoint,
          grpc_executor_,
          batching_options_);
        weak_ref_holder = ref_holder;
      }
      return ref_holder;
    }

    unsigned long partition_index_(const std::string& user_id) const noexcept
    {
      return (
        AdServer::Commons::external_id_distribution_hash(
          String::SubString(user_id)) >> 8) % try_count_;
    }

    Partition_var get_partition_(unsigned long partition_num) noexcept
    {
      Partition_var result;
      {
        Sync::Policy::PosixThread::WriteGuard lock(
          partition_holders_[partition_num]->lock);
        result = partition_holders_[partition_num]->partition;
      }

      if (!result)
      {
        try_to_reresolve_partition_(partition_num);
      }

      return result;
    }

    void try_to_reresolve_partition_(unsigned int partition_num) noexcept
    {
      if (deactivated_.load(std::memory_order_acquire))
      {
        return;
      }

      const auto now = Generics::Time::get_time_of_day();
      {
        Sync::Policy::PosixThread::WriteGuard lock(
          partition_holders_[partition_num]->lock);
        if (partition_holders_[partition_num]->resolve_in_progress ||
          now < partition_holders_[partition_num]->last_try_to_resolve + pool_timeout_)
        {
          return;
        }
        partition_holders_[partition_num]->resolve_in_progress = true;
      }

      task_runner_->enqueue_task(
        Generics::Task_var(new ResolvePartitionTask(this, partition_num)));
    }

    void resolve_partition_(unsigned int partition_num) noexcept
    {
      std::shared_ptr<Partition> new_partition;
      const auto& endpoint = controller_refs_[partition_num];

      try
      {
        auto channel = grpc::CreateChannel(
          endpoint,
          grpc::InsecureChannelCredentials());
        auto stub = ControllerGrpc::NewStub(channel);

        grpc::ClientContext context;
        set_deadline_(context);
        adserver::user_info_svcs::user_bind_controller::
          GetSessionDescriptionRequest request;
        adserver::user_info_svcs::user_bind_controller::
          GetSessionDescriptionResponse response;

        const auto status =
          stub->get_session_description(&context, request, &response);
        if (!status.ok())
        {
          return;
        }

        std::map<unsigned long, std::vector<RefHolder_var>> chunk_refs;
        std::vector<RefHolder_var> ref_holders;
        std::set<const RefHolder*> seen_refs;
        unsigned int max_chunk_number = 0;
        bool resolve_interrupted = false;

        for (const auto& server : response.user_bind_servers())
        {
          auto ref_holder = get_or_create_ref_holder_(
            server.user_bind_server_endpoint());
          if (!ref_holder)
          {
            resolve_interrupted = true;
            break;
          }

          for (const auto chunk_id : server.chunk_ids())
          {
            chunk_refs[chunk_id].emplace_back(ref_holder);
            if (seen_refs.insert(ref_holder.get()).second)
            {
              ref_holders.emplace_back(ref_holder);
            }
            if (chunk_id >= max_chunk_number)
            {
              max_chunk_number = chunk_id + 1;
            }
          }
        }

        if (!resolve_interrupted && !chunk_refs.empty())
        {
          auto fill_partition = std::make_shared<Partition>();
          fill_partition->ref_holders = std::move(ref_holders);
          fill_partition->max_chunk_number = max_chunk_number;

          for (auto& chunk_ref : chunk_refs)
          {
            auto pool = std::make_shared<Pool>();
            pool->set_refs(chunk_ref.second);
            pool->activate_object();
            fill_partition->chunks_ref_pool_map.emplace(
              chunk_ref.first,
              std::move(pool));
          }

          new_partition = fill_partition;
        }
      }
      catch (...)
      {
      }

      const auto now = Generics::Time::get_time_of_day();
      bool partition_installed = false;
      Partition_var old_partition;
      {
        Sync::Policy::PosixThread::WriteGuard lock(
          partition_holders_[partition_num]->lock);
        if (new_partition && !deactivated_.load(std::memory_order_acquire))
        {
          old_partition = std::move(partition_holders_[partition_num]->partition);
          partition_holders_[partition_num]->partition = std::move(new_partition);
          partition_installed = true;
          partition_holders_[partition_num]->resolve_in_progress = false;
        }
        else
        {
          partition_holders_[partition_num]->last_try_to_resolve = now;
          partition_holders_[partition_num]->resolve_in_progress = false;
        }
      }

      deactivate_partition_pools_(old_partition);

      if (new_partition && !partition_installed)
      {
        deactivate_partition_pools_(new_partition);
      }
    }

    std::optional<Pool::Ref> get_ref_(const std::string& user_id)
    {
      for (unsigned i = 0; i < try_count_; ++i)
      {
        const auto partition_num =
          (partition_index_(user_id) + i) % partition_holders_.size();
        Partition_var partition = get_partition_(partition_num);
        if (!partition)
        {
          try_to_reresolve_partition_(partition_num);
          continue;
        }

        const auto chunk_index = partition->chunk_index(user_id);
        auto iter = partition->chunks_ref_pool_map.find(chunk_index);
        if (iter == partition->chunks_ref_pool_map.end())
        {
          try_to_reresolve_partition_(partition_num);
          continue;
        }

        auto ref = iter->second->get_object();
        if (!ref)
        {
          continue;
        }

        return ref;
      }

      return std::nullopt;
    }

    static std::size_t validate_controller_refs_(
      const ControllerRefList& controller_refs)
    {
      if (controller_refs.empty())
      {
        throw Exception(
          "UserBindDistributedGrpcClient: empty UserBindController2 refs");
      }

      return controller_refs.size();
    }

    template<typename Response, typename Callback>
    void finish_with_unavailable_(Callback&& callback)
    {
      callback(
        grpc::Status(
          grpc::StatusCode::UNAVAILABLE,
          "no available UserBindServer grpc client"),
        Response());
    }

    static void merge_stats_(
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

  private:
    Generics::ActiveObjectCallback_var callback_;
    const unsigned long try_count_;
    const Generics::Time pool_timeout_;
    const ControllerRefList controller_refs_;
    const AdServer::Grpc::BatchingOptions batching_options_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    Generics::FixedTaskRunner_var task_runner_;
    PartitionHolderArray partition_holders_;
    std::mutex ref_holders_lock_;
    std::map<std::string, std::weak_ptr<RefHolder>> ref_holders_;
    std::atomic_bool deactivated_{false};
  };

  UserBindDistributedGrpcClient::UserBindDistributedGrpcClient(
    const UserBindControllerRefs& user_bind_controller_refs,
    AdServer::Grpc::BatchingOptions batching_options,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    Logging::Logger* logger)
  {
    ReferenceCounting::SmartPtr<Distributor> distributor =
      new Distributor(
        logger,
        user_bind_controller_refs,
        std::move(batching_options),
        std::move(grpc_executor));
    user_bind_mapper_ = distributor;
    add_child_object(distributor);
  }

  AdServer::Grpc::Stats
  UserBindDistributedGrpcClient::stats() const noexcept
  {
    return user_bind_mapper_->stats();
  }

  void
  UserBindDistributedGrpcClient::get_bind_request(
    const adserver::user_info_svcs::user_bind::GetBindRequestRequest& request,
    GetBindRequestCallback callback)
  {
    user_bind_mapper_->get_bind_request(request, std::move(callback));
  }

  void
  UserBindDistributedGrpcClient::add_bind_request(
    const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
    AddBindRequestCallback callback)
  {
    user_bind_mapper_->add_bind_request(request, std::move(callback));
  }

  void
  UserBindDistributedGrpcClient::get_user_id(
    const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
    GetUserIdCallback callback)
  {
    user_bind_mapper_->get_user_id(request, std::move(callback));
  }

  void
  UserBindDistributedGrpcClient::add_user_id(
    const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
    AddUserIdCallback callback)
  {
    user_bind_mapper_->add_user_id(request, std::move(callback));
  }

  void
  UserBindDistributedGrpcClient::get_source(
    const adserver::user_info_svcs::user_bind::GetSourceRequest& request,
    GetSourceCallback callback)
  {
    user_bind_mapper_->get_source(request, std::move(callback));
  }
}
