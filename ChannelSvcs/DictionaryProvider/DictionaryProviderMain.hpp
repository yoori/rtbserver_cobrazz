#pragma once

#include <memory>

#include <eh/Exception.hpp>

#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Singleton.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <xsd/ChannelSvcs/DictionaryProviderConfig.hpp>
#include "DictionaryProviderImpl.hpp"


class DictionaryProviderApp_ :
  private Logging::LoggerCallbackHolder,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  DictionaryProviderApp_() /*throw(eh::Exception)*/;
  virtual ~DictionaryProviderApp_() noexcept{};

  void main(int& argc, char** argv) noexcept;

private:
  using Logging::LoggerCallbackHolder::callback;
  using Logging::LoggerCallbackHolder::logger;

  typedef std::unique_ptr<xsd::AdServer::Configuration::DictionaryProviderConfigType> ConfigPtr;

  void load_config_(const char* name) /*throw(Exception)*/;
  void init_logger_() /*throw(Exception)*/;
  void init_corba_() /*throw(Exception, CORBA::SystemException)*/;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;
  std::shared_ptr<AdServer::ChannelSvcs::DictionaryProviderImpl> server_impl_;
  std::shared_ptr<Generics::CompositeActiveObject> active_objects_;

  ConfigPtr configuration_;
};

typedef ReferenceCounting::SmartPtr<DictionaryProviderApp_> DictionaryProviderApp_var;

typedef Generics::Singleton<DictionaryProviderApp_, DictionaryProviderApp_var>
  DictionaryProviderApp;
