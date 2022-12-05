#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/io_service.hpp>
//#include <boost/beast/core/bind_handler.hpp>

//#include <eh/Errno.hpp>

#include <deque>

#include <Frontends/FrontendCommons/FCGI.hpp>
#include "FrontendsPool.hpp"

#include "BoostAsioContextRunActiveObject.hpp"
#include "AcceptorBoostAsio.hpp"

namespace AdServer
{
namespace Frontends
{
  namespace Aspect
  {
    const char WORKER[] = "FCGI::Worker";
  }
  
  // State
  struct AcceptorBoostAsio::State: public ReferenceCounting::AtomicImpl
  {
    State(
      Logging::Logger* logger,
      FrontendCommons::FrontendInterface* frontend,
      WorkerStatsObject* workers_stats);

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

    /*
    Sync::PosixMutex stop_lock_;
    Sync::Conditional stopped_;
    bool stop_in_progress_;
    */
  };

  class AcceptorBoostAsio::Connection:
    public std::enable_shared_from_this<AcceptorBoostAsio::Connection>
  {
  public:
    typedef boost::asio::local::stream_protocol::socket SocketType;

    Connection(
      std::shared_ptr<boost::asio::io_service> io_service,
      Logging::Logger* logger,
      FrontendCommons::FrontendInterface* frontend,
      State* state)
      noexcept;

    virtual
    ~Connection() noexcept;

    void
    activate();

    void
    deactivate();

    void
    handle_read(
      const boost::system::error_code& error,
      size_t bytes_transferred);

    void
    handle_write(const boost::system::error_code& error);

    void
    send_response(
      int code,
      FCGI::HttpResponse* response)
      noexcept;

    SocketType&
    socket() noexcept;

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;
    typedef Sync::Policy::PosixThread WriteBufSyncPolicy;

    struct SendBuf
    {
      SendBuf() noexcept
      {};

      SendBuf(SendBuf&& init);

      //std::vector<char> wbuf;
      FCGI::HttpResponse_var response; // hold buffers ownership
      std::vector<boost::asio::const_buffer> bufs;
    };

    typedef std::unique_ptr<SendBuf> SendBufPtr;
    typedef std::vector<SendBufPtr> SendBufPtrArray;

  protected:
    void
    work_() noexcept;

    Logging::Logger*
    logger_i_() noexcept;

    void
    order_read_();

    void
    order_write_();

    bool
    process_read_data_(size_t bytes_transferred);

  private:
    static const int READ_BUF_SIZE_ = 512 * 1024; // 512 Kb

  private:
    Logging::Logger_var logger_;
    FrontendCommons::Frontend_var frontend_;
    State_var state_;
    std::shared_ptr<boost::asio::io_service> io_service_;
    //std::shared_ptr<boost::asio::io_context::strand> strand_;

    SocketType socket_;
    std::atomic<int> active_;

    std::atomic<int> read_ordered_;
    //unsigned char rbuf_[READ_BUF_SIZE_];
    std::vector<unsigned char> rbuf_;
    std::vector<unsigned char> full_rbuf_;

    std::atomic<int> write_ordered_;
    WriteBufSyncPolicy::Mutex send_bufs_lock_;
    SendBufPtrArray send_bufs_;
    SendBufPtrArray ordered_send_bufs_;
  };

  // AcceptorBoostAsio::HttpResponseWriterImpl
  class AcceptorBoostAsio::HttpResponseWriterImpl: public FCGI::HttpResponseWriter
  {
  public:
    HttpResponseWriterImpl(Connection_var conn)
      : conn_(conn),
        sent_response_(0)
    {}

    virtual void
    write(int res, FCGI::HttpResponse* response_ptr)
    {
      if(res == 0)
      {
        res = 200; // OK
      }

      if(sent_response_.exchange_and_add(1) == 0)
      {
        conn_->send_response(res, response_ptr);
      }
    }

  private:
    AcceptorBoostAsio::Connection_var conn_;
    Generics::AtomicInt sent_response_;
  };

  // State implementation
  AcceptorBoostAsio::State::State(
    Logging::Logger* logger,
    FrontendCommons::FrontendInterface* frontend,
    WorkerStatsObject* worker_stats)
    : logger_(ReferenceCounting::add_ref(logger)),
      frontend_(ReferenceCounting::add_ref(frontend)),
      worker_stats_(ReferenceCounting::add_ref(worker_stats))
      /*,
      stop_in_progress_(false)
      */
  {}

  // AcceptorBoostAsio::Connection::SendBuf
  AcceptorBoostAsio::Connection::SendBuf::SendBuf(SendBuf&& init)
  {
    //wbuf.swap(init.wbuf);
    response.swap(init.response);
    bufs.swap(init.bufs);
  }

  // AcceptorBoostAsio::Connection implementation
  AcceptorBoostAsio::Connection::Connection(
    std::shared_ptr<boost::asio::io_service> io_service,
    Logging::Logger* logger,
    FrontendCommons::FrontendInterface* frontend,
    State* /*state*/)
    noexcept
    : logger_(ReferenceCounting::add_ref(logger)),
      frontend_(ReferenceCounting::add_ref(frontend)),
      io_service_(std::move(io_service)),
      //strand_(new boost::asio::io_context::strand(*io_service_)),
      socket_(*io_service_),
      active_(0),
      read_ordered_(0),
      write_ordered_(0)
  {
    rbuf_.resize(READ_BUF_SIZE_);
  }

  AcceptorBoostAsio::Connection::~Connection() noexcept
  {
    //std::cerr << "AcceptorBoostAsio::Connection::~Connection()" << std::endl;
  }

  AcceptorBoostAsio::Connection::SocketType&
  AcceptorBoostAsio::Connection::socket() noexcept
  {
    return socket_;
  }

  void
  AcceptorBoostAsio::Connection::activate()
  {
    active_ = true;
    order_read_();
  }

  void
  AcceptorBoostAsio::Connection::deactivate()
  {
    if(active_)
    {
      active_ = false;
      socket_.close();
    }
  }

  void
  AcceptorBoostAsio::Connection::handle_read(
    const boost::system::error_code& error,
    size_t bytes_transferred)
  {
    if(!error)
    {
      // process got buffer
      process_read_data_(bytes_transferred);

      --read_ordered_;

      order_write_();
      order_read_();
    }
    else
    {
      // destroy & close on ref count == 0
    }
  }

  void
  AcceptorBoostAsio::Connection::handle_write(const boost::system::error_code& error)
  {
    if(!error)
    {
      // writing done
      --write_ordered_;

      order_write_();
    }
    else
    {
      // destroy & close on ref count == 0
    }
  }

  void
  AcceptorBoostAsio::Connection::order_read_()
  {
    if(++read_ordered_ == 1)
    {
      socket_.async_read_some(
        boost::asio::buffer(&rbuf_[0], READ_BUF_SIZE_),
        boost::bind(
          &Connection::handle_read,
          shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      --read_ordered_;
    }
  }

  void
  AcceptorBoostAsio::Connection::order_write_()
  {
    SendBufPtrArray destroy_bufs_;

    if(++write_ordered_ == 1)
    {
      // pass buffer that point to ordered_send_buf_
      {
        WriteBufSyncPolicy::WriteGuard lock(send_bufs_lock_);
        ordered_send_bufs_.swap(destroy_bufs_);
        send_bufs_.swap(ordered_send_bufs_);
      }

      if(!ordered_send_bufs_.empty())
      {
        if(ordered_send_bufs_.size() > 1)
        {
          std::vector<boost::asio::const_buffer> buffer_seq;
          for(auto buf_it = ordered_send_bufs_.begin(); buf_it != ordered_send_bufs_.end(); ++buf_it)
          {
            buffer_seq.assign((*buf_it)->bufs.begin(), (*buf_it)->bufs.end());
          }

          boost::asio::async_write(
            socket_,
            buffer_seq,
            boost::bind(
              &Connection::handle_write,
              shared_from_this(),
              boost::asio::placeholders::error));
        }
        else
        {
          boost::asio::async_write(
            socket_,
            (*ordered_send_bufs_.begin())->bufs,
            boost::bind(
              &Connection::handle_write,
              shared_from_this(),
              boost::asio::placeholders::error));
          
        }
      }
      else
      {
        --write_ordered_;
      }
    }
    else
    {
      --write_ordered_;
    }
  }

  bool
  AcceptorBoostAsio::Connection::process_read_data_(size_t bytes_transferred)
  {
    const unsigned char* data_start;
    const unsigned char* data_end;

    if(!full_rbuf_.empty())
    {
      // previous read didn't give full request
      full_rbuf_.insert(full_rbuf_.end(), &*rbuf_.begin(), &*(rbuf_.begin() + bytes_transferred));
      data_start = &full_rbuf_[0];
      data_end = &full_rbuf_[0] + full_rbuf_.size();
    }
    else
    {
      data_start = &*rbuf_.begin();
      data_end = &*(rbuf_.begin() + bytes_transferred);
    }

    // try parse request
    FCGI::HttpRequestHolder_var request_holder(new FCGI::HttpRequestHolder());

    int parse_res = request_holder->parse(data_start, data_end - data_start);
    if(parse_res == FCGI::PARSE_OK)
    {
      FCGI::HttpResponseWriter_var response_writer(
        new HttpResponseWriterImpl(shared_from_this()));

      // process
      frontend_->handle_request_noparams(
        std::move(request_holder),
        std::move(response_writer));
    }

    switch(parse_res)
    {
    case FCGI::PARSE_NEED_MORE:
      //logger_i_()->warning(String::SubString("getting PARSE_NEED_MORE"), Aspect::WORKER);
      break;

    case FCGI::PARSE_INVALID_HEADER:
      logger_i_()->info(String::SubString("invalid fcgi header"), Aspect::WORKER);
      return false;
    case FCGI::PARSE_BEGIN_REQUEST_EXPECTED:
      logger_i_()->info(String::SubString("begin request expected"), Aspect::WORKER);
      return false;
    case FCGI::PARSE_INVALID_ID:
      logger_i_()->info(String::SubString("invalid FCGI header id"), Aspect::WORKER);
      return false;
    case FCGI::PARSE_FRAGMENTED_STDIN:
      logger_i_()->info(String::SubString("fragmented stdin"), Aspect::WORKER);
      return false;
    }

    return true;
  }

  void
  AcceptorBoostAsio::Connection::send_response(
    int code,
    FCGI::HttpResponse* response_ptr)
    noexcept
  {
    FCGI::HttpResponse_var response(ReferenceCounting::add_ref(response_ptr));

    // send response
    std::vector<String::SubString> buffers;
    response->end_response(buffers, code);

    std::unique_ptr<SendBuf> send_buf(new SendBuf());
    send_buf->response.swap(response);
    for(auto buf_it = buffers.begin(); buf_it != buffers.end(); ++buf_it)
    {
      send_buf->bufs.push_back(boost::asio::const_buffer(buf_it->data(), buf_it->size()));
    }

    {
      WriteBufSyncPolicy::WriteGuard lock(send_bufs_lock_);
      send_bufs_.emplace_back(std::move(send_buf));
    }

    order_write_();
  }

  Logging::Logger*
  AcceptorBoostAsio::Connection::logger_i_() noexcept
  {
    return logger_;
  }

  // AcceptorBoostAsio implementation
  AcceptorBoostAsio::AcceptorBoostAsio(
    Logging::Logger* logger,
    FrontendCommons::FrontendInterface* frontend,
    Generics::ActiveObjectCallback* callback,
    const String::SubString& bind_address,
    unsigned long backlog,
    unsigned long process_threads)
    /*throw(eh::Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      frontend_(ReferenceCounting::add_ref(frontend)),
      worker_stats_object_(new WorkerStatsObject(
        logger,
        callback)),
      state_(new AcceptorBoostAsio::State(logger, frontend, worker_stats_object_.in())),
      //accept_io_service_(new boost::asio::io_service()),
      io_service_(new boost::asio::io_service()),
      accept_ordered_(0),
      shutdown_uniq_(0)
  {
    ::unlink(bind_address.str().c_str());

    acceptor_ = std::make_shared<AcceptorType>(*io_service_);
    acceptor_->open(boost::asio::local::stream_protocol());
    acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_->bind(boost::asio::local::stream_protocol::endpoint(bind_address.str()));
    acceptor_->listen(backlog);

    /*
    add_child_object(Generics::ActiveObject_var(
      new BoostAsioContextRunActiveObject(
        callback_,
        accept_io_service_,
        1)));
    */

    add_child_object(Generics::ActiveObject_var(
      new BoostAsioContextRunActiveObject(
        callback_,
        io_service_,
        process_threads)));

    create_accept_stub_();
  }

  AcceptorBoostAsio::~AcceptorBoostAsio() noexcept
  {}

  void
  AcceptorBoostAsio::activate_object()
    /*throw(Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::activate_object();

    worker_stats_object_->activate_object();
  }

  void
  AcceptorBoostAsio::deactivate_object()
    /*throw(Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::deactivate_object();
  }

  void
  AcceptorBoostAsio::wait_object()
    /*throw(Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::wait_object();

    // Deactivate worker stats
    worker_stats_object_->deactivate_object();
    worker_stats_object_->wait_object();
  }

  void
  AcceptorBoostAsio::create_accept_stub_()
  {
    /*
    if(++accept_ordered_ == 1)
    {
    */
    // create stub for new connection
    Connection_var new_connection(
      new Connection(io_service_, logger_, frontend_, state_));

    acceptor_->async_accept(
      new_connection->socket(),
      boost::bind(
      //boost::bind_front_handler(
        &AcceptorBoostAsio::handle_accept_,
        this,
        new_connection,
        boost::asio::placeholders::error));
    /*
    }
    else
    {
      --accept_ordered_;
    }
    */
  }

  void
  AcceptorBoostAsio::handle_accept_(
    const Connection_var& accepted_connection,
    const boost::system::error_code& error)
  {
    //--accept_ordered_;

    if(!error)
    {
      // activate stub as normal connection
      accepted_connection->activate();
    }
    else
    {
      // descriptors limit reached ? don't stop
      std::cerr << "Can't accept connection: " << error << std::endl;
    }

    create_accept_stub_();
  }

  FrontendCommons::FrontendInterface*
  AcceptorBoostAsio::handler() noexcept
  {
    return frontend_.in();
  }

  Logging::Logger*
  AcceptorBoostAsio::logger() noexcept
  {
    return logger_.in();
  }
}
}
