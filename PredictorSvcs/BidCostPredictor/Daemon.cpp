// STD
#include <iostream>

// THIS
#include "Daemon.hpp"

// POSIX
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace Aspect
{
const char* DAEMON = "DAEMON";
}

namespace PredictorSvcs
{
namespace BidCostPredictor
{

class ShutdownManagerDaemon : private Generics::Uncopyable
{
public:
  static ShutdownManagerDaemon& getInstance()
  {
    static ShutdownManagerDaemon shutdownManager;
    return shutdownManager;
  }

  void stop() noexcept
  {
    shutdown_manager_.stop();
  }

  void wait() noexcept
  {
    shutdown_manager_.wait();
  }

private:
  ShutdownManagerDaemon() = default;

  ~ShutdownManagerDaemon() = default;

private:
  ShutdownManager shutdown_manager_;
};

void handleSignal(int)
{
  ShutdownManagerDaemon::getInstance().stop();
}

Daemon::Daemon(
        const std::string& pid_path,
        const Logging::Logger_var& logger)
        : pid_path_(pid_path),
          logger_(logger)
{
}

Daemon::~Daemon()
{
  wait();
}

void Daemon::run()
{
  try
  {
    start();
    wait();
  }
  catch (const eh::Exception& exc)
  {
    stop();
    wait();
    throw;
  }
}

void Daemon::start()
{
  try
  {
    umask(0);

    pid_t pid = -1;
    if ((pid = fork()) < 0)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << ": Reason: "
           << "fork is failed";
      throw Exception (ostr);
    }
    else if (pid != 0)
    {
      return;
    }

    setsid();

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << ": Reason : it is impossible to ignore"
           << " the signal SIGHUP";
      throw Exception(ostr);
    }

    if ((pid = fork()) < 0)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << ": Reason: "
           << "fork is failed";
      throw Exception(ostr);
    }
    else if (pid != 0)
    {
      return;
    }

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << ": Reason: "
           << "getcwd is failed";
      throw Exception(ostr);
    }

    if (chdir(cwd) < 0)
    {
      Stream::Error ostr;
      ostr << __PRETTY_FUNCTION__
           << ": Reason: "
           << "chdir is failed";
      throw Exception(ostr);
    }

    pid_setter_ = std::make_unique<PidSetter>(pid_path_);
    if (!pid_setter_->set())
    {
      throw Exception("Daemon already running");
    }

    signal(SIGINT, handleSignal);
    signal(SIGQUIT, handleSignal);
    signal(SIGTERM, handleSignal);

    is_running_ = true;

    startLogic();
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error ostr;
    ostr << __PRETTY_FUNCTION__
         << " : Can't start Daemon. Reason : "
         << exc.what();
    stop();
    throw Exception(ostr);
  }
}

void Daemon::stop() noexcept
{
  auto& shutdown_manager = ShutdownManagerDaemon::getInstance();
  shutdown_manager.stop();

  if (is_running_)
    stopLogic();
}

void Daemon::wait() noexcept
{
  if (!is_running_)
    return;
  is_running_ = false;

  auto& shutdown_manager = ShutdownManagerDaemon::getInstance();
  shutdown_manager.wait();

  stopLogic();
  waitLogic();
}

const char* Daemon::name() noexcept
{
  return "BidCostPredictorDaemon";
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs