#ifndef BIDCOSTPREDICTOR_MODELCTRIMPL_HPP
#define BIDCOSTPREDICTOR_MODELCTRIMPL_HPP

// UNIXCOMMONS
#include "ReferenceCounting/AtomicImpl.hpp"

// THIS
#include <PredictorSvcs/BidCostPredictor/CtrCollector.hpp>
#include <PredictorSvcs/BidCostPredictor/ModelCtr.hpp>


namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelCtrImpl final :
  public ModelCtr,
  public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  explicit ModelCtrImpl(Logger* logger);

  Ctr get_ctr(
    const TagId& tag_id,
    const Url& url,
    const CreativeCategoryIds& creative_category_ids) const;

  void set_ctr(
    const TagId& tag_id,
    const Url& url,
    const CreativeCategoryId& creative_category_id,
    const Ctr& ctr) override;

  void clear() noexcept;

  void load(const std::string& path);

  void save(const std::string& path) const;

protected:
  ~ModelCtrImpl() override = default;

private:
  const Logger_var logger_;

  CtrCollector collector_;
};

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_MODELCTRIMPL_HPP
