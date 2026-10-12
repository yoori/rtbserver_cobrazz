#pragma once


#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <SNMPAgent/SNMPAgentX.hpp>


#include <xsd/LogProcessing/LogGeneralizerConfig.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Commons/Grpc/DbAccess.hpp>
#include <Commons/SignalActiveObject.hpp>
#include <Commons/HttpServer/HttpServer.hpp>
#include "LogGeneralizerImpl.hpp"
#include "LogGeneralizerGrpc.hpp"
#include "ProcStatImpl.hpp"

class LogGeneralizerApp_:
  private Logging::LoggerCallbackHolder,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  LogGeneralizerApp_() /*throw(eh::Exception)*/;

  void main(int& argc, char** argv, AdServer::Commons::SignalActiveObject& signals) noexcept;

private:
  void shutdown();
  using Logging::LoggerCallbackHolder::callback;
  using Logging::LoggerCallbackHolder::logger;
  typedef xsd::AdServer::Configuration::LogGeneralizerConfigType
    LogGeneralizerConfigType;

  typedef std::unique_ptr<LogGeneralizerConfigType> ConfigPtr;

  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

  virtual ~LogGeneralizerApp_() noexcept {}

  const LogGeneralizerConfigType& config() const noexcept;

  ConfigPtr configuration_;

  ShutdownMutex shutdown_lock_;
  bool shutdown_requested_ = false;

  AdServer::LogProcessing::LogGeneralizerImpl_var log_generalizer_impl_;
  AdServer::LogProcessing::ProcStatImpl_var proc_stat_impl_;
  AdServer::Commons::HttpServer::HttpServer_var http_server_;
  std::unique_ptr<AdServer::LogProcessing::LogGeneralizerGrpcService> grpc_service_;
  std::shared_ptr<AdServer::Grpc::DbAccessServer> db_access_server_;
  SNMPAgentX::SNMPStatsImpl_var snmp_stat_impl_;
};

typedef ReferenceCounting::SmartPtr<LogGeneralizerApp_>
  LogGeneralizerApp_var;

typedef Generics::Singleton<LogGeneralizerApp_, LogGeneralizerApp_var>
  LogGeneralizerApp;

//////////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////////

inline
const LogGeneralizerApp_::LogGeneralizerConfigType&
LogGeneralizerApp_::config() const noexcept
{
  return *configuration_.get();
}
