#pragma once

#include <atomic>
#include <boost/intrusive/list.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <Generics/MemBuf.hpp>
#include <Generics/MonoAllocator.hpp>
#include <Generics/Time.hpp>

namespace AdServer::ProfilingCommons
{
  class RocksDBBatchingProcessorQueue final
  {
  public:
    using CheckCallback = std::function<void(bool, std::optional<std::string>)>;
    using GetCallback = std::function<void(
      Generics::ConstSmartMemBuf_var,
      std::optional<std::string>)>;
    using GetOwnCallback = std::function<void(
      Generics::SmartMemBuf_var,
      std::optional<std::string>)>;
    using SaveCallback = std::function<void(std::optional<std::string>)>;
    using RemoveCallback = std::function<void(bool, std::optional<std::string>)>;

    enum OperationType
    {
      OT_CHECK,
      OT_GET,
      OT_TOUCH,
      OT_SAVE,
      OT_REMOVE
    };

    struct Operation final
    {
      OperationType type;
      Generics::Time enqueue_time;
      std::string key;
      std::size_t key_hash = 0;
      Generics::ConstSmartMemBuf_var profile;
      std::optional<CheckCallback> check_callback;
      std::optional<GetCallback> get_callback;
      std::optional<GetOwnCallback> get_own_callback;
      std::optional<SaveCallback> save_callback;
      std::optional<RemoveCallback> remove_callback;
    };

    using Operations = std::list<Operation>;
    using SelectedKeys = std::vector<std::string_view>;

    struct OperationCounts final
    {
      std::uint64_t check = 0;
      std::uint64_t get = 0;
      std::uint64_t touch = 0;
      std::uint64_t save = 0;
      std::uint64_t remove = 0;
    };

    struct ReadyState final
    {
      std::uint64_t generation = 0;
      bool has_operation = false;
      bool write_operations = false;
      Generics::Time enqueue_time;
      Generics::Time ready_time;
    };

    struct EnqueueResult final
    {
      bool accepted = false;
      OperationCounts counts;
      std::optional<ReadyState> ready_state;
    };

    RocksDBBatchingProcessorQueue(
      unsigned long batch_size,
      const Generics::Time& max_delay,
      unsigned long enqueue_buckets_count);

    ~RocksDBBatchingProcessorQueue();

    RocksDBBatchingProcessorQueue(const RocksDBBatchingProcessorQueue&) = delete;
    RocksDBBatchingProcessorQueue& operator=(const RocksDBBatchingProcessorQueue&) = delete;

    void activate() noexcept;

    ReadyState deactivate() noexcept;

    EnqueueResult enqueue(Operations&& operations);

    void start_batch() noexcept;

    ReadyState collect_batch(Operations& batch, SelectedKeys& selected_keys) noexcept;

    ReadyState complete_batch(const Operations& batch) noexcept;

    void finish_batch() noexcept;

    void wait_pending();

    void wait_drained();

    bool drained() const noexcept;

    bool accepting() const noexcept;

    static bool is_write_operation(OperationType type) noexcept;

  private:
    using ReadyQueueIndex = std::uint64_t;

    struct StringHash final
    {
      using is_transparent = void;

      std::size_t operator()(std::string_view value) const noexcept
      {
        return boost::hash<std::string_view>{}(value);
      }
    };

    using InFlightKeys = boost::unordered_flat_map<
      std::string,
      unsigned long,
      StringHash,
      std::equal_to<>>;

    using OperationGroupHook = boost::intrusive::list_member_hook<
      boost::intrusive::link_mode<boost::intrusive::safe_link>>;

    struct OperationGroup final
    {
      OperationGroup() = default;
      OperationGroup(const OperationGroup&) = delete;

      std::string_view key;
      std::size_t key_hash = 0;
      Generics::Time enqueue_time;
      Operations operations;
      OperationGroupHook queue_hook;
    };

    struct OperationGroupHash final
    {
      using is_transparent = void;

      std::size_t operator()(const OperationGroup* group) const noexcept
      {
        return group->key_hash;
      }

      std::size_t operator()(std::string_view key) const noexcept
      {
        return std::hash<std::string_view>{}(key);
      }
    };

    struct OperationGroupEqual final
    {
      using is_transparent = void;

      bool operator()(const OperationGroup* left, const OperationGroup* right) const noexcept
      {
        return left->key == right->key;
      }

      bool operator()(const OperationGroup* left, std::string_view right) const noexcept
      {
        return left->key == right;
      }

      bool operator()(std::string_view left, const OperationGroup* right) const noexcept
      {
        return left == right->key;
      }
    };

    using OperationGroups = boost::intrusive::list<
      OperationGroup,
      boost::intrusive::member_hook<
        OperationGroup, OperationGroupHook, &OperationGroup::queue_hook>,
      boost::intrusive::constant_time_size<false>>;
    using OperationGroupIndex = boost::unordered_flat_set<
      OperationGroup*,
      OperationGroupHash,
      OperationGroupEqual>;

    struct OperationQueue final
    {
      explicit OperationQueue(std::size_t reserve_size);

      bool empty() const noexcept;

      OperationGroup* acquire_group();

      void release_group(OperationGroup* group) noexcept;

      Generics::MonoAllocatorArena arena;
      Generics::MonoDeque<OperationGroup> group_storage;
      Generics::MonoDeque<OperationGroup*> free_groups;
      OperationGroups operation_groups;
      OperationGroupIndex groups;
    };

    struct EnqueueBucket final
    {
      explicit EnqueueBucket(std::size_t reserve_size)
        : read_operations(reserve_size),
          write_operations(reserve_size)
      {}

      std::mutex lock;
      OperationQueue read_operations;
      OperationQueue write_operations;
      std::atomic<bool> has_pending_operations{false};
    };

    struct StagedBucket final
    {
      Operations read_operations;
      Operations write_operations;
    };

    void account_operation_(OperationCounts& counts, OperationType type) const noexcept;

    void enqueue_operations_i_(OperationQueue& target, Operations& source);

    void insert_operation_group_i_(
      OperationQueue& target,
      OperationGroup& group) noexcept;

    ReadyState ready_state_i_() noexcept;

    std::optional<ReadyState> publish_enqueued_queue_(
      ReadyQueueIndex queue_index,
      const Generics::Time& enqueue_time) noexcept;

    bool try_publish_ready_queue_i_(
      ReadyQueueIndex queue_index,
      const Generics::Time& enqueue_time) noexcept;

    ReadyQueueIndex reset_ready_queue_i_() noexcept;

    void recalculate_ready_queue_i_() noexcept;

    const OperationGroup* find_ready_group_i_(
      const OperationQueue& operation_queue,
      bool write_operations,
      bool& blocked_operation_seen) const noexcept;

    Generics::Time operation_ready_time_i_(
      bool write_operations,
      const Generics::Time& enqueue_time) const noexcept;

    static ReadyQueueIndex make_ready_queue_index_(
      std::size_t bucket_index,
      bool write_operations) noexcept;

    static std::size_t queue_bucket_index_(ReadyQueueIndex queue_index) noexcept;

    static bool queue_write_operations_(ReadyQueueIndex queue_index) noexcept;

    static Generics::Time time_from_microseconds_(std::int64_t microseconds) noexcept;

    void collect_batch_i_(
      Operations& batch,
      SelectedKeys& selected_keys,
      bool collect_reads,
      std::size_t start_bucket_index) noexcept;

    unsigned long collect_from_queue_i_(
      OperationQueue& source,
      bool collect_reads,
      Operations& batch,
      SelectedKeys& selected_keys) noexcept;

    bool empty_i_() const noexcept;

  private:
    static constexpr std::int64_t NO_READY_ENQUEUE_TIME = std::numeric_limits<std::int64_t>::max();
    static constexpr ReadyQueueIndex UPDATING_READY_QUEUE_INDEX =
      std::numeric_limits<ReadyQueueIndex>::max();

    const unsigned long batch_size_;
    const Generics::Time max_delay_;
    std::vector<std::unique_ptr<EnqueueBucket>> enqueue_buckets_;

    mutable std::mutex state_lock_;
    mutable std::condition_variable drain_condition_;
    InFlightKeys in_flight_read_keys_;
    InFlightKeys in_flight_write_keys_;
    std::atomic<unsigned long> pending_operations_{0};
    std::atomic<unsigned long> pending_read_operations_{0};
    std::atomic<unsigned long> pending_write_operations_{0};
    std::atomic<unsigned long> active_workers_{0};
    std::atomic<bool> accepting_{false};
    std::atomic<ReadyQueueIndex> ready_queue_index_{0};
    std::atomic<std::int64_t> min_ready_enqueue_time_{NO_READY_ENQUEUE_TIME};
    std::atomic<std::uint64_t> ready_generation_{0};
    std::size_t next_bucket_index_ = 0;
    bool recalculate_on_complete_ = false;
  };

  inline bool
  RocksDBBatchingProcessorQueue::is_write_operation(OperationType type) noexcept
  {
    return type == OT_TOUCH || type == OT_SAVE || type == OT_REMOVE;
  }

  inline bool
  RocksDBBatchingProcessorQueue::accepting() const noexcept
  {
    return accepting_.load(std::memory_order_acquire);
  }
}
