#ifndef AD_CHANNEL_SVCS_SERVER_MAIN_HPP_
#define AD_CHANNEL_SVCS_SERVER_MAIN_HPP_

#include <eh/Exception.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/TaskPool.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>
#include <UServerUtils/Manager.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ProcessControl.hpp>
#include <CORBACommons/Stats_s.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <xsd/ChannelSvcs/ChannelServerConfig.hpp>

#include "ChannelServerImpl.hpp"

class ChannelServerApp_ final:
  public AdServer::Commons::ProcessControlVarsLoggerImpl,
  public Generics::CompositeActiveObject
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

private:
  using ManagerCoro = UServerUtils::Manager;
  using ManagerCoro_var = UServerUtils::Manager_var;
  using ShutdownMutex = Sync::PosixMutex;
  using ShutdownGuard = Sync::PosixGuard;

public:
  ChannelServerApp_() /*throw(eh::Exception)*/;
  virtual ~ChannelServerApp_() noexcept{};

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

  /**
   * Called by is_alive implementation to determine AS_ALIVE or AS_READY
   * status
   */
  virtual bool
  is_ready_() noexcept;

    /**
     * Provides extended status of the process to the caller
     * @return empty string, may be reimplemented in derived classes
     */
  virtual char*
  comment() /*throw(CORBACommons::OutOfMemory)*/;

private:

  typedef std::unique_ptr<xsd::AdServer::Configuration::
    ChannelServerConfigType> ConfigPtr;

private:
  void load_config_(const char* name) /*throw(Exception)*/;
  void init_corba_() /*throw(Exception, CORBA::SystemException)*/;
  void init_coro_();

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;

  CORBACommons::CorbaConfig corba_config_;

  ConfigPtr configuration_;

  AdServer::ChannelSvcs::ChannelServerCustomImpl_var server_impl_;

  ShutdownMutex shutdown_lock_;
};

typedef ReferenceCounting::SmartPtr<ChannelServerApp_> ChannelServerApp_var;

typedef Generics::Singleton<ChannelServerApp_, ChannelServerApp_var>
  ChannelServerApp;


//////////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////////

inline
bool ChannelServerApp_::is_ready_() noexcept
{
  if(server_impl_.in())
  {
    return server_impl_->ready();
  }
  else
  {
    return false;
  }
}

inline
char* ChannelServerApp_::comment() /*throw(CORBACommons::OutOfMemory)*/
{
  if(server_impl_.in())
  {
    return server_impl_->comment();
  }
  else
  {
    return 0;
  }
}


#endif /*AD_CHANNEL_SVCS_SERVER_MAIN_HPP_*/

