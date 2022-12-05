#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <deque>

#include <boost/bind.hpp>
#include <boost/asio/local/stream_protocol.hpp>

#include <gears/Errno.hpp>
#include <gears/Exception.hpp>
#include <gears/SubString.hpp>
#include <gears/OutputMemoryStream.hpp>

#include <core/Frontends/FCGI.hpp>

#include "FCGIRequestHandlerPool.hpp"
#include "BoostAsioContextRunActiveObject.hpp"

#include "FCGIServer.hpp"

namespace AdServer
{
namespace Frontends
{
  namespace Aspect
  {
    const char WORKER[] = "FCGI::Connection";
  }

  class FCGIServer::Connection:
    public std::enable_shared_from_this<FCGIServer::Connection>
  {
  public:
    typedef boost::asio::local::stream_protocol::socket SocketType;

    Connection(
      std::shared_ptr<boost::asio::io_service> io_service,
      Gears::Logger_var logger,
      AdServer::HTTP::FCGIRequestHandler_var frontend,
      State_var state)
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
      FCGI::HttpResponse_var response)
      noexcept;

    SocketType&
    socket() noexcept;

  private:
    typedef Gears::Mutex SyncPolicy;
    typedef Gears::Mutex WriteBufSyncPolicy;

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

    Gears::Logger*
    logger_i_() noexcept;

    void
    order_read_();

    void
    order_write_();

    bool
    process_read_data_(size_t bytes_transferred);

    void
    process_request_(
      boost::asio::yield_context yield,
      FCGI::HttpRequest_var request) noexcept;

  private:
    static const int READ_BUF_SIZE_ = 1024 * 1024; // 1 Mb

  private:
    Gears::Logger_var logger_;
    HTTP::FCGIRequestHandler_var frontend_;
    State_var state_;
    std::shared_ptr<boost::asio::io_service> io_service_;
    std::shared_ptr<boost::asio::io_context::strand> strand_;

    SocketType socket_;
    std::atomic<int> active_;

    std::atomic<int> read_ordered_;
    unsigned char rbuf_[READ_BUF_SIZE_];
    std::vector<unsigned char> full_rbuf_;

    std::atomic<int> write_ordered_;
    WriteBufSyncPolicy send_bufs_lock_;
    SendBufPtrArray send_bufs_;
    SendBufPtrArray ordered_send_bufs_;
  };

  class FCGIServer::ResponseWriter :
    public HTTP::HttpResponseWriter,
    public std::enable_shared_from_this<FCGIServer::ResponseWriter>
  {
  public:
    ResponseWriter(
      Connection_var connection)
      : connection_(std::move(connection))
    {}

    virtual void
    write(int code, FCGI::HttpResponse_var response)
    {
      connection_->send_response(code, response);
    }

  protected:
    FCGIServer::Connection_var connection_;
  };

  // State implementation
  FCGIServer::State::State() noexcept
  {}

  // FCGIServer::Connection::SendBuf
  FCGIServer::Connection::SendBuf::SendBuf(SendBuf&& init)
  {
    //wbuf.swap(init.wbuf);
    response.swap(init.response);
    bufs.swap(init.bufs);
  }

  // Connection implementation
  FCGIServer::Connection::Connection(
    std::shared_ptr<boost::asio::io_service> io_service,
    Gears::Logger_var logger,
    AdServer::HTTP::FCGIRequestHandler_var frontend,
    State_var state)
    noexcept
    : logger_(std::move(logger)),
      frontend_(std::move(frontend)),
      io_service_(std::move(io_service)),
      strand_(new boost::asio::io_context::strand(*io_service_)),
      socket_(*io_service_),
      active_(0),
      read_ordered_(0),
      write_ordered_(0)
  {}

  FCGIServer::Connection::~Connection() noexcept
  {}

  FCGIServer::Connection::SocketType&
  FCGIServer::Connection::socket() noexcept
  {
    return socket_;
  }

  void
  FCGIServer::Connection::activate()
  {
    active_ = true;
    order_read_();
  }

  void
  FCGIServer::Connection::deactivate()
  {
    if(active_)
    {
      active_ = false;
      socket_.close();
    }
  }

  void
  FCGIServer::Connection::handle_read(
    const boost::system::error_code& error,
    size_t bytes_transferred)
  {
    if(!error)
    {
      // process got buffer
      bool order_read = process_read_data_(bytes_transferred);

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
  FCGIServer::Connection::handle_write(const boost::system::error_code& error)
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
  FCGIServer::Connection::order_read_()
  {
    if(++read_ordered_ == 1)
    {
      socket_.async_read_some(
        boost::asio::buffer(rbuf_, READ_BUF_SIZE_),
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
  FCGIServer::Connection::order_write_()
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
  FCGIServer::Connection::process_read_data_(size_t bytes_transferred)
  {
    bool order_next_read = true;

    const unsigned char* data_start;
    const unsigned char* data_end;

    if(!full_rbuf_.empty())
    {
      // previous read didn't give full request
      full_rbuf_.insert(full_rbuf_.end(), rbuf_, rbuf_ + bytes_transferred);
      data_start = &full_rbuf_[0];
      data_end = &full_rbuf_[0] + full_rbuf_.size();
    }
    else
    {
      data_start = rbuf_;
      data_end = rbuf_ + bytes_transferred;
    }

    // try parse request
    FCGI::HttpRequest_var request(new FCGI::HttpRequest);
    int parse_res = request->parse(data_start, data_end - data_start);
    if(parse_res == FCGI::PARSE_OK)
    {
      // push to async processing
      auto self(shared_from_this());
      boost::asio::spawn(
        *strand_, [this, self, request](auto yield)
        {
          process_request_(yield, request);
        }
        );
    }

    switch(parse_res)
    {
    case FCGI::PARSE_NEED_MORE:
      //logger_i_()->warning(Gears::SubString("getting PARSE_NEED_MORE"), Aspect::WORKER);
      break;

    case FCGI::PARSE_INVALID_HEADER:
      logger_i_()->info(Gears::SubString("invalid fcgi header"), Aspect::WORKER);
      return false;
    case FCGI::PARSE_BEGIN_REQUEST_EXPECTED:
      logger_i_()->info(Gears::SubString("begin request expected"), Aspect::WORKER);
      return false;
    case FCGI::PARSE_INVALID_ID:
      logger_i_()->info(Gears::SubString("invalid FCGI header id"), Aspect::WORKER);
      return false;
    case FCGI::PARSE_FRAGMENTED_STDIN:
      logger_i_()->info(Gears::SubString("fragmented stdin"), Aspect::WORKER);
      return false;
    }

    return true;
  }

  void
  FCGIServer::Connection::process_request_(
    boost::asio::yield_context yield,
    FCGI::HttpRequest_var request) noexcept
  {
    int res_code;

    try
    {
      frontend_->handle_request_noparams(
        yield,
        *strand_,
        request,
        HTTP::HttpResponseWriter_var(
          new FCGIServer::ResponseWriter(shared_from_this())));
    }
    catch(const Gears::Exception& e)
    {
      Gears::ErrorStream ostr;
      ostr << "Can't handle request '" << request->uri() <<
        "': " << e.what();

      logger_i_()->error(ostr.str(), Aspect::WORKER);      
    }
  }

  void
  FCGIServer::Connection::send_response(
    int code,
    FCGI::HttpResponse_var response_ptr)
    noexcept
  {
    FCGI::HttpResponse_var response(std::move(response_ptr));

    // send response
    std::vector<Gears::SubString> buffers;
    size_t sendsize = response->end_response(buffers, code);

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

  Gears::Logger*
  FCGIServer::Connection::logger_i_() noexcept
  {
    return logger_.get();
  }

  // FCGIServer::Response impl

  // FCGIServer implementation
  FCGIServer::FCGIServer(
    Gears::Logger_var logger,
    AdServer::HTTP::FCGIRequestHandler_var frontend,
    Gears::ActiveObjectCallback_var callback,
    const Gears::SubString& bind_address,
    unsigned long backlog,
    unsigned long process_threads)
    /*throw(Gears::Exception)*/
      : callback_(std::move(callback)),
      logger_(std::move(logger)),
      frontend_(std::move(frontend)),
      io_service_(new boost::asio::io_service()),
      state_(new FCGIServer::State()),
      accept_ordered_(0)
  {
    static const char* FUN = "FCGIServer::FCGIServer()";

    try
    {
      ::unlink(bind_address.str().c_str());

      /*
      acceptor_ = std::make_shared<AcceptorType>(
        *io_service_,
        boost::asio::local::stream_protocol::endpoint(bind_address.str()));
      */

      acceptor_ = std::make_shared<AcceptorType>(*io_service_);
      acceptor_->open(boost::asio::local::stream_protocol());
      acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
      acceptor_->bind(boost::asio::local::stream_protocol::endpoint(bind_address.str()));
      acceptor_->listen(backlog);
    }
    catch(const std::exception& ex)
    {
      Gears::ErrorStream ostr;
      ostr << FUN << ": can't init acceptor(socket = " << bind_address << " ): " << ex.what();
      throw Exception(ostr.str());
    }

    // order is important
    add_child_object(Gears::ActiveObject_var(
      new BoostAsioContextRunActiveObject(
        callback_,
        io_service_,
        process_threads)));

    // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
    /*
    boost::asio::ip::tcp::resolver resolver(io_service_);
    boost::asio::ip::tcp::resolver::query query(address, port);
    boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    */

    /*
    EndpointType endpoint(bind_address.str());
    acceptor_->open(endpoint);
    */

    create_accept_stub_();
  }

  FCGIServer::~FCGIServer() noexcept
  {}

  void
  FCGIServer::wait_object()
    /*throw(Exception, Gears::Exception)*/
  {
    Gears::CompositeActiveObject::wait_object();

    frontend_->shutdown();
  }

  AdServer::HTTP::FCGIRequestHandler_var
  FCGIServer::handler() noexcept
  {
    return frontend_;
  }

  void
  FCGIServer::create_accept_stub_()
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
          &FCGIServer::handle_accept,
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
  FCGIServer::handle_accept(
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
}
}
