#pragma once

#include <Commons/JsonFormatter.hpp>
#include <Generics/Time.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserNavigationProfile.hpp>

namespace AdServer::RequestInfoSvcs
{
  inline void append_navigation_json(
    AdServer::Commons::JsonObject& object, const UserNavigationProfileReader* profile)
  {
    object.add_number("version", 3);
    const auto append = [&](const char* collection, const char* field, auto entries, auto value)
    {
      AdServer::Commons::JsonObject array(object.add_array(collection));
      if (profile)
      {
        for (const auto day : profile->days())
        {
          const std::string date = Generics::Time(day.date()).get_gm_time().format("%F");
          for (const auto entry : entries(day))
          {
            AdServer::Commons::JsonObject json(array.add_object());
            json.add_escaped_string("date", date);
            json.add_escaped_string(field, value(entry));
            json.add_number("count", entry.count());
          }
        }
      }
    };
    append("urls", "url", [](const auto& day) { return day.navigations(); },
      [](const auto& entry) { return entry.url(); });
    append("page_keywords", "keyword", [](const auto& day) { return day.page_keywords(); },
      [](const auto& entry) { return entry.keyword(); });
  }
}
