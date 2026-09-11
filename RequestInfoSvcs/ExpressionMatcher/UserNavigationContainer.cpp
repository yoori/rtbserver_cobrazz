#include "UserNavigationContainer.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>

#include <Commons/Coro/SetAwaitable.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserNavigationProfileAdapter.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserNavigationProfile.hpp>

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
    const Generics::Time NAVIGATION_HISTORY_PERIOD = Generics::Time::ONE_DAY * 30;
    constexpr std::uint64_t SECONDS_IN_DAY = 86400;

    struct NavigationLess
    {
      bool operator()(const NavigationWriter& left, std::string_view right) const noexcept
      {
        return std::string_view(left.url()) < right;
      }
    };

    std::size_t
    navigation_count(const UserNavigationProfileWriter::days_Container& days) noexcept
    {
      std::size_t result = 0;
      for (const auto& day : days)
      {
        result += day.navigations().size();
      }
      return result;
    }

    void
    erase_oldest_navigations(
      UserNavigationProfileWriter::days_Container& days,
      std::size_t erase_count)
    {
      auto first_day = days.begin();
      while (first_day != days.end() && erase_count >= first_day->navigations().size())
      {
        erase_count -= first_day->navigations().size();
        ++first_day;
      }
      days.erase(days.begin(), first_day);

      if (erase_count != 0)
      {
        auto& navigations = days.front().navigations();
        navigations.erase(navigations.begin(), navigations.begin() + erase_count);
      }
    }

    std::uint64_t add_saturated(std::uint64_t left, std::uint64_t right) noexcept
    {
      const std::uint64_t max = std::numeric_limits<std::uint64_t>::max();
      return right > max - left ? max : left + right;
    }

    void
    merge_navigations(
      NavigationDayWriter::navigations_Container& target,
      NavigationDayWriter::navigations_Container& source)
    {
      NavigationDayWriter::navigations_Container merged;
      merged.reserve(target.size() + source.size());

      auto target_it = target.begin();
      auto source_it = source.begin();
      while (target_it != target.end() && source_it != source.end())
      {
        if (target_it->url() < source_it->url())
        {
          merged.push_back(std::move(*target_it));
          ++target_it;
        }
        else if (source_it->url() < target_it->url())
        {
          merged.push_back(std::move(*source_it));
          ++source_it;
        }
        else
        {
          target_it->count() = add_saturated(target_it->count(), source_it->count());
          merged.push_back(std::move(*target_it));
          ++target_it;
          ++source_it;
        }
      }

      merged.insert(
        merged.end(),
        std::make_move_iterator(target_it),
        std::make_move_iterator(target.end()));
      merged.insert(
        merged.end(),
        std::make_move_iterator(source_it),
        std::make_move_iterator(source.end()));
      target = std::move(merged);
    }

    void
    merge_days(
      UserNavigationProfileWriter::days_Container& target,
      UserNavigationProfileWriter::days_Container& source)
    {
      if (!source.empty() && (target.empty() || target.back().date() < source.front().date()))
      {
        target.insert(
          target.end(),
          std::make_move_iterator(source.begin()),
          std::make_move_iterator(source.end()));
        return;
      }

      for (auto& source_day : source)
      {
        const auto target_day = std::lower_bound(
          target.begin(),
          target.end(),
          source_day.date(),
          [](const NavigationDayWriter& day, std::uint32_t date) noexcept
          {
            return day.date() < date;
          });

        if (target_day == target.end() || target_day->date() != source_day.date())
        {
          target.insert(target_day, std::move(source_day));
        }
        else
        {
          merge_navigations(target_day->navigations(), source_day.navigations());
        }
      }
    }
  }

  unsigned int UserNavigationContainer::UserNavigationKeyAccessor::size(
    const UserNavigationKey& key) noexcept
  {
    const unsigned int user_id_size = AdServer::ProfilingCommons::UserIdAccessor::size(key.user_id);
    return key.period_id == UserNavigationKey::LEGACY_PERIOD_ID ?
      user_id_size : user_id_size + sizeof(key.period_id);
  }

  void
  UserNavigationContainer::UserNavigationKeyAccessor::load(
    const void* buf,
    unsigned int size,
    UserNavigationKey& key)
  {
    const unsigned int user_id_size = Generics::Uuid::size();
    if (size != user_id_size && size != user_id_size + sizeof(key.period_id))
    {
      Stream::Error ostr;
      ostr << "Unexpected user navigation key size = " << size;
      throw Exception(ostr);
    }

    AdServer::ProfilingCommons::UserIdAccessor::load(buf, user_id_size, key.user_id);
    key.period_id = UserNavigationKey::LEGACY_PERIOD_ID;
    if (size != user_id_size)
    {
      std::memcpy(
        &key.period_id,
        static_cast<const unsigned char*>(buf) + user_id_size,
        sizeof(key.period_id));
    }
  }

  void
  UserNavigationContainer::UserNavigationKeyAccessor::save(
    const UserNavigationKey& key,
    void* buf,
    unsigned int size)
  {
    if (size != UserNavigationKeyAccessor::size(key))
    {
      Stream::Error ostr;
      ostr << "Unexpected user navigation key buffer size = " << size;
      throw Exception(ostr);
    }

    const unsigned int user_id_size = Generics::Uuid::size();
    AdServer::ProfilingCommons::UserIdAccessor::save(key.user_id, buf, user_id_size);
    if (key.period_id != UserNavigationKey::LEGACY_PERIOD_ID)
    {
      std::memcpy(
        static_cast<unsigned char*>(buf) + user_id_size,
        &key.period_id,
        sizeof(key.period_id));
    }
  }

  UserNavigationContainer::UserNavigationContainer(
    Logging::Logger* logger,
    unsigned long common_chunks_number,
    const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
    const char* file_prefix,
    const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits,
    std::size_t user_navigations_limit,
    unsigned long navigation_period_days,
    std::shared_ptr<AdServer::ProfilingCommons::RocksDBProfileMapProcessor>
      rocksdb_processor)
    : logger_(ReferenceCounting::add_ref(logger)),
      expire_time_(user_level_map_traits.expire_time),
      user_navigations_limit_(user_navigations_limit),
      navigation_period_seconds_(navigation_period_days * SECONDS_IN_DAY)
  {
    static const char* FUN = "UserNavigationContainer::UserNavigationContainer()";

    if (navigation_period_seconds_ == 0)
    {
      throw Exception("User navigation period must be positive");
    }

    try
    {
      auto user_map = AdServer::ProfilingCommons::ProfileMapFactory::
        open_rocksdb_chunked_map<
          UserNavigationKey,
          UserNavigationKeyAccessor,
          UserNavigationKeyHash>(
            common_chunks_number,
            chunk_folders,
            file_prefix,
            AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits(
              user_level_map_traits.expire_time),
            UserNavigationKeyHash(),
            0,
            false,
            ".rocksdb",
            2,
            std::move(rocksdb_processor));
      user_map_ = user_map.first;
      add_child_object(user_map.second);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init profiles map. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  unsigned long UserNavigationContainer::profile_size() const noexcept
  {
    return user_map_->size();
  }

  std::uint32_t UserNavigationContainer::navigation_period_id_(std::uint32_t date) const noexcept
  {
    return static_cast<std::uint32_t>(date / navigation_period_seconds_);
  }

  AdServer::Commons::StartableAwaitable<void>
  UserNavigationContainer::co_get_profile_part_(
    UserNavigationKey key, Generics::ConstSmartMemBuf_var& profile)
  {
    profile = co_await user_map_->co_get_profile(key);
  }

  AdServer::Commons::StartableAwaitable<void>
  UserNavigationContainer::co_get_legacy_profile_part_(
    UserNavigationKey key, Generics::ConstSmartMemBuf_var& profile)
  {
    Generics::SmartMemBuf_var mutable_profile = co_await user_map_->co_get_own_profile(key);
    if (mutable_profile.in())
    {
      profile = Generics::transfer_membuf(mutable_profile);
    }
  }

  AdServer::Commons::Awaitable<void>
  UserNavigationContainer::co_migrate_legacy_profile_(const AdServer::Commons::UserId& user_id)
  {
    const UserNavigationKey legacy_key(user_id, UserNavigationKey::LEGACY_PERIOD_ID);
    UserNavigationMap::Transaction_var legacy_transaction =
      co_await user_map_->co_get_transaction(legacy_key);
    Generics::SmartMemBuf_var legacy_profile = co_await legacy_transaction->co_get_own_profile();
    if (!legacy_profile.in())
    {
      co_return;
    }

    const Generics::ConstSmartMemBuf_var legacy_const_profile =
      Generics::transfer_membuf(legacy_profile);
    const Generics::ConstSmartMemBuf_var adapted_profile =
      UserNavigationProfileAdapter()(legacy_const_profile.in());
    UserNavigationProfileWriter legacy_writer;
    legacy_writer.init(adapted_profile->membuf().data(), adapted_profile->membuf().size());

    const Generics::Time now = Generics::Time::get_time_of_day();
    const std::uint32_t oldest_date = static_cast<std::uint32_t>(
      (Algs::round_to_day(now) - NAVIGATION_HISTORY_PERIOD).tv_sec);
    auto& legacy_days = legacy_writer.days();
    const auto first_actual = std::lower_bound(
      legacy_days.begin(),
      legacy_days.end(),
      oldest_date,
      [](const NavigationDayWriter& day, std::uint32_t date) noexcept
      {
        return day.date() < date;
      });
    legacy_days.erase(legacy_days.begin(), first_actual);

    auto day = legacy_days.begin();
    while (day != legacy_days.end())
    {
      const std::uint32_t period_id = navigation_period_id_(day->date());
      UserNavigationProfileWriter part_writer;
      part_writer.version() = CURRENT_USER_NAVIGATION_PROFILE_VERSION;
      auto& part_days = part_writer.days();
      do
      {
        part_days.push_back(std::move(*day));
        ++day;
      }
      while (day != legacy_days.end() && navigation_period_id_(day->date()) == period_id);

      UserNavigationMap::Transaction_var transaction =
        co_await user_map_->co_get_transaction(UserNavigationKey(user_id, period_id));
      Generics::ConstSmartMemBuf_var profile = co_await transaction->co_get_profile();
      UserNavigationProfileWriter profile_writer;
      if (profile.in())
      {
        profile = UserNavigationProfileAdapter()(profile.in());
        profile_writer.init(profile->membuf().data(), profile->membuf().size());
        merge_days(profile_writer.days(), part_days);
      }
      else
      {
        profile_writer = std::move(part_writer);
      }

      auto& days = profile_writer.days();
      const std::size_t navigations_size = navigation_count(days);
      if (navigations_size > user_navigations_limit_)
      {
        erase_oldest_navigations(days, navigations_size - user_navigations_limit_);
      }

      Generics::SmartMemBuf_var result(new Generics::SmartMemBuf(profile_writer.size()));
      profile_writer.save(result->membuf().data(), result->membuf().size());
      co_await transaction->co_save_profile(Generics::transfer_membuf(result), now);
    }

    co_await legacy_transaction->co_remove_profile();
  }

  AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
  UserNavigationContainer::co_get_profile(
    const AdServer::Commons::UserId& user_id,
    std::optional<std::uint32_t> date)
  {
    static const char* FUN = "UserNavigationContainer::co_get_profile()";

    const Generics::Time today = Algs::round_to_day(Generics::Time::get_time_of_day());
    const std::uint32_t oldest_date = static_cast<std::uint32_t>(
      (today - NAVIGATION_HISTORY_PERIOD).tv_sec);
    const std::uint32_t current_date = static_cast<std::uint32_t>(today.tv_sec);
    const std::uint32_t first_period_id = navigation_period_id_(oldest_date);
    const std::uint32_t last_period_id = navigation_period_id_(current_date);

    if (!user_map_->dispose_profile(UserNavigationKey(user_id, first_period_id)))
    {
      co_return Generics::ConstSmartMemBuf_var();
    }

    try
    {
      const std::size_t periods_count = last_period_id - first_period_id + 1;
      std::vector<Generics::ConstSmartMemBuf_var> profiles(periods_count + 1);
      std::vector<AdServer::Commons::StartableAwaitable<void>> operations;
      operations.reserve(profiles.size());

      for (std::size_t i = 0; i < periods_count; ++i)
      {
        operations.emplace_back(co_get_profile_part_(
          UserNavigationKey(user_id, static_cast<std::uint32_t>(first_period_id + i)),
          profiles[i]));
      }
      operations.emplace_back(co_get_legacy_profile_part_(
        UserNavigationKey(user_id, UserNavigationKey::LEGACY_PERIOD_ID),
        profiles.back()));
      co_await AdServer::Commons::SetAwaitable(std::move(operations));

      bool profile_found = false;
      UserNavigationProfileWriter profile_writer;
      profile_writer.version() = CURRENT_USER_NAVIGATION_PROFILE_VERSION;
      for (auto& profile : profiles)
      {
        if (!profile.in())
        {
          continue;
        }

        profile_found = true;
        profile = UserNavigationProfileAdapter()(profile.in());

        UserNavigationProfileWriter part_writer;
        part_writer.init(profile->membuf().data(), profile->membuf().size());
        merge_days(profile_writer.days(), part_writer.days());
      }

      if (!profile_found)
      {
        co_return Generics::ConstSmartMemBuf_var();
      }

      if (profiles.back().in())
      {
        co_await co_migrate_legacy_profile_(user_id);
      }

      auto& days = profile_writer.days();
      const auto first_actual = std::lower_bound(
        days.begin(),
        days.end(),
        oldest_date,
        [](const NavigationDayWriter& day, std::uint32_t actual_date) noexcept
        {
          return day.date() < actual_date;
        });
      days.erase(days.begin(), first_actual);

      const std::size_t navigations_size = navigation_count(days);
      if (navigations_size > user_navigations_limit_)
      {
        erase_oldest_navigations(days, navigations_size - user_navigations_limit_);
      }

      if (date.has_value())
      {
        const auto first_after_date = std::upper_bound(
          days.begin(),
          days.end(),
          *date,
          [](std::uint32_t requested_date, const NavigationDayWriter& day) noexcept
          {
            return requested_date < day.date();
          });
        days.erase(first_after_date, days.end());
      }

      Generics::SmartMemBuf_var result(new Generics::SmartMemBuf(profile_writer.size()));
      profile_writer.save(result->membuf().data(), result->membuf().size());
      co_return Generics::transfer_membuf(result);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get profile. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  AdServer::Commons::StartableAwaitable<void>
  UserNavigationContainer::co_process_request(const RequestInfo& request_info)
  {
    static const char* FUN = "UserNavigationContainer::co_process_request()";

    if (request_info.user_id.is_null() ||
      std::none_of(
        request_info.urls.begin(),
        request_info.urls.end(),
        [](std::string_view url) noexcept
        {
          return !url.empty();
        }))
    {
      co_return;
    }

    try
    {
      co_await co_process_request_trans_(request_info);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception on processing request transaction: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserNavigationContainer::clear_expired()
  {
    const Generics::Time now = Generics::Time::get_time_of_day();
    user_map_->clear_expired(now - expire_time_);
  }

  AdServer::Commons::Awaitable<void>
  UserNavigationContainer::co_process_request_trans_(const RequestInfo& request_info)
  {
    static const char* FUN = "UserNavigationContainer::co_process_request_trans_()";

    UserNavigationMap::Transaction_var transaction;

    try
    {
      const Generics::Time now = Generics::Time::get_time_of_day();
      const std::uint32_t oldest_date = static_cast<std::uint32_t>(
        (Algs::round_to_day(now) - NAVIGATION_HISTORY_PERIOD).tv_sec);
      const std::uint32_t request_date = static_cast<std::uint32_t>(
        Algs::round_to_day(request_info.time).tv_sec);

      if (request_date < oldest_date)
      {
        co_return;
      }

      const UserNavigationKey key(request_info.user_id, navigation_period_id_(request_date));

      Generics::ConstSmartMemBuf_var mem_buf;
      try
      {
        transaction = co_await user_map_->co_get_transaction(key);
        mem_buf = co_await transaction->co_get_profile();
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": on read, for user_id = " << request_info.user_id.to_string() <<
          " caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }

      UserNavigationProfileWriter profile_writer;
      if (mem_buf.in())
      {
        mem_buf = UserNavigationProfileAdapter()(mem_buf.in());
        profile_writer.init(mem_buf->membuf().data(), mem_buf->membuf().size());
      }
      else
      {
        profile_writer.version() = CURRENT_USER_NAVIGATION_PROFILE_VERSION;
      }

      auto& days = profile_writer.days();
      const auto first_actual = std::lower_bound(
        days.begin(),
        days.end(),
        oldest_date,
        [](const NavigationDayWriter& day, std::uint32_t date) noexcept
        {
          return day.date() < date;
        });

      bool profile_changed = first_actual != days.begin();
      days.erase(days.begin(), first_actual);

      auto day = days.end();
      if (days.empty() || days.back().date() < request_date)
      {
        NavigationDayWriter new_day;
        new_day.date() = request_date;
        days.push_back(std::move(new_day));
        day = std::prev(days.end());
      }
      else if (days.back().date() == request_date)
      {
        day = std::prev(days.end());
      }
      else
      {
        day = std::lower_bound(
          days.begin(),
          days.end(),
          request_date,
          [](const NavigationDayWriter& left, std::uint32_t right) noexcept
          {
            return left.date() < right;
          });

        if (day == days.end() || day->date() != request_date)
        {
          NavigationDayWriter new_day;
          new_day.date() = request_date;
          day = days.insert(day, std::move(new_day));
        }
      }

      auto& navigations = day->navigations();
      for (const std::string_view url : request_info.urls)
      {
        if (url.empty())
        {
          continue;
        }

        const auto navigation = std::lower_bound(
          navigations.begin(),
          navigations.end(),
          url,
          NavigationLess());

        if (navigation != navigations.end() && std::string_view(navigation->url()) == url)
        {
          if (navigation->count() != std::numeric_limits<std::uint64_t>::max())
          {
            ++navigation->count();
          }
        }
        else
        {
          NavigationWriter new_navigation;
          new_navigation.url().assign(url.data(), url.size());
          new_navigation.count() = 1;
          navigations.insert(navigation, std::move(new_navigation));
        }

        profile_changed = true;
      }

      const std::size_t navigations_size = navigation_count(days);
      if (navigations_size > user_navigations_limit_)
      {
        erase_oldest_navigations(days, navigations_size - user_navigations_limit_);
        profile_changed = true;
      }

      if (profile_changed)
      {
        Generics::SmartMemBuf_var new_mem_buf(new Generics::SmartMemBuf(profile_writer.size()));
        profile_writer.save(new_mem_buf->membuf().data(), new_mem_buf->membuf().size());
        co_await transaction->co_save_profile(Generics::transfer_membuf(new_mem_buf), now);
      }
    }
    catch (const PlainTypes::CorruptedStruct& ex)
    {
      if (transaction.in())
      {
        co_await transaction->co_remove_profile();
      }

      Stream::Error ostr;
      ostr << FUN << ": Caught PlainTypes::CorruptedStruct: " << ex.what();
      throw Exception(ostr);
    }
  }
}
