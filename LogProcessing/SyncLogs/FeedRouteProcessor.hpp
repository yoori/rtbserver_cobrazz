// @file SyncLogs/FeedRouteProcessor.hpp

#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <LogCommons/FileReceiver.hpp>

#include "FeedRouteMover.hpp"

/**
 * BasicFeedRouteProcessor
 * FeedRouteProcessor
 *   implement serialized (in process method call thread) files processing
 * ThreadPoolFeedRouteProcessor
 *   process call only fetch files and delegate its processing to predefined thread pool (
 *     that can be shared between route processors)
 *   it don't do file processing in call context for full load thread pool
 */
namespace AdServer::LogProcessing
{
  class MoveTaskScheduler final: public std::enable_shared_from_this<MoveTaskScheduler>
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    MoveTaskScheduler(
      Generics::TaskRunner* task_runner,
      unsigned hard_threads,
      unsigned soft_threads,
      const Generics::Time& soft_max_file_age);

    void
    enqueue_task(
      Generics::Task* task,
      const char* file_path);

    void
    enqueue_task(
      Generics::Task* task,
      const Generics::Time& modification_time);

  public:
    ~MoveTaskScheduler() = default;

  private:
    class ScheduledTask final : public Generics::TaskImpl
    {
    public:
      ScheduledTask(
        const std::shared_ptr<MoveTaskScheduler>& scheduler,
        Generics::Task* task);

      void
      execute() noexcept override;

    protected:
      ~ScheduledTask() override = default;

    private:
      std::shared_ptr<MoveTaskScheduler> scheduler_;
      Generics::Task_var task_;
    };

    using PendingKey = std::pair<Generics::Time, std::uint64_t>;
    using PendingTasks = std::map<PendingKey, Generics::Task_var>;
    using SyncPolicy = Sync::Policy::PosixThread;

    void
    task_finished_() noexcept;

    void
    dispatch_();

  private:
    Generics::FixedTaskRunner_var task_runner_;
    const unsigned hard_threads_;
    const unsigned soft_threads_;
    const Generics::Time soft_max_file_age_;
    const bool soft_mode_enabled_;

    SyncPolicy::Mutex lock_;
    PendingTasks pending_tasks_;
    std::uint64_t next_task_id_ = 0;
    unsigned running_tasks_ = 0;
  };

  using MoveTaskScheduler_var = std::shared_ptr<MoveTaskScheduler>;

  /** BasicFeedRouteProcessor */
  class BasicFeedRouteProcessor:
    public RouteProcessor,
    public Generics::RefCountableCompositeActiveObject
  {
  public:
    using Exception = FeedRouteMover::Exception;

    enum CommandType
    {
      CT_GENERIC,
      CT_RSYNC
    };

    enum FetchType
    {
      FT_FULL, // classic fetching via directory_selector
      FT_NEW, // fetching via FileReceiver in regular mode
      FT_COMMITED // fetching via FileReceiver in commit mode
    };

    typedef FileReceiver::FileGuard_var FileEntry;
    typedef std::list<FileReceiver::FileGuard_var> FileEntries;

  public:
    BasicFeedRouteProcessor(
      Utils::ErrorPool* error_logger,
      const LocalInterfaceChecker& host_checker,
      const char* src_files_pattern,
      RouteBasicHelper* dest_router,
      const char* dst_dir,
      unsigned long tries_per_file,
      const char* local_copy_command_templ,
      CommandType local_copy_command_type,
      const char* remote_copy_command_templ,
      CommandType remote_copy_command_type,
      bool parse_source,
      bool unlink_source,
      bool interruptible,
      SchedType feed_type,
      const char* post_command,
      FetchType fetch_type)
      /*throw(Exception)*/;

    virtual
    ~BasicFeedRouteProcessor() noexcept {}

    virtual void process() noexcept;

    static FetchType get_fetch_type(const std::string& str)
      /*throw(Exception)*/;

  protected:
    virtual void
    process_files_(
      StringList& unlink_files,
      const FileEntries& files,
      const char* src_dir)
      noexcept = 0;

  private:
    FileEntries make_file_list_for_feed_() const
      /*throw(eh::Exception)*/;

    void
    configure_fetch_mode_(FetchType fetch_type) /*throw(eh::Exception)*/;

  protected:
    Utils::ErrorPool* const error_logger_;

    const bool parse_source_;
    std::string file_mask_;

    FeedRouteMover_var mover_;
    StringList unlink_files_;

  private:
    std::string src_dir_;
    FileReceiver_var file_receiver_;
  };

  using BasicFeedRouteProcessor_var = ReferenceCounting::QualPtr<BasicFeedRouteProcessor>;

  /** FeedRouteProcessor */
  class FeedRouteProcessor: public BasicFeedRouteProcessor
  {
  public:
    FeedRouteProcessor(
      Utils::ErrorPool* error_logger,
      const LocalInterfaceChecker& host_checker,
      const char* src_files_pattern,
      RouteBasicHelper* dest_router,
      const char* dst_dir,
      unsigned long tries_per_file,
      const char* local_copy_command_templ,
      CommandType local_copy_command_type,
      const char* remote_copy_command_templ,
      CommandType remote_copy_command_type,
      bool parse_source,
      bool unlink_source,
      bool interruptible,
      SchedType feed_type,
      const char* post_command,
      FetchType fetch_type)
      /*throw(BasicFeedRouteProcessor::Exception)*/;

  protected:
    virtual ~FeedRouteProcessor() noexcept {}

    virtual void process_files_(
      StringList& unlink_files,
      const FileEntries& files,
      const char* src_dir)
      noexcept;
  };

  /** ThreadPoolFeedRouteProcessor */
  class ThreadPoolFeedRouteProcessor: public BasicFeedRouteProcessor
  {
  public:
    ThreadPoolFeedRouteProcessor(
      Utils::ErrorPool* error_logger,
      const LocalInterfaceChecker& host_checker,
      const char* src_files_pattern,
      RouteBasicHelper* dest_router,
      const char* dst_dir,
      unsigned long tries_per_file,
      const char* local_copy_command_templ,
      CommandType local_copy_command_type,
      const char* remote_copy_command_templ,
      CommandType remote_copy_command_type,
      bool parse_source,
      bool unlink_source,
      bool interruptible,
      SchedType feed_type,
      const MoveTaskScheduler_var& task_scheduler,
      const char* post_command,
      FetchType fetch_type)
      /*throw(BasicFeedRouteProcessor::Exception)*/;

    virtual void
    deactivate_object() noexcept;

  protected:
    struct MoveState: public virtual ReferenceCounting::AtomicImpl
    {
      using FileNameSet = std::set<std::string>;
      using SyncPolicy = Sync::Policy::PosixThread;

      MoveState(): tasks_count(0) {};

      Sync::Semaphore tasks_count;
      SyncPolicy::Mutex lock;
      FileNameSet unlink_files;

    protected:
      virtual ~MoveState() noexcept {}
    };

    using FixedMoveState_var = ReferenceCounting::FixedPtr<MoveState>;

    class MoveTask:
      public virtual Generics::Task,
      public ReferenceCounting::AtomicImpl
    {
    public:
      MoveTask(
        FeedRouteMover* route_mover,
        MoveState* move_state,
        const char* src_path,
        const FileEntry& src_file) noexcept
        : route_mover_(ReferenceCounting::add_ref(route_mover)),
          move_state_(ReferenceCounting::add_ref(move_state)),
          src_path_(src_path),
          src_file_(src_file)
      {}

      virtual void execute() noexcept;

    protected:
      virtual ~MoveTask() noexcept {}

    private:
      FixedFeedRouteMover_var route_mover_;
      FixedMoveState_var move_state_;
      const std::string src_path_;
      const FileEntry src_file_;
    };

  protected:
    virtual ~ThreadPoolFeedRouteProcessor() noexcept {}

    virtual void process_files_(
      StringList& unlink_files,
      const FileEntries& files,
      const char* src_dir)
      noexcept;

  private:
    MoveTaskScheduler_var task_scheduler_;
    FixedMoveState_var move_state_;
  };
}

namespace AdServer::LogProcessing
{
  inline
  void
  ThreadPoolFeedRouteProcessor::MoveTask::execute() noexcept
  {
    try
    {
      if (!route_mover_->move(src_path_.c_str(), src_file_->file_name().c_str()))
      {
        src_file_->revert();
      }
    }
    catch (const Utils::UnlinkException& ex)
    {
      MoveState::SyncPolicy::WriteGuard lock(move_state_->lock);
      move_state_->unlink_files.insert(ex.file_name);
    }

    move_state_->tasks_count.release();
  }
}
