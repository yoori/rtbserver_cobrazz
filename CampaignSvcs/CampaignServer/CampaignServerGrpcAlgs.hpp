#pragma once

#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.pb.h>

namespace AdServer::CampaignSvcs::CampaignGrpc
{
  namespace rpc = adserver::campaign::server;

  void pack(const AppFormatInfo& source, rpc::AppFormatInfo& target);
  void unpack(const rpc::AppFormatInfo& source, AppFormatInfo& target);

  void pack(const SizeInfo& source, rpc::SizeInfo& target);
  void unpack(const rpc::SizeInfo& source, SizeInfo& target);

  void pack(const CreativeOptionInfo& source, rpc::CreativeOptionInfo& target);
  void unpack(const rpc::CreativeOptionInfo& source, CreativeOptionInfo& target);

  void pack(const OptionValueInfo& source, rpc::OptionValueInfo& target);
  void unpack(const rpc::OptionValueInfo& source, OptionValueInfo& target);

  void pack(const CountryInfo& source, rpc::CountryInfo& target);
  void unpack(const rpc::CountryInfo& source, CountryInfo& target);

  void pack(const ExternalCategoryInfo& source, rpc::ExternalCategoryInfo& target);
  void unpack(const rpc::ExternalCategoryInfo& source, ExternalCategoryInfo& target);

  void pack(const CreativeCategoryInfo& source, rpc::CreativeCategoryInfo& target);
  void unpack(const rpc::CreativeCategoryInfo& source, CreativeCategoryInfo& target);

  void pack(const CreativeSizeInfo& source, rpc::CreativeSizeInfo& target);
  void unpack(const rpc::CreativeSizeInfo& source, CreativeSizeInfo& target);

  void pack(const CreativeInfo& source, rpc::CreativeInfo& target);
  void unpack(const rpc::CreativeInfo& source, CreativeInfo& target);

  void pack(const ChannelWeight& source, rpc::ChannelWeight& target);
  void unpack(const rpc::ChannelWeight& source, ChannelWeight& target);

  void pack(const DeletedIdInfo& source, rpc::DeletedIdInfo& target);
  void unpack(const rpc::DeletedIdInfo& source, DeletedIdInfo& target);

  void pack(const DeletedStringIdInfo& source, rpc::DeletedStringIdInfo& target);
  void unpack(const rpc::DeletedStringIdInfo& source, DeletedStringIdInfo& target);

  void pack(const CampaignEcpmInfo& source, rpc::CampaignEcpmInfo& target);
  void unpack(const rpc::CampaignEcpmInfo& source, CampaignEcpmInfo& target);

  void pack(const ExpressionInfo& source, rpc::ExpressionInfo& target);
  void unpack(const rpc::ExpressionInfo& source, ExpressionInfo& target);

  void pack(const ExpressionChannelInfo& source, rpc::ExpressionChannelInfo& target);
  void unpack(const rpc::ExpressionChannelInfo& source, ExpressionChannelInfo& target);

  void pack(const BehavioralParameterInfo& source, rpc::BehavioralParameterInfo& target);
  void unpack(const rpc::BehavioralParameterInfo& source, BehavioralParameterInfo& target);

  void pack(const WeeklyRunInterval& source, rpc::WeeklyRunInterval& target);
  void unpack(const rpc::WeeklyRunInterval& source, WeeklyRunInterval& target);

  void pack(const CampaignDeliveryLimitsInfo& source, rpc::CampaignDeliveryLimitsInfo& target);
  void unpack(const rpc::CampaignDeliveryLimitsInfo& source, CampaignDeliveryLimitsInfo& target);

  void pack(const TagDeliveryInfo& source, rpc::TagDeliveryInfo& target);
  void unpack(const rpc::TagDeliveryInfo& source, TagDeliveryInfo& target);

  void pack(const ContractInfo& source, rpc::ContractInfo& target);
  void unpack(const rpc::ContractInfo& source, ContractInfo& target);

  void pack(const CampaignInfo& source, rpc::CampaignInfo& target);
  void unpack(const rpc::CampaignInfo& source, CampaignInfo& target);

  void pack(const TagPricingInfo& source, rpc::TagPricingInfo& target);
  void unpack(const rpc::TagPricingInfo& source, TagPricingInfo& target);

  void pack(const TemplateOptionsInfo& source, rpc::TemplateOptionsInfo& target);
  void unpack(const rpc::TemplateOptionsInfo& source, TemplateOptionsInfo& target);

  void pack(const TagSizeInfo& source, rpc::TagSizeInfo& target);
  void unpack(const rpc::TagSizeInfo& source, TagSizeInfo& target);

  void pack(const TagInfo& source, rpc::TagInfo& target);
  void unpack(const rpc::TagInfo& source, TagInfo& target);

  void pack(const SiteInfo& source, rpc::SiteInfo& target);
  void unpack(const rpc::SiteInfo& source, SiteInfo& target);

  void pack(const ColocationInfo& source, rpc::ColocationInfo& target);
  void unpack(const rpc::ColocationInfo& source, ColocationInfo& target);

  void pack(const ColocationFlags& source, rpc::ColocationFlags& target);
  void unpack(const rpc::ColocationFlags& source, ColocationFlags& target);

  void pack(const FreqCapInfo& source, rpc::FreqCapInfo& target);
  void unpack(const rpc::FreqCapInfo& source, FreqCapInfo& target);

  void pack(const CreativeTemplateFileInfo& source, rpc::CreativeTemplateFileInfo& target);
  void unpack(const rpc::CreativeTemplateFileInfo& source, CreativeTemplateFileInfo& target);

  void pack(const CreativeTemplateInfo& source, rpc::CreativeTemplateInfo& target);
  void unpack(const rpc::CreativeTemplateInfo& source, CreativeTemplateInfo& target);

  void pack(const CurrencyInfo& source, rpc::CurrencyInfo& target);
  void unpack(const rpc::CurrencyInfo& source, CurrencyInfo& target);

  void pack(const CampaignKeywordInfo& source, rpc::CampaignKeywordInfo& target);
  void unpack(const rpc::CampaignKeywordInfo& source, CampaignKeywordInfo& target);

  void pack(const AdvActionInfo& source, rpc::AdvActionInfo& target);
  void unpack(const rpc::AdvActionInfo& source, AdvActionInfo& target);

  void pack(const AccountInfo& source, rpc::AccountInfo& target);
  void unpack(const rpc::AccountInfo& source, AccountInfo& target);

  void pack(
    const CategoryChannelLocalizationInfo& source, rpc::CategoryChannelLocalizationInfo& target);
  void unpack(
    const rpc::CategoryChannelLocalizationInfo& source, CategoryChannelLocalizationInfo& target);

  void pack(const CategoryChannelInfo& source, rpc::CategoryChannelInfo& target);
  void unpack(const rpc::CategoryChannelInfo& source, CategoryChannelInfo& target);

  void pack(const PriceRange& source, rpc::PriceRange& target);
  void unpack(const rpc::PriceRange& source, PriceRange& target);

  void pack(const BehavParameter& source, rpc::BehavParameter& target);
  void unpack(const rpc::BehavParameter& source, BehavParameter& target);

  void pack(const BehavParamInfo& source, rpc::BehavParamInfo& target);
  void unpack(const rpc::BehavParamInfo& source, BehavParamInfo& target);

  void pack(const KeyBehavParamInfo& source, rpc::KeyBehavParamInfo& target);
  void unpack(const rpc::KeyBehavParamInfo& source, KeyBehavParamInfo& target);

  void pack(const BriefBehavParamInfo& source, rpc::BriefBehavParamInfo& target);
  void unpack(const rpc::BriefBehavParamInfo& source, BriefBehavParamInfo& target);

  void pack(const BriefKeyBehavParamInfo& source, rpc::BriefKeyBehavParamInfo& target);
  void unpack(const rpc::BriefKeyBehavParamInfo& source, BriefKeyBehavParamInfo& target);

  void pack(const SimpleChannelKey& source, rpc::SimpleChannelKey& target);
  void unpack(const rpc::SimpleChannelKey& source, SimpleChannelKey& target);

  void pack(const BriefSimpleChannelKey& source, rpc::BriefSimpleChannelKey& target);
  void unpack(const rpc::BriefSimpleChannelKey& source, BriefSimpleChannelKey& target);

  void pack(const GeoIPRegionCityInfo& source, rpc::GeoIPRegionCityInfo& target);
  void unpack(const rpc::GeoIPRegionCityInfo& source, GeoIPRegionCityInfo& target);

  void pack(const GeoChannelInfo& source, rpc::GeoChannelInfo& target);
  void unpack(const rpc::GeoChannelInfo& source, GeoChannelInfo& target);

  void pack(const GeoCoordChannelInfo& source, rpc::GeoCoordChannelInfo& target);
  void unpack(const rpc::GeoCoordChannelInfo& source, GeoCoordChannelInfo& target);

  void pack(const BlockChannelInfo& source, rpc::BlockChannelInfo& target);
  void unpack(const rpc::BlockChannelInfo& source, BlockChannelInfo& target);

  void pack(const FraudConditionInfo& source, rpc::FraudConditionInfo& target);
  void unpack(const rpc::FraudConditionInfo& source, FraudConditionInfo& target);

  void pack(const SearchEngineRegExpInfo& source, rpc::SearchEngineRegExpInfo& target);
  void unpack(const rpc::SearchEngineRegExpInfo& source, SearchEngineRegExpInfo& target);

  void pack(const SearchEngineInfo& source, rpc::SearchEngineInfo& target);
  void unpack(const rpc::SearchEngineInfo& source, SearchEngineInfo& target);

  void pack(const WebBrowserDetectorInfo& source, rpc::WebBrowserDetectorInfo& target);
  void unpack(const rpc::WebBrowserDetectorInfo& source, WebBrowserDetectorInfo& target);

  void pack(const WebBrowserInfo& source, rpc::WebBrowserInfo& target);
  void unpack(const rpc::WebBrowserInfo& source, WebBrowserInfo& target);

  void pack(const PlatformDetectorInfo& source, rpc::PlatformDetectorInfo& target);
  void unpack(const rpc::PlatformDetectorInfo& source, PlatformDetectorInfo& target);

  void pack(const PlatformInfo& source, rpc::PlatformInfo& target);
  void unpack(const rpc::PlatformInfo& source, PlatformInfo& target);

  void pack(const WebOperationInfo& source, rpc::WebOperationInfo& target);
  void unpack(const rpc::WebOperationInfo& source, WebOperationInfo& target);

  void pack(const CampaignConfigUpdateInfo& source, rpc::CampaignConfigUpdateInfo& target);
  void unpack(const rpc::CampaignConfigUpdateInfo& source, CampaignConfigUpdateInfo& target);

  void pack(const CreativeStatInfo& source, rpc::CreativeStatInfo& target);
  void unpack(const rpc::CreativeStatInfo& source, CreativeStatInfo& target);

  void pack(const PublisherCCGStatInfo& source, rpc::PublisherCCGStatInfo& target);
  void unpack(const rpc::PublisherCCGStatInfo& source, PublisherCCGStatInfo& target);

  void pack(const TagCCGStatInfo& source, rpc::TagCCGStatInfo& target);
  void unpack(const rpc::TagCCGStatInfo& source, TagCCGStatInfo& target);

  void pack(const CtrResetStatInfo& source, rpc::CtrResetStatInfo& target);
  void unpack(const rpc::CtrResetStatInfo& source, CtrResetStatInfo& target);

  void pack(const CCGStatInfo& source, rpc::CCGStatInfo& target);
  void unpack(const rpc::CCGStatInfo& source, CCGStatInfo& target);

  void pack(const CampaignStatInfo& source, rpc::CampaignStatInfo& target);
  void unpack(const rpc::CampaignStatInfo& source, CampaignStatInfo& target);

  void pack(const AmountStatInfo& source, rpc::AmountStatInfo& target);
  void unpack(const rpc::AmountStatInfo& source, AmountStatInfo& target);

  void pack(const StatInfo& source, rpc::StatInfo& target);
  void unpack(const rpc::StatInfo& source, StatInfo& target);

  void pack(const DayAmount& source, rpc::DayAmount& target);
  void unpack(const rpc::DayAmount& source, DayAmount& target);

  void pack(const AmountDistributionInfo& source, rpc::AmountDistributionInfo& target);
  void unpack(const rpc::AmountDistributionInfo& source, AmountDistributionInfo& target);

  void pack(
    const AccountAmountDistributionInfo& source, rpc::AccountAmountDistributionInfo& target);
  void unpack(
    const rpc::AccountAmountDistributionInfo& source, AccountAmountDistributionInfo& target);

  void pack(const DayAmountCount& source, rpc::DayAmountCount& target);
  void unpack(const rpc::DayAmountCount& source, DayAmountCount& target);

  void pack(const AmountCountDistributionInfo& source, rpc::AmountCountDistributionInfo& target);
  void unpack(const rpc::AmountCountDistributionInfo& source, AmountCountDistributionInfo& target);

  void pack(
    const CampaignAmountDistributionInfo& source, rpc::CampaignAmountDistributionInfo& target);
  void unpack(
    const rpc::CampaignAmountDistributionInfo& source, CampaignAmountDistributionInfo& target);

  void pack(const CCGAmountDistributionInfo& source, rpc::CCGAmountDistributionInfo& target);
  void unpack(const rpc::CCGAmountDistributionInfo& source, CCGAmountDistributionInfo& target);

  void pack(const BillStatInfo& source, rpc::BillStatInfo& target);
  void unpack(const rpc::BillStatInfo& source, BillStatInfo& target);

  void pack(const DiscoverChannel& source, rpc::DiscoverChannel& target);
  void unpack(const rpc::DiscoverChannel& source, DiscoverChannel& target);

  void pack(const SimpleChannelAnswer& source, rpc::SimpleChannelAnswer& target);
  void unpack(const rpc::SimpleChannelAnswer& source, SimpleChannelAnswer& target);

  void pack(const BriefSimpleChannelAnswer& source, rpc::BriefSimpleChannelAnswer& target);
  void unpack(const rpc::BriefSimpleChannelAnswer& source, BriefSimpleChannelAnswer& target);

  void pack(const BehaveInfo& source, rpc::BehaveInfo& target);
  void unpack(const rpc::BehaveInfo& source, BehaveInfo& target);

  void pack(const CSSimpleChannel& source, rpc::CSSimpleChannel& target);
  void unpack(const rpc::CSSimpleChannel& source, CSSimpleChannel& target);

  void pack(const ChannelServerChannelAnswer& source, rpc::ChannelServerChannelAnswer& target);
  void unpack(const rpc::ChannelServerChannelAnswer& source, ChannelServerChannelAnswer& target);

  void pack(const CampaignGetConfigSettings& source, rpc::CampaignGetConfigSettings& target);
  void unpack(const rpc::CampaignGetConfigSettings& source, CampaignGetConfigSettings& target);

  void pack(const FraudConditionConfig& source, rpc::FraudConditionConfig& target);
  void unpack(const rpc::FraudConditionConfig& source, FraudConditionConfig& target);

  void pack(const DetectorsConfig& source, rpc::DetectorsConfig& target);
  void unpack(const rpc::DetectorsConfig& source, DetectorsConfig& target);

  void pack(const FreqCapConfigInfo& source, rpc::FreqCapConfigInfo& target);
  void unpack(const rpc::FreqCapConfigInfo& source, FreqCapConfigInfo& target);

  void pack(const ExpressionChannelCCGInfo& source, rpc::ExpressionChannelCCGInfo& target);
  void unpack(const rpc::ExpressionChannelCCGInfo& source, ExpressionChannelCCGInfo& target);

  void pack(const ChannelTriggersInfo& source, rpc::ChannelTriggersInfo& target);
  void unpack(const rpc::ChannelTriggersInfo& source, ChannelTriggersInfo& target);

  void pack(const ExpressionChannelsInfo& source, rpc::ExpressionChannelsInfo& target);
  void unpack(const rpc::ExpressionChannelsInfo& source, ExpressionChannelsInfo& target);

  void pack(const AccountDeliveryInfo& source, rpc::AccountDeliveryInfo& target);
  void unpack(const rpc::AccountDeliveryInfo& source, AccountDeliveryInfo& target);

  void pack(const CampaignDeliveryInfo& source, rpc::CampaignDeliveryInfo& target);
  void unpack(const rpc::CampaignDeliveryInfo& source, CampaignDeliveryInfo& target);

  void pack(const CCGDeliveryInfo& source, rpc::CCGDeliveryInfo& target);
  void unpack(const rpc::CCGDeliveryInfo& source, CCGDeliveryInfo& target);

  void pack(
    const CampaignServer::GetExpressionChannelsInfo& source,
    rpc::GetExpressionChannelsInfo& target);
  void unpack(
    const rpc::GetExpressionChannelsInfo& source,
    CampaignServer::GetExpressionChannelsInfo& target);

  void pack(
    const CampaignServer::GetSimpleChannelsInfo& source, rpc::GetSimpleChannelsInfo& target);
  void unpack(
    const rpc::GetSimpleChannelsInfo& source, CampaignServer::GetSimpleChannelsInfo& target);

  void pack(const CampaignServer::PassbackInfo& source, rpc::PassbackInfo& target);
  void unpack(const rpc::PassbackInfo& source, CampaignServer::PassbackInfo& target);

  void pack(const CampaignServer::ColocationPropInfo& source, rpc::ColocationPropInfo& target);
  void unpack(const rpc::ColocationPropInfo& source, CampaignServer::ColocationPropInfo& target);

  void pack(
    const CampaignServer::DeliveryLimitConfigInfo& source, rpc::DeliveryLimitConfigInfo& target);
  void unpack(
    const rpc::DeliveryLimitConfigInfo& source, CampaignServer::DeliveryLimitConfigInfo& target);

}
