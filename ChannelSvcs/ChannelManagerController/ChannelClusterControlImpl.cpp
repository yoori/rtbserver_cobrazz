
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <eh/Exception.hpp>
#include <Generics/ActiveObject.hpp>
#include<Stream/MemoryStream.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/CorbaClientAdapter.hpp>
#include <CORBACommons/ProcessControl.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelClusterControl.hpp>

#include<ChannelSvcs/ChannelManagerController/ChannelClusterControlImpl.hpp>


ChannelClusterSessionFactoryImpl::ChannelClusterSessionFactoryImpl(
  Generics::ActiveObjectCallback* callback) noexcept :
    callback_(ReferenceCounting::add_ref(callback))
{
}

CORBA::ValueBase*
ChannelClusterSessionFactoryImpl::create_for_unmarshal()
{
  try
  {
    return new AdServer::ChannelSvcs::ChannelClusterSessionImpl(callback_);
  }
  catch(const eh::Exception& e)
  {
    if(callback_)
    {
      Stream::Error ostr;
      ostr << "ChannelClusterSessionFactoryImpl::create_for_unmarshal: "
        "Catch eh::Exception on creating ChannelClusterSessionImpl. "
        ": " << e.what();
      callback_->error(ostr.str());
    }
    throw;
  }
}

void ChannelClusterSessionFactoryImpl::report_error(
    Generics::ActiveObjectCallback::Severity severity,
    const String::SubString& description,
    const char* error_code) noexcept
{
  callback_->report_error(severity, description, error_code);
}

namespace AdServer
{
namespace ChannelSvcs
{
  ChannelClusterSessionImpl::ChannelClusterSessionImpl(
    Generics::ActiveObjectCallback* callback) noexcept:
    callback_(ReferenceCounting::add_ref(callback))
  {
  }

  ChannelClusterSessionImpl::ChannelClusterSessionImpl(
    const AdServer::ChannelSvcs::ProcessControlDescriptionSeq& descr) noexcept:
    ChannelClusterSession(descr),
    callback_()
  {
  }

  CORBACommons::IProcessControl::ALIVE_STATUS
  ChannelClusterSessionImpl::is_alive() noexcept
  {
    CORBA::ULong i = 0;
    try
    {
      CORBACommons::IProcessControl::ALIVE_STATUS status =
        CORBACommons::IProcessControl::AS_READY;
      const AdServer::ChannelSvcs::ProcessControlDescriptionSeq& servers_ref =
        servers();
      for (; i < servers_ref.length(); ++i)
      {
        switch (servers_ref[i]->is_alive())
        {
        case CORBACommons::IProcessControl::AS_NOT_ALIVE:
          return CORBACommons::IProcessControl::AS_NOT_ALIVE;
        case CORBACommons::IProcessControl::AS_ALIVE:
          status = CORBACommons::IProcessControl::AS_ALIVE;
        default: ;
        }
      }
      return status;
    }
    catch(const CORBA::SystemException& e)
    {
      if(callback_)
      {
        Stream::Error ostr;
        ostr << "ChannelClusterSessionImpl::is_alive: Caught "
          "CORBA::SystemException on polling " << i << " server. "
          ": " << e;
        callback_->error(ostr.str());
      }
    }
    catch(const eh::Exception& e)
    {
      if(callback_)
      {
        Stream::Error ostr;
        ostr << "ChannelClusterSessionImpl::is_alive: Caught "
          "eh::Exception on polling " << i << " server. "
          ": " << e.what();
        callback_->error(ostr.str());
      }
    }
    return CORBACommons::IProcessControl::AS_NOT_ALIVE;
  }

  void ChannelClusterSessionImpl::shutdown(CORBA::Boolean wait) noexcept
  {
    const AdServer::ChannelSvcs::ProcessControlDescriptionSeq& servers_ref =
      servers();
    for (CORBA::ULong i = 0; i < servers_ref.length(); ++i)
    {
      try
      {
        servers_ref[i]->shutdown(wait);
      }
      catch(const CORBA::SystemException& e)
      {
        if(callback_)
        {
          Stream::Error ostr;
          ostr << "ChannelClusterSessionImpl::shutdown: Caught "
            "CORBA::SystemException on shutdown " << i << " server. "
            ": " << e;
          callback_->error(ostr.str());
        }
      }
      catch(const eh::Exception& e)
      {
        if(callback_)
        {
          Stream::Error ostr;
          ostr << "ChannelClusterSessionImpl::shutdown: Caught "
            "eh::Exception on shutdown " << i << " server. "
            ": " << e.what();
          callback_->error(ostr.str());
        }
      }
    }
  }

  char* ChannelClusterSessionImpl::comment() /*throw(CORBACommons::OutOfMemory)*/
  {
    const char* FUN ="ChannelClusterSessionImpl::comment";
    try
    {
      Stream::Error ostr;
      const AdServer::ChannelSvcs::ProcessControlDescriptionSeq& servers_ref =
        servers();
      for (CORBA::ULong i = 0; i < servers_ref.length(); ++i)
      {
        try
        {
          ostr << CORBACommons::CorbaClientAdapter::get_object_info(
            servers_ref[i]).host << ": ";
          ostr << servers_ref[i]->comment() << ';';
        }
        catch(const CORBACommons::OutOfMemory& e)
        {
          if(callback_)
          {
            Stream::Error ostr;
            ostr << FUN <<
              ": caught CORBACommons::OutOfMemory on getting comment for "
              << i << " server.";
            callback_->error(ostr.str());
          }
          ostr << "ChannelServer in critical state." << std::endl;
        }
        catch(const CORBA::SystemException& e)
        {
          if(callback_)
          {
            Stream::Error ostr;
            ostr << FUN
              << ": Caught CORBA::SystemException on getting comment for "
              << i << " server. : " << e;
            callback_->error(ostr.str());
          }
          ostr << "ChannelServer in not accessible." << std::endl;
        }
        catch(const eh::Exception& e)
        {
          if(callback_)
          {
            Stream::Error ostr;
            ostr << FUN <<
              ": Caught eh::Exception on getting comment for " << i
              << " server. : " << e.what();
            callback_->error(ostr.str());
          }
          ostr << "ChannelServer internal error." << std::endl;
        }
      }
      CORBA::String_var str;
      str << ostr.str();
      return str._retn();
    }
    catch(const eh::Exception& e)
    {
      throw CORBACommons::OutOfMemory();
    }
  }

  void ChannelClusterSessionFactory::init(
    CORBACommons::CorbaClientAdapter& corba_client_adapter,
    Generics::ActiveObjectCallback* callback)
    /*throw(eh::Exception)*/
  {
    try
    {
      CORBA::ValueFactoryBase_var cluster_session_factory =
        new ChannelClusterSessionFactoryImpl(callback);
      corba_client_adapter.register_value_factory(
        ChannelClusterSessionImpl::_tao_obv_static_repository_id(),
        cluster_session_factory);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "ChannelClusterSessionFactory::init: "
        "Can't init ChannelClusterSessionFactory. "
        ": " << e.what();
      throw Exception(ostr);
    }
  }

}
}

