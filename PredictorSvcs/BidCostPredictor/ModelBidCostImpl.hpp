#ifndef BIDCOSTPREDICTOR_MODELBIDCOSTIMPL_HPP
#define BIDCOSTPREDICTOR_MODELBIDCOSTIMPL_HPP

// STD
#include <set>
#include <tuple>

// THIS
#include "ModelBidCost.hpp"
#include "BidCostCollector.hpp"
#include "HelpCollector.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelBidCostImpl :
        public ModelBidCost,
        public virtual ReferenceCounting::AtomicImpl
{
public:
  using TagId = typename ModelBidCost::TagId;
  using Url = typename ModelBidCost::Url;
  using WinRate = typename ModelBidCost::WinRate;
  using Cost = typename ModelBidCost::Cost;
  using WinRates = std::set<std::tuple<std::string_view, TagId, WinRate>>;

public:
  ModelBidCostImpl(Logging::Logger_var& logger);

  ~ModelBidCostImpl() override = default;

  FixedNumber getCost(
          const TagId& tag_id,
          const Url& url,
          const WinRate& win_rate,
          const Cost& cur_cost) const override;

  void setCost(
          const TagId& tag_id,
          const Url_var& url,
          const WinRate& win_rate,
          const Cost& cost,
          const Cost& max_cost) override;

  void clear() noexcept override;

  void save(const std::string& path) const override;

  void load(const std::string& path) override;

private:
  Logging::Logger_var& logger_;

  WinRates win_rates_;

  LogProcessing::BidCostCollector collector_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif // BIDCOSTPREDICTOR_MODELBIDCOSTIMPL_HPP
