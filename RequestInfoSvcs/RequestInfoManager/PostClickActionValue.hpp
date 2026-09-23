#pragma once

#include <cstdint>
#include <string_view>

#include <Generics/Time.hpp>

namespace AdServer::RequestInfoSvcs
{
  std::string_view
  post_click_action_type(std::string_view action_name) noexcept;

  struct PostClickActionValue
  {
    bool landing_bounced = false;
    std::uint64_t landing_session_time = 0;
    std::uint64_t landing_page_views = 0;
    bool landing_is_new_user = false;
    std::uint64_t yandex_ref_id = 0;
    Generics::Time yandex_event_date;
    bool yandex_reporting_comparable = false;
  };

  PostClickActionValue
  parse_post_click_action_value(std::string_view value);
}
