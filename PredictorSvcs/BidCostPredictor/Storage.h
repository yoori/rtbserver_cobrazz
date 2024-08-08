#ifndef BIDCOSTPREDICTOR_STORAGE_H
#define BIDCOSTPREDICTOR_STORAGE_H

// STD
#include <shared_mutex>

// UNIXCOMMONS
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

// THIS
#include <Commons/DelegateTaskGoal.hpp>
#include "ActiveObjectObserver.hpp"
#include "Container.h"
#include "Processor.hpp"
#include "ShutdownManager.hpp"
#include "Utils.hpp"

namespace PredictorSvcs::BidCostPredictor
{

class Storage final :
  public Container,
  public Generics::CompositeActiveObject,
  public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using SharedMutex = std::shared_mutex;

public:
  explicit Storage(
    Logging::Logger* logger,
    Generics::ActiveObjectCallback* callback,
    const std::string& bid_cost_model_dir,
    const std::string& ctr_model_dir,
    const std::size_t update_period);

  Cost get_cost(
    const TagId& tag_id,
    const Url& url,
    const WinRate& win_rate,
    const Cost& current_cost) const override;

  Ctr get_ctr(
    const TagId& tag_id,
    const Url& url,
    const CreativeCategoryId& creative_category_id) const override;

protected:
  ~Storage() override = default;

private:
  void create_scheduler_task(const std::size_t period) noexcept;

  void update() noexcept;

private:
  const Logging::Logger_var logger_;

  const std::string bid_cost_model_dir_;

  const std::string ctr_model_dir_;

  const std::size_t update_period_;

  Generics::Planner_var scheduler_;

  Generics::TaskRunner_var task_runner_;

  Container_var container_;

  mutable SharedMutex shared_mutex_;
};

using Storage_var = ReferenceCounting::SmartPtr<Storage>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_STORAGE_H