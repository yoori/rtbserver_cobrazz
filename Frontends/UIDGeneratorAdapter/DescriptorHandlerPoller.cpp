#include <fcntl.h>
#include <sys/epoll.h>
#include <vector>
#include <list>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <eh/Errno.hpp>

#include <Commons/CountActiveObject.hpp>
#include <Commons/DelegateActiveObject.hpp>

#include "DescriptorHandlerPoller.hpp"

namespace AdServer
{
  namespace
  {
    const bool TRACE_POLLER_ = false; // true;
    const char TRACE_TIME_FORMAT[] = "%F %T";

    void
    mem_barrier()
    {
      asm volatile ("" :: : "memory");
    }

    const uint32_t EPOLL_FLAGS = EPOLLRDHUP | EPOLLET | EPOLLONESHOT /*| EPOLLEXCLUSIVE*/;
  }

  /* stopping with using StopPipeDescriptorHandler
   * write to pipe stop message
   * StopPipeDescriptorHandler read it - stop loop, decrease threads_to_stop and write message again if threads_to_stop isn't zero
   */
  class DescriptorHandlerPoller::StopPipeDescriptorHandler: public DescriptorHandler
  {
  public:
    StopPipeDescriptorHandler()
      noexcept;

    virtual int
    fd() const noexcept;

    virtual unsigned long
    read() /*throw(Exception)*/;

    virtual unsigned long
    write() /*throw(Exception)*/;

    virtual void
    stopped() noexcept;

    void
    stop() noexcept;

  protected:
    virtual
    ~StopPipeDescriptorHandler() noexcept;

    void
    stop_() noexcept;

  protected:
    volatile sig_atomic_t stopped_;
    int read_fd_;
    int write_fd_;
  };

  // DescriptorHandlerCleaner
  // class that allow to clean collected DescriptorHandler's only when
  // all threads called release
  //
  // For epoll this garantee that handler will be freed only when
  // all epoll_wait calls finished
  //
  class DescriptorHandlerPoller::DescriptorHandlerCleaner: public ReferenceCounting::AtomicImpl
  {
  public:
    DescriptorHandlerCleaner(unsigned long threads)
      noexcept;

    void
    push(DescriptorHandlerHolder* descriptor_handler_holder)
      noexcept;

    void
    release(unsigned long id)
      noexcept;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;
    typedef std::list<DescriptorHandlerHolder_var> ObjList;

  protected:
    virtual
    ~DescriptorHandlerCleaner() noexcept
    {}

  protected:
    std::vector<Algs::AtomicInt> release_array_;
    Algs::AtomicInt unreleased_count_;

    // descriptors that will be freed when current release loop will be finished
    SyncPolicy::Mutex free_lock_;
    ObjList free_;

    // descriptors for that will be started new release after finish current
    SyncPolicy::Mutex free_candidates_lock_;
    ObjList free_candidates_;
  };

  // DescriptorHandlerPoller::DescriptorHandlerHolder impl
  DescriptorHandlerPoller::DescriptorHandlerHolder::DescriptorHandlerHolder(
    DescriptorHandler* descriptor_handler)
    noexcept
    : descriptor_handler(ReferenceCounting::add_ref(descriptor_handler)),
      handle_read(true),
      handle_write(true),
      handle_in_progress(0),
      handling_finished(0),
      destroyed(false)
  {}

  // DescriptorHandlerPoller::StopPipeDescriptorHandler impl
  DescriptorHandlerPoller::StopPipeDescriptorHandler::StopPipeDescriptorHandler()
    noexcept
    : stopped_(0)
  {
    static const char* FUN = "StopPipeDescriptorHandler::StopPipeDescriptorHandler()";

    int fds[2];

    if(::pipe(fds) < 0)
    {
      eh::throw_errno_exception<Exception>(errno, FUN, "pipe() failed");
    }

    read_fd_ = fds[0];
    write_fd_ = fds[1]; // blocked mode

    set_non_blocking(read_fd_);
  }

  DescriptorHandlerPoller::StopPipeDescriptorHandler::~StopPipeDescriptorHandler()
    noexcept
  {
    ::close(read_fd_);
    ::close(write_fd_);
  }

  int
  DescriptorHandlerPoller::StopPipeDescriptorHandler::fd() const
    noexcept
  {
    return read_fd_;
  }

  unsigned long
  DescriptorHandlerPoller::StopPipeDescriptorHandler::read()
    /*throw(Exception)*/
  {
    static const char* FUN = "StopPipeDescriptorHandler::read()";

    if(stopped_)
    {
      return DescriptorHandler::STOP_PROCESSING;
    }

    uint32_t value;
    ssize_t read_res = ::read(read_fd_, &value, 4);

    if(read_res < 0)
    {
      if(errno == EAGAIN || errno == EWOULDBLOCK)
      {
        return DescriptorHandler::STOP_WRITE_HANDLE;
      }
      else
      {
        eh::throw_errno_exception<Exception>(errno, FUN, "read() failed");
      }
    }

    assert(read_res > 0);

    // memory barrier
    mem_barrier();

    // trust that write to pipe for 4 bytes can't be blocked
    stop_();

    // other thread(if exists) will write
    return static_cast<unsigned long>(DescriptorHandler::STOP_PROCESSING) |
      DescriptorHandler::STOP_WRITE_HANDLE;
  }

  unsigned long
  DescriptorHandlerPoller::StopPipeDescriptorHandler::write()
    /*throw(Exception)*/
  {
    return DescriptorHandler::STOP_WRITE_HANDLE;
  }

  void
  DescriptorHandlerPoller::StopPipeDescriptorHandler::stopped()
    noexcept
  {}

  void
  DescriptorHandlerPoller::StopPipeDescriptorHandler::stop()
    noexcept
  {
    // synchroniously write stop message
    stopped_ = 1;

    mem_barrier();

    // write stop message
    stop_();
  }

  void
  DescriptorHandlerPoller::StopPipeDescriptorHandler::stop_()
    noexcept
  {
    static const char* FUN = "StopPipeDescriptorHandler::stop_()";

    // write stop message
    uint32_t value = 0;
    ssize_t write_res = ::write(write_fd_, &value, 4);

    if(write_res < 0)
    {
      eh::throw_errno_exception<Exception>(errno, FUN, "write() failed");
    }
  }

  // DescriptorHandlerPoller::DescriptorHandlerCleaner
  DescriptorHandlerPoller::
  DescriptorHandlerCleaner::DescriptorHandlerCleaner(unsigned long threads)
    noexcept
    : release_array_(threads, Algs::AtomicInt(0)),
      unreleased_count_(threads)
  {}

  void
  DescriptorHandlerPoller::
  DescriptorHandlerCleaner::push(DescriptorHandlerHolder* obj)
    noexcept
  {
    ObjList obj_list;
    obj_list.push_back(ReferenceCounting::add_ref(obj));

    {
      SyncPolicy::WriteGuard lock(free_candidates_lock_);
      free_candidates_.splice(free_candidates_.end(), obj_list);

      if(TRACE_POLLER_)
      {
        std::ostringstream ostr;
        ostr << "CLEAN: add handler, size = " << free_candidates_.size() << std::endl;
        std::cerr << ostr.str() << std::endl;
      }
    }
  }

  void
  DescriptorHandlerPoller::
  DescriptorHandlerCleaner::release(unsigned long id)
    noexcept
  {
    if(release_array_[id] == 0) // optimization : prevent excess atomic call
    {
      if(release_array_[id].exchange_and_add(1) == 0)
      {
        if(unreleased_count_.exchange_and_add(-1) == 1)
        {
          // if release loop finished
          // clear all descriptors and swap candidates with free descriptors

          // this block can be called concurrently
          ObjList clear_list;

          {
            // free_ = free_candidates_
            // free_candidates_ => empty
            SyncPolicy::WriteGuard lock1(free_candidates_lock_);
            SyncPolicy::WriteGuard lock2(free_lock_);

            free_.swap(clear_list);
            free_candidates_.swap(free_);
          }

          // revert release markers
          for(auto it = release_array_.begin(); it != release_array_.end(); ++it)
          {
            if(it->exchange_and_add(-1) == 1)
            {
              unreleased_count_ += 1;
            }
          }

          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            ostr << "CLEAN: free handlers, size = " << clear_list.size() << std::endl;
            std::cerr << ostr.str() << std::endl;
          }

          // destroy clear list (liberated descriptors) in clear_list d-tor
        }
      }
      // revert increase for consistency
      else if(release_array_[id].exchange_and_add(-1) == 1)
      {
        // if some release appeared concurrently with free ignore it
        // unreleased_count_ should be consistent with release_array_
        unreleased_count_ += 1;
      }
    }
  }

  // DescriptorHandlerPoller::Proxy impl
  DescriptorHandlerPoller::Proxy::Proxy(
    DescriptorHandlerPoller* poller)
    : owner_(poller)
  {}

  DescriptorHandlerPoller::Proxy::~Proxy()
    noexcept
  {}

  bool
  DescriptorHandlerPoller::Proxy::add(
    DescriptorHandler* desc_handler)
    /*throw(Exception)*/
  {
    DescriptorHandlerPoller_var owner = lock_owner_();

    if(owner.in())
    {
      owner->add(desc_handler);
      return true;
    }

    return false;
  }

  DescriptorHandlerPoller_var
  DescriptorHandlerPoller::Proxy::lock_owner_() const noexcept
  {
    SyncPolicy::ReadGuard lock(lock_);
    return ReferenceCounting::add_ref(owner_);
  }

  void
  DescriptorHandlerPoller::Proxy::detach_() noexcept
  {
    SyncPolicy::WriteGuard lock(lock_);
    owner_ = 0;
  }

  // DescriptorHandlerPoller impl
  DescriptorHandlerPoller::DescriptorHandlerPoller(
    Generics::ActiveObjectCallback* callback,
    unsigned long threads,
    const Generics::Time& connection_clean_period)
    /*throw(Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      connection_clean_period_(connection_clean_period.microseconds() / 1000)
  {
    proxy_ = new Proxy(this);

    descriptor_handler_cleaner_ = new DescriptorHandlerCleaner(threads);

    epoll_fd_ = ::epoll_create1(0);

    stop_handler_ = new StopPipeDescriptorHandler();

    add(stop_handler_);

    for(unsigned long thread_i = 0; thread_i < threads; ++thread_i)
    {
      Commons::DelegateActiveObject_var worker =
        Commons::make_delegate_active_object(
          std::bind(&DescriptorHandlerPoller::process_, this, thread_i),
          callback,
          1);

      add_child_object(worker);
    }
  }

  DescriptorHandlerPoller::~DescriptorHandlerPoller() noexcept
  {
    proxy_->detach_();

    for(auto it = handlers_.begin(); it != handlers_.end(); ++it)
    {
      epoll_event ev;
      ::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, it->first->fd(), &ev);
    }

    handlers_.clear();

    ::close(epoll_fd_);
  }

  DescriptorHandlerPoller::Proxy_var
  DescriptorHandlerPoller::proxy() const noexcept
  {
    return proxy_;
  }

  void
  DescriptorHandlerPoller::add(DescriptorHandler* desc_handler)
    /*throw(Exception)*/
  {
    static const char* FUN = "DescriptorHandlerPoller::add()";

    bool inserted;

    DescriptorHandlerHolder_var holder = new DescriptorHandlerHolder(
      desc_handler);

    {
      SyncPolicy::WriteGuard lock(handlers_lock_);
      inserted = handlers_.insert(std::make_pair(desc_handler, holder)).second;
    }

    if(inserted)
    {
      epoll_event ev;
      ev.events = EPOLL_FLAGS | EPOLLIN | EPOLLOUT;
      ev.data.ptr = holder;

      if(TRACE_POLLER_)
      {
        std::cerr << "epoll_add_fd_(" << desc_handler->fd() << ")" << std::endl;
      }

      if(::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, desc_handler->fd(), &ev) == -1)
      {
        eh::throw_errno_exception<Exception>(errno, FUN, "epoll_ctl() failed");
      }
    }
  }

  void
  DescriptorHandlerPoller::deactivate_object()
    /*throw(Generics::ActiveObject::Exception, eh::Exception)*/
  {
    stop_handler_->stop();

    Generics::CompositeActiveObject::deactivate_object();
  }

  void
  DescriptorHandlerPoller::wait_object()
    /*throw(Generics::ActiveObject::Exception, eh::Exception)*/
  {
    // wait when all threads will be finished
    Generics::CompositeActiveObject::wait_object();
  }

  void
  DescriptorHandlerPoller::set_non_blocking(int fd)
    /*throw(Exception)*/
  {
    static const char* FUN = "DescriptorHandlerPoller::set_non_blocking()";

    int flags = fcntl(fd, F_GETFL, 0);

    if(flags < 0)
    {
      eh::throw_errno_exception<Exception>(errno, FUN, "fcntl() failed");
    }

    if(::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
      eh::throw_errno_exception<Exception>(
        errno,
        FUN,
        "fcntl() for enable non blocking failed");
    }
  }

  bool
  DescriptorHandlerPoller::handle_(
    unsigned long thread_i,
    DescriptorHandlerHolder* descriptor_handler_holder,
    uint32_t events)
  {
    //static const char* FUN = "DescriptorHandlerPoller::handle_()";

    if(TRACE_POLLER_)
    {
      std::ostringstream ostr;
      Generics::ExtendedTime now = Generics::Time::get_time_of_day().get_gm_time();
      ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
        "> #" << thread_i <<
        ": process fd(" << descriptor_handler_holder->descriptor_handler->fd() << "), events = " << events <<
        " =>" << (events & EPOLLIN ? " EPOLLIN" : "") <<
        (events & EPOLLOUT ? " EPOLLOUT" : "") <<
        (events & EPOLLRDHUP ? " EPOLLRDHUP" : "") <<
        (events & EPOLLERR ? " EPOLLERR" : "") <<
        std::endl;
      std::cerr << ostr.str() << std::endl;
    }

    bool stop_processing = false;

    // lock handling - only one thread can call handlers
    // cleaner garantee that object live
    //
    int prev_handle_in_progress =
      descriptor_handler_holder->handle_in_progress.exchange_and_add(1);

    if(descriptor_handler_holder->handling_finished == 0 &&
      prev_handle_in_progress == 0
      // delegate handling to thread that already do this
      )
    {
      if(TRACE_POLLER_)
      {
        std::ostringstream ostr;
        Generics::ExtendedTime now = Generics::Time::get_time_of_day().get_gm_time();
        ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
          "> #" << thread_i <<
          ": real process fd(" << descriptor_handler_holder->descriptor_handler->fd() << "): " <<
          events << "(in = " <<
          descriptor_handler_holder->handle_read << ", out = " <<
          descriptor_handler_holder->handle_write << ")" << std::endl;
        std::cerr << ostr.str() << std::endl;
      }

      DescriptorHandler* descriptor_handler =
        descriptor_handler_holder->descriptor_handler;

      assert(descriptor_handler);

      do
      {
        assert(prev_handle_in_progress >= 0);

        // signal epoll continue handling
        // rearm with state flags, before handle
        epoll_rearm_fd_(thread_i, descriptor_handler_holder);

        // call all handlers - it should detect
        // all event types (EPOLLIN, EPOLLOUT, EPOLLRDHUP, EPOLLERR)
        unsigned long state_modify_on_read = 0;
        unsigned long state_modify_on_write = 0;

        if(descriptor_handler_holder->handle_read)
        {
          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            Generics::ExtendedTime now = Generics::Time::get_time_of_day().get_gm_time();
            ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
              "> #" << thread_i <<
              ": to handle read" <<
              std::endl;
            std::cerr << ostr.str() << std::endl;
          }

          state_modify_on_read = descriptor_handler->read();
          stop_processing |= apply_state_modify_(
            thread_i,
            descriptor_handler_holder,
            state_modify_on_read);

          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            Generics::ExtendedTime now = Generics::Time::get_time_of_day().get_gm_time();
            ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
              "> #" << thread_i <<
              ": from handle read (in = " <<
              descriptor_handler_holder->handle_read << ", out = " <<
              descriptor_handler_holder->handle_write << ", sw = " <<
              (state_modify_on_read & DescriptorHandler::START_READ_HANDLE) << ")" <<
              std::endl;
            std::cerr << ostr.str() << std::endl;
          }
        }

        if(descriptor_handler_holder->handle_write)
        {
          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            Generics::ExtendedTime now = Generics::Time::get_time_of_day().get_gm_time();
            ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
              "> #" << thread_i <<
              ": to handle write" <<
              std::endl;
            std::cerr << ostr.str() << std::endl;
          }

          state_modify_on_write = descriptor_handler->write();
          stop_processing |= apply_state_modify_(
            thread_i,
            descriptor_handler_holder,
            state_modify_on_write);

          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            Generics::ExtendedTime now = Generics::Time::get_time_of_day().get_gm_time();
            ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
              "> #" << thread_i <<
              ": from handle write (in = " <<
              descriptor_handler_holder->handle_read << ", out = " <<
              descriptor_handler_holder->handle_write << ")" <<
              std::endl;
            std::cerr << ostr.str() << std::endl;
          }
        }

        if(!descriptor_handler_holder->handle_read &&
          !descriptor_handler_holder->handle_write)
        {
          if(TRACE_POLLER_)
          {
            std::ostringstream ostr;
            Generics::ExtendedTime now = Generics::Time::get_time_of_day().get_gm_time();
            ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
              "> #" << thread_i <<
              ": stop handle(" << descriptor_handler->fd() << "): " <<
              "state_modify_on_read = " << state_modify_on_read <<
              ", state_modify_on_write = " << state_modify_on_write <<
              std::endl;
            std::cerr << ostr.str() << std::endl;
          }

          epoll_del_fd_(descriptor_handler->fd());

          descriptor_handler->stopped();

          descriptor_handler_holder->handling_finished = 1;

          // minimize size of descriptor_handler_holder for keep in cleaner
          descriptor_handler_holder->descriptor_handler = DescriptorHandler_var();

          // add to clean list
          descriptor_handler_cleaner_->push(descriptor_handler_holder);

          SyncPolicy::WriteGuard lock(handlers_lock_);
          handlers_.erase(descriptor_handler);
          break; // ignore other thread handles on stop, because handle_in_progress isn't 0
        }
      }
      // process handles appeared in other threads
      while((prev_handle_in_progress =
        descriptor_handler_holder->handle_in_progress.exchange_and_add(-1)) > 1);
    }

    assert(prev_handle_in_progress >= 0);

    // release current thread for cleaner
    descriptor_handler_cleaner_->release(thread_i);

    return stop_processing;
  }

  void
  DescriptorHandlerPoller::process_(unsigned long thread_i)
    noexcept
  {
    static const char* FUN = "DescriptorHandlerPoller::process_()";

    const int MAX_EVENTS = 10;

    epoll_event events[MAX_EVENTS];

    try
    {
      while(true)
      {
        int n = ::epoll_wait(epoll_fd_, events, MAX_EVENTS, connection_clean_period_);

        if(n < 0)
        {
          eh::throw_errno_exception<Exception>(errno, FUN, "epoll_wait() failed");
        }

        bool stop = false;

        for(int i = 0; i < n; ++i)
        {
          // handle
          DescriptorHandlerHolder* descriptor_handler_holder =
            static_cast<DescriptorHandlerHolder*>(events[i].data.ptr);

          stop |= handle_(thread_i, descriptor_handler_holder, events[i].events);
        }

        if(stop)
        {
          return;
        }

        // make release only after processing for decrease processing latency
        // release can call much handlers destroying
        descriptor_handler_cleaner_->release(thread_i);
      }
    }
    catch(const eh::Exception& ex)
    {
      //std::cerr << ex.what() << std::endl;

      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      callback_->critical(String::SubString(ostr.str()));
    }
  }

  void
  DescriptorHandlerPoller::epoll_del_fd_(int fd)
    /*throw(Exception)*/
  {
    static const char* FUN = "DescriptorHandlerPoller::epoll_del_fd_()";

    if(TRACE_POLLER_)
    {
      std::ostringstream ostr;
      Generics::ExtendedTime now = Generics::Time::get_time_of_day().get_gm_time();
      ostr << now.format(TRACE_TIME_FORMAT) << "." << std::setfill('0') << std::setw(3) << (now.tm_usec / 1000) <<
        "> epoll_del_fd_(" << fd << ")" << std::endl;
      std::cerr << ostr.str() << std::endl;
    }

    epoll_event ev;

    if(::epoll_ctl(
         epoll_fd_,
         EPOLL_CTL_DEL,
         fd,
         &ev) == -1)
    {
      eh::throw_errno_exception<Exception>(errno, FUN, "epoll_ctl(EPOLL_CTL_DEL) failed");
    }
  }

  void
  DescriptorHandlerPoller::epoll_rearm_fd_(
    unsigned long thread_i,
    DescriptorHandlerHolder* descriptor_handler_holder)
    const /*throw(Exception)*/
  {
    static const char* FUN = "DescriptorHandlerPoller::epoll_rearm_fd_()";

    if(TRACE_POLLER_)
    {
      std::ostringstream ostr;
      ostr << Generics::Time::get_time_of_day().get_gm_time().format(TRACE_TIME_FORMAT) << "> #" << thread_i <<
        ": rearm fd(" <<
        descriptor_handler_holder->descriptor_handler->fd() << ")"
        ", handle_read = " << descriptor_handler_holder->handle_read <<
        ", handle_write = " << descriptor_handler_holder->handle_write <<
        std::endl;
      std::cerr << ostr.str() << std::endl;
    }

    epoll_event ev;
    ev.events = EPOLL_FLAGS |
      (descriptor_handler_holder->handle_read ? EPOLLIN : static_cast<uint32_t>(0)) |
      (descriptor_handler_holder->handle_write ? EPOLLOUT : static_cast<uint32_t>(0));
    ev.data.ptr = descriptor_handler_holder;

    if(::epoll_ctl(
         epoll_fd_,
         EPOLL_CTL_MOD,
         descriptor_handler_holder->descriptor_handler->fd(),
         &ev) == -1)
    {
      eh::throw_errno_exception<Exception>(errno, FUN, "epoll_ctl() failed");
    }
  }

  bool
  DescriptorHandlerPoller::apply_state_modify_(
    unsigned long thread_i,
    DescriptorHandlerHolder* descriptor_handler_holder,
    unsigned long state_modify)
    const noexcept
  {
    bool rearm = false;
    bool res_handle_read = descriptor_handler_holder->handle_read;
    bool res_handle_write = descriptor_handler_holder->handle_write;

    if(state_modify & DescriptorHandler::START_READ_HANDLE)
    {
      res_handle_read = true;
    }
    else if(state_modify & DescriptorHandler::STOP_READ_HANDLE)
    {
      res_handle_read = false;
    }

    if(state_modify & DescriptorHandler::START_WRITE_HANDLE)
    {
      res_handle_write = true;
    }
    else if(state_modify & DescriptorHandler::STOP_WRITE_HANDLE)
    {
      res_handle_write = false;
    }

    // rearm if processing stopped (other threads can use rearm)
    rearm = (res_handle_write != descriptor_handler_holder->handle_write) ||
      (res_handle_read != descriptor_handler_holder->handle_read);

    descriptor_handler_holder->handle_read = res_handle_read;
    descriptor_handler_holder->handle_write = res_handle_write;

    if(rearm)
    {
      epoll_rearm_fd_(thread_i, descriptor_handler_holder);
    }

    return state_modify & DescriptorHandler::STOP_PROCESSING;
  }
}

