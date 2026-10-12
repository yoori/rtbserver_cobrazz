#pragma once

#include <cstdint>
#include <string>
#include <CampaignSvcs/CampaignServer/CampaignServer.pb.h>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/WireTypes.hpp>

namespace AdServer::CampaignSvcs
{
  namespace ChannelFlags
  {
    inline constexpr std::uint32_t HIDDEN = 0x04;
    inline constexpr std::uint32_t TARGETED = 0x10000;
  }

  using TimestampInfo = AdServer::Commons::TimestampInfo;

  using StringSeq = google::protobuf::RepeatedPtrField<std::string>;

  using AppFormatInfo = adserver::campaign::server::AppFormatInfo;

  using AppFormatSeq = google::protobuf::RepeatedPtrField<AppFormatInfo>;

  using SizeInfo = adserver::campaign::server::SizeInfo;

  using SizeSeq = google::protobuf::RepeatedPtrField<SizeInfo>;

  using CreativeOptionInfo = adserver::campaign::server::CreativeOptionInfo;

  using CreativeOptionSeq = google::protobuf::RepeatedPtrField<CreativeOptionInfo>;

  using OptionValueInfo = adserver::campaign::server::OptionValueInfo;

  using OptionValueSeq = google::protobuf::RepeatedPtrField<OptionValueInfo>;

  using CreativeCategoryIdSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using CountryInfo = adserver::campaign::server::CountryInfo;

  using CountryInfoSeq = google::protobuf::RepeatedPtrField<CountryInfo>;

  using ExternalCategoryInfo = adserver::campaign::server::ExternalCategoryInfo;

  using ExternalCategorySeq = google::protobuf::RepeatedPtrField<ExternalCategoryInfo>;

  using CreativeCategoryInfo = adserver::campaign::server::CreativeCategoryInfo;

  using CreativeCategorySeq = google::protobuf::RepeatedPtrField<CreativeCategoryInfo>;

  using CreativeSizeInfo = adserver::campaign::server::CreativeSizeInfo;

  using CreativeSizeSeq = google::protobuf::RepeatedPtrField<CreativeSizeInfo>;

  using CreativeInfo = adserver::campaign::server::CreativeInfo;

  using ChannelIdSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using PublisherAccountIdSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using ChannelWeight = adserver::campaign::server::ChannelWeight;

  using ChannelWeightSeq = google::protobuf::RepeatedPtrField<ChannelWeight>;

  using DeletedIdInfo = adserver::campaign::server::DeletedIdInfo;

  using DeletedIdSeq = google::protobuf::RepeatedPtrField<DeletedIdInfo>;

  using DeletedStringIdInfo = adserver::campaign::server::DeletedStringIdInfo;

  using DeletedStringIdSeq = google::protobuf::RepeatedPtrField<DeletedStringIdInfo>;

  using CampaignEcpmInfo = adserver::campaign::server::CampaignEcpmInfo;

  using EcpmSeq = google::protobuf::RepeatedPtrField<CampaignEcpmInfo>;

  using ChannelCategorySeq = google::protobuf::RepeatedField<std::uint32_t>;

  using ExpressionInfo = adserver::campaign::server::ExpressionInfo;

  using ExpressionSeq = google::protobuf::RepeatedPtrField<ExpressionInfo>;

  using ExpressionChannelInfo = adserver::campaign::server::ExpressionChannelInfo;

  using ExpressionChannelSeq = google::protobuf::RepeatedPtrField<ExpressionChannelInfo>;

  using BehavioralParameterInfo = adserver::campaign::server::BehavioralParameterInfo;

  using BehavioralParameterSeq = google::protobuf::RepeatedPtrField<BehavioralParameterInfo>;

  using WeeklyRunInterval = adserver::campaign::server::WeeklyRunInterval;

  using CreativeSeq = google::protobuf::RepeatedPtrField<CreativeInfo>;

  using WeeklyRunIntervalSeq = google::protobuf::RepeatedPtrField<WeeklyRunInterval>;

  using ColoIdSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using AccountIdSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using TagIdSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using CreativeTemplateType = adserver::campaign::server::CreativeTemplateType;
  using adserver::campaign::server::CTT_NONE;
  using adserver::campaign::server::CTT_TEXT;
  using adserver::campaign::server::CTT_XSLT;

  using CampaignDeliveryLimitsInfo = adserver::campaign::server::CampaignDeliveryLimitsInfo;

  using TagDeliveryInfo = adserver::campaign::server::TagDeliveryInfo;

  using TagDeliverySeq = google::protobuf::RepeatedPtrField<TagDeliveryInfo>;

  using UserGroupIdSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using ULongSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using ContractInfo = adserver::campaign::server::ContractInfo;

  using ContractSeq = google::protobuf::RepeatedPtrField<ContractInfo>;

  using CampaignInfo = adserver::campaign::server::CampaignInfo;

  using TagPricingInfo = adserver::campaign::server::TagPricingInfo;

  using TagPricingInfoSeq = google::protobuf::RepeatedPtrField<TagPricingInfo>;

  using TemplateOptionsInfo = adserver::campaign::server::TemplateOptionsInfo;

  using TemplateOptionsSeq = google::protobuf::RepeatedPtrField<TemplateOptionsInfo>;

  using TagSizeInfo = adserver::campaign::server::TagSizeInfo;

  using TagSizeSeq = google::protobuf::RepeatedPtrField<TagSizeInfo>;

  using TagInfo = adserver::campaign::server::TagInfo;

  using CampaignIdSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using CCIdSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using SiteInfo = adserver::campaign::server::SiteInfo;

  using ColocationInfo = adserver::campaign::server::ColocationInfo;

  using ColocationSeq = google::protobuf::RepeatedPtrField<ColocationInfo>;

  using ColocationFlags = adserver::campaign::server::ColocationFlags;

  using ColocationFlagsSeq = google::protobuf::RepeatedPtrField<ColocationFlags>;

  using FreqCapInfo = adserver::campaign::server::FreqCapInfo;

  using FreqCapSeq = google::protobuf::RepeatedPtrField<FreqCapInfo>;

  using CreativeTemplateFileInfo = adserver::campaign::server::CreativeTemplateFileInfo;

  using CreativeTemplateFileInfoSeq = google::protobuf::RepeatedPtrField<CreativeTemplateFileInfo>;

  using CreativeTemplateInfo = adserver::campaign::server::CreativeTemplateInfo;

  using CurrencyInfo = adserver::campaign::server::CurrencyInfo;

  using CampaignKeywordInfo = adserver::campaign::server::CampaignKeywordInfo;

  using CampaignKeywordSeq = google::protobuf::RepeatedPtrField<CampaignKeywordInfo>;

  using CCGIdSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using AdvActionInfo = adserver::campaign::server::AdvActionInfo;

  using AdvActionInfoSeq = google::protobuf::RepeatedPtrField<AdvActionInfo>;

  using AccountInfo = adserver::campaign::server::AccountInfo;

  using AccountInfoSeq = google::protobuf::RepeatedPtrField<AccountInfo>;

  using CategoryChannelLocalizationInfo =
    adserver::campaign::server::CategoryChannelLocalizationInfo;

  using CategoryChannelLocalizationSeq =
    google::protobuf::RepeatedPtrField<CategoryChannelLocalizationInfo>;

  using CategoryChannelInfo = adserver::campaign::server::CategoryChannelInfo;

  using CategoryChannelSeq = google::protobuf::RepeatedPtrField<CategoryChannelInfo>;

  using PriceRangeInfo = adserver::campaign::server::PriceRange;

  using PriceRangeSeq = google::protobuf::RepeatedPtrField<PriceRangeInfo>;

  using BehavParameter = adserver::campaign::server::BehavParameter;

  using BehavParameterSeq = google::protobuf::RepeatedPtrField<BehavParameter>;

  using BehavParamInfo = adserver::campaign::server::BehavParamInfo;

  using BehavParamInfoSeq = google::protobuf::RepeatedPtrField<BehavParamInfo>;

  using KeyBehavParamInfo = adserver::campaign::server::KeyBehavParamInfo;

  using KeyBehavParamInfoSeq = google::protobuf::RepeatedPtrField<KeyBehavParamInfo>;

  using BriefBehavParamInfo = adserver::campaign::server::BriefBehavParamInfo;

  using BriefBehavParamInfoSeq = google::protobuf::RepeatedPtrField<BriefBehavParamInfo>;

  using BriefKeyBehavParamInfo = adserver::campaign::server::BriefKeyBehavParamInfo;

  using BriefKeyBehavParamInfoSeq = google::protobuf::RepeatedPtrField<BriefKeyBehavParamInfo>;

  using ChannelTriggerIdSeq = google::protobuf::RepeatedField<std::uint32_t>;

  using SimpleChannelKey = adserver::campaign::server::SimpleChannelKey;

  using SimpleChannelKeySeq = google::protobuf::RepeatedPtrField<SimpleChannelKey>;

  using BriefSimpleChannelKey = adserver::campaign::server::BriefSimpleChannelKey;

  using BriefSimpleChannelKeySeq = google::protobuf::RepeatedPtrField<BriefSimpleChannelKey>;

  using GeoIPRegionCityInfo = adserver::campaign::server::GeoIPRegionCityInfo;

  using GeoIPRegionCitySeq = google::protobuf::RepeatedPtrField<GeoIPRegionCityInfo>;

  using GeoChannelInfo = adserver::campaign::server::GeoChannelInfo;

  using GeoChannelSeq = google::protobuf::RepeatedPtrField<GeoChannelInfo>;

  using GeoCoordChannelInfo = adserver::campaign::server::GeoCoordChannelInfo;

  using GeoCoordChannelSeq = google::protobuf::RepeatedPtrField<GeoCoordChannelInfo>;

  using BlockChannelInfo = adserver::campaign::server::BlockChannelInfo;

  using BlockChannelSeq = google::protobuf::RepeatedPtrField<BlockChannelInfo>;

  using FraudConditionInfo = adserver::campaign::server::FraudConditionInfo;

  using FraudConditionSeq = google::protobuf::RepeatedPtrField<FraudConditionInfo>;

  using SearchEngineRegExpInfo = adserver::campaign::server::SearchEngineRegExpInfo;

  using SearchEngineRegExpSeq = google::protobuf::RepeatedPtrField<SearchEngineRegExpInfo>;

  using SearchEngineInfo = adserver::campaign::server::SearchEngineInfo;

  using SearchEngineSeq = google::protobuf::RepeatedPtrField<SearchEngineInfo>;

  using WebBrowserDetectorInfo = adserver::campaign::server::WebBrowserDetectorInfo;

  using WebBrowserDetectorSeq = google::protobuf::RepeatedPtrField<WebBrowserDetectorInfo>;

  using WebBrowserInfo = adserver::campaign::server::WebBrowserInfo;

  using WebBrowserSeq = google::protobuf::RepeatedPtrField<WebBrowserInfo>;

  using PlatformDetectorInfo = adserver::campaign::server::PlatformDetectorInfo;

  using PlatformDetectorSeq = google::protobuf::RepeatedPtrField<PlatformDetectorInfo>;

  using PlatformInfo = adserver::campaign::server::PlatformInfo;

  using PlatformSeq = google::protobuf::RepeatedPtrField<PlatformInfo>;

  using WebOperationInfo = adserver::campaign::server::WebOperationInfo;

  using WebOperationSeq = google::protobuf::RepeatedPtrField<WebOperationInfo>;

  using CampaignInfoSeq = google::protobuf::RepeatedPtrField<CampaignInfo>;

  using SiteInfoSeq = google::protobuf::RepeatedPtrField<SiteInfo>;

  using TagInfoSeq = google::protobuf::RepeatedPtrField<TagInfo>;

  using ColocationInfoSeq = google::protobuf::RepeatedPtrField<ColocationInfo>;

  using CreativeTemplateInfoSeq = google::protobuf::RepeatedPtrField<CreativeTemplateInfo>;

  using CurrencyInfoSeq = google::protobuf::RepeatedPtrField<CurrencyInfo>;

  using CampaignConfigUpdateInfo = adserver::campaign::server::CampaignConfigUpdateInfo;
}
