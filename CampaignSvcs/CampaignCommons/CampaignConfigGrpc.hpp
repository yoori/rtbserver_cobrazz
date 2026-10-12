#pragma once

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignConfigTypes.hpp>

namespace AdServer::CampaignSvcs
{
  template<typename SequenceType, typename ValueType>
  inline void
  fill_interval_sequence(SequenceType& target_seq, const Commons::IntervalSet<ValueType>& source)
    noexcept
  {
    GrpcAlgs::resize_repeated(target_seq, source.size());
    std::uint32_t i = 0;
    for (typename Commons::IntervalSet<ValueType>::const_iterator sit = source.begin();
      sit != source.end(); ++sit, ++i)
    {
      target_seq[i].set_min(sit->min);
      target_seq[i].set_max(sit->max);
    }
  }

  template<typename SequenceType, typename ValueType>
  inline void
  convert_interval_sequence(Commons::IntervalSet<ValueType>& target, const SequenceType& source_seq)
    noexcept
  {
    for (const auto& interval : source_seq)
    {
      target.insert(Commons::Interval<ValueType>(interval.min(), interval.max()));
    }
  }

  inline void pack_delivery_limits(CampaignDeliveryLimitsInfo& delivery_info,
    const CampaignDeliveryLimits& delivery_limits) noexcept
  {
    delivery_info.Clear();
    delivery_info.set_date_start(GrpcAlgs::pack_time(delivery_limits.date_start));
    delivery_info.set_date_end(GrpcAlgs::pack_time(delivery_limits.date_end));
    delivery_info.set_budget(GrpcAlgs::pack_optional_decimal(delivery_limits.budget));
    delivery_info.set_daily_budget(GrpcAlgs::pack_optional_decimal(delivery_limits.daily_budget));
    delivery_info.set_delivery_pacing(static_cast<unsigned char>(delivery_limits.delivery_pacing));

    if (delivery_limits.imps.has_value())
    {
      delivery_info.set_imps_defined(true);
      delivery_info.set_imps(*delivery_limits.imps);
    }

    if (delivery_limits.clicks.has_value())
    {
      delivery_info.set_clicks_defined(true);
      delivery_info.set_clicks(*delivery_limits.clicks);
    }
  }

  inline void unpack_delivery_limits(CampaignDeliveryLimits& delivery_limits,
    const CampaignDeliveryLimitsInfo& delivery_info) noexcept
  {
    delivery_limits.date_start = GrpcAlgs::unpack_time(delivery_info.date_start());
    delivery_limits.date_end = GrpcAlgs::unpack_time(delivery_info.date_end());
    delivery_limits.budget =
      GrpcAlgs::unpack_optional_decimal<RevenueDecimal>(delivery_info.budget());
    delivery_limits.daily_budget =
      GrpcAlgs::unpack_optional_decimal<RevenueDecimal>(delivery_info.daily_budget());
    delivery_limits.delivery_pacing = static_cast<char>(delivery_info.delivery_pacing());

    delivery_limits.imps = delivery_info.imps_defined()
      ? Commons::Optional<unsigned long>(delivery_info.imps())
      : Commons::Optional<unsigned long>();

    delivery_limits.daily_imps = Commons::Optional<unsigned long>();
    delivery_limits.clicks = delivery_info.clicks_defined()
      ? Commons::Optional<unsigned long>(delivery_info.clicks())
      : Commons::Optional<unsigned long>();

    delivery_limits.daily_clicks = Commons::Optional<unsigned long>();
  }

  inline void pack_option_value_map(OptionValueSeq& token_seq, const OptionValueMap& tokens)
    noexcept
  {
    GrpcAlgs::resize_repeated(token_seq, tokens.size());
    std::uint32_t i = 0;
    for (OptionValueMap::const_iterator token_it = tokens.begin(); token_it != tokens.end();
      ++token_it, ++i)
    {
      token_seq[i].set_option_id(token_it->first);
      token_seq[i].set_value(token_it->second);
    }
  }

  inline void unpack_option_value_map(OptionValueMap& tokens, const OptionValueSeq& token_seq)
    noexcept
  {
    for (const auto& token : token_seq)
    {
      tokens[token.option_id()] = token.value();
    }
  }
}
