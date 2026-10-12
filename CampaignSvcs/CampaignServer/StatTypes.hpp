#pragma once

#include <cstdint>
#include <string>
#include <CampaignSvcs/CampaignServer/CampaignServer.pb.h>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/WireTypes.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignConfigTypes.hpp>

namespace AdServer::CampaignSvcs
{
  using CreativeStatInfo = adserver::campaign::server::CreativeStatInfo;

  using CreativeStatSeq = google::protobuf::RepeatedPtrField<CreativeStatInfo>;

  using PublisherCCGStatInfo = adserver::campaign::server::PublisherCCGStatInfo;

  using PublisherCCGStatSeq = google::protobuf::RepeatedPtrField<PublisherCCGStatInfo>;

  using TagCCGStatInfo = adserver::campaign::server::TagCCGStatInfo;

  using TagCCGStatSeq = google::protobuf::RepeatedPtrField<TagCCGStatInfo>;

  using CtrResetStatInfo = adserver::campaign::server::CtrResetStatInfo;

  using CtrResetStatSeq = google::protobuf::RepeatedPtrField<CtrResetStatInfo>;

  using CCGStatInfo = adserver::campaign::server::CCGStatInfo;

  using CCGStatSeq = google::protobuf::RepeatedPtrField<CCGStatInfo>;

  using CampaignStatInfo = adserver::campaign::server::CampaignStatInfo;

  using AmountStatInfo = adserver::campaign::server::AmountStatInfo;

  using AmountStatSeq = google::protobuf::RepeatedPtrField<AmountStatInfo>;

  using AccountStatSeq = AmountStatSeq;

  using CampaignStatSeq = google::protobuf::RepeatedPtrField<CampaignStatInfo>;

  using StatInfo = adserver::campaign::server::StatInfo;
}
