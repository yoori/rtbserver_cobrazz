#ifndef BIDCOSTPREDICTOR_ACTIVEOBJECTOBSERVER_HPP
#define BIDCOSTPREDICTOR_ACTIVEOBJECTOBSERVER_HPP

// STD
#include <mutex>

// UNIXCOMMONS
#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace PredictorSvcs::BidCostPredictor
{

class ActiveObjectObserver final :
  public Generics::ActiveObjectCallback,
  public ReferenceCounting::AtomicImpl
{
public:
  using ErrorCallback = std::function<void(
    const Severity,
    const String::SubString&,
    const char*)>;

public:
  explicit ActiveObjectObserver(const ErrorCallback& error_callback)
    : error_callback_(error_callback)
  {
  }

protected:
  ~ActiveObjectObserver() override = default;

private:
  void report_error(
    Severity severity,
    const String::SubString& description,
    const char* error_code = nullptr) noexcept override
  {
    try
    {
      if (error_callback_)
      {
        error_callback_(severity, description, error_code);
      }
    }
    catch (...)
    {
    }
  }

private:
  const ErrorCallback error_callback_;
};

using ActiveObjectObserver_var = ReferenceCounting::SmartPtr<ActiveObjectObserver>;

} // namespace PredictorSvcs::BidCostPredictor

#endif //BIDCOSTPREDICTOR_ACTIVEOBJECTOBSERVER_HPP