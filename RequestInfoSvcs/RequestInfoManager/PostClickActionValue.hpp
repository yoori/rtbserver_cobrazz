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
    bool landing_is_robot = false;
    bool landing_robot_available = false;
    std::uint64_t external_metrics_connection_type_id = 0;
    Generics::Time source_date;
    bool reporting_comparable = false;
  };

  PostClickActionValue
  parse_post_click_action_value(std::string_view value);

  struct PostClickConversionValue
  {
    std::uint64_t visits = 0;
    std::uint64_t goals = 0;
    std::uint64_t external_metrics_connection_type_id = 0;
    Generics::Time source_date;
  };

  bool is_post_click_conversion(std::string_view name) noexcept;
  PostClickConversionValue
  parse_post_click_conversion(std::string_view name, std::string_view value);
}
