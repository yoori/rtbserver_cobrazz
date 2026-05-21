#pragma once

#include <future>
#include <optional>
#include <utility>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include "ProfileMap.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  template<typename KeyType, typename AsyncMapType>
  class MigratingProfileMap:
    public virtual ProfileMap<KeyType>,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, typename ProfileMap<KeyType>::Exception);

    MigratingProfileMap(
      AsyncMapType* primary_map,
      AsyncProfileMap<KeyType>* fallback_async_map)
      noexcept
      : primary_map_(ReferenceCounting::add_ref(primary_map)),
        fallback_async_map_(ReferenceCounting::add_ref(fallback_async_map))
    {}

    bool
    check_profile(const KeyType& key) const override
    {
      return wait_check_(primary_map_.in(), key) ||
        wait_check_(fallback_async_map_.in(), key);
    }

    Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0) override
    {
      Generics::ConstSmartMemBuf_var result = wait_get_(
        primary_map_.in(),
        key,
        last_access_time);
      if(result.in())
      {
        return result;
      }

      return wait_get_(fallback_async_map_.in(), key, last_access_time);
    }

    void
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME) override
    {
      static_cast<void>(op_priority);

      wait_save_(primary_map_.in(), key, mem_buf, now);
    }

    bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME) override
    {
      const bool primary_removed = wait_remove_(
        primary_map_.in(),
        key,
        op_priority);
      const bool fallback_removed = wait_remove_(
        fallback_async_map_.in(),
        key,
        op_priority);
      return primary_removed || fallback_removed;
    }

    void
    clear_expired(const Generics::Time& expire_time) override
    {
      wait_clear_expired_(fallback_async_map_.in(), expire_time);
    }

    unsigned long
    size() const noexcept override
    {
      return primary_map_->size();
    }

    unsigned long
    area_size() const noexcept override
    {
      return primary_map_->area_size();
    }

    typename ProfileMap<KeyType>::Stats
    stats() const noexcept override
    {
      return primary_map_->stats();
    }

  protected:
    ~MigratingProfileMap() noexcept override
    {
      primary_map_->deactivate_object();
      primary_map_->wait_object();
    }

  private:
    bool
    wait_check_(
      AsyncProfileMap<KeyType>* map,
      const KeyType& key) const
    {
      using Result = std::pair<bool, std::optional<std::string>>;
      std::promise<Result> promise;
      std::future<Result> future = promise.get_future();

      map->check_profile_async(
        key,
        [&promise](bool result, std::optional<std::string> error)
        {
          promise.set_value(std::make_pair(result, std::move(error)));
        });

      const auto result = future.get();
      if(result.second)
      {
        throw Exception(*result.second);
      }

      return result.first;
    }

    Generics::ConstSmartMemBuf_var
    wait_get_(
      AsyncProfileMap<KeyType>* map,
      const KeyType& key,
      Generics::Time* last_access_time)
    {
      using Result = std::pair<
        Generics::ConstSmartMemBuf_var,
        std::optional<std::string>>;
      std::promise<Result> promise;
      std::future<Result> future = promise.get_future();

      map->get_profile_async(
        key,
        [&promise](
          const Generics::ConstSmartMemBuf_var& profile,
          std::optional<std::string> error)
        {
          promise.set_value(std::make_pair(profile, std::move(error)));
        },
        last_access_time ?
          std::optional<Generics::Time>(*last_access_time) :
          std::nullopt);

      const auto result = future.get();
      if(result.second)
      {
        throw Exception(*result.second);
      }

      return result.first;
    }

    void
    wait_save_(
      AsyncProfileMap<KeyType>* map,
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now)
    {
      std::promise<std::optional<std::string>> promise;
      std::future<std::optional<std::string>> future = promise.get_future();

      map->save_profile_async(
        key,
        mem_buf,
        now,
        [&promise](std::optional<std::string> error)
        {
          promise.set_value(std::move(error));
        });

      const auto error = future.get();
      if(error)
      {
        throw Exception(*error);
      }
    }

    bool
    wait_remove_(
      AsyncProfileMap<KeyType>* map,
      const KeyType& key,
      OperationPriority op_priority)
    {
      using Result = std::pair<bool, std::optional<std::string>>;
      std::promise<Result> promise;
      std::future<Result> future = promise.get_future();

      map->remove_profile_async(
        key,
        op_priority,
        [&promise](bool result, std::optional<std::string> error)
        {
          promise.set_value(std::make_pair(result, std::move(error)));
        });

      const auto result = future.get();
      if(result.second)
      {
        throw Exception(*result.second);
      }

      return result.first;
    }

    void
    wait_clear_expired_(
      AsyncProfileMap<KeyType>* map,
      const Generics::Time& expire_time)
    {
      std::promise<void> promise;
      std::future<void> future = promise.get_future();

      map->clear_expired_async(
        expire_time,
        [&promise]()
        {
          promise.set_value();
        });

      future.get();
    }

    ReferenceCounting::SmartPtr<
      AsyncMapType,
      ReferenceCounting::PolicyNotNull> primary_map_;
    ReferenceCounting::SmartPtr<
      AsyncProfileMap<KeyType>,
      ReferenceCounting::PolicyNotNull> fallback_async_map_;
  };
}
}
