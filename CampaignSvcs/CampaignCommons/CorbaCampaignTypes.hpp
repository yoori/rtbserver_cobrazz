#ifndef _CORBA_CAMPAIGN_TYPES_HPP_
#define _CORBA_CAMPAIGN_TYPES_HPP_

#include <optional>

#include "CampaignTypes.hpp"
#include <CampaignSvcs/CampaignCommons/CampaignCommons.hpp>
#include "CampaignSvcsVersionAdapter.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    template<typename SequenceType, typename ValueType>
    void
    fill_interval_sequence(
      SequenceType& target_seq,
      const Commons::IntervalSet<ValueType>& source)
      noexcept;

    template<typename SequenceType, typename ValueType>
    void
    convert_interval_sequence(
      Commons::IntervalSet<ValueType>& target,
      const SequenceType& source_seq)
      noexcept;

    void
    pack_delivery_limits(
      CampaignDeliveryLimitsInfo& delivery_info,
      const CampaignDeliveryLimits& delivery_limits)
      noexcept;

    void
    unpack_delivery_limits(
      CampaignDeliveryLimits& delivery_limits,
      const CampaignDeliveryLimitsInfo& delivery_info)
      noexcept;

    void
    pack_option_value_map(
      OptionValueSeq& token_seq,
      const OptionValueMap& tokens)
      noexcept;

    void
    unpack_option_value_map(
      OptionValueMap& tokens,
      const OptionValueSeq& token_seq)
      noexcept;
  }
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    template<typename SequenceType, typename ValueType>
    inline
    void
    fill_interval_sequence(
      SequenceType& target_seq,
      const Commons::IntervalSet<ValueType>& source)
      noexcept
    {
      target_seq.length(source.size());
      CORBA::ULong i = 0;
      for(typename Commons::IntervalSet<ValueType>::const_iterator sit =
            source.begin(); sit != source.end(); ++sit, ++i)
      {
        target_seq[i].min = sit->min;
        target_seq[i].max = sit->max;
      }
    }

    template<typename SequenceType, typename ValueType>
    inline
    void
    convert_interval_sequence(
      Commons::IntervalSet<ValueType>& target,
      const SequenceType& source_seq)
      noexcept
    {
      for(CORBA::ULong i = 0; i < source_seq.length(); ++i)
      {
        target.insert(Commons::Interval<ValueType>(
          source_seq[i].min, source_seq[i].max));
      }
    }

    inline
    void
    pack_delivery_limits(
      CampaignDeliveryLimitsInfo& delivery_info,
      const CampaignDeliveryLimits& delivery_limits)
      noexcept
    {
      delivery_info.date_start = CorbaAlgs::pack_time(delivery_limits.date_start);
      delivery_info.date_end = CorbaAlgs::pack_time(delivery_limits.date_end);
      delivery_info.budget = CorbaAlgs::pack_optional_decimal(
        delivery_limits.budget);
      delivery_info.daily_budget = CorbaAlgs::pack_optional_decimal(
        delivery_limits.daily_budget);
      delivery_info.delivery_pacing = delivery_limits.delivery_pacing;

      if(delivery_limits.imps.has_value())
      {
        delivery_info.imps_defined = true;
        delivery_info.imps = *delivery_limits.imps;
      }
      else
      {
        delivery_info.imps_defined = false;
      }

      /*
      if(delivery_limits.daily_imps.has_value())
      {
        delivery_info.daily_imps_defined = true;
        delivery_info.daily_imps = *delivery_limits.daily_imps;
      }
      else
      {
        delivery_info.daily_imps_defined = false;
      }
      */

      if(delivery_limits.clicks.has_value())
      {
        delivery_info.clicks_defined = true;
        delivery_info.clicks = *delivery_limits.clicks;
      }
      else
      {
        delivery_info.clicks_defined = false;
      }

      /*
      if(delivery_limits.daily_clicks.has_value())
      {
        delivery_info.daily_clicks_defined = true;
        delivery_info.daily_clicks = *delivery_limits.daily_clicks;
      }
      else
      {
        delivery_info.daily_clicks_defined = false;
      }
      */
    }

    inline
    void
    unpack_delivery_limits(
      CampaignDeliveryLimits& delivery_limits,
      const CampaignDeliveryLimitsInfo& delivery_info)
      noexcept
    {
      delivery_limits.date_start = CorbaAlgs::unpack_time(delivery_info.date_start);
      delivery_limits.date_end = CorbaAlgs::unpack_time(delivery_info.date_end);
      delivery_limits.budget = CorbaAlgs::unpack_optional_decimal<RevenueDecimal>(
        delivery_info.budget);
      delivery_limits.daily_budget = CorbaAlgs::unpack_optional_decimal<RevenueDecimal>(
        delivery_info.daily_budget);
      delivery_limits.delivery_pacing = delivery_info.delivery_pacing;

      delivery_limits.imps = delivery_info.imps_defined ?
        std::optional<unsigned long>(delivery_info.imps) :
        std::nullopt;

      delivery_limits.daily_imps = std::optional<unsigned long>();
      /*
      delivery_limits.daily_imps = delivery_info.daily_imps_defined ?
        std::optional<unsigned long>(delivery_info.daily_imps) :
        std::optional<unsigned long>();
      */
      delivery_limits.clicks = delivery_info.clicks_defined ?
        std::optional<unsigned long>(delivery_info.clicks) :
        std::nullopt;

      delivery_limits.daily_clicks = std::optional<unsigned long>();
      /*
      delivery_limits.daily_clicks = delivery_info.daily_clicks_defined ?
        std::optional<unsigned long>(delivery_info.daily_clicks) :
        std::optional<unsigned long>();
      */
    }

    inline
    void
    pack_option_value_map(
      OptionValueSeq& token_seq,
      const OptionValueMap& tokens)
      noexcept
    {
      token_seq.length(tokens.size());
      CORBA::ULong i = 0;
      for(OptionValueMap::const_iterator token_it = tokens.begin();
          token_it != tokens.end(); ++token_it, ++i)
      {
        token_seq[i].option_id = token_it->first;
        token_seq[i].value << token_it->second;
      }
    }

    inline
    void
    unpack_option_value_map(
      OptionValueMap& tokens,
      const OptionValueSeq& token_seq)
      noexcept
    {
      for(CORBA::ULong i = 0; i < token_seq.length(); ++i)
      {
        tokens[token_seq[i].option_id] = token_seq[i].value.in();
      }
    }
  }
}

#endif /*_CORBA_CAMPAIGN_TYPES_HPP_*/
