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
    constexpr std::uint32_t BOUND_PROFILE_TYPE = 2;
  }

  UserBindRocksDBChunk::UserBindRocksDBChunk(
    const char* user_bind_path,
    const Generics::Time& bound_expire_time,
    std::optional<Generics::Time> bind_min_age,
    unsigned long max_bad_event)
    : bind_min_age_(bind_min_age),
      max_bad_event_(max_bad_event),
      user_bind_map_(new RocksDBMap(
        String::SubString(user_bind_path),
        bound_expire_time,
        DEFAULT_WORKERS,
        DEFAULT_BATCH_SIZE,
        Generics::Time::ZERO,
        true)),
      user_locks_(DEFAULT_LOCKS)
  {
    user_bind_map_->activate_object();
  }

  UserBindRocksDBChunk::~UserBindRocksDBChunk() noexcept
  {
    if(user_bind_map_)
    {
      user_bind_map_->deactivate_object();
    }

    if(user_bind_map_)
    {
      user_bind_map_->wait_object();
    }
  }

  AdServer::Commons::StartableAwaitable<UserBindProcessor::UserInfo>
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
    co_return co_await co_add_user_id_i_(
      external_id,
      user_id,
      now,
      resave_if_exists,
      ignore_bad_event,
      set_cookie_flag,
      loaded_record ? &*loaded_record : nullptr);
  }

  AdServer::Commons::StartableAwaitable<UserBindProcessor::UserInfo>
  UserBindRocksDBChunk::co_get_user_id(
    const String::SubString& external_id,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    bool silent,
    const Generics::Time& create_time,
    bool for_set_cookie,
    bool generate_user_id)
  {
    auto user_lock = co_await user_locks_.scoped_lock_async(
      Generics::StringHashAdapter(external_id));

    if (std::optional<BoundRecord> bound_record = co_await co_load_bound_record_(external_id))
    {
      if (bound_record->user_id.is_null())
      {
        if (silent)
        {
          co_return adapt_unbound_record_(*bound_record, false, now);
        }

        bool changed = false;
        const Generics::Time use_create_time =
          create_time == Generics::Time::ZERO ? now : create_time;

        if (bound_record->first_seen_time == Generics::Time::ZERO)
        {
          bound_record->first_seen_time = use_create_time;
          changed = true;
        }
        else if (
          create_time != Generics::Time::ZERO &&
          create_time < bound_record->first_seen_time)
        {
          bound_record->first_seen_time = create_time;
          changed = true;
        }

        UserInfo result = adapt_unbound_record_(*bound_record, false, now);
        if (
          generate_user_id &&
          result.min_age_reached)
        {
          const Commons::UserId new_user_id =
            Commons::UserId::create_random_based();
          result = co_await co_add_user_id_i_(
            external_id,
            new_user_id,
            now,
            false,
            false,
            for_set_cookie,
            &*bound_record);
          if (!result.user_found && !result.invalid_operation)
          {
            result.user_id = new_user_id;
            result.user_id_generated = true;
            result.created = false;
            result.min_age_reached = true;
          }
        }
        else if (changed)
        {
          co_await co_save_bound_record_(external_id, *bound_record, now);
        }

        co_return result;
      }

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

    if (!bind_min_age_ || *bind_min_age_ == Generics::Time::ZERO)
    {
      UserInfo result;
      result.min_age_reached = true;
      result.created = false;
      result.user_found = false;
      if (generate_user_id && !silent)
      {
        const Commons::UserId new_user_id =
          Commons::UserId::create_random_based();
        result = co_await co_add_user_id_i_(
          external_id,
          new_user_id,
          now,
          false,
          false,
          for_set_cookie,
          nullptr);
        if (!result.user_found && !result.invalid_operation)
        {
          result.user_id = new_user_id;
          result.user_id_generated = true;
          result.created = false;
          result.min_age_reached = true;
        }
      }
      co_return result;
    }

    if (silent) // read only mode
    {
      UserInfo result;
      result.user_found = false;
      co_return result;
    }

    const Generics::Time use_create_time = create_time == Generics::Time::ZERO ? now : create_time;
    BoundRecord bound_record;
    bound_record.first_seen_time = use_create_time;
    bound_record.update_time = now;
    UserInfo result = adapt_unbound_record_(bound_record, true, now);
    if (generate_user_id && result.min_age_reached)
    {
      const Commons::UserId new_user_id =
        Commons::UserId::create_random_based();
      result = co_await co_add_user_id_i_(
        external_id,
        new_user_id,
        now,
        false,
        false,
        for_set_cookie,
        &bound_record);
      if (!result.user_found && !result.invalid_operation)
      {
        result.user_id = new_user_id;
        result.user_id_generated = true;
        result.created = true;
        result.min_age_reached = true;
      }
      co_return result;
    }

    co_await co_save_bound_record_(external_id, bound_record, now);

    co_return result;
  }

  AdServer::Commons::StartableAwaitable<UserBindProcessor::UserInfo>
  UserBindRocksDBChunk::co_add_user_id_i_(
    const String::SubString& external_id,
    const Commons::UserId& user_id,
    const Generics::Time& now,
    bool resave_if_exists,
    bool ignore_bad_event,
    bool set_cookie_flag,
    const BoundRecord* loaded_record)
  {
    const bool found_record = loaded_record != nullptr;
    BoundRecord record = found_record ? *loaded_record : BoundRecord();

    if (!found_record)
    {
      record.first_seen_time = now;
      record.update_time = now;
    }

    const bool found_bound_user = found_record && !record.user_id.is_null();
    UserInfo result = adapt_bound_record_(
      record,
      false,
      found_bound_user,
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

      record.first_seen_time = Generics::Time(reader.first_seen_time());
      record.update_time = Generics::Time(reader.update_time());
      if (reader.user_id()[0] != 0)
      {
        record.user_id = Commons::UserId(reader.user_id(), strlen(reader.user_id()) == 24);
      }
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

  AdServer::Commons::StartableAwaitable<
    std::optional<UserBindRocksDBChunk::BoundRecord>>
  UserBindRocksDBChunk::co_load_bound_record_(
    const String::SubString& external_id)
  {
    const std::string key = external_id.str();

    const auto bound_profile = co_await user_bind_map_->co_get_profile(key);

    BoundRecord record;
    if(bound_profile.in() && deserialize_bound_(record, bound_profile.in()))
    {
      co_return record;
    }

    co_return std::nullopt;
  }

  AdServer::Commons::StartableAwaitable<bool>
  UserBindRocksDBChunk::co_save_bound_record_(
    const String::SubString& external_id,
    const BoundRecord& record,
    const Generics::Time& now)
  {
    const std::string key = external_id.str();
    const std::string value = serialize_bound_(record);
    const auto profile = make_profile_(value);

    user_bind_map_->save_profile_async(key, profile.in(), now);

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
  UserBindRocksDBChunk::adapt_unbound_record_(
    const BoundRecord& record,
    bool created,
    const Generics::Time& now) const
  {
    UserInfo result;
    result.min_age_reached = (bind_min_age_ && record.first_seen_time + *bind_min_age_ <= now);
    result.created = created;
    result.user_found = true;
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

    if (record.first_seen_time == Generics::Time::ZERO)
    {
      record.first_seen_time = now;
    }
    record.update_time = now;
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
