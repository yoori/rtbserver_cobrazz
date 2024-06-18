#ifndef _GRPC_CAMPAIGN_TYPES_HPP_
#define _GRPC_CAMPAIGN_TYPES_HPP_

#include "CampaignTypes.hpp"
#include "CampaignManager.pb.h"
#include "CampaignSvcsVersionAdapter.hpp"
#include <Commons/GrpcAlgs.hpp>

namespace AdServer::CampaignSvcs
{

template<class T, class H>
inline void fill_interval_sequence(
  google::protobuf::RepeatedPtrField<T>& target_seq,
  const Commons::IntervalSet<H>& source)
{
  target_seq.Reserve(source.size());
  for(auto it = source.begin(); it != source.end(); ++it)
  {
    auto* element = target_seq.Add();
    element->set_min(it->min);
    element->set_max(it->max);
  }
}

inline void pack_delivery_limits(
  Proto::CampaignDeliveryLimitsInfo& delivery_info,
  const CampaignDeliveryLimits& delivery_limits)
{
  delivery_info.set_date_start(GrpcAlgs::pack_time(delivery_limits.date_start));
  delivery_info.set_date_end(GrpcAlgs::pack_time(delivery_limits.date_end));
  delivery_info.set_budget(GrpcAlgs::pack_optional_decimal(
    delivery_limits.budget));
  delivery_info.set_daily_budget(GrpcAlgs::pack_optional_decimal(
    delivery_limits.daily_budget));
  delivery_info.set_delivery_pacing(delivery_limits.delivery_pacing);

  if(delivery_limits.imps.has_value())
  {
    delivery_info.set_imps_defined(true);
    delivery_info.set_imps(*delivery_limits.imps);
  }
  else
  {
    delivery_info.set_imps_defined(false);
  }

  if(delivery_limits.clicks.has_value())
  {
    delivery_info.set_clicks_defined(true);
    delivery_info.set_clicks(*delivery_limits.clicks);
  }
  else
  {
    delivery_info.set_clicks_defined(false);
  }
}

} // namespace AdServer::CampaignSvcs

#endif //_GRPC_CAMPAIGN_TYPES_HPP_
