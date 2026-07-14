#pragma once

namespace AdServer
{
namespace ProfilingCommons
{
  namespace ProfileMapDetail
  {
    inline Generics::SmartMemBuf_var
    copy_own_profile_(const Generics::ConstSmartMemBuf* profile)
    {
      if(profile)
      {
        Generics::SmartMemBuf_var result(new Generics::SmartMemBuf());
        result->membuf().assign(
          profile->membuf().data(),
          profile->membuf().size());
        return result;
      }

      return Generics::SmartMemBuf_var();
    }
  }

  template<typename KeyType>
  void
  ProfileMap<KeyType>::wait_preconditions(
    const KeyType&,
    OperationPriority) const
    /*throw(Exception)*/
  {}

  template<typename KeyType>
  Generics::SmartMemBuf_var
  ProfileMap<KeyType>::get_own_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
    /*throw(Exception)*/
  {
    return ProfileMapDetail::copy_own_profile_(
      get_profile(key, last_access_time));
  }

  template<typename KeyType>
  void
  ProfileMap<KeyType>::clear_expired(const Generics::Time&)
    /*throw(Exception)*/
  {
    throw Exception("clear_expired isn't supported");
  }

  template<typename KeyType>
  void
  ProfileMap<KeyType>::process_keys(
    std::function<void(const KeyType&)>,
    std::function<void(void)>)
    /*throw(Exception)*/
  {
    throw Exception("process_keys isn't supported");
  }

  template<typename KeyType>
  unsigned long
  ProfileMap<KeyType>::size() const noexcept
  {
    return 0;
  }

  template<typename KeyType>
  unsigned long
  ProfileMap<KeyType>::area_size() const noexcept
  {
    return 0;
  }

  template<typename KeyType>
  typename ProfileMap<KeyType>::Stats
  ProfileMap<KeyType>::stats() const noexcept
  {
    return {};
  }

  template<typename KeyType>
  template<typename ResultType>
  AsyncProfileMap<KeyType>::CallbackAwaitable<ResultType>::CallbackAwaitable(
    RawAwaitable awaitable)
    : awaitable_(std::move(awaitable))
  {}

  template<typename KeyType>
  template<typename ResultType>
  bool
  AsyncProfileMap<KeyType>::CallbackAwaitable<ResultType>::await_ready()
    const noexcept
  {
    return awaitable_.await_ready();
  }

  template<typename KeyType>
  template<typename ResultType>
  bool
  AsyncProfileMap<KeyType>::CallbackAwaitable<ResultType>::await_suspend(
    std::coroutine_handle<> handle)
  {
    return awaitable_.await_suspend(handle);
  }

  template<typename KeyType>
  template<typename ResultType>
  ResultType
  AsyncProfileMap<KeyType>::CallbackAwaitable<ResultType>::await_resume()
  {
    auto [result, error] = awaitable_.await_resume();
    if(error)
    {
      throw typename ProfileMap<KeyType>::Exception(*error);
    }

    return std::move(result);
  }

  template<typename KeyType>
  AsyncProfileMap<KeyType>::VoidCallbackAwaitable::VoidCallbackAwaitable(
    RawAwaitable awaitable)
    : awaitable_(std::move(awaitable))
  {}

  template<typename KeyType>
  bool
  AsyncProfileMap<KeyType>::VoidCallbackAwaitable::await_ready() const noexcept
  {
    return awaitable_.await_ready();
  }

  template<typename KeyType>
  bool
  AsyncProfileMap<KeyType>::VoidCallbackAwaitable::await_suspend(
    std::coroutine_handle<> handle)
  {
    return awaitable_.await_suspend(handle);
  }

  template<typename KeyType>
  void
  AsyncProfileMap<KeyType>::VoidCallbackAwaitable::await_resume()
  {
    auto error = awaitable_.await_resume();
    if(error)
    {
      throw typename ProfileMap<KeyType>::Exception(*error);
    }
  }

  template<typename KeyType>
  typename AsyncProfileMap<KeyType>::template CallbackAwaitable<bool>
  AsyncProfileMap<KeyType>::co_check_profile(const KeyType& key) const
  {
    return CallbackAwaitable<bool>(
      AdServer::Commons::async_callback<
        bool,
        std::optional<std::string> >(
          [this](const KeyType& key, CheckCallback callback)
          {
            check_profile_async(key, std::move(callback));
          },
          key));
  }

  template<typename KeyType>
  Generics::SmartMemBuf_var
  AsyncProfileMap<KeyType>::get_own_profile_async(
    const KeyType& key,
    GetOwnCallback callback,
    std::optional<Generics::Time> last_access_time)
    /*throw(Exception)*/
  {
    Generics::ConstSmartMemBuf_var direct_result = get_profile_async(
      key,
      [
        callback = std::move(callback)
      ](
        Generics::ConstSmartMemBuf_var profile,
        std::optional<std::string> error) mutable
      {
        Generics::SmartMemBuf_var own_profile;
        if(!error)
        {
          own_profile = ProfileMapDetail::copy_own_profile_(profile);
        }

        if(callback)
        {
          callback(std::move(own_profile), std::move(error));
        }
      },
      last_access_time);

    return ProfileMapDetail::copy_own_profile_(direct_result);
  }

  template<typename KeyType>
  typename AsyncProfileMap<KeyType>::template CallbackAwaitable<
    Generics::ConstSmartMemBuf_var>
  AsyncProfileMap<KeyType>::co_get_profile(
    const KeyType& key,
    std::optional<Generics::Time> last_access_time)
  {
    return CallbackAwaitable<Generics::ConstSmartMemBuf_var>(
      AdServer::Commons::async_callback<
        Generics::ConstSmartMemBuf_var,
        std::optional<std::string> >(
          [this](
            const KeyType& key,
            std::optional<Generics::Time> last_access_time,
            GetCallback callback)
          {
            get_profile_async(
              key,
              std::move(callback),
              last_access_time);
          },
          key,
          last_access_time));
  }

  template<typename KeyType>
  typename AsyncProfileMap<KeyType>::template CallbackAwaitable<
    Generics::SmartMemBuf_var>
  AsyncProfileMap<KeyType>::co_get_own_profile(
    const KeyType& key,
    std::optional<Generics::Time> last_access_time)
  {
    return CallbackAwaitable<Generics::SmartMemBuf_var>(
      AdServer::Commons::async_callback<
        Generics::SmartMemBuf_var,
        std::optional<std::string> >(
          [this](
            const KeyType& key,
            std::optional<Generics::Time> last_access_time,
            typename AsyncProfileMap<KeyType>::GetOwnCallback callback)
          {
            get_own_profile_async(
              key,
              std::move(callback),
              last_access_time);
          },
          key,
          last_access_time));
  }

  template<typename KeyType>
  typename AsyncProfileMap<KeyType>::VoidCallbackAwaitable
  AsyncProfileMap<KeyType>::co_save_profile(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now)
  {
    Generics::ConstSmartMemBuf_var profile_holder(
      ReferenceCounting::add_ref(mem_buf));
    return VoidCallbackAwaitable(
      AdServer::Commons::async_callback<std::optional<std::string> >(
        [this](
          const KeyType& key,
          Generics::ConstSmartMemBuf_var profile_holder,
          const Generics::Time& now,
          SaveCallback callback)
        {
          save_profile_async(
            key,
            profile_holder,
            now,
            std::move(callback));
        },
        key,
        profile_holder,
        now));
  }

  template<typename KeyType>
  typename AsyncProfileMap<KeyType>::template CallbackAwaitable<bool>
  AsyncProfileMap<KeyType>::co_remove_profile(
    const KeyType& key,
    OperationPriority op_priority)
  {
    return CallbackAwaitable<bool>(
      AdServer::Commons::async_callback<
        bool,
        std::optional<std::string> >(
          [this](
            const KeyType& key,
            OperationPriority op_priority,
            RemoveCallback callback)
          {
            remove_profile_async(
              key,
              op_priority,
              std::move(callback));
          },
          key,
          op_priority));
  }

  template<typename KeyType>
  AdServer::Commons::CallbackCoro<>
  AsyncProfileMap<KeyType>::co_clear_expired(
    const Generics::Time& expire_time)
  {
    return AdServer::Commons::async_callback<>(
      [this](
        const Generics::Time& expire_time,
        CompleteCallback complete)
      {
        clear_expired_async(
          expire_time,
          std::move(complete));
      },
      expire_time);
  }

  template<typename KeyType>
  AsyncProfileMapToProfileMap<KeyType>::AsyncProfileMapToProfileMap(
    AsyncProfileMap<KeyType>* async_profile_map)
    noexcept
    : async_profile_map_(ReferenceCounting::add_ref(async_profile_map))
  {}

  template<typename KeyType>
  bool
  AsyncProfileMapToProfileMap<KeyType>::check_profile(
    const KeyType& key) const
  {
    using Result = std::pair<bool, std::optional<std::string>>;
    std::promise<Result> promise;
    std::future<Result> future = promise.get_future();

    async_profile_map_->check_profile_async(
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

  template<typename KeyType>
  Generics::ConstSmartMemBuf_var
  AsyncProfileMapToProfileMap<KeyType>::get_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
  {
    using Result = std::pair<
      Generics::ConstSmartMemBuf_var,
      std::optional<std::string>>;
    std::promise<Result> promise;
    std::future<Result> future = promise.get_future();

    async_profile_map_->get_profile_async(
      key,
      [&promise](
        Generics::ConstSmartMemBuf_var profile,
        std::optional<std::string> error)
      {
        promise.set_value(std::make_pair(std::move(profile), std::move(error)));
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

  template<typename KeyType>
  Generics::SmartMemBuf_var
  AsyncProfileMapToProfileMap<KeyType>::get_own_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
  {
    using Result = std::pair<
      Generics::SmartMemBuf_var,
      std::optional<std::string>>;
    std::promise<Result> promise;
    std::future<Result> future = promise.get_future();

    async_profile_map_->get_own_profile_async(
      key,
      [&promise](
        Generics::SmartMemBuf_var profile,
        std::optional<std::string> error)
      {
        promise.set_value(std::make_pair(
          std::move(profile),
          std::move(error)));
      },
      last_access_time ?
        std::optional<Generics::Time>(*last_access_time) :
        std::nullopt);

    auto result = future.get();
    if(result.second)
    {
      throw Exception(*result.second);
    }

    return std::move(result.first);
  }

  template<typename KeyType>
  void
  AsyncProfileMapToProfileMap<KeyType>::save_profile(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now,
    OperationPriority op_priority)
  {
    static_cast<void>(op_priority);

    std::promise<std::optional<std::string>> promise;
    std::future<std::optional<std::string>> future = promise.get_future();

    async_profile_map_->save_profile_async(
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

  template<typename KeyType>
  bool
  AsyncProfileMapToProfileMap<KeyType>::remove_profile(
    const KeyType& key,
    OperationPriority op_priority)
  {
    using Result = std::pair<bool, std::optional<std::string>>;
    std::promise<Result> promise;
    std::future<Result> future = promise.get_future();

    async_profile_map_->remove_profile_async(
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

  template<typename KeyType>
  void
  AsyncProfileMapToProfileMap<KeyType>::clear_expired(
    const Generics::Time& expire_time)
  {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    async_profile_map_->clear_expired_async(
      expire_time,
      [&promise]()
      {
        promise.set_value();
      });

    future.get();
  }
}
}
