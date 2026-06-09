#include "UserBindServerCore.hpp"

#include <utility>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindOperationSaver.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindOperationLoader.hpp>

#include "BindRequestContainer.hpp"

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    const Generics::Time RELOAD_PERIOD(60);
  }

  UserBindServerCore::LoadUserBindTask::LoadUserBindTask(
    Generics::TaskRunner* task_runner,
    UserBindServerCore* core) noexcept
    : Generics::TaskGoal(task_runner),
      core_(core)
  {}

  void
  UserBindServerCore::LoadUserBindTask::execute() noexcept
  {
    core_->load_user_bind_();
  }

  UserBindServerCore::LoadBindRequestTask::LoadBindRequestTask(
    Generics::TaskRunner* task_runner,
    UserBindServerCore* core) noexcept
    : Generics::TaskGoal(task_runner),
      core_(core)
  {}

  void
  UserBindServerCore::LoadBindRequestTask::execute() noexcept
  {
    core_->load_bind_request_();
  }

  UserBindServerCore::ClearUserBindExpiredTask::ClearUserBindExpiredTask(
    Generics::TaskRunner* task_runner,
    UserBindServerCore* core,
    bool reschedule) noexcept
    : Generics::TaskGoal(task_runner),
      core_(core),
      reschedule_(reschedule)
  {}

  void
  UserBindServerCore::ClearUserBindExpiredTask::execute() noexcept
  {
    core_->clear_user_bind_expired_(reschedule_);
  }

  UserBindServerCore::ClearBindRequestExpiredTask::ClearBindRequestExpiredTask(
    Generics::TaskRunner* task_runner,
    UserBindServerCore* core,
    bool reschedule) noexcept
    : Generics::TaskGoal(task_runner),
      core_(core),
      reschedule_(reschedule)
  {}

  void
  UserBindServerCore::ClearBindRequestExpiredTask::execute() noexcept
  {
    core_->clear_bind_request_expired_(reschedule_);
  }

  UserBindServerCore::DumpUserBindTask::DumpUserBindTask(
    Generics::TaskRunner* task_runner,
    UserBindServerCore* core) noexcept
    : Generics::TaskGoal(task_runner),
      core_(core)
  {}

  void
  UserBindServerCore::DumpUserBindTask::execute() noexcept
  {
    core_->dump_user_bind_();
  }

  UserBindServerCore::UserBindServerCore(
    const Config& config,
    Logging::Logger* logger)
    : logger_(ReferenceCounting::add_ref(logger)),
      config_(config),
      scheduler_(new Generics::Planner(
        Generics::ActiveObjectCallback_var(
          new Logging::ActiveObjectCallbackImpl(logger_, "", "UserBindServerCore")))),
      task_runner_(new Generics::TaskRunner(
        Generics::ActiveObjectCallback_var(
          new Logging::ActiveObjectCallbackImpl(logger_, "", "UserBindServerCore")),
        2)),
      user_bind_container_(new UserBindProcessorHolder()),
      bind_request_container_(new BindRequestProcessorHolder()),
      chunks_number_(config.storage.common_chunks_number)
  {
    add_child_object(task_runner_);
    add_child_object(scheduler_);
    add_child_object(user_bind_container_);
    add_child_object(bind_request_container_);

    AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
      chunks_,
      config.storage.chunks_root.c_str(),
      "Chunk");

    user_id_black_list_.load(config.user_id_black_list, logger_, "UserBindServer");

    task_runner_->enqueue_task(Generics::Task_var(new LoadUserBindTask(task_runner_, this)));
    task_runner_->enqueue_task(Generics::Task_var(new LoadBindRequestTask(task_runner_, this)));
    task_runner_->enqueue_task(Generics::Task_var(
      new ClearUserBindExpiredTask(task_runner_, this, true)));
    task_runner_->enqueue_task(Generics::Task_var(
      new ClearBindRequestExpiredTask(task_runner_, this, true)));
  }

  UserBindServerCore::~UserBindServerCore()
  {
    try
    {
      dump();
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::ERROR, "UserBindServer") <<
        "UserBindServerCore::~UserBindServerCore(): Can't dump user binds: " <<
        ex.what();
    }
    catch(...)
    {
      logger_->sstream(Logging::Logger::ERROR, "UserBindServer") <<
        "UserBindServerCore::~UserBindServerCore(): Can't dump user binds";
    }
  }

  AdServer::Commons::Task<UserBindServerCore::GetUserResponseInfo>
  UserBindServerCore::co_get_user_id(const GetUserRequestInfo& request_info)
  {
    get_user_id_total_requests_.fetch_add(1, std::memory_order_relaxed);

    UserBindProcessorHolder::Accessor user_bind_accessor =
      user_bind_container_->get_accessor();

    if(!user_bind_accessor.get())
    {
      throw NotReady("user bind container is not ready");
    }

    try
    {
      UserBindContainer::UserInfo user_info =
        co_await user_bind_accessor->co_get_user_id(
          String::SubString(request_info.id),
          request_info.current_user_id,
          request_info.timestamp,
          request_info.silent,
          request_info.create_timestamp,
          request_info.for_set_cookie);

      GetUserResponseInfo res;

      if(!user_info.user_id.is_null() ||
        !request_info.generate_user_id ||
        user_info.invalid_operation ||
        user_info.user_found)
      {
        if(user_id_black_list_.is_blacklisted(user_info.user_id))
        {
          const Commons::UserId new_user_id = Commons::UserId::create_random_based();

          user_info = co_await user_bind_accessor->co_add_user_id(
            String::SubString(request_info.id),
            new_user_id,
            request_info.timestamp,
            true,
            false);

          res.user_id = new_user_id;
          res.created = true;
          res.min_age_reached = true;
        }
        else
        {
          res.user_id = user_info.user_id;
          res.created = false;
          res.min_age_reached = user_info.min_age_reached;
        }

        res.invalid_operation = user_info.invalid_operation;
        res.user_found = user_info.user_found;

        co_return res;
      }

      const Commons::UserId new_user_id = Commons::UserId::create_random_based();
      user_info = co_await user_bind_accessor->co_add_user_id(
        String::SubString(request_info.id),
        new_user_id,
        request_info.timestamp,
        false,
        false);

      res.min_age_reached = true;
      res.user_found = user_info.user_found;

      if(!user_info.user_found)
      {
        res.user_id = new_user_id;
        res.invalid_operation = user_info.invalid_operation;
        res.created = true;
      }
      else
      {
        res.user_id = user_info.user_id;
        res.created = false;
      }

      co_return res;
    }
    catch(const UserBindProcessor::ChunkNotFound& ex)
    {
      throw ChunkNotFound(ex.what());
    }
  }

  AdServer::Commons::Task<UserBindServerCore::AddUserResponseInfo>
  UserBindServerCore::co_add_user_id(const AddUserRequestInfo& request_info)
  {
    add_user_id_requests_.fetch_add(1, std::memory_order_relaxed);

    UserBindProcessorHolder::Accessor user_bind_accessor =
      user_bind_container_->get_accessor();

    if(!user_bind_accessor.get())
    {
      throw NotReady("user bind container is not ready");
    }

    try
    {
      const UserBindContainer::UserInfo user_info =
        co_await user_bind_accessor->co_add_user_id(
          String::SubString(request_info.id),
          request_info.user_id,
          request_info.timestamp,
          true,
          false);

      AddUserResponseInfo res;
      res.merge_user_id = user_info.user_id;
      res.invalid_operation = user_info.invalid_operation;
      co_return res;
    }
    catch(const UserBindProcessor::ChunkNotFound& ex)
    {
      throw ChunkNotFound(ex.what());
    }
  }

  UserBindServerCore::BindRequestInfo
  UserBindServerCore::get_bind_request(
    const std::string& id,
    const Generics::Time& timestamp)
  {
    BindRequestProcessorHolder::Accessor bind_request_accessor =
      bind_request_container_->get_accessor();

    if(!bind_request_accessor.get())
    {
      throw NotReady("bind request container is not ready");
    }

    try
    {
      BindRequestProcessor::BindRequest bind_request =
        bind_request_accessor->get_bind_request(String::SubString(id), timestamp);

      BindRequestInfo res;
      res.bind_user_ids.swap(bind_request.bind_user_ids);
      return res;
    }
    catch(const BindRequestProcessor::ChunkNotFound& ex)
    {
      throw ChunkNotFound(ex.what());
    }
  }

  void
  UserBindServerCore::add_bind_request(
    const std::string& id,
    const BindRequestInfo& bind_request_info,
    const Generics::Time& timestamp)
  {
    BindRequestProcessorHolder::Accessor bind_request_accessor =
      bind_request_container_->get_accessor();

    if(!bind_request_accessor.get())
    {
      throw NotReady("bind request container is not ready");
    }

    try
    {
      BindRequestContainer::BindRequest bind_request;
      bind_request.bind_user_ids = bind_request_info.bind_user_ids;

      bind_request_accessor->add_bind_request(
        String::SubString(id),
        bind_request,
        timestamp);
    }
    catch(const BindRequestProcessor::ChunkNotFound& ex)
    {
      throw ChunkNotFound(ex.what());
    }
  }

  UserBindServerCore::Source
  UserBindServerCore::get_source() const
  {
    if(!active())
    {
      throw NotReady("core isn't active");
    }

    Source res;
    res.chunks.reserve(chunks_.size());

    for(UserBindContainer::ChunkPathMap::const_iterator it = chunks_.begin();
        it != chunks_.end(); ++it)
    {
      res.chunks.push_back(it->first);
    }

    res.chunks_number = chunks_number_;
    return res;
  }

  UserBindServerCore::Stats
  UserBindServerCore::stats() const noexcept
  {
    return {
      get_user_id_total_requests_.load(std::memory_order_relaxed),
      add_user_id_requests_.load(std::memory_order_relaxed)
    };
  }

  const UserBindContainer::ChunkPathMap&
  UserBindServerCore::chunks() const noexcept
  {
    return chunks_;
  }

  unsigned long
  UserBindServerCore::chunks_number() const noexcept
  {
    return chunks_number_;
  }

  UserBindServerCore::UserBindProcessorHolder_var
  UserBindServerCore::user_bind_container() const noexcept
  {
    return user_bind_container_;
  }

  UserBindServerCore::BindRequestProcessorHolder_var
  UserBindServerCore::bind_request_container() const noexcept
  {
    return bind_request_container_;
  }

  void
  UserBindServerCore::dump() const
  {
    const UserBindProcessor_var user_bind_processor =
      user_bind_container_->get_object();
    if(user_bind_processor.in())
    {
      user_bind_processor->dump();
    }

    const BindRequestProcessor_var bind_request_processor =
      bind_request_container_->get_object();
    if(bind_request_processor.in())
    {
      bind_request_processor->dump();
    }
  }

  void
  UserBindServerCore::load_user_bind_() noexcept
  {
    static const char* FUN = "UserBindServerCore::load_user_bind_()";

    bool reschedule = true;

    try
    {
      UserBindProcessor_var user_bind_processor = new UserBindContainer(
        logger_,
        config_.storage.common_chunks_number,
        chunks_,
        config_.storage.prefix.c_str(),
        config_.storage.bound_prefix.c_str(),
        config_.storage.expire_time / 4,
        config_.storage.bound_expire_time / 4,
        config_.min_age,
        config_.bind_on_min_age,
        config_.max_bad_event,
        config_.storage.portions,
        config_.storage.load_slave,
        config_.partition_index,
        config_.partitions_number);

      UserBindProcessor_var result_user_bind_processor;

      if(config_.operation_backup.has_value())
      {
        const auto& operation_backup = *config_.operation_backup;
        UserBindOperationSaver_var user_bind_operation_saver =
          new UserBindOperationSaver(
            logger_,
            operation_backup.dir.c_str(),
            operation_backup.file_prefix.c_str(),
            config_.storage.common_chunks_number,
            operation_backup.rotate_period,
            user_bind_processor);

        add_child_object(user_bind_operation_saver);
        result_user_bind_processor = user_bind_operation_saver;
      }
      else
      {
        result_user_bind_processor = user_bind_processor;
      }

      if(config_.operation_load.has_value())
      {
        const auto& operation_load = *config_.operation_load;
        UserBindOperationLoader::ChunkIdSet chunk_ids;
        for(UserBindContainer::ChunkPathMap::const_iterator chunk_it = chunks_.begin();
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
            operation_load.dir.c_str(),
            operation_load.unprocessed_dir.c_str(),
            operation_load.file_prefix.c_str(),
            operation_load.check_period,
            operation_load.threads,
            chunk_ids);

        add_child_object(user_bind_operation_loader);
      }

      *user_bind_container_ = result_user_bind_processor;
      reschedule = false;
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        "UserBindServer") << FUN << ": caught eh::Exception: " << ex.what();
    }

    if(reschedule)
    {
      try
      {
        scheduler_->schedule(
          Generics::Goal_var(new LoadUserBindTask(task_runner_, this)),
          Generics::Time::get_time_of_day() + RELOAD_PERIOD);
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          "UserBindServer") << FUN << ": Can't schedule task: " << ex.what();
      }
      return;
    }

    if(config_.storage.dump_period.has_value() &&
      *config_.storage.dump_period > Generics::Time::ZERO)
    {
      try
      {
        scheduler_->schedule(
          Generics::Goal_var(new DumpUserBindTask(task_runner_, this)),
          Generics::Time::get_time_of_day() + *config_.storage.dump_period);
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          "UserBindServer") << FUN << ": Can't schedule dump task: " << ex.what();
      }
    }
  }

  void
  UserBindServerCore::load_bind_request_() noexcept
  {
    static const char* FUN = "UserBindServerCore::load_bind_request_()";

    bool reschedule = true;
    try
    {
      BindRequestProcessor_var bind_request_processor = new BindRequestContainer(
        logger_,
        config_.bind_request_storage.common_chunks_number,
        chunks_,
        config_.bind_request_storage.prefix.c_str(),
        config_.bind_request_storage.expire_time / 4,
        config_.bind_request_storage.portions);
      *bind_request_container_ = bind_request_processor;
      reschedule = false;
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        "UserBindServer") << FUN << ": caught eh::Exception: " << ex.what();
    }

    if(reschedule)
    {
      try
      {
        scheduler_->schedule(
          Generics::Goal_var(new LoadBindRequestTask(task_runner_, this)),
          Generics::Time::get_time_of_day() + RELOAD_PERIOD);
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          "UserBindServer") << FUN << ": Can't schedule task: " << ex.what();
      }
    }
  }

  void
  UserBindServerCore::clear_user_bind_expired_(bool reschedule) noexcept
  {
    static const char* FUN = "UserBindServerCore::clear_user_bind_expired_()";

    UserBindProcessorHolder::Accessor user_bind_accessor =
      user_bind_container_->get_accessor();

    try
    {
      if(user_bind_accessor.get())
      {
        Generics::Time now = Generics::Time::get_time_of_day();
        user_bind_accessor->clear_expired(
          now - config_.storage.expire_time,
          now - config_.storage.bound_expire_time);
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        "UserBindServer") << FUN << ": Can't delete old user binds: " << ex.what();
    }

    if(reschedule)
    {
      scheduler_->schedule(
        Generics::Goal_var(new ClearUserBindExpiredTask(task_runner_, this, true)),
        Generics::Time::get_time_of_day() + Generics::Time::ONE_MINUTE);
    }
  }

  void
  UserBindServerCore::clear_bind_request_expired_(bool reschedule) noexcept
  {
    static const char* FUN = "UserBindServerCore::clear_bind_request_expired_()";

    BindRequestProcessorHolder::Accessor bind_request_accessor =
      bind_request_container_->get_accessor();

    try
    {
      if(bind_request_accessor.get())
      {
        Generics::Time now = Generics::Time::get_time_of_day();
        bind_request_accessor->clear_expired(
          now - config_.bind_request_storage.expire_time);
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        "UserBindServer") << FUN << ": Can't delete old bind requests: " << ex.what();
    }

    if(reschedule)
    {
      scheduler_->schedule(
        Generics::Goal_var(new ClearBindRequestExpiredTask(task_runner_, this, true)),
        Generics::Time::get_time_of_day() + Generics::Time::ONE_MINUTE);
    }
  }

  void
  UserBindServerCore::dump_user_bind_() noexcept
  {
    static const char* FUN = "UserBindServerCore::dump_user_bind_()";

    try
    {
      dump();
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        "UserBindServer") << FUN << ": Can't dump user binds: " << ex.what();
    }

    if(config_.storage.dump_period.has_value() &&
      *config_.storage.dump_period > Generics::Time::ZERO)
    {
      scheduler_->schedule(
        Generics::Goal_var(new DumpUserBindTask(task_runner_, this)),
        Generics::Time::get_time_of_day() + *config_.storage.dump_period);
    }
  }
}
