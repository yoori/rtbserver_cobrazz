
#ifndef AD_CHANNEL_SVCS_DICTIONARY_PROVIDER_HPP_
#define AD_CHANNEL_SVCS_DICTIONARY_PROVIDER_HPP_

#include <eh/Exception.hpp>

#include <Generics/Singleton.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>

#include <xsd/ChannelSvcs/DictionaryProviderConfig.hpp>
#include "DictionaryProviderImpl.hpp"


class DictionaryProviderApp_ :
  public AdServer::Commons::ProcessControlVarsLoggerImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  DictionaryProviderApp_() /*throw(eh::Exception)*/;
  virtual ~DictionaryProviderApp_() noexcept{};

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
    DictionaryProviderConfigType> ConfigPtr;

  void load_config_(const char* name) /*throw(Exception)*/;
  void init_logger_() /*throw(Exception)*/;
  void init_corba_() /*throw(Exception, CORBA::SystemException)*/;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;
  AdServer::ChannelSvcs::DictionaryProviderImpl_var server_impl_;

  ConfigPtr configuration_;

  typedef Sync::PosixMutex ShutdownMutex;
  typedef Sync::PosixGuard ShutdownGuard;

  ShutdownMutex shutdown_lock_;
};

typedef ReferenceCounting::SmartPtr<DictionaryProviderApp_> DictionaryProviderApp_var;

typedef Generics::Singleton<DictionaryProviderApp_, DictionaryProviderApp_var>
  DictionaryProviderApp;

#endif /*AD_CHANNEL_SVCS_DICTIONARY_PROVIDER_HPP_*/
