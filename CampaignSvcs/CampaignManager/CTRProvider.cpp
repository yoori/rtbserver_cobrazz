#include "CTRProvider.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  CTRProvider::~CTRProvider() noexcept = default;

  CTRProvider::Calculation::~Calculation() noexcept = default;

  CTRProvider::CalculationContext::~CalculationContext() noexcept = default;

  FeatureNameResolver::FeatureNameResolver() noexcept
  {}

  bool
  FeatureNameResolver::basic_feature_by_name(
    BasicFeature& basic_feature,
    const String::SubString& feature_name) const noexcept
  {
    const FeatureDescriptor* feature_descriptor =
      FeatureDescriptorResolver::instance().resolve_by_name(
        std::string_view(feature_name.data(), feature_name.size()));

    if (feature_descriptor)
    {
      basic_feature = feature_descriptor->feature;
      return true;
    }

    return false;
  }
}
