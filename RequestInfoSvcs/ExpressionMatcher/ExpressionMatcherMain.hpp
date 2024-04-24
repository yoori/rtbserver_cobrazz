#ifndef _CAMPAIGN_SVCS_EXPRESSION_MATCHER_MAIN_HPP_
#define _CAMPAIGN_SVCS_EXPRESSION_MATCHER_MAIN_HPP_

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>
#include <SNMPAgent/SNMPAgentX.hpp>
#include <UServerUtils/Grpc/Manager.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include "ExpressionMatcherImpl.hpp"
#include "ExpressionMatcherStats.hpp"

class ExpressionMatcherApp_ :
  public AdServer::Commons::ProcessControlVarsLoggerImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  using ManagerCoro = UServerUtils::Grpc::Manager;
  using ManagerCoro_var = UServerUtils::Grpc::Manager_var;
  using Configuration = AdServer::RequestInfoSvcs::ExpressionMatcherImpl::ExpressionMatcherConfig;

public:
  ExpressionMatcherApp_() /*throw(eh::Exception)*/;

  void main(int& argc, char** argv) noexcept;

protected:
  /* ProcessControl interface */
  virtual void shutdown(CORBA::Boolean wait_for_completion)
    /*throw(CORBA::SystemException)*/;

  virtual CORBACommons::IProcessControl::ALIVE_STATUS
  is_alive() /*throw(CORBA::SystemException)*/;

private:
  virtual ~ExpressionMatcherApp_() noexcept{};

  const Configuration& config() const noexcept;

  typedef std::unique_ptr<Configuration> ConfigPtr;

private:
  ManagerCoro_var manager_;

  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  AdServer::RequestInfoSvcs::ExpressionMatcherImpl_var
    expression_matcher_impl_;

  ConfigPtr configuration_;
  SNMPAgentX::SNMPStatsImpl_var snmp_stat_provider_;

  typedef Sync::Policy::PosixThread SyncPolicy;
  typedef SyncPolicy::Mutex ShutdownMutex;
  typedef SyncPolicy::WriteGuard ShutdownGuard;

  ShutdownMutex shutdown_lock_;
};

typedef ReferenceCounting::SmartPtr<ExpressionMatcherApp_>
  ExpressionMatcherApp_var;

typedef Generics::Singleton<ExpressionMatcherApp_, ExpressionMatcherApp_var>
  ExpressionMatcherApp;


/*
 * Inlines
 */

inline
const ExpressionMatcherApp_::Configuration&
ExpressionMatcherApp_::config() const noexcept
{
  return *configuration_.get();
}

#endif /*_CAMPAIGN_SVCS_EXPRESSION_MATCHER_MAIN_HPP_*/
