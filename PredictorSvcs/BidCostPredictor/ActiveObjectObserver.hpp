#ifndef BIDCOSTPREDICTOR_ACTIVEOBJECTOBSERVER_HPP
#define BIDCOSTPREDICTOR_ACTIVEOBJECTOBSERVER_HPP

// STD
#include <mutex>

// THIS
#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class ActiveObjectDelegate : Generics::Uncopyable
{
public:
  using Severity = Generics::ActiveObjectCallback::Severity;

public:
  ActiveObjectDelegate() = default;

  virtual void report_error(
    Severity severity,
    const String::SubString& description,
    const char* error_code = 0) noexcept = 0;

protected:
  virtual ~ActiveObjectDelegate() = default;
};

class ActiveObjectObserver final :
  public Generics::ActiveObjectCallback,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  explicit ActiveObjectObserver(
    ActiveObjectDelegate* delegate)
    : delegate_(delegate)
  {
  }

  ~ActiveObjectObserver() override = default;

  void report_error(
    Severity severity,
    const String::SubString& description,
    const char* error_code = 0) noexcept override
  {
    std::lock_guard lock(mutex_);
    if (delegate_)
      delegate_->report_error(severity, description, error_code);
  }

  void clear_delegate()
  {
    std::lock_guard lock(mutex_);
    delegate_ = nullptr;
  }

private:
  ActiveObjectDelegate* delegate_ = nullptr;

  std::mutex mutex_;
};

using ActiveObjectObserver_var = ReferenceCounting::SmartPtr<ActiveObjectObserver>;

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_ACTIVEOBJECTOBSERVER_HPP
