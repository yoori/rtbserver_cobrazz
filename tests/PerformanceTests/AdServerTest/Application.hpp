#pragma once

#include "ShutdownSignal.hpp"
#include "QuerySender.hpp"
#include <Logger/StreamLogger.hpp>
#include <Logger/DistributorLogger.hpp>

class Application :
  public Generics::ActiveObjectCallback,
  public ReferenceCounting::AtomicImpl
{

public:
  Application(unsigned long log_level, const char* cfg_file_path);

  ~Application() noexcept;


  void init() /*throw(eh::Exception)*/;

  void run();

  void shutdown();

  // from ActiveObjectCallback
  virtual void report_error(
    Generics::ActiveObjectCallback::Severity severity,
    const String::SubString& description, const char* error_code = 0) noexcept;

private:

  void _stop();
  void _log_execution_time();

  unsigned long log_level_;
  Configuration config_;
  Logging::Logger_var logger_;
  ShutdownSignal shutdown_signal_;
  Generics::TaskRunner_var task_runner_;
  Generics::Planner_var scheduler_;
  HTTP::PoolPolicy_var http_pool_policy_;
  HTTP::HttpActiveInterface_var http_pool_;
  QuerySender* sender_;
};
