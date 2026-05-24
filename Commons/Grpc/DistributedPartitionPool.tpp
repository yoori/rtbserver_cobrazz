#pragma once

namespace AdServer::Grpc
{
  template<typename ClientType>
  DistributedPartitionPool<ClientType>::RefHolder::RefHolder(
    std::string endpoint_val,
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
    std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
      coalesce_runner,
    AdServer::Grpc::BatchingOptions batching_options)
    : endpoint(std::move(endpoint_val)),
      name(endpoint),
      client(std::make_shared<Client>(
        endpoint,
        std::move(grpc_executor),
        std::move(coalesce_runner),
        std::move(batching_options)))
  {
    client->activate_object();
  }

  template<typename ClientType>
  DistributedPartitionPool<ClientType>::RefHolder::~RefHolder() noexcept
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

  template<typename ClientType>
  AdServer::Grpc::Stats
  DistributedPartitionPool<ClientType>::RefHolder::stats() const noexcept
  {
    return static_cast<AdServer::Grpc::Client*>(client.get())->stats();
  }

  template<typename ClientType>
  class DistributedPartitionPool<ClientType>::ResolvePartitionTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    ResolvePartitionTask(
      DistributedPartitionPool* owner,
      unsigned long partition_num) noexcept
      : owner_(owner),
        partition_num_(partition_num)
    {}

    void execute() noexcept override
    {
      owner_->resolve_partition_(partition_num_);
    }

  private:
    ~ResolvePartitionTask() noexcept override = default;

    DistributedPartitionPool* owner_;
    const unsigned long partition_num_;
  };

  template<typename ClientType>
  class DistributedPartitionPool<ClientType>::PeriodicResolveTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    explicit PeriodicResolveTask(DistributedPartitionPool* owner) noexcept
      : owner_(owner)
    {}

    void execute() noexcept override
    {
      owner_->periodic_resolve_loop_();
    }

  private:
    ~PeriodicResolveTask() noexcept override = default;

    DistributedPartitionPool* owner_;
  };

  template<typename ClientType>
  DistributedPartitionPool<ClientType>::DistributedPartitionPool(
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
    const Generics::Time& pool_timeout,
    const Generics::Time& resolve_period)
    : name_(std::move(name)),
      pool_timeout_(pool_timeout),
      resolve_period_(resolve_period),
      controller_refs_(validate_controller_refs_(
        std::move(controller_refs),
        name_)),
      batching_options_(std::move(batching_options)),
      grpc_executor_(std::move(grpc_executor)),
      coalesce_runner_(std::move(coalesce_runner)),
      resolver_(std::move(resolve_partition)),
      partition_index_(std::move(partition_index)),
      chunk_index_(std::move(chunk_index)),
      callback_(new Logging::ActiveObjectCallbackImpl(
        logger,
        name_.c_str(),
        log_aspect.c_str())),
      task_runner_(new Generics::TaskRunner(
        callback_,
        controller_refs_.size() + 1)),
      logger_(ReferenceCounting::add_ref(logger))
  {
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
    partition_holders_.reserve(controller_refs_.size());
    for (std::size_t i = 0; i < controller_refs_.size(); ++i)
    {
      partition_holders_.emplace_back(std::make_shared<PartitionHolder>());
    }
  }

  template<typename ClientType>
  DistributedPartitionPool<ClientType>::~DistributedPartitionPool() noexcept =
    default;

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::activate_object_()
  {
    deactivated_.store(false, std::memory_order_release);
    Generics::CompositeActiveObject::activate_object_();
    resolve_all_partitions_();
    task_runner_->enqueue_task(
      Generics::Task_var(new PeriodicResolveTask(this)));
  }

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::deactivate_object_()
  {
    deactivated_.store(true, std::memory_order_release);
    resolve_cond_.notify_all();
    Generics::CompositeActiveObject::deactivate_object_();
    deactivate_partitions_();
  }

  template<typename ClientType>
  AdServer::Grpc::Stats
  DistributedPartitionPool<ClientType>::stats() const noexcept
  {
    AdServer::Grpc::Stats result;
    std::set<const RefHolder*> seen_refs;
    for (const auto& holder : partition_holders_)
    {
      PartitionPtr partition;
      {
        std::shared_lock<std::shared_mutex> lock(holder->lock);
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
        merge_stats_(result, ref_holder->stats());
      }
    }

    return result;
  }

  template<typename ClientType>
  std::optional<typename DistributedPartitionPool<ClientType>::Ref>
  DistributedPartitionPool<ClientType>::get_ref(const std::string& key) noexcept
  {
    try
    {
      if (key.empty())
      {
        return get_any_ref();
      }

      const auto partitions_number = partition_holders_.size();
      const auto primary_partition = partition_index_(key, partitions_number);
      for (unsigned long i = 0; i < partitions_number; ++i)
      {
        const auto partition_num = (primary_partition + i) % partitions_number;
        auto partition = get_partition_(partition_num);
        if (!partition)
        {
          continue;
        }

        const auto chunk_id = chunk_index_(key, partition->max_chunk_number);
        const auto it = partition->chunk_pools.find(chunk_id);
        if (it == partition->chunk_pools.end() || !it->second)
        {
          try_to_reresolve_partition(partition_num);
          continue;
        }

        auto ref = it->second->get_object();
        if (ref)
        {
          return ref;
        }
      }
    }
    catch (...)
    {
    }

    return std::nullopt;
  }

  template<typename ClientType>
  std::optional<typename DistributedPartitionPool<ClientType>::Ref>
  DistributedPartitionPool<ClientType>::get_any_ref() noexcept
  {
    for (unsigned long i = 0; i < partition_holders_.size(); ++i)
    {
      auto partition = get_partition_(i);
      if (!partition)
      {
        continue;
      }

      for (const auto& chunk_pool : partition->chunk_pools)
      {
        if (!chunk_pool.second)
        {
          continue;
        }
        auto ref = chunk_pool.second->get_object();
        if (ref)
        {
          return ref;
        }
      }
    }

    return std::nullopt;
  }

  template<typename ClientType>
  std::optional<unsigned long>
  DistributedPartitionPool<ClientType>::chunk_index(
    const std::string& key) noexcept
  {
    try
    {
      if (key.empty())
      {
        return std::nullopt;
      }

      const auto partitions_number = partition_holders_.size();
      const auto primary_partition = partition_index_(key, partitions_number);
      for (unsigned long i = 0; i < partitions_number; ++i)
      {
        const auto partition_num = (primary_partition + i) % partitions_number;
        auto partition = get_partition_(partition_num);
        if (partition && partition->max_chunk_number)
        {
          return chunk_index_(key, partition->max_chunk_number);
        }
      }
    }
    catch (...)
    {
    }

    return std::nullopt;
  }

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::try_to_reresolve_partition(
    unsigned long partition_num) noexcept
  {
    schedule_reresolve_partition_(partition_num, false);
  }

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::resolve_all_partitions_() noexcept
  {
    for (unsigned long i = 0; i < partition_holders_.size(); ++i)
    {
      resolve_partition_(i);
    }
  }

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::periodic_resolve_loop_() noexcept
  {
    while (!deactivated_.load(std::memory_order_acquire))
    {
      wait_next_resolve_();
      if (deactivated_.load(std::memory_order_acquire))
      {
        break;
      }

      for (unsigned long i = 0; i < partition_holders_.size(); ++i)
      {
        schedule_reresolve_partition_(i, true);
      }
    }
  }

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::wait_next_resolve_() noexcept
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

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::schedule_reresolve_partition_(
    unsigned long partition_num,
    bool force) noexcept
  {
    if (partition_num >= partition_holders_.size() ||
      !begin_resolve_partition_(partition_num, force))
    {
      return;
    }

    try
    {
      task_runner_->enqueue_task(
        Generics::Task_var(new ResolvePartitionTask(this, partition_num)));
    }
    catch (...)
    {
      finish_resolve_partition_(partition_num, false);
    }
  }

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::resolve_partition_(
    unsigned long partition_num) noexcept
  {
    if (partition_num >= partition_holders_.size())
    {
      return;
    }

    EndpointChunksList refs;
    try
    {
      auto resolved_refs = resolver_(controller_refs_[partition_num]);
      if (!resolved_refs)
      {
        finish_resolve_partition_(partition_num, false);
        return;
      }

      refs = std::move(*resolved_refs);
      for (auto& endpoint_chunks : refs)
      {
        std::sort(
          endpoint_chunks.chunk_ids.begin(),
          endpoint_chunks.chunk_ids.end());
        endpoint_chunks.chunk_ids.erase(
          std::unique(
            endpoint_chunks.chunk_ids.begin(),
            endpoint_chunks.chunk_ids.end()),
          endpoint_chunks.chunk_ids.end());
      }
      std::sort(
        refs.begin(),
        refs.end(),
        [](const auto& lhs, const auto& rhs)
        {
          return lhs.endpoint < rhs.endpoint ||
            (lhs.endpoint == rhs.endpoint && lhs.chunk_ids < rhs.chunk_ids);
        });
      refs.erase(std::unique(refs.begin(), refs.end()), refs.end());
    }
    catch (const eh::Exception& ex)
    {
      if (logger_)
      {
        logger_->sstream(
          Logging::Logger::ERROR,
          name_.c_str(),
          "ADS-IMPL-72") <<
          "Can't resolve controller '" << controller_refs_[partition_num] <<
          "': " << ex.what();
      }
      finish_resolve_partition_(partition_num, false);
      return;
    }
    catch (...)
    {
      finish_resolve_partition_(partition_num, false);
      return;
    }

    if (refs.empty())
    {
      finish_resolve_partition_(partition_num, false);
      return;
    }

    bool state_unchanged = false;
    {
      std::shared_lock<std::shared_mutex> lock(
        partition_holders_[partition_num]->lock);
      state_unchanged =
        partition_holders_[partition_num]->partition &&
        partition_holders_[partition_num]->partition->state == refs;
    }

    if (state_unchanged)
    {
      finish_resolve_partition_(partition_num, true);
      return;
    }

    std::map<unsigned long, std::vector<RefHolderPtr>> chunk_refs;
    std::vector<RefHolderPtr> ref_holders;
    std::set<const RefHolder*> seen_refs;
    unsigned long max_chunk_number = 0;

    for (const auto& endpoint_chunks : refs)
    {
      auto ref_holder = get_or_create_ref_holder_(endpoint_chunks.endpoint);
      if (!ref_holder)
      {
        finish_resolve_partition_(partition_num, false);
        return;
      }

      if (seen_refs.insert(ref_holder.get()).second)
      {
        ref_holders.emplace_back(ref_holder);
      }

      for (const auto chunk_id : endpoint_chunks.chunk_ids)
      {
        chunk_refs[chunk_id].emplace_back(ref_holder);
        max_chunk_number = std::max<unsigned long>(
          max_chunk_number,
          chunk_id + 1);
      }
    }

    if (chunk_refs.empty())
    {
      finish_resolve_partition_(partition_num, false);
      return;
    }

    auto new_partition = std::make_shared<Partition>();
    new_partition->state = std::move(refs);
    new_partition->max_chunk_number = max_chunk_number;
    new_partition->ref_holders = std::move(ref_holders);
    for (auto& chunk_ref : chunk_refs)
    {
      auto pool = std::make_shared<Pool>(
        std::move(chunk_ref.second),
        coalesce_runner_);
      pool->activate_object();
      new_partition->chunk_pools.emplace(chunk_ref.first, std::move(pool));
    }

    PartitionPtr old_partition;
    bool installed = false;
    {
      std::unique_lock<std::shared_mutex> lock(
        partition_holders_[partition_num]->lock);
      if (!deactivated_.load(std::memory_order_acquire))
      {
        old_partition = std::move(partition_holders_[partition_num]->partition);
        partition_holders_[partition_num]->partition = std::move(new_partition);
        installed = true;
      }
    }

    deactivate_partition_pools_(old_partition);
    if (!installed)
    {
      deactivate_partition_pools_(new_partition);
    }
    finish_resolve_partition_(partition_num, installed);
  }

  template<typename ClientType>
  bool
  DistributedPartitionPool<ClientType>::begin_resolve_partition_(
    unsigned long partition_num,
    bool force) noexcept
  {
    if (deactivated_.load(std::memory_order_acquire))
    {
      return false;
    }

    const auto now = Generics::Time::get_time_of_day();
    std::unique_lock<std::shared_mutex> lock(
      partition_holders_[partition_num]->lock);
    if (partition_holders_[partition_num]->resolve_in_progress)
    {
      return false;
    }

    if (!force &&
      now < partition_holders_[partition_num]->last_try_to_resolve +
        pool_timeout_)
    {
      return false;
    }

    partition_holders_[partition_num]->resolve_in_progress = true;
    return true;
  }

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::finish_resolve_partition_(
    unsigned long partition_num,
    bool installed) noexcept
  {
    if (partition_num >= partition_holders_.size())
    {
      return;
    }

    std::unique_lock<std::shared_mutex> lock(
      partition_holders_[partition_num]->lock);
    if (!installed)
    {
      partition_holders_[partition_num]->last_try_to_resolve =
        Generics::Time::get_time_of_day();
    }
    partition_holders_[partition_num]->resolve_in_progress = false;
  }

  template<typename ClientType>
  typename DistributedPartitionPool<ClientType>::PartitionPtr
  DistributedPartitionPool<ClientType>::get_partition_(
    unsigned long partition_num) noexcept
  {
    PartitionPtr result;
    {
      std::shared_lock<std::shared_mutex> lock(
        partition_holders_[partition_num]->lock);
      result = partition_holders_[partition_num]->partition;
    }

    if (!result)
    {
      try_to_reresolve_partition(partition_num);
    }

    return result;
  }

  template<typename ClientType>
  typename DistributedPartitionPool<ClientType>::RefHolderPtr
  DistributedPartitionPool<ClientType>::get_or_create_ref_holder_(
    const std::string& endpoint)
  {
    std::lock_guard<std::mutex> lock(ref_holders_lock_);
    if (deactivated_.load(std::memory_order_acquire))
    {
      return RefHolderPtr();
    }

    auto& weak_ref_holder = ref_holders_[endpoint];
    auto ref_holder = weak_ref_holder.lock();
    if (!ref_holder)
    {
      ref_holder = std::make_shared<RefHolder>(
        endpoint,
        grpc_executor_,
        coalesce_runner_,
        batching_options_);
      weak_ref_holder = ref_holder;
    }
    return ref_holder;
  }

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::deactivate_partitions_() noexcept
  {
    std::vector<PartitionPtr> partitions;
    std::vector<RefHolderPtr> ref_holders;
    std::set<const RefHolder*> seen_refs;

    for (auto& holder : partition_holders_)
    {
      try
      {
        std::unique_lock<std::shared_mutex> lock(holder->lock);
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

      for (const auto& ref_holder : partition->ref_holders)
      {
        if (ref_holder && seen_refs.insert(ref_holder.get()).second)
        {
          ref_holders.emplace_back(ref_holder);
        }
      }

      deactivate_partition_pools_(partition);
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

    std::lock_guard<std::mutex> lock(ref_holders_lock_);
    ref_holders_.clear();
  }

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::deactivate_partition_pools_(
    const PartitionPtr& partition) noexcept
  {
    if (!partition)
    {
      return;
    }

    std::vector<PoolPtr> pools;
    std::set<const Pool*> seen_pools;
    for (const auto& chunk_pool : partition->chunk_pools)
    {
      if (chunk_pool.second && seen_pools.insert(chunk_pool.second.get()).second)
      {
        pools.emplace_back(chunk_pool.second);
      }
    }
    deactivate_pools_(pools);
  }

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::deactivate_pools_(
    const std::vector<PoolPtr>& pools) noexcept
  {
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

  template<typename ClientType>
  void
  DistributedPartitionPool<ClientType>::merge_stats_(
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
    result.timing_coalesce_items += source.timing_coalesce_items;
    result.max_streams = std::max(result.max_streams, source.max_streams);
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
        result.consumer_stream_write =
          AdServer::Grpc::Stats::ConsumerStreamWrite();
      }
      result.consumer_stream_write->count += source.consumer_stream_write->count;
      result.consumer_stream_write->sum_us += source.consumer_stream_write->sum_us;
      result.consumer_stream_write->max_us = std::max(
        result.consumer_stream_write->max_us,
        source.consumer_stream_write->max_us);
    }
  }

  template<typename ClientType>
  typename DistributedPartitionPool<ClientType>::ControllerRefList
  DistributedPartitionPool<ClientType>::validate_controller_refs_(
    ControllerRefList controller_refs,
    const std::string& name)
  {
    if (controller_refs.empty())
    {
      Stream::Error ostr;
      ostr << name << ": empty controller refs";
      throw Exception(ostr);
    }
    return controller_refs;
  }
}
