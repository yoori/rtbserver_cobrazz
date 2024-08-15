#include <boost/bind/bind.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>

#include <eh/Errno.hpp>

#include <deque>

#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/FrontendCommons/FrontendsPool.hpp>

#include "Acceptor.hpp"

namespace AdServer
{
namespace Frontends
{
  namespace Aspect
  {
    const char WORKER[] = "FCGI::Worker";
  }

  // State
  struct Acceptor::State: public ReferenceCounting::AtomicImpl
  {
    State(
      Logging::Logger* logger,
      FrontendCommons::FrontendInterface* frontend,
      WorkerStatsObject* workers_stats);

    void
    wakeup_worker(int socket) noexcept;

    void
    push_to_free_workers(Worker* worker) noexcept;

    void
    stop_and_wait() noexcept;

  protected:
    typedef Sync::Policy::PosixThread WorkersSyncPolicy;

  protected:
    virtual ~State() noexcept
    {};

  private:
    Logging::Logger_var logger_;
    FrontendCommons::Frontend_var frontend_;
    WorkerStatsObject_var worker_stats_;

    WorkersSyncPolicy::Mutex free_workers_lock_;
    WorkerArray free_workers_;

    WorkersSyncPolicy::Mutex workers_lock_;
    WorkerArray workers_;    

    Sync::PosixMutex stop_lock_;
    Sync::Conditional stopped_;
    bool stop_in_progress_;
  };

  // Worker
  class Acceptor::Worker: public ReferenceCounting::AtomicImpl
  {
  public:
    Worker(
      Logging::Logger* logger,
      FrontendCommons::FrontendInterface* frontend,
      State* state)
      noexcept;

    void
    attach_socket(int sock) noexcept;

    void
    stop();

    void
    wait();

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;
    typedef Sync::PosixGuard Guard;

  protected:
    virtual
    ~Worker() noexcept;

    static void*
    thread_proc(void* ctx);

    void
    work_() noexcept;

    void
    socket_loop_() noexcept;

    Logging::Logger*
    logger() noexcept;

  private:
    static const int BUF_SIZE = 1024 * 1024; // 1 Mb
    Logging::Logger_var logger_;
    FrontendCommons::Frontend_var frontend_;
    const State_var state_;

    pthread_t thread_;

    SyncPolicy::Mutex sock_lock_;
    int sock_;
    volatile bool active_;
    Sync::Conditional sock_attached_;

    char rbuf_[BUF_SIZE];

    Sync::PosixMutex rpos_lock_;
    char* rpos_;
  };

  // Acceptor::AcceptActiveObject
  class Acceptor::AcceptActiveObject: public Commons::DelegateActiveObject
  {
    friend class Acceptor::Worker;
    
  public:
    AcceptActiveObject(
      Generics::ActiveObjectCallback* callback,
      Logging::Logger* logger,
      State* state,
      FrontendCommons::FrontendInterface* frontend,
      const char* bind_address,
      int backlog,
      unsigned long accept_threads,
      WorkerStatsObject* worker_stats)
      /*throw(eh::Exception)*/;

    void
    wait_object() noexcept;

  protected:
    virtual
    ~AcceptActiveObject() noexcept;

    virtual void
    work_() noexcept;

    virtual void
    terminate_() noexcept;

  private:
    Generics::ActiveObjectCallback_var callback_;
    Logging::Logger_var logger_;

    State_var state_;
    FrontendCommons::Frontend_var frontend_;
    const std::string bind_address_;

    int accept_sock_;
    bool remove_sock_;

    WorkerStatsObject_var worker_stats_;
  };

  // Acceptor::HttpResponseWriterImpl
  class Acceptor::HttpResponseWriterImpl: public FrontendCommons::HttpResponseWriter
  {
  public:
    HttpResponseWriterImpl(int sock)
      : sock_(sock)
    {}

    virtual void
    write(int res, FrontendCommons::HttpResponse* response_ptr)
    {
      FrontendCommons::HttpResponse& response = *response_ptr;

      if(res == 0)
      {
        res = 200; // OK
      }

      std::vector<String::SubString> buffers;
      size_t sendsize = response.end_response(buffers, res);

      std::vector<iovec> v(buffers.size());
      struct msghdr msg;
      memset(&msg, 0, sizeof(msg));

      size_t vec_i = 0;
      for(auto buf_it = buffers.begin(); buf_it != buffers.end(); ++buf_it, ++vec_i)
      {
        v[vec_i].iov_base = (void*)buf_it->data();
        v[vec_i].iov_len = buf_it->size();
      }

      //size_t orig_sendsize = sendsize;
      msg.msg_iov = v.data();
      msg.msg_iovlen = v.size();

      while(sendsize)
      {
        if(msg.msg_iovlen == 0)
        {
          assert(0);
        }

        ssize_t res = ::sendmsg(sock_, &msg, 0);
        if (res == -1)
        {
          return;
        }

        while(res > 0)
        {
          if(msg.msg_iov[0].iov_len > 0)
          {
            size_t n = std::min((size_t)res, msg.msg_iov[0].iov_len);
            msg.msg_iov[0].iov_base = ((char*)msg.msg_iov[0].iov_base) + n;
            msg.msg_iov[0].iov_len -= n;
            res -= n;
            sendsize -= n;
          }

          // assert(res == 0 || msg.msg_iov[0].iov_len == 0);

          if (msg.msg_iov[0].iov_len == 0)
          {
            ++msg.msg_iov;
            --msg.msg_iovlen;
          }
        }
      }
    }

  private:
    int sock_;
  };

  // State implementation
  Acceptor::State::State(
    Logging::Logger* logger,
    FrontendCommons::FrontendInterface* frontend,
    WorkerStatsObject* worker_stats)
    : logger_(ReferenceCounting::add_ref(logger)),
      frontend_(ReferenceCounting::add_ref(frontend)),
      worker_stats_(ReferenceCounting::add_ref(worker_stats)),
      stop_in_progress_(false)
  {}

  void
  Acceptor::State::wakeup_worker(int socket)
    noexcept
  {
    // this code can't be runned after terminate
    // get free worker
    Worker_var use_worker;

    {
      WorkersSyncPolicy::WriteGuard guard(free_workers_lock_);
      if(!free_workers_.empty())
      {
        use_worker.swap(free_workers_.front());
        free_workers_.pop_front();
      }
    }

    if(!use_worker)
    {
      use_worker = new Worker(logger_, frontend_, this);

      WorkersSyncPolicy::WriteGuard guard(workers_lock_);
      workers_.emplace_back(use_worker);
    }
    else
    {
      worker_stats_->dec_free_workers();
    }

    worker_stats_->incr_use_workers();

    use_worker->attach_socket(socket);
  }

  void
  Acceptor::State::push_to_free_workers(Worker* worker) noexcept
  {
    worker_stats_->incr_free_workers();
    worker_stats_->dec_use_workers();

    Worker_var push_worker = ReferenceCounting::add_ref(worker);

    WorkersSyncPolicy::WriteGuard guard(free_workers_lock_);
    free_workers_.emplace_front(std::move(push_worker));
  }

  void
  Acceptor::State::stop_and_wait() noexcept
  {
    bool first_stopper = false;
    WorkerArray workers;
    WorkerArray free_workers;

    {
      Sync::PosixGuard guard(stop_lock_);
      first_stopper = !stop_in_progress_;
      stop_in_progress_ = true;
    }

    {
      WorkersSyncPolicy::WriteGuard lock(workers_lock_);
      workers.swap(workers_);
    }

    {
      WorkersSyncPolicy::WriteGuard lock(free_workers_lock_);
      free_workers.swap(free_workers_);
    }

    for(auto worker_it = workers.begin();
      worker_it != workers.end(); ++worker_it)
    {
      (*worker_it)->stop();
    }

    for(auto worker_it = workers.begin();
      worker_it != workers.end(); ++worker_it)
    {
      (*worker_it)->wait();
    }

    if(first_stopper)
    {
      Sync::PosixGuard guard(stop_lock_);
      stop_in_progress_ = false;
      stopped_.broadcast();
    }
    else
    {
      Sync::ConditionalGuard stop_lock(stopped_, stop_lock_);
      while(stop_in_progress_)
      {
        stop_lock.wait();
      }
    }
  }

  // AcceptActiveObject implementation
  Acceptor::AcceptActiveObject::AcceptActiveObject(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    Acceptor::State* state,
    FrontendCommons::FrontendInterface* frontend,
    const char* bind_address,
    int backlog,
    unsigned long accept_threads,
    WorkerStatsObject* worker_stats)
    /*throw(eh::Exception)*/
    : Commons::DelegateActiveObject(callback, accept_threads, 128*1024),
      callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      state_(ReferenceCounting::add_ref(state)),
      frontend_(ReferenceCounting::add_ref(frontend)),
      bind_address_(bind_address),
      worker_stats_(ReferenceCounting::add_ref(worker_stats))
  {
    accept_sock_ = ::socket(AF_LOCAL, SOCK_STREAM, 0);

    if(accept_sock_ == -1)
    {
      eh::throw_errno_exception<Exception>(errno, FNE, "socket() failed");
    }

    sockaddr_un bind_addr;
    ::memset(&bind_addr, 0, sizeof(bind_addr));

    bind_addr.sun_family = AF_UNIX;
    ::strncpy(bind_addr.sun_path, bind_address, sizeof(bind_addr.sun_path) - 1);

    // check if socket file was not removed earlier
    struct stat bind_address_stat;
    if(::stat(bind_address, &bind_address_stat) == 0 &&
      S_ISSOCK(bind_address_stat.st_mode))
    {
      ::unlink(bind_address);
    }

    if(::bind(
      accept_sock_,
      (struct sockaddr *)&bind_addr,
      sizeof(bind_addr)) == -1)
    {
      eh::throw_errno_exception<Exception>(errno, FNE, "bind() failed");
    }

    remove_sock_ = true;

    if (::listen(accept_sock_, backlog) == -1)
    {
      eh::throw_errno_exception<Exception>(errno, FNE, "listen() failed");
    }
  }

  Acceptor::AcceptActiveObject::~AcceptActiveObject() noexcept
  {
    if(remove_sock_)
    {
      ::unlink(bind_address_.c_str());
    }

    if (accept_sock_ != -1)
    {
      ::close(accept_sock_);
    }
  }

  void
  Acceptor::AcceptActiveObject::work_() noexcept
  {
    while(active())
    {
      sockaddr_un peer_addr;
      socklen_t p_size = sizeof(peer_addr);
      int res = ::accept(accept_sock_, (struct sockaddr*)&peer_addr, &p_size);
      if (res == -1)
      {
        if (active())
        {
          char err_msg[256];
          eh::ErrnoHelper::compose_safe(err_msg, sizeof(err_msg), errno, FNE, "accept() failed");
          callback_->error(String::SubString(err_msg));
        }
        continue;
      }

      state_->wakeup_worker(res);
    }
  }

  void
  Acceptor::AcceptActiveObject::terminate_() noexcept
  {
    // terminate only accept loop
    if(accept_sock_ != -1)
    {
      ::shutdown(accept_sock_, SHUT_RDWR);
      ::close(accept_sock_);
      accept_sock_ = -1;
    }
  }

  void
  Acceptor::AcceptActiveObject::wait_object() noexcept
  {
    Commons::DelegateActiveObject::wait_object();
  }

  // Worker implementation
  Acceptor::Worker::Worker(
    Logging::Logger* logger,
    FrontendCommons::FrontendInterface* frontend,
    Acceptor::State* state)
    noexcept
    : logger_(ReferenceCounting::add_ref(logger)),
      frontend_(ReferenceCounting::add_ref(frontend)),
      state_(ReferenceCounting::add_ref(state)),
      thread_(0),
      sock_(-1),
      active_(true),
      rpos_(rbuf_)
  {
    pthread_attr_t attr;
    size_t size = 256 * 1024;
    ::pthread_attr_init(&attr);
    ::pthread_attr_setstacksize(&attr, size);
    int res = ::pthread_create(&thread_, 0, thread_proc, this);
    if (res)
    {
      char err_msg[256];
      eh::ErrnoHelper::compose_safe(err_msg, sizeof(err_msg), res, FNE, "pthread_create() failed");
      logger->error(String::SubString(err_msg), Aspect::WORKER);
    }
  }

  Acceptor::Worker::~Worker() noexcept
  {
    assert(thread_ == 0);

    /*
    state_->unreg_worker(id_, thread_);
    if (sock_ != -1)
    {
      close(sock_);
    }
    */
  }

  Logging::Logger*
  Acceptor::Worker::logger() noexcept
  {
    return logger_;
  }

  void
  Acceptor::Worker::stop()
  {
    int sock;

    {
      SyncPolicy::WriteGuard guard(sock_lock_);
      active_ = false;
      sock = sock_;
      sock_ = -1;
      sock_attached_.signal();
    }
    
    {
      Guard lock(rpos_lock_);
      if (rpos_ != rbuf_)
      {
        return;
      }
    }

    ::shutdown(sock, SHUT_RDWR);
    ::close(sock);
  }

  void
  Acceptor::Worker::wait()
  {
    ::pthread_join(thread_, 0);
    thread_ = 0;
  }

  void
  Acceptor::Worker::attach_socket(int sock) noexcept
  {
    {
      SyncPolicy::WriteGuard guard(sock_lock_);
      sock_ = sock;
      sock_attached_.signal();
    }
  }

  void*
  Acceptor::Worker::thread_proc(void* ctx)
  {
    Worker* w(static_cast<Worker*>(ctx));
    w->work_();
    return 0;
  }

  void
  Acceptor::Worker::socket_loop_() noexcept
  {
    // process socket
    while (active_)
    {
      FCGI::HttpRequestHolderFCGI_var request_holder(new FCGI::HttpRequestHolderFCGI);

      {
        Guard lock(rpos_lock_);
        rpos_ = rbuf_;
      }

      size_t rsize = 0;

      do
      {
        ssize_t recv_res = recv(sock_, rpos_, sizeof(rbuf_) - rsize, 0);

        if (recv_res == -1)
        {
          // TODO: handle stop event and signal interrupt
          /*
          char err_msg[256];
          eh::ErrnoHelper::compose_safe(err_msg, sizeof(err_msg), errno, FNE, "recv() failed");
          logger()->error(String::SubString(err_msg), Aspect::WORKER);
          */
          return;
        }

        if (recv_res == 0)
        {
          // TODO: handle closed conn
          logger()->info(String::SubString("connection closed"), Aspect::WORKER);
          return;
        }

        {
          Guard lock(rpos_lock_);
          rpos_ += recv_res;
        }

        rsize += recv_res;

        int parse_res = request_holder->parse(rbuf_, rsize);
        if (parse_res == FCGI::PARSE_OK)
        {
          break;
        }

        switch(parse_res)
        {
        case FCGI::PARSE_NEED_MORE:
          logger()->warning(String::SubString("getting PARSE_NEED_MORE"), Aspect::WORKER);
          break;

        case FCGI::PARSE_INVALID_HEADER:
          logger()->info(String::SubString("invalid fcgi header"), Aspect::WORKER);
          return;
        case FCGI::PARSE_BEGIN_REQUEST_EXPECTED:
          logger()->info(String::SubString("begin request expected"), Aspect::WORKER);
          return;
        case FCGI::PARSE_INVALID_ID:
          logger()->info(String::SubString("invalid FCGI header id"), Aspect::WORKER);
          return;
        case FCGI::PARSE_FRAGMENTED_STDIN:
          logger()->info(String::SubString("fragmented stdin"), Aspect::WORKER);
          return;
        }

        if (rsize >= sizeof(rbuf_))
        {
          logger()->info(String::SubString("request too big"), Aspect::WORKER);
          return;
        }
      }
      while(true);

      FrontendCommons::HttpResponseWriter_var response_writer(new HttpResponseWriterImpl(sock_));

      // process
      frontend_->handle_request_noparams(
        std::move(request_holder),
        std::move(response_writer));
    }
  }

  void
  Acceptor::Worker::work_() noexcept
  {
    while(active_)
    {
      // wait socket attach
      {
        Sync::ConditionalGuard guard(sock_attached_, sock_lock_);
        while(sock_ == -1 && active_)
        {
          guard.wait();
        }
      }

      if(!active_)
      {
        break;
      }

      socket_loop_();
      ::close(sock_);
      sock_ = -1;

      // push self to free workers
      state_->push_to_free_workers(this);
    }

    if(sock_ != -1)
    {
      ::close(sock_);
    }
  }

  // Acceptor implementation
  Acceptor::Acceptor(
    Logging::Logger* logger,
    FrontendCommons::FrontendInterface* frontend,
    Generics::ActiveObjectCallback* callback,
    const char* bind_address,
    unsigned long backlog,
    unsigned long accept_threads)
    /*throw(eh::Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      frontend_(ReferenceCounting::add_ref(frontend)),
      worker_stats_object_(new WorkerStatsObject(
        logger,
        callback)),
      state_(new Acceptor::State(logger, frontend, worker_stats_object_.in())),
      shutdown_uniq_(0)
  {
    add_child_object(Generics::ActiveObject_var(
      new AcceptActiveObject(
        callback,
        logger,
        state_,
        frontend_,
        bind_address,
        backlog,
        accept_threads,
        worker_stats_object_)));
  }

  Acceptor::~Acceptor() noexcept
  {}

  void
  Acceptor::activate_object()
    /*throw(Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::activate_object();

    worker_stats_object_->activate_object();
    //join_active_object_->activate_object();
  }

  void
  Acceptor::deactivate_object()
    /*throw(Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::deactivate_object();
  }

  void
  Acceptor::wait_object()
    /*throw(Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::wait_object();

    // accept stopped - workers_ can't be increased now
    state_->stop_and_wait();

    // Deactivate worker stats
    worker_stats_object_->deactivate_object();
    worker_stats_object_->wait_object();
  }

  FrontendCommons::FrontendInterface*
  Acceptor::handler() noexcept
  {
    return frontend_.in();
  }

  Logging::Logger*
  Acceptor::logger() noexcept
  {
    return logger_.in();
  }
}
}
