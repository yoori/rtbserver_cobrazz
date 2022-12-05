#ifndef _FRONTENDCOMMONS_TASKSCHDULER_HPP_
#define _FRONTENDCOMMONS_TASKSCHDULER_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>

namespace FrontendCommons
{
  class TaskScheduler:
    public virtual ReferenceCounting::AtomicImpl,
    public virtual Generics::CompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    TaskScheduler(
      Generics::ActiveObjectCallback* active_object_callback,
      Generics::TaskRunner* task_runner = 0)
      /*throw(Exception)*/;

    Generics::TaskRunner*
    task_runner() noexcept;

    Generics::Planner*
    planner() noexcept;

    void
    schedule(Generics::Goal* task, const Generics::Time& time)
      /*throw(Exception)*/;

  private:
    typedef ReferenceCounting::SmartPtr<Generics::Task> Task_var;

    Generics::ActiveObjectCallback_var active_object_callback_;
    Generics::Planner_var scheduler_;
    Generics::TaskRunner_var task_runner_;
  };

  typedef ReferenceCounting::SmartPtr<TaskScheduler> TaskScheduler_var;

}

namespace FrontendCommons
{
  inline
  TaskScheduler::TaskScheduler(
    Generics::ActiveObjectCallback* active_object_callback,
    Generics::TaskRunner* task_runner)
    /*throw(Exception)*/
    : active_object_callback_(ReferenceCounting::add_ref(active_object_callback)),
      scheduler_(new Generics::Planner(active_object_callback_)),
      task_runner_(ReferenceCounting::add_ref(task_runner))
  {
    if(task_runner_.in() == 0)
    {
      try
      {
        task_runner_ = new Generics::TaskRunner(active_object_callback_, 1);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "TaskScheduler::TaskScheduler(): "
          << "Can't create TaskRunner. eh::Exception caught. "
          << ": " << ex.what() << ".";
        throw Exception(ostr);
      }
    }

    try
    {
      add_child_object(task_runner_);
      add_child_object(scheduler_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "TaskScheduler::TaskScheduler(): "
        << "CompositeActiveObject::Exception caught. "
        << ": " << ex.what() << ".";
      throw Exception(ostr);
    }
  }

  inline
  Generics::TaskRunner*
  TaskScheduler::task_runner() noexcept
  {
    return task_runner_;
  }

  inline
  Generics::Planner*
  TaskScheduler::planner() noexcept
  {
    return scheduler_;
  }

  inline
  void
  TaskScheduler::schedule(
    Generics::Goal* task, const Generics::Time& time)
    /*throw(Exception)*/
  {
    try
    {
      scheduler_->schedule(task, time);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "TaskScheduler::schedule(): "
        << "eh::Exception caught. "
        << ": " << ex.what() << ".";
      throw Exception(ostr);
    }
  }
}

#endif /*_FRONTENDCOMMONS_TASKSCHDULER_HPP_*/
