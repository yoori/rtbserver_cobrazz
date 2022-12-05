#include <list>
#include <vector>
#include <set>

#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManagerControl.hpp>
#include "UserInfoManagerSessionFactory.hpp"
#include "UserInfoManagerControllerImpl.hpp"

namespace
{
  const Generics::Time REINIT_SOURCES_INTERVAL = Generics::Time(10); // 10 sec
}

namespace Aspect
{
  const char USER_INFO_MANAGER_CONTROLLER[] = "UserInfoManagerController";
}

namespace AdServer{
namespace UserInfoSvcs{

  /**
   * UserInfoManagerControllerImpl
   */

  UserInfoManagerControllerImpl::UserInfoManagerControllerImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const UserInfoManagerControllerConfig&
      user_info_manager_controller_config)
    /*throw(Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_, 1)),
      user_info_manager_controller_config_(
        user_info_manager_controller_config),
      inited_(false)
  {
    static const char* FUN =
      "UserInfoManagerControllerImpl::UserInfoManagerControllerImpl()";

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

      fill_user_info_manager_refs_();

      Task_var msg =
        new InitUserInfoManagerSourcesTask(this, 0);
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

  UserInfoManagerControllerImpl::~UserInfoManagerControllerImpl() noexcept
  {}

  /* UserInfoManagerController interface */
  AdServer::UserInfoSvcs::UserInfoManagerSession*
  UserInfoManagerControllerImpl::get_session()
    /*throw(AdServer::UserInfoSvcs::UserInfoManagerController::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManagerController::NotReady)*/
  {
    AdServer::UserInfoSvcs::UserInfoManagerDescriptionSeq user_info_manager_descr_seq;

    fill_user_info_manager_descr_seq_(user_info_manager_descr_seq);

    OBV_AdServer::UserInfoSvcs::UserInfoManagerSession*
      ret = new OBV_AdServer::UserInfoSvcs::UserInfoManagerSession(
        user_info_manager_descr_seq);

    return ret;
  }

  void
  UserInfoManagerControllerImpl::get_session_description(
    AdServer::UserInfoSvcs::UserInfoManagerDescriptionSeq_out session_description)
    /*throw(AdServer::UserInfoSvcs::UserInfoManagerController::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManagerController::NotReady)*/
  {
    session_description =
      new AdServer::UserInfoSvcs::UserInfoManagerDescriptionSeq();

    fill_user_info_manager_descr_seq_(*session_description);
  }

  /** UserInfoManagerController protected methods */
  void
  UserInfoManagerControllerImpl::fill_user_info_manager_refs_()
    /*throw(Exception)*/
  {
    try
    {
      typedef
        UserInfoManagerControllerConfig::UserInfoManagerHost_sequence
        UserInfoManagerHostSeq;

      const UserInfoManagerHostSeq& user_info_manager_seq =
        user_info_manager_controller_config_.UserInfoManagerHost();

      UserInfoManagersConfig_var user_info_managers_config =
        new UserInfoManagersConfig();

      user_info_managers_config->all_ready = false;

      for(UserInfoManagerHostSeq::const_iterator it =
            user_info_manager_seq.begin();
          it != user_info_manager_seq.end(); ++it)
      {
        UserInfoManagerRef user_info_manager_host;

        /* load UIMControl ref */
        CORBACommons::CorbaObjectRef corba_object_ref;

        try
        {
          Config::CorbaConfigReader::read_corba_ref(
            it->UserInfoManagerControlRef(), corba_object_ref);

          user_info_manager_host.user_info_manager_control =
            UserInfoManagerRef::UIMControlCORBARef(
              corba_client_adapter_, corba_object_ref);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << "Can't resolve corba object ref: '" <<
            corba_object_ref.object_ref <<
            "'. Caught eh::Exception: " << e.what();

          throw Exception(ostr);
        }

        /* load UIM ref */
        try
        {
          Config::CorbaConfigReader::read_corba_ref(
            it->UserInfoManagerRef(), corba_object_ref);

          user_info_manager_host.user_info_manager =
            UserInfoManagerRef::UIMCORBARef(
              corba_client_adapter_, corba_object_ref);
          if (it->UserInfoManagerRef().name().present())
          {
            user_info_manager_host.user_info_manager_host_name =
              it->UserInfoManagerRef().name().get();
          }
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr
            << "Can't resolve UserInfoManager corba object ref: '"
            << corba_object_ref.object_ref << "'. "
            << ": " << e.what();

          throw Exception(ostr);
        }

        /* load UIM stats ref */
        try
        {
          Config::CorbaConfigReader::read_corba_ref(
            it->UserInfoManagerStatsRef(), corba_object_ref);

          user_info_manager_host.uim_stats = 
            UserInfoManagerRef::ProcessControlCORBARef(
              corba_client_adapter_, corba_object_ref);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr
            << "Can't read UserInfoManagerStats corba object ref: '"
            << corba_object_ref.object_ref << "'. "
            << ": " << e.what();

          throw Exception(ostr);
        }

        user_info_manager_host.ready = false;

        user_info_managers_config->user_info_managers.push_back(
          user_info_manager_host);
      }

      {
        WriteGuard_ lock(lock_);
        user_info_managers_config_.swap(user_info_managers_config);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoManagerControllerImpl::fill_user_info_manager_refs_(): "
              "Can't resolve UserInfoManager references. : "
           << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserInfoManagerControllerImpl::admit_user_info_managers_()
    /*throw(Exception)*/
  {
    UserInfoManagersConfig_var user_info_managers_config;

    {
      ReadGuard_ lock(lock_);
      user_info_managers_config =
        new UserInfoManagersConfig(*user_info_managers_config_);
    }

    bool all_admitted = true;
    std::ostringstream errors;

    for(UserInfoManagerRefVector::iterator uim_it =
          user_info_managers_config->user_info_managers.begin();
        uim_it != user_info_managers_config->user_info_managers.end();
        ++uim_it)
    {
      try
      {
        uim_it->user_info_manager_control->admit();
      }
      catch(const CORBA::SystemException& ex)
      {
        uim_it->ready = false;
        all_admitted = false;

        errors << "  Can't admit UIM: '"
          << uim_it->user_info_manager.ref().object_ref <<
          "': CORBA::SystemException: " << ex;
      }
    }

    if(!all_admitted)
    {
      Stream::Error ostr;
      ostr << "Can't admit all UIMs: " << errors.str();
      throw Exception(ostr);
    }
  }

  bool
  UserInfoManagerControllerImpl::get_user_info_manager_sources_(
    UserInfoManagersConfig* user_info_managers_config)
    /*throw(Exception)*/
  {
    std::ostringstream tracing;
    std::ostringstream errors;

    bool has_errors = false;
    bool all_ready = true;

    UserInfoManagerRefVector& user_info_managers =
      user_info_managers_config->user_info_managers;

    for(UserInfoManagerRefVector::iterator uim_it =
          user_info_managers.begin();
        uim_it != user_info_managers.end(); ++uim_it)
    {
      try
      {
        AdServer::UserInfoSvcs::UserInfoManagerStatus uim_status =
          uim_it->user_info_manager_control->status();

        if(uim_status < AdServer::UserInfoSvcs::S_READY)
        {
          /* uim isn't ready */
          uim_it->ready = false;
          uim_it->chunks.clear();

          all_ready = false;

          if(logger_->log_level() >= Logging::Logger::TRACE)
          {
            tracing << "Not ready UIM '"
                    << uim_it->user_info_manager.ref().object_ref
                    << "'. " << std::endl;
          }
        }
        else if(!uim_it->ready)
        {
          /* uim is ready but not inited localy (sample: after restart uimc) */
          AdServer::UserInfoSvcs::ChunksConfig_var chunks_config;
          uim_it->user_info_manager_control->get_source_info(chunks_config.out());

          CorbaAlgs::convert_sequence(chunks_config->chunk_ids, uim_it->chunks);
          uim_it->common_chunks_number = chunks_config->common_chunks_number;
          uim_it->ready = uim_it->user_info_manager->uim_ready();

          if(logger_->log_level() >= TraceLevel::HIGH)
          {
            Stream::Error ostr;

            ostr << "Ready UIM '"
                 << uim_it->user_info_manager.ref().object_ref << "'. "
                 << "Chunks: ";

            CorbaAlgs::print_sequence(ostr, chunks_config->chunk_ids);

            logger_->log(ostr.str(),
              TraceLevel::HIGH,
              Aspect::USER_INFO_MANAGER_CONTROLLER);
          }
        }
      }
      catch(const CORBA::SystemException& ex)
      {
        all_ready = false;
        has_errors = true;

        errors << " Caught CORBA::SystemException at UIM '"
          << uim_it->user_info_manager.ref().object_ref << "': " << ex;
      }
      catch(const eh::Exception& e)
      {
        all_ready = false;
        has_errors = true;

        errors << " Caught eh::Exception at UIM '"
          << uim_it->user_info_manager.ref().object_ref << "': " << e.what();
      }
    }

    if(has_errors)
    {
      logger_->sstream(Logging::Logger::ERROR,
                       Aspect::USER_INFO_MANAGER_CONTROLLER,
                       "ADS-IMPL-72")
        << "Errors of getting UIM sources: " << errors.str();
    }

    if(logger_->log_level() >= TraceLevel::LOW)
    {
      if(!all_ready)
      {
        logger_->stream(TraceLevel::LOW,
                        Aspect::USER_INFO_MANAGER_CONTROLLER)
          << "Not ready UIMs: " << tracing.str();
      }
      else
      {
        logger_->log(String::SubString("All UIM's ready"),
                     TraceLevel::LOW,
                     Aspect::USER_INFO_MANAGER_CONTROLLER);
      }
    }

    return all_ready;
  }

  void
  UserInfoManagerControllerImpl::init_user_info_manager_sources_()
    noexcept
  {
    try
    {
      UserInfoManagersConfig_var user_info_managers_config;

      {
        ReadGuard_ lock(lock_);
        user_info_managers_config =
          new UserInfoManagersConfig(*user_info_managers_config_);
      }

      bool has_errors = false;
      bool all_ready = true;

      try
      {
        all_ready =
          get_user_info_manager_sources_(user_info_managers_config.in());

        if(all_ready)
        {
          /* all ready - check common constraints */
          check_source_consistency_(user_info_managers_config.in());

          /* admit hosts to cluster */
          admit_user_info_managers_();

          user_info_managers_config->all_ready = true;
          user_info_managers_config->first_all_ready = true;
        }
      }
      catch(const Exception& ex)
      {
        has_errors = true;

        logger_->sstream(Logging::Logger::ERROR,
                         Aspect::USER_INFO_MANAGER_CONTROLLER,
                         "ADS-IMPL-72")
         << "Errors of getting UIM sources: " << ex.what();
      }

      if(all_ready && !has_errors)
      {
        {
          WriteGuard_ lock(lock_);
          user_info_managers_config_.swap(user_info_managers_config);
        }

        /* set cyclic state checking task */
        Task_var msg =
          new CheckUserInfoManagerStatesTask(this, 0);
        task_runner_->enqueue_task(msg);
      }
      else
      {
        /* schedule repeated sources initialization */
        Generics::Time tm = Generics::Time::get_time_of_day() +
          REINIT_SOURCES_INTERVAL;

        Task_var msg =
          new InitUserInfoManagerSourcesTask(this, task_runner_);
        scheduler_->schedule(msg, tm);
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::ERROR,
                       Aspect::USER_INFO_MANAGER_CONTROLLER,
                      "ADS-IMPL-72")
        << __func__ << "Can't get UIMs sources. Caught eh::Exception. : "
        << ex.what();
    }
  }

  void
  UserInfoManagerControllerImpl::check_user_info_manager_states_()
    noexcept
  {
    try
    {
      UserInfoManagersConfig_var user_info_managers_config;

      {
        ReadGuard_ lock(lock_);
        user_info_managers_config = user_info_managers_config_;
      }

      bool need_sources_reinit = false;

      UserInfoManagerRefVector& user_info_managers =
        user_info_managers_config->user_info_managers;

      for(UserInfoManagerRefVector::iterator uim_it =
            user_info_managers.begin();
          uim_it != user_info_managers.end(); ++uim_it)
      {
        try
        {
          AdServer::UserInfoSvcs::UserInfoManagerStatus uim_status =
            uim_it->user_info_manager_control->status();

          if(uim_status != AdServer::UserInfoSvcs::S_ADMITTED)
          {
            need_sources_reinit = true;
          }
        }
        catch(const CORBA::SystemException& ex)
        {
          logger_->sstream(Logging::Logger::ERROR,
                           Aspect::USER_INFO_MANAGER_CONTROLLER,
                           "ADS-IMPL-72")
            << "UserInfoManagerControllerImpl::"
            "check_user_info_manager_states_(): "
            << "Caught eh::Exception at checking status of '"
            << uim_it->user_info_manager.ref().object_ref << "' . : "
            << ex;
        }
      }

      if(!need_sources_reinit)
      {
        /* schedule next status checking */
        Generics::Time tm = Generics::Time::get_time_of_day() +
          user_info_manager_controller_config_.status_check_period();

        Task_var msg =
          new CheckUserInfoManagerStatesTask(this, task_runner_);
        scheduler_->schedule(msg, tm);
      }
      else
      {
        {
          /* controller not ready before sources
             will not be successfully reinited */
          WriteGuard_ lock(lock_);
          user_info_managers_config_->all_ready = false;
        }

        /* schedule sources reinitialization */
        Generics::Time tm = Generics::Time::get_time_of_day() +
          REINIT_SOURCES_INTERVAL;

        Task_var msg =
          new InitUserInfoManagerSourcesTask(this, task_runner_);
        scheduler_->schedule(msg, tm);
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::ERROR,
                       Aspect::USER_INFO_MANAGER_CONTROLLER,
                       "ADS-IMPL-52")
        << "UserInfoManagerControllerImpl::"
        "check_user_info_manager_states_(): "
        << "Caught eh::Exception. : "
        << ex.what();
    }
  }

  void
  UserInfoManagerControllerImpl::check_source_consistency_(
    UserInfoManagersConfig* user_info_managers_config)
    /*throw(Exception)*/
  {
    bool has_errors = false;
    std::ostringstream errors;

    unsigned long common_chunks_number = 0;

    /* check common_chunks_number equality */
    for(UserInfoManagerRefVector::iterator uim_it =
          user_info_managers_config->user_info_managers.begin();
        uim_it != user_info_managers_config->user_info_managers.end();
        ++uim_it)
    {
      if(uim_it == user_info_managers_config->user_info_managers.begin())
      {
        common_chunks_number = uim_it->common_chunks_number;
      }
      else if(common_chunks_number != uim_it->common_chunks_number)
      {
        UserInfoManagerRefVector::iterator first_uim_it =
          user_info_managers_config->user_info_managers.begin();

        has_errors = true;
        errors << "UIS's has different information about common chunk number. "
               << "first UIS '"
               << first_uim_it->user_info_manager.ref().object_ref << "' has value "
               << first_uim_it->common_chunks_number
               << ", second UIM '"
               << uim_it->user_info_manager.ref().object_ref << "' has value "
               << uim_it->common_chunks_number << ". ";
      }
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->sstream(Logging::Logger::TRACE,
                       Aspect::USER_INFO_MANAGER_CONTROLLER)
        << "Common chunks number in all UIM's equal "
        << common_chunks_number << ".";
    }

    typedef std::vector<int> ChunkRefsVector;

    ChunkRefsVector chunk_refs(common_chunks_number, -1);

    /* check full chunk set covering */
    unsigned long uim_index = 0;

    UserInfoManagerRefVector& user_info_managers =
      user_info_managers_config->user_info_managers;

    for(UserInfoManagerRefVector::iterator uim_it =
          user_info_managers.begin();
        uim_it != user_info_managers.end();
        ++uim_it, ++uim_index)
    {
      for(ChunkIdSet::const_iterator chunk_it =
            uim_it->chunks.begin();
          chunk_it != uim_it->chunks.end(); ++chunk_it)
      {
        if(*chunk_it >= common_chunks_number)
        {
          has_errors = true;
          errors << "UIM '"
                 << uim_it->user_info_manager.ref().object_ref
                 << "' has chunk with index = " << *chunk_it
                 << ", that > common_chunks_number("
                 << common_chunks_number << "). ";
        }
        else if(chunk_refs[*chunk_it] != -1)
        {
          UserInfoManagerRefVector::iterator first_uim_it =
            user_info_managers_config->user_info_managers.begin() +
            chunk_refs[*chunk_it];

          has_errors = true;
          errors
            << "Chunk #" << *chunk_it << " twice defined. "
            << "First UIM '"
            << first_uim_it->user_info_manager.ref().object_ref
            << "', second UIM '"
            << uim_it->user_info_manager.ref().object_ref << "'. ";
        }
        else
        {
          chunk_refs[*chunk_it] = uim_index;
        }
      }
    }

    for(ChunkRefsVector::const_iterator ch_it =
          chunk_refs.begin();
        ch_it != chunk_refs.end(); ++ch_it)
    {
      if(*ch_it == -1)
      {
        has_errors = true;
        errors << "No one UIM don't contain Chunk #"
               << (ch_it - chunk_refs.begin()) << ". ";
      }
    }

    if(has_errors)
    {
      Stream::Error ostr;
      ostr << "Data sources isn't consistency: " << errors.str();
      throw Exception(ostr);
    }
  }

  CORBACommons::StatsValueSeq*
  UserInfoManagerControllerImpl::get_stats() noexcept
  {
    CORBACommons::StatsValueSeq_var res = new CORBACommons::StatsValueSeq();

    UserInfoManagersConfig_var user_info_managers_config;
    {
      ReadGuard_ lock(lock_);
      user_info_managers_config = user_info_managers_config_;
    }

    if(user_info_managers_config->all_ready)
    {
      res->length(2);
      CORBACommons::StatsValueSeq& res_ref = *res;

      res_ref[0].key = "Sum size of user containers";
      res_ref[1].key = "Sum size of context bounded maps";

      unsigned long users_count = 0;
      unsigned long context_users_count = 0;

      UserInfoManagerRefVector& uims_vector =
        user_info_managers_config->user_info_managers;
      unsigned long len = uims_vector.size();

      for (unsigned long i = 0; i < len; ++i)
      {
        CORBACommons::StatsValueSeq_var stats =
          uims_vector[i].uim_stats->get_stats();

        CORBA::ULong value;

        stats[size_t(0)].value >>= value;
        users_count += value;

        stats[size_t(1)].value >>= value;
        context_users_count += value;
      }

      res_ref[0].value <<= (CORBA::ULong)users_count;
      res_ref[1].value <<= (CORBA::ULong)context_users_count;
    }

    return res._retn();
  }

  CORBACommons::IProcessControl::ALIVE_STATUS
  UserInfoManagerControllerImpl::get_status() noexcept
  {
    try
    {
      UserInfoManagersConfig_var user_info_managers_config;

      {
        ReadGuard_ lock(lock_);
        user_info_managers_config = user_info_managers_config_;
      }

      if(!user_info_managers_config->all_ready)
      {
        return CORBACommons::IProcessControl::AS_NOT_ALIVE;
      }
      else
      {
        return get_user_info_cluster_control_()->is_alive();
      }
    }
    catch(...)
    {
      return CORBACommons::IProcessControl::AS_NOT_ALIVE;
    }
  }

  char*
  UserInfoManagerControllerImpl::get_comment() /*throw(CORBACommons::OutOfMemory)*/
  {
    return get_user_info_cluster_control_()->comment();
  }

  UserInfoClusterControlImpl_var
  UserInfoManagerControllerImpl:: get_user_info_cluster_control_() noexcept
  {
    UserInfoManagersConfig_var user_info_managers_config;
    {
      ReadGuard_ lock(lock_);
      user_info_managers_config = user_info_managers_config_;
    }

    UIMRefVector uims;
    std::vector<std::string> hosts;
    const UserInfoManagerRefVector& uim_refs =
      user_info_managers_config->user_info_managers;

    for (unsigned int i = 0; i < uim_refs.size(); ++i)
    {
      uims.push_back(uim_refs[i].user_info_manager.ref());
      hosts.push_back(uim_refs[i].user_info_manager_host_name);
    }

    UserInfoClusterControlImpl_var control(new UserInfoClusterControlImpl(
      corba_client_adapter_.in(),
      uims,
      hosts));

    return control;
  }

  void
  UserInfoManagerControllerImpl::fill_user_info_manager_descr_seq_(
    AdServer::UserInfoSvcs::UserInfoManagerDescriptionSeq& user_info_manager_descr_seq)
    /*throw(AdServer::UserInfoSvcs::UserInfoManagerController::ImplementationException,
          AdServer::UserInfoSvcs::UserInfoManagerController::NotReady)*/
  {
    static const char* FUN = "UserInfoManagerControllerImpl::fill_user_info_manager_descr_seq_()";

    try
    {
      UserInfoManagersConfig_var user_info_managers_config;

      {
        ReadGuard_ lock(lock_);
        user_info_managers_config = user_info_managers_config_;
      }

      if(!user_info_managers_config->first_all_ready)
      {
        throw AdServer::UserInfoSvcs::UserInfoManagerController::NotReady();
      }

      /* fill UserInfoManagerDescriptionSeq struct for session initialization */
      const UserInfoManagerRefVector& user_info_managers =
        user_info_managers_config->user_info_managers;

      unsigned int uim_count = user_info_managers.size();

      user_info_manager_descr_seq.length(uim_count);

      for(unsigned int i = 0; i < uim_count; ++i)
      {
        const UserInfoManagerRef& user_info_manager_ref = user_info_managers[i];
        AdServer::UserInfoSvcs::UserInfoManagerDescription&
          user_info_manager_descr = user_info_manager_descr_seq[i];

        user_info_manager_descr.user_info_manager =
          user_info_manager_ref.user_info_manager.get();

        CorbaAlgs::fill_sequence(
          user_info_manager_ref.chunks.begin(),
          user_info_manager_ref.chunks.end(),
          user_info_manager_descr.chunk_ids);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init session. Caught eh::Exception: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserInfoManagerController::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init session. Caught CORBA::SystemException: " <<
        ex;

      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserInfoManagerController::ImplementationException>(
          ostr.str());
    }
  }

  /*UserInfoClusterControlImpl*/
  UserInfoClusterImpl::UserInfoClusterImpl(
    UserInfoManagerControllerImpl* controller) noexcept
      :
      uimc_(ReferenceCounting::add_ref(controller))
  {}

  UserInfoClusterImpl::~UserInfoClusterImpl() noexcept
  {}

  CORBACommons::IProcessControl::ALIVE_STATUS
  UserInfoClusterImpl::is_alive() noexcept
  {
    return uimc_->get_status();
  }

  char*
  UserInfoClusterImpl::comment() /*throw(CORBACommons::OutOfMemory)*/
  {
    return uimc_->get_comment();
  }

  /*UserInfoClusterStatsImpl*/
  UserInfoClusterStatsImpl::UserInfoClusterStatsImpl(
    UserInfoManagerControllerImpl* controller) noexcept
      :
      uimc_(ReferenceCounting::add_ref(controller))
  {}

  UserInfoClusterStatsImpl::~UserInfoClusterStatsImpl() noexcept
  {}

  CORBACommons::StatsValueSeq*
  UserInfoClusterStatsImpl::get_stats() noexcept
  {
    return uimc_->get_stats();
  }
} /*UserInfoSvcs*/
} /*AdServer*/

