#include "UserBindRocksDBChunk.hpp"

#include <algorithm>

#include <eh/Exception.hpp>
#include <Generics/MemBuf.hpp>
#include <Stream/MemoryStream.hpp>

#include <Commons/Algs.hpp>

#include "UserBindRocksDBProfile.hpp"

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    constexpr unsigned long DEFAULT_WORKERS = 2;
    constexpr unsigned long DEFAULT_BATCH_SIZE = 128;
    constexpr unsigned long DEFAULT_LOCKS = 1000;
    constexpr std::uint32_t PROFILE_VERSION = 1;
  }

  UserBindRocksDBChunk::UserBindRocksDBChunk(
    const char* user_seen_path,
    const char* user_bind_path,
    const Generics::Time& expire_time,
    const Generics::Time& bound_expire_time,
    const Generics::Time& min_bind_age,
    bool bind_at_min_age,
    unsigned long max_bad_event)
    : min_bind_age_(min_bind_age),
      bind_at_min_age_(bind_at_min_age),
      max_bad_event_(max_bad_event),
      user_seen_map_(new RocksDBMap(
        String::SubString(user_seen_path),
        expire_time,
        DEFAULT_WORKERS,
        DEFAULT_BATCH_SIZE)),
      user_bind_map_(new RocksDBMap(
        String::SubString(user_bind_path),
        bound_expire_time,
        DEFAULT_WORKERS,
        DEFAULT_BATCH_SIZE)),
      user_locks_(DEFAULT_LOCKS)
  {
    user_seen_map_->activate_object();
    user_bind_map_->activate_object();
  }

  UserBindRocksDBChunk::~UserBindRocksDBChunk() noexcept
  {
    if(user_seen_map_)
    {
      user_seen_map_->deactivate_object();
    }

    if(user_bind_map_)
    {
      user_bind_map_->deactivate_object();
    }

    if(user_seen_map_)
    {
      user_seen_map_->wait_object();
    }

    if(user_bind_map_)
    {
      user_bind_map_->wait_object();
    }
  }

  AdServer::Commons::SyncCoro<UserBindProcessor::UserInfo>
  UserBindRocksDBChunk::co_add_user_id(
    const String::SubString& external_id,
    const Commons::UserId& user_id,
    const Generics::Time& now,
    bool resave_if_exists,
    bool ignore_bad_event)
  {
    auto user_lock = co_await user_locks_.scoped_lock_async(
      Generics::StringHashAdapter(external_id));

    Record record;
    const bool found = co_await co_load_record_(record, external_id);
    const bool found_bound = found && record.type == Record::RT_BOUND;

    if(record.type != Record::RT_BOUND)
    {
      record = Record();
      record.type = Record::RT_BOUND;
      record.update_time = now;
    }

    UserInfo result = adapt_bound_record_(
      record,
      false,
      found_bound,
      false);

    if(resave_if_exists || !result.user_found)
    {
      save_user_id_(record, result, user_id, ignore_bad_event, now);
      co_await co_save_record_(external_id, record, now);
    }

    co_return result;
  }

  AdServer::Commons::SyncCoro<UserBindProcessor::UserInfo>
  UserBindRocksDBChunk::co_get_user_id(
    const String::SubString& external_id,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    bool silent,
    const Generics::Time& create_time,
    bool for_set_cookie)
  {
    auto user_lock = co_await user_locks_.scoped_lock_async(
      Generics::StringHashAdapter(external_id));

    Record record;
    const bool found = co_await co_load_record_(record, external_id);

    if(found && record.type == Record::RT_BOUND)
    {
      bool invalid_operation = false;
      if(for_set_cookie)
      {
        if(record.flags & BF_SETCOOKIE_)
        {
          if(record.user_id != current_user_id)
          {
            rotate_bad_event_count_(record, now);
            ++record.bad_event_count;
          }
        }
        else
        {
          record.flags |= BF_SETCOOKIE_;
        }

        invalid_operation =
          (record.flags & BF_SETCOOKIE_) &&
          record.bad_event_count > max_bad_event_;

        co_await co_save_record_(external_id, record, now);
      }

      co_return adapt_bound_record_(record, false, true, invalid_operation);
    }

    if(silent)
    {
      UserInfo result;
      result.user_found = found;
      if(found && record.type == Record::RT_SEEN)
      {
        result = adapt_seen_record_(record, false, true, now);
      }
      co_return result;
    }

    bool created = false;
    if(!found || record.type != Record::RT_SEEN)
    {
      record = Record();
      record.type = Record::RT_SEEN;
      record.first_seen_time =
        create_time == Generics::Time::ZERO ? now : create_time;
      created = true;
    }
    else if(create_time != Generics::Time::ZERO &&
      create_time < record.first_seen_time)
    {
      record.first_seen_time = create_time;
    }

    if(bind_at_min_age_ && record.first_seen_time + min_bind_age_ < now)
    {
      Record bound_record;
      bound_record.type = Record::RT_BOUND;
      bound_record.update_time = now;
      bound_record.user_id = Commons::UserId::create_random_based();
      if(for_set_cookie)
      {
        bound_record.flags |= BF_SETCOOKIE_;
      }

      co_await co_save_record_(external_id, bound_record, now);
      co_return adapt_bound_record_(bound_record, true, true, false);
    }

    co_await co_save_record_(external_id, record, now);
    co_return adapt_seen_record_(record, created, true, now);
  }

  void
  UserBindRocksDBChunk::clear_expired(
    const Generics::Time&,
    const Generics::Time&)
  {}

  void
  UserBindRocksDBChunk::dump()
  {}

  AdServer::Commons::SyncCoro<bool>
  UserBindRocksDBChunk::co_migrate_seen_user(
    const String::SubString& external_id,
    bool min_age_reached,
    const Generics::Time& create_time,
    const Generics::Time& now)
  {
    auto user_lock = co_await user_locks_.scoped_lock_async(
      Generics::StringHashAdapter(external_id));

    Record record;
    record.type = Record::RT_SEEN;
    record.update_time = now;

    if(min_age_reached)
    {
      record.first_seen_time = now - min_bind_age_ - Generics::Time::ONE_SECOND;
    }
    else
    {
      record.first_seen_time =
        create_time == Generics::Time::ZERO ? now : create_time;
    }

    co_await co_save_record_(external_id, record, now);
    co_return true;
  }

  AdServer::Commons::SyncCoro<bool>
  UserBindRocksDBChunk::co_migrate_bound_user(
    const String::SubString& external_id,
    const Commons::UserId& user_id,
    const Generics::Time& now,
    bool for_set_cookie)
  {
    auto user_lock = co_await user_locks_.scoped_lock_async(
      Generics::StringHashAdapter(external_id));

    Record record;
    record.type = Record::RT_BOUND;
    record.update_time = now;
    record.user_id = user_id;
    if(for_set_cookie)
    {
      record.flags |= BF_SETCOOKIE_;
    }

    co_await co_save_record_(external_id, record, now);
    co_return true;
  }

  Generics::ConstSmartMemBuf_var
  UserBindRocksDBChunk::make_profile_(const std::string& value) const
  {
    return Generics::ConstSmartMemBuf_var(
      new Generics::ConstSmartMemBuf(value.data(), value.size()));
  }

  std::string
  UserBindRocksDBChunk::serialize_(const Record& record) const
  {
    UserBindRocksDBRecordWriter writer;
    writer.version() = PROFILE_VERSION;
    writer.type() = record.type;
    writer.first_seen_time() = record.first_seen_time.tv_sec;
    writer.update_time() = record.update_time.tv_sec;
    writer.user_id() =
      record.type == Record::RT_BOUND ? record.user_id.to_string() : "";
    writer.flags() = record.flags;
    writer.bad_event_count() = record.bad_event_count;
    writer.last_bad_event_day() = record.last_bad_event_day;

    std::string value(writer.size(), '\0');
    writer.save(value.data(), value.size());
    return value;
  }

  bool
  UserBindRocksDBChunk::deserialize_(
    Record& record,
    const Generics::ConstSmartMemBuf* profile) const
  {
    if(!profile)
    {
      return false;
    }

    try
    {
      const auto& buffer = profile->membuf();
      UserBindRocksDBRecordReader reader(buffer.data(), buffer.size());
      if(reader.version() != PROFILE_VERSION)
      {
        return false;
      }

      if(reader.type() != Record::RT_SEEN &&
        reader.type() != Record::RT_BOUND)
      {
        return false;
      }

      Record loaded_record;
      loaded_record.type = static_cast<Record::Type>(reader.type());
      loaded_record.first_seen_time = Generics::Time(reader.first_seen_time());
      loaded_record.update_time = Generics::Time(reader.update_time());

      if(loaded_record.type == Record::RT_BOUND)
      {
        const std::string user_id = reader.user_id();
        if(user_id.empty())
        {
          return false;
        }

        loaded_record.user_id =
          Commons::UserId(user_id, user_id.size() == 24);
        loaded_record.flags = static_cast<unsigned char>(reader.flags());
        loaded_record.bad_event_count = static_cast<std::uint8_t>(
          std::min<std::uint32_t>(reader.bad_event_count(), 255));
        loaded_record.last_bad_event_day =
          static_cast<std::uint16_t>(reader.last_bad_event_day());
      }

      record = loaded_record;
      return true;
    }
    catch(const eh::Exception&)
    {
      return false;
    }
  }

  AdServer::Commons::SyncCoro<bool>
  UserBindRocksDBChunk::co_load_record_(
    Record& record,
    const String::SubString& external_id)
  {
    const std::string key = external_id.str();

    const auto bound_profile = co_await user_bind_map_->co_get_profile(key);
    if(deserialize_(record, bound_profile.in()))
    {
      co_return true;
    }

    const auto seen_profile = co_await user_seen_map_->co_get_profile(key);
    co_return deserialize_(record, seen_profile.in());
  }

  AdServer::Commons::SyncCoro<bool>
  UserBindRocksDBChunk::co_save_record_(
    const String::SubString& external_id,
    const Record& record,
    const Generics::Time& now)
  {
    const std::string value = serialize_(record);
    const auto profile = make_profile_(value);
    const std::string key = external_id.str();

    if(record.type == Record::RT_BOUND)
    {
      user_bind_map_->save_profile_async(key, profile.in(), now);
      co_await user_seen_map_->co_remove_profile(key);
    }
    else if(record.type == Record::RT_SEEN)
    {
      user_seen_map_->save_profile_async(key, profile.in(), now);
    }

    co_return true;
  }

  UserBindProcessor::UserInfo
  UserBindRocksDBChunk::adapt_bound_record_(
    const Record& record,
    bool user_id_generated,
    bool user_found,
    bool invalid_operation) const
  {
    UserInfo result;
    result.user_id = record.user_id;
    result.min_age_reached = true;
    result.user_id_generated = user_id_generated;
    result.invalid_operation = invalid_operation;
    result.user_found = user_found;
    return result;
  }

  UserBindProcessor::UserInfo
  UserBindRocksDBChunk::adapt_seen_record_(
    const Record& record,
    bool created,
    bool user_found,
    const Generics::Time& now) const
  {
    UserInfo result;
    result.min_age_reached = record.first_seen_time + min_bind_age_ < now;
    result.created = created;
    result.user_found = user_found;
    return result;
  }

  void
  UserBindRocksDBChunk::save_user_id_(
    Record& record,
    UserInfo& result,
    const Commons::UserId& user_id,
    bool ignore_bad_event,
    const Generics::Time& now) const
  {
    if(record.flags & BF_SETCOOKIE_)
    {
      if(user_id != record.user_id)
      {
        rotate_bad_event_count_(record, now);

        if(record.bad_event_count < max_bad_event_)
        {
          record.user_id = user_id;
        }
        else
        {
          result.invalid_operation = true;
        }
        ++record.bad_event_count;
      }
    }
    else
    {
      record.flags |= BF_SETCOOKIE_;
      record.user_id = user_id;
    }

    if(ignore_bad_event)
    {
      record.user_id = user_id;
    }
  }

  void
  UserBindRocksDBChunk::rotate_bad_event_count_(
    Record& record,
    const Generics::Time& now) const noexcept
  {
    const unsigned long cur_day =
      Algs::round_to_day(now).tv_sec / Generics::Time::ONE_DAY.tv_sec;

    if(cur_day != record.last_bad_event_day)
    {
      record.last_bad_event_day = cur_day;
      record.bad_event_count = 0;
    }
  }

}
