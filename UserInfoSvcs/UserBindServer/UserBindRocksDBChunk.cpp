#include "UserBindRocksDBChunk.hpp"

#include <algorithm>
#include <iostream>

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
    constexpr std::uint32_t BOUND_PROFILE_TYPE = 2;
  }

  UserBindRocksDBChunk::UserBindRocksDBChunk(
    const char* user_seen_path,
    const char* user_bind_path,
    const Generics::Time& expire_time,
    const Generics::Time& bound_expire_time,
    std::optional<Generics::Time> bind_min_age,
    unsigned long max_bad_event)
    : bind_min_age_(bind_min_age),
      max_bad_event_(max_bad_event),
      user_seen_map_(new RocksDBMap(
        String::SubString(user_seen_path),
        expire_time,
        DEFAULT_WORKERS,
        DEFAULT_BATCH_SIZE,
        Generics::Time::ZERO,
        true)),
      user_bind_map_(new RocksDBMap(
        String::SubString(user_bind_path),
        bound_expire_time,
        DEFAULT_WORKERS,
        DEFAULT_BATCH_SIZE,
        Generics::Time::ZERO,
        true)),
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
    bool ignore_bad_event,
    bool set_cookie_flag)
  {
    auto user_lock = co_await user_locks_.scoped_lock_async(
      Generics::StringHashAdapter(external_id));

    const std::optional<BoundRecord> loaded_record = co_await co_load_bound_record_(external_id);
    const bool found_bound = loaded_record.has_value();
    BoundRecord record = found_bound ? *loaded_record : BoundRecord();

    if (!found_bound)
    {
      record.update_time = now;
    }

    UserInfo result = adapt_bound_record_(
      record,
      false,
      found_bound,
      false);

    if (resave_if_exists || !result.user_found)
    {
      save_user_id_(
        record,
        result,
        user_id,
        ignore_bad_event,
        set_cookie_flag,
        now);
      co_await co_save_bound_record_(external_id, record, now);
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

    if (std::optional<BoundRecord> bound_record = co_await co_load_bound_record_(external_id))
    {
      bool invalid_operation = false;
      if (for_set_cookie)
      {
        bool changed = false;

        if (bound_record->flags & BF_SETCOOKIE_)
        {
          if (bound_record->user_id != current_user_id)
          {
            rotate_bad_event_count_(*bound_record, now);
            ++bound_record->bad_event_count;
            changed = true;
          }
        }
        else
        {
          bound_record->flags |= BF_SETCOOKIE_;
          changed = true;
        }

        invalid_operation = (bound_record->flags & BF_SETCOOKIE_) &&
          bound_record->bad_event_count > max_bad_event_;

        if (changed)
        {
          co_await co_save_bound_record_(external_id, *bound_record, now);
        }
      }

      co_return adapt_bound_record_(*bound_record, false, true, invalid_operation);
    }

    // min age checking with seen records usage
    std::optional<SeenRecord> seen_record;
    if (!(bind_min_age_ && *bind_min_age_ == Generics::Time::ZERO))
    {
      seen_record = co_await co_load_seen_record_(external_id);
    }

    if (silent)
    {
      UserInfo result;
      result.user_found = seen_record.has_value();
      if (seen_record)
      {
        result = adapt_seen_record_(*seen_record, false, true, now);
      }
      co_return result;
    }

    if (bind_min_age_ && *bind_min_age_ == Generics::Time::ZERO)
    {
      if (!seen_record.has_value())
      {
        seen_record = SeenRecord();
      }
      seen_record->first_seen_time = create_time == Generics::Time::ZERO ? now : create_time;

      co_return adapt_seen_record_(*seen_record, !seen_record.has_value(), true, now);
    }

    bool created = false;
    if (!seen_record.has_value())
    {
      seen_record = SeenRecord();
      seen_record->first_seen_time = create_time == Generics::Time::ZERO ? now : create_time;
      created = true;
    }
    else if (create_time != Generics::Time::ZERO && create_time < seen_record->first_seen_time)
    {
      seen_record->first_seen_time = create_time;
    }

    co_await co_save_seen_record_(external_id, *seen_record, now);
    co_return adapt_seen_record_(*seen_record, created, true, now);
  }

  void
  UserBindRocksDBChunk::clear_expired(const Generics::Time&, const Generics::Time&)
  {}

  void
  UserBindRocksDBChunk::dump()
  {}

  Generics::ConstSmartMemBuf_var
  UserBindRocksDBChunk::make_profile_(const std::string& value) const
  {
    return Generics::ConstSmartMemBuf_var(
      new Generics::ConstSmartMemBuf(value.data(), value.size()));
  }

  std::string
  UserBindRocksDBChunk::serialize_bound_(const BoundRecord& record) const
  {
    UserBindRocksDBRecordWriter writer;
    writer.version() = PROFILE_VERSION;
    writer.type() = BOUND_PROFILE_TYPE;
    writer.first_seen_time() = record.first_seen_time.tv_sec;
    writer.update_time() = record.update_time.tv_sec;
    writer.user_id() = record.user_id.to_string();
    writer.flags() = record.flags;
    writer.bad_event_count() = record.bad_event_count;
    writer.last_bad_event_day() = record.last_bad_event_day;

    std::string value(writer.size(), '\0');
    writer.save(value.data(), value.size());
    return value;
  }

  std::string
  UserBindRocksDBChunk::serialize_seen_(const SeenRecord& record) const
  {
    UserBindSeenRocksDBRecordWriter writer;
    writer.version() = PROFILE_VERSION;
    writer.first_seen_time() = record.first_seen_time.tv_sec;

    std::string value(writer.size(), '\0');
    writer.save(value.data(), value.size());
    return value;
  }

  bool
  UserBindRocksDBChunk::deserialize_bound_(
    BoundRecord& record,
    const Generics::ConstSmartMemBuf* profile) const
  {
    if (!profile)
    {
      return false;
    }

    try
    {
      const auto& buffer = profile->membuf();
      UserBindRocksDBRecordReader reader(buffer.data(), buffer.size());

      if (reader.version() != PROFILE_VERSION || reader.type() != BOUND_PROFILE_TYPE)
      {
        return false;
      }

      if (reader.user_id()[0] == 0)
      {
        return false;
      }

      record.first_seen_time = Generics::Time(reader.first_seen_time());
      record.update_time = Generics::Time(reader.update_time());
      record.user_id = Commons::UserId(reader.user_id(), strlen(reader.user_id()) == 24);
      record.flags = static_cast<unsigned char>(reader.flags());
      record.bad_event_count = static_cast<std::uint8_t>(
        std::min<std::uint32_t>(reader.bad_event_count(), 255));
      record.last_bad_event_day = static_cast<std::uint16_t>(reader.last_bad_event_day());

      return true;
    }
    catch(const eh::Exception&)
    {
      return false;
    }
  }

  bool
  UserBindRocksDBChunk::deserialize_seen_(
    SeenRecord& record,
    const Generics::ConstSmartMemBuf* profile) const
  {
    if (!profile)
    {
      return false;
    }

    try
    {
      const auto& buffer = profile->membuf();
      UserBindSeenRocksDBRecordReader reader(buffer.data(), buffer.size());
      if (reader.version() != PROFILE_VERSION)
      {
        return false;
      }

      record.first_seen_time = Generics::Time(reader.first_seen_time());

      return true;
    }
    catch(const eh::Exception&)
    {
      return false;
    }
  }

  AdServer::Commons::SyncCoro<
    std::optional<UserBindRocksDBChunk::BoundRecord>>
  UserBindRocksDBChunk::co_load_bound_record_(
    const String::SubString& external_id)
  {
    const std::string key = external_id.str();

    const auto bound_profile = co_await user_bind_map_->co_get_profile(key);
    std::cout
      << "UserBindRocksDBChunk::co_load_bound_record_():"
      << " key='" << key << "'"
      << " bound_profile="
      << (bound_profile.in() ? bound_profile->membuf().size() : 0)
      << std::endl;

    BoundRecord record;
    if(bound_profile.in() && deserialize_bound_(record, bound_profile.in()))
    {
      std::cout
        << "UserBindRocksDBChunk::co_load_bound_record_():"
        << " key='" << key << "'"
        << " result=bound"
        << " flags=" << static_cast<unsigned int>(record.flags)
        << " bad_event_count="
        << static_cast<unsigned int>(record.bad_event_count)
        << std::endl;
      co_return record;
    }

    co_return std::nullopt;
  }

  AdServer::Commons::SyncCoro<
    std::optional<UserBindRocksDBChunk::SeenRecord>>
  UserBindRocksDBChunk::co_load_seen_record_(
    const String::SubString& external_id)
  {
    const std::string key = external_id.str();

    if(bind_min_age_ && *bind_min_age_ == Generics::Time::ZERO)
    {
      co_return std::nullopt;
    }

    const auto seen_profile = co_await user_seen_map_->co_get_profile(key);
    std::cout
      << "UserBindRocksDBChunk::co_load_seen_record_():"
      << " key='" << key << "'"
      << " seen_profile="
      << (seen_profile.in() ? seen_profile->membuf().size() : 0)
      << std::endl;

    SeenRecord record;
    const bool loaded = deserialize_seen_(record, seen_profile.in());
    std::cout
      << "UserBindRocksDBChunk::co_load_seen_record_():"
      << " key='" << key << "'"
      << " result=" << (loaded ? "seen" : "none")
      << " first_seen=" << record.first_seen_time.tv_sec
      << "." << record.first_seen_time.tv_usec
      << std::endl;
    if(loaded)
    {
      co_return record;
    }

    co_return std::nullopt;
  }

  AdServer::Commons::SyncCoro<bool>
  UserBindRocksDBChunk::co_save_bound_record_(
    const String::SubString& external_id,
    const BoundRecord& record,
    const Generics::Time& now)
  {
    const std::string key = external_id.str();
    const std::string value = serialize_bound_(record);
    const auto profile = make_profile_(value);

    std::cout
      << "UserBindRocksDBChunk::co_save_bound_record_():"
      << " key='" << key << "'"
      << " value_size=" << value.size()
      << " now=" << now.tv_sec << "." << now.tv_usec
      << " first_seen=" << record.first_seen_time.tv_sec
      << "." << record.first_seen_time.tv_usec
      << " update=" << record.update_time.tv_sec
      << "." << record.update_time.tv_usec
      << " flags=" << static_cast<unsigned int>(record.flags)
      << " bad_event_count="
      << static_cast<unsigned int>(record.bad_event_count)
      << std::endl;

    user_bind_map_->save_profile_async(key, profile.in(), now);
    co_await user_seen_map_->co_remove_profile(key);

    co_return true;
  }

  AdServer::Commons::SyncCoro<bool>
  UserBindRocksDBChunk::co_save_seen_record_(
    const String::SubString& external_id,
    const SeenRecord& record,
    const Generics::Time& now)
  {
    const std::string key = external_id.str();
    const std::string value = serialize_seen_(record);
    const auto profile = make_profile_(value);

    std::cout
      << "UserBindRocksDBChunk::co_save_seen_record_():"
      << " key='" << key << "'"
      << " value_size=" << value.size()
      << " now=" << now.tv_sec << "." << now.tv_usec
      << " first_seen=" << record.first_seen_time.tv_sec
      << "." << record.first_seen_time.tv_usec
      << std::endl;

    user_seen_map_->save_profile_async(key, profile.in(), now);

    co_return true;
  }

  UserBindProcessor::UserInfo
  UserBindRocksDBChunk::adapt_bound_record_(
    const BoundRecord& record,
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
    const SeenRecord& record,
    bool created,
    bool user_found,
    const Generics::Time& now) const
  {
    UserInfo result;
    result.min_age_reached = (bind_min_age_ && record.first_seen_time + *bind_min_age_ <= now);
    result.created = created;
    result.user_found = user_found;
    return result;
  }

  void
  UserBindRocksDBChunk::save_user_id_(
    BoundRecord& record,
    UserInfo& result,
    const Commons::UserId& user_id,
    bool ignore_bad_event,
    bool set_cookie_flag,
    const Generics::Time& now) const
  {
    if (record.flags & BF_SETCOOKIE_)
    {
      if (user_id != record.user_id)
      {
        rotate_bad_event_count_(record, now);

        if (record.bad_event_count < max_bad_event_)
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
      if (set_cookie_flag)
      {
        record.flags |= BF_SETCOOKIE_;
      }
      record.user_id = user_id;
    }

    if (ignore_bad_event)
    {
      record.user_id = user_id;
    }
  }

  void
  UserBindRocksDBChunk::rotate_bad_event_count_(
    BoundRecord& record,
    const Generics::Time& now) const noexcept
  {
    const unsigned long cur_day =
      Algs::round_to_day(now).tv_sec / Generics::Time::ONE_DAY.tv_sec;

    if (cur_day != record.last_bad_event_day)
    {
      record.last_bad_event_day = cur_day;
      record.bad_event_count = 0;
    }
  }

}
