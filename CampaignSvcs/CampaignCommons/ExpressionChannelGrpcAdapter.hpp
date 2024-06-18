#ifndef EXPRESSION_CHANNEL_GRPC_ADAPTER_HPP_
#define EXPRESSION_CHANNEL_GRPC_ADAPTER_HPP_

#include "CampaignTypes.hpp"
#include "CampaignManager.pb.h"
#include <CampaignSvcs/CampaignCommons/CampaignCommons.hpp>
#include <Commons/GrpcAlgs.hpp>

namespace AdServer::CampaignSvcs
{

inline void pack_expression(
  Proto::ExpressionInfo& expression_info,
  const ExpressionChannel::Expression& expression)
{
  expression_info.set_operation(expression.op);
  if(expression.op == ExpressionChannel::NOP)
  {
    if(expression.channel.in() &&
      (expression.channel->expression_channel().in() ||
      expression.channel->simple_channel().in()))
    {
      expression_info.set_channel_id(expression.channel->params().channel_id);
    }
    else
    {
      expression_info.set_channel_id(0);
    }
  }
  else
  {
    auto* sub_channels_proto = expression_info.mutable_sub_channels();
    sub_channels_proto->Reserve(expression.sub_channels.size());
    for(const auto& sub_channel : expression.sub_channels)
    {
      auto& element = *sub_channels_proto->Add();
      pack_expression(element, sub_channel);
    }
  }
}

inline void pack_channel_params(
  Proto::ExpressionChannelInfo& channel_info,
  const ChannelParams& channel_params)
{
  channel_info.set_channel_id(channel_params.channel_id);
  channel_info.set_type(channel_params.type);
  channel_info.set_country_code(channel_params.country);
  channel_info.set_status(channel_params.status);
  channel_info.set_action_id(channel_params.action_id);
  channel_info.set_timestamp(GrpcAlgs::pack_time(channel_params.timestamp));

  if(channel_params.common_params.in())
  {
    channel_info.set_account_id(channel_params.common_params->account_id);
    channel_info.set_flags(channel_params.common_params->flags);
    channel_info.set_is_public(channel_params.common_params->is_public);
    channel_info.set_freq_cap_id(channel_params.common_params->freq_cap_id);
    channel_info.set_language(channel_params.common_params->language);
  }
  else
  {
    channel_info.set_account_id(0);
    channel_info.set_flags(0);
    channel_info.set_is_public(true);
    channel_info.set_freq_cap_id(0);
  }

  if(channel_params.descriptive_params.in())
  {
    channel_info.set_name(channel_params.descriptive_params->name);
    channel_info.set_parent_channel_id(channel_params.descriptive_params->parent_channel_id);
  }
  else
  {
    channel_info.set_parent_channel_id(0);
  }

  if(channel_params.discover_params.in())
  {
    channel_info.set_discover_query(channel_params.discover_params->query);
    channel_info.set_discover_annotation(channel_params.discover_params->annotation);
  }

  if(channel_params.cmp_params.in())
  {
    channel_info.set_channel_rate_id(channel_params.cmp_params->channel_rate_id);
    channel_info.set_imp_revenue(GrpcAlgs::pack_decimal(
      channel_params.cmp_params->imp_revenue));
    channel_info.set_click_revenue(GrpcAlgs::pack_decimal(
      channel_params.cmp_params->click_revenue));
  }
  else
  {
    channel_info.set_channel_rate_id(0);
  }
}

inline void pack_simple_channel(
  Proto::ExpressionChannelInfo& channel_info,
  const SimpleChannel* simple_channel)
{
  pack_channel_params(channel_info, simple_channel->params());

  channel_info.mutable_expression()->set_operation(static_cast<std::uint32_t>('S'));
}

inline void pack_expression_channel(
  Proto::ExpressionChannelInfo& channel_info,
  const ExpressionChannel* expression_channel)
{
  pack_channel_params(channel_info, expression_channel->params());

  const ExpressionChannel::Expression& expr = expression_channel->expression();
  pack_expression(*channel_info.mutable_expression(), expr);
}

inline void pack_channel(
  Proto::ExpressionChannelInfo& channel_info,
  const ExpressionChannelBase* channel)
{
  ConstSimpleChannel_var simple_channel = channel->simple_channel();
  if(simple_channel.in())
  {
    pack_simple_channel(channel_info, simple_channel);
  }
  else
  {
    ConstExpressionChannel_var expression_channel = channel->expression_channel();
    assert(expression_channel.in());
    pack_expression_channel(channel_info, expression_channel);
  }
}

} // namespace AdServer::CampaignSvcs

#endif //EXPRESSION_CHANNEL_GRPC_ADAPTER_HPP_
