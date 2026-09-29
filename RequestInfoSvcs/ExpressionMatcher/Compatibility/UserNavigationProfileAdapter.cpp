#include "UserNavigationProfileAdapter.hpp"

#include <algorithm>
#include <iterator>
#include <string_view>
#include <utility>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserNavigationProfile_v1.hpp>
#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserNavigationProfile_v2.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserNavigationProfile.hpp>

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
    void append_legacy(NavigationDayWriter& day, std::string_view value, std::uint64_t count)
    {
      if (value.substr(0, 3) == "rtb" || value.substr(0, 4) == "poad")
      {
        PageKeywordWriter keyword;
        keyword.keyword().assign(value.data(), value.size());
        keyword.count() = count;
        day.page_keywords().push_back(std::move(keyword));
      }
      else
      {
        NavigationWriter navigation;
        navigation.url().assign(value.data(), value.size());
        navigation.count() = count;
        day.navigations().push_back(std::move(navigation));
      }
    }

    Generics::ConstSmartMemBuf_var convert_v2(const Generics::MemBuf& mem_buf)
    {
      const RequestInfoSvcs_v2::UserNavigationProfileReader old_profile(
        mem_buf.data(), mem_buf.size());
      UserNavigationProfileWriter profile;
      profile.version() = CURRENT_USER_NAVIGATION_PROFILE_VERSION;
      for (const auto old_day : old_profile.days())
      {
        NavigationDayWriter day;
        day.date() = old_day.date();
        for (const auto navigation : old_day.navigations())
        {
          append_legacy(day, navigation.url(), navigation.count());
        }
        profile.days().push_back(std::move(day));
      }
      Generics::SmartMemBuf_var result(new Generics::SmartMemBuf(profile.size()));
      profile.save(result->membuf().data(), result->membuf().size());
      return Generics::transfer_membuf(result);
    }

    Generics::ConstSmartMemBuf_var
    convert_v1(const Generics::MemBuf& mem_buf)
    {
      const RequestInfoSvcs_v1::UserNavigationProfileReader old_profile(
        mem_buf.data(),
        mem_buf.size());

      UserNavigationProfileWriter profile;
      profile.version() = CURRENT_USER_NAVIGATION_PROFILE_VERSION;

      auto& days = profile.days();
      const auto old_navigations = old_profile.navigations();
      auto old_navigation = old_navigations.begin();
      const auto old_navigation_end = old_navigations.end();
      while (old_navigation != old_navigation_end)
      {
        const std::uint32_t date = (*old_navigation).date();
        const auto day_end = std::find_if(
          old_navigation,
          old_navigation_end,
          [date](const RequestInfoSvcs_v1::NavigationReader& navigation) noexcept
          {
            return navigation.date() != date;
          });

        NavigationDayWriter day;
        day.date() = date;
        auto& navigations = day.navigations();
        navigations.reserve(std::distance(old_navigation, day_end));
        for (; old_navigation != day_end; ++old_navigation)
        {
          append_legacy(day, (*old_navigation).url(), (*old_navigation).count());
        }
        days.push_back(std::move(day));
      }

      Generics::SmartMemBuf_var result(new Generics::SmartMemBuf(profile.size()));
      profile.save(result->membuf().data(), result->membuf().size());
      return Generics::transfer_membuf(result);
    }
  }

  Generics::ConstSmartMemBuf_var
  UserNavigationProfileAdapter::operator()(const Generics::ConstSmartMemBuf* mem_buf) const
  {
    static const char* FUN = "UserNavigationProfileAdapter::operator()";

    if (mem_buf->membuf().size() < UserNavigationProfileVersionReader::FIXED_SIZE)
    {
      throw Exception("Corrupt header");
    }

    try
    {
      const UserNavigationProfileVersionReader version(
        mem_buf->membuf().data(),
        mem_buf->membuf().size());

      if (version.version() == CURRENT_USER_NAVIGATION_PROFILE_VERSION)
      {
        return ReferenceCounting::add_ref(mem_buf);
      }

      if (version.version() == 1)
      {
        return convert_v1(mem_buf->membuf());
      }

      if (version.version() == 2)
      {
        return convert_v2(mem_buf->membuf());
      }

      Stream::Error ostr;
      ostr << FUN << ": unsupported profile version = " << version.version();
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
}
