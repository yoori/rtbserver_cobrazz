#pragma once

#include <cstdint>
#include <list>
#include <functional>
#include <future>
#include <optional>
#include <string>
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
      /*throw(Exception)*/
    {}

    virtual bool
    check_profile(const KeyType& key) const /*throw(Exception)*/ = 0;

    virtual Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0)
      /*throw(Exception)*/ = 0;

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

    virtual void clear_expired(const Generics::Time& /*expire_time*/)
      /*throw(Exception)*/
    {
      throw Exception("clear_expired isn't supported");
    }

    virtual void copy_keys(KeyList& /*keys*/) /*throw(Exception)*/
    {
      throw Exception("copy_keys isn't supported");
    };

    virtual unsigned long size() const noexcept = 0;

    virtual unsigned long area_size() const noexcept = 0;

    virtual Stats stats() const noexcept
    {
      return {};
    }
  };

  template<typename KeyType>
  struct AsyncProfileMap: public virtual ReferenceCounting::Interface
  {
    using CheckCallback = std::function<void(bool, std::optional<std::string>)>;
    using GetCallback = std::function<void(
      const Generics::ConstSmartMemBuf_var&,
      std::optional<std::string> error)>;
    using SaveCallback = std::function<void(std::optional<std::string> error)>;

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

    virtual void
    save_profile_async(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      SaveCallback callback = SaveCallback())
      /*throw(Exception)*/ = 0;
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
      noexcept
      : async_profile_map_(ReferenceCounting::add_ref(async_profile_map))
    {}

    bool
    check_profile(const KeyType& key) const override
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

    Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0) override
    {
      using Result = std::pair<
        Generics::ConstSmartMemBuf_var,
        std::optional<std::string>>;
      std::promise<Result> promise;
      std::future<Result> future = promise.get_future();

      async_profile_map_->get_profile_async(
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
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME) override
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

    bool
    remove_profile(
      const KeyType&,
      OperationPriority = OP_RUNTIME) override
    {
      throw Exception("remove_profile isn't supported");
    }

    unsigned long
    size() const noexcept override
    {
      return 0;
    }

    unsigned long
    area_size() const noexcept override
    {
      return 0;
    }

  private:
    ReferenceCounting::SmartPtr<
      AsyncProfileMap<KeyType>,
      ReferenceCounting::PolicyNotNull> async_profile_map_;
  };
}
}
