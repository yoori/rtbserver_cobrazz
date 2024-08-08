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

inline constexpr char DAEMON[] = "DAEMON";

} // namespace Aspect

namespace PredictorSvcs::BidCostPredictor
{

Daemon::Daemon(
  const std::string& pid_path,
  Logging::Logger* logger)
  : pid_path_(pid_path),
    logger_(ReferenceCounting::add_ref(logger)),
    pid_setter_(std::make_unique<PidSetter>(pid_path_))
{
}

void Daemon::run()
{
  try
  {
    {
      std::ostringstream stream;
      stream << FNS
             << "Start daemon";
      logger_->info(stream.str(), Aspect::DAEMON);
    }

    start();
    wait();

    {
      std::ostringstream stream;
      stream << FNS
             << "Daemon is success stopped";
      logger_->info(stream.str(), Aspect::DAEMON);
    }
  }
  catch (const eh::Exception& exc)
  {
    stop();
    wait();

    Stream::Error stream;
    stream << FNS
           << "Daemon is failed. Reason : "
           << exc.what();
    throw Exception(stream);
  }
  catch (...)
  {
    stop();
    wait();

    Stream::Error stream;
    stream << FNS
           << "Daemon is failed. Reason : Unknown error";
    throw Exception(stream);
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
      ostr << FNS
           << "Fork is failed";
      throw Exception(ostr);
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
      Stream::Error stream;
      stream << FNS
             << "It is impossible to ignore"
             << " the signal SIGHUP";
      throw Exception(stream);
    }

    if ((pid = fork()) < 0)
    {
      Stream::Error stream;
      stream << FNS
             << "Fork is failed";
      throw Exception(stream);
    }
    else if (pid != 0)
    {
      return;
    }

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr)
    {
      Stream::Error stream;
      stream << FNS
             << "getcwd is failed";
      throw Exception(stream);
    }

    if (chdir(cwd) < 0)
    {
      Stream::Error stream;
      stream << FNS
             << "chdir is failed";
      throw Exception(stream);
    }

    if (!pid_setter_->set())
    {
      throw Exception("Daemon already running");
    }

    is_running_ = true;

    start_logic();
  }
  catch (const eh::Exception& exc)
  {
    stop();

    Stream::Error stream;
    stream << FNS
           << "Can't start Daemon. Reason : "
           << exc.what();
    throw Exception(stream);
  }
  catch (...)
  {
    stop();

    Stream::Error stream;
    stream << FNS
           << "Can't start Daemon. Reason : Unknown error";
    throw Exception(stream);
  }
}

void Daemon::stop() noexcept
{
  if (!is_running_)
  {
    return;
  }

  try
  {
    const auto pid = ::getpid();
    if (kill(pid, SIGINT) == -1)
    {
      Stream::Error stream;
      stream << FNS
             << "Function kill is failed";
      logger_->critical(stream.str(), Aspect::DAEMON);
    }

    stop_logic();
  }
  catch(...)
  {
  }
}

void Daemon::wait() noexcept
{
  if (!is_running_)
  {
    return;
  }
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
      Stream::Error stream;
      stream << FNS
             << "sigwait is failed";
      logger_->critical(stream.str(), Aspect::DAEMON);
    }
    else
    {
      std::ostringstream stream;
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

} // namespace PredictorSvcs::BidCostPredictor