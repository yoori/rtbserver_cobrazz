#include "UserNavigationContainer.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

#include <RequestInfoSvcs/RequestInfoCommons/UserNavigationProfile.hpp>

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
    constexpr std::uint32_t CURRENT_USER_NAVIGATION_PROFILE_VERSION = 1;
    const Generics::Time NAVIGATION_HISTORY_PERIOD = Generics::Time::ONE_DAY * 30;

    struct NavigationKey
    {
      std::uint32_t date;
      std::string_view url;
    };

    struct NavigationLess
    {
      bool
      operator()(const NavigationWriter& left, const NavigationKey& right) const noexcept
      {
        if (left.date() != right.date)
        {
          return left.date() < right.date;
        }

        return std::string_view(left.url()) < right.url;
      }
    };
  }

  UserNavigationContainer::UserNavigationContainer(
    Logging::Logger* logger,
    unsigned long common_chunks_number,
    const AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders,
    const char* file_prefix,
    const AdServer::ProfilingCommons::LevelMapTraits& user_level_map_traits,
    std::shared_ptr<AdServer::ProfilingCommons::RocksDBProfileMapProcessor>
      rocksdb_processor)
    : logger_(ReferenceCounting::add_ref(logger)),
      expire_time_(user_level_map_traits.expire_time)
  {
    static const char* FUN = "UserNavigationContainer::UserNavigationContainer()";

    try
    {
      auto user_map = AdServer::ProfilingCommons::ProfileMapFactory::
        open_rocksdb_chunked_map<
          AdServer::Commons::UserId,
          AdServer::ProfilingCommons::UserIdAccessor,
          unsigned long (*)(const Generics::Uuid&)>(
            common_chunks_number,
            chunk_folders,
            file_prefix,
            AdServer::ProfilingCommons::ProfileMapFactory::ProfileMapTraits(
              user_level_map_traits.expire_time),
            AdServer::Commons::uuid_distribution_hash,
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

  AdServer::Commons::Awaitable<Generics::ConstSmartMemBuf_var>
  UserNavigationContainer::co_get_profile(
    const AdServer::Commons::UserId& user_id,
    std::optional<std::uint32_t> date)
  {
    static const char* FUN = "UserNavigationContainer::co_get_profile()";

    try
    {
      Generics::ConstSmartMemBuf_var profile = co_await user_map_->co_get_profile(user_id);
      if (!profile.in() || !date.has_value())
      {
        co_return profile;
      }

      UserNavigationProfileWriter profile_writer;
      profile_writer.init(profile->membuf().data(), profile->membuf().size());

      auto& navigations = profile_writer.navigations();
      const auto first_after_date = std::upper_bound(
        navigations.begin(),
        navigations.end(),
        *date,
        [](std::uint32_t requested_date, const NavigationWriter& navigation) noexcept
        {
          return requested_date < navigation.date();
        });

      if (first_after_date == navigations.end())
      {
        co_return profile;
      }

      navigations.erase(first_after_date, navigations.end());
      Generics::SmartMemBuf_var filtered_profile(
        new Generics::SmartMemBuf(profile_writer.size()));
      profile_writer.save(
        filtered_profile->membuf().data(),
        filtered_profile->membuf().size());
      co_return Generics::transfer_membuf(filtered_profile);
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

    if (request_info.user_id.is_null() || request_info.url.empty())
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

      Generics::ConstSmartMemBuf_var mem_buf;
      try
      {
        transaction = co_await user_map_->co_get_transaction(request_info.user_id);
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
        profile_writer.init(mem_buf->membuf().data(), mem_buf->membuf().size());
      }
      else
      {
        profile_writer.version() = CURRENT_USER_NAVIGATION_PROFILE_VERSION;
      }

      auto& navigations = profile_writer.navigations();
      const auto first_actual = std::lower_bound(
        navigations.begin(),
        navigations.end(),
        oldest_date,
        [](const NavigationWriter& navigation, std::uint32_t date) noexcept
        {
          return navigation.date() < date;
        });

      bool profile_changed = first_actual != navigations.begin();
      navigations.erase(navigations.begin(), first_actual);

      if (request_date >= oldest_date)
      {
        const NavigationKey key{request_date, request_info.url};
        const auto navigation = std::lower_bound(
          navigations.begin(),
          navigations.end(),
          key,
          NavigationLess());

        if (navigation != navigations.end() &&
          navigation->date() == request_date &&
          std::string_view(navigation->url()) == request_info.url)
        {
          if (navigation->count() != std::numeric_limits<std::uint64_t>::max())
          {
            ++navigation->count();
          }
        }
        else
        {
          NavigationWriter new_navigation;
          new_navigation.date() = request_date;
          new_navigation.url().assign(request_info.url.data(), request_info.url.size());
          new_navigation.count() = 1;
          navigations.insert(navigation, std::move(new_navigation));
        }

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
