#include <filesystem>
#include <system_error>
#include <unistd.h>

#include "CTRProvider.hpp"

namespace AdServer::CampaignSvcs::CTR
{
  CTRProvider::~CTRProvider() noexcept = default;

  CTRProvider::Calculation::~Calculation() noexcept = default;

  CTRProvider::CalculationContext::~CalculationContext() noexcept = default;

  void
  CTRProvider::remove_config_paths(
    const std::list<std::string>& config_files,
    const std::list<std::string>& config_directories) noexcept
  {
    for (const std::string& file : config_files)
    {
      ::unlink(file.c_str());
    }

    for (const std::string& directory : config_directories)
    {
      std::error_code error;
      std::filesystem::remove_all(directory, error);
    }
  }

  FeatureNameResolver::FeatureNameResolver() noexcept
  {}

  bool
  FeatureNameResolver::basic_feature_by_name(
    BasicFeature& basic_feature,
    std::string_view feature_name) const noexcept
  {
    const FeatureDescriptor* feature_descriptor =
      FeatureDescriptorResolver::instance().resolve_by_name(feature_name);

    if (feature_descriptor)
    {
      basic_feature = feature_descriptor->feature;
      return true;
    }

    return false;
  }
}
