
#pragma once

#include <eh/Exception.hpp>

#include <Generics/Singleton.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <LogProcessing/SyncLogsConfig.hpp>

#include "SyncLogsImpl.hpp"

/**
 * Starts SyncLogs and handles configuration, logging and errors.
 */
class SyncLogsApp_:
  private Logging::LoggerCallbackHolder,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  SyncLogsApp_() /*throw(eh::Exception)*/;

  /**
   * Parses command line, opens the config file and starts the service.
   */
  void main(int& argc, char** argv) noexcept;

protected:
  ~SyncLogsApp_() noexcept override = default;

private:
  using Logging::LoggerCallbackHolder::callback;
  using Logging::LoggerCallbackHolder::logger;

  // Reads configuration from config XML tree.
  void read_config(const char* filename, const char* argv0)
    /*throw(Exception, eh::Exception)*/;

  AdServer::LogProcessing::SyncLogsImpl::ConfigurationPtr configuration_;
  AdServer::LogProcessing::SyncLogsImpl_var sync_logs_impl_;
};

using SyncLogsApp_var = ReferenceCounting::SmartPtr<SyncLogsApp_>;
using SyncLogsApp = Generics::Singleton<SyncLogsApp_, SyncLogsApp_var>;
