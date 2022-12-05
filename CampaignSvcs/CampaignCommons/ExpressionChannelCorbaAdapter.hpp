#ifndef _EXPRESSION_CHANNEL_CORBA_ADAPTER_HPP_
#define _EXPRESSION_CHANNEL_CORBA_ADAPTER_HPP_

#include <Commons/CorbaAlgs.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignCommons.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include "ExpressionChannel.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    enum ExpressionChannelPackDetails
    {
      ECPD_ALL = 0xFF,
      ECPD_COMMON_PARAMS = 1,
      ECPD_DISCOVER_PARAMS = 2,
      ECPD_CMP_PARAMS = 4
    };

    inline
    void pack_platform_channel_params(
      ExpressionChannelInfo& channel_info,
      unsigned long channel_id,
      const Generics::Time& timestamp)
    {
      channel_info.channel_id = channel_id;
      channel_info.type = 'V';
      channel_info.status = 'A';
      channel_info.action_id = 0;
      channel_info.timestamp = CorbaAlgs::pack_time(timestamp);

      channel_info.account_id = 0;
      channel_info.flags = 0;
      channel_info.is_public = true;
      channel_info.freq_cap_id = 0;
      channel_info.channel_rate_id = 0;
    }

    inline
    void pack_channel_params(
      ExpressionChannelInfo& channel_info,
      const ChannelParams& channel_params)
    {
      channel_info.channel_id = channel_params.channel_id;
      channel_info.type = channel_params.type;
      channel_info.country_code << channel_params.country;
      channel_info.status = channel_params.status;
      channel_info.action_id = channel_params.action_id;
      channel_info.timestamp = CorbaAlgs::pack_time(channel_params.timestamp);

      if(channel_params.common_params.in())
      {
        channel_info.account_id = channel_params.common_params->account_id;
        channel_info.flags = channel_params.common_params->flags;
        channel_info.is_public = channel_params.common_params->is_public;
        channel_info.freq_cap_id = channel_params.common_params->freq_cap_id;
        channel_info.language << channel_params.common_params->language;
      }
      else
      {
        channel_info.account_id = 0;
        channel_info.flags = 0;
        channel_info.is_public = true;
        channel_info.freq_cap_id = 0;
      }

      if(channel_params.descriptive_params.in())
      {
        channel_info.name << channel_params.descriptive_params->name;
        channel_info.parent_channel_id = channel_params.descriptive_params->parent_channel_id;
      }
      else
      {
        channel_info.parent_channel_id = 0;
      }

      if(channel_params.discover_params.in())
      {
        channel_info.discover_query << channel_params.discover_params->query;
        channel_info.discover_annotation << channel_params.discover_params->annotation;
      }

      if(channel_params.cmp_params.in())
      {
        channel_info.channel_rate_id = channel_params.cmp_params->channel_rate_id;
        channel_info.imp_revenue = CorbaAlgs::pack_decimal(
          channel_params.cmp_params->imp_revenue);
        channel_info.click_revenue = CorbaAlgs::pack_decimal(
          channel_params.cmp_params->click_revenue);
      }
      else
      {
        channel_info.channel_rate_id = 0;
      }
    }

    inline
    void unpack_channel_params(
      ChannelParams& channel_params,
      const ExpressionChannelInfo& channel_info)
    {
      channel_params.channel_id = channel_info.channel_id;
      channel_params.type = channel_info.type;
      channel_params.country = channel_info.country_code;
      channel_params.status = channel_info.status;
      channel_params.action_id = channel_info.action_id;
      channel_params.timestamp = CorbaAlgs::unpack_time(channel_info.timestamp);

      ChannelParams::CommonParams_var common_params(
        new ChannelParams::CommonParams());
      common_params->account_id = channel_info.account_id;
      common_params->language = channel_info.language;
      common_params->flags = channel_info.flags;
      common_params->is_public = channel_info.is_public;
      common_params->freq_cap_id = channel_info.freq_cap_id;
      channel_params.common_params = common_params;

      if(channel_info.name[0] || channel_info.parent_channel_id)
      {
        ChannelParams::DescriptiveParams_var descriptive_params(
          new ChannelParams::DescriptiveParams());
        descriptive_params->name = channel_info.name;
        descriptive_params->parent_channel_id = channel_info.parent_channel_id;
        channel_params.descriptive_params = descriptive_params;
      }

      if(channel_info.discover_query[0])
      {
        ChannelParams::DiscoverParams_var discover_params(
          new ChannelParams::DiscoverParams(
            channel_info.discover_query,
            channel_info.discover_annotation));
        channel_params.discover_params = discover_params;
      }

      if(channel_info.channel_rate_id)
      {
        ChannelParams::CMPParams_var cmp_params(
          new ChannelParams::CMPParams(
            channel_info.channel_rate_id,
            CorbaAlgs::unpack_decimal<RevenueDecimal>(channel_info.imp_revenue),
            CorbaAlgs::unpack_decimal<RevenueDecimal>(channel_info.click_revenue)));
        channel_params.cmp_params = cmp_params;
      }
    }

    inline
    void pack_simple_channel(
      ExpressionChannelInfo& channel_info,
      const SimpleChannel* simple_channel)
      /*throw(eh::Exception)*/
    {
      pack_channel_params(channel_info, simple_channel->params());

      channel_info.expression.operation = 'S';
//    channel_info.threshold = simple_channel->threshold();
    }

    inline
    void pack_expression(
      ExpressionInfo& expression_info,
      const ExpressionChannel::Expression& expression)
      noexcept
    {
      expression_info.operation = expression.op;
      if(expression.op == ExpressionChannel::NOP)
      {
        if(expression.channel.in() &&
           (expression.channel->expression_channel().in() ||
            expression.channel->simple_channel().in()))
        {
          expression_info.channel_id = expression.channel->params().channel_id;
        }
        else
        {
          expression_info.channel_id = 0;
        }
      }
      else
      {
        expression_info.sub_channels.length(expression.sub_channels.size());
        CORBA::ULong ei = 0;
        for(ExpressionChannel::Expression::ExpressionList::const_iterator eit =
              expression.sub_channels.begin();
            eit != expression.sub_channels.end(); ++eit, ++ei)
        {
          pack_expression(expression_info.sub_channels[ei], *eit);
        }
      }
    }

    template<typename ChannelContainerType>
    void unpack_expression(
      ExpressionChannel::Expression& expression,
      const ExpressionInfo& expression_info,
      ChannelContainerType& channels)
    {
      expression.op = static_cast<ExpressionChannel::Operation>(
        expression_info.operation);

      if(expression.op == ExpressionChannel::NOP)
      {
        if(expression_info.channel_id)
        {
          typename ChannelContainerType::iterator ch_it =
            channels.find(expression_info.channel_id);
          if(ch_it != channels.end())
          {
            expression.channel = ch_it->second;
          }
          else
          {
            ExpressionChannelHolder_var channel_holder(
              new ExpressionChannelHolder());
            channels.insert(std::make_pair(
              expression_info.channel_id, channel_holder));
            expression.channel = channel_holder;
          }
        }
      }
      else
      {
        for(CORBA::ULong ei = 0; ei < expression_info.sub_channels.length(); ++ei)
        {
          ExpressionChannel::Expression sub_expr;
          unpack_expression(sub_expr, expression_info.sub_channels[ei], channels);
          expression.sub_channels.push_back(sub_expr);
        }
      }
    }

    inline
    void pack_expression_channel(
      ExpressionChannelInfo& channel_info,
      const ExpressionChannel* expression_channel)
      noexcept
    {
      pack_channel_params(channel_info, expression_channel->params());

      const ExpressionChannel::Expression& expr = expression_channel->expression();
      pack_expression(channel_info.expression, expr);
    }

    inline
    void pack_channel(
      ExpressionChannelInfo& channel_info,
      const ExpressionChannelBase* channel)
      noexcept
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

    template<typename ChannelContainerType>
    ExpressionChannelBase_var unpack_channel(
      const ExpressionChannelInfo& channel_info,
      ChannelContainerType& channels)
      /*throw(eh::Exception)*/
    {
      ChannelParams channel_params;
      unpack_channel_params(channel_params, channel_info);
      
      if(channel_info.expression.operation == 'S')
      {
        return new SimpleChannel(channel_params);
      }
      else
      {
        ExpressionChannel::Expression expr;
        unpack_expression(expr, channel_info.expression, channels);
        return new ExpressionChannel(channel_params, expr);
      }
    }
  }
}

#endif /*_EXPRESSION_CHANNEL_CORBA_ADAPTER_HPP_*/
