#pragma once

#include <set>
#include <string_view>
#include <vector>
#include <ostream>

#include <Generics/Singleton.hpp>

#include "CTRFeatureCalculators.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  enum BasicFeature
  {
    // request level features
    BF_PUBLISHER_ID = 1,
    BF_SITE_ID = 2,
    BF_TAG_ID = 3,
    BF_ETAG_ID = 4,
    BF_DOMAIN = 5,
    BF_URL = 6,
    BF_HOUR = 7,
    BF_WEEK_DAY = 8,
    BF_ISP_ID = 9,
    BF_COLO_ID = 10,
    BF_DEVICE_CHANNEL_ID = 11,
    BF_AFTER_HOUR = 12,
    BF_VISIBILITY = 13,
    BF_PREDICTED_VIEWABILITY = 14,

    // auction level features
    BF_AUCTION_LEVEL_FIRST_ID = 50,
    BF_SIZE_TYPE_ID = 50,
    BF_SIZE_ID = 51,

    // candidate level features
    BF_CANDIDATE_LEVEL_FIRST_ID = 100,
    BF_ADVERTISER_ID = 100,
    BF_CAMPAIGN_ID = 101,
    BF_CCG_ID = 102,
    BF_CAMPAIGN_FREQ_ID = 105,
    BF_CAMPAIGN_FREQ_LOG_ID = 106,

    BF_CREATIVE_LEVEL_FIRST_ID = 130,
    BF_CREATIVE_ID = 130,
    BF_CC_ID = 131, // non request feature

    // candidate level, array features
    BF_ARRAY_REQUEST_LEVEL_FIRST_ID = 150,
    BF_HISTORY_CHANNELS = 150,
    BF_GEO_CHANNELS = 151,

    BF_ARRAY_AUCTION_LEVEL_FIRST_ID = 152,
    BF_ARRAY_CANDIDATE_LEVEL_FIRST_ID = 152,
    BF_CONTENT_CATEGORIES = 152,
    BF_VISUAL_CATEGORIES = 153,

    // request level SSP-specific features; numeric IDs are direct float slots
    BF_SSP_TAG_ID = 9999,
    BF_SSP_CTR = 10000,
    BF_SSP_VIEWABILITY = 10001,
    BF_SSP_VTR = 10002,
  };

  constexpr bool
  is_direct_ssp_float_feature(BasicFeature feature) noexcept
  {
    return
      feature == BF_SSP_CTR ||
      feature == BF_SSP_VIEWABILITY ||
      feature == BF_SSP_VTR;
  }

  constexpr float SSP_FLOAT_UNSPECIFIED_VALUE = -1.0F;

  // FeatureDescriptor
  struct FeatureDescriptor
  {
    // smart ptr ownership semantic !!!
    // don't increase ref count on passed calculator creator
    // and hash calculator
    FeatureDescriptor(
      BasicFeature feature_val,
      std::string_view name_val,
      FeatureCalculatorCreator* calculator_creator_val)
      : feature(feature_val),
        name(name_val),
        calculator_creator(calculator_creator_val)
    {}

    BasicFeature feature;
    std::string_view name;
    FeatureCalculatorCreator_var calculator_creator;
  };

  // FeatureDescriptorResolver
  class FeatureDescriptorResolver_
  {
  public:
    FeatureDescriptorResolver_();

    // return null if feature unknown (return value life before exit call)
    const FeatureDescriptor* resolve(BasicFeature basic_feature) const;

    const FeatureDescriptor*
    resolve_by_name(std::string_view feature_name) const;

  private:
    using FeatureDescriptorMap = std::map<BasicFeature, FeatureDescriptor>;
    using FeatureDescriptorByNameMap = std::map<std::string_view, FeatureDescriptor>;

  private:
    FeatureDescriptorMap feature_descriptors_;
    FeatureDescriptorByNameMap feature_descriptors_by_name_;
  };

  using FeatureDescriptorResolver = Generics::Singleton<FeatureDescriptorResolver_>;
}
