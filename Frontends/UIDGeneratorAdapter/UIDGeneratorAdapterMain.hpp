#ifndef UIDGENERATORADAPTERMAIN_HPP_
#define UIDGENERATORADAPTERMAIN_HPP_

#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Sync/SyncPolicy.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <Frontends/UIDGeneratorAdapterConfig.hpp>

#include "UIDGeneratorAdapterImpl.hpp"

class UIDGeneratorAdapterApp_:
  public AdServer::Commons::ProcessControlVarsLoggerImpl
{
public:
  virtual void
  shutdown(CORBA::Boolean wait_for_completion)
    /*throw(CORBA::SystemException)*/;

  virtual CORBACommons::IProcessControl::ALIVE_STATUS
  is_alive() /*throw(CORBA::SystemException)*/;

  void
  main(int& argc, char** argv) noexcept;

protected:
  typedef Sync::Policy::PosixThread ShutdownSyncPolicy;

  typedef AdServer::Frontends::UIDGeneratorAdapterImpl::Config
    Config;

  typedef std::unique_ptr<Config> ConfigPtr;

protected:
  virtual
  ~UIDGeneratorAdapterApp_() noexcept;

  void
  init_corba_() /*throw(Exception)*/;

  void
  read_config_(
    const char *filename,
    const char* argv0)
    /*throw(Exception, eh::Exception)*/;

protected:
  ConfigPtr config_;
  CORBACommons::CorbaConfig corba_config_;
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  AdServer::Frontends::UIDGeneratorAdapterImpl_var uid_generator_adapter_impl_;

  ShutdownSyncPolicy::Mutex shutdown_lock_;
};

typedef ReferenceCounting::SmartPtr<UIDGeneratorAdapterApp_>
  UIDGeneratorAdapterApp_var;

typedef Generics::Singleton<
  UIDGeneratorAdapterApp_, UIDGeneratorAdapterApp_var>
  UIDGeneratorAdapterApp;

#endif /*UIDGENERATORADAPTERMAIN_HPP_*/
