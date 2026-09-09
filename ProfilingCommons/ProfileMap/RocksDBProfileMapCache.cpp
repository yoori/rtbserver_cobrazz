#include <algorithm>
#include <list>
#include <mutex>
#include <string_view>
#include <utility>

#include <boost/unordered/unordered_flat_map.hpp>

#include "RocksDBProfileMapCache.hpp"

namespace AdServer::ProfilingCommons
{
  class RocksDBProfileMapCache::Portion final
  {
  private:
    struct Key final
    {
      Key(std::uint64_t map_id_val, const std::string_view& key_val)
        : map_id(map_id_val),
          key(key_val.data(), key_val.size())
      {}

      std::uint64_t map_id;
      Generics::StringHashAdapter key;
    };

    struct KeyView final
    {
      std::uint64_t map_id;
      Generics::StringViewHashAdapter key;
    };

    struct KeyHash final
    {
      using is_transparent = void;

      std::size_t operator()(const Key& value) const noexcept
      {
        return hash(value.map_id, value.key.hash());
      }

      std::size_t operator()(const KeyView& value) const noexcept
      {
        return hash(value.map_id, value.key.hash());
      }

      static std::size_t hash(std::uint64_t map_id, std::size_t key_hash) noexcept
      {
        key_hash ^= static_cast<std::size_t>(map_id) + 0x9e3779b9U +
          (key_hash << 6) + (key_hash >> 2);
        return key_hash;
      }
    };

    struct KeyEqual final
    {
      using is_transparent = void;

      bool operator()(const Key& left, const Key& right) const noexcept
      {
        return left.map_id == right.map_id && left.key.text() == right.key.text();
      }

      bool operator()(const Key& left, const KeyView& right) const noexcept
      {
        return left.map_id == right.map_id && left.key.text() == right.key.text();
      }

      bool operator()(const KeyView& left, const Key& right) const noexcept
      {
        return left.map_id == right.map_id && left.key.text() == right.key.text();
      }
    };

    enum class EntryState
    {
      PROFILE,
      NOT_FOUND,
      INVALID
    };

    using Lru = std::list<Key>;

    struct Entry final
    {
      explicit Entry(Lru::iterator lru_it_val)
        : lru_it(lru_it_val)
      {}

      EntryState state = EntryState::INVALID;
      Generics::ConstSmartMemBuf_var profile;
      // current_revision orders cache publications; persisted_revision filters stale writes.
      std::uint64_t current_revision = 0;
      std::uint64_t persisted_revision = 0;
      std::size_t pending_writes = 0;
      Generics::Time write_time;
      bool touch_pending = false;
      std::size_t accounted_size = 0;
      Lru::iterator lru_it;
    };

    using Entries = boost::unordered_flat_map<Key, Entry, KeyHash, KeyEqual>;

  public:
    explicit Portion(RocksDBProfileMapCache& owner)
      : owner_(owner)
    {}

    static std::size_t hash(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key) noexcept
    {
      return KeyHash::hash(map_id, key.hash());
    }

    LookupResult lookup(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const Generics::Time& expire_time,
      OperationState& state,
      bool allow_touch)
    {
      LookupResult result;
      result.type = ResultType::MISS;

      const KeyView key_view{map_id, Generics::StringViewHashAdapter(key)};
      std::lock_guard guard(lock_);
      auto it = entries_.find(key_view);
      if (it == entries_.end())
      {
        set_miss_(state);
        owner_.misses_.fetch_add(1, std::memory_order_relaxed);
        return result;
      }

      Entry& entry = it->second;
      if (entry.state == EntryState::PROFILE)
      {
        const Generics::Time now = Generics::Time::get_time_of_day();
        if (entry.pending_writes == 0 && expire_time > Generics::Time::ZERO &&
          entry.write_time + expire_time <= now)
        {
          erase_i_(it);
          set_miss_(state);
          owner_.misses_.fetch_add(1, std::memory_order_relaxed);
          return result;
        }

        result.type = ResultType::PROFILE;
        result.profile = entry.profile;
        if (allow_touch && entry.pending_writes == 0 && !entry.touch_pending &&
          should_touch_(entry.write_time, now, expire_time))
        {
          entry.touch_pending = true;
          result.touch_revision = entry.current_revision;
          result.touch = true;
        }
      }
      else if (entry.state == EntryState::INVALID)
      {
        set_miss_(state);
        owner_.misses_.fetch_add(1, std::memory_order_relaxed);
        return result;
      }
      else
      {
        result.type = ResultType::NOT_FOUND;
      }

      touch_i_(entry);
      owner_.hits_.fetch_add(1, std::memory_order_relaxed);
      return result;
    }

    void publish(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const Generics::ConstSmartMemBuf_var& profile,
      bool remove,
      OperationState& state)
    {
      std::lock_guard guard(lock_);
      Entry& entry = find_or_create_i_(map_id, key.text());
      state.write_revision = ++next_revision_;
      entry.current_revision = state.write_revision;
      ++entry.pending_writes;
      entry.touch_pending = false;
      if (remove)
      {
        entry.state = EntryState::NOT_FOUND;
        entry.profile.reset();
      }
      else
      {
        entry.state = EntryState::PROFILE;
        entry.profile = profile;
      }
      touch_i_(entry);
      update_size_i_(entry);
      evict_i_();
    }

    void cancel(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const OperationState& state) noexcept
    {
      try
      {
        const KeyView key_view{map_id, Generics::StringViewHashAdapter(key)};
        std::lock_guard guard(lock_);
        auto it = entries_.find(key_view);
        if (it == entries_.end())
        {
          return;
        }

        Entry& entry = it->second;
        decrement_pending_(entry);
        if (entry.current_revision == state.write_revision)
        {
          entry.state = EntryState::INVALID;
          entry.profile.reset();
          entry.touch_pending = false;
          update_size_i_(entry);
        }

        erase_invalid_i_(it);
        evict_i_();
      }
      catch (...)
      {
      }
    }

    bool requires_write(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const OperationState& state,
      bool touch) noexcept
    {
      try
      {
        const KeyView key_view{map_id, Generics::StringViewHashAdapter(key)};
        std::lock_guard guard(lock_);
        const auto it = entries_.find(key_view);
        if (it == entries_.end())
        {
          return true;
        }

        const Entry& entry = it->second;
        if (touch)
        {
          return state.touch_revision != 0 &&
            entry.state == EntryState::PROFILE &&
            entry.current_revision == state.touch_revision &&
            entry.touch_pending;
        }

        return state.write_revision > entry.persisted_revision;
      }
      catch (...)
      {
        return true;
      }
    }

    void complete(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const OperationState& state,
      bool touch) noexcept
    {
      try
      {
        const KeyView key_view{map_id, Generics::StringViewHashAdapter(key)};
        std::lock_guard guard(lock_);
        auto it = entries_.find(key_view);
        if (it == entries_.end())
        {
          return;
        }

        Entry& entry = it->second;
        if (touch)
        {
          if (entry.current_revision == state.touch_revision && entry.touch_pending)
          {
            entry.touch_pending = false;
            entry.write_time = Generics::Time::get_time_of_day();
          }
        }
        else
        {
          entry.persisted_revision = std::max(
            entry.persisted_revision,
            state.write_revision);
          decrement_pending_(entry);
          if (entry.current_revision == state.write_revision)
          {
            entry.write_time = Generics::Time::get_time_of_day();
          }
        }

        touch_i_(entry);
        erase_invalid_i_(it);
        evict_i_();
      }
      catch (...)
      {
      }
    }

    void fail(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const OperationState& state,
      bool touch) noexcept
    {
      try
      {
        const KeyView key_view{map_id, Generics::StringViewHashAdapter(key)};
        std::lock_guard guard(lock_);
        auto it = entries_.find(key_view);
        if (it == entries_.end())
        {
          return;
        }

        Entry& entry = it->second;
        if (touch)
        {
          if (entry.current_revision == state.touch_revision)
          {
            entry.state = EntryState::INVALID;
            entry.profile.reset();
            entry.touch_pending = false;
            update_size_i_(entry);
          }
        }
        else
        {
          decrement_pending_(entry);
          if (entry.current_revision == state.write_revision)
          {
            entry.state = EntryState::INVALID;
            entry.profile.reset();
            entry.touch_pending = false;
            update_size_i_(entry);
          }
        }

        erase_invalid_i_(it);
        evict_i_();
      }
      catch (...)
      {
      }
    }

    std::uint64_t fill(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      const OperationState& state,
      Generics::ConstSmartMemBuf_var profile,
      const Generics::Time& write_time,
      bool schedule_touch) noexcept
    {
      try
      {
        if (!state.read_miss)
        {
          return 0;
        }

        const KeyView key_view{map_id, Generics::StringViewHashAdapter(key)};
        std::lock_guard guard(lock_);
        // Pending writes are not evicted, so a key appearing after the miss is enough
        // to reject the stale RocksDB result.
        if (entries_.find(key_view) != entries_.end())
        {
          return 0;
        }

        Entry& entry = find_or_create_i_(map_id, key.text());
        entry.state = EntryState::PROFILE;
        entry.profile = std::move(profile);
        entry.current_revision = ++next_revision_;
        entry.persisted_revision = entry.current_revision;
        entry.write_time = write_time;
        entry.touch_pending = schedule_touch;
        update_size_i_(entry);
        const std::uint64_t revision = entry.current_revision;
        evict_i_();
        return revision;
      }
      catch (...)
      {
        return 0;
      }
    }

    void cancel_touch(
      std::uint64_t map_id,
      const Generics::StringHashAdapter& key,
      std::uint64_t touch_revision) noexcept
    {
      try
      {
        const KeyView key_view{map_id, Generics::StringViewHashAdapter(key)};
        std::lock_guard guard(lock_);
        const auto it = entries_.find(key_view);
        if (it != entries_.end() && it->second.current_revision == touch_revision)
        {
          it->second.touch_pending = false;
          evict_i_();
        }
      }
      catch (...)
      {
      }
    }

    void remove_map(std::uint64_t map_id) noexcept
    {
      try
      {
        std::lock_guard guard(lock_);
        for (auto it = entries_.begin(); it != entries_.end();)
        {
          if (it->first.map_id == map_id)
          {
            auto current = it++;
            erase_i_(current);
          }
          else
          {
            ++it;
          }
        }
      }
      catch (...)
      {
      }
    }

  private:
    static void set_miss_(OperationState& state) noexcept
    {
      state.read_miss = true;
    }

    static bool should_touch_(
      const Generics::Time& write_time,
      const Generics::Time& now,
      const Generics::Time& expire_time) noexcept
    {
      const Generics::Time touch_period(expire_time.tv_sec / 4);
      return touch_period > Generics::Time::ZERO && write_time <= now &&
        write_time.tv_sec / touch_period.tv_sec < now.tv_sec / touch_period.tv_sec;
    }

    Entry& find_or_create_i_(std::uint64_t map_id, const std::string_view& key)
    {
      const KeyView key_view{map_id, Generics::StringViewHashAdapter(key)};
      const auto existing = entries_.find(key_view);
      if (existing != entries_.end())
      {
        return existing->second;
      }

      lru_.emplace_front(map_id, key);
      try
      {
        auto [it, inserted] = entries_.emplace(lru_.front(), Entry(lru_.begin()));
        static_cast<void>(inserted);
        owner_.entries_.fetch_add(1, std::memory_order_relaxed);
        return it->second;
      }
      catch (...)
      {
        lru_.pop_front();
        throw;
      }
    }

    void touch_i_(Entry& entry) noexcept
    {
      lru_.splice(lru_.begin(), lru_, entry.lru_it);
    }

    static std::size_t entry_size_(const Entry& entry) noexcept
    {
      const std::size_t profile_size = entry.profile ? entry.profile->membuf().size() : 0;
      return sizeof(Entry) + 2 * sizeof(Key) +
        2 * entry.lru_it->key.text().size() + profile_size;
    }

    void update_size_i_(Entry& entry) noexcept
    {
      const std::size_t new_size = entry_size_(entry);
      if (new_size > entry.accounted_size)
      {
        const std::size_t difference = new_size - entry.accounted_size;
        owner_.size_.fetch_add(difference, std::memory_order_relaxed);
      }
      else
      {
        const std::size_t difference = entry.accounted_size - new_size;
        owner_.size_.fetch_sub(difference, std::memory_order_relaxed);
      }
      entry.accounted_size = new_size;
    }

    void erase_i_(Entries::iterator it) noexcept
    {
      owner_.size_.fetch_sub(it->second.accounted_size, std::memory_order_relaxed);
      owner_.entries_.fetch_sub(1, std::memory_order_relaxed);
      lru_.erase(it->second.lru_it);
      entries_.erase(it);
    }

    void erase_invalid_i_(Entries::iterator it) noexcept
    {
      if (it->second.state == EntryState::INVALID && it->second.pending_writes == 0)
      {
        erase_i_(it);
      }
    }

    void evict_i_() noexcept
    {
      while (owner_.size_.load(std::memory_order_relaxed) > owner_.limit_)
      {
        auto lru_it = lru_.end();
        auto candidate = entries_.end();
        while (lru_it != lru_.begin())
        {
          --lru_it;
          const KeyView key_view{
            lru_it->map_id,
            Generics::StringViewHashAdapter(lru_it->key)};
          auto entry_it = entries_.find(key_view);
          if (entry_it != entries_.end() &&
            entry_it->second.pending_writes == 0 &&
            !entry_it->second.touch_pending)
          {
            candidate = entry_it;
            break;
          }
        }

        if (candidate == entries_.end())
        {
          return;
        }

        erase_i_(candidate);
        owner_.evictions_.fetch_add(1, std::memory_order_relaxed);
      }
    }

    static void decrement_pending_(Entry& entry) noexcept
    {
      if (entry.pending_writes != 0)
      {
        --entry.pending_writes;
      }
    }

  private:
    RocksDBProfileMapCache& owner_;
    std::mutex lock_;
    Entries entries_;
    Lru lru_;
    std::uint64_t next_revision_ = 0;
  };

  RocksDBProfileMapCache::RocksDBProfileMapCache(
    std::size_t limit,
    unsigned long portions_count)
    : limit_(limit)
  {
    portions_count = std::max(1UL, portions_count);
    portions_.reserve(portions_count);
    for (unsigned long i = 0; i < portions_count; ++i)
    {
      portions_.emplace_back(std::make_unique<Portion>(*this));
    }
  }

  RocksDBProfileMapCache::~RocksDBProfileMapCache() noexcept = default;

  RocksDBProfileMapCache::LookupResult
  RocksDBProfileMapCache::lookup(
    std::uint64_t map_id,
    const Generics::StringHashAdapter& key,
    const Generics::Time& expire_time,
    OperationState& state,
    bool allow_touch)
  {
    return portion_(map_id, key).lookup(map_id, key, expire_time, state, allow_touch);
  }

  void
  RocksDBProfileMapCache::publish(
    std::uint64_t map_id,
    const Generics::StringHashAdapter& key,
    const Generics::ConstSmartMemBuf_var& profile,
    bool remove,
    OperationState& state)
  {
    portion_(map_id, key).publish(map_id, key, profile, remove, state);
  }

  void
  RocksDBProfileMapCache::cancel(
    std::uint64_t map_id,
    const Generics::StringHashAdapter& key,
    const OperationState& state) noexcept
  {
    portion_(map_id, key).cancel(map_id, key, state);
  }

  bool
  RocksDBProfileMapCache::requires_write(
    std::uint64_t map_id,
    const Generics::StringHashAdapter& key,
    const OperationState& state,
    bool touch) noexcept
  {
    return portion_(map_id, key).requires_write(map_id, key, state, touch);
  }

  void
  RocksDBProfileMapCache::complete(
    std::uint64_t map_id,
    const Generics::StringHashAdapter& key,
    const OperationState& state,
    bool touch) noexcept
  {
    portion_(map_id, key).complete(map_id, key, state, touch);
  }

  void
  RocksDBProfileMapCache::fail(
    std::uint64_t map_id,
    const Generics::StringHashAdapter& key,
    const OperationState& state,
    bool touch) noexcept
  {
    portion_(map_id, key).fail(map_id, key, state, touch);
  }

  std::uint64_t
  RocksDBProfileMapCache::fill(
    std::uint64_t map_id,
    const Generics::StringHashAdapter& key,
    const OperationState& state,
    Generics::ConstSmartMemBuf_var profile,
    const Generics::Time& write_time,
    bool schedule_touch) noexcept
  {
    return portion_(map_id, key).fill(
      map_id,
      key,
      state,
      std::move(profile),
      write_time,
      schedule_touch);
  }

  void
  RocksDBProfileMapCache::cancel_touch(
    std::uint64_t map_id,
    const Generics::StringHashAdapter& key,
    std::uint64_t touch_revision) noexcept
  {
    portion_(map_id, key).cancel_touch(map_id, key, touch_revision);
  }

  void RocksDBProfileMapCache::remove_map(std::uint64_t map_id) noexcept
  {
    for (auto& portion : portions_)
    {
      portion->remove_map(map_id);
    }
  }

  RocksDBProfileMapCache::Stats RocksDBProfileMapCache::stats() const noexcept
  {
    return {
      size_.load(std::memory_order_relaxed),
      entries_.load(std::memory_order_relaxed),
      hits_.load(std::memory_order_relaxed),
      misses_.load(std::memory_order_relaxed),
      evictions_.load(std::memory_order_relaxed)
    };
  }

  RocksDBProfileMapCache::Portion&
  RocksDBProfileMapCache::portion_(
    std::uint64_t map_id,
    const Generics::StringHashAdapter& key) noexcept
  {
    return *portions_[Portion::hash(map_id, key) % portions_.size()];
  }
}
