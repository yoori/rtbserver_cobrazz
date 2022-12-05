#ifndef UIDGENERATORADAPTERIMPL_HPP_
#define UIDGENERATORADAPTERIMPL_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/Singleton.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>

#include <Commons/ProcessControlVarsImpl.hpp>
#include <Commons/AtomicInt.hpp>

#include <xsd/Frontends/UIDGeneratorAdapterConfig.hpp>

#include "DescriptorHandlerPoller.hpp"
#include "UIDGeneratorAdapterLogger.hpp"

namespace AdServer
{
namespace Frontends
{
  class UIDGeneratorAdapterImpl:
    public Generics::CompositeActiveObject,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef xsd::AdServer::Configuration::UIDGeneratorAdapterConfigType
      Config;

    struct Stats: public ReferenceCounting::AtomicImpl
    {
      Stats()
        : processed_messages_count(0)
      {}

      Algs::AtomicInt processed_messages_count;
    };

    typedef ReferenceCounting::SmartPtr<Stats> Stats_var;

  public:
    UIDGeneratorAdapterImpl(
      Logging::Logger* logger,
      const Config& config)
      /*throw(eh::Exception)*/;

  protected:
    typedef Generics::TaskGoal TaskBase;
    typedef ReferenceCounting::SmartPtr<TaskBase> Task_var;

    class FlushLogsTask: public TaskBase
    {
    public:
      FlushLogsTask(
        Generics::TaskRunner& task_runner,
        UIDGeneratorAdapterImpl* uid_generator_adapter_impl)
        noexcept;

      virtual void
      execute() noexcept;

    protected:
      UIDGeneratorAdapterImpl* uid_generator_adapter_impl_;
    };

    class PrintStatsTask: public TaskBase
    {
    public:
      PrintStatsTask(
        Generics::TaskRunner& task_runner,
        UIDGeneratorAdapterImpl* uid_generator_adapter_impl)
        noexcept;

      virtual void
      execute() noexcept;

    protected:
      UIDGeneratorAdapterImpl* uid_generator_adapter_impl_;
    };

  protected:
    void
    flush_logs_() noexcept;

    void
    print_stats_() noexcept;

  private:
    virtual
    ~UIDGeneratorAdapterImpl() noexcept
    {}

  private:
    Logging::Logger_var logger_;
    Config config_;
    Generics::Planner_var scheduler_;
    Generics::TaskRunner_var task_runner_;

    DescriptorHandlerPoller_var poller_;
    UIDGeneratorAdapterLogger_var stat_logger_;
    Stats_var stats_;

    //Logging::LoggerCallbackHolder logger_callback_holder_;
    //StatHolder_var stats_;
  };

  typedef ReferenceCounting::QualPtr<UIDGeneratorAdapterImpl>
    UIDGeneratorAdapterImpl_var;
}
}

namespace AdServer
{
namespace Frontends
{
  inline
  UIDGeneratorAdapterImpl::FlushLogsTask::FlushLogsTask(
    Generics::TaskRunner& task_runner,
    UIDGeneratorAdapterImpl* uid_generator_adapter_impl)
      noexcept
      : TaskBase(&task_runner),
        uid_generator_adapter_impl_(uid_generator_adapter_impl)
  {}

  inline void
  UIDGeneratorAdapterImpl::FlushLogsTask::execute()
    noexcept
  {
    uid_generator_adapter_impl_->flush_logs_();
  }

  inline
  UIDGeneratorAdapterImpl::PrintStatsTask::PrintStatsTask(
    Generics::TaskRunner& task_runner,
    UIDGeneratorAdapterImpl* uid_generator_adapter_impl)
      noexcept
      : TaskBase(&task_runner),
        uid_generator_adapter_impl_(uid_generator_adapter_impl)
  {}

  inline void
  UIDGeneratorAdapterImpl::PrintStatsTask::execute()
    noexcept
  {
    uid_generator_adapter_impl_->print_stats_();
  }
}
}

#endif /*UIDGENERATORADAPTERIMPL_HPP_*/
