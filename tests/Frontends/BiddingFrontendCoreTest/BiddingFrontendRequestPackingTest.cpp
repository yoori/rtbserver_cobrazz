#include <iostream>
#include <stdexcept>
#include <string>

#include <Logger/StreamLogger.hpp>

#include <Commons/GrpcAlgs.hpp>
#include <Frontends/Modules/BiddingFrontend/BiddingFrontendCore.hpp>

namespace
{
  namespace CM = adserver::campaign_svcs::campaign_manager;
  namespace UIM = adserver::user_info_svcs::user_info_manager;
  using AdServer::Bidding::BiddingFrontendCore;
  using AdServer::Bidding::RequestInfo;
  using AdServer::Commons::UserId;
  using AdServer::CampaignSvcs::AccuracyDecimal;
  using AdServer::CampaignSvcs::CoordDecimal;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  RequestInfo make_request_info()
  {
    RequestInfo info;
    info.user_id = UserId::create_random_based();
    info.track_user_id = UserId::create_random_based();
    info.only_display_ad = true;
    for (unsigned int i = 1; i <= 2; ++i)
    {
      auto& coord = info.coord_location.emplace_back();
      coord.longitude = CoordDecimal(false, i, 0);
      coord.latitude = CoordDecimal(false, i + 10, 0);
      coord.accuracy = AccuracyDecimal(false, i + 20, 0);
      info.ad_slots.emplace_back(info.arena());
    }

    return info;
  }

  CM::GetCampaignCreativeRequest prepare(
    BiddingFrontendCore& core,
    const RequestInfo& info,
    const UIM::MatchResponse* history = nullptr,
    const UserId& user_id = {},
    bool passback = false,
    bool interrupted = false)
  {
    CM::GetCampaignCreativeRequest request;
    core.prepare_get_campaign_creative_request_(
      request, history, nullptr, nullptr, info, user_id, passback, history != nullptr, interrupted);
    return request;
  }

  void test_request_fallback(BiddingFrontendCore& core)
  {
    auto info = make_request_info();
    const auto request = prepare(core, info);
    const auto& params = request.request_params();
    const auto& common = params.common_info();
    require(common.user_id() == GrpcAlgs::pack_user_id(*info.user_id), "source user ID lost");
    require(common.track_user_id() == GrpcAlgs::pack_user_id(*info.track_user_id),
      "source tracking ID lost");
    require(common.coord_location_size() == 2, "request coordinates lost without history");
    for (int i = 0; i < common.coord_location_size(); ++i)
    {
      const auto& actual = common.coord_location(i);
      const auto& expected = info.coord_location[i];
      require(actual.longitude() == GrpcAlgs::pack_decimal(expected.longitude) &&
        actual.latitude() == GrpcAlgs::pack_decimal(expected.latitude) &&
        actual.accuracy() == GrpcAlgs::pack_decimal(expected.accuracy),
        "request coordinate changed");
    }

    const auto zero_time = GrpcAlgs::pack_time(Generics::Time::ZERO);
    require(params.client_create_time() == zero_time && params.session_start() == zero_time,
      "missing history must use packed zero times");
    require(!params.only_display_ad(), "RTB must override only_display_ad");
    require(!params.profiling_available() && !params.fraud(), "unexpected history flags");

    info.user_id.reset();
    const auto without_user = prepare(core, info);
    require(without_user.request_params().common_info().user_id().empty(),
      "absent user ID must remain empty");
    require(without_user.request_params().common_info().track_user_id() == common.track_user_id(),
      "tracking ID must not depend on source user ID");

    info.user_id = UserId();
    info.track_user_id.reset();
    const auto without_tracking = prepare(core, info);
    require(without_tracking.request_params().common_info().track_user_id().empty(),
      "absent tracking ID must remain empty");
    require(without_tracking.request_params().common_info().user_id() ==
      GrpcAlgs::pack_user_id(UserId()), "present null user ID must remain packed");
  }

  void test_resolved_user_and_history(BiddingFrontendCore& core)
  {
    auto info = make_request_info();
    UIM::MatchResponse history;
    auto* result = history.mutable_match_result();
    result->set_create_time(GrpcAlgs::pack_time(Generics::Time(123, 456)));
    result->set_session_start(GrpcAlgs::pack_time(Generics::Time(789, 123)));
    result->set_fraud_request(true);
    auto* coord = result->add_geo_data_seq();
    coord->set_longitude(GrpcAlgs::pack_decimal(CoordDecimal(false, 40, 0)));
    coord->set_latitude(GrpcAlgs::pack_decimal(CoordDecimal(false, 50, 0)));
    coord->set_accuracy(GrpcAlgs::pack_decimal(AccuracyDecimal(false, 60, 0)));

    const auto resolved_user = UserId::create_random_based();
    const auto request = prepare(core, info, &history, resolved_user);
    const auto& params = request.request_params();
    const auto& common = params.common_info();
    const auto packed_user = GrpcAlgs::pack_user_id(resolved_user);
    require(common.user_id() == packed_user && common.track_user_id() == packed_user,
      "resolved user must override both source IDs");
    require(common.coord_location_size() == 1, "history must replace all request coordinates");
    require(common.coord_location(0).longitude() == coord->longitude() &&
      common.coord_location(0).latitude() == coord->latitude() &&
      common.coord_location(0).accuracy() == coord->accuracy(), "history coordinate changed");
    require(params.client_create_time() == result->create_time() &&
      params.session_start() == result->session_start(), "history times changed");
    require(params.profiling_available() && params.fraud(), "history flags lost");
    require(params.ad_slots(0).passback() && params.ad_slots(1).passback(),
      "fraud must force passback for every slot");

    UIM::MatchResponse empty_history;
    const auto empty_request = prepare(core, info, &empty_history);
    const auto& empty_params = empty_request.request_params();
    require(empty_params.common_info().coord_location_size() == 0,
      "present empty history must not fall back to request coordinates");
    require(empty_params.client_create_time().empty() && empty_params.session_start().empty(),
      "present empty history must preserve empty times");
  }

  void test_debug_modes(BiddingFrontendCore& core)
  {
    struct Case
    {
      const char* mode;
      bool explicit_debug;
      unsigned long request_ccg;
      unsigned long second_slot_ccg;
      bool interrupted;
      bool expected_debug;
    };

    const Case cases[] = {
      {"", false, 0, 0, false, false},
      {"header", false, 0, 0, false, true},
      {"BoDy", false, 0, 0, false, true},
      {"invalid", false, 0, 0, false, false},
      {"", true, 0, 0, false, true},
      {"", false, 42, 0, false, true},
      {"", false, 0, 43, false, true},
      {"body", false, 42, 43, true, false},
      {"", true, 0, 0, true, true}
    };

    for (const auto& test : cases)
    {
      auto info = make_request_info();
      info.require_debug_info = test.mode;
      info.need_debug_info = test.explicit_debug;
      info.debug_ccg = test.request_ccg;
      info.ad_slots[0].debug_ccg = 99;
      info.ad_slots[1].debug_ccg = test.second_slot_ccg;
      const auto request = prepare(core, info, nullptr, {}, false, test.interrupted);
      const auto& params = request.request_params();
      require(params.need_debug_info() == test.expected_debug, "wrong debug mode");
      require(params.ad_slots(0).debug_ccg() == (test.interrupted ? 99 : test.request_ccg),
        "first slot CCG override changed");
      require(params.ad_slots(1).debug_ccg() == test.second_slot_ccg,
        "second slot CCG changed");
    }

    RequestInfo empty_info;
    empty_info.require_debug_info = "body";
    require(!prepare(core, empty_info).request_params().need_debug_info(),
      "empty request must not implicitly enable debug");
    empty_info.need_debug_info = true;
    require(prepare(core, empty_info).request_params().need_debug_info(),
      "explicit debug must be preserved even without slots");
  }

  void test_passback(BiddingFrontendCore& core)
  {
    auto info = make_request_info();
    info.ad_slots[1].passback = true;
    const auto normal = prepare(core, info);
    require(!normal.request_params().ad_slots(0).passback() &&
      normal.request_params().ad_slots(1).passback(), "per-slot passback changed");
    const auto passback = prepare(core, info, nullptr, {}, true);
    require(passback.request_params().ad_slots(0).passback() &&
      passback.request_params().ad_slots(1).passback(), "request passback must cover all slots");
    const auto interrupted = prepare(core, info, nullptr, {}, false, true);
    require(interrupted.request_params().ad_slots(0).passback() &&
      interrupted.request_params().ad_slots(1).passback(), "interruption must cover all slots");
  }
}

int main()
{
  try
  {
    BiddingFrontendCore::InitParams params{};
    params.logger = new Logging::Null::Logger;
    BiddingFrontendCore core(params);
    test_request_fallback(core);
    test_resolved_user_and_history(core);
    test_debug_modes(core);
    test_passback(core);
    std::cout << "BiddingFrontendRequestPackingTest passed\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "BiddingFrontendRequestPackingTest: " << ex.what() << '\n';
    return 1;
  }
}
