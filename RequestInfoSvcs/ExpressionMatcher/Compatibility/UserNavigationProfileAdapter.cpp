#include "UserNavigationProfileAdapter.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <RequestInfoSvcs/ExpressionMatcher/Compatibility/UserNavigationProfile_v1.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserNavigationProfile.hpp>

namespace AdServer::RequestInfoSvcs
{
  namespace
  {
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
          NavigationWriter navigation;
          navigation.url() = (*old_navigation).url();
          navigation.count() = (*old_navigation).count();
          navigations.push_back(std::move(navigation));
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
