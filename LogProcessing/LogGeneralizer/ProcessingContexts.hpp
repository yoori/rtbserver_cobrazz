#ifndef LOG_PROCESSING_CONTEXT_HPP
#define LOG_PROCESSING_CONTEXT_HPP

#include <ctime>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>
//#include <ReferenceCounting/Vector.hpp>
#include <vector>
#include <Generics/Time.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Logger/Logger.hpp>
#include <Sync/PosixLock.hpp>
#include <Sync/Condition.hpp>

#include <xsd/LogProcessing/LogGeneralizerConfig.hpp>
#include "ProcStatImpl.hpp"
#include "LogProcessorDecl.hpp"

namespace AdServer
{
namespace LogProcessing
{

  struct Configuration
  {
    static time_t db_dump_timeout;
  };

  struct LogProcThreadInfo: public ReferenceCounting::AtomicImpl
  {
    typedef Sync::PosixMutex MutexT;
    typedef Sync::PosixGuard GuardT;
    typedef Sync::Conditional ConditionalT;
    typedef Sync::ConditionalGuard ConditionalGuardT;

    template <typename LogProcessingParams>
    LogProcThreadInfo(
      const xsd::cxx::tree::optional<LogProcessingParams>& log_proc_params,
      const std::string& in_dir,
      const char* log_base_name,
      const Logging::Logger_var& log,
      Generics::TaskRunner_var& tasker,
      Generics::Planner_var& planner,
      Generics::ActiveObjectCallback_var& cb,
      const ProcStatImpl_var& stat_values)
      noexcept
      : CHECK_LOGS_PERIOD(log_proc_params.get().check_logs_period()),
        MAX_TIME(log_proc_params.get().max_time()),
        MAX_DUMP_TASK_COUNT(log_proc_params.get().max_upload_task_count()),
        LOG_TYPE(log_base_name),
        IN_DIR(in_dir),
        BACKUP_MODE(log_proc_params.get().backup_mode()),
        DISTRIB_COUNT(log_proc_params.get().distrib_count()),
        task_runner(tasker),
        scheduler(planner),
        available_dump_task_count(MAX_DUMP_TASK_COUNT),
        upload_enabled(true),
        logger(log),
        callback(cb),
        proc_stat_impl(stat_values)
    {}

    const Generics::Time CHECK_LOGS_PERIOD;
    const Generics::Time MAX_TIME;
    const unsigned long MAX_DUMP_TASK_COUNT;
    const std::string LOG_TYPE;
    const std::string IN_DIR;
    const bool BACKUP_MODE;
    const unsigned long DISTRIB_COUNT;

    mutable Generics::TaskRunner_var task_runner;
    mutable Generics::Planner_var scheduler;
    MutexT upload_event_mutex;
    ConditionalT upload_event;
    Generics::Time last_upload_time;
    unsigned long available_dump_task_count;
    bool upload_enabled;
    LogProcessor_var log_processor;
    Logging::Logger_var logger;
    Generics::ActiveObjectCallback_var callback;
    ProcStatImpl_var proc_stat_impl;

  private:
    virtual
    ~LogProcThreadInfo() noexcept
    {}
  };

  typedef ReferenceCounting::SmartPtr<LogProcThreadInfo>
    LogProcThreadInfo_var;

  /**
   * Contexts creation is not thread-safe. All contexts should be created before
   * multi-threading usage.
   */
  class ProcessingContexts
  {
  public:
    template <typename LogTraits>
    static
    const LogProcThreadInfo_var&
    get() /*throw(eh::Exception)*/
    {
      std::size_t index = TypeIndex::get<LogTraits>();
      return index < contexts_.size() ? contexts_[index] : null_;
    }

    template <typename LogTraits, typename ConfigParam>
    static
    LogProcThreadInfo_var
    create(const ConfigParam& log_proc_params,
      const std::string& in_dir,
      const Logging::Logger_var& logger,
      Generics::TaskRunner_var& tasker,
      Generics::Planner_var& planner,
      Generics::ActiveObjectCallback_var& cb,
      const ProcStatImpl_var& stat_values)
      /*throw(eh::Exception)*/
    {
      std::size_t index = TypeIndex::get<LogTraits>();
      if (index == contexts_.size())
      {
        contexts_.push_back(
          new LogProcThreadInfo(
            log_proc_params, in_dir, LogTraits::log_base_name(),
            logger, tasker, planner, cb, stat_values));
        return contexts_.back();
      }
      return contexts_[index];
    }

    static void
    deactivation() noexcept
    {
      // awake possible waiters for free task slot (forever captured by deactivated tasks)
      for (Contexts::iterator it = contexts_.begin();
        it != contexts_.end(); ++it)
      {
        LogProcThreadInfo::GuardT guard((*it)->upload_event_mutex);
        (*it)->upload_event.broadcast();
      }
    }

    static void
    clear() noexcept
    {
      // resolve cyclic references
      for (Contexts::iterator it = contexts_.begin();
        it != contexts_.end(); ++it)
      {
        (*it)->log_processor.reset();
      }
      contexts_.clear();
    }
  private:
    class TypeIndex
    {
    public:
      template <typename T>
      static std::size_t
      get() noexcept
      {
        static std::size_t index = counter_();
        return index;
      }
    private:
      static std::size_t
      counter_() noexcept
      {
        static volatile _Atomic_word index_counter = 0;
        _Atomic_word old = __gnu_cxx::__exchange_and_add(&index_counter, 1);
        return old;
      }
    };

//    typedef ReferenceCounting::Vector<LogProcThreadInfo_var> Contexts;
    typedef std::vector<LogProcThreadInfo_var> Contexts;
    static Contexts contexts_;
    static LogProcThreadInfo_var null_;
  };
}
}

#endif // LOG_PROCESSING_CONTEXT_HPP
