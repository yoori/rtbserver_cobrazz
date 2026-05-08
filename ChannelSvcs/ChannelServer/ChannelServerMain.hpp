#pragma once

#include <memory>

#include <eh/Exception.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ProcessControl.hpp>
#include <Commons/HttpServer/HttpServer.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <xsd/ChannelSvcs/ChannelServerConfig.hpp>

#include "ChannelServerCore.hpp"
#include "ChannelServerCustomImpl.hpp"
#include "ChannelServerGrpc.hpp"

class ChannelServerApp_ :
  public AdServer::Commons::ProcessControlVarsLoggerImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

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
  void stop_() noexcept;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  ConfigPtr configuration_;
  AdServer::ChannelSvcs::ChannelServerCorePtr server_core_;
  AdServer::ChannelSvcs::ChannelServerCustomImpl_var server_impl_;
  AdServer::ChannelSvcs::ChannelServerGrpc_var grpc_adapter_;
  AdServer::Commons::HttpServer::HttpServer_var http_server_;

  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

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
  if(server_core_)
  {
    return server_core_->ready();
  }
  else
  {
    return false;
  }
}

inline
char* ChannelServerApp_::comment() /*throw(CORBACommons::OutOfMemory)*/
{
  if(server_core_)
  {
    try
    {
      CORBA::String_var result;
      result << server_core_->comment();
      return result._retn();
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "ChannelServerApp_::comment: caught eh::Exception: " <<
        e.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        "ChannelServer",
        "ADS-IMPL-44");
      throw CORBACommons::OutOfMemory();
    }
  }
  else
  {
    return 0;
  }
}
