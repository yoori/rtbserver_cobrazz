#pragma once

#include <set>
#include <string>
#include <vector>

#include <eh/Exception.hpp>

#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Sync/SyncPolicy.hpp>

#include <xsd/UserInfoSvcs/UserBindController2Config.hpp>

#include "UserBindControllerGrpc.pb.h"

namespace AdServer::UserInfoSvcs
{
  class UserBindController2Impl:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);

    using UserBindControllerConfig =
      xsd::AdServer::Configuration::UserBindController2ConfigType;
    using SessionDescription =
      adserver::user_info_svcs::user_bind_controller::GetSessionDescriptionResponse;

    UserBindController2Impl(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const UserBindControllerConfig& config);

    void fill_session_description(SessionDescription& response) const;

  protected:
    ~UserBindController2Impl() noexcept override;

  private:
    using TaskBase = Generics::TaskGoal;
    using Task_var = ReferenceCounting::SmartPtr<TaskBase>;
    using ChunkIdSet = std::set<unsigned long>;
    using SyncPolicy = Sync::Policy::PosixThreadRW;

    class InitUserBindSourceTask;
    class CheckUserBindServerStateTask;

    struct UserBindServerRef
    {
      std::string endpoint;
      ChunkIdSet chunks;
      bool ready = false;
    };

    using UserBindServerRefArray = std::vector<UserBindServerRef>;

    class UserBindConfig: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      bool all_ready = false;
      bool first_all_ready = false;
      unsigned long common_chunks_number = 0;
      UserBindServerRefArray user_bind_servers;

    private:
      ~UserBindConfig() noexcept override = default;
    };

    using UserBindConfig_var = ReferenceCounting::SmartPtr<UserBindConfig>;

    void fill_refs_();

    bool get_user_bind_server_sources_(UserBindConfig* user_bind_config);

    void init_user_bind_state_() noexcept;

    void check_user_bind_state_() noexcept;

    void check_source_consistency_(UserBindConfig* user_bind_config) const;

  private:
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var logger_;
    mutable SyncPolicy::Mutex lock_;
    Generics::Planner_var scheduler_;
    Generics::TaskRunner_var task_runner_;
    const UserBindControllerConfig config_;
    UserBindConfig_var user_bind_config_;
  };

  using UserBindController2Impl_var =
    ReferenceCounting::SmartPtr<UserBindController2Impl>;
}

namespace AdServer::UserInfoSvcs
{
  class UserBindController2Impl::InitUserBindSourceTask: public TaskBase
  {
  public:
    InitUserBindSourceTask(
      UserBindController2Impl* controller,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        controller_(controller)
    {}

    void execute() noexcept override
    {
      controller_->init_user_bind_state_();
    }

  private:
    ~InitUserBindSourceTask() noexcept override = default;

    UserBindController2Impl* controller_;
  };

  class UserBindController2Impl::CheckUserBindServerStateTask: public TaskBase
  {
  public:
    CheckUserBindServerStateTask(
      UserBindController2Impl* controller,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        controller_(controller)
    {}

    void execute() noexcept override
    {
      controller_->check_user_bind_state_();
    }

  private:
    ~CheckUserBindServerStateTask() noexcept override = default;

    UserBindController2Impl* controller_;
  };
}
