#pragma once

#include <atomic>
#include <optional>
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

#include "UserBindProcessor.hpp"
#include "BindRequestProcessor.hpp"
#include "UserBindContainer.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserBindServerCore:
    public Generics::CompositeActiveObject,
    public virtual Generics::RefCountableActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    struct StorageConfig
    {
      std::string chunks_root;
      std::string prefix;
      std::string bound_prefix;
      unsigned long common_chunks_number = 0;
      Generics::Time expire_time;
      Generics::Time bound_expire_time;
      std::optional<Generics::Time> dump_period;
      unsigned long portions = 0;
      unsigned long rocksdb_batching_threads = 2;
      bool load_slave = false;
    };

    struct BindRequestStorageConfig
    {
      std::string prefix;
      unsigned long common_chunks_number = 0;
      Generics::Time expire_time;
      unsigned long portions = 0;
    };

    struct OperationBackupConfig
    {
      std::string dir;
      std::string file_prefix;
      Generics::Time rotate_period;
      unsigned long threads = 1;
    };

    struct OperationLoadConfig
    {
      std::string dir;
      std::string unprocessed_dir;
      std::string file_prefix;
      Generics::Time check_period;
      unsigned long threads = 0;
    };

    struct Config
    {
      StorageConfig storage;
      BindRequestStorageConfig bind_request_storage;
      std::optional<OperationBackupConfig> operation_backup;
      std::optional<OperationLoadConfig> operation_load;
      std::string user_id_black_list;
      std::optional<Generics::Time> bind_min_age;
      unsigned long max_bad_event = 0;
      unsigned long partition_index = 0;
      unsigned long partitions_number = 1;
    };

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

    using UserBindProcessorHolder = AdServer::Commons::AccessActiveObject<UserBindProcessor_var>;
    using UserBindProcessorHolder_var = ReferenceCounting::SmartPtr<UserBindProcessorHolder>;

    using UserBindContainerHolder = AdServer::Commons::AccessActiveObject<UserBindContainer_var>;
    using UserBindContainerHolder_var = ReferenceCounting::SmartPtr<UserBindContainerHolder>;

    using BindRequestProcessorHolder =
      AdServer::Commons::AccessActiveObject<BindRequestProcessor_var>;
    using BindRequestProcessorHolder_var = ReferenceCounting::SmartPtr<BindRequestProcessorHolder>;

  public:
    UserBindServerCore(const Config& config, Logging::Logger* logger);

    virtual ~UserBindServerCore();

    AdServer::Commons::StartableAwaitable<BindRequestInfo>
    co_get_bind_request(const std::string& id, const Generics::Time& timestamp);

    BindRequestInfo get_bind_request(const std::string& id, const Generics::Time& timestamp);

    void add_bind_request(
      const std::string& id,
      const BindRequestInfo& bind_request,
      const Generics::Time& timestamp);

    AdServer::Commons::StartableAwaitable<GetUserResponseInfo>
    co_get_user_id(const GetUserRequestInfo& request_info);

    AdServer::Commons::StartableAwaitable<AddUserResponseInfo>
    co_add_user_id(const AddUserRequestInfo& request_info);

    Source get_source() const;

    Stats stats() const noexcept;

    AdServer::ProfilingCommons::RocksDBProfileMapProcessor::Stats
    rocksdb_stats() const noexcept;

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
      LoadUserBindTask(Generics::TaskRunner* task_runner, UserBindServerCore* core) noexcept;

      void execute() noexcept override;

    private:
      UserBindServerCore* core_;
    };

    class LoadBindRequestTask: public Generics::TaskGoal
    {
    public:
      LoadBindRequestTask(Generics::TaskRunner* task_runner, UserBindServerCore* core) noexcept;

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
      DumpUserBindTask(Generics::TaskRunner* task_runner, UserBindServerCore* core) noexcept;

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
    const Config config_;
    Generics::FixedPlanner_var scheduler_;
    Generics::FixedTaskRunner_var task_runner_;
    const UserBindProcessorHolder_var user_bind_container_;
    const UserBindContainerHolder_var rocksdb_stats_container_;
    const BindRequestProcessorHolder_var bind_request_container_;

    UserBindContainer::ChunkPathMap chunks_;
    unsigned long chunks_number_;
    Commons::UserIdBlackList user_id_black_list_;
    mutable std::atomic<unsigned long> get_user_id_total_requests_{0};
    mutable std::atomic<unsigned long> add_user_id_requests_{0};
  };

  using UserBindServerCore_var = ReferenceCounting::SmartPtr<UserBindServerCore>;
}
