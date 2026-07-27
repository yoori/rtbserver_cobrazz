#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include <Generics/HashTableAdapters.hpp>
#include <Generics/Time.hpp>
#include <Commons/AsyncLockMap.hpp>
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
      const char* user_bind_path,
      const Generics::Time& bound_expire_time,
      std::optional<Generics::Time> bind_min_age,
      unsigned long max_bad_event);

    AdServer::Commons::StartableAwaitable<UserInfo>
    co_add_user_id(
      const String::SubString& external_id,
      const Commons::UserId& user_id,
      const Generics::Time& now,
      bool resave_if_exists,
      bool ignore_bad_event,
      bool set_cookie_flag) override;

    AdServer::Commons::StartableAwaitable<UserInfo>
    co_get_user_id(
      const String::SubString& external_id,
      const Commons::UserId& current_user_id,
      const Generics::Time& now,
      bool silent,
      const Generics::Time& create_time,
      bool for_set_cookie,
      bool generate_user_id) override;

    void
    clear_expired(
      const Generics::Time& unbound_expire_time,
      const Generics::Time& bound_expire_time) override;

    void
    dump() override;

  protected:
    ~UserBindRocksDBChunk() noexcept override;

  private:
    struct BoundRecord
    {
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
    serialize_bound_(const BoundRecord& record) const;

    bool
    deserialize_bound_(
      BoundRecord& record,
      const Generics::ConstSmartMemBuf* profile)
      const;

    AdServer::Commons::StartableAwaitable<std::optional<BoundRecord>>
    co_load_bound_record_(
      const String::SubString& external_id);

    AdServer::Commons::StartableAwaitable<bool>
    co_save_bound_record_(
      const String::SubString& external_id,
      const BoundRecord& record,
      const Generics::Time& now);

    AdServer::Commons::StartableAwaitable<UserInfo>
    co_add_user_id_i_(
      const String::SubString& external_id,
      const Commons::UserId& user_id,
      const Generics::Time& now,
      bool resave_if_exists,
      bool ignore_bad_event,
      bool set_cookie_flag,
      const BoundRecord* loaded_record);

    UserInfo
    adapt_bound_record_(
      const BoundRecord& record,
      bool user_id_generated,
      bool user_found,
      bool invalid_operation) const;

    UserInfo
    adapt_unbound_record_(
      const BoundRecord& record,
      bool created,
      const Generics::Time& now) const;

    void
    save_user_id_(
      BoundRecord& record,
      UserInfo& result,
      const Commons::UserId& user_id,
      bool ignore_bad_event,
      bool set_cookie_flag,
      const Generics::Time& now) const;

    void
    rotate_bad_event_count_(
      BoundRecord& record,
      const Generics::Time& now) const noexcept;

  private:
    static constexpr unsigned char BF_SETCOOKIE_ = 1;

    using UserLockMap = AdServer::Commons::AsyncNoAllocLockMap<
      Generics::StringHashAdapter>;

    const std::optional<Generics::Time> bind_min_age_;
    const unsigned long max_bad_event_;

    using RocksDBMap =
      AdServer::ProfilingCommons::RocksDBBatchingProfileMap<std::string>;

    std::unique_ptr<RocksDBMap> user_bind_map_;
    UserLockMap user_locks_;
  };

  using UserBindRocksDBChunk_var =
    ReferenceCounting::SmartPtr<UserBindRocksDBChunk>;
}
