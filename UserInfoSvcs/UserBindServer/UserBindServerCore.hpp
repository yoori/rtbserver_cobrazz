#pragma once

#include <atomic>
#include <string>
#include <vector>
#include <utility>

#include <eh/Exception.hpp>

#include <Commons/AccessActiveObject.hpp>
#include <Commons/UserIdBlackList.hpp>
#include <Commons/UserInfoManip.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>
#include <xsd/UserInfoSvcs/UserBindServerConfig.hpp>

#include "UserBindProcessor.hpp"
#include "BindRequestProcessor.hpp"
#include "UserBindContainer.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserBindServerCore:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    typedef xsd::AdServer::Configuration::UserBindServerConfigType
      UserBindServerConfig;

  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotReady, Exception);
    DECLARE_EXCEPTION(ChunkNotFound, Exception);

    struct BindRequestInfo
    {
      std::vector<std::string> bind_user_ids;
    };

    struct GetUserRequestInfo
    {
      std::string id;
      Generics::Time timestamp;
      bool silent = false;
      bool generate_user_id = false;
      bool for_set_cookie = false;
      Generics::Time create_timestamp;
      Commons::UserId current_user_id;
    };

    struct GetUserResponseInfo
    {
      Commons::UserId user_id;
      bool min_age_reached = false;
      bool created = false;
      bool invalid_operation = false;
      bool user_found = false;
    };

    struct AddUserRequestInfo
    {
      std::string id;
      Generics::Time timestamp;
      Commons::UserId user_id;
    };

    struct AddUserResponseInfo
    {
      Commons::UserId merge_user_id;
      bool invalid_operation = false;
    };

    struct Source
    {
      std::vector<unsigned long> chunks;
      unsigned long chunks_number = 0;
    };

    struct Stats
    {
      unsigned long get_user_id_total_requests = 0;
      unsigned long add_user_id_requests = 0;
    };

    typedef AdServer::Commons::AccessActiveObject<UserBindProcessor_var>
      UserBindProcessorHolder;
    typedef ReferenceCounting::SmartPtr<UserBindProcessorHolder>
      UserBindProcessorHolder_var;

    typedef AdServer::Commons::AccessActiveObject<BindRequestProcessor_var>
      BindRequestProcessorHolder;
    typedef ReferenceCounting::SmartPtr<BindRequestProcessorHolder>
      BindRequestProcessorHolder_var;

  public:
    UserBindServerCore(
      const UserBindServerConfig& user_bind_server_config,
      Logging::Logger* logger);

    virtual ~UserBindServerCore();

    BindRequestInfo get_bind_request(
      const std::string& id,
      const Generics::Time& timestamp);

    void add_bind_request(
      const std::string& id,
      const BindRequestInfo& bind_request,
      const Generics::Time& timestamp);

    GetUserResponseInfo get_user_id(const GetUserRequestInfo& request_info);

    AddUserResponseInfo add_user_id(const AddUserRequestInfo& request_info);

    Source get_source() const;

    Stats stats() const noexcept;

    const UserBindContainer::ChunkPathMap&
    chunks() const noexcept;

    unsigned long
    chunks_number() const noexcept;

    UserBindProcessorHolder_var
    user_bind_container() const noexcept;

    BindRequestProcessorHolder_var
    bind_request_container() const noexcept;

    void
    dump() const;

  private:
    class LoadUserBindTask: public Generics::TaskGoal
    {
    public:
      LoadUserBindTask(
        Generics::TaskRunner* task_runner,
        UserBindServerCore* core) noexcept;

      void execute() noexcept override;

    private:
      UserBindServerCore* core_;
    };

    class LoadBindRequestTask: public Generics::TaskGoal
    {
    public:
      LoadBindRequestTask(
        Generics::TaskRunner* task_runner,
        UserBindServerCore* core) noexcept;

      void execute() noexcept override;

    private:
      UserBindServerCore* core_;
    };

    class ClearUserBindExpiredTask: public Generics::TaskGoal
    {
    public:
      ClearUserBindExpiredTask(
        Generics::TaskRunner* task_runner,
        UserBindServerCore* core,
        bool reschedule) noexcept;

      void execute() noexcept override;

    private:
      UserBindServerCore* core_;
      bool reschedule_;
    };

    class ClearBindRequestExpiredTask: public Generics::TaskGoal
    {
    public:
      ClearBindRequestExpiredTask(
        Generics::TaskRunner* task_runner,
        UserBindServerCore* core,
        bool reschedule) noexcept;

      void execute() noexcept override;

    private:
      UserBindServerCore* core_;
      bool reschedule_;
    };

    class DumpUserBindTask: public Generics::TaskGoal
    {
    public:
      DumpUserBindTask(
        Generics::TaskRunner* task_runner,
        UserBindServerCore* core) noexcept;

      void execute() noexcept override;

    private:
      UserBindServerCore* core_;
    };

    void load_user_bind_() noexcept;
    void load_bind_request_() noexcept;
    void clear_user_bind_expired_(bool reschedule) noexcept;
    void clear_bind_request_expired_(bool reschedule) noexcept;
    void dump_user_bind_() noexcept;

  private:
    const Logging::Logger_var logger_;
    const UserBindServerConfig user_bind_server_config_;
    Generics::FixedPlanner_var scheduler_;
    Generics::FixedTaskRunner_var task_runner_;
    const UserBindProcessorHolder_var user_bind_container_;
    const BindRequestProcessorHolder_var bind_request_container_;

    UserBindContainer::ChunkPathMap chunks_;
    unsigned long chunks_number_;
    Commons::UserIdBlackList user_id_black_list_;
    mutable std::atomic<unsigned long> get_user_id_total_requests_{0};
    mutable std::atomic<unsigned long> add_user_id_requests_{0};
  };

  typedef ReferenceCounting::SmartPtr<UserBindServerCore>
    UserBindServerCore_var;
}
