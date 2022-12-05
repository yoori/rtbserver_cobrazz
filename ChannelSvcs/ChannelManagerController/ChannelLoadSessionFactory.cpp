#include <list>
#include <vector>
#include <set>
#include <iterator>
#include <time.h>

#include <String/StringManip.hpp>
#include <Generics/Statistics.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelLoadSessionImpl.hpp>

#include "ChannelLoadSessionFactory.hpp"
#include "ChannelSessionFactory.hpp"
#include "ThreadHandlerTemplate.hpp"

const char* ChannelLoadSessionFactoryImpl::ASPECT =
  "ChannelLoadSessionFactoryImpl";

/**
 * ChannelLoadSessionFactoryImpl
 */

ChannelLoadSessionFactoryImpl::ChannelLoadSessionFactoryImpl(
    unsigned long count_threads,
    Generics::ActiveObjectCallback* callback) noexcept
  : callback_(ReferenceCounting::add_ref(callback)),
    task_runner_(new Generics::TaskRunner(callback_, count_threads))
{
  try
  {
    add_child_object(task_runner_);
    activate_object();
  }
  catch(const eh::Exception& e)
  {
    if(callback_)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << e.what();
      callback_->critical(ostr.str(), "ADS-IMPL-600");
    }
  }
}

ChannelLoadSessionFactoryImpl::~ChannelLoadSessionFactoryImpl() noexcept
{
}

CORBA::ValueBase* ChannelLoadSessionFactoryImpl::create_for_unmarshal()
{
  try
  {
    return new ::AdServer::ChannelSvcs::ChannelLoadSessionImpl(
      callback_, task_runner_.in());
  }
  catch(const eh::Exception& e)
  {
    if(callback_)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << e.what();
      callback_->critical(ostr.str(), "ADS-IMPL-601");
    }
    throw;
  }
}

void ChannelLoadSessionFactoryImpl::report_error(
    Generics::ActiveObjectCallback::Severity severity,
    const char* description,
    const char* error_code) 
    noexcept
{
  if(callback_)
  {
    Stream::Error ostr;
    ostr << __func__ << ": " << description;
    callback_->report_error(severity, ostr.str(), error_code);
  } 
}

