#ifndef BIDCOSTPREDICTOR_CONTAINERIMPL_H
#define BIDCOSTPREDICTOR_CONTAINERIMPL_H

/// THIS
#include "Container.h"
#include "ModelBidCost.hpp"
#include "ModelCtr.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class ContainerImpl :
  public Container,
  public virtual ReferenceCounting::AtomicImpl
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
public:
  ContainerImpl(
    Logging::Logger* logger,
    const std::string& bid_cost_model_dir,
    const std::string& ctr_model_dir);

  ~ContainerImpl() = default;

  Cost get_cost(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& current_cost) const override;

  Ctr get_ctr(
    const TagId& tag_id,
    const Url& url) const override;

private:
  void initialize();

  std::string get_last_file(const std::string& path_dir);

private:
  Logging::Logger_var logger_;

  const std::string bid_cost_model_dir_;

  const std::string ctr_model_dir_;

  ModelBidCost_var model_bid_cost_;

  ModelCtr_var model_ctr_;
};

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_CONTAINERIMPL_H
