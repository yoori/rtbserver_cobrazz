#include "BidCost.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer
{
namespace LogProcessing
{
  template <> const char* BidCostTraits::B::base_name_ = "BidCost";
  template <> const char* BidCostTraits::B::signature_ = "BidCost";
  template <> const char* BidCostTraits::B::current_version_ = "1.0";
}
}
