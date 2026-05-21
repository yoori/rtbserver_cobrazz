#pragma once

#include <memory>

#include <eh/Exception.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/HttpServer/HttpServer.hpp>

#include <xsd/ChannelSvcs/ChannelServerConfig.hpp>

#include "ChannelServerCore.hpp"
#include "ChannelServerCustomImpl.hpp"
#include "ChannelServerGrpc.hpp"

class ChannelServerApp_ :
  private Logging::LoggerCallbackHolder,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  ChannelServerApp_() /*throw(eh::Exception)*/;
  virtual ~ChannelServerApp_() noexcept{};

  void main(int& argc, char** argv) noexcept;

private:
  using Logging::LoggerCallbackHolder::callback;
  using Logging::LoggerCallbackHolder::logger;

  typedef std::unique_ptr<xsd::AdServer::Configuration::
    ChannelServerConfigType> ConfigPtr;

private:
  void load_config_(const char* name) /*throw(Exception)*/;
  void init_corba_() /*throw(Exception, CORBA::SystemException)*/;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  ConfigPtr configuration_;
  AdServer::ChannelSvcs::ChannelServerCorePtr server_core_;
  AdServer::ChannelSvcs::ChannelServerCustomImpl_var server_impl_;
  std::shared_ptr<AdServer::ChannelSvcs::ChannelServerGrpc> grpc_adapter_;
  AdServer::Commons::HttpServer::HttpServer_var http_server_;
  std::shared_ptr<Generics::CompositeActiveObject> active_objects_;
};

typedef ReferenceCounting::SmartPtr<ChannelServerApp_> ChannelServerApp_var;

typedef Generics::Singleton<ChannelServerApp_, ChannelServerApp_var>
  ChannelServerApp;
