#ifndef RTBSERVER_CTRPREDICTOR_HPP
#define RTBSERVER_CTRPREDICTOR_HPP

// STD
#include <shared_mutex>

// UNIXCOMMONS
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

// THIS
#include <PredictorSvcs/BidCostPredictor/ModelCtr.hpp>
#include <PredictorSvcs/BidCostPredictor/Types.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class CtrPredictor final : public ReferenceCounting::AtomicImpl
{
public:
  using Logger_var = Logging::Logger_var;
  using Logger = Logging::Logger;
  using Ctr = Types::Ctr;
  using TagId = Types::TagId;
  using Url = Types::Url;
  using CreativeCategoryIds = ModelCtr::CreativeCategoryIds;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit CtrPredictor(Logger* logger);

  Ctr predict(
    const TagId& tag_id,
    const Url& url,
    const CreativeCategoryIds& creative_category_ids);

  void load(const std::string& path);

protected:
  ~CtrPredictor() override = default;

private:
  const Logger_var logger_;

  const ModelCtr_var model_ctr_;
};

using CtrPredictor_var = ReferenceCounting::SmartPtr<CtrPredictor>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //RTBSERVER_CTRPREDICTOR_HPP
