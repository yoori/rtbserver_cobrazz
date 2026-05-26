#pragma once

#include <mutex>

#include <Generics/HashTableAdapters.hpp>
#include <Commons/LockMap.hpp>

#include "UserBindChunk.hpp"
#include "UserBindRocksDBChunk.hpp"

namespace AdServer::UserInfoSvcs
{
  class MigratingUserBindChunk final:
    public UserBindProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    MigratingUserBindChunk(
      UserBindRocksDBChunk* rocksdb_chunk,
      UserBindChunk* legacy_chunk);

    AdServer::Commons::Task<UserInfo>
    co_add_user_id(
      const String::SubString& external_id,
      const Commons::UserId& user_id,
      const Generics::Time& now,
      bool resave_if_exists,
      bool ignore_bad_event) override;

    AdServer::Commons::Task<UserInfo>
    co_get_user_id(
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

  protected:
    ~MigratingUserBindChunk() noexcept override = default;

  private:
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

    UserBindRocksDBChunk_var rocksdb_chunk_;
    UserBindChunk_var legacy_chunk_;
    UserLockMap user_locks_;
  };

  typedef ReferenceCounting::SmartPtr<MigratingUserBindChunk>
    MigratingUserBindChunk_var;
}
