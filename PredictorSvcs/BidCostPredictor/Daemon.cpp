// STD
#include <iostream>
#include <sstream>

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

Daemon::Daemon(
  const std::string& pid_path,
  Logging::Logger* logger)
  : pid_path_(pid_path),
    logger_(ReferenceCounting::add_ref(logger))
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

    is_running_ = true;

    start_logic();
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
  const auto pid = ::getpid();
  if (kill(pid, SIGINT) == -1)
  {
    std::stringstream stream;
    stream << __PRETTY_FUNCTION__
           << " function kill is failed";
    logger_->critical(stream.str(), Aspect::DAEMON);
  }

  if (is_running_)
    stop_logic();
}

void Daemon::wait() noexcept
{
  if (!is_running_)
    return;
  is_running_ = false;

  try
  {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    int signo = 0;
    if (sigwait(&mask, &signo) != 0)
    {
      std::stringstream stream;
      stream << __PRETTY_FUNCTION__
             << " : sigwait is failed";
      logger_->critical(stream.str(), Aspect::DAEMON);
    }
    else
    {
      std::stringstream stream;
      stream << "Signal=";
      switch (signo)
      {
      case SIGINT:
        stream << "SIGINT";
        break;
      case SIGQUIT:
        stream << "SIGQUIT";
        break;
      case SIGTERM:
        stream << "SIGTERM";
        break;
      default:
        stream << "Unexpected signal";
      }
      stream << " interrupted service";
      logger_->info(stream.str(), Aspect::DAEMON);
    }

    stop_logic();
    wait_logic();
  }
  catch (...)
  {
  }
}

const char* Daemon::name() noexcept
{
  return "BidCostPredictorDaemon";
}

} // namespace BidCostPredictor
} // namespace PredictorSvcs