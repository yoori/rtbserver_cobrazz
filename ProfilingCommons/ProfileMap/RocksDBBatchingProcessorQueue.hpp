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

#include <Generics/HashTableAdapters.hpp>
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
      Generics::StringHashAdapter key;
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
      bool has_operation = false;
      Generics::Time enqueue_time;
      Generics::Time ready_time;
    };

    struct EnqueueResult final
    {
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

    ReadyState flush_pending() noexcept;

    EnqueueResult enqueue(Operations&& operations);

    void start_batch() noexcept;

    ReadyState collect_batch(Operations& batch, SelectedKeys& selected_keys) noexcept;

    ReadyState complete_batch(const Operations& batch) noexcept;

    void finish_batch() noexcept;

    void wait_pending();

    void wait_drained();

    bool drained() const noexcept;

    static bool is_write_operation(OperationType type) noexcept;

  private:
    struct KeyHash final
    {
      using is_transparent = void;

      std::size_t operator()(const Generics::StringHashAdapter& value) const noexcept
      {
        return value.hash();
      }

      std::size_t operator()(const Generics::StringViewHashAdapter& value) const noexcept
      {
        return value.hash();
      }
    };

    struct KeyEqual final
    {
      using is_transparent = void;

      bool operator()(
        const Generics::StringHashAdapter& left,
        const Generics::StringHashAdapter& right) const noexcept
      {
        return left.text() == right.text();
      }

      bool operator()(
        const Generics::StringHashAdapter& left,
        const Generics::StringViewHashAdapter& right) const noexcept
      {
        return left.text() == right.text();
      }

      bool operator()(
        const Generics::StringViewHashAdapter& left,
        const Generics::StringHashAdapter& right) const noexcept
      {
        return left.text() == right.text();
      }
    };

    using InFlightKeys = boost::unordered_flat_map<
      Generics::StringHashAdapter,
      unsigned long,
      KeyHash,
      KeyEqual>;

    using OperationGroupHook = boost::intrusive::list_member_hook<
      boost::intrusive::link_mode<boost::intrusive::safe_link>>;

    struct OperationGroup final
    {
      OperationGroup() = default;
      OperationGroup(const OperationGroup&) = delete;

      Generics::StringViewHashAdapter key;
      Generics::Time enqueue_time;
      Operations operations;
      OperationGroupHook queue_hook;
    };

    struct OperationGroupHash final
    {
      using is_transparent = void;

      std::size_t operator()(const OperationGroup* group) const noexcept
      {
        return group->key.hash();
      }

      std::size_t operator()(const Generics::StringViewHashAdapter& key) const noexcept
      {
        return key.hash();
      }
    };

    struct OperationGroupEqual final
    {
      using is_transparent = void;

      bool operator()(const OperationGroup* left, const OperationGroup* right) const noexcept
      {
        return left->key.text() == right->key.text();
      }

      bool operator()(
        const OperationGroup* left,
        const Generics::StringViewHashAdapter& right) const noexcept
      {
        return left->key.text() == right.text();
      }

      bool operator()(
        const Generics::StringViewHashAdapter& left,
        const OperationGroup* right) const noexcept
      {
        return left.text() == right->key.text();
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

    struct CollectResult final
    {
      bool collected = false;
      std::optional<Generics::Time> min_enqueue_time;
    };

    void account_operation_(OperationCounts& counts, OperationType type) const noexcept;

    static void add_in_flight_key_(InFlightKeys& keys, const Operation& operation);

    void enqueue_operations_i_(OperationQueue& target, Operations& source);

    void insert_operation_group_i_(
      OperationQueue& target,
      OperationGroup& group) noexcept;

    ReadyState request_ready_i_(
      bool has_pending_operations,
      bool force_update = false) noexcept;

    ReadyState make_ready_state_i_() const noexcept;

    const OperationGroup* find_ready_group_i_(
      const OperationQueue& operation_queue,
      bool write_operations) const noexcept;

    Generics::Time operation_ready_time_i_(const Generics::Time& enqueue_time) const noexcept;

    static Generics::Time time_from_microseconds_(std::int64_t microseconds) noexcept;

    CollectResult collect_batch_i_(
      Operations& batch,
      SelectedKeys& selected_keys) noexcept;

    unsigned long collect_from_queue_i_(
      OperationQueue& source,
      bool collect_reads,
      Operations& batch,
      SelectedKeys& selected_keys) noexcept;

    bool empty_i_() const noexcept;

  private:
    static constexpr std::int64_t NO_ENQUEUE_TIME = std::numeric_limits<std::int64_t>::max();

    const unsigned long batch_size_;
    const Generics::Time max_delay_;
    std::vector<std::unique_ptr<EnqueueBucket>> enqueue_buckets_;

    mutable std::mutex state_lock_;
    mutable std::condition_variable drain_condition_;
    InFlightKeys in_flight_read_keys_;
    InFlightKeys in_flight_write_keys_;
    std::atomic<unsigned long> pending_operations_{0};
    std::atomic<unsigned long> active_workers_{0};
    std::atomic<bool> ready_published_{false};
    std::atomic<std::int64_t> min_enqueue_time_{NO_ENQUEUE_TIME};
    std::size_t next_bucket_index_ = 0;
  };

  inline bool
  RocksDBBatchingProcessorQueue::is_write_operation(OperationType type) noexcept
  {
    return type == OT_TOUCH || type == OT_SAVE || type == OT_REMOVE;
  }

}
