#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

#include <Commons/Grpc/DistributedPartitionPool.hpp>

namespace AdServer::Grpc
{
  template<
    typename ClientType,
    typename ControllerClientType = BasicControllerRefHolder>
  class BalancedDistributedPartitionPool:
    public DistributedPartitionPool<ClientType, ControllerClientType>
  {
  private:
    using Base = DistributedPartitionPool<ClientType, ControllerClientType>;

  public:
    using ControllerRefArray = typename Base::ControllerRefArray;
    using ControllerClient = typename Base::ControllerClient;
    using ResolvePartition = typename Base::ResolvePartition;
    using ChunkIndex = typename Base::ChunkIndex;
    using PartitionHash = std::function<std::uint64_t(const std::string&)>;

    BalancedDistributedPartitionPool(
      std::string name,
      std::string log_aspect,
      ControllerRefArray controller_refs,
      AdServer::Grpc::BatchingOptions batching_options,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
        coalesce_runner,
      Logging::Logger* logger,
      ResolvePartition resolve_partition,
      PartitionHash partition_hash,
      ChunkIndex chunk_index,
      const Generics::Time& pool_timeout = Generics::Time::ONE_SECOND,
      const Generics::Time& resolve_period = Generics::Time(10))
      : BalancedDistributedPartitionPool(
          std::move(name),
          std::move(log_aspect),
          ControllerRefsHolder(std::move(controller_refs)),
          std::move(batching_options),
          std::move(grpc_executor),
          std::move(coalesce_runner),
          logger,
          std::move(resolve_partition),
          std::move(partition_hash),
          std::move(chunk_index),
          pool_timeout,
          resolve_period)
    {}

  private:
    struct ControllerRefsHolder
    {
      explicit ControllerRefsHolder(ControllerRefArray refs_val)
        : refs(std::move(refs_val)),
          size(refs.size())
      {}

      ControllerRefArray refs;
      std::size_t size;
    };

    BalancedDistributedPartitionPool(
      std::string name,
      std::string log_aspect,
      ControllerRefsHolder controller_refs,
      AdServer::Grpc::BatchingOptions batching_options,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
        coalesce_runner,
      Logging::Logger* logger,
      ResolvePartition resolve_partition,
      PartitionHash partition_hash,
      ChunkIndex chunk_index,
      const Generics::Time& pool_timeout,
      const Generics::Time& resolve_period)
      : Base(
          std::move(name),
          std::move(log_aspect),
          std::move(controller_refs.refs),
          std::move(batching_options),
          std::move(grpc_executor),
          std::move(coalesce_runner),
          logger,
          [this, resolve_partition = std::move(resolve_partition)](
            ControllerClient& controller_client) mutable
          {
            auto refs = resolve_partition(controller_client);
            if (refs)
            {
              set_partition_weight_(controller_client.partition_index, refs->size());
            }
            return refs;
          },
          [this](const std::string& key, unsigned long partitions_number)
          {
            return partition_index_(key, partitions_number);
          },
          std::move(chunk_index),
          pool_timeout,
          resolve_period),
        partition_hash_(std::move(partition_hash)),
        partition_weights_(controller_refs.size, 1)
    {
      rebuild_partition_routing_table_i_();
    }

    void rebuild_partition_routing_table_i_() noexcept
    {
      std::vector<unsigned long> routing_table;

      for (std::size_t partition_index = 0;
        partition_index < partition_weights_.size();
        ++partition_index)
      {
        const auto weight = std::max<std::size_t>(1, partition_weights_[partition_index]);
        routing_table.insert(
          routing_table.end(),
          weight,
          static_cast<unsigned long>(partition_index));
      }

      if (routing_table.empty())
      {
        routing_table.emplace_back(0);
      }

      partition_routing_table_.swap(routing_table);
    }

    void set_partition_weight_(const std::size_t partition_index, const std::size_t weight) noexcept
    {
      const auto normalized_weight = std::max<std::size_t>(1, weight);
      {
        std::shared_lock lock(partition_routing_lock_);
        if (partition_index >= partition_weights_.size() ||
          partition_weights_[partition_index] == normalized_weight)
        {
          return;
        }
      }

      std::unique_lock lock(partition_routing_lock_);
      if (partition_index >= partition_weights_.size())
      {
        return;
      }

      if (partition_weights_[partition_index] == normalized_weight)
      {
        return;
      }

      partition_weights_[partition_index] = normalized_weight;
      rebuild_partition_routing_table_i_();
    }

    unsigned long partition_index_(const std::string& key, unsigned long partitions_number) const
    {
      if (!partitions_number)
      {
        return 0;
      }

      const auto key_hash = partition_hash_(key);
      constexpr auto group_hash_shift = sizeof(key_hash) * 8 / 2;

      std::shared_lock lock(partition_routing_lock_);
      if (partition_routing_table_.empty())
      {
        return (key_hash >> 8) % partitions_number;
      }

      return partition_routing_table_[
        (key_hash >> group_hash_shift) % partition_routing_table_.size()];
    }

  private:
    const PartitionHash partition_hash_;
    mutable std::shared_mutex partition_routing_lock_;
    std::vector<std::size_t> partition_weights_;
    std::vector<unsigned long> partition_routing_table_;
  };
}
