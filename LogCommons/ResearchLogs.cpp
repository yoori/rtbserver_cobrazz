#include "ResearchLogs.hpp"

namespace AdServer
{
namespace LogProcessing
{
  template <> const char*
  ResearchBidTraits::B::base_name_ = "<UNDEFINED>";
  template <> const char*
  ResearchBidTraits::B::signature_ = "<UNDEFINED>";
  template <> const char*
  ResearchBidTraits::B::current_version_ = "<UNDEFINED>";

  template <> const char*
  ResearchImpressionTraits::B::base_name_ = "<UNDEFINED>";
  template <> const char*
  ResearchImpressionTraits::B::signature_ = "<UNDEFINED>";
  template <> const char*
  ResearchImpressionTraits::B::current_version_ = "<UNDEFINED>";

  template <> const char*
  ResearchClickTraits::B::base_name_ = "<UNDEFINED>";
  template <> const char*
  ResearchClickTraits::B::signature_ = "<UNDEFINED>";
  template <> const char*
  ResearchClickTraits::B::current_version_ = "<UNDEFINED>";

  template <> const char*
  ResearchActionTraits::B::base_name_ = "<UNDEFINED>";
  template <> const char*
  ResearchActionTraits::B::signature_ = "<UNDEFINED>";
  template <> const char*
  ResearchActionTraits::B::current_version_ = "<UNDEFINED>";
} // namespace LogProcessing
} // namespace AdServer

