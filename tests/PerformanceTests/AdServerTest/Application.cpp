
#include <array>
#include <ace/Reactor.h>
#include "Application.hpp"

//constants

namespace PerformanceConst
{
  const time_t DEFAULT_TIMEOUT                  = 1;
  const unsigned DEFAULT_CONNECTIONS_PER_THREAD = 5;
  const unsigned MAX_CONNECTIONS_PER_SERVER     = 100;
  const unsigned MAX_CONNECTIONS_PER_THREAD     = MAX_CONNECTIONS_PER_SERVER / 10;
};

// Utils
inline unsigned max(unsigned x, unsigned y)
{
  return (x > y)? x : y;
}

inline unsigned min(unsigned x, unsigned y)
{
  return (x < y)? x : y;
}




// SignalHandler class

SignalHandler::SignalHandler(Application* app) :
  app_(app)
{ }

int SignalHandler::handle_signal (int signum, siginfo_t *, ucontext_t * )
{
  switch(signum)
    {
    case SIGINT:
      app_->shutdown();
      break;
    case SIGTERM:
      app_->shutdown();
      break;
    default:
      break;
    }
  return 0;
}

// Application class

Application::Application(unsigned long log_level,
                         const char* cfg_file_path) :
  log_level_(log_level),
  config_(cfg_file_path),
  logger_(),
  sig_handler_(this),
  task_runner_(),
  scheduler_(),
  http_pool_policy_(),
  http_pool_(),
  sender_(0)
{ }

Application::~Application() noexcept
{
  delete sender_;
}

void Application::init() /*throw(eh::Exception)*/
{
  config_.read();

  std::array<Logging::Logger_var, 2> loggers{{
    Logging::Logger_var(new Logging::SeveritySelectorLogger(
      Logging::Logger_var(new Logging::OStream::Logger(
       Logging::OStream::Config(std::cerr, Logging::Logger::CRITICAL))),
       Logging::Logger::EMERGENCY, Logging::Logger::CRITICAL)),
      Logging::Logger_var(new Logging::SeveritySelectorLogger(
      Logging::Logger_var(new Logging::OStream::Logger(
        Logging::OStream::Config(std::cout, Logging::Logger::TRACE))),
      Logging::Logger::CRITICAL, Logging::Logger::TRACE))}};
  logger_ = new Logging::DistributorLogger(loggers.begin(), loggers.end());

  logger_->log_level(log_level_);

  task_runner_ =
    Generics::TaskRunner_var(new Generics::TaskRunner(this, config_.threads_number()));
  scheduler_   =
    Generics::Planner_var(new Generics::Planner(this, 0, true));
  unsigned connections_per_server =
      min(config_.client_config()->count,
          PerformanceConst::MAX_CONNECTIONS_PER_SERVER);
  unsigned connections_per_thread =
      min(max(config_.client_config()->count / 10,
              PerformanceConst::DEFAULT_CONNECTIONS_PER_THREAD),
          PerformanceConst::MAX_CONNECTIONS_PER_THREAD);

  // Use NsLookup timeout for HTTPPool if defined NSLookup request,
  // else use default_timeout
  time_t timeout =
      config_.client_config()->ns_request().get()?
      config_.client_config()->ns_request()->constraint->timeout:
      PerformanceConst::DEFAULT_TIMEOUT;

  HttpPoolPolicy* http_pool_policy =
      new HttpPoolPolicy(logger_,
                         connections_per_server,
                         connections_per_thread,
                         timeout);
  http_pool_policy_ = http_pool_policy;
  http_pool_ = CreatePool(http_pool_policy_.in(), task_runner_);
  sender_      = new QuerySender(config_, logger_, scheduler_, http_pool_, http_pool_policy);
  ACE_Reactor::instance()->register_handler(SIGTERM, &sig_handler_); // FIXME Get rid of ACE
  ACE_Reactor::instance()->register_handler(SIGINT, &sig_handler_); // FIXME Get rid of ACE
}

void Application::run()
{
  try
    {
      task_runner_->activate_object();
      scheduler_->activate_object();
      http_pool_->activate_object();
      sender_->start();
      sender_->wait();
      sender_->close();
      _stop();
      sender_->dump(true);
      sender_->dump_confluence_report();
      _log_execution_time();
    }
  catch (eh::Exception& e)
    {
      logger_->stream(Logging::Logger::CRITICAL) <<
        "Application stopping by exception: " << e.what();
      shutdown();
    }
  catch (...)
    {
      logger_->stream(Logging::Logger::CRITICAL) <<
        "Application stopping by unexpected exception";
      shutdown();
    }
}

void Application::shutdown()
{
  sender_->shutdown();
}

void Application::_stop()
{
  try
    {
      http_pool_->deactivate_object();
      http_pool_->wait_object();
      scheduler_->deactivate_object();
      scheduler_->wait_object();
      task_runner_->deactivate_object();
      task_runner_->wait_object();
    }
  catch (eh::Exception& e)
    {
      std::cerr << "Got exception when application stopping:" << e.what() << std::endl;
      exit(1);
    }
  catch (...)
    {
      std::cerr << "Got unexpected exception when application stopping" << std::endl;
      exit(1);
    }

}

void Application::_log_execution_time()
{
   logger_->stream( Logging::Logger::INFO) <<
     "Test execution time: " << sender_->get_total_duration();
}

void Application::report_error(
  Generics::ActiveObjectCallback::Severity,
  const String::SubString& description, const char*) noexcept
{

  logger_->stream( Logging::Logger::CRITICAL) <<
    "Application stopped due task runner error: " << description;
  shutdown();
}






