#ifndef BIDCOSTPREDICTOR_TYPES_H
#define BIDCOSTPREDICTOR_TYPES_H

// STD
#include <memory>
#include <vector>

// THIS
#include <LogCommons/LogCommons.hpp>

namespace PredictorSvcs::BidCostPredictor::Types
{

using TagId = unsigned long;
using Url = std::string;
using UrlPtr = std::shared_ptr<Url>;
using Imps = unsigned long;
using Clicks = unsigned long;
using FixedNumber = AdServer::LogProcessing::FixedNumber;
using Cost = FixedNumber;
using WinRate = Cost;
using Point = FixedNumber;
using Points = std::vector<Point>;
using Ctr = FixedNumber;
using CcId = unsigned long;
using CreativeCategoryId = unsigned long;

} // namespace PredictorSvcs::BidCostPredictor::Types

#endif //BIDCOSTPREDICTOR_TYPES_H
