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

#include <xsd/UserInfoSvcs/UserInfoControllerConfig.hpp>

#include "UserInfoControllerGrpc.pb.h"

namespace AdServer::UserInfoSvcs
{
  class UserInfoControllerImpl:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);

    using UserInfoControllerConfig = xsd::AdServer::Configuration::UserInfoControllerConfigType;
    using SessionDescription =
      adserver::user_info_svcs::user_info_controller::GetSessionDescriptionResponse;

    UserInfoControllerImpl(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      const UserInfoControllerConfig& config);

    void fill_session_description(SessionDescription& response) const;

  protected:
    ~UserInfoControllerImpl() noexcept override;

  private:
    using TaskBase = Generics::TaskGoal;
    using Task_var = ReferenceCounting::SmartPtr<TaskBase>;
    using ChunkIdSet = std::set<unsigned long>;
    using SyncPolicy = Sync::Policy::PosixThreadRW;

    class InitUserInfoManagerSourceTask;
    class CheckUserInfoManagerStateTask;

    struct UserInfoManagerRef
    {
      std::string endpoint;
      ChunkIdSet chunks;
      bool ready = false;
    };

    using UserInfoManagerRefArray = std::vector<UserInfoManagerRef>;

    class UserInfoConfig: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      bool all_ready = false;
      bool first_all_ready = false;
      unsigned long common_chunks_number = 0;
      UserInfoManagerRefArray user_info_managers;

    private:
      ~UserInfoConfig() noexcept override = default;
    };

    using UserInfoConfig_var = ReferenceCounting::SmartPtr<UserInfoConfig>;

    void fill_refs_();

    bool get_user_info_manager_sources_(UserInfoConfig* user_info_config);

    void init_user_info_manager_state_() noexcept;

    void check_user_info_manager_state_() noexcept;

    void check_source_consistency_(UserInfoConfig* user_info_config) const;

  private:
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var logger_;
    mutable SyncPolicy::Mutex lock_;
    Generics::Planner_var scheduler_;
    Generics::TaskRunner_var task_runner_;
    const UserInfoControllerConfig config_;
    UserInfoConfig_var user_info_config_;
  };

  using UserInfoControllerImpl_var = ReferenceCounting::SmartPtr<UserInfoControllerImpl>;
}

namespace AdServer::UserInfoSvcs
{
  class UserInfoControllerImpl::InitUserInfoManagerSourceTask: public TaskBase
  {
  public:
    InitUserInfoManagerSourceTask(
      UserInfoControllerImpl* controller,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        controller_(controller)
    {}

    void execute() noexcept override
    {
      controller_->init_user_info_manager_state_();
    }

  private:
    ~InitUserInfoManagerSourceTask() noexcept override = default;

    UserInfoControllerImpl* controller_;
  };

  class UserInfoControllerImpl::CheckUserInfoManagerStateTask: public TaskBase
  {
  public:
    CheckUserInfoManagerStateTask(
      UserInfoControllerImpl* controller,
      Generics::TaskRunner* task_runner)
      noexcept
      : TaskBase(task_runner),
        controller_(controller)
    {}

    void execute() noexcept override
    {
      controller_->check_user_info_manager_state_();
    }

  private:
    ~CheckUserInfoManagerStateTask() noexcept override = default;

    UserInfoControllerImpl* controller_;
  };
}
