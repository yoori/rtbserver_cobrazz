#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include <Generics/HashTableAdapters.hpp>
#include <Generics/Time.hpp>
#include <Commons/LockMap.hpp>
#include <ProfilingCommons/ProfileMap/RocksDBBatchingProfileMap.hpp>

#include "UserBindProcessor.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserBindRocksDBChunk final:
    public UserBindProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    UserBindRocksDBChunk(
      const char* user_seen_path,
      const char* user_bind_path,
      const Generics::Time& expire_time,
      const Generics::Time& bound_expire_time,
      const Generics::Time& min_bind_age,
      bool bind_at_min_age,
      unsigned long max_bad_event);

    UserInfo
    add_user_id(
      const String::SubString& external_id,
      const Commons::UserId& user_id,
      const Generics::Time& now,
      bool resave_if_exists,
      bool ignore_bad_event) override;

    UserInfo
    get_user_id(
      const String::SubString& external_id,
      const Commons::UserId& current_user_id,
      const Generics::Time& now,
      bool silent,
      const Generics::Time& create_time,
      bool for_set_cookie) override;

    void
    clear_expired(
      const Generics::Time& unbound_expire_time,
      const Generics::Time& bound_expire_time) override;

    void
    dump() override;

    void
    migrate_seen_user(
      const String::SubString& external_id,
      bool min_age_reached,
      const Generics::Time& create_time,
      const Generics::Time& now);

    void
    migrate_bound_user(
      const String::SubString& external_id,
      const Commons::UserId& user_id,
      const Generics::Time& now,
      bool for_set_cookie);

  protected:
    ~UserBindRocksDBChunk() noexcept override;

  private:
    struct Record
    {
      enum Type
      {
        RT_NONE,
        RT_SEEN,
        RT_BOUND
      };

      Type type = RT_NONE;
      Generics::Time first_seen_time;
      Generics::Time update_time;
      Commons::UserId user_id;
      unsigned char flags = 0;
      std::uint8_t bad_event_count = 0;
      std::uint16_t last_bad_event_day = 0;
    };

    Generics::ConstSmartMemBuf_var
    make_profile_(const std::string& value) const;

    std::string
    serialize_(const Record& record) const;

    bool
    deserialize_(Record& record, const Generics::ConstSmartMemBuf* profile)
      const;

    bool
    load_record_(Record& record, const String::SubString& external_id);

    void
    save_record_(
      const String::SubString& external_id,
      const Record& record,
      const Generics::Time& now);

    UserInfo
    adapt_bound_record_(
      const Record& record,
      bool user_id_generated,
      bool user_found,
      bool invalid_operation) const;

    UserInfo
    adapt_seen_record_(
      const Record& record,
      bool created,
      bool user_found,
      const Generics::Time& now) const;

    void
    save_user_id_(
      Record& record,
      UserInfo& result,
      const Commons::UserId& user_id,
      bool ignore_bad_event,
      const Generics::Time& now) const;

    void
    rotate_bad_event_count_(
      Record& record,
      const Generics::Time& now) const noexcept;

  private:
    static constexpr unsigned char BF_SETCOOKIE_ = 1;

    struct UserLockPolicy
    {
      typedef std::mutex Mutex;
      typedef std::lock_guard<Mutex> ReadGuard;
      typedef std::lock_guard<Mutex> WriteGuard;
    };

    typedef AdServer::Commons::NoAllocLockMap<
      Generics::StringHashAdapter,
      UserLockPolicy>
      UserLockMap;

    const Generics::Time min_bind_age_;
    const bool bind_at_min_age_;
    const unsigned long max_bad_event_;

    typedef AdServer::ProfilingCommons::RocksDBBatchingProfileMap<std::string>
      RocksDBMap;

    std::unique_ptr<RocksDBMap> user_seen_map_;
    std::unique_ptr<RocksDBMap> user_bind_map_;
    UserLockMap user_locks_;
  };

  typedef ReferenceCounting::SmartPtr<UserBindRocksDBChunk>
    UserBindRocksDBChunk_var;
}
