#include <list>
#include <vector>
#include <iterator>
#include <time.h>

#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>

#include "UserInfoExchangerImpl.hpp"

namespace Aspect
{
  char USER_INFO_EXCHANGER[] = "UserInfoExchanger";
}

namespace AdServer{
namespace UserInfoSvcs{

  /**
   * UserInfoExchangerImpl
   */

  UserInfoExchangerImpl::UserInfoExchangerImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const UserInfoExchangerConfig& user_info_exchanger_config)
    /*throw(Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      user_info_exchanger_config_(user_info_exchanger_config),
      scheduler_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_, 1)),
      inited_(false)
  {
    try
    {
      add_child_object(task_runner_);
      add_child_object(scheduler_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangerImpl::UserExchangerImpl(): "
        << "CompositeActiveObject::Exception caught. "
        << ": " << ex.what() << ".";
      throw Exception(ostr);
    }

    try
    {
      corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

      expire_time_ =
        user_info_exchanger_config_.expire_time().present() ?
        Generics::Time::ONE_HOUR *
          user_info_exchanger_config_.expire_time().get() :
        DEFAULT_EXPIRE_TIME;

      user_info_exchange_pool_ =
        new UserInfoExchangePool(
          Generics::ActiveObjectCallback_var(
            new Logging::ActiveObjectCallbackImpl(
              logger_,
              "UserInfoExchanger",
              "UserInfoExchangePool",
              "ADS-IMPL-?")),
          user_info_exchanger_config_.RepositoryPlace().path().c_str(),
          expire_time_/4);

      add_child_object(user_info_exchange_pool_);

      Task_var erase_old_profiles_msg =
        new EraseOldProfilesTask(this, 0);
      task_runner_->enqueue_task(erase_old_profiles_msg);

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->sstream(
          Logging::Logger::TRACE,
          Aspect::USER_INFO_EXCHANGER)
          << "EraseOldProfilesTask was enqueued.";

      }
    }
    catch(const Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangerImpl::UserInfoExchangerImpl(): "
        "Can't instantiate object. Caught Exception. "
        ": "
        << ex.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangerImpl::UserInfoExchangerImpl(): "
        "Can't instantiate object. Caught eh::Exception. "
        ": "
        << ex.what();
      throw Exception(ostr);
    }
  }

  UserInfoExchangerImpl::~UserInfoExchangerImpl() noexcept
  {
  }

  /* UserInfoExchange corba interface implementation */
  void
  UserInfoExchangerImpl::register_users_request(
    const char* customer_id,
    const AdServer::UserInfoSvcs::ColoUsersRequestSeq& colo_users)
    /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
      AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/
  {
    try
    {
      for(CORBA::ULong i = 0; i < colo_users.length(); ++i)
      {
        std::ostringstream ostr;
        ostr << colo_users[i].colo_id;

        UserInfoExchangePool::UserIdList user_ids;
        const AdServer::UserInfoSvcs::UserIdSeq& users_seq = colo_users[i].users;
        CORBA::ULong len = users_seq.length();

        for(CORBA::ULong user_i = 0; user_i < len; ++user_i)
        {
          std::ostringstream prov_user_id;
          prov_user_id << ostr.str() << "_" << users_seq[user_i].in();

          user_ids.push_back(prov_user_id.str());
        }

        user_info_exchange_pool_->register_users_request(
          customer_id,
          ostr.str().c_str(),
          user_ids);
      }

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << "UserInfoExchangerImpl::register_users_request(): " << std::endl
          << "Input parameters: " << std::endl
          << "  provider_id = '" << customer_id << "'" << std::endl
          << "  number of requested users: " << std::endl;

        for(CORBA::ULong i = 0; i < colo_users.length(); ++i)
        {
          ostr << "    from colo = " << colo_users[i].colo_id << " : "
               << colo_users[i].users.length() << std::endl;
        }

        logger_->log(ostr.str(),
                     Logging::Logger::TRACE,
                     Aspect::USER_INFO_EXCHANGER);
      }
    }
    catch(const UserInfoExchangePool::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangerImpl::register_users_requests(): "
        << "Caught UserInfoExchangePool::Exception: "
        << ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr
        << "UserInfoExchangerImpl::register_users_request(): "
        << "Caught eh::Exception. : " << ex.what()
        << ". Input parameters: provider_id = '" << customer_id << "'";

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCHANGER,
                   "ADS-IMPL-62");
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;

      ostr
        << "UserInfoExchangerImpl::register_users_request(): "
        << "Caught CORBA::SystemException. : " << ex
        << " Input parameters: provider_id = '" << customer_id << "'";

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCHANGER,
                   "ADS-IMPL-62");
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
  }

  void
  UserInfoExchangerImpl::receive_users(
    const char* customer_id,
    AdServer::UserInfoSvcs::UserProfileSeq_out user_profiles_out,
    const AdServer::UserInfoSvcs::ReceiveCriteria& receive_criteria)
    /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
      AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/
  {
    try
    {
      UserInfoExchangePool::UserProfileList user_profiles;
      UserInfoExchangePool::ReceiveCriteria pool_receive_criteria;

      pool_receive_criteria.common_chunks_number =
        receive_criteria.common_chunks_number;
      pool_receive_criteria.max_response_plain_size =
        receive_criteria.max_response_plain_size;
      CorbaAlgs::convert_sequence(
        receive_criteria.chunk_ids, pool_receive_criteria.chunk_ids);

      user_info_exchange_pool_->receive_users(
        customer_id,
        user_profiles,
        pool_receive_criteria);

      /* convert internal UserProfile's format to CORBA format */
      user_profiles_out = new AdServer::UserInfoSvcs::UserProfileSeq();

      AdServer::UserInfoSvcs::UserProfileSeq& user_profiles_ref =
        *user_profiles_out;

      user_profiles_ref.length(user_profiles.size());

      CORBA::ULong i = 0;
      for(UserInfoExchangePool::UserProfileList::const_iterator it =
            user_profiles.begin();
          it != user_profiles.end(); ++it, ++i)
      {
        AdServer::UserInfoSvcs::UserProfile& user_profile_out =
          user_profiles_ref[i];
        user_profile_out.user_id << it->user_id;

        unsigned long colo_id;
        Stream::Parser istr(it->provider_id);
        istr >> colo_id;
        if(istr.bad() || istr.fail())
        {
          Stream::Error ostr;
          ostr << "Provider has non numeric identificator '"
            << it->provider_id << "'.";

          throw Exception(ostr);
        }

        user_profile_out.colo_id = colo_id;

        user_profile_out.plain_profile.length(it->plain_size);

        memcpy(
          user_profile_out.plain_profile.get_buffer(),
          it->plain_user_info.get(),
          it->plain_size);

        user_profile_out.plain_history_profile.length(it->plain_history_size);

        if(it->plain_history_user_info.get())
        {
          memcpy(
            user_profile_out.plain_history_profile.get_buffer(),
            it->plain_history_user_info.get(),
            it->plain_history_size);
        }
      }

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->stream(Logging::Logger::TRACE,
                        Aspect::USER_INFO_EXCHANGER)
          << "UserInfoExchangerImpl::receive_users(): " << std::endl
          << "Input parameters: " << std::endl
          << "  customer_id = '" << customer_id << "'" << std::endl
          << "  max_response_plain_size = "
          << receive_criteria.max_response_plain_size << std::endl
          << "Output: " << std::endl
          << "  users count = " << user_profiles_out->length();
      }
    }
    catch(const UserInfoExchangePool::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangerImpl::receive_users(): "
        << "Caught UserInfoExchangePool::Exception: "
        << ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;

      ostr << "UserInfoExchangerImpl::receive_users(): "
        << "Caught eh::Exception. : " << ex.what()
        << " Input parameters: customer_id = '" << customer_id << "'"
        << " max_response_plain_size = "
        << receive_criteria.max_response_plain_size;

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCHANGER,
                   "ADS-IMPL-63");
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;

      ostr << "UserInfoExchangerImpl::receive_users(): "
        << "Caught CORBA::SystemException. : " << ex
        << " Input parameters: customer_id = '" << customer_id << "'"
        << " max_response_plain_size = "
        << receive_criteria.max_response_plain_size;

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCHANGER,
                   "ADS-IMPL-63");
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
  }

  void
  UserInfoExchangerImpl::get_users_requests(
    const char* provider_id,
    AdServer::UserInfoSvcs::UserIdSeq_out user_ids_out)
    /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
      AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/
  {
    try
    {
      UserInfoExchangePool::UserIdList user_ids;

      user_info_exchange_pool_->get_users_request(
        provider_id,
        user_ids);

      CORBA::ULong i = 0;
      user_ids_out = new AdServer::UserInfoSvcs::UserIdSeq();
      AdServer::UserInfoSvcs::UserIdSeq& user_ids_ref = *user_ids_out;
      user_ids_ref.length(user_ids.size());

      for(UserInfoExchangePool::UserIdList::const_iterator
            uid_it = user_ids.begin();
          uid_it != user_ids.end(); ++uid_it, ++i)
      {
        user_ids_ref[i] = uid_it->c_str();
      }

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->stream(Logging::Logger::TRACE,
                        Aspect::USER_INFO_EXCHANGER)
          << "UserInfoExchangerImpl::get_users_requests(): " << std::endl
          << "Input parameters: " << std::endl
          << "  provider_id = '" << provider_id << "'" << std::endl
          << "Output: " << std::endl
          << "  number of requested users: " << user_ids_out->length();
      }
    }
    catch(const UserInfoExchangePool::Exception& ex)
    {
      Stream::Error ostr;
      ostr
        << "UserInfoExchangerImpl::get_users_requests(): "
        << "Caught UserInfoExchangePool::Exception: "
        << ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;

      ostr
        << "UserInfoExchangerImpl::get_users_requests(): "
        << "Caught eh::Exception. : " << ex.what()
        << " Input parameters: provider_id = '" << provider_id << "'";

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCHANGER,
                   "ADS-IMPL-64");
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;

      ostr
        << "UserInfoExchangerImpl::get_users_requests(): "
        << "Caught CORBA::SystemException. : " << ex 
        << " Input parameters: provider_id = '" << provider_id << "'";

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCHANGER,
                   "ADS-IMPL-64");
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
  }

  void
  UserInfoExchangerImpl::send_users(
    const char* provider_id,
    const AdServer::UserInfoSvcs::UserProfileSeq& user_profiles_in)
    /*throw(AdServer::UserInfoSvcs::UserInfoExchanger::NotReady,
      AdServer::UserInfoSvcs::UserInfoExchanger::ImplementationException)*/
  {
    try
    {
      UserInfoExchangePool::UserProfileList user_profiles;

      for(CORBA::ULong i = 0; i < user_profiles_in.length(); ++i)
      {
        const AdServer::UserInfoSvcs::UserProfile& user_profile_in =
          user_profiles_in[i];

        UserInfoExchangePool::UserProfile new_user_profile;

        std::ostringstream ostr;
        ostr << provider_id << "_" << user_profile_in.user_id;

        new_user_profile.user_id = ostr.str();
        new_user_profile.provider_id = provider_id;

        new_user_profile.plain_size = user_profile_in.plain_profile.length();
        new_user_profile.plain_user_info.reset(new_user_profile.plain_size);

        memcpy(
          new_user_profile.plain_user_info.get(),
          user_profile_in.plain_profile.get_buffer(),
          new_user_profile.plain_size);

        new_user_profile.plain_history_size =
          user_profile_in.plain_history_profile.length();

        new_user_profile.plain_history_user_info.reset(
          new_user_profile.plain_history_size);

        memcpy(
          new_user_profile.plain_history_user_info.get(),
          user_profile_in.plain_history_profile.get_buffer(),
          new_user_profile.plain_history_size);

        user_profiles.push_back(std::move(new_user_profile));
      }

      user_info_exchange_pool_->add_users(
        provider_id,
        user_profiles);

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << "UserInfoExchangerImpl::send_users(): " << std::endl
          << "Input parameters: " << std::endl
          << "  provider_id = '" << provider_id << "'" << std::endl
          << "  users: " << std::endl;

        for(CORBA::ULong i = 0; i < user_profiles_in.length(); ++i)
        {
          ostr << "    " << user_profiles_in[i].user_id << std::endl;
        }

        logger_->log(ostr.str(),
                     Logging::Logger::TRACE,
                     Aspect::USER_INFO_EXCHANGER);
      }
    }
    catch(const UserInfoExchangePool::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "UserInfoExchangerImpl::send_users(): "
        << "Caught UserInfoExchangePool::Exception: "
        << ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;

      ostr
        << "UserInfoExchangerImpl::send_users(): "
        "Caught eh::Exception. : " << ex.what()
        << " Input Parameters: provider_id = '" << provider_id<< "'"
        << " users:";

      for(CORBA::ULong i = 0; i < user_profiles_in.length(); ++i)
      {
        ostr << "    " << user_profiles_in[i].user_id;
      }

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCHANGER,
                   "ADS-IMPL-65");
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;

      ostr << "UserInfoExchangerImpl::send_users(...). "
        "Caught CORBA::SystemException. : " << ex 
        << " Input parameters: provider_id = '" << provider_id << "'"
        << " users:";

      for(CORBA::ULong i = 0; i < user_profiles_in.length(); ++i)
      {
        ostr << "    " << user_profiles_in[i].user_id;
      }

      logger_->log(ostr.str(),
                   Logging::Logger::ERROR,
                   Aspect::USER_INFO_EXCHANGER,
                   "ADS-IMPL-65");
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoExchanger::ImplementationException>(
          ostr.str());
    }
  }

  void UserInfoExchangerImpl::erase_old_profiles_() noexcept
  {
    const char FUN[] = "UserInfoExchngerImpl::erase_old_profiles_()";
    bool res = false;

    try
    {
      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->sstream(Logging::Logger::TRACE,
                         Aspect::USER_INFO_EXCHANGER)
          << "EraseOldProfilesTask started.";
      }

      if(user_info_exchange_pool_.in())
      {
        user_info_exchange_pool_->erase_old_profiles(expire_time_);
        res = true;
      }
      else
      {
        logger_->log(
          String::SubString(
            ": Can't do profiles cleaning - UserInfoExchanger isn't ready."),
          Logging::Logger::TRACE,
          Aspect::USER_INFO_EXCHANGER);
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
                       Aspect::USER_INFO_EXCHANGER,
                       "ADS-IMPL-66")
       << FUN
       << ": Can't delete old user profiles. Caught eh::Exception. "
         ": "
       << ex.what();
    }
    catch(...)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
                       Aspect::USER_INFO_EXCHANGER)
        << FUN
        << ": Can't delete old user profiles. Caught unknown exception. ";
    }

    if (res && logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->sstream(Logging::Logger::TRACE,
                       Aspect::USER_INFO_EXCHANGER)
        << FUN << ": cleanup old profiles task has finished.";
    }

    try
    {
      Generics::Time tm = Generics::Time::get_time_of_day();

      if (!res)
      {
        tm += 60;
      }
      else
      {
        tm += expire_time_;
      }

      Task_var msg =
        new EraseOldProfilesTask(this, task_runner_);
      scheduler_->schedule(msg, tm);

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->sstream(Logging::Logger::TRACE,
                         Aspect::USER_INFO_EXCHANGER)
          << FUN
          << "EraseOldProfilesTask was rescheduled.";
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
                       Aspect::USER_INFO_EXCHANGER,
                       "ADS-IMPL-52")
        << FUN << ": Can't schedule task. "
        "Caught eh::Exception. : "
        << ex.what();
    }
  }


} /*UserInfoSvcs*/
} /*AdServer*/

