#ifndef AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_MAIN_HPP
#define AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_MAIN_HPP


#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <SNMPAgent/SNMPAgentX.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/StatsImpl.hpp>

#include <xsd/LogProcessing/LogGeneralizerConfig.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include "LogGeneralizerImpl.hpp"
#include "ProcStatImpl.hpp"

class LogGeneralizerApp_:
  public AdServer::Commons::ProcessControlVarsLoggerImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  LogGeneralizerApp_() /*throw(eh::Exception)*/;

  void main(int &argc, char **argv) noexcept;

protected:
  //
  // IDL:CORBACommons/IProcessControl/shutdown:1.0
  //
  virtual void shutdown(CORBA::Boolean wait_for_completion)
    /*throw(CORBA::SystemException)*/;

  //
  // IDL:CORBACommons/IProcessControl/is_alive:1.0
  //
  virtual CORBACommons::IProcessControl::ALIVE_STATUS
  is_alive() /*throw(CORBA::SystemException)*/;

private:
  typedef xsd::AdServer::Configuration::LogGeneralizerConfigType
    LogGeneralizerConfigType;

  typedef std::unique_ptr<LogGeneralizerConfigType> ConfigPtr;

  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

  virtual ~LogGeneralizerApp_() noexcept {}

  const LogGeneralizerConfigType& config() const noexcept;

  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  ConfigPtr configuration_;

  ShutdownMutex shutdown_lock_;

  AdServer::LogProcessing::LogGeneralizerImpl_var log_generalizer_impl_;
  AdServer::LogProcessing::ProcStatImpl_var proc_stat_impl_;
  CORBACommons::POA_ProcessStatsControl_var proc_stat_ctrl_;
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

#endif /* AD_SERVER_LOG_PROCESSING_LOG_GENERALIZER_MAIN_HPP */

