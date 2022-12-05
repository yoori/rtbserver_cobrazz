#ifndef LOGPROCESSING_SYNCLOGS_FEEDROUTEGROUPPROCESSOR_HPP_
#define LOGPROCESSING_SYNCLOGS_FEEDROUTEGROUPPROCESSOR_HPP_

#include <queue>

#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>

#include "Commons/DelegateActiveObject.hpp"
#include "LogCommons/FileReceiverFacade.hpp"

#include "FeedRouteMover.hpp"
#include "FileRouter.hpp"
#include "RouteHelpers.hpp"
#include "Utils.hpp"

namespace AdServer
{
namespace LogProcessing
{
  namespace SyncLog
  {
    typedef unsigned int LogType;
    typedef LogProcessing::FileReceiverFacade<
      LogProcessing::DefaultOrderStrategy<LogType> > FileReceiverFacade;
    typedef ReferenceCounting::AssertPtr<FileReceiverFacade>::Ptr
      FileReceiverFacade_var;
  }

  class Route
  {
  public:
    enum CommandType
    {
      CT_GENERIC,
      CT_RSYNC
    };

    typedef FeedRouteMover::Exception Exception;

  public:
    FeedRouteMover_var mover;
    FeedRouteMover_var commit_mover;
    std::string src_dir;
    std::string intermediate_dir;
    std::string file_mask;
    FileReceiver_var file_receiver;
    std::string commit_src_dir;
    std::string commit_file_mask;
    bool commit_mode;

  public:
    Route(
      Utils::ErrorPool* error_logger,
      const LocalInterfaceChecker& host_checker,
      const char* src_files_pattern,
      RouteBasicHelper* destination_host_router,
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
      bool commit_mode_flag,
      Generics::ActiveObject* interrupter)
    /*throw(Exception)*/;
  };

  class FeedRouteGroupProcessor:
    public Commons::DelegateActiveObject
  {
  public:
    FeedRouteGroupProcessor(
      const std::vector<Route>& routes,
      SyncLog::FileReceiverFacade* file_receiver_facade,
      Utils::ErrorPool* error_logger,
      Generics::TaskRunner* task_runner,
      Generics::Planner* scheduler,
      Generics::ActiveObjectCallback* callback,
      unsigned threads_number,
      unsigned long timeout)
      /*throw(eh::Exception)*/;

    void
    try_unlink_files() noexcept;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;
    typedef SyncLog::FileReceiverFacade::FileEntity FileEntity;

    struct PendingTask
    {
      FileEntity file;
      Generics::Time time;
      bool add_commit_suffix_flag;

      PendingTask(
        const FileEntity& f,
        const Generics::Time& t)
        : file(f), time(t)
      {}
    };

    typedef std::priority_queue<PendingTask> PendingTasks;
    typedef std::list<FileReceiver::FileGuard_var> UnlinkedFiles;

  protected:
    virtual
    ~FeedRouteGroupProcessor() noexcept
    {}

    virtual void
    work_() noexcept;

    void
    process_file_(const FileEntity& file) noexcept;

    void
    add_pending_task_(
      const FileEntity& file,
      const unsigned long timeout)
      /*throw(eh::Exception)*/;

    void
    fetch_commit_files_(SyncLog::LogType log_type) noexcept;

    void
    try_unlink_files_() noexcept;

    void
    fetch_files_(SyncLog::LogType log_type) noexcept;

    friend bool
    operator< (
      const PendingTask& arg1,
      const PendingTask& arg2)
      noexcept
    {
      return arg1.time < arg2.time;
    }

  protected:
    std::vector<Route> routes_;
    SyncLog::FileReceiverFacade_var file_receiver_facade_;
    Utils::ErrorPool* error_logger_;
    Generics::TaskRunner_var task_runner_;
    Generics::Planner_var scheduler_;

    SyncPolicy::Mutex pending_tasks_lock_;
    PendingTasks pending_tasks_;
    unsigned long timeout_;

    SyncPolicy::Mutex unlink_files_lock_;
    UnlinkedFiles unlink_files_;
  };

  typedef ReferenceCounting::AssertPtr<FeedRouteGroupProcessor>::Ptr
    FeedRouteGroupProcessor_var;
}
}

#endif /* LOGPROCESSING_SYNCLOGS_FEEDROUTEGROUPPROCESSOR_HPP_ */
