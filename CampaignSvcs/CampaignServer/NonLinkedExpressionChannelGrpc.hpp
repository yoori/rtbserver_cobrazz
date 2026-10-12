#pragma once

#include <CampaignSvcs/CampaignCommons/CampaignConfigTypes.hpp>

#include <CampaignSvcs/CampaignCommons/ExpressionChannelGrpc.hpp>
#include "NonLinkedExpressionChannel.hpp"

namespace AdServer::CampaignSvcs
{
  inline void pack_non_linked_expression(ExpressionInfo& expression_info,
    const NonLinkedExpressionChannel::Expression& expression) noexcept
  {
    expression_info.Clear();
    expression_info.set_operation(expression.op);
    expression_info.set_channel_id(expression.channel_id);
    for (const auto& sub_channel : expression.sub_channels)
    {
      pack_non_linked_expression(*expression_info.add_sub_channels(), sub_channel);
    }
  }

  inline void unpack_non_linked_expression(
    NonLinkedExpressionChannel::Expression& expression, const ExpressionInfo& expression_info)
  {
    expression.op = static_cast<NonLinkedExpressionChannel::Operation>(expression_info.operation());
    expression.channel_id = expression_info.channel_id();
    expression.sub_channels.clear();
    for (const auto& sub_channel : expression_info.sub_channels())
    {
      NonLinkedExpressionChannel::Expression sub_expr;
      unpack_non_linked_expression(sub_expr, sub_channel);
      expression.sub_channels.push_back(sub_expr);
    }
  }

  inline void pack_platform_expression_channel(ExpressionChannelInfo& channel_info,
    unsigned long channel_id, const Generics::Time& timestamp) noexcept
  {
    pack_platform_channel_params(channel_info, channel_id, timestamp);
    channel_info.mutable_expression()->set_operation('S');
  }

  inline void pack_non_linked_expression_channel(ExpressionChannelInfo& channel_info,
    const NonLinkedExpressionChannel* expression_channel) noexcept
  {
    pack_channel_params(channel_info, expression_channel->params());

    const NonLinkedExpressionChannel::Expression* expr = expression_channel->expression();
    if (expr)
    {
      pack_non_linked_expression(*channel_info.mutable_expression(), *expr);
    }
    else
    {
      channel_info.mutable_expression()->set_operation('S');
    }
  }

  inline NonLinkedExpressionChannel_var unpack_non_linked_channel(
    const ExpressionChannelInfo& channel_info)
  /*throw(eh::Exception)*/
  {
    ChannelParams channel_params;
    unpack_channel_params(channel_params, channel_info);

    if (static_cast<char>(channel_info.expression().operation()) != 'S')
    {
      NonLinkedExpressionChannel::Expression expr;
      unpack_non_linked_expression(expr, channel_info.expression());
      return new NonLinkedExpressionChannel(channel_params, expr);
    }
    else
    {
      return new NonLinkedExpressionChannel(channel_params);
    }
  }
}
