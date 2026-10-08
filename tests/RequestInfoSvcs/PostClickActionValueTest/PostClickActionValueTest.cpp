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
      R"("external_metrics_connection_id":17,"source_date":"2026-09-10",)"
      R"("reporting_comparable":true})");
    const auto typed = parse_post_click_action_value(
      R"({"landing_bounced":false,"landing_session_time":42,)"
      R"("landing_page_views":5,"landing_is_new_user":true,)"
      R"("external_metrics_connection_type_id":2,"source_date":"2026-09-10"})");
    if (typed.external_metrics_connection_type_id != 2)
    {
      throw std::runtime_error("Connector type was interpreted as a connection ID");
    }

    const auto goal = AdServer::RequestInfoSvcs::parse_post_click_conversion(
      "ym_111_9:g:2026-09-10:123:12",
      R"({"external_metrics_connection_type_id":1,"source_date":"2026-09-10",)"
      R"("visits":0,"goals":1})");
    const auto old_goal = AdServer::RequestInfoSvcs::parse_post_click_conversion(
      "ym_111_9:g:2026-09-10:123:12",
      R"({"external_metrics_connection_id":999999,"source_date":"2026-09-10",)"
      R"("visits":0,"goals":1})");
    if (goal.external_metrics_connection_type_id != 1 || goal.visits != 0 || goal.goals != 1 ||
      old_goal.external_metrics_connection_type_id != 1)
    {
      throw std::runtime_error("Named conversion connector type mismatch");
    }
    expect_error(
      R"({"landing_bounced":false,"landing_session_time":42,)"
      R"("landing_page_views":5,"landing_is_new_user":true,)"
      R"("external_metrics_connection_type_id":32768,"source_date":"2026-09-10"})");
    if (yandex_value.external_metrics_connection_type_id != 1 ||
        yandex_value.source_date != Generics::Time(
          String::SubString("2026-09-10"), "%Y-%m-%d", true) ||
        !yandex_value.reporting_comparable)
    {
      throw std::runtime_error("Parsed Yandex post-click metadata mismatch");
    }

    const auto external = parse_post_click_action_value(
      R"({"landing_bounced":false,"landing_session_time":1,)"
      R"("landing_page_views":1,"landing_is_new_user":false,)"
      R"("external_metrics_connection_id":18,"source_date":"2026-09-10"})");
    if (external.external_metrics_connection_type_id != 1 ||
      value.external_metrics_connection_type_id != 0)
    {
      throw std::runtime_error("Generic external/internal connection mismatch");
    }

    const auto legacy = parse_post_click_action_value(
      R"({"landing_bounced":false,"landing_session_time":1,)"
      R"("landing_page_views":1,"landing_is_new_user":false,)"
      R"("yandex_ref_id":17,"yandex_event_date":"2026-09-10",)"
      R"("yandex_reporting_comparable":true})");
    if (legacy.external_metrics_connection_type_id != 1 || !legacy.reporting_comparable)
    {
      throw std::runtime_error("Legacy pending profile cannot be read");
    }

    expect_error(
      R"({"landing_bounced":false,"landing_session_time":1,)"
      R"("landing_page_views":1,"landing_is_new_user":false,)"
      R"("yandex_ref_id":17,"external_metrics_connection_id":18,)"
      R"("source_date":"2026-09-10"})");
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
      R"("external_metrics_connection_id":17})");
    expect_error(
      R"({"landing_bounced":false,"landing_session_time":1,)"
      R"("landing_page_views":1,"landing_is_new_user":false,)"
      R"("external_metrics_connection_id":17,"source_date":"invalid",)"
      R"("reporting_comparable":true})");
    expect_error(
      R"({"landing_bounced":false,"landing_session_time":1,)"
      R"("landing_page_views":1,"landing_is_new_user":false,)"
      R"("external_metrics_connection_id":-1,"source_date":"2026-09-10",)"
      R"("reporting_comparable":true})");
    expect_error("not-json");
  }
  catch (const std::exception& ex)
  {
    std::cerr << ex.what() << '\n';
    return 1;
  }

  return 0;
}
