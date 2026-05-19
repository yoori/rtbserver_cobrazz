#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/RefPool.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <Logger/ActiveObjectCallback.hpp>

namespace AdServer::Grpc
{
  template<typename ClientType>
  class DistributedPartitionPool:
    public Generics::CompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct EndpointChunks
    {
      std::string endpoint;
      std::vector<unsigned long> chunk_ids;

      bool operator==(const EndpointChunks& rhs) const noexcept = default;
    };

    using EndpointChunksList = std::vector<EndpointChunks>;
    using ControllerRefList = std::vector<std::string>;
    using Client = ClientType;
    using ClientPtr = std::shared_ptr<Client>;
    using ResolvePartition = std::function<
      std::optional<EndpointChunksList>(const std::string&)>;
    using PartitionIndex = std::function<
      unsigned long(const std::string&, unsigned long)>;
    using ChunkIndex = std::function<
      unsigned long(const std::string&, unsigned long)>;

    struct RefHolder
    {
      RefHolder(
      std::string endpoint_val,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
        coalesce_runner,
      AdServer::Grpc::BatchingOptions batching_options);

      ~RefHolder() noexcept;

      AdServer::Grpc::Stats stats() const noexcept;

      const std::string endpoint;
      const std::string name;
      ClientPtr client;
    };

    using RefHolderPtr = std::shared_ptr<RefHolder>;
    using Pool = AdServer::Grpc::RefPool<RefHolder>;
    using PoolPtr = std::shared_ptr<Pool>;
    using Ref = typename Pool::Ref;

    DistributedPartitionPool(
      std::string name,
      std::string log_aspect,
      ControllerRefList controller_refs,
      AdServer::Grpc::BatchingOptions batching_options,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
        coalesce_runner,
      Logging::Logger* logger,
      ResolvePartition resolve_partition,
      PartitionIndex partition_index,
      ChunkIndex chunk_index,
      const Generics::Time& pool_timeout = Generics::Time::ONE_SECOND,
      const Generics::Time& resolve_period = Generics::Time(10));

    ~DistributedPartitionPool() noexcept override;

    AdServer::Grpc::Stats stats() const noexcept;

    std::optional<Ref> get_ref(const std::string& key) noexcept;
    std::optional<Ref> get_any_ref() noexcept;
    void try_to_reresolve_partition(unsigned long partition_num) noexcept;

  private:
    class ResolvePartitionTask;
    class PeriodicResolveTask;

    struct Partition
    {
      std::map<unsigned long, PoolPtr> chunk_pools;
      EndpointChunksList state;
      unsigned long max_chunk_number = 0;
      std::vector<RefHolderPtr> ref_holders;
    };

    using PartitionPtr = std::shared_ptr<const Partition>;

    struct PartitionHolder
    {
      mutable std::shared_mutex lock;
      PartitionPtr partition;
      Generics::Time last_try_to_resolve;
      bool resolve_in_progress = false;
    };

    using PartitionHolderPtr = std::shared_ptr<PartitionHolder>;

    void activate_object_() override;
    void deactivate_object_() override;

    void resolve_all_partitions_() noexcept;
    void periodic_resolve_loop_() noexcept;
    void wait_next_resolve_() noexcept;
    void schedule_reresolve_partition_(
      unsigned long partition_num,
      bool force) noexcept;
    void resolve_partition_(unsigned long partition_num) noexcept;
    bool begin_resolve_partition_(
      unsigned long partition_num,
      bool force) noexcept;
    void finish_resolve_partition_(
      unsigned long partition_num,
      bool installed) noexcept;
    PartitionPtr get_partition_(unsigned long partition_num) noexcept;
    RefHolderPtr get_or_create_ref_holder_(const std::string& endpoint);
    void deactivate_partitions_() noexcept;
    static void deactivate_partition_pools_(const PartitionPtr& partition) noexcept;
    static void deactivate_pools_(const std::vector<PoolPtr>& pools) noexcept;
    static void merge_stats_(
      AdServer::Grpc::Stats& result,
      const AdServer::Grpc::Stats& source) noexcept;
    static ControllerRefList validate_controller_refs_(
      ControllerRefList controller_refs,
      const std::string& name);

  private:
    const std::string name_;
    const Generics::Time pool_timeout_;
    const Generics::Time resolve_period_;
    const ControllerRefList controller_refs_;
    const AdServer::Grpc::BatchingOptions batching_options_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner_;
    const ResolvePartition resolver_;
    const PartitionIndex partition_index_;
    const ChunkIndex chunk_index_;

    Generics::ActiveObjectCallback_var callback_;
    Generics::FixedTaskRunner_var task_runner_;
    Logging::Logger_var logger_;
    std::vector<PartitionHolderPtr> partition_holders_;
    std::mutex ref_holders_lock_;
    std::map<std::string, std::weak_ptr<RefHolder>> ref_holders_;
    std::mutex resolve_lock_;
    std::condition_variable resolve_cond_;
    std::atomic_bool deactivated_{true};
  };
}

#include "DistributedPartitionPool.tpp"
