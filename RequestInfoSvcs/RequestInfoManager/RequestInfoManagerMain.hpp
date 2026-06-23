#pragma once

#include <memory>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <xsd/RequestInfoSvcs/RequestInfoManagerConfig.hpp>

#include "RequestInfoManager.hpp"
#include "RequestInfoManagerImpl.hpp"

class RequestInfoManagerApp_
  : private Logging::LoggerCallbackHolder,
    public virtual ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  RequestInfoManagerApp_() /*throw(eh::Exception)*/;

  void main(int& argc, char** argv) noexcept;

private:
  using Logging::LoggerCallbackHolder::callback;
  using Logging::LoggerCallbackHolder::logger;

  virtual ~RequestInfoManagerApp_() noexcept{};

  const AdServer::RequestInfoSvcs::RequestInfoManagerImpl::RequestInfoManagerConfig&
    config() const noexcept;

  typedef std::unique_ptr<
    AdServer::RequestInfoSvcs::RequestInfoManagerImpl::RequestInfoManagerConfig>
    ConfigPtr;

private:
  CORBACommons::CorbaConfig corba_config_;

  ConfigPtr configuration_;
};

typedef ReferenceCounting::SmartPtr<RequestInfoManagerApp_>
  RequestInfoManagerApp_var;

typedef Generics::Singleton<RequestInfoManagerApp_, RequestInfoManagerApp_var>
  RequestInfoManagerApp;


//////////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////////

inline
const AdServer::RequestInfoSvcs::RequestInfoManagerImpl::RequestInfoManagerConfig&
RequestInfoManagerApp_::config() const noexcept
{
  return *configuration_.get();
}
