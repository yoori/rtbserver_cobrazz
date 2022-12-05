#include <list>
#include <vector>
#include <set>

#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>

#include "UserBindControllerImpl.hpp"

namespace
{
  const Generics::Time REINIT_SOURCES_INTERVAL = Generics::Time(10); // 10 sec
}

namespace Aspect
{
  const char USER_BIND_CONTROLLER[] = "UserBindController";
}

namespace AdServer
{
namespace UserInfoSvcs
{
  // UserBindControllerImpl
  UserBindControllerImpl::UserBindControllerImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const UserBindControllerConfig& user_bind_controller_config)
    /*throw(Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_, 1)),
      user_bind_controller_config_(user_bind_controller_config)
  {
    static const char* FUN = "UserBindControllerImpl::UserBindControllerImpl()";

    try
    {
      add_child_object(task_runner_);
      add_child_object(scheduler_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": CompositeActiveObject::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

      fill_refs_();

      Task_var msg = new InitUserBindSourceTask(this, 0);
      task_runner_->enqueue_task(msg);
    }
    catch(const Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't instantiate object. Caught Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't instantiate object. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  UserBindControllerImpl::~UserBindControllerImpl() noexcept
  {}

  AdServer::UserInfoSvcs::UserBindDescriptionSeq*
  UserBindControllerImpl::get_session_description()
    /*throw(AdServer::UserInfoSvcs::UserBindController::ImplementationException,
      AdServer::UserInfoSvcs::UserBindController::NotReady)*/
  {
    AdServer::UserInfoSvcs::UserBindDescriptionSeq_var result =
      new AdServer::UserInfoSvcs::UserBindDescriptionSeq();
    fill_user_bind_server_descr_seq_(*result);
    return result._retn();
  }

  CORBACommons::IProcessControl::ALIVE_STATUS
  UserBindControllerImpl::get_status() noexcept
  {
    try
    {
      UserBindConfig_var user_bind_config;

      {
        SyncPolicy::ReadGuard lock(lock_);
        user_bind_config = user_bind_config_;
      }

      if(!user_bind_config->all_ready)
      {
        return CORBACommons::IProcessControl::AS_NOT_ALIVE;
      }
      else
      {
        UserBindConfig_var user_bind_config;

        {
          SyncPolicy::ReadGuard lock(lock_);
          user_bind_config = user_bind_config_;
        }

        return CORBACommons::IProcessControl::AS_ALIVE; // get_user_info_cluster_control_()->is_alive();
      }
    }
    catch(...)
    {
      return CORBACommons::IProcessControl::AS_NOT_ALIVE;
    }
  }

  char*
  UserBindControllerImpl::get_comment() /*throw(CORBACommons::OutOfMemory)*/
  {
    return 0;
  }

  void
  UserBindControllerImpl::fill_refs_()
    /*throw(Exception)*/
  {
    //static const char* FUN = "UserBindControllerImpl::fill_refs_";

    typedef UserBindControllerConfig::UserBindServerHost_sequence
      UserBindServerHostArray;

    const UserBindServerHostArray& user_bind_server_hosts =
      user_bind_controller_config_.UserBindServerHost();

    UserBindConfig_var user_bind_config = new UserBindConfig();

    user_bind_config->first_all_ready = false;
    user_bind_config->all_ready = false;

    for(UserBindServerHostArray::const_iterator it =
          user_bind_server_hosts.begin();
        it != user_bind_server_hosts.end(); ++it)
    {
      UserBindServerRef user_bind_server_host;
      CORBACommons::CorbaObjectRef corba_object_ref;

      try
      {
        Config::CorbaConfigReader::read_corba_ref(
          it->UserBindServerControlRef(), corba_object_ref);

        user_bind_server_host.process_control = AdServer::Commons::CorbaObject<
          CORBACommons::IProcessControl>(
            corba_client_adapter_,
            corba_object_ref);
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << "Can't resolve corba object ref '" <<
          corba_object_ref.object_ref <<
          "', caught CORBA::SystemException: " << ex;
        throw Exception(ostr);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << "Can't resolve corba object ref '" <<
          corba_object_ref.object_ref <<
          "', caught eh::Exception: " << e.what();
        throw Exception(ostr);
      }

      // load UserBindMapper ref
      try
      {
        Config::CorbaConfigReader::read_corba_ref(
          it->UserBindServerRef(), corba_object_ref);

        user_bind_server_host.user_bind_server = AdServer::Commons::CorbaObject<
          AdServer::UserInfoSvcs::UserBindServer>(
            corba_client_adapter_,
            corba_object_ref);

        if(it->UserBindServerRef().name().present())
        {
          user_bind_server_host.host_name =
            it->UserBindServerRef().name().get();
        }
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << "Can't resolve UserBindServer corba object ref: '" <<
          corba_object_ref.object_ref <<
          "', caught CORBA::Exception: " << ex;
        throw Exception(ostr);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << "Can't resolve UserBindServer corba object ref: '" <<
          corba_object_ref.object_ref << "': " <<
          e.what();
        throw Exception(ostr);
      }

      user_bind_server_host.ready = false;

      user_bind_config->user_bind_servers.push_back(
        user_bind_server_host);
    }

    SyncPolicy::WriteGuard lock(lock_);
    user_bind_config_.swap(user_bind_config);
  }

  bool
  UserBindControllerImpl::get_user_bind_server_sources_(
    UserBindConfig* user_bind_config)
    /*throw(Exception)*/
  {
    std::ostringstream tracing;
    std::ostringstream errors;

    bool has_errors = false;
    bool all_ready = true;

    UserBindServerRefArray& user_bind_servers =
      user_bind_config->user_bind_servers;

    for(UserBindServerRefArray::iterator host_it =
          user_bind_servers.begin();
        host_it != user_bind_servers.end(); ++host_it)
    {
      try
      {
        CORBACommons::IProcessControl::ALIVE_STATUS status =
          host_it->process_control->is_alive();

        if(status != CORBACommons::IProcessControl::AS_READY)
        {
          // server isn't ready
          host_it->ready = false;
          host_it->chunks.clear();

          all_ready = false;

          if(logger_->log_level() >= Logging::Logger::TRACE)
          {
            tracing << "Not ready UserBindServer: " <<
              host_it->user_bind_server.ref() << std::endl;
          }
        }
        else if(!host_it->ready)
        {
          // resolve user_bind_server (
          //   all ready can be installed only if all servers resolved)
          UserBindServer::Source_var source =
            host_it->user_bind_server->get_source();

          assert(source->chunks_number);

          if(user_bind_config->common_chunks_number == 0)
          {
            user_bind_config->common_chunks_number = source->chunks_number;
          }
          else if(user_bind_config->common_chunks_number != source->chunks_number)
          {
            Stream::Error ostr;
            ostr << "Different chunks number on servers: ";
            throw Exception(ostr);
          }

          CorbaAlgs::convert_sequence(source->chunks, host_it->chunks);
          host_it->ready = true;

          if(logger_->log_level() >= TraceLevel::HIGH)
          {
            Stream::Error ostr;

            ostr << "Ready UserBindServer '" <<
              host_it->user_bind_server.ref() <<
              "', chunks: ";

            CorbaAlgs::print_sequence(ostr, source->chunks);

            logger_->log(ostr.str(),
              TraceLevel::HIGH,
              Aspect::USER_BIND_CONTROLLER);
          }
        }
      }
      catch(const CORBA::SystemException& ex)
      {
        all_ready = false;
        has_errors = true;

        errors << " Caught CORBA::SystemException at UserBindServer '" <<
          host_it->user_bind_server.ref() << "': " << ex;
      }
      catch(const eh::Exception& e)
      {
        all_ready = false;
        has_errors = true;

        errors << " Caught eh::Exception at UserBindServer '" <<
          host_it->user_bind_server.ref() << "': " << e.what();
      }
    }

    if(has_errors)
    {
      logger_->sstream(Logging::Logger::ERROR,
        Aspect::USER_BIND_CONTROLLER,
        "ADS-IMPL-72") <<
        "Errors of getting UserBindServer sources: " << errors.str();
    }

    if(all_ready)
    {
      user_bind_config->first_all_ready = true;
    }

    if(logger_->log_level() >= TraceLevel::LOW)
    {
      if(!all_ready)
      {
        logger_->stream(TraceLevel::LOW,
          Aspect::USER_BIND_CONTROLLER) <<
          "Not ready UserBindServers: " << tracing.str();
      }
      else
      {
        logger_->log(String::SubString("All UserBindServers ready"),
          TraceLevel::LOW,
          Aspect::USER_BIND_CONTROLLER);
      }
    }

    return all_ready;
  }

  void
  UserBindControllerImpl::init_user_bind_state_()
    noexcept
  {
    static const char* FUN = "UserBindControllerImpl::init_user_bind_state_()";

    try
    {
      UserBindConfig_var user_bind_config;

      {
        SyncPolicy::ReadGuard lock(lock_);
        user_bind_config = new UserBindConfig(*user_bind_config_);
      }

      bool has_errors = false;
      bool all_ready = true;

      try
      {
        all_ready = get_user_bind_server_sources_(user_bind_config.in());

        if(all_ready)
        {
          // all ready - check common constraints
          check_source_consistency_(user_bind_config.in());

          user_bind_config->all_ready = true;
          user_bind_config->first_all_ready = true;
        }
      }
      catch(const Exception& ex)
      {
        has_errors = true;

        logger_->sstream(Logging::Logger::ERROR,
          Aspect::USER_BIND_CONTROLLER,
          "ADS-IMPL-72") <<
          "Errors of getting UBS sources: " << ex.what();
      }

      if(all_ready && !has_errors)
      {
        {
          SyncPolicy::WriteGuard lock(lock_);
          user_bind_config_.swap(user_bind_config);
        }

        // set cyclic state checking task
        task_runner_->enqueue_task(
          Task_var(new CheckUserBindServerStateTask(this, 0)));
      }
      else
      {
        // schedule repeated sources initialization
        scheduler_->schedule(
          Task_var(new InitUserBindSourceTask(this, task_runner_)),
          Generics::Time::get_time_of_day() + REINIT_SOURCES_INTERVAL);
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::ERROR,
        Aspect::USER_BIND_CONTROLLER,
        "ADS-IMPL-72") << FUN <<
        ": Can't get sources. Caught eh::Exception: " <<
        ex.what();
    }
  }

  void
  UserBindControllerImpl::check_user_bind_state_()
    noexcept
  {
    static const char* FUN = "UserBindControllerImpl::check_user_bind_state_()";

    try
    {
      UserBindConfig_var user_bind_config;

      {
        SyncPolicy::ReadGuard lock(lock_);
        user_bind_config = user_bind_config_;
      }

      bool need_sources_reinit = false;

      UserBindServerRefArray& user_bind_servers =
        user_bind_config->user_bind_servers;

      for(UserBindServerRefArray::iterator it =
            user_bind_servers.begin();
          it != user_bind_servers.end(); ++it)
      {
        try
        {
          CORBACommons::IProcessControl::ALIVE_STATUS status =
            it->process_control->is_alive();

          if(status != CORBACommons::IProcessControl::AS_ALIVE)
          {
            need_sources_reinit = true;
          }
        }
        catch(const CORBA::SystemException& ex)
        {
          logger_->sstream(Logging::Logger::ERROR,
            Aspect::USER_BIND_CONTROLLER,
            "ADS-IMPL-72") <<
            FUN << ": Caught eh::Exception at checking status of '" <<
            it->user_bind_server.ref().object_ref << "': " << ex;
        }
      }

      if(!need_sources_reinit)
      {
        // schedule next status checking
        Generics::Time tm = Generics::Time::get_time_of_day() +
          user_bind_controller_config_.status_check_period();

        Task_var msg = new CheckUserBindServerStateTask(
          this, task_runner_);
        scheduler_->schedule(msg, tm);
      }
      else
      {
        {
          // controller not ready before sources
          // will not be successfully reinited
          SyncPolicy::WriteGuard lock(lock_);
          user_bind_config_->all_ready = false;
        }

        // schedule sources reinitialization
        Generics::Time tm = Generics::Time::get_time_of_day() +
          REINIT_SOURCES_INTERVAL;

        Task_var msg = new CheckUserBindServerStateTask(
          this, task_runner_);
        scheduler_->schedule(msg, tm);
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::ERROR,
        Aspect::USER_BIND_CONTROLLER,
        "ADS-IMPL-52") << FUN << ": Caught eh::Exception: " <<
        ex.what();
    }
  }

  void
  UserBindControllerImpl::check_source_consistency_(
    UserBindConfig* user_bind_config)
    /*throw(Exception)*/
  {
    bool has_errors = false;
    std::ostringstream errors;

    typedef std::vector<long> ChunkRefArray;

    ChunkRefArray chunk_refs(user_bind_config->common_chunks_number, -1);

    // check full chunk set covering
    unsigned long server_index = 0;

    UserBindServerRefArray& user_bind_servers =
      user_bind_config->user_bind_servers;

    for(UserBindServerRefArray::iterator server_it =
          user_bind_servers.begin();
        server_it != user_bind_servers.end();
        ++server_it, ++server_index)
    {
      for(ChunkIdSet::const_iterator chunk_it =
            server_it->chunks.begin();
          chunk_it != server_it->chunks.end(); ++chunk_it)
      {
        if(*chunk_it >= user_bind_config->common_chunks_number)
        {
          has_errors = true;
          errors << "UBS '" << server_it->host_name << "'"
            " has chunk with index = " << *chunk_it <<
            ", that > common_chunks_number(" <<
            user_bind_config->common_chunks_number << "). ";
        }
        else if(chunk_refs[*chunk_it] != -1)
        {
          UserBindServerRefArray::iterator first_server_it =
            user_bind_config->user_bind_servers.begin() +
            chunk_refs[*chunk_it];

          has_errors = true;
          errors << "Chunk #" << *chunk_it << " twice defined. "
            "First UBS '" << first_server_it->host_name << "', "
            "second UBS '" << server_it->host_name << "'. ";
        }
        else
        {
          chunk_refs[*chunk_it] = server_index;
        }
      }
    }

    for(ChunkRefArray::const_iterator ch_it = chunk_refs.begin();
        ch_it != chunk_refs.end(); ++ch_it)
    {
      if(*ch_it == -1)
      {
        has_errors = true;
        errors << "No one UBS don't contain Chunk #" <<
          (ch_it - chunk_refs.begin()) << ". ";
      }
    }

    if(has_errors)
    {
      Stream::Error ostr;
      ostr << "Data sources isn't consistency: " << errors.str();
      throw Exception(ostr);
    }
  }

  void
  UserBindControllerImpl::fill_user_bind_server_descr_seq_(
    AdServer::UserInfoSvcs::UserBindDescriptionSeq& user_bind_server_descr_seq)
    /*throw(AdServer::UserInfoSvcs::UserBindController::ImplementationException,
      AdServer::UserInfoSvcs::UserBindController::NotReady)*/
  {
    static const char* FUN = "UserBindControllerImpl::fill_user_bind_server_descr_seq_()";

    try
    {
      UserBindConfig_var user_bind_config;

      {
        SyncPolicy::ReadGuard lock(lock_);
        user_bind_config = user_bind_config_;
      }

      if(!user_bind_config->first_all_ready)
      {
        throw AdServer::UserInfoSvcs::UserBindController::NotReady();
      }

      const UserBindServerRefArray& user_bind_servers =
        user_bind_config->user_bind_servers;

      user_bind_server_descr_seq.length(user_bind_servers.size());

      CORBA::ULong i = 0;
      for(UserBindServerRefArray::const_iterator it =
            user_bind_servers.begin();
          it != user_bind_servers.end(); ++it, ++i)
      {
        const UserBindServerRef& user_bind_server_ref = user_bind_servers[i];
        AdServer::UserInfoSvcs::UserBindDescription&
          user_bind_server_descr = user_bind_server_descr_seq[i];

        assert(user_bind_server_ref.user_bind_server.get());

        user_bind_server_descr.user_bind_server =
          user_bind_server_ref.user_bind_server.get();

        CorbaAlgs::fill_sequence(
          user_bind_server_ref.chunks.begin(),
          user_bind_server_ref.chunks.end(),
          user_bind_server_descr.chunk_ids);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init session. Caught eh::Exception: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserBindController::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init session. Caught CORBA::SystemException: " <<
        ex;

      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserBindController::ImplementationException>(
          ostr.str());
    }
  }

  // UserInfoClusterControlImpl impl
  UserBindClusterControlImpl::UserBindClusterControlImpl(
    UserBindControllerImpl* user_bind_controller)
    noexcept
    : user_bind_controller_(
        ReferenceCounting::add_ref(user_bind_controller))
  {}

  UserBindClusterControlImpl::~UserBindClusterControlImpl()
    noexcept
  {}

  CORBACommons::IProcessControl::ALIVE_STATUS
  UserBindClusterControlImpl::is_alive()
    noexcept
  {
    return user_bind_controller_->get_status();
  }

  char*
  UserBindClusterControlImpl::comment()
    /*throw(CORBACommons::OutOfMemory)*/
  {
    return user_bind_controller_->get_comment();
  }
} /*UserInfoSvcs*/
} /*AdServer*/

