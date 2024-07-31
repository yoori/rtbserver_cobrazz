#ifndef BIDCOSTPREDICTOR_CONTAINERIMPL_H
#define BIDCOSTPREDICTOR_CONTAINERIMPL_H

/// THIS
#include "Container.h"
#include "ModelBidCost.hpp"
#include "ModelCtr.hpp"

namespace PredictorSvcs::BidCostPredictor
{

class ContainerImpl final :
  public Container,
  public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  ContainerImpl(
    Logger* logger,
    const std::string& bid_cost_model_dir,
    const std::string& ctr_model_dir);

  Cost get_cost(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& current_cost) const override;

  Ctr get_ctr(
    const TagId& tag_id,
    const Url& url) const override;

protected:
  ~ContainerImpl() override = default;

private:
  void initialize();

  std::string get_last_file(const std::string& path_dir);

private:
  const Logger_var logger_;

  const std::string bid_cost_model_dir_;

  const std::string ctr_model_dir_;

  ModelBidCost_var model_bid_cost_;

  ModelCtr_var model_ctr_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_CONTAINERIMPL_H
