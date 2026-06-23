#pragma once

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Commons/HttpServer/HttpServer.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <xsd/CampaignSvcs/BillingServerConfig.hpp>

#include "BillingServerCore.hpp"
#include "BillingServerGrpc.hpp"

class BillingServerApp_
  : private Logging::LoggerCallbackHolder,
    public virtual Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  BillingServerApp_() /*throw(eh::Exception)*/;

  void
  main(int argc, char** argv) noexcept;

  typedef std::unique_ptr<
    AdServer::CampaignSvcs::BillingServerCore::BillingServerConfig>
    ConfigPtr;

protected:
  virtual ~BillingServerApp_() noexcept {};

  const AdServer::CampaignSvcs::BillingServerCore::BillingServerConfig&
  config() const noexcept;

private:
  using Logging::LoggerCallbackHolder::callback;
  using Logging::LoggerCallbackHolder::logger;

  AdServer::CampaignSvcs::BillingServerCore_var billing_server_core_;

  ConfigPtr configuration_;
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
