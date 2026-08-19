#pragma once

#include <string_view>

#include <eh/Exception.hpp>

#include "CTRGenerator.hpp"

namespace AdServer::CampaignSvcs
{
  struct CTRGeneratorFeatureConfig
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    unsigned long features_dimension;
    CTRGenerator::FeatureList features;
  };

  CTRGeneratorFeatureConfig
  load_ctr_generator_feature_config(std::string_view file_name);
}
