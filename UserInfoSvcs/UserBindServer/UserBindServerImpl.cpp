#include <list>
#include <vector>
#include <iterator>

#include <PrivacyFilter/Filter.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/FreqCapManip.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>

#include <UserInfoSvcs/UserInfoCommons/Allocator.hpp>

#include "UserBindContainer.hpp"
#include "UserBindOperationSaver.hpp"
#include "UserBindOperationLoader.hpp"
#include "UserBindServerImpl.hpp"

namespace Aspect
{
  const char USER_BIND_SERVER[] = "UserBindServer";
}

namespace AdServer
{
namespace UserInfoSvcs
{
  namespace
  {
    const Generics::Time RELOAD_PERIOD(60);
  }

  // UserBindServerImpl::ClearUserBindExpiredTask
  UserBindServerImpl::ClearUserBindExpiredTask::ClearUserBindExpiredTask(
    Generics::TaskRunner* task_runner,
    UserBindServerImpl* user_bind_server_impl,
    bool reschedule)
    noexcept
    : Generics::TaskGoal(task_runner),
      user_bind_server_impl_(user_bind_server_impl),
      reschedule_(reschedule)
  {}

  void
  UserBindServerImpl::ClearUserBindExpiredTask::execute()
    noexcept
  {
    user_bind_server_impl_->clear_user_bind_expired_(reschedule_);
  }

  // UserBindServerImpl::DumpUserBindTask
  UserBindServerImpl::DumpUserBindTask::DumpUserBindTask(
    Generics::TaskRunner* task_runner,
    UserBindServerImpl* user_bind_server_impl)
    noexcept
    : Generics::TaskGoal(task_runner),
      user_bind_server_impl_(user_bind_server_impl)
  {}

  void
  UserBindServerImpl::DumpUserBindTask::execute()
    noexcept
  {
    user_bind_server_impl_->dump_user_bind_();
  }

  // UserBindServerImpl::ClearBindRequestExpiredTask
  UserBindServerImpl::ClearBindRequestExpiredTask::ClearBindRequestExpiredTask(
    Generics::TaskRunner* task_runner,
    UserBindServerImpl* user_bind_server_impl,
    bool reschedule)
    noexcept
    : Generics::TaskGoal(task_runner),
      user_bind_server_impl_(user_bind_server_impl),
      reschedule_(reschedule)
  {}

  void
  UserBindServerImpl::ClearBindRequestExpiredTask::execute()
    noexcept
  {
    user_bind_server_impl_->clear_bind_request_expired_(reschedule_);
  }

  // UserBindServerImpl::LoadUserBindTask
  UserBindServerImpl::LoadUserBindTask::LoadUserBindTask(
    Generics::TaskRunner* task_runner,
    UserBindServerImpl* user_bind_server_impl)
    noexcept
    : Generics::TaskGoal(task_runner),
      user_bind_server_impl_(user_bind_server_impl)
  {}

  void
  UserBindServerImpl::LoadUserBindTask::execute()
    noexcept
  {
    user_bind_server_impl_->load_user_bind_();
  }

  // UserBindServerImpl::LoadBindRequestTask
  UserBindServerImpl::LoadBindRequestTask::LoadBindRequestTask(
    Generics::TaskRunner* task_runner,
    UserBindServerImpl* user_bind_server_impl)
    noexcept
    : Generics::TaskGoal(task_runner),
      user_bind_server_impl_(user_bind_server_impl)
  {}

  void
  UserBindServerImpl::LoadBindRequestTask::execute()
    noexcept
  {
    user_bind_server_impl_->load_bind_request_();
  }

  // UserBindServerImpl
  UserBindServerImpl::UserBindServerImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const UserBindServerConfig& user_bind_server_config)
    /*throw(Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_, 2)),
      user_bind_server_config_(user_bind_server_config),
      user_bind_container_(new UserBindProcessorHolder()),
      bind_request_container_(new BindRequestProcessorHolder())
  {
    static const char* FUN = "UserBindServerImpl::UserBindServerImpl()";

    try
    {
      AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
        chunks_,
        user_bind_server_config_.Storage().chunks_root().c_str(),
        "Chunk");
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      user_id_black_list_.load(user_bind_server_config, logger_, Aspect::USER_BIND_SERVER);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught on reading blacklist from xsd config: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      add_child_object(task_runner_);
      add_child_object(scheduler_);
      add_child_object(user_bind_container_);
      add_child_object(bind_request_container_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": CompositeActiveObject::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      Generics::Task_var load_user_bind_task(new LoadUserBindTask(task_runner_, this));
      task_runner_->enqueue_task(load_user_bind_task);

      Generics::Task_var load_bind_request_task(new LoadBindRequestTask(task_runner_, this));
      task_runner_->enqueue_task(load_bind_request_task);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      Generics::Task_var clear_user_bind_expired_task(
        new ClearUserBindExpiredTask(task_runner_, this, true));
      task_runner_->enqueue_task(clear_user_bind_expired_task);

      Generics::Task_var clear_bind_request_expired_task(
        new ClearBindRequestExpiredTask(task_runner_, this, true));
      task_runner_->enqueue_task(clear_bind_request_expired_task);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  AdServer::UserInfoSvcs::UserBindMapper::GetUserResponseInfo*
  UserBindServerImpl::get_user_id(
    const AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo&
      request_info)
    /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
      AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/
  {
    static const char* FUN = "UserBindServerImpl::get_user_id()";

    /*
    std::cerr << FUN << ": " << std::endl <<
      "  id = " << request_info.id.in() << std::endl <<
      "  current_user_id = " << CorbaAlgs::unpack_user_id(request_info.current_user_id).to_string() << std::endl <<
      "  request_info.silent = " << request_info.silent << std::endl <<
      "  request_info.generate_user_id = " << request_info.generate_user_id << std::endl <<
      "  request_info.for_set_cookie = " << request_info.for_set_cookie << std::endl <<
      std::endl;
    */

    UserBindProcessorHolder::Accessor user_bind_accessor =
      user_bind_container_->get_accessor();

    if(!user_bind_accessor.get())
    {
      throw AdServer::UserInfoSvcs::UserBindServer::NotReady();
    }

    const Generics::Time current_time = CorbaAlgs::unpack_time(
      request_info.timestamp);
    const String::SubString external_id(request_info.id);
    const Commons::UserId current_user_id =
      request_info.current_user_id.length() > 0 ?
      CorbaAlgs::unpack_user_id(request_info.current_user_id) :
      Commons::UserId();

    try
    {
      UserBindContainer::UserInfo user_info =
        user_bind_accessor->get_user_id(
          external_id,
          current_user_id,
          current_time,
          request_info.silent,
          CorbaAlgs::unpack_time(request_info.create_timestamp),
          request_info.for_set_cookie);

      if(!user_info.user_id.is_null() ||
         !request_info.generate_user_id ||
         user_info.invalid_operation ||
         user_info.user_found)
      {
        AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo_var res(
          new AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo());

        if (user_id_black_list_.is_blacklisted(user_info.user_id))
        {
          // generate new user id
          AdServer::Commons::UserId new_user_id =
            AdServer::Commons::UserId::create_random_based();

          user_info = user_bind_accessor->add_user_id(
            external_id,
            new_user_id,
            current_time,
            true, // rewrite user_id
            false // ignore_bad_event
            );

          res->user_id = CorbaAlgs::pack_user_id(new_user_id);
          res->created = true;
          res->min_age_reached = true;
        }
        else
        {
          res->user_id = CorbaAlgs::pack_user_id(user_info.user_id);
          res->created = false;
          res->min_age_reached = user_info.min_age_reached;
        }

        res->invalid_operation = user_info.invalid_operation;
        res->user_found = user_info.user_found;

        /*
        std::cerr << "get_user_id(): output #1" << std::endl <<
          "  id(input) = " << request_info.id << std::endl <<
          "  silent(input) = " << request_info.silent << std::endl <<
          "  generate_user_id(input) = " << request_info.generate_user_id << std::endl <<
          "  for_set_cookie(input) = " << request_info.for_set_cookie << std::endl <<
          "  -----------------------" << std::endl <<
          "  user_info.user_id = " << user_info.user_id << std::endl <<
          "  user_info.invalid_operation = " << user_info.invalid_operation << std::endl <<
          "  user_info.user_found = " << user_info.user_found << std::endl <<
          "  >>>>>>>>>>>>>>>>>>>>>>>" << std::endl <<
          "  user_id = " << CorbaAlgs::unpack_user_id(res->user_id) << std::endl <<
          "  min_age_reached = " << res->min_age_reached << std::endl <<
          "  created = " << res->created << std::endl <<
          "  invalid_operation = " << res->invalid_operation << std::endl <<
          "  user_found = " << res->user_found << std::endl <<
          std::endl;
        */
        return res._retn();
      }

      // generate new user id
      AdServer::Commons::UserId new_user_id =
        AdServer::Commons::UserId::create_random_based();
      user_info = user_bind_accessor->add_user_id(
        external_id,
        new_user_id,
        current_time,
        false, // don't change if exists
        false // ignore_bad_event
        );

      AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo_var res(
        new AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo());
      res->min_age_reached = true;
      res->user_found = user_info.user_found;

      if(!user_info.user_found)
      {
        res->user_id = CorbaAlgs::pack_user_id(new_user_id);
        res->invalid_operation = user_info.invalid_operation;
        res->created = true;
      }
      else
      {
        // other thread already created user ...
        res->user_id = CorbaAlgs::pack_user_id(user_info.user_id);
        res->created = false;
      }

      /*
      std::cerr << "get_user_id(): output #2" << std::endl <<
        "  user_id = " << CorbaAlgs::unpack_user_id(res->user_id) << std::endl <<
        "  min_age_reached = " << res->min_age_reached << std::endl <<
        "  created = " << res->created << std::endl <<
        "  invalid_operation = " << res->invalid_operation << std::endl <<
        "  user_found = " << res->user_found << std::endl <<
        std::endl;
      */
      return res._retn();
    }
    catch(const UserBindProcessor::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserBindProcessor::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserBindServer::ChunkNotFound>(
          ostr.str());
    }

    return 0;
  }

  AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo*
  UserBindServerImpl::add_user_id(
    const AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo&
      request_info)
    /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
      AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/
  {
    static const char* FUN = "UserBindServerImpl::add_user_id()";

    /*
    std::cerr << FUN << ": " << std::endl <<
      "  id = " << request_info.id.in() << std::endl <<
      "  user_id = " << CorbaAlgs::unpack_user_id(request_info.user_id).to_string() << std::endl <<
      std::endl;
    */

    UserBindProcessorHolder::Accessor user_bind_accessor =
      user_bind_container_->get_accessor();

    if(!user_bind_accessor.get())
    {
      throw AdServer::UserInfoSvcs::UserBindServer::NotReady();
    }

    try
    {
      UserBindContainer::UserInfo user_info =
        user_bind_accessor->add_user_id(
          String::SubString(request_info.id),
          CorbaAlgs::unpack_user_id(request_info.user_id),
          CorbaAlgs::unpack_time(request_info.timestamp),
          true, // resave if exists
          false // ignore_bad_event
          );

      AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo_var res(
        new AdServer::UserInfoSvcs::UserBindMapper::AddUserResponseInfo());
      res->merge_user_id = CorbaAlgs::pack_user_id(user_info.user_id);
      res->invalid_operation = user_info.invalid_operation;
      //res->min_age_reached = user_info.min_age_reached;
      //res->created = true;
      return res._retn();
    }
    catch(const UserBindProcessor::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserBindProcessor::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserBindServer::ChunkNotFound>(
          ostr.str());
    }

    return 0;
  }

  AdServer::UserInfoSvcs::UserBindMapper::BindRequestInfo*
  UserBindServerImpl::get_bind_request(
    const char* id,
    const CORBACommons::TimestampInfo& now)
    /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
      AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/
  {
    static const char* FUN = "UserBindServerImpl::get_bind_request()";

    BindRequestProcessorHolder::Accessor bind_request_accessor =
      bind_request_container_->get_accessor();

    if(!bind_request_accessor.get())
    {
      throw AdServer::UserInfoSvcs::UserBindServer::NotReady();
    }

    Generics::Time current_time = CorbaAlgs::unpack_time(now);
    String::SubString external_id(id);

    try
    {
      BindRequestProcessor::BindRequest bind_request =
        bind_request_accessor->get_bind_request(
          external_id,
          current_time);

      AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo_var res(
        new AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo());

      res->bind_user_ids.length(bind_request.bind_user_ids.size());

      CORBA::ULong bind_i = 0;
      for(auto bind_it = bind_request.bind_user_ids.begin();
          bind_it != bind_request.bind_user_ids.end(); ++bind_it, ++bind_i)
      {
        res->bind_user_ids[bind_i] << *bind_it;
      }

      return res._retn();
    }
    catch(const BindRequestProcessor::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught BindRequestProcessor::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserBindServer::ChunkNotFound>(
          ostr.str());
    }

    return 0;
  }

  void
  UserBindServerImpl::add_bind_request(
    const char* id,
    const AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo& bind_request_info,
    const CORBACommons::TimestampInfo& timestamp)
    /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady,
      AdServer::UserInfoSvcs::UserBindServer::ChunkNotFound)*/
  {
    static const char* FUN = "UserBindServerImpl::add_bind_request()";

    BindRequestProcessorHolder::Accessor bind_request_accessor =
      bind_request_container_->get_accessor();

    if(!bind_request_accessor.get())
    {
      throw AdServer::UserInfoSvcs::UserBindServer::NotReady();
    }

    try
    {
      BindRequestContainer::BindRequest bind_request;
      bind_request.bind_user_ids.reserve(bind_request_info.bind_user_ids.length());
      for(CORBA::ULong bind_i = 0; bind_i < bind_request_info.bind_user_ids.length(); ++bind_i)
      {
        bind_request.bind_user_ids.push_back(bind_request_info.bind_user_ids[bind_i].in());
      }

      bind_request_accessor->add_bind_request(
        String::SubString(id),
        bind_request,
        CorbaAlgs::unpack_time(timestamp)
        );
    }
    catch(const UserBindProcessor::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught UserBindProcessor::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<AdServer::UserInfoSvcs::
        UserBindServer::ChunkNotFound>(
          ostr.str());
    }
  }

  AdServer::UserInfoSvcs::UserBindServer::Source*
  UserBindServerImpl::get_source()
    /*throw(AdServer::UserInfoSvcs::UserBindServer::NotReady)*/
  {
    AdServer::UserInfoSvcs::UserBindServer::Source_var res =
      new AdServer::UserInfoSvcs::UserBindServer::Source();
    res->chunks.length(chunks_.size());
    CORBA::ULong chunk_i = 0;
    for(UserBindContainer::ChunkPathMap::const_iterator chunk_it =
          chunks_.begin();
        chunk_it != chunks_.end(); ++chunk_it, ++chunk_i)
    {
      res->chunks[chunk_i] = chunk_it->first;
    }

    res->chunks_number = user_bind_server_config_.Storage().common_chunks_number();

    return res._retn();
  }

  void
  UserBindServerImpl::wait_object()
    /*throw(Generics::ActiveObject::Exception, eh::Exception)*/
  {
    static const char* FUN = "UserBindServerImpl::wait_object()";

    Generics::CompositeActiveObject::wait_object();

    // dump container only when its usage stopped (AccessActiveObject)
    try
    {
      user_bind_container_->get_object()->dump();
      bind_request_container_->get_object()->dump();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't dump containers: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserBindServerImpl::load_user_bind_()
    noexcept
  {
    static const char* FUN = "UserBindServerImpl::load_user_bind_()";

    bool reschedule = true;

    try
    {
      UserBindProcessor_var user_bind_processor = new UserBindContainer(
        logger_,
        user_bind_server_config_.Storage().common_chunks_number(),
        chunks_,
        user_bind_server_config_.Storage().prefix().c_str(),
        user_bind_server_config_.Storage().bound_prefix().c_str(),
        Generics::Time(user_bind_server_config_.Storage().expire_time() / 4),
        Generics::Time(user_bind_server_config_.Storage().bound_expire_time() / 4),
        Generics::Time(user_bind_server_config_.min_age()),
        user_bind_server_config_.bind_on_min_age(),
        user_bind_server_config_.max_bad_event(),
        user_bind_server_config_.Storage().portions(),
        (user_bind_server_config_.Storage().user_bind_keep_mode() == "keep slave"), // load_slave
        user_bind_server_config_.partition_index(),
        user_bind_server_config_.partitions_number());

      UserBindProcessor_var res_user_bind_processor;

      if(user_bind_server_config_.OperationBackup().present())
      {
        UserBindOperationSaver_var user_bind_operation_saver =
          new UserBindOperationSaver(
            logger_,
            user_bind_server_config_.OperationBackup()->dir().c_str(),
            user_bind_server_config_.OperationBackup()->file_prefix().c_str(),
            user_bind_server_config_.Storage().common_chunks_number(),
            Generics::Time(user_bind_server_config_.OperationBackup()->rotate_period()),
            user_bind_processor);

        add_child_object(user_bind_operation_saver);

        res_user_bind_processor = user_bind_operation_saver;
      }
      else
      {
        res_user_bind_processor = user_bind_processor;
      }

      if(user_bind_server_config_.OperationLoad().present())
      {
        UserBindOperationLoader::ChunkIdSet chunk_ids;

        for(UserBindContainer::ChunkPathMap::const_iterator chunk_it =
              chunks_.begin();
            chunk_it != chunks_.end(); ++chunk_it)
        {
          chunk_ids.insert(chunk_it->first);
        }

        Generics::ActiveObject_var user_bind_operation_loader =
          new UserBindOperationLoader(
            Generics::ActiveObjectCallback_var(
              new Logging::ActiveObjectCallbackImpl(
                logger_,
                "",
                "UserBindOperationLoader")),
            user_bind_processor,
            user_bind_server_config_.OperationLoad()->dir().c_str(),
            user_bind_server_config_.OperationLoad()->unprocessed_dir().c_str(),
            user_bind_server_config_.OperationLoad()->file_prefix().c_str(),
            Generics::Time(user_bind_server_config_.OperationLoad()->check_period()),
            user_bind_server_config_.OperationLoad()->threads(),
            chunk_ids);

        add_child_object(user_bind_operation_loader);
      }

      *user_bind_container_ = res_user_bind_processor;

      reschedule = false;

      if(user_bind_server_config_.Storage().dump_period().present() &&
        *user_bind_server_config_.Storage().dump_period() > 0)
      {
        // run periodic dump
        try
        {
          Generics::Time now = Generics::Time::get_time_of_day();

          Generics::Goal_var goal(new DumpUserBindTask(task_runner_, this));
          scheduler_->schedule(
            goal,
            now + *user_bind_server_config_.Storage().dump_period());
        }
        catch(const eh::Exception& ex)
        {
          logger_->sstream(Logging::Logger::EMERGENCY,
            Aspect::USER_BIND_SERVER) << FUN <<
            ": Can't schedule task. Caught eh::Exception: " <<
            ex.what();
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_BIND_SERVER) << FUN <<
        ": caught eh::Exception: " <<
        ex.what();
    }

    if(reschedule)
    {
      try
      {
        Generics::Time now = Generics::Time::get_time_of_day();

        Generics::Goal_var goal(new LoadUserBindTask(task_runner_, this));
        scheduler_->schedule(goal, now + RELOAD_PERIOD);
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::USER_BIND_SERVER) << FUN <<
          ": Can't schedule task. Caught eh::Exception: " <<
          ex.what();
      }
    }
  }

  void
  UserBindServerImpl::load_bind_request_()
    noexcept
  {
    static const char* FUN = "UserBindServerImpl::load_bind_request_()";

    bool reschedule = true;

    try
    {
      BindRequestProcessor_var bind_request_processor = new BindRequestContainer(
        logger_,
        user_bind_server_config_.BindRequestStorage().common_chunks_number(),
        chunks_,
        user_bind_server_config_.BindRequestStorage().prefix().c_str(),
        Generics::Time(user_bind_server_config_.BindRequestStorage().expire_time() / 4),
        user_bind_server_config_.BindRequestStorage().portions());

      *bind_request_container_ = bind_request_processor;

      reschedule = false;
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_BIND_SERVER) << FUN <<
        ": caught eh::Exception: " <<
        ex.what();
    }

    if(reschedule)
    {
      try
      {
        Generics::Time now = Generics::Time::get_time_of_day();

        Generics::Goal_var goal(new LoadBindRequestTask(task_runner_, this));
        scheduler_->schedule(goal, now + RELOAD_PERIOD);
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::USER_BIND_SERVER) << FUN <<
          ": Can't schedule task. Caught eh::Exception: " <<
          ex.what();
      }
    }
  }

  void
  UserBindServerImpl::clear_user_bind_expired_(bool reschedule)
    noexcept
  {
    static const char* FUN = "UserBindServerImpl::clear_user_bind_expired_()";

    try
    {
      UserBindProcessorHolder::Accessor user_bind_accessor =
        user_bind_container_->get_accessor();

      if(user_bind_accessor.get())
      {
        Generics::Time now = Generics::Time::get_time_of_day();
        user_bind_accessor->clear_expired(
          now - Generics::Time(user_bind_server_config_.Storage().expire_time()),
          now - Generics::Time(user_bind_server_config_.Storage().bound_expire_time()));
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_BIND_SERVER) << FUN <<
        ": Can't delete old user binds. Caught eh::Exception: " <<
        ex.what();
    }

    if(reschedule)
    {
      try
      {
        Generics::Time now = Generics::Time::get_time_of_day();

        Generics::Goal_var clear_expired_goal(
          new ClearUserBindExpiredTask(task_runner_, this, reschedule));
        scheduler_->schedule(
          clear_expired_goal,
          now + Generics::Time::ONE_MINUTE);
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::USER_BIND_SERVER) << FUN <<
          ": Can't schedule task. Caught eh::Exception: " <<
          ex.what();
      }
    }
  }

  void
  UserBindServerImpl::dump_user_bind_()
    noexcept
  {
    static const char* FUN = "UserBindServerImpl::dump_user_bind_()";

    try
    {
      user_bind_container_->get_object()->dump();
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_BIND_SERVER) << FUN <<
        ": Can't dump user binds. Caught eh::Exception: " <<
        ex.what();
    }

    if(user_bind_server_config_.Storage().dump_period().present() &&
      *user_bind_server_config_.Storage().dump_period() > 0)
    {
      try
      {
        Generics::Time now = Generics::Time::get_time_of_day();

        Generics::Goal_var dump_goal(
          new DumpUserBindTask(task_runner_, this));
        scheduler_->schedule(
          dump_goal,
          now + *user_bind_server_config_.Storage().dump_period());
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::USER_BIND_SERVER) << FUN <<
          ": Can't schedule task. Caught eh::Exception: " <<
          ex.what();
      }
    }
  }

  void
  UserBindServerImpl::clear_bind_request_expired_(bool reschedule)
    noexcept
  {
    static const char* FUN = "UserBindServerImpl::clear_bind_request_expired_()";

    try
    {
      BindRequestProcessorHolder::Accessor bind_request_accessor =
        bind_request_container_->get_accessor();

      if(bind_request_accessor.get())
      {
        Generics::Time now = Generics::Time::get_time_of_day();
        bind_request_accessor->clear_expired(
          now - Generics::Time(user_bind_server_config_.BindRequestStorage().expire_time()));
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_BIND_SERVER) << FUN <<
        ": Can't delete old user binds. Caught eh::Exception: " <<
        ex.what();
    }

    if(reschedule)
    {
      try
      {
        Generics::Time now = Generics::Time::get_time_of_day();

        Generics::Goal_var clear_expired_goal(
          new ClearBindRequestExpiredTask(task_runner_, this, reschedule));
        scheduler_->schedule(
          clear_expired_goal,
          now + Generics::Time::ONE_MINUTE);
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::USER_BIND_SERVER) << FUN <<
          ": Can't schedule task. Caught eh::Exception: " <<
          ex.what();
      }
    }
  }
} /*UserInfoSvcs*/
} /*AdServer*/
