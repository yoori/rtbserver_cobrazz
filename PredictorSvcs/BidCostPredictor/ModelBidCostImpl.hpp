#ifndef BIDCOSTPREDICTOR_MODELBIDCOSTIMPL_HPP
#define BIDCOSTPREDICTOR_MODELBIDCOSTIMPL_HPP

// STD
#include <set>
#include <tuple>

// THIS
#include <PredictorSvcs/BidCostPredictor/ModelBidCost.hpp>
#include <PredictorSvcs/BidCostPredictor/BidCostCollector.hpp>
#include <PredictorSvcs/BidCostPredictor/BidCostHelpCollector.hpp>

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
  using UrlView = std::string_view;
  using WinRate = typename ModelBidCost::WinRate;
  using Cost = typename ModelBidCost::Cost;
  using MaxCost = typename ModelBidCost::Cost;
  using WinRates = std::map<
      std::tuple<TagId, UrlView, WinRate>,
      std::pair<Cost, MaxCost>>;

public:
  explicit ModelBidCostImpl(Logger* logger);

  Cost get_cost(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& cur_cost) const override;

  void set_cost(
    const TagId& tag_id,
    const Url& url,
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
