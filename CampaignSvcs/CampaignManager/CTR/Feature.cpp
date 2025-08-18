#include "FeatureDescriptorResolver.hpp"

#include "Feature.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  // Feature
  void
  Feature::print(std::ostream& out) const
  {
    for(BasicFeatureSet::const_iterator fit = basic_features.begin();
        fit != basic_features.end(); ++fit)
    {
      if(fit != basic_features.begin())
      {
        out << ",";
      }

      const FeatureDescriptor* feature_descriptor =
        FeatureDescriptorResolver::instance().resolve(*fit);
      if(feature_descriptor)
      {
        out << "'" << feature_descriptor->name << "'";
      }
      else
      {
        out << "'unknown'";
      }
    }
    out << ": hash_seed = " << hash_seed << "(signed = " << (int32_t)hash_seed << ")";
  }

  bool
  Feature::operator<(const Feature& right) const
  {
    return std::lexicographical_compare(
      basic_features.begin(),
      basic_features.end(),
      right.basic_features.begin(),
      right.basic_features.end());
  }
}
