#ifndef _AD_SERVER_CHANNEL_SEARCH_SVCS_CHANNEL_SEARCH_SERVICE_MAIN_HPP_
#define _AD_SERVER_CHANNEL_SEARCH_SVCS_CHANNEL_SEARCH_SERVICE_MAIN_HPP_


#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <xsd/ChannelSearchSvcs/ChannelSearchServiceConfig.hpp>
#include "ChannelSearchServiceImpl.hpp"

class ChannelSearchServiceApp_ :
  public AdServer::Commons::ProcessControlVarsLoggerImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  ChannelSearchServiceApp_() /*throw(eh::Exception)*/;

  void main(int argc, char **argv) noexcept;

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
  virtual ~ChannelSearchServiceApp_() noexcept {}
  const xsd::AdServer::Configuration::ChannelSearchServiceConfigType&
    config() const noexcept;

  typedef std::unique_ptr<
    xsd::AdServer::Configuration::ChannelSearchServiceConfigType>
    ConfigPtr;

private:
  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

  ConfigPtr configuration_;
  ShutdownMutex shutdown_lock_;
  CORBACommons::CorbaConfig corba_config_;
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  AdServer::ChannelSearchSvcs::ChannelSearchServiceImpl_var service_impl_;
};

typedef ReferenceCounting::SmartPtr<ChannelSearchServiceApp_>
  ChannelSearchServiceApp_var;

typedef Generics::Singleton<ChannelSearchServiceApp_, ChannelSearchServiceApp_var>
  ChannelSearchServiceApp;

inline
const xsd::AdServer::Configuration::ChannelSearchServiceConfigType&
ChannelSearchServiceApp_::config() const noexcept
{
  return *configuration_.get();
}

#endif /* _AD_SERVER_CHANNEL_SEARCH_SVCS_CHANNEL_SEARCH_SERVICE_MAIN_HPP_ */

