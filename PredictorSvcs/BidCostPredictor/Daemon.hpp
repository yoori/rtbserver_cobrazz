#ifndef BIDCOSTPREDICTOR_DAEMON_HPP
#define BIDCOSTPREDICTOR_DAEMON_HPP

// STD
#include <memory>
#include <string>

// UNIXCOMMONS
#include <Logger/Logger.hpp>
#include <ReferenceCounting/Interface.hpp>

// THIS
#include "Pid.hpp"
#include "Processor.hpp"
#include "ShutdownManager.hpp"

namespace PredictorSvcs::BidCostPredictor
{

class Daemon : public virtual ReferenceCounting::Interface
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  Daemon(
    const std::string& pid_path,
    Logging::Logger* logger);

  void run();

  void stop() noexcept;

protected:
  ~Daemon() override = default;

  virtual void start_logic() = 0;

  virtual void stop_logic() noexcept = 0;

  virtual void wait_logic() noexcept = 0;

private:
  void start();

  void wait() noexcept;

private:
  const std::string pid_path_;

  const Logger_var logger_;

  const std::unique_ptr<PidSetter> pid_setter_;

  bool is_running_ = false;
};

using Daemon_var = ReferenceCounting::SmartPtr<Daemon>;

} // namespace PredictorSvcs::BidCostPredictor

#endif // BIDCOSTPREDICTOR_DAEMON_HPP