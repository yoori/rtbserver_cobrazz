#pragma once

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>
#include <Sync/SyncPolicy.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <xsd/CampaignSvcs/BillingServerConfig.hpp>

#include "BillingServerCore.hpp"
#include "BillingServerGrpc.hpp"
#include "BillingServerImpl.hpp"

class BillingServerApp_
  : public AdServer::Commons::ProcessControlVarsLoggerImpl,
    public virtual Generics::CompositeActiveObject
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  BillingServerApp_() /*throw(eh::Exception)*/;

  void
  main(int argc, char** argv) noexcept;

protected:
  typedef Sync::Policy::PosixThread ShutdownSyncPolicy;

  typedef std::unique_ptr<
    AdServer::CampaignSvcs::BillingServerCore::BillingServerConfig>
    ConfigPtr;

protected:
  virtual ~BillingServerApp_() noexcept {};

  // ProcessControl interface
  virtual void
  shutdown(CORBA::Boolean wait_for_completion)
    /*throw(CORBA::SystemException)*/;

  virtual CORBACommons::IProcessControl::ALIVE_STATUS
  is_alive() /*throw(CORBA::SystemException)*/;

  const AdServer::CampaignSvcs::BillingServerCore::BillingServerConfig&
  config() const noexcept;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  AdServer::CampaignSvcs::BillingServerCore_var billing_server_core_;
  AdServer::CampaignSvcs::BillingServerImpl_var billing_server_impl_;
  AdServer::CampaignSvcs::BillingServerGrpc_var grpc_adapter_;

  ConfigPtr configuration_;

  ShutdownSyncPolicy::Mutex shutdown_lock_;
};

typedef ReferenceCounting::SmartPtr<BillingServerApp_>
  BillingServerApp_var;

typedef Generics::Singleton<BillingServerApp_, BillingServerApp_var>
  BillingServerApp;

// Inlines
inline
const AdServer::CampaignSvcs::BillingServerCore::BillingServerConfig&
BillingServerApp_::config() const noexcept
{
  return *configuration_.get();
}
