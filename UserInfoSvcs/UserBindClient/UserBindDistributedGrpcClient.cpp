#include "UserBindDistributedGrpcClient.hpp"

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

#include <grpcpp/grpcpp.h>

#include <Commons/UserInfoManip.hpp>
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
      AdServer::Grpc::GrpcExecutor* grpc_executor,
      const Generics::Time& pool_timeout = DEFAULT_POOL_TIMEOUT)
      : callback_(new Logging::ActiveObjectCallbackImpl(
          logger,
          "UserBindGrpcOperationDistributor",
          "UserInfo")),
        try_count_(validate_controller_refs_(controller_refs)),
        pool_timeout_(pool_timeout),
        controller_refs_(std::move(controller_refs)),
        batching_options_(std::move(batching_options)),
        grpc_executor_(ReferenceCounting::add_ref(grpc_executor)),
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
      auto client = get_client_(request.request_id());
      if (!client)
      {
        finish_with_unavailable_<
          adserver::user_info_svcs::user_bind::GetBindRequestResponse>(
            std::move(callback));
        return;
      }
      client->get_bind_request(request, std::move(callback));
    }

    void add_bind_request(
      const adserver::user_info_svcs::user_bind::AddBindRequestRequest& request,
      AddBindRequestCallback callback) override
    {
      auto client = get_client_(request.request_id());
      if (!client)
      {
        finish_with_unavailable_<
          adserver::user_info_svcs::user_bind::AddBindRequestResponse>(
            std::move(callback));
        return;
      }
      client->add_bind_request(request, std::move(callback));
    }

    void get_user_id(
      const adserver::user_info_svcs::user_bind::GetUserIdRequest& request,
      GetUserIdCallback callback) override
    {
      auto client = get_client_(request.id());
      if (!client)
      {
        finish_with_unavailable_<
          adserver::user_info_svcs::user_bind::GetUserIdResponse>(
            std::move(callback));
        return;
      }
      client->get_user_id(request, std::move(callback));
    }

    void add_user_id(
      const adserver::user_info_svcs::user_bind::AddUserIdRequest& request,
      AddUserIdCallback callback) override
    {
      auto client = get_client_(request.id());
      if (!client)
      {
        finish_with_unavailable_<
          adserver::user_info_svcs::user_bind::AddUserIdResponse>(
            std::move(callback));
        return;
      }
      client->add_user_id(request, std::move(callback));
    }

    void get_source(
      const adserver::user_info_svcs::user_bind::GetSourceRequest& request,
      GetSourceCallback callback) override
    {
      auto client = get_client_(std::string());
      if (!client)
      {
        finish_with_unavailable_<
          adserver::user_info_svcs::user_bind::GetSourceResponse>(
            std::move(callback));
        return;
      }
      client->get_source(request, std::move(callback));
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

        for (const auto& chunk_ref : partition->chunks_ref_map)
        {
          const auto* ref_holder = chunk_ref.second.get();
          if (!seen_refs.insert(ref_holder).second)
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
        AdServer::Grpc::GrpcExecutor* grpc_executor,
        AdServer::Grpc::BatchingOptions batching_options)
        : endpoint(std::move(endpoint)),
          client(new ServerClient(
            this->endpoint,
            grpc_executor,
            std::move(batching_options)))
      {
        client->activate_object();
      }

      bool is_bad(const Generics::Time& timeout) const noexcept
      {
        if (!marked_as_bad)
        {
          return false;
        }

        const auto now = Generics::Time::get_time_of_day();
        Sync::Policy::PosixThread::WriteGuard lock(lock_);
        if (marked_as_bad && now >= marked_as_bad_time + timeout)
        {
          marked_as_bad = false;
        }
        return marked_as_bad;
      }

      void release_bad() noexcept
      {
        Sync::Policy::PosixThread::WriteGuard lock(lock_);
        marked_as_bad = true;
        marked_as_bad_time = Generics::Time::get_time_of_day();
      }

      const std::string endpoint;
      ReferenceCounting::SmartPtr<ServerClient> client;
      mutable Sync::Policy::PosixThread::Mutex lock_;
      Generics::Time marked_as_bad_time;
      mutable bool marked_as_bad = false;

      ~RefHolder() noexcept
      {
        try
        {
          client->deactivate_object();
          client->wait_object();
        }
        catch (...)
        {
        }
      }
    };

    using RefHolder_var = std::shared_ptr<RefHolder>;

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

      std::map<unsigned long, RefHolder_var> chunks_ref_map;
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
      clear_partitions_();
    }

    void clear_partitions_() noexcept
    {
      for (auto& holder : partition_holders_)
      {
        try
        {
          Sync::Policy::PosixThread::WriteGuard lock(holder->lock);
          holder->partition.reset();
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

        auto fill_partition = std::make_shared<Partition>();

        for (const auto& server : response.user_bind_servers())
        {
          auto ref_holder = get_or_create_ref_holder_(
            server.user_bind_server_endpoint());
          for (const auto chunk_id : server.chunk_ids())
          {
            fill_partition->chunks_ref_map.emplace(chunk_id, ref_holder);
            if (chunk_id >= fill_partition->max_chunk_number)
            {
              fill_partition->max_chunk_number = chunk_id + 1;
            }
          }
        }

        if (!fill_partition->chunks_ref_map.empty())
        {
          new_partition = fill_partition;
        }
      }
      catch (...)
      {
      }

      const auto now = Generics::Time::get_time_of_day();
      Sync::Policy::PosixThread::WriteGuard lock(
        partition_holders_[partition_num]->lock);
      if (new_partition)
      {
        partition_holders_[partition_num]->partition = std::move(new_partition);
        partition_holders_[partition_num]->resolve_in_progress = false;
      }
      else
      {
        partition_holders_[partition_num]->last_try_to_resolve = now;
        partition_holders_[partition_num]->resolve_in_progress = false;
      }
    }

    ServerClient_var get_client_(const std::string& user_id)
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
        auto iter = partition->chunks_ref_map.find(chunk_index);
        if (iter == partition->chunks_ref_map.end())
        {
          try_to_reresolve_partition_(partition_num);
          continue;
        }

        if (iter->second->is_bad(pool_timeout_))
        {
          continue;
        }

        return iter->second->client;
      }

      return ServerClient_var();
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
    AdServer::Grpc::GrpcExecutor_var grpc_executor_;
    Generics::FixedTaskRunner_var task_runner_;
    PartitionHolderArray partition_holders_;
    std::mutex ref_holders_lock_;
    std::map<std::string, std::weak_ptr<RefHolder>> ref_holders_;
  };

  UserBindDistributedGrpcClient::UserBindDistributedGrpcClient(
    const UserBindControllerRefs& user_bind_controller_refs,
    AdServer::Grpc::BatchingOptions batching_options,
    AdServer::Grpc::GrpcExecutor* grpc_executor,
    Logging::Logger* logger)
  {
    ReferenceCounting::SmartPtr<Distributor> distributor =
      new Distributor(
        logger,
        user_bind_controller_refs,
        std::move(batching_options),
        grpc_executor);
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
