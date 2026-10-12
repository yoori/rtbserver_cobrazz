#pragma once

#include <cstdint>
#include <string>
#include <CampaignSvcs/CampaignServer/CampaignServer.pb.h>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/WireTypes.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignConfigTypes.hpp>
#include <stdexcept>
#include <CampaignSvcs/CampaignServer/StatTypes.hpp>
#include <CampaignSvcs/CampaignServer/BillStatTypes.hpp>

namespace AdServer::CampaignSvcs
{
  using DiscoverChannel = adserver::campaign::server::DiscoverChannel;

  using DiscoverChannelSeq = google::protobuf::RepeatedPtrField<DiscoverChannel>;

  using SimpleChannelAnswer = adserver::campaign::server::SimpleChannelAnswer;

  using BriefSimpleChannelAnswer = adserver::campaign::server::BriefSimpleChannelAnswer;

  using BehaveInfo = adserver::campaign::server::BehaveInfo;

  using BehaveInfoSeq = google::protobuf::RepeatedPtrField<BehaveInfo>;

  using CSSimpleChannel = adserver::campaign::server::CSSimpleChannel;

  using CSSimpleChannelSeq = google::protobuf::RepeatedPtrField<CSSimpleChannel>;

  using ChannelServerChannelAnswer = adserver::campaign::server::ChannelServerChannelAnswer;

  using CampaignGetConfigSettings = adserver::campaign::server::CampaignGetConfigSettings;

  using FraudConditionConfig = adserver::campaign::server::FraudConditionConfig;

  using DetectorsConfig = adserver::campaign::server::DetectorsConfig;

  using FreqCapConfigInfo = adserver::campaign::server::FreqCapConfigInfo;

  using ExpressionChannelCCGInfo = adserver::campaign::server::ExpressionChannelCCGInfo;

  using ExpressionChannelCCGSeq = google::protobuf::RepeatedPtrField<ExpressionChannelCCGInfo>;

  using ChannelTriggersInfo = adserver::campaign::server::ChannelTriggersInfo;

  using ChannelTriggersSeq = google::protobuf::RepeatedPtrField<ChannelTriggersInfo>;

  using ExpressionChannelsInfo = adserver::campaign::server::ExpressionChannelsInfo;

  using AccountDeliveryInfo = adserver::campaign::server::AccountDeliveryInfo;

  using AccountDeliverySeq = google::protobuf::RepeatedPtrField<AccountDeliveryInfo>;

  using CampaignDeliveryInfo = adserver::campaign::server::CampaignDeliveryInfo;

  using CampaignDeliverySeq = google::protobuf::RepeatedPtrField<CampaignDeliveryInfo>;

  using CCGDeliveryInfo = adserver::campaign::server::CCGDeliveryInfo;

  using CCGDeliverySeq = google::protobuf::RepeatedPtrField<CCGDeliveryInfo>;

  namespace CampaignServer
  {
    class ImplementationException : public std::runtime_error
    {
    public:
      using std::runtime_error::runtime_error;
    };

    class NotReady : public std::runtime_error
    {
    public:
      using std::runtime_error::runtime_error;
    };

    class NotSupport : public std::runtime_error
    {
    public:
      using std::runtime_error::runtime_error;
    };

    using GetExpressionChannelsInfo = adserver::campaign::server::GetExpressionChannelsInfo;

    using GetSimpleChannelsInfo = adserver::campaign::server::GetSimpleChannelsInfo;

    using PassbackInfo = adserver::campaign::server::PassbackInfo;

    using ColocationPropInfo = adserver::campaign::server::ColocationPropInfo;

    using DeliveryLimitConfigInfo = adserver::campaign::server::DeliveryLimitConfigInfo;
  }
}
