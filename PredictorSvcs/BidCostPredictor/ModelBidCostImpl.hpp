#ifndef BIDCOSTPREDICTOR_MODELBIDCOSTIMPL_HPP
#define BIDCOSTPREDICTOR_MODELBIDCOSTIMPL_HPP

// STD
#include <set>
#include <tuple>

// THIS
#include "ModelBidCost.hpp"
#include "BidCostCollector.hpp"
#include "HelpCollector.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelBidCostImpl final :
  public ModelBidCost,
  public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using TagId = typename ModelBidCost::TagId;
  using Url = typename ModelBidCost::Url;
  using WinRate = typename ModelBidCost::WinRate;
  using Cost = typename ModelBidCost::Cost;
  using BidCostKey = LogProcessing::BidCostKey;
  using BidCostData = LogProcessing::BidCostData;
  using WinRates = std::map<
      std::tuple<TagId, std::string_view, WinRate>,
      BidCostData>;
  using BidCostCollector = LogProcessing::BidCostCollector;
  using BidCostTraits = LogProcessing::BidCostTraits;

public:
  explicit ModelBidCostImpl(Logger* logger);

  Cost get_cost(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& cur_cost) const override;

  void set_cost(
    const TagId& tag_id,
    const UrlPtr& url,
    const WinRate& win_rate,
    const Cost& cost,
    const Cost& max_cost) override;

  void clear() noexcept override;

  void save(const std::string& path) const override;

  void load(const std::string& path) override;

protected:
  ~ModelBidCostImpl() override = default;

private:
  const Logger_var logger_;

  WinRates win_rates_;

  BidCostCollector collector_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif // BIDCOSTPREDICTOR_MODELBIDCOSTIMPL_HPP
