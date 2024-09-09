#include "LogHelper.hpp"

namespace AdServer::LogProcessing
{

template<> const char *BidCostStatInnerTraits::B::base_name_ = "BidCostAggStat";
template<> const char *BidCostStatInnerTraits::B::signature_ = "BidCostAggStat";
template<> const char *BidCostStatInnerTraits::B::current_version_ = "2.5";

} // namespace AdServer::LogProcessing
