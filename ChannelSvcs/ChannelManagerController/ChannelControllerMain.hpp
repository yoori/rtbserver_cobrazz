
#ifndef AD_CHANNEL_SVCS_CONTROLLER_MAIN_HPP_
#define AD_CHANNEL_SVCS_CONTROLLER_MAIN_HPP_

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ProcessControl.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <xsd/ChannelSvcs/ChannelManagerControllerConfig.hpp>
#include "ChannelControllerImpl.hpp"

class ChannelControllerApp_:
  public AdServer::Commons::ProcessControlVarsLoggerImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  //DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  ChannelControllerApp_() /*throw(eh::Exception)*/;
  virtual ~ChannelControllerApp_() noexcept{};

  void main(int& argc, char** argv) noexcept;

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

  typedef std::unique_ptr<xsd::AdServer::Configuration::
    ChannelControllerConfigType> ConfigPtr;

private:
  void load_config_(const char* name) /*throw(Exception)*/;
  void init_corba_() /*throw(Exception)*/;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  ConfigPtr configuration_;

  AdServer::ChannelSvcs::ChannelControllerImpl_var controller_impl_;

  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

  ShutdownMutex shutdown_lock_;
};

typedef ReferenceCounting::SmartPtr<ChannelControllerApp_>
  ChannelControllerApp_var;

typedef Generics::Singleton<ChannelControllerApp_, ChannelControllerApp_var>
  ChannelControllerApp;

#endif /*AD_CHANNEL_SVCS_CONTROLLER_MAIN_HPP_*/
