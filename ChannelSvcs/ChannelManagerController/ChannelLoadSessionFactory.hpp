#ifndef CHANNEL_SVCS_CHANNEL_LOAD_SESSION_FACTORY_HPP_
#define CHANNEL_SVCS_CHANNEL_LOAD_SESSION_FACTORY_HPP_


#include <ReferenceCounting/ReferenceCounting.hpp>
#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>

#include <Generics/CompositeActiveObject.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>

/**
 * ChannelLoadSessionFactoryImpl
 * implementation of corba valuetype factory
 */
class ChannelLoadSessionFactoryImpl :
  public virtual Generics::CompositeActiveObject,
  public virtual CORBACommons::ReferenceCounting::CorbaRefCountImpl<
    CORBA::ValueFactoryBase>
{
public:
  ChannelLoadSessionFactoryImpl(
    unsigned long count_threads,
    Generics::ActiveObjectCallback* callback)
    noexcept;

  ~ChannelLoadSessionFactoryImpl() noexcept;

  virtual CORBA::ValueBase* create_for_unmarshal();

  virtual void report_error(
    Generics::ActiveObjectCallback::Severity severity,
    const char* description,
    const char* error_code = 0) noexcept;
private:
  Generics::ActiveObjectCallback_var callback_;
  Generics::TaskRunner_var task_runner_;
  static const char* ASPECT;
};

typedef ReferenceCounting::SmartPtr<ChannelLoadSessionFactoryImpl> 
  ChannelLoadSessionFactoryImpl_var;

#endif /*CHANNEL_SVCS_CHANNEL_LOAD_SESSION_FACTORY_HPP_*/
