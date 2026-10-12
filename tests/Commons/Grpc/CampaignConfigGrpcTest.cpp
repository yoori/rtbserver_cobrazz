#include <CampaignSvcs/CampaignCommons/CampaignConfigGrpc.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServerGrpcAlgs.hpp>
#include <CampaignSvcs/CampaignServer/NonLinkedExpressionChannelGrpc.hpp>

#include <iostream>
#include <stdexcept>

namespace
{
  using namespace AdServer::CampaignSvcs;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void test_delivery_limits()
  {
    CampaignDeliveryLimits source{};
    source.date_start = Generics::Time(1700000000, 123456);
    source.date_end = Generics::Time(1800000000, 654321);
    source.budget = RevenueDecimal("123.45678901");
    source.daily_budget = RevenueDecimal("12.34567890");
    source.delivery_pacing = 'F';
    source.imps = 123UL;
    source.clicks = 45UL;
    CampaignDeliveryLimitsInfo wire;
    pack_delivery_limits(wire, source);
    CampaignGrpc::validate(wire);
    CampaignDeliveryLimits restored{};
    unpack_delivery_limits(restored, wire);
    require(restored == source, "Delivery limits round trip");

    source.budget = OptionalRevenueDecimal();
    source.daily_budget = OptionalRevenueDecimal();
    source.imps = AdServer::Commons::Optional<unsigned long>();
    source.clicks = AdServer::Commons::Optional<unsigned long>();
    pack_delivery_limits(wire, source);
    CampaignGrpc::validate(wire);
    require(!wire.imps_defined() && !wire.clicks_defined() && !wire.imps() && !wire.clicks(),
      "Reused delivery limits contain stale counts");
    unpack_delivery_limits(restored, wire);
    require(restored == source, "Absent delivery limits round trip");
  }

  void test_tokens_and_intervals()
  {
    OptionValueMap tokens;
    tokens[42] = std::string("a\0b", 3);
    tokens[43] = "";
    OptionValueSeq wire;
    pack_option_value_map(wire, tokens);
    OptionValueMap restored;
    unpack_option_value_map(restored, wire);
    require(restored == tokens, "Option values round trip");
    tokens.erase(42);
    pack_option_value_map(wire, tokens);
    require(wire.size() == 1 && wire[0].option_id() == 43, "Option sequence shrink");

    AdServer::Commons::IntervalSet<unsigned long> intervals;
    intervals.insert(AdServer::Commons::Interval<unsigned long>(3, 12));
    intervals.insert(AdServer::Commons::Interval<unsigned long>(20, 30));
    google::protobuf::RepeatedPtrField<adserver::campaign::server::WeeklyRunInterval> packed;
    fill_interval_sequence(packed, intervals);
    AdServer::Commons::IntervalSet<unsigned long> unpacked;
    convert_interval_sequence(unpacked, packed);
    require(unpacked == intervals, "Intervals round trip");
    intervals.clear();
    fill_interval_sequence(packed, intervals);
    require(packed.empty(), "Interval sequence clear");
  }

  void test_channels()
  {
    ChannelParams params(123);
    params.timestamp = Generics::Time(1700000000, 123456);
    params.country = "RU";
    params.status = static_cast<char>(225);
    params.cmp_params =
      new ChannelParams::CMPParams(17, RevenueDecimal("0.12345678"), RevenueDecimal("1.23456789"));
    NonLinkedExpressionChannel::Expression expression;
    expression.op = NonLinkedExpressionChannel::AND;
    expression.sub_channels.emplace_back(10);
    expression.sub_channels.emplace_back();
    auto& nested = expression.sub_channels.back();
    nested.op = NonLinkedExpressionChannel::OR;
    nested.sub_channels.emplace_back(20);
    nested.sub_channels.emplace_back(30);
    NonLinkedExpressionChannel_var source = new NonLinkedExpressionChannel(params, expression);

    ExpressionChannelInfo wire;
    pack_non_linked_expression_channel(wire, source);
    CampaignGrpc::validate(wire);
    ExpressionChannelInfo parsed;
    require(wire.status() == 225, "Character sign extension changed wire value");
    require(parsed.ParseFromString(wire.SerializeAsString()), "Expression protobuf parsing");
    auto restored = unpack_non_linked_channel(parsed);
    require(restored->expression() && *restored->expression() == expression,
      "Nested expression round trip");
    require(restored->params().channel_id == params.channel_id &&
        restored->params().timestamp == params.timestamp &&
        restored->params().cmp_params->imp_revenue == params.cmp_params->imp_revenue,
      "Channel params round trip");

    ExpressionChannelHolderMap channels;
    ExpressionChannel::Expression linked;
    unpack_expression(linked, parsed.expression(), channels);
    require(
      channels.size() == 3 && linked.sub_channels.size() == 2, "Linked expression references");
    unpack_expression(linked, parsed.expression(), channels);
    require(channels.size() == 3 && linked.sub_channels.size() == 2,
      "Reused expression duplicates references");
    unpack_non_linked_expression(expression, parsed.expression());
    require(
      expression.sub_channels.size() == 2, "Reused non-linked expression duplicates children");

    pack_platform_expression_channel(wire, 456, params.timestamp);
    CampaignGrpc::validate(wire);
    require(wire.expression().operation() == 'S' && wire.expression().sub_channels().empty() &&
        wire.imp_revenue().empty() && !wire.channel_rate_id(),
      "Platform contains stale fields");
    unpack_channel_params(params, wire);
    require(
      params.channel_id == 456 && !params.cmp_params.in(), "Reused channel params retain rate");
    auto platform = unpack_non_linked_channel(wire);
    require(!platform->expression(), "Platform must be a simple channel");
    auto linked_platform = unpack_channel(wire, channels);
    require(linked_platform->params().channel_id == 456, "Linked platform params");

    pack_non_linked_expression_channel(wire, source);
    pack_non_linked_expression_channel(wire, platform);
    require(wire.expression().sub_channels().empty(), "Simple channel contains stale expression");
  }
}

int main()
{
  try
  {
    test_delivery_limits();
    test_tokens_and_intervals();
    test_channels();
    std::cout << "PASS: delivery limits, tokens, intervals, expressions and object reuse\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
