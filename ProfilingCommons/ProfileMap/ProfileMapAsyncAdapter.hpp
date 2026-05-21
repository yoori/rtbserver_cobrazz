#pragma once

#include <exception>
#include <memory>
#include <optional>
#include <string>

#include <Commons/ExecutorPool.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include "ProfileMap.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  template<typename KeyType>
  class ProfileMapAsyncAdapter:
    public virtual AsyncProfileMap<KeyType>,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    ProfileMapAsyncAdapter(
      ProfileMap<KeyType>* profile_map,
      std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool)
      noexcept
      : profile_map_(ReferenceCounting::add_ref(profile_map)),
        executor_pool_(std::move(executor_pool))
    {}

    void
    check_profile_async(
      const KeyType& key,
      typename AsyncProfileMap<KeyType>::CheckCallback callback) const override
    {
      const auto profile_map = profile_map_;
      executor_pool_->post(
        [profile_map, key, callback = std::move(callback)]() mutable
        {
          bool result = false;
          std::optional<std::string> error;
          try
          {
            result = profile_map->check_profile(key);
          }
          catch(const std::exception& ex)
          {
            error = ex.what();
          }
          catch(...)
          {
            error = "unknown check error";
          }

          if(callback)
          {
            callback(result, std::move(error));
          }
        });
    }

    Generics::ConstSmartMemBuf_var
    get_profile_async(
      const KeyType& key,
      typename AsyncProfileMap<KeyType>::GetCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt) override
    {
      const auto profile_map = profile_map_;
      executor_pool_->post(
        [profile_map,
         key,
         callback = std::move(callback),
         last_access_time]() mutable
        {
          Generics::ConstSmartMemBuf_var result;
          std::optional<std::string> error;
          try
          {
            Generics::Time access_time;
            result = profile_map->get_profile(
              key,
              last_access_time ? &access_time : nullptr);
          }
          catch(const std::exception& ex)
          {
            error = ex.what();
          }
          catch(...)
          {
            error = "unknown get error";
          }

          if(callback)
          {
            callback(result, std::move(error));
          }
        });

      return Generics::ConstSmartMemBuf_var();
    }

    void
    save_profile_async(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      typename AsyncProfileMap<KeyType>::SaveCallback callback =
        typename AsyncProfileMap<KeyType>::SaveCallback()) override
    {
      const auto profile_map = profile_map_;
      Generics::ConstSmartMemBuf_var profile_holder(
        ReferenceCounting::add_ref(mem_buf));
      executor_pool_->post(
        [profile_map,
         key,
         profile_holder,
         now,
         callback = std::move(callback)]() mutable
        {
          std::optional<std::string> error;
          try
          {
            profile_map->save_profile(key, profile_holder, now);
          }
          catch(const std::exception& ex)
          {
            error = ex.what();
          }
          catch(...)
          {
            error = "unknown save error";
          }

          if(callback)
          {
            callback(std::move(error));
          }
        });
    }

    void
    remove_profile_async(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME,
      typename AsyncProfileMap<KeyType>::RemoveCallback callback =
        typename AsyncProfileMap<KeyType>::RemoveCallback()) override
    {
      const auto profile_map = profile_map_;
      executor_pool_->post(
        [profile_map,
         key,
         op_priority,
         callback = std::move(callback)]() mutable
        {
          bool result = false;
          std::optional<std::string> error;
          try
          {
            result = profile_map->remove_profile(key, op_priority);
          }
          catch(const std::exception& ex)
          {
            error = ex.what();
          }
          catch(...)
          {
            error = "unknown remove error";
          }

          if(callback)
          {
            callback(result, std::move(error));
          }
        });
    }

    void
    clear_expired_async(
      const Generics::Time& expire_time,
      typename AsyncProfileMap<KeyType>::CompleteCallback complete =
        typename AsyncProfileMap<KeyType>::CompleteCallback()) override
    {
      const auto profile_map = profile_map_;
      executor_pool_->post(
        [profile_map,
         expire_time,
         complete = std::move(complete)]() mutable
        {
          try
          {
            profile_map->clear_expired(expire_time);
          }
          catch(...)
          {}

          if(complete)
          {
            complete();
          }
        });
    }

  protected:
    ~ProfileMapAsyncAdapter() noexcept override = default;

  private:
    ReferenceCounting::SmartPtr<
      ProfileMap<KeyType>,
      ReferenceCounting::PolicyNotNull> profile_map_;
    std::shared_ptr<AdServer::Commons::ExecutorPool> executor_pool_;
  };
}
}
