/**
 * @file SyncLogsActive.cpp
 * Test in some threads Generics::execute_and_listen,
 * while in other thread do malloc()/free()
 * Code like SyncLogs application
 */

#include <iostream>
#include <vector>

#include <eh/Errno.hpp>
#include <Generics/Rand.hpp>
#include <Generics/Listener.hpp>
#include <Generics/ThreadRunner.hpp>

namespace
{
  const std::size_t THREADS_BUNDLE_SIZE = 100;
  const std::size_t FORKS_IN_THREAD = 20;
  const std::size_t REALLOCATIONS = 1024*1024*10;
  std::string cmd("/usr/bin/printf");

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
}

struct InterruptCallback
{
  virtual ~InterruptCallback() noexcept {}
  virtual bool interrupt() /*throw(eh::Exception)*/ = 0;
};

class InterruptCallbackImpl :
  /*protected*/public InterruptCallback
{
  virtual bool
  interrupt() /*throw(eh::Exception)*/
  {
    return false; //  !active();
  }
};


  class Callback :
    public Generics::ExecuteAndListenCallback,
    public ReferenceCounting::AtomicImpl
  {
  public:
    Callback(std::string* output, std::string& error,
      InterruptCallback* interrupter)
      /*throw(eh::Exception)*/;

    virtual void
    on_data_ready(int fd, std::size_t fd_index, const char* str,
      std::size_t size) noexcept;

    virtual void
    on_periodic() noexcept;

    virtual void
    report_error(Severity severity, const String::SubString& description,
      const char* error_code = 0) noexcept;

    virtual void
    set_pid(pid_t pid) noexcept;

  protected:
    virtual
    ~Callback() noexcept;

  private:
    std::string* output_;
    std::string& error_;
    InterruptCallback* interrupter_;
    pid_t pid_;
  };

  Callback::Callback(std::string* output, std::string& error,
    InterruptCallback* interrupter)
    /*throw(eh::Exception)*/
    : output_(output), error_(error), interrupter_(interrupter)
  {
  }

  Callback::~Callback() noexcept
  {
  }

  void
  Callback::on_data_ready(int /*fd*/, std::size_t fd_index, const char* str,
    std::size_t size) noexcept
  {
    try
    {
      (output_ && !fd_index ? *output_ : error_).append(str, size);
    }
    catch (...)
    {
      report_error(ERROR, String::SubString("Memory allocation error"));
    }
  }

  void
  Callback::on_periodic() noexcept
  {
    if (interrupter_ && interrupter_->interrupt())
    {
      kill(pid_, SIGTERM);
    }
  }

  void
  Callback::report_error(Severity /*severity*/,
    const String::SubString& /*description*/,
    const char* /*error_code*/) noexcept
  {
    // FIXME
  }

  void
  Callback::set_pid(pid_t pid) noexcept
  {
    pid_ = pid;
  }


  /**
   * This thread do fork() as SyncLog
   */
  class SyncLogThread : public Generics::ThreadJob
  {
  public:
    //
    //construct this
    SyncLogThread() noexcept
    {
    }

    //execute this
    virtual void
    work() noexcept
    {
      try
      {
        std::unique_ptr<InterruptCallback> interrupter(
          new InterruptCallbackImpl);

        static const int LISTEN[] = { STDOUT_FILENO };
        static const int REDIRECT[] = { STDIN_FILENO, STDERR_FILENO };

        std::string output_str;
        std::string error;
        Generics::ExecuteAndListenCallback_var callback(
          new Callback(&output_str, error, interrupter.get()));
        std::vector<const char*> arg_ptr_vec;
        arg_ptr_vec.reserve(2);

        arg_ptr_vec.push_back(cmd.c_str());
        arg_ptr_vec.push_back("*");
        arg_ptr_vec.push_back(0);

        for (std::size_t i = 0; i < FORKS_IN_THREAD; ++i)
        {
          int status = 0;
          try
          {
            status = Generics::execute_and_listen(callback, cmd.c_str(),
              const_cast<char**>(&arg_ptr_vec[0]), 1,
              LISTEN, 2, REDIRECT, 1, false, true);
            if (!error.empty())
            {
              Stream::Error ostr;
              ostr << error.c_str() << ", status=" << status
                << ", tid=" << pthread_self();
              throw Exception(ostr);
            }

            if (!WIFEXITED(status))
            {
              Stream::Error ostr;
              ostr << " child finished abnormally, status=" <<
                status << "\nError: " << error <<
                ", current output size: " << output_str.size()
                << ", tid=" << pthread_self();
              eh::throw_errno_exception<Exception>(FNE, ostr.str());
            }
          }
          catch (const eh::Exception& ex)
          {
            Guard lock(cerr_mutex_);
            std::cerr << "Exception: " << ex.what() << std::endl;
          }
          catch (...)
          {
            Guard lock(cerr_mutex_);
            std::cerr << "unknown exception occurred" << std::endl;
          }
        }
        bool success = false;
        if (output_str.size() == FORKS_IN_THREAD)
        {
          std::string::const_iterator cit = output_str.begin();
          for (; *cit == '*'; ++cit);
          success = (cit == output_str.end());
        }
        if (success)
        {
          Guard lock(mutex_);
          ++success_threads_;
        }
      }
      catch (const eh::Exception& ex)
      {
        Guard lock(cerr_mutex_);
        std::cerr << "Exception: " << ex.what() << std::endl;
      }
      catch (...)
      {
        Guard lock(cerr_mutex_);
        std::cerr << "unknown exception occurred" << std::endl;
      }
    }

    static
    std::size_t
    get_successes() noexcept
    {
      return success_threads_;
    }

  protected:
    virtual
    ~SyncLogThread() noexcept
    {
    }

  private:
    typedef Sync::PosixMutex Mutex;
    typedef Sync::PosixGuard Guard;
    /// Counter for threads successes
    static Mutex mutex_;
    static std::size_t success_threads_;
    static Mutex cerr_mutex_;
  };

  SyncLogThread::Mutex SyncLogThread::mutex_;
  std::size_t SyncLogThread::success_threads_ = 0;
  SyncLogThread::Mutex SyncLogThread::cerr_mutex_;

  /**
   * This thread do malloc()/free()
   */
  class MemoryUsageThread : public Generics::ThreadJob
  {
  public:
    //
    //construct this
    MemoryUsageThread() noexcept
    {
    }

    //execute this
    virtual void
    work() noexcept
    {
      try
      {
        for (std::size_t i = 0; i < REALLOCATIONS; ++i)
        {
          std::size_t len = Generics::safe_rand(10240);
          void* ptr = malloc(len);
//          memset(ptr, 0, len);
          free(ptr);
        }
        printf("Memory shaken\n");
      }
      catch (...)
      {
        std::cerr << "unknown ... occurred" << std::endl;
      }
    }

  protected:
    virtual
    ~MemoryUsageThread() noexcept
    {
    }

  private:
  };

int
main() noexcept
{
  try
  {
    std::cout << "SyncLogsActive multi-thread processing test started.."
      << std::endl;

    Generics::ThreadRunner* mem_thread_runner =
      new Generics::ThreadRunner(
        Generics::ThreadJob_var(new SyncLogThread()).in(), THREADS_BUNDLE_SIZE);
    mem_thread_runner->start();

    Generics::ThreadRunner* thread_runner =
      new Generics::ThreadRunner(
        Generics::ThreadJob_var(new MemoryUsageThread()).in(), 1);
    thread_runner->start();

    delete thread_runner;
    std::cout << "All forkable threads finished" << std::endl;
    delete mem_thread_runner;

    if (SyncLogThread::get_successes() == THREADS_BUNDLE_SIZE)
    {
      std::cout << "Performed " << THREADS_BUNDLE_SIZE * FORKS_IN_THREAD
        << " forks\nSUCCESS" << std::endl;
    }
    else
    {
      std::cerr << "Test FAILED, not all fork() task complete" << std::endl
        << "Must pass " << THREADS_BUNDLE_SIZE << " threads, but really "
        << SyncLogThread::get_successes() << " threads finished well"
        << std::endl;
    }
    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception" << std::endl;
  }

  return -1;
}
