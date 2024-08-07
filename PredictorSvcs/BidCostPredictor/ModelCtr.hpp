#ifndef BIDCOSTPREDICTOR_MODELCTR_HPP
#define BIDCOSTPREDICTOR_MODELCTR_HPP

// STD
#include <string>

// UNIXCOMMONS
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

// THIS
#include "CtrCollector.hpp"
#include "ModelManager.hpp"
#include "Types.hpp"

namespace PredictorSvcs::BidCostPredictor
{

namespace LogProcessing = AdServer::LogProcessing;

class ModelCtr : public ModelManager
{
public:
  using TagId = Types::TagId;
  using Url = Types::Url;
  using UrlPtr = Types::UrlPtr;
  using CreativeCategoryId = Types::CreativeCategoryId;
  using Ctr = typename LogProcessing::CtrData::Ctr;

public:
  explicit ModelCtr() = default;

  virtual Ctr get_ctr(
    const TagId& tag_id,
    const Url& url,
    const CreativeCategoryId& creative_category_id) const = 0;

  virtual void set_ctr(
    const TagId& tag_id,
    const UrlPtr& url,
    const CreativeCategoryId& creative_category_id,
    const Ctr& ctr) = 0;

  virtual void clear() noexcept = 0;

protected:
  ~ModelCtr() override = default;
};

using ModelCtr_var = ReferenceCounting::SmartPtr<ModelCtr>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_MODELCTR_HPP
