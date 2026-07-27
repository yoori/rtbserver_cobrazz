#pragma once

#include <coroutine>
#include <exception>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <Generics/Time.hpp>
#include <Commons/Coro/ScopedCoroutineResumeScheduler.hpp>
#include <Commons/Coro/Utils.hpp>
#include <Commons/ExecutorPool.hpp>

namespace AdServer::Commons
{
  template<typename Cache, typename... Args>
  class AsyncCacheAwaiter;

  template<typename Key, typename Value, typename... Args>
  class AsyncCache: public std::enable_shared_from_this<AsyncCache<Key, Value, Args...>>
  {
  public:
    static constexpr std::size_t UNLIMITED_SIZE = static_cast<std::size_t>(-1);

    using KeyType = Key;
    using ValueType = Value;

    struct Holder
    {
      using OptionalValue = std::optional<Value>;

      Holder(
        Value value_val,
        const Generics::Time& check_time_val,
        const Generics::Time& mod_time_val,
        std::size_t size_val)
        : Holder(
            OptionalValue(std::move(value_val)),
            check_time_val,
            mod_time_val,
            size_val)
      {}

      Holder(
        OptionalValue value_val,
        const Generics::Time& check_time_val,
        const Generics::Time& mod_time_val,
        std::size_t size_val)
        : value(std::move(value_val)),
          check_time(check_time_val),
          mod_time(mod_time_val),
          size(size_val)
      {}

      OptionalValue value;
      Generics::Time check_time;
      Generics::Time mod_time;
      std::size_t size;
    };

    using HolderPtr = std::shared_ptr<Holder>;
    using GetCallback = std::function<void(Value)>;
    using UpdateCallback = std::function<void(HolderPtr)>;
    using CacheAwaiter = AsyncCacheAwaiter<AsyncCache<Key, Value, Args...>, Args...>;
    using SyncUpdate = std::function<HolderPtr(
      const Key& key,
      const HolderPtr& old_holder,
      const Args&... args)>;
    using AsyncUpdate = std::function<void(
      Key key,
      HolderPtr old_holder,
      UpdateCallback callback,
      Args... args)>;

    AsyncCache(
      std::size_t max_size,
      const Generics::Time& update_period,
      const Generics::Time& max_keep_time,
      SyncUpdate sync_update,
      AsyncUpdate async_update);

    Value
    get_sync(const Key& key, Args... args);

    void
    get_async(const Key& key, GetCallback callback, Args... args) noexcept;

    CacheAwaiter
    co_get(
      std::shared_ptr<ExecutorPool> workers,
      Key key,
      Args... args) noexcept;

    CacheAwaiter
    co_get(
      Key key,
      Args... args) noexcept;

  private:
    struct Entry
    {
      HolderPtr holder;
      typename std::list<Key>::iterator lru_it;
      bool update_in_progress = false;
    };

    struct LookupResult
    {
      HolderPtr old_holder;
      HolderPtr background_holder;
      Value cached_value;
      bool cache_hit = false;
      bool run_background_update = false;
    };

    struct PendingLoad
    {
      std::vector<GetCallback> callbacks;
    };

    struct PendingLoadResult
    {
      Value cached_value;
      bool cache_hit = false;
      bool start_load = false;
    };

  private:
    LookupResult
    lookup_(const Key& key, const Generics::Time& now);

    PendingLoadResult
    enqueue_load_(
      const Key& key,
      HolderPtr& old_holder,
      const Generics::Time& now,
      GetCallback& callback);

    void
    touch_i_(Entry& entry);

    void
    erase_lru_i_(Entry& entry);

    void
    erase_i_(typename std::unordered_map<Key, Entry>::iterator it);

    void
    insert_i_(const Key& key, HolderPtr holder);

    void
    insert_(const Key& key, HolderPtr holder);

    void
    complete_load_(const Key& key, HolderPtr holder) noexcept;

    void
    start_background_update_(Key key, HolderPtr old_holder, Args... args) noexcept;

    void
    complete_background_update_(
      const Key& key,
      const HolderPtr& old_holder,
      HolderPtr new_holder) noexcept;

    Value
    value_(const HolderPtr& holder) const;

  private:
    const std::size_t max_size_;
    const Generics::Time update_period_;
    const Generics::Time max_keep_time_;
    SyncUpdate sync_update_;
    AsyncUpdate async_update_;

    mutable std::shared_mutex entries_lock_;
    std::unordered_map<Key, Entry> entries_;
    std::unordered_map<Key, PendingLoad> pending_loads_;

    mutable std::mutex lru_lock_;
    std::list<Key> lru_;
    std::size_t current_size_ = 0;
  };

  template<typename Cache, typename... Args>
  class AsyncCacheAwaiter
  {
  public:
    using CachePtr = std::shared_ptr<Cache>;
    using Key = typename Cache::KeyType;
    using Value = typename Cache::ValueType;

    AsyncCacheAwaiter(
      CachePtr cache,
      std::shared_ptr<ExecutorPool> workers,
      Key key,
      Args... args);

    AsyncCacheAwaiter(const AsyncCacheAwaiter&) = delete;
    AsyncCacheAwaiter& operator=(const AsyncCacheAwaiter&) = delete;
    AsyncCacheAwaiter(AsyncCacheAwaiter&&) noexcept = default;
    AsyncCacheAwaiter& operator=(AsyncCacheAwaiter&&) noexcept = default;

    ~AsyncCacheAwaiter() noexcept;

    bool
    await_ready() const noexcept;

    bool
    await_suspend(std::coroutine_handle<> handle);

    Value
    await_resume();

  private:
    struct State
    {
      std::mutex lock;
      std::coroutine_handle<> handle;
      CoroutineResumeScheduler resume_scheduler;
      Value result;
      std::exception_ptr exception;
      bool completed = false;
      bool suspended = false;
      bool cancelled = false;
    };

    std::shared_ptr<State> state_;
    CachePtr cache_;
    std::shared_ptr<ExecutorPool> workers_;
    Key key_;
    std::tuple<Args...> args_;
  };

  template<typename Cache, typename... Args>
  inline
  AsyncCacheAwaiter<Cache, Args...>::AsyncCacheAwaiter(
    typename AsyncCacheAwaiter<Cache, Args...>::CachePtr cache,
    std::shared_ptr<ExecutorPool> workers,
    typename AsyncCacheAwaiter<Cache, Args...>::Key key,
    Args... args)
    : state_(std::make_shared<State>()),
      cache_(std::move(cache)),
      workers_(std::move(workers)),
      key_(std::move(key)),
      args_(std::move(args)...)
  {}

  template<typename Cache, typename... Args>
  inline
  AsyncCacheAwaiter<Cache, Args...>::~AsyncCacheAwaiter() noexcept
  {
    if (state_)
    {
      std::lock_guard<std::mutex> guard(state_->lock);
      state_->cancelled = true;
      state_->handle = {};
    }
  }

  template<typename Cache, typename... Args>
  inline bool
  AsyncCacheAwaiter<Cache, Args...>::await_ready() const noexcept
  {
    return false;
  }

  template<typename Cache, typename... Args>
  inline bool
  AsyncCacheAwaiter<Cache, Args...>::await_suspend(std::coroutine_handle<> handle)
  {
    {
      std::lock_guard<std::mutex> guard(state_->lock);
      state_->handle = handle;
      if(const auto* scheduler = current_coroutine_resume_scheduler())
      {
        state_->resume_scheduler = *scheduler;
      }
    }

    try
    {
      auto callback =
        [
          state = state_,
          workers = workers_
        ](typename AsyncCacheAwaiter<Cache, Args...>::Value result) mutable
        {
          auto complete =
            [
              state = std::move(state),
              result = std::move(result)
            ]() mutable
            {
              std::coroutine_handle<> handle;
              CoroutineResumeScheduler resume_scheduler;
              {
                std::lock_guard<std::mutex> guard(state->lock);
                if (state->cancelled)
                {
                  return;
                }

                state->result = std::move(result);
                state->completed = true;
                if (state->suspended)
                {
                  handle = state->handle;
                  resume_scheduler = state->resume_scheduler;
                }
              }

              if (handle)
              {
                if(resume_scheduler)
                {
                  resume_scheduler(handle);
                }
                else
                {
                  resume_coroutine(handle);
                }
              }
            };

          if (workers)
          {
            workers->post(std::move(complete));
          }
          else
          {
            complete();
          }
        };

      std::apply(
        [this, callback = std::move(callback)](auto&... args) mutable
        {
          cache_->get_async(key_, std::move(callback), std::move(args)...);
        },
        args_);
    }
    catch(...)
    {
      std::lock_guard<std::mutex> guard(state_->lock);
      state_->exception = std::current_exception();
      state_->completed = true;
    }

    std::lock_guard<std::mutex> guard(state_->lock);
    if (state_->completed)
    {
      return false;
    }

    state_->suspended = true;
    return true;
  }

  template<typename Cache, typename... Args>
  inline typename AsyncCacheAwaiter<Cache, Args...>::Value
  AsyncCacheAwaiter<Cache, Args...>::await_resume()
  {
    if (state_->exception)
    {
      std::rethrow_exception(state_->exception);
    }

    return std::move(state_->result);
  }

}

namespace AdServer::Commons
{
  template<typename Key, typename Value, typename... Args>
  inline
  AsyncCache<Key, Value, Args...>::AsyncCache(
    std::size_t max_size,
    const Generics::Time& update_period,
    const Generics::Time& max_keep_time,
    SyncUpdate sync_update,
    AsyncUpdate async_update)
    : max_size_(max_size),
      update_period_(update_period),
      max_keep_time_(max_keep_time),
      sync_update_(std::move(sync_update)),
      async_update_(std::move(async_update))
  {}

  template<typename Key, typename Value, typename... Args>
  inline typename AsyncCache<Key, Value, Args...>::CacheAwaiter
  AsyncCache<Key, Value, Args...>::co_get(
    std::shared_ptr<ExecutorPool> workers,
    Key key,
    Args... args) noexcept
  {
    return CacheAwaiter(
      this->shared_from_this(),
      std::move(workers),
      std::move(key),
      std::move(args)...);
  }

  template<typename Key, typename Value, typename... Args>
  inline typename AsyncCache<Key, Value, Args...>::CacheAwaiter
  AsyncCache<Key, Value, Args...>::co_get(
    Key key,
    Args... args) noexcept
  {
    return CacheAwaiter(
      this->shared_from_this(),
      std::shared_ptr<ExecutorPool>(),
      std::move(key),
      std::move(args)...);
  }

  template<typename Key, typename Value, typename... Args>
  inline Value
  AsyncCache<Key, Value, Args...>::get_sync(const Key& key, Args... args)
  {
    const Generics::Time now = Generics::Time::get_time_of_day();

    LookupResult lookup = lookup_(key, now);
    if (lookup.run_background_update)
    {
      start_background_update_(
        key,
        std::move(lookup.background_holder),
        std::move(args)...);
    }

    if (lookup.cache_hit)
    {
      return std::move(lookup.cached_value);
    }

    HolderPtr holder = sync_update_(key, lookup.old_holder, args...);
    if (!holder)
    {
      return Value();
    }

    insert_(key, holder);

    return value_(holder);
  }

  template<typename Key, typename Value, typename... Args>
  inline void
  AsyncCache<Key, Value, Args...>::get_async(
    const Key& key,
    GetCallback callback,
    Args... args) noexcept
  {
    try
    {
      const Generics::Time now = Generics::Time::get_time_of_day();

      LookupResult lookup = lookup_(key, now);
      if (lookup.run_background_update)
      {
        start_background_update_(
          key,
          std::move(lookup.background_holder),
          std::move(args)...);
      }

      if (lookup.cache_hit)
      {
        callback(std::move(lookup.cached_value));
        return;
      }

      auto self = this->shared_from_this();
      Key update_key = key;
      Key cache_key = key;
      HolderPtr old_holder = std::move(lookup.old_holder);
      PendingLoadResult pending_load = enqueue_load_(key, old_holder, now, callback);
      if (pending_load.cache_hit)
      {
        callback(std::move(pending_load.cached_value));
        return;
      }

      if (!pending_load.start_load)
      {
        return;
      }

      try
      {
        async_update_(
          std::move(update_key),
          std::move(old_holder),
          [self = std::move(self), cache_key = std::move(cache_key)](HolderPtr holder) mutable
          {
            self->complete_load_(cache_key, std::move(holder));
          },
          std::move(args)...);
      }
      catch(...)
      {
        complete_load_(cache_key, HolderPtr());
      }
    }
    catch(...)
    {
      callback(Value());
    }
  }

  template<typename Key, typename Value, typename... Args>
  inline typename AsyncCache<Key, Value, Args...>::LookupResult
  AsyncCache<Key, Value, Args...>::lookup_(const Key& key, const Generics::Time& now)
  {
    LookupResult result;
    bool need_write_lookup = false;

    {
      std::shared_lock<std::shared_mutex> guard(entries_lock_);
      auto it = entries_.find(key);
      if (it != entries_.end())
      {
        const HolderPtr& holder = it->second.holder;
        if (holder->check_time + max_keep_time_ <= now ||
          (holder->check_time + update_period_ <= now && !it->second.update_in_progress))
        {
          need_write_lookup = true;
        }
        else
        {
          result.cached_value = value_(holder);
          result.cache_hit = true;
          touch_i_(it->second);
        }
      }
    }

    if (!need_write_lookup)
    {
      return result;
    }

    {
      std::unique_lock<std::shared_mutex> guard(entries_lock_);
      auto it = entries_.find(key);
      if (it == entries_.end())
      {
        return result;
      }

      const HolderPtr& holder = it->second.holder;
      if (holder->check_time + max_keep_time_ <= now)
      {
        result.old_holder = holder;
        erase_i_(it);
        return result;
      }

      result.cached_value = value_(holder);
      result.cache_hit = true;
      if (holder->check_time + update_period_ <= now && !it->second.update_in_progress)
      {
        it->second.update_in_progress = true;
        result.background_holder = holder;
        result.run_background_update = true;
      }

      touch_i_(it->second);
    }

    return result;
  }

  template<typename Key, typename Value, typename... Args>
  inline typename AsyncCache<Key, Value, Args...>::PendingLoadResult
  AsyncCache<Key, Value, Args...>::enqueue_load_(
    const Key& key,
    HolderPtr& old_holder,
    const Generics::Time& now,
    GetCallback& callback)
  {
    PendingLoadResult result;

    std::unique_lock<std::shared_mutex> guard(entries_lock_);

    auto entry_it = entries_.find(key);
    if (entry_it != entries_.end())
    {
      const HolderPtr& holder = entry_it->second.holder;
      if (holder->check_time + max_keep_time_ <= now)
      {
        old_holder = holder;
        erase_i_(entry_it);
      }
      else
      {
        result.cached_value = value_(holder);
        result.cache_hit = true;
        touch_i_(entry_it->second);
        return result;
      }
    }

    auto pending_it = pending_loads_.find(key);
    if (pending_it == pending_loads_.end())
    {
      pending_it = pending_loads_.emplace(key, PendingLoad()).first;
      result.start_load = true;
    }

    pending_it->second.callbacks.emplace_back(std::move(callback));
    return result;
  }

  template<typename Key, typename Value, typename... Args>
  inline void
  AsyncCache<Key, Value, Args...>::complete_load_(
    const Key& key,
    HolderPtr holder) noexcept
  {
    std::vector<GetCallback> callbacks;
    Value value;

    try
    {
      std::unique_lock<std::shared_mutex> guard(entries_lock_);

      auto pending_it = pending_loads_.find(key);
      if (pending_it == pending_loads_.end())
      {
        return;
      }

      callbacks.swap(pending_it->second.callbacks);
      pending_loads_.erase(pending_it);

      if (holder)
      {
        value = value_(holder);
        insert_i_(key, std::move(holder));
      }
    }
    catch(...)
    {}

    for (auto& callback : callbacks)
    {
      try
      {
        callback(value);
      }
      catch(...)
      {}
    }
  }

  template<typename Key, typename Value, typename... Args>
  inline void
  AsyncCache<Key, Value, Args...>::touch_i_(Entry& entry)
  {
    if (max_size_ == UNLIMITED_SIZE)
    {
      return;
    }

    std::lock_guard<std::mutex> guard(lru_lock_);
    lru_.splice(lru_.end(), lru_, entry.lru_it);
  }

  template<typename Key, typename Value, typename... Args>
  inline void
  AsyncCache<Key, Value, Args...>::erase_lru_i_(Entry& entry)
  {
    current_size_ -= entry.holder->size;
    lru_.erase(entry.lru_it);
  }

  template<typename Key, typename Value, typename... Args>
  inline void
  AsyncCache<Key, Value, Args...>::erase_i_(typename std::unordered_map<Key, Entry>::iterator it)
  {
    if (max_size_ != UNLIMITED_SIZE)
    {
      std::lock_guard<std::mutex> guard(lru_lock_);
      erase_lru_i_(it->second);
    }

    entries_.erase(it);
  }

  template<typename Key, typename Value, typename... Args>
  inline void
  AsyncCache<Key, Value, Args...>::insert_i_(const Key& key, HolderPtr holder)
  {
    if (!holder)
    {
      return;
    }

    auto existing_it = entries_.find(key);
    if (existing_it != entries_.end())
    {
      erase_i_(existing_it);
    }

    if (max_size_ == UNLIMITED_SIZE)
    {
      entries_.emplace(
        key,
        Entry{std::move(holder), typename std::list<Key>::iterator()});
      return;
    }

    const std::size_t holder_size = holder->size;
    std::lock_guard<std::mutex> guard(lru_lock_);
    while (current_size_ + holder_size > max_size_ && !lru_.empty())
    {
      const Key oldest_key = lru_.front();
      auto oldest_it = entries_.find(oldest_key);
      if (oldest_it == entries_.end())
      {
        lru_.pop_front();
        continue;
      }

      erase_lru_i_(oldest_it->second);
      entries_.erase(oldest_it);
    }

    if (current_size_ + holder_size > max_size_)
    {
      return;
    }

    lru_.push_back(key);
    entries_.emplace(
      key,
      Entry{
        std::move(holder),
        std::prev(lru_.end())});
    current_size_ += holder_size;
  }

  template<typename Key, typename Value, typename... Args>
  inline void
  AsyncCache<Key, Value, Args...>::insert_(const Key& key, HolderPtr holder)
  {
    if (!holder)
    {
      return;
    }

    std::unique_lock<std::shared_mutex> guard(entries_lock_);
    insert_i_(key, std::move(holder));
  }

  template<typename Key, typename Value, typename... Args>
  inline void
  AsyncCache<Key, Value, Args...>::start_background_update_(
    Key key,
    HolderPtr old_holder,
    Args... args) noexcept
  {
    const Key error_key = key;
    const HolderPtr error_old_holder = old_holder;

    try
    {
      auto self = this->shared_from_this();
      Key callback_key = key;
      HolderPtr callback_old_holder = old_holder;
      async_update_(
        std::move(key),
        std::move(old_holder),
        [
          self = std::move(self),
          callback_key = std::move(callback_key),
          old_holder = std::move(callback_old_holder)
        ](HolderPtr new_holder) mutable
        {
          self->complete_background_update_(
            callback_key,
            old_holder,
            std::move(new_holder));
        },
        std::move(args)...);
    }
    catch(...)
    {
      complete_background_update_(error_key, error_old_holder, HolderPtr());
    }
  }

  template<typename Key, typename Value, typename... Args>
  inline void
  AsyncCache<Key, Value, Args...>::complete_background_update_(
    const Key& key,
    const HolderPtr& old_holder,
    HolderPtr new_holder) noexcept
  {
    try
    {
      std::unique_lock<std::shared_mutex> guard(entries_lock_);

      auto it = entries_.find(key);
      if (it == entries_.end() || it->second.holder != old_holder)
      {
        return;
      }

      if (!new_holder)
      {
        it->second.update_in_progress = false;
        return;
      }

      erase_i_(it);
      insert_i_(key, std::move(new_holder));
    }
    catch(...)
    {}
  }

  template<typename Key, typename Value, typename... Args>
  inline Value
  AsyncCache<Key, Value, Args...>::value_(const HolderPtr& holder) const
  {
    return holder && holder->value ? *holder->value : Value();
  }
}
