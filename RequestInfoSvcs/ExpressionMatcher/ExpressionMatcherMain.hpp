#pragma once

#include <memory>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Time.hpp>
#include <Generics/Singleton.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <SNMPAgent/SNMPAgentX.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include "ExpressionMatcherImpl.hpp"
#include "ExpressionMatcherStats.hpp"

class ExpressionMatcherApp_ :
  private Logging::LoggerCallbackHolder,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  typedef
    AdServer::RequestInfoSvcs::ExpressionMatcherImpl::ExpressionMatcherConfig
    Configuration;

public:
  ExpressionMatcherApp_() /*throw(eh::Exception)*/;

  void main(int& argc, char** argv) noexcept;

private:
  using Logging::LoggerCallbackHolder::callback;
  using Logging::LoggerCallbackHolder::logger;

  virtual ~ExpressionMatcherApp_() noexcept{};

  const Configuration& config() const noexcept;

  typedef std::unique_ptr<Configuration> ConfigPtr;

private:
  CORBACommons::CorbaServerAdapter_var corba_server_adapter_;
  CORBACommons::CorbaConfig corba_config_;

  AdServer::RequestInfoSvcs::ExpressionMatcherImpl_var
    expression_matcher_impl_;
  std::shared_ptr<Generics::CompositeActiveObject> active_objects_;

  ConfigPtr configuration_;
  SNMPAgentX::SNMPStatsImpl_var snmp_stat_provider_;
};

typedef ReferenceCounting::SmartPtr<ExpressionMatcherApp_>
  ExpressionMatcherApp_var;

typedef Generics::Singleton<ExpressionMatcherApp_, ExpressionMatcherApp_var>
  ExpressionMatcherApp;


/*
 * Inlines
 */

inline
const ExpressionMatcherApp_::Configuration&
ExpressionMatcherApp_::config() const noexcept
{
  return *configuration_.get();
}
