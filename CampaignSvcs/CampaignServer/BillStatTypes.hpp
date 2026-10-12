#pragma once

#include <cstdint>
#include <string>
#include <CampaignSvcs/CampaignServer/CampaignServer.pb.h>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/WireTypes.hpp>

namespace AdServer::CampaignSvcs
{
  using DayAmount = adserver::campaign::server::DayAmount;

  using DayAmountSeq = google::protobuf::RepeatedPtrField<DayAmount>;

  using AmountDistributionInfo = adserver::campaign::server::AmountDistributionInfo;

  using AmountDistributionSeq = google::protobuf::RepeatedPtrField<AmountDistributionInfo>;

  using AccountAmountDistributionInfo = adserver::campaign::server::AccountAmountDistributionInfo;

  using AccountAmountDistributionSeq =
    google::protobuf::RepeatedPtrField<AccountAmountDistributionInfo>;

  using DayAmountCount = adserver::campaign::server::DayAmountCount;

  using DayAmountCountSeq = google::protobuf::RepeatedPtrField<DayAmountCount>;

  using AmountCountDistributionInfo = adserver::campaign::server::AmountCountDistributionInfo;

  using CampaignAmountDistributionInfo = adserver::campaign::server::CampaignAmountDistributionInfo;

  using CampaignAmountDistributionSeq =
    google::protobuf::RepeatedPtrField<CampaignAmountDistributionInfo>;

  using CCGAmountDistributionInfo = adserver::campaign::server::CCGAmountDistributionInfo;

  using CCGAmountDistributionSeq = google::protobuf::RepeatedPtrField<CCGAmountDistributionInfo>;

  using BillStatInfo = adserver::campaign::server::BillStatInfo;
}
