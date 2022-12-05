/**
 * @file ThreadLogSaverImpl.hpp
 */
#ifndef THREAD_LOG_SAVER_IMPL_HPP
#define THREAD_LOG_SAVER_IMPL_HPP

//#define BUILD_WITH_DEBUG_MESSAGES
#include <Generics/Trace.hpp>
#include <Generics/TaskRunner.hpp>
#include <LogCommons/CollectorBundleTypeDefs.hpp>
#include <LogCommons/LogSaverBaseImpl.hpp>
#include "LogSaveProcessor.hpp"
#include "ProcessingContexts.hpp"


namespace AdServer {
namespace LogProcessing {

namespace
{
  const char ASPECT[] = "LogGeneralizer";
  const Generics::Time DUMP_TIMEOUT(3600);

#ifdef BUILD_WITH_DEBUG_MESSAGES
  template <typename Traits,
    typename Param>
  inline
  void
  trace_msg_fun(const Traits&,
    const Param& p)
     noexcept
  {
    if (!strcmp(Traits::log_base_name(), "ColoUpdateStat"))
    {
      try
      {
        trace_message(Traits::log_base_name(), p.str());
      }
      catch (...)
      {}
    }
  }
#define trace_msg(a, b) {Stream::Error ostr; \
  ostr << b << ", line:" << __LINE__; trace_msg_fun(a, ostr);}

#else
#define trace_msg(a, b)
#endif

}
/**
 * @tparam LogExtTraits We needs full traits due snmp_friendly_name() call.
 * @tparam LogSaverType Needs to specify different types of saver in LogExtTraits
 *   (choose alternate saver type) and also indicate exactly type in possible
 *    basic types of LogExtTraits
 */

template <typename LogExtTraits, typename LogSaverType>
class ThreadLogSaverImpl :
  public ThreadLogSaver<typename LogSaverType::CollectorBundleType>
{
  typedef typename LogSaverType::CollectorBundleType CollectorBundleType;
  typedef ThreadLogSaver<CollectorBundleType> BaseType;

  typedef typename BaseType::Spillover_var Spillover_var;
  typedef typename LogSaveProcessor<LogExtTraits>::LogSaverBase_var
    LogSaverBase_var;
  typedef typename LogSaveProcessor<LogExtTraits>::SmartPtr
    LogSaveProcessor_var;

  typedef Generics::Goal MsgBase;
  typedef ReferenceCounting::SmartPtr<MsgBase> Msg_var;

  typedef Generics::GoalTask BaseTask;
  typedef ReferenceCounting::SmartPtr<BaseTask> PlannerTask_var;

  class DbDumpTimeoutMsg:
    public MsgBase,
    public ReferenceCounting::AtomicImpl
  {
  public:
    DbDumpTimeoutMsg(
      const Generics::Time& start_time,
      const std::string& log_base_name,
      Logging::Logger* logger)
      noexcept;

    virtual void
    deliver() noexcept;

  private:
    const Generics::Time START_TIME_;
    const std::string& log_base_name_;
    Logging::Logger_var logger_;
  };

  class DbDumpTimeoutGuard
  {
  public:
    DbDumpTimeoutGuard(
      const LogProcThreadInfo_var& context,
      Generics::Timer& dump_timer,
      const std::string& log_base_name)
      noexcept
      : context_(context),
        dump_timer_(dump_timer)
    {
      dump_timer_.start();
      if (Configuration::db_dump_timeout)
      {
        try
        {
          timeout_msg_ = new DbDumpTimeoutMsg(
            dump_timer.start_time(), log_base_name, context_->logger);
          context_->scheduler->schedule(timeout_msg_,
            Generics::Time::get_time_of_day() +
              Configuration::db_dump_timeout);
        }
        catch (const eh::Exception& ex)
        {
          Stream::Error es;
          es << FNS << ": an eh::Exception has been caught "
            "while scheduling timeout message: " << ex.what();
          context->callback->critical(es.str());
        }
      }
    }

    ~DbDumpTimeoutGuard() noexcept
    {
      if (Configuration::db_dump_timeout)
      {
        try
        {
          context_->scheduler->unschedule(timeout_msg_);
        }
        catch (const eh::Exception& ex)
        {
          Stream::Error es;
          es << FNS << ": an eh::Exception has been caught "
            "while unscheduling timeout message: " << ex.what();
          context_->callback->critical(es.str());
        }
      }
      dump_timer_.stop();
    }
  private:
    LogProcThreadInfo_var context_;
    Generics::Timer& dump_timer_;
    Msg_var timeout_msg_;
  };

  class DumpLogsTask : public BaseTask
  {
  public:
    DumpLogsTask(
      const Spillover_var& data,
      const LogProcThreadInfo_var& context,
      const LogSaveProcessor_var& save_processor,
      CollectorBundleType* spillovers_owner = 0)
      noexcept
    : BaseTask(context->scheduler, context->task_runner),
      data_(data),
      context_(context),
      save_processor_(save_processor),
      spillovers_owner_(spillovers_owner)
    {}

    virtual void
    execute() noexcept
    {
      const bool PERIODIC = spillovers_owner_ != 0;

      bool db_non_active = execute_dump_();

      Generics::Time last_upload_time;
      {
        LogProcThreadInfo::GuardT guard(context_->upload_event_mutex);
        last_upload_time = context_->last_upload_time;
        ++context_->available_dump_task_count;
        context_->upload_event.broadcast();
        trace_msg(LogExtTraits(), (PERIODIC ? "P" : "U") <<
          "task release slot, tasks slots=" << context_->available_dump_task_count);
      }
      if (PERIODIC || db_non_active)
      {
        try
        {
          schedule(last_upload_time + context_->MAX_TIME);
        }
        catch (const eh::Exception &ex)
        {
          Stream::Error es;
          es << FNT << ": an eh::Exception has been caught while "
            "rescheduling the task: " << ex.what();
          context_->callback->critical(es.str());
        }
      }
    }

  private:
    /**
      @return true If DB dump failed due not active DB mode. False is otherwise.
     */
    bool
    execute_dump_() noexcept
    {
      const bool PERIODIC = spillovers_owner_ != 0;
      try
      {
        if (PERIODIC)
        {
          LogProcThreadInfo::ConditionalGuardT
            guard(context_->upload_event, context_->upload_event_mutex);
          while (!context_->available_dump_task_count || !context_->upload_enabled)
          {
            if (!context_->task_runner->active()) // LG is stopping
            {
              return false;
            }
            guard.wait();
          }
          context_->last_upload_time = Generics::Time::get_time_of_day();
          --context_->available_dump_task_count;
          // take away data from bundle, if we haven't got delayed data
          if (!data_->collector_size)
          {
            spillovers_owner_->take_data(data_);
          }
          trace_msg(LogExtTraits(),
            "Ptask got data, size=" << data_->collector_size
            << ", occupy slot, tasks slots="
            << context_->available_dump_task_count);
        }
        if (data_->collector_size)
        {
          Generics::Timer dump_timer;
          {
            DbDumpTimeoutGuard dumper(context_, dump_timer, context_->LOG_TYPE);
            trace_msg(LogExtTraits(),
              (PERIODIC ? "periodic" : "urgent") << " dump, size=" <<
              data_->collector_size << ", tasks slots=" <<
              context_->available_dump_task_count);
            save_processor_->dump(dump_timer, data_);
          }

          if (dump_timer.elapsed_time() > DUMP_TIMEOUT)
          {
            Stream::Error es;
            es << FNT << ": " << context_->LOG_TYPE
              << ": Too long dump operation: start time = "
              << dump_timer.start_time().get_gm_time() <<
              ", finish time = " << dump_timer.stop_time().get_gm_time();
            context_->logger->warning(es.str());
          }
        }
      }
      catch (const DbConnectionFactory::NotActive&)
      {
        return true;
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ << ": an eh::Exception has been caught "
          "while dumping logs. : " << ex.what();
        context_->logger->error(es.str(), ASPECT, LOG_GEN_IMPL_ERR_CODE_3);
      }
      return false;
    }

    Spillover_var data_;
    LogProcThreadInfo_var context_;
    LogSaveProcessor_var save_processor_;
    // Task is periodic if spillovers_owner != 0
    CollectorBundleType* spillovers_owner_;
  };

public:
  typedef ReferenceCounting::SmartPtr<
    ThreadLogSaverImpl<LogExtTraits, LogSaverType> > SmartPtr;

  ThreadLogSaverImpl(
    const LogProcThreadInfo_var& context,
    const LogSaverBase_var& saver) noexcept :
    context_(context),
    save_processor_(new LogSaveProcessor<LogExtTraits>(
      saver, context->proc_stat_impl, context->IN_DIR,
      context->BACKUP_MODE, context->logger))
  {}

  virtual void
  dump_start(
    const Spillover_var& data,
    CollectorBundleType* spillovers_owner = 0) noexcept
  {
    try
    {
      PlannerTask_var task(
        new DumpLogsTask(data, context_, save_processor_, spillovers_owner));
      task->deliver();
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error es;
      es << FNT << ": an eh::Exception has been caught while "
        "scheduling a periodic dump task: " << ex.what();
      context_->callback->critical(es.str());
    }
  }

  virtual void
  dump_portion(const Spillover_var& data) noexcept
  {
    {
      LogProcThreadInfo::ConditionalGuardT
        guard(context_->upload_event, context_->upload_event_mutex);
      while (!context_->available_dump_task_count || !context_->upload_enabled)
      {
        if (!context_->task_runner->active())
        {
          // data stay unprocessed, but LG is stopping now
          return;
        }
        guard.wait();
      }
      context_->last_upload_time = Generics::Time::get_time_of_day();
      --context_->available_dump_task_count;
      trace_msg(LogExtTraits(),
        "Urgent task with data size=" << data->collector_size
          << " will be push into runner, tasks slots="
          << context_->available_dump_task_count);
    }

    dump_start(data);
  }

private:
  virtual
  ~ThreadLogSaverImpl() noexcept
  {}

  LogProcThreadInfo_var context_;
  LogSaveProcessor_var save_processor_;
};


template <typename LogExtTraits, typename LogSaverType>
inline
ThreadLogSaverImpl<LogExtTraits, LogSaverType>::
  DbDumpTimeoutMsg::DbDumpTimeoutMsg(
  const Generics::Time& start_time,
  const std::string& log_base_name,
  Logging::Logger* logger)
  noexcept
  : START_TIME_(start_time),
    log_base_name_(log_base_name),
    logger_(ReferenceCounting::add_ref(logger))
{}

template <typename LogExtTraits, typename LogSaverType>
void
ThreadLogSaverImpl<LogExtTraits, LogSaverType>::DbDumpTimeoutMsg::deliver()
  noexcept
{
  unsigned long secs =
   (Generics::Time::get_time_of_day() - START_TIME_).tv_sec;

  Stream::Error es;
  es << log_base_name_ << ": DB dump timeout: At least " << secs
    << " second(s) have passed since the beginning of the dump operation";
  logger_->warning(es.str(), ASPECT);
}

} // namespace LogProcessing
} // namespace AdServer

#endif // THREAD_LOG_SAVER_IMPL_HPP
