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
using Imps = long;
using Clicks = long;
using FixedNumber = AdServer::LogProcessing::FixedNumber;
using Cost = FixedNumber;
using WinRate = Cost;
using Point = Types::FixedNumber;
using Points = std::vector<Point>;

} // namespace PredictorSvcs::BidCostPredictor::Types

#endif //BIDCOSTPREDICTOR_TYPES_H
