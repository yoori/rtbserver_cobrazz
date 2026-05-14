#include "MigratingUserBindChunk.hpp"

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    constexpr unsigned long DEFAULT_LOCKS = 1000;
  }

  MigratingUserBindChunk::MigratingUserBindChunk(
    UserBindRocksDBChunk* rocksdb_chunk,
    UserBindChunk* legacy_chunk)
    : rocksdb_chunk_(ReferenceCounting::add_ref(rocksdb_chunk)),
      legacy_chunk_(ReferenceCounting::add_ref(legacy_chunk)),
      user_locks_(DEFAULT_LOCKS)
  {}

  UserBindProcessor::UserInfo
  MigratingUserBindChunk::add_user_id(
    const String::SubString& external_id,
    const Commons::UserId& user_id,
    const Generics::Time& now,
    bool resave_if_exists,
    bool ignore_bad_event)
  {
    UserLockMap::WriteGuard user_lock(
      user_locks_.write_lock(Generics::StringHashAdapter(external_id)));

    UserInfo current = rocksdb_chunk_->get_user_id(
      external_id,
      Commons::UserId(),
      now,
      true,
      Generics::Time::ZERO,
      false);

    if(!current.user_found && legacy_chunk_.in())
    {
      current = legacy_chunk_->get_user_id(
        external_id,
        Commons::UserId(),
        now,
        true,
        Generics::Time::ZERO,
        false);

      if(current.user_found && !current.user_id.is_null())
      {
        rocksdb_chunk_->migrate_bound_user(
          external_id,
          current.user_id,
          now,
          false);
        legacy_chunk_->remove_user_id(external_id);
      }
    }

    return rocksdb_chunk_->add_user_id(
      external_id,
      user_id,
      now,
      resave_if_exists,
      ignore_bad_event);
  }

  UserBindProcessor::UserInfo
  MigratingUserBindChunk::get_user_id(
    const String::SubString& external_id,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    bool silent,
    const Generics::Time& create_time,
    bool for_set_cookie)
  {
    UserLockMap::WriteGuard user_lock(
      user_locks_.write_lock(Generics::StringHashAdapter(external_id)));

    UserInfo result = rocksdb_chunk_->get_user_id(
      external_id,
      Commons::UserId(),
      now,
      true,
      Generics::Time::ZERO,
      false);

    if(result.user_found)
    {
      return rocksdb_chunk_->get_user_id(
        external_id,
        current_user_id,
        now,
        silent,
        create_time,
        for_set_cookie);
    }

    if(legacy_chunk_.in())
    {
      result = legacy_chunk_->get_user_id(
        external_id,
        Commons::UserId(),
        now,
        true,
        Generics::Time::ZERO,
        false);

      if(result.user_found)
      {
        result = legacy_chunk_->get_user_id(
          external_id,
          current_user_id,
          now,
          silent,
          create_time,
          for_set_cookie);

        if(!result.user_id.is_null())
        {
          rocksdb_chunk_->migrate_bound_user(
            external_id,
            result.user_id,
            now,
            for_set_cookie);
          legacy_chunk_->remove_user_id(external_id);
        }
        else
        {
          rocksdb_chunk_->migrate_seen_user(
            external_id,
            result.min_age_reached,
            create_time,
            now);
          legacy_chunk_->remove_user_id(external_id);
        }

        return result;
      }
    }

    return rocksdb_chunk_->get_user_id(
      external_id,
      current_user_id,
      now,
      silent,
      create_time,
      for_set_cookie);
  }

  void
  MigratingUserBindChunk::clear_expired(
    const Generics::Time& unbound_expire_time,
    const Generics::Time& bound_expire_time)
  {
    rocksdb_chunk_->clear_expired(unbound_expire_time, bound_expire_time);
    if(legacy_chunk_.in())
    {
      legacy_chunk_->clear_expired(unbound_expire_time, bound_expire_time);
    }
  }

  void
  MigratingUserBindChunk::dump()
  {
    rocksdb_chunk_->dump();
  }
}
