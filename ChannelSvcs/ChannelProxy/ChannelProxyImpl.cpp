
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <set>
#include <vector>
#include <sstream>
#include <iostream>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ServantImpl.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/PathManip.hpp>

#include <ChannelSvcs/ChannelManagerController/ChannelManagerController.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelSessionFactory.hpp>
#include <ChannelSvcs/ChannelManagerController/ChannelLoadSessionFactory.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelServer.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUpdateStatLogger.hpp>
#include <ChannelSvcs/ChannelProxy/ChannelProxy_s.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>
#include <xsd/ChannelSvcs/ChannelProxyConfig.hpp>

#include "ChannelProxyImpl.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  namespace
  {
    const char ASPECT[] = "ChannelProxy";
  }

  /*************************************************************************
   * ChannelProxyImpl
   * **********************************************************************/
  ChannelProxyImpl::ChannelProxyImpl(
    Logging::Logger* init_logger,
    const ChannelProxyConfig* config)
    /*throw(Exception, eh::Exception)*/
    : active_(false),
      callback_(
        new Logging::ActiveObjectCallbackImpl(
          init_logger,
          "ChannelProxyImpl",
          ASPECT,
          "ADSC-ICON-0")),
      task_runner_(new Generics::TaskRunner(callback_, 2)),
      update_period_(20)
  {
    try
    {
      c_adapter_ = new CORBACommons::CorbaClientAdapter();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << ": Can't init CorbaClientAdapter - caught eh::Exception: "
          << ex.what();

      logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        ASPECT,
        "ADS-IMPL-0");
    }

    init_corba_refs_(config);
    init_logger_(config);
    init_();
  }

  void ChannelProxyImpl::init_corba_refs_(const ChannelProxyConfig* config)
    /*throw(eh::Exception, Exception)*/
  {
    if(config->ControllerCorbaRefs().present())
    {//controller mode
      logger()->log(String::SubString("Proxy works in controller mode."),
                  Logging::Logger::TRACE,
                  ASPECT);
      Config::CorbaConfigReader::read_multi_corba_ref(
          config->ControllerCorbaRefs().get(),
          corba_object_refs_);

      proxy_ = false;
      AdServer::ChannelSvcs::ChannelServerSessionFactory::init(
          *c_adapter_,
          0,
          &load_session_factory_,
          callback_,
          0,
          0,
          config->threads());
    }
    else if(config->ProxyCorbaRefs().present())
    {//proxy mode
      logger()->log(String::SubString("Proxy works in proxy mode."),
                  Logging::Logger::TRACE,
                  ASPECT);
      Config::CorbaConfigReader::read_multi_corba_ref(
          config->ProxyCorbaRefs().get(),
          corba_object_refs_);

      proxy_ = true;
    }
    else
    {
      throw Exception("ChannelProxyImpl::init_corba_refs_. "
          "Can't find references in ChannelProxyConfig");
    }
  }

  void ChannelProxyImpl::init_logger_(const ChannelProxyConfig* config)
    noexcept
  {
    try
    {
      proxy_logger_.reset();
      if(config->UpdateStatLogger().present())
      {
        const xsd::AdServer::Configuration::LogFlushPolicyType& statistic =
          config->UpdateStatLogger().get();
        std::string path;
        unsigned long size = 0;
        if(statistic.size().present())
        {
          size = statistic.size().get();
        }
        if(statistic.path().present())
        {
          path = config->log_root();
          PathManip::create_path(path, statistic.path().get().c_str());
        }
        proxy_logger_ = new ChannelUpdateStatLogger(
          size,
          statistic.period(),
          path.c_str());
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "ChannelProxyImpl::init_logger_: "
        "caught eh::Exception. : "
        << e.what();

      logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        ASPECT,
        "ADS-IMPL-34");
    }
  }

  ChannelProxyImpl::~ChannelProxyImpl() noexcept
  {
    deactivate_object();
    wait_object();
    logger()->log(String::SubString("ChannelProxyImpl destroying"),
        Logging::Logger::TRACE,
        ASPECT);
  }

  /* init proxy on startup*/
  void ChannelProxyImpl::init_() /*throw(Exception)*/
  {
    try
    {
      LoadSessionPoolConfig pool_config(
        c_adapter_.in(),
        load_session_factory_,
        proxy_);

      std::copy(
        corba_object_refs_.begin(),
        corba_object_refs_.end(),
        std::back_inserter(pool_config.iors_list));
      pool_config.timeout = Generics::Time(10);
      load_pool_ = LoadSessionPoolPtr(new LoadSessionPool(
        pool_config,
        CORBACommons::ChoosePolicyType::PT_BAD_SWITCH));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error err;
      err << "ChannelProxyImpl::init_: Caught eh::Exception " << e.what();
      throw Exception(err);
    }
  }

  void
  ChannelProxyImpl::activate_object()
    /*throw(ActiveObject::AlreadyActive,
      ActiveObject::Exception,
      eh::Exception)*/
  {
    WriteGuard_ guard(lock_);

    if(active_)
    {
      throw ActiveObject::AlreadyActive(
          "ChannelProxyImpl::ChannelProxy already active");
    }

    try
    {
      if(task_runner_) task_runner_->activate_object();
      if(scheduler_) scheduler_->activate_object();
    }
    catch(...)
    {
      try
      {
        if(task_runner_ && task_runner_->active())
          task_runner_->deactivate_object();
        if(scheduler_ && scheduler_->active())
          scheduler_->deactivate_object();
      }
      catch(...)
      {
      }
      throw;
    }

    active_ = true;
  }

  void ChannelProxyImpl::deactivate_object()
    /*throw(ActiveObject::Exception, eh::Exception)*/
  {
    WriteGuard_ guard(lock_);
    if(active_)
    {
      if(task_runner_) task_runner_->deactivate_object();
      if(scheduler_) scheduler_->deactivate_object();
      if(load_session_factory_) load_session_factory_->deactivate_object();
    }
  }

  void ChannelProxyImpl::wait_object()
    /*throw(ActiveObject::Exception, eh::Exception)*/
  {
    WriteGuard_ guard(lock_);
    if(task_runner_) task_runner_->wait_object();
    if(scheduler_) scheduler_->wait_object();
    if(load_session_factory_) load_session_factory_->wait_object();
    active_ = false;
  }

  void ChannelProxyImpl::dump_logs_() noexcept
  {
    const char* FN="ChannelProxyImpl::dump_logs_";
    try
    {
      proxy_logger_->flush_if_required();
    }
    catch(const ChannelUpdateStatLogger::Exception& e)
    {
      Stream::Error ostr;
      ostr << FN << ": caught ChannelColoUpdateStatLogger::Exception "
        "on flushing logs. : " << e.what();

      logger()->log(
        ostr.str(),
        Logging::Logger::ERROR,
        ASPECT,
        "ADS-IMPL-31");
    }
  }

  void ChannelProxyImpl::log_update_(unsigned long colo, const char* version)
    noexcept
  {
    const char* FN="ChannelProxyImpl::log_update_";
    if(proxy_logger_)
    {
      try
      {
        std::string log_version = version;
        if(log_version.size())
        {
          proxy_logger_->process_config_update(colo, log_version);
        }
        Task_var msg = new LogTask(this, task_runner_);
        task_runner_->enqueue_task(msg);
      }
      catch(const ChannelUpdateStatLogger::Exception& e)
      {
        logger()->sstream(Logging::Logger::WARNING, ASPECT, "ADS-IMPL-32")
          << FN << ": caught ChannelColoUpdateStatLogger::Exception. "
          ": " << e.what();
      }
      catch(const eh::Exception& e)
      {
        logger()->sstream(Logging::Logger::WARNING, ASPECT, "ADS-IMPL-32")
          <<  FN << ": caught eh::Exception. : " << e.what();
      }
    }
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelUpdate_v33/check:1.0
  //
  void ChannelProxyImpl::check(
    const ::AdServer::ChannelSvcs::ChannelUpdate_v33::CheckQuery& query,
    ::AdServer::ChannelSvcs::ChannelUpdate_v33::CheckData_out data)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    const char* FN="ChannelProxyImpl::check";
    try
    {
      LoadSessionPool::ObjectHandlerType load_session =
        load_pool_->get_object();
      try
      {
        load_session->check(query, data);
      }
      catch(const AdServer::ChannelSvcs::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FN << ": ChannelSvcs::ImplementationException: " << e.description;
        load_session.release_bad(ostr.str());
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(
            ostr.str());
      }
      catch (const AdServer::ChannelSvcs::NotConfigured& ex)
      {
        Stream::Error ostr;
        ostr << FN << ": ChannelSvcs::NotConfigured. "
          ": " << ex.description;
        load_session.release_bad(ostr.str());
        CORBACommons::throw_desc<
          ChannelSvcs::NotConfigured>(ostr.str());
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FN << ": caught CORBA::SystemException. : " << e;
        (proxy_ ?
          logger()->log(
            ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-ECON-0")
          :
          logger()->log(
            ostr.str(),
            Logging::Logger::CRITICAL,
            ASPECT,
            "ADS-ICON-0")
         );
        load_session.release_bad(ostr.str());
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(ostr.str());
      }
    }
    catch(const LoadSessionPool::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FN << ": caught LoadSessionPool::Exception: " << ex.what();
      CORBACommons::throw_desc<ChannelSvcs::ImplementationException>(
          ostr.str());
    }
    log_update_(query.colo_id, query.version.in());
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelServerControl/update_triggers:1.0
  //
  void
  ChannelProxyImpl::update_triggers(
    const ::AdServer::ChannelSvcs::ChannelIdSeq& ids,
    ::AdServer::ChannelSvcs::ChannelCurrent::UpdateData_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    const char* FN="ChannelProxyImpl::update_triggers";
    try
    {
      LoadSessionPool::ObjectHandlerType load_session =
        load_pool_->get_object();
      try
      {
        load_session->update_triggers(ids, result);
      }
      catch(const AdServer::ChannelSvcs::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FN << ": AdServer::ChannelSvcs::ImplementationException: "
          << e.description;
        load_session.release_bad(ostr.str());
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(
            ostr.str());
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FN << ": Caught CORBA::SystemException. : " << e;
        (proxy_ ?
          logger()->log(
            ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-ECON-0")
          :
          logger()->log(
            ostr.str(),
            Logging::Logger::CRITICAL,
            ASPECT,
            "ADS-ICON-0")
        );
        load_session.release_bad(ostr.str());
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(
            ostr.str());
      }
    }
    catch(const LoadSessionPool::Exception& ex)
    {
      (proxy_ ?
        logger()->sstream(Logging::Logger::ERROR, ASPECT,
          "ADS-ECON-0")
        :
        logger()->sstream(Logging::Logger::CRITICAL, ASPECT,
          "ADS-ICON-0")
      )
        << FN << ": caught LoadSessionPool::Exception " << ex.what();
      throw ChannelSvcs::NotConfigured(
        "Session is not initialized or dead");
    }
  }

  void ChannelProxyImpl::update_all_ccg(
    const AdServer::ChannelSvcs::ChannelCurrent::CCGQuery& in,
    AdServer::ChannelSvcs::ChannelCurrent::PosCCGResult_out result)
    /*throw(AdServer::ChannelSvcs::ImplementationException,
      AdServer::ChannelSvcs::NotConfigured)*/
  {
    const char* FN="ChannelProxyImpl::update_all_ccg";
    try
    {
      LoadSessionPool::ObjectHandlerType load_session =
        load_pool_->get_object();
      try
      {
        load_session->update_all_ccg(in, result);
      }
      catch(const AdServer::ChannelSvcs::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FN << ": ChannelSvcs::ImplementationException: "
          << e.description;
        load_session.release_bad(ostr.str());
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(
            ostr.str());
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FN << ": Caught CORBA::SystemException. : " << e;
        (proxy_ ?
          logger()->log(
            ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-ECON-0")
        :
          logger()->log(
            ostr.str(),
            Logging::Logger::CRITICAL,
            ASPECT,
            "ADS-ICON-0")
        );
        load_session.release_bad(ostr.str());
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(
            ostr.str());
      }
    }
    catch(const LoadSessionPool::Exception& ex)
    {
      (proxy_ ?
        logger()->sstream(Logging::Logger::ERROR, ASPECT, "ADS-ECON-0")
      :
        logger()->sstream(Logging::Logger::CRITICAL, ASPECT, "ADS-ICON-0")
      )
        << FN << ": caught LoadSessionPool::Exception: " << ex.what();
      throw AdServer::ChannelSvcs::NotConfigured(
        "Session is not initialized or dead");
    }
  }

  //
  // IDL:AdServer/ChannelSvcs/ChannelProxy/get_count_chunks:1.0
  //
  ::CORBA::ULong ChannelProxyImpl::get_count_chunks()
  {
    const char* FN="ChannelProxyImpl::get_count_chunks";
    try
    {
      LoadSessionPool::ObjectHandlerType load_session =
        load_pool_->get_object();
      try
      {
        return load_session->get_count_chunks();
      }
      catch(const AdServer::ChannelSvcs::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FN << ": ChannelSvcs::ImplementationException: "
          << e.description;
        load_session.release_bad(ostr.str());
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(
            ostr.str());
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FN << ": Caught CORBA::SystemException. : " << e;
        (proxy_ ?
          logger()->log(
            ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-ECON-0")
        :
          logger()->log(
            ostr.str(),
            Logging::Logger::CRITICAL,
            ASPECT,
            "ADS-ICON-0")
        );
        load_session.release_bad(ostr.str());
        CORBACommons::throw_desc<
          ChannelSvcs::ImplementationException>(
            ostr.str());
      }
    }
    catch(const LoadSessionPool::Exception& ex)
    {
      (proxy_ ?
        logger()->sstream(Logging::Logger::ERROR, ASPECT, "ADS-ECON-0")
      :
        logger()->sstream(Logging::Logger::CRITICAL, ASPECT, "ADS-ICON-0")
      )
        << FN << ": caught LoadSessionPool::Exception: " << ex.what();
      throw AdServer::ChannelSvcs::NotConfigured(
        "Session is not initialized or dead");
    }
    return 0; // never reach
  }

} /* ChannelSvcs */
} /* AdServer */
