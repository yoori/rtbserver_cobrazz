#pragma once

#include <Commons/GrpcAlgs.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignConfigTypes.hpp>

#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>

namespace AdServer::CampaignSvcs
{
  inline void pack_platform_channel_params(
    ExpressionChannelInfo& channel_info, unsigned long channel_id, const Generics::Time& timestamp)
  {
    channel_info.Clear();
    channel_info.set_channel_id(channel_id);
    channel_info.set_type('V');
    channel_info.set_status('A');
    channel_info.set_action_id(0);
    channel_info.set_timestamp(GrpcAlgs::pack_time(timestamp));

    channel_info.set_account_id(0);
    channel_info.set_flags(0);
    channel_info.set_is_public(true);
    channel_info.set_freq_cap_id(0);
    channel_info.set_channel_rate_id(0);
  }

  inline void pack_channel_params(
    ExpressionChannelInfo& channel_info, const ChannelParams& channel_params)
  {
    channel_info.Clear();
    channel_info.set_channel_id(channel_params.channel_id);
    channel_info.set_type(static_cast<unsigned char>(channel_params.type));
    channel_info.set_country_code(channel_params.country);
    channel_info.set_status(static_cast<unsigned char>(channel_params.status));
    channel_info.set_action_id(channel_params.action_id);
    channel_info.set_timestamp(GrpcAlgs::pack_time(channel_params.timestamp));

    if (channel_params.common_params.in())
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

    if (channel_params.descriptive_params.in())
    {
      channel_info.set_name(channel_params.descriptive_params->name);
      channel_info.set_parent_channel_id(channel_params.descriptive_params->parent_channel_id);
    }
    else
    {
      channel_info.set_parent_channel_id(0);
    }

    if (channel_params.discover_params.in())
    {
      channel_info.set_discover_query(channel_params.discover_params->query);
      channel_info.set_discover_annotation(channel_params.discover_params->annotation);
    }

    if (channel_params.cmp_params.in())
    {
      channel_info.set_channel_rate_id(channel_params.cmp_params->channel_rate_id);
      channel_info.set_imp_revenue(GrpcAlgs::pack_decimal(channel_params.cmp_params->imp_revenue));
      channel_info.set_click_revenue(
        GrpcAlgs::pack_decimal(channel_params.cmp_params->click_revenue));
    }
    else
    {
      channel_info.set_channel_rate_id(0);
    }
  }

  inline void unpack_channel_params(
    ChannelParams& channel_params, const ExpressionChannelInfo& channel_info)
  {
    channel_params = ChannelParams();
    channel_params.channel_id = channel_info.channel_id();
    channel_params.type = static_cast<char>(channel_info.type());
    channel_params.country = channel_info.country_code();
    channel_params.status = static_cast<char>(channel_info.status());
    channel_params.action_id = channel_info.action_id();
    channel_params.timestamp = GrpcAlgs::unpack_time(channel_info.timestamp());

    ChannelParams::CommonParams_var common_params(new ChannelParams::CommonParams());
    common_params->account_id = channel_info.account_id();
    common_params->language = channel_info.language();
    common_params->flags = channel_info.flags();
    common_params->is_public = channel_info.is_public();
    common_params->freq_cap_id = channel_info.freq_cap_id();
    channel_params.common_params = common_params;

    if (!channel_info.name().empty() || channel_info.parent_channel_id())
    {
      ChannelParams::DescriptiveParams_var descriptive_params(
        new ChannelParams::DescriptiveParams());
      descriptive_params->name = channel_info.name();
      descriptive_params->parent_channel_id = channel_info.parent_channel_id();
      channel_params.descriptive_params = descriptive_params;
    }

    if (!channel_info.discover_query().empty())
    {
      ChannelParams::DiscoverParams_var discover_params(new ChannelParams::DiscoverParams(
        channel_info.discover_query().c_str(), channel_info.discover_annotation().c_str()));
      channel_params.discover_params = discover_params;
    }

    if (channel_info.channel_rate_id())
    {
      ChannelParams::CMPParams_var cmp_params(
        new ChannelParams::CMPParams(channel_info.channel_rate_id(),
          GrpcAlgs::unpack_decimal<RevenueDecimal>(channel_info.imp_revenue()),
          GrpcAlgs::unpack_decimal<RevenueDecimal>(channel_info.click_revenue())));
      channel_params.cmp_params = cmp_params;
    }
  }

  template<typename ChannelContainerType>
  void unpack_expression(ExpressionChannel::Expression& expression,
    const ExpressionInfo& expression_info, ChannelContainerType& channels)
  {
    expression = ExpressionChannel::Expression();
    expression.op = static_cast<ExpressionChannel::Operation>(expression_info.operation());

    if (expression.op == ExpressionChannel::NOP)
    {
      if (expression_info.channel_id())
      {
        typename ChannelContainerType::iterator ch_it = channels.find(expression_info.channel_id());
        if (ch_it != channels.end())
        {
          expression.channel = ch_it->second;
        }
        else
        {
          ExpressionChannelHolder_var channel_holder(new ExpressionChannelHolder());
          channels.insert(std::make_pair(expression_info.channel_id(), channel_holder));
          expression.channel = channel_holder;
        }
      }
    }
    else
    {
      for (const auto& sub_channel : expression_info.sub_channels())
      {
        ExpressionChannel::Expression sub_expr;
        unpack_expression(sub_expr, sub_channel, channels);
        expression.sub_channels.push_back(sub_expr);
      }
    }
  }

  template<typename ChannelContainerType>
  ExpressionChannelBase_var unpack_channel(
    const ExpressionChannelInfo& channel_info, ChannelContainerType& channels)
  /*throw(eh::Exception)*/
  {
    ChannelParams channel_params;
    unpack_channel_params(channel_params, channel_info);

    if (static_cast<char>(channel_info.expression().operation()) == 'S')
    {
      return new SimpleChannel(channel_params);
    }
    else
    {
      ExpressionChannel::Expression expr;
      unpack_expression(expr, channel_info.expression(), channels);
      return new ExpressionChannel(channel_params, expr);
    }
  }
}
