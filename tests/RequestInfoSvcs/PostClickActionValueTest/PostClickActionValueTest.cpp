#include <iostream>
#include <stdexcept>
#include <string_view>

#include <eh/Exception.hpp>

#include <RequestInfoSvcs/RequestInfoManager/PostClickActionValue.hpp>

namespace
{
  using AdServer::RequestInfoSvcs::parse_post_click_action_value;
  using AdServer::RequestInfoSvcs::post_click_action_type;

  void
  expect_error(std::string_view value)
  {
    try
    {
      parse_post_click_action_value(value);
    }
    catch (const eh::Exception&)
    {
      return;
    }

    throw std::runtime_error("Expected post-click value parse error");
  }
}

int
main()
{
  try
  {
    if (post_click_action_type("landing") != "landing" ||
        post_click_action_type("landing:17-123") != "landing" ||
        post_click_action_type("custom:value:part") != "custom" ||
        !post_click_action_type(":suffix").empty())
    {
      throw std::runtime_error("Post-click action type mismatch");
    }

    const auto value = parse_post_click_action_value(
      R"({"landing_bounced":true,"landing_session_time":17,)"
      R"("landing_page_views":3,"landing_is_new_user":false})");
    if (!value.landing_bounced || value.landing_session_time != 17 ||
        value.landing_page_views != 3 || value.landing_is_new_user)
    {
      throw std::runtime_error("Parsed post-click value mismatch");
    }

    const auto numeric_bools = parse_post_click_action_value(
      R"({"landing_bounced":0,"landing_session_time":0,)"
      R"("landing_page_views":1,"landing_is_new_user":1})");
    if (numeric_bools.landing_bounced || numeric_bools.landing_session_time != 0 ||
        numeric_bools.landing_page_views != 1 || !numeric_bools.landing_is_new_user)
    {
      throw std::runtime_error("Numeric post-click boolean mismatch");
    }

    const auto yandex_value = parse_post_click_action_value(
      R"({"landing_bounced":false,"landing_session_time":42,)"
      R"("landing_page_views":5,"landing_is_new_user":true,)"
      R"("yandex_ref_id":17,"yandex_event_date":"2026-09-10",)"
      R"("yandex_reporting_comparable":true})");
    if (yandex_value.yandex_ref_id != 17 ||
        yandex_value.yandex_event_date != Generics::Time(
          String::SubString("2026-09-10"), "%Y-%m-%d", true) ||
        !yandex_value.yandex_reporting_comparable)
    {
      throw std::runtime_error("Parsed Yandex post-click metadata mismatch");
    }

    expect_error(R"({"landing_bounced":true})");
    expect_error(
      R"({"landing_bounced":true,"landing_bounced":false,)"
      R"("landing_session_time":1,"landing_page_views":1,)"
      R"("landing_is_new_user":false})");
    expect_error(
      R"({"landing_bounced":true,"landing_session_time":-1,)"
      R"("landing_page_views":1,"landing_is_new_user":false})");
    expect_error(
      R"({"landing_bounced":2,"landing_session_time":1,)"
      R"("landing_page_views":1,"landing_is_new_user":false})");
    expect_error(
      R"({"landing_bounced":false,"landing_session_time":1,)"
      R"("landing_page_views":1,"landing_is_new_user":false,)"
      R"("yandex_ref_id":17})");
    expect_error(
      R"({"landing_bounced":false,"landing_session_time":1,)"
      R"("landing_page_views":1,"landing_is_new_user":false,)"
      R"("yandex_ref_id":17,"yandex_event_date":"invalid",)"
      R"("yandex_reporting_comparable":true})");
    expect_error(
      R"({"landing_bounced":false,"landing_session_time":1,)"
      R"("landing_page_views":1,"landing_is_new_user":false,)"
      R"("yandex_ref_id":-1,"yandex_event_date":"2026-09-10",)"
      R"("yandex_reporting_comparable":true})");
    expect_error("not-json");
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }

  return 0;
}
