#ifndef BIDCOSTPREDICTOR_DAEMON_HPP
#define BIDCOSTPREDICTOR_DAEMON_HPP

// STD
#include <memory>
#include <string>

// THIS
#include <Logger/Logger.hpp>
#include <ReferenceCounting/Interface.hpp>
#include "Pid.hpp"
#include "Processor.hpp"
#include "ShutdownManager.hpp"

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class Daemon : public virtual ReferenceCounting::Interface
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
public:
  Daemon(
    const std::string& pid_path,
    Logging::Logger* logger);

  virtual ~Daemon();

  void run();

  void stop() noexcept;

protected:
  virtual void start_logic() = 0;

  virtual void stop_logic() noexcept = 0;

  virtual void wait_logic() noexcept = 0;

private:
  void start();

  void wait() noexcept;

  const char* name() noexcept;

private:
  const std::string pid_path_;

  Logging::Logger_var logger_;

  Processor* delegate_;

  std::unique_ptr<PidSetter> pid_setter_;

  bool is_running_ = false;
};

using Daemon_var = ReferenceCounting::SmartPtr<Daemon>;

} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif // BIDCOSTPREDICTOR_DAEMON_HPP
