
#ifndef THREAD_HANDLER_TEMPLATE_HPP
#define THREAD_HANDLER_TEMPLATE_HPP

#include <sstream>
#include <vector>
#include <Generics/TaskRunner.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <eh/Errno.hpp>
#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Sync/Semaphore.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelCommons.hpp>

namespace AdServer
{
namespace ChannelSvcs
{
namespace Protected
{

  template<class T>
  class ThreadsHandler:
    public ReferenceCounting::DefaultImpl<Sync::Policy::PosixThread>
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef std::vector<T> ResultsVector;

    ThreadsHandler(size_t tasks) noexcept;
    ~ThreadsHandler() noexcept;
    const std::ostringstream& get_exception() const noexcept;
    ResultsVector& get_result() /*throw(Exception)*/;
    ResultsVector& results() noexcept;
    void set_result(size_t num, T& result) noexcept;
    void set_exception(size_t num, std::ostringstream& descr) noexcept;
    size_t count_exception() const noexcept;

    template<class DESCR>
    void set_exception(
      size_t num,
      const char* func,
      const char* ex,
      const DESCR& descr)
      noexcept;

  private:
    typedef Sync::PosixRWLock Mutex_;
    typedef Sync::PosixRGuard ReadGuard_;
    typedef Sync::PosixWGuard WriteGuard_;

    ResultsVector result_;
    std::ostringstream ex_descr_;
    size_t count_exception_;
    volatile _Atomic_word res_count_;
    mutable Mutex_ lock_exception_;
    mutable Sync::Semaphore wait_end_;
  };

  template<typename RESULT, typename REF>
  struct BaseCaller
  {
    typedef RESULT ResultType;
    typedef ThreadsHandler<RESULT> Handler;
    typedef ReferenceCounting::SmartPtr<Handler> CallBackType;

    BaseCaller(const char* name, size_t len) /*throw(eh::Exception)*/;
    unsigned int id_;
    const char* name_;
    REF* ref_;
    CallBackType callback_;
  };

  template<typename RESULT, typename REF>
  BaseCaller<RESULT, REF>::BaseCaller(const char* name, size_t len)
    /*throw(eh::Exception)*/
    :name_(name)
  {
    callback_ = new Handler(len);
  }

  template<class CALLER>
  class CallTask : public Generics::TaskImpl
  {
    public:
      CallTask(const CALLER& caller) noexcept;
      virtual ~CallTask() noexcept{};
      virtual void execute() noexcept;
    private:
    CALLER caller_;
  };

  template<class T>
  ThreadsHandler<T>::ThreadsHandler(size_t tasks) noexcept
    : result_(tasks, 0),
      count_exception_(0),
      res_count_(0),
      wait_end_(0)
  {
  }

  template<class T>
  ThreadsHandler<T>::~ThreadsHandler() noexcept
  {
  }

  template<class T>
  size_t ThreadsHandler<T>::count_exception() const noexcept
  {
    return count_exception_;
  }

  template<class T>
  const std::ostringstream& ThreadsHandler<T>::get_exception() const noexcept
  {
    return ex_descr_;
  }

  template<class T>
  typename ThreadsHandler<T>::ResultsVector& ThreadsHandler<T>::get_result()
    /*throw(Exception)*/
  {
    try
    {
      wait_end_.acquire();
      return result_;
    }
    catch(const Sync::Semaphore::Exception& e)
    {
      Stream::Error err;
      err << "can't acquire Semaphore: " << e.what();
      throw Exception(err);
    }
  }

  template<class T>
  typename ThreadsHandler<T>::ResultsVector& ThreadsHandler<T>::results()
    noexcept
  {
    return result_;
  }

  template<class T>
  void ThreadsHandler<T>::set_result(size_t num, T& result) noexcept
  {
    try
    {
      result_[num] = result._retn();
      _Atomic_word old = __gnu_cxx::__exchange_and_add(&res_count_, 1);
      if(old + 1 == static_cast<int>(result_.size()))
      {
        wait_end_.release();
      }
    }
    catch(const Sync::Semaphore::Exception& e)
    {
      std::ostringstream ostr;
      ostr << "can't post Semaphore: " << e.what();
      set_exception(num, ostr);
    }
  }

  template<class T>
  void ThreadsHandler<T>::set_exception(size_t num, std::ostringstream& descr)
    noexcept
  {
    {
      WriteGuard_ guard(lock_exception_);
      count_exception_++;
      if(ex_descr_.str().size())
      {
        ex_descr_ << ". ";
      }
      ex_descr_ << descr.str();
    }
    try
    {
      result_[num] = 0;
      __gnu_cxx::__atomic_add(&res_count_, 1);
      if(res_count_ == (int)result_.size())
      {
        wait_end_.release();
      }
    }
    catch(const Sync::Semaphore::Exception& e)
    {
      //can't do anything
      abort();
    }
  }

  template<class T>
  template<class DESCR>
  void ThreadsHandler<T>::set_exception(
    size_t num,
    const char* func,
    const char* ex,
    const DESCR& descr)
    noexcept
  {
    std::ostringstream ostr;
    ostr << func << ": caught " << ex << ". Instance " << num
      << ". : " << descr;
    set_exception(num, ostr);
  }
    
  template<class CALLER>
  CallTask<CALLER>::CallTask(const CALLER& caller) noexcept
    : caller_(caller)
  {
  }

  template<class CALLER>
  void CallTask<CALLER>::execute() noexcept
  {
    try
    {
      caller_.execute();
    }
    catch(const eh::Exception& e)
    {
      caller_.callback_->set_exception(
        caller_.id_,
        caller_.name_,
        "eh::Exception",
        e.what());
    }
    catch(const AdServer::ChannelSvcs::NotConfigured& e)
    {
      caller_.callback_->set_exception(
        caller_.id_,
        caller_.name_,
        "ChannelSvcs::NotConfigured",
        "");
    }
    catch(const AdServer::ChannelSvcs::ImplementationException& e)
    {
      caller_.callback_->set_exception(
        caller_.id_,
        caller_.name_,
        "ChannelSvcs::ImplementationException",
        e.description);
    }
    catch(const CORBA::SystemException& e)
    {
      caller_.callback_->set_exception(
        caller_.id_,
        caller_.name_,
        "CORBA::SystemException",
        e);
    }
  }

}// namespace Protected
}//namespace ChannelSvcs
}// namespace AdServer

#endif //THREAD_HANDLER_TEMPLATE_HPP

