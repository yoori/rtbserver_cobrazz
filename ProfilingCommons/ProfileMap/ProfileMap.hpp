#pragma once

#include <cstdint>
#include <exception>
#include <list>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <Commons/Coro/CallbackCoro.hpp>
#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
namespace ProfilingCommons
{
  enum OperationPriority
  {
    OP_RUNTIME,
    OP_BACKGROUND
  };

  template<typename KeyType>
  struct ProfileMap: public virtual ReferenceCounting::Interface
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(CorruptedRecord, Exception);

    typedef KeyType KeyTypeT;
    typedef std::list<KeyType> KeyList;

    struct Stats
    {
      std::uint64_t logical_read_operations = 0;
      std::uint64_t logical_write_operations = 0;
      std::uint64_t physical_read_operations = 0;
      std::uint64_t physical_write_operations = 0;
    };

    virtual void
    wait_preconditions(const KeyType&, OperationPriority) const
      /*throw(Exception)*/;

    virtual bool
    check_profile(const KeyType& key) const /*throw(Exception)*/ = 0;

    virtual Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0)
      /*throw(Exception)*/ = 0;

    virtual Generics::SmartMemBuf_var
    get_own_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0)
      /*throw(Exception)*/;

    virtual void
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME)
      /*throw(Exception)*/ = 0;

    virtual bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME)
      /*throw(Exception)*/ = 0;

    virtual void
    clear_expired(const Generics::Time& expire_time)
      /*throw(Exception)*/;

    virtual void
    process_keys(
      std::function<void(const KeyType&)> process_key,
      std::function<void(void)> process_complete)
      /*throw(Exception)*/;

    virtual unsigned long size() const noexcept;

    virtual unsigned long area_size() const noexcept;

    virtual Stats stats() const noexcept;
  };

  template<typename KeyType>
  struct AsyncProfileMap: public virtual ReferenceCounting::Interface
  {
    /*
     * Async operation ordering contract:
     * - Reads are check_profile_async/get_profile_async operations.
     * - Writes are save_profile_async/remove_profile_async operations.
     * - If a read is submitted after a write for the same key is accepted by
     *   the same AsyncProfileMap instance, the read must not observe storage
     *   before that earlier write has completed.
     * - Write/write ordering for the same key is the caller's responsibility
     *   and should be provided by a transaction layer when required.
     */
    using CheckCallback = std::function<void(bool, std::optional<std::string>)>;
    using GetCallback = std::function<void(
      const Generics::ConstSmartMemBuf_var&,
      std::optional<std::string> error)>;
    using GetOwnCallback = std::function<void(
      Generics::SmartMemBuf_var,
      std::optional<std::string> error)>;
    using SaveCallback = std::function<void(std::optional<std::string> error)>;
    using RemoveCallback = std::function<void(
      bool,
      std::optional<std::string> error)>;
    using CompleteCallback = std::function<void()>;

    template<typename ResultType>
    class CallbackAwaitable
    {
    public:
      using RawAwaitable = AdServer::Commons::CallbackCoro<
        ResultType,
        std::optional<std::string> >;

      explicit
      CallbackAwaitable(RawAwaitable awaitable);

      bool
      await_ready() const noexcept;

      bool
      await_suspend(std::coroutine_handle<> handle);

      ResultType
      await_resume();

    private:
      RawAwaitable awaitable_;
    };

    class VoidCallbackAwaitable
    {
    public:
      using RawAwaitable = AdServer::Commons::CallbackCoro<
        std::optional<std::string> >;

      explicit
      VoidCallbackAwaitable(RawAwaitable awaitable);

      bool
      await_ready() const noexcept;

      bool
      await_suspend(std::coroutine_handle<> handle);

      void
      await_resume();

    private:
      RawAwaitable awaitable_;
    };

    virtual void
    check_profile_async(
      const KeyType& key,
      CheckCallback callback) const
      /*throw(Exception)*/ = 0;

    virtual Generics::ConstSmartMemBuf_var
    get_profile_async(
      const KeyType& key,
      GetCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt)
      /*throw(Exception)*/ = 0;

    virtual Generics::SmartMemBuf_var
    get_own_profile_async(
      const KeyType& key,
      GetOwnCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt)
      /*throw(Exception)*/;

    virtual void
    save_profile_async(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      SaveCallback callback = SaveCallback())
      /*throw(Exception)*/ = 0;

    virtual void
    remove_profile_async(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME,
      RemoveCallback callback = RemoveCallback())
      /*throw(Exception)*/ = 0;

    virtual void
    clear_expired_async(
      const Generics::Time& expire_time,
      CompleteCallback complete = CompleteCallback())
      /*throw(Exception)*/ = 0;

    CallbackAwaitable<bool>
    co_check_profile(const KeyType& key) const;

    CallbackAwaitable<Generics::ConstSmartMemBuf_var>
    co_get_profile(
      const KeyType& key,
      std::optional<Generics::Time> last_access_time = std::nullopt);

    CallbackAwaitable<Generics::SmartMemBuf_var>
    co_get_own_profile(
      const KeyType& key,
      std::optional<Generics::Time> last_access_time = std::nullopt);

    VoidCallbackAwaitable
    co_save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day());

    CallbackAwaitable<bool>
    co_remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME);

    AdServer::Commons::CallbackCoro<>
    co_clear_expired(const Generics::Time& expire_time);
  };

  template<typename KeyType>
  class AsyncProfileMapToProfileMap:
    public virtual ProfileMap<KeyType>,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, typename ProfileMap<KeyType>::Exception);

    explicit
    AsyncProfileMapToProfileMap(AsyncProfileMap<KeyType>* async_profile_map)
      noexcept;

    bool
    check_profile(const KeyType& key) const override;

    Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0) override;

    Generics::SmartMemBuf_var
    get_own_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0) override;

    void
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME) override;

    bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME) override;

    void
    clear_expired(const Generics::Time& expire_time) override;

  private:
    ReferenceCounting::SmartPtr<
      AsyncProfileMap<KeyType>,
      ReferenceCounting::PolicyNotNull> async_profile_map_;
  };

}
}

#include "ProfileMap.tpp"
