
#include <array>

#include "Application.hpp"

namespace AdServerBenchmark
{
  const unsigned MAX_CONNECTIONS_PER_SERVER     = 100;
  const unsigned MAX_CONNECTIONS_PER_THREAD     = MAX_CONNECTIONS_PER_SERVER / 10;
};

void print_time(std::ostream& out, const Generics::Time& time)
{
  std::ostringstream time_str;
  unsigned long msec = time.tv_usec / 10;
  time_str << time.tv_sec << ".";
  time_str.fill('0');
  time_str.width(5);
  time_str << msec;
  out << time_str.str();
}

Application::Application(unsigned long log_level,
                         const char* cfg_file_path) :
  config_(cfg_file_path),
  log_level_(log_level),
  logger_(),
  task_runner_(),
  http_pool_policy_(),
  http_pool_()
{ }

Application::~Application() noexcept
{
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
  http_pool_policy_ =
      new HttpPoolPolicy(
        logger_,
        AdServerBenchmark::MAX_CONNECTIONS_PER_SERVER,
        AdServerBenchmark::MAX_CONNECTIONS_PER_THREAD,
        60);
  http_pool_ = CreatePool(http_pool_policy_, task_runner_);
}

void Application::run()
{

  _start();

  const Configuration::BenchmarkList& benchmark_configs =
      config_.benchmarks();
  Configuration::BenchmarkList::const_iterator
      bcfg_it(benchmark_configs.begin());
  Configuration::BenchmarkList::const_iterator
      bcfg_end(benchmark_configs.end());

  Statistics stats;
  BenchmarkStorage storage(&stats);
  unsigned long benchmark_idx = 0;

  for (; bcfg_it != bcfg_end; ++bcfg_it)
  {
    BenchmarkBase* benchmark(0);
    std::ostringstream dsc;
    dsc << "Benchmark#" << benchmark_idx <<
      " '" << (*bcfg_it)->description <<
      "' (" << (*bcfg_it)->frontend_type << ")";

    switch ((*bcfg_it)->frontend_type)
    {
    case FrontendType::nslookup:
    case FrontendType::userbind:
    case FrontendType::openrtb:
      logger_->stream(Logging::Logger::INFO) <<
        "Start " << dsc.str() << "...";
      benchmark = new Benchmark(*bcfg_it,
                                &storage,
                                http_pool_.in(),
                                logger_);
      break;
    default:
      logger_->stream(Logging::Logger::INFO) <<
        "Start generated " << dsc.str() << "...";
      benchmark = new GeneratedBenchmark(*bcfg_it,
                                         &storage,
                                         http_pool_.in(),
                                         logger_);
      break;
    }
    Benchmark_var benchmark_var(benchmark);
    if (!benchmark_var->run()) break;
    http_pool_policy_->wait_empty();

    std::ostringstream ostr;
    Report(
      dsc.str().c_str(),
      (*bcfg_it)->frontend_type,
      stats,
      ostr).dump();
    logger_->log(ostr.str(), Logging::Logger::INFO);
    logger_->stream(Logging::Logger::INFO) <<
        dsc.str() + " finished.";
    benchmark_idx++;
    stats.reset();
  }
  _stop();
}


void Application::report_error(
  Generics::ActiveObjectCallback::Severity,
  const String::SubString& description, const char*) noexcept
{
  logger_->stream(Logging::Logger::CRITICAL) <<
    "Application stopped due task runner error: " << description;
}


void Application::_start()
{
  task_runner_->activate_object();
  http_pool_->activate_object();
}

void Application::_stop()
{
  task_runner_->deactivate_object();
  task_runner_->wait_object();
  http_pool_->deactivate_object();
  http_pool_->wait_object();
}
