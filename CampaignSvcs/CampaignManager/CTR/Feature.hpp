#ifndef CAMPAIGNMANAGER_CTR_FEATURE_HPP_
#define CAMPAIGNMANAGER_CTR_FEATURE_HPP_

#include <set>
#include <vector>
#include <ostream>

#include "FeatureDescriptorResolver.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  typedef std::set<BasicFeature> BasicFeatureSet;

  // Feature
  struct Feature
  {
    void
    print(std::ostream& out) const;

    bool
    operator<(const Feature& right) const;

    BasicFeatureSet basic_features;
    std::size_t hash_seed;
    FeatureCalculator_var feature_calculator;
  };

  typedef std::vector<Feature> FeatureArray;
}

#endif /*CAMPAIGNMANAGER_CTR_FEATURE_HPP_*/
