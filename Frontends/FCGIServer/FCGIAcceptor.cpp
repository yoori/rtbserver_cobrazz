#include <algorithm>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/io_service.hpp>

#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>

#include "FCGIAcceptor.hpp"

namespace
{
  const std::string REQUEST_URI("REQUEST_URI");
  const std::string REQUEST_METHOD("REQUEST_METHOD");
  const std::string QUERY_STRING("QUERY_STRING");
  const std::string HTTPS("HTTPS");
  const std::string REMOTE_ADDR("REMOTE_ADDR");

  const std::string GET("GET");
  const std::string POST("POST");
  const std::string HEAD("HEAD");

  std::string PASS_REMOTE_ADDR(".remotehost");
  std::string PASS_SECURE("secure");
  std::string PASS_SECURE_VALUE("1");

  std::string CRLF("\r\n");
  const String::SubString STATUS_HEADER("Status: ");
  const String::SubString HEADER_SEPARATOR(": ");
  const String::SubString SET_COOKIE_HEADER("Set-Cookie: ");
  const String::SubString CONTENT_LENGTH_HEADER("Content-Length: ");
  std::string STATUS_200("OK");
  std::string STATUS_204("No Content");
  std::string STATUS_301("Moved Permanently");
  std::string STATUS_302("Found");
  std::string STATUS_303("See Other");
  std::string STATUS_307("Temporary Redirect");
  std::string STATUS_400("Bad Request");
  std::string STATUS_403("Forbidden");
  std::string STATUS_404("Not Found");
  std::string STATUS_500("Internal Server Error");
}

namespace FCGI
{
  int
  HttpRequestHolder::parse(const void* buf, unsigned long size)
  {
    buf_.assign(static_cast<const char*>(buf), size);
    char* mutable_buf = const_cast<char*>(buf_.data());
    const size_t mutable_size = size;

    HttpRequest& request = http_request_;
    request.set_method(HttpRequest::RM_GET);
    request.set_uri(String::SubString());
    request.set_args(String::SubString());
    request.set_body(String::SubString());
    request.set_headers(HTTP::SubHeaderList());
    request.set_params(HTTP::ParamList());
    request.set_server_name(String::SubString());
    request.set_secure(false);
    request.set_header_only(false);

    tinyfcgi::const_message m(mutable_buf, mutable_size);

    uint16_t id = 0;
    bool need_more = true;

    auto stdin_i = m.end();

    HTTP::SubHeaderList headers;
    for(auto i = m.begin(); i != m.end(); ++i)
    {
      const tinyfcgi::header& h = *i;

      if (!h.valid())
      {
        return PARSE_INVALID_HEADER;
      }

      if (id == 0)
      {
        if (h.type != FCGI_BEGIN_REQUEST)
        {
          return PARSE_BEGIN_REQUEST_EXPECTED;
        }
        id = h.id();
      }
      else if (id != h.id())
      {
        return PARSE_INVALID_ID;
      }

      if (h.type == FCGI_STDIN)
      {
        if (h.size() == 0)
        {
          need_more = false;
          break;
        }

        if (stdin_i == m.end())
        {
          stdin_i = i;
        }
        else
        {
          return PARSE_FRAGMENTED_STDIN;
        }

        auto next_i = i;
        ++next_i;
        if (next_i.valid() && next_i->valid() &&
          next_i->type == FCGI_STDIN && next_i->size() > 0)
        {
          tinyfcgi::header* mh = (tinyfcgi::header*)&h;
          mh->merge_next();
          continue;
        }
      }
    }

    if(need_more)
    {
      return PARSE_NEED_MORE;
    }

    if (stdin_i.valid())
    {
      request.set_body(stdin_i->str());
    }

    for(auto i = m.begin(); i != m.end(); ++i)
    {
      const tinyfcgi::header& h = *i;
      if (h.type == FCGI_PARAMS)
      {
        tinyfcgi::const_params params(h.str());
        for(auto i = params.begin(); i != params.end(); ++i)
        {
          String::SubString name, value;
          i->read(name, value);

          if(name == QUERY_STRING)
          {
            request.set_args(value);
          }
          else if(name == REQUEST_URI)
          {
            request.set_uri(value);
          }
          else if(name == REQUEST_METHOD)
          {
            if(value == GET)
            {
              request.set_method(HttpRequest::RM_GET);
            }
            else if(value == HEAD)
            {
              request.set_method(HttpRequest::RM_GET);
              request.set_header_only(true);
            }
            else if(value == POST)
            {
              request.set_method(HttpRequest::RM_POST);
            }
          }
          else if(name == HTTPS)
          {
            request.set_secure(true);
            headers.push_back(HTTP::SubHeader(PASS_SECURE, PASS_SECURE_VALUE));
          }
          else if(name == REMOTE_ADDR)
          {
            headers.push_back(HTTP::SubHeader(PASS_REMOTE_ADDR, value));
          }
          else if(name.size() > 5 && name.compare(0, 5, "HTTP_") == 0)
          {
            headers.push_back(HTTP::SubHeader(name.substr(5), value));
          }
        }
      }
    }
    request.set_headers(std::move(headers));
    return PARSE_OK;
  }
}

namespace AdServer
{
namespace Frontends
{
  namespace Aspect
  {
    const char WORKER[] = "FCGI::Worker";
  }
  
  // State
  struct FCGIAcceptor::State: public ReferenceCounting::AtomicImpl
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
  };

  class FCGIAcceptor::Connection:
    public std::enable_shared_from_this<FCGIAcceptor::Connection>
  {
  public:
    typedef boost::asio::local::stream_protocol::socket SocketType;

    Connection(
      boost::asio::io_service& io_service,
      Logging::Logger* logger,
      FrontendCommons::FrontendInterface* frontend,
      State* state,
      FCGIAcceptorStats* stats)
      noexcept;

    virtual
    ~Connection() noexcept;

    void
    activate();

    void
    deactivate();

    void
    handle_read_(
      const boost::system::error_code& error,
      size_t bytes_transferred);

    void
    handle_write_(const boost::system::error_code& error);

    void
    send_response(
      FCGI::HttpResponse_var response,
      std::vector<std::string>&& response_chunks)
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
      std::vector<std::string> response_chunks;
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
    FCGIAcceptorStats_var stats_;
    State_var state_;
    boost::asio::io_service& io_service_;
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

  // FCGIAcceptor::FCGIResponseWriter
  class FCGIAcceptor::FCGIResponseWriter: public FCGI::BaseHttpResponseWriter
  {
  public:
    FCGIResponseWriter(Connection_var conn)
      : conn_(conn),
        sent_response_(0)
    {}

    virtual void
    write(FCGI::HttpResponse_var response_ptr)
    {
      if(sent_response_.exchange_and_add(1) == 0)
      {
        conn_->send_response(
          response_ptr,
          make_fcgi_response_(response_ptr));
      }
    }

  private:
    static std::vector<std::string>
    make_fcgi_response_(const FCGI::HttpResponse_var& response)
    {
      std::vector<std::string> result;

      const int status = response->status() == 0 ? 200 : response->status();
      std::string status_text;
      switch(status)
      {
        case 200: status_text = STATUS_200; break;
        case 204: status_text = STATUS_204; break;
        case 301: status_text = STATUS_301; break;
        case 302: status_text = STATUS_302; break;
        case 303: status_text = STATUS_303; break;
        case 307: status_text = STATUS_307; break;
        case 400: status_text = STATUS_400; break;
        case 403: status_text = STATUS_403; break;
        case 404: status_text = STATUS_404; break;
        case 500: status_text = STATUS_500; break;
        default: status_text = "";
      }

      std::vector<char> status_buf(4096);
      tinyfcgi::message status_msg(1, status_buf.data(), status_buf.size());
      const std::string status_line = std::to_string(status) + " ";
      status_msg.append(FCGI_STDOUT, STATUS_HEADER)
        .append(FCGI_STDOUT, status_line)
        .append(FCGI_STDOUT, status_text.empty() ? String::SubString(status_line) : String::SubString(status_text))
        .append(FCGI_STDOUT, CRLF)
        .clear_padding();
      auto status_chunk = status_msg.str();
      result.emplace_back(status_chunk.data(), status_chunk.size());

      std::vector<char> header_buf(32 * 1024);
      tinyfcgi::message headers_msg(1, header_buf.data(), header_buf.size());
      for(const auto& header : response->headers())
      {
        headers_msg.append(FCGI_STDOUT, header.name)
          .append(FCGI_STDOUT, HEADER_SEPARATOR)
          .append(FCGI_STDOUT, header.value)
          .append(FCGI_STDOUT, CRLF);
      }
      for(const auto& cookie : response->cookies())
      {
        headers_msg.append(FCGI_STDOUT, SET_COOKIE_HEADER)
          .append(FCGI_STDOUT, cookie)
          .append(FCGI_STDOUT, CRLF);
      }
      const std::string content_length = std::to_string(response->body().size());
      headers_msg.append(FCGI_STDOUT, CONTENT_LENGTH_HEADER)
        .append(FCGI_STDOUT, content_length)
        .append(FCGI_STDOUT, CRLF)
        .append(FCGI_STDOUT, CRLF)
        .clear_padding();
      auto header_chunk = headers_msg.str();
      result.emplace_back(header_chunk.data(), header_chunk.size());

      const std::string& body = response->body();
      size_t offset = 0;
      while(offset < body.size())
      {
        std::vector<char> body_buf(64 * 1024);
        tinyfcgi::message body_msg(1, body_buf.data(), body_buf.size());
        const size_t chunk_size = std::min(
          body.size() - offset,
          body_msg.capacity() - body_msg.size());
        body_msg.append(FCGI_STDOUT, String::SubString(body.data() + offset, chunk_size))
          .clear_padding();
        auto body_chunk = body_msg.str();
        result.emplace_back(body_chunk.data(), body_chunk.size());
        offset += chunk_size;
      }

      std::vector<char> stdout_end_buf(256);
      tinyfcgi::message stdout_end_msg(
        1,
        stdout_end_buf.data(),
        stdout_end_buf.size());
      stdout_end_msg.end_stream(FCGI_STDOUT);
      auto stdout_end_chunk = stdout_end_msg.str();
      result.emplace_back(stdout_end_chunk.data(), stdout_end_chunk.size());

      std::vector<char> end_buf(256);
      tinyfcgi::message end_msg(1, end_buf.data(), end_buf.size());
      end_msg.end_request(0, FCGI_REQUEST_COMPLETE);
      auto end_chunk = end_msg.str();
      result.emplace_back(end_chunk.data(), end_chunk.size());

      return result;
    }

    FCGIAcceptor::Connection_var conn_;
    Generics::AtomicInt sent_response_;
  };

  // State implementation
  FCGIAcceptor::State::State(
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

  // FCGIAcceptor::Connection::SendBuf
  FCGIAcceptor::Connection::SendBuf::SendBuf(SendBuf&& init)
  {
    //wbuf.swap(init.wbuf);
    response.swap(init.response);
    response_chunks.swap(init.response_chunks);
    bufs.swap(init.bufs);
  }

  // FCGIAcceptor::Connection implementation
  FCGIAcceptor::Connection::Connection(
    boost::asio::io_service& io_service,
    Logging::Logger* logger,
    FrontendCommons::FrontendInterface* frontend,
    State* /*state*/,
    FCGIAcceptorStats* stats)
    noexcept
    : logger_(ReferenceCounting::add_ref(logger)),
      frontend_(ReferenceCounting::add_ref(frontend)),
      stats_(ReferenceCounting::add_ref(stats)),
      io_service_(io_service),
      //strand_(new boost::asio::io_context::strand(*io_service_)),
      socket_(io_service_),
      active_(0),
      read_ordered_(0),
      write_ordered_(0)
  {
    rbuf_.resize(READ_BUF_SIZE_);
  }

  FCGIAcceptor::Connection::~Connection() noexcept
  {
    deactivate();
    //std::cerr << "FCGIAcceptor::Connection::~Connection()" << std::endl;
  }

  FCGIAcceptor::Connection::SocketType&
  FCGIAcceptor::Connection::socket() noexcept
  {
    return socket_;
  }

  void
  FCGIAcceptor::Connection::activate()
  {
    if(active_.exchange(1) == 0 && stats_.in())
    {
      stats_->add_fcgi_connection();
    }
    order_read_();
  }

  void
  FCGIAcceptor::Connection::deactivate()
  {
    if(active_.exchange(0) != 0)
    {
      socket_.close();
      if(stats_.in())
      {
        stats_->complete_fcgi_connection();
      }
    }
  }

  void
  FCGIAcceptor::Connection::handle_read_(
    const boost::system::error_code& error,
    size_t bytes_transferred)
  {
    --read_ordered_;

    if(!error)
    {
      // process got buffer
      if(!process_read_data_(bytes_transferred))
      {
        deactivate();
        return;
      }

      order_write_();
      order_read_();
    }
    else
    {
      // destroy & close on ref count == 0
      deactivate();
    }
  }

  void
  FCGIAcceptor::Connection::handle_write_(const boost::system::error_code& error)
  {
    --write_ordered_;

    if(!error)
    {
      // writing done
      order_write_();
    }
    else
    {
      // destroy & close on ref count == 0
      deactivate();
    }
  }

  void
  FCGIAcceptor::Connection::order_read_()
  {
    if(++read_ordered_ == 1)
    {
      auto self = shared_from_this();
      socket_.async_read_some(
        boost::asio::buffer(&rbuf_[0], READ_BUF_SIZE_),
	[self](const boost::system::error_code& error, size_t bytes_transferred)
	{
	  self->handle_read_(error, bytes_transferred);
	});
    }
    else
    {
      --read_ordered_;
    }
  }

  void
  FCGIAcceptor::Connection::order_write_()
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
	auto self = shared_from_this();

        if(ordered_send_bufs_.size() > 1)
        {
          std::vector<boost::asio::const_buffer> buffer_seq;
          for(auto buf_it = ordered_send_bufs_.begin(); buf_it != ordered_send_bufs_.end(); ++buf_it)
          {
            buffer_seq.insert(
              buffer_seq.end(),
              (*buf_it)->bufs.begin(),
              (*buf_it)->bufs.end());
          }

          boost::asio::async_write(
            socket_,
            buffer_seq,
	    [self](const boost::system::error_code& error, size_t /*bytes_transferred*/)
	    {
	      self->handle_write_(error);
	    });
        }
        else
        {
          boost::asio::async_write(
            socket_,
            (*ordered_send_bufs_.begin())->bufs,
	    [self](const boost::system::error_code& error, size_t /*bytes_transferred*/)
	    {
	      self->handle_write_(error);
	    });
          
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
  FCGIAcceptor::Connection::process_read_data_(size_t bytes_transferred)
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
      FCGI::BaseHttpResponseWriter_var response_writer(
        new FCGIResponseWriter(shared_from_this()));

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
  FCGIAcceptor::Connection::send_response(
    FCGI::HttpResponse_var response_ptr,
    std::vector<std::string>&& response_chunks)
    noexcept
  {
    FCGI::HttpResponse_var response(std::move(response_ptr));

    std::unique_ptr<SendBuf> send_buf(new SendBuf());
    send_buf->response.swap(response);
    send_buf->response_chunks = std::move(response_chunks);
    send_buf->bufs.reserve(send_buf->response_chunks.size());
    for(const auto& chunk : send_buf->response_chunks)
    {
      send_buf->bufs.push_back(boost::asio::const_buffer(chunk.data(), chunk.size()));
    }

    {
      WriteBufSyncPolicy::WriteGuard lock(send_bufs_lock_);
      send_bufs_.emplace_back(std::move(send_buf));
    }

    order_write_();
  }

  Logging::Logger*
  FCGIAcceptor::Connection::logger_i_() noexcept
  {
    return logger_;
  }

  // FCGIAcceptor implementation
  FCGIAcceptor::FCGIAcceptor(
    Logging::Logger* logger,
    FrontendCommons::FrontendInterface* frontend,
    Generics::ActiveObjectCallback* callback,
    FCGIAcceptorStats* stats,
    const String::SubString& bind_address,
    unsigned long backlog,
    unsigned long process_threads)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      frontend_(ReferenceCounting::add_ref(frontend)),
      stats_(ReferenceCounting::add_ref(stats)),
      worker_stats_object_(new WorkerStatsObject(
        logger,
        callback)),
      state_(new FCGIAcceptor::State(logger, frontend, worker_stats_object_.in())),
      io_service_(std::make_shared<boost::asio::io_service>()),
      bind_address_(bind_address.str()),
      backlog_(backlog)
  {
    add_child_object(Generics::ActiveObject_var(
      new AdServer::Commons::BoostAsioContextRunActiveObject(
        callback,
        io_service_,
        process_threads,
        128 * 1024,
        "fcgi-accept")));
  }

  FCGIAcceptor::FCGIAcceptor(
    Logging::Logger* logger,
    FrontendCommons::FrontendInterface* frontend,
    Generics::ActiveObjectCallback* callback,
    const String::SubString& bind_address,
    unsigned long backlog,
    unsigned long process_threads)
    /*throw(eh::Exception)*/
    : FCGIAcceptor(
        logger,
        frontend,
        callback,
        nullptr,
        bind_address,
        backlog,
        process_threads)
  {
  }

  FCGIAcceptor::~FCGIAcceptor() noexcept
  {}

  void
  FCGIAcceptor::activate_object()
    /*throw(Exception, eh::Exception)*/
  {
    ::unlink(bind_address_.c_str());

    acceptor_ = std::make_shared<AcceptorType>(*io_service_);
    acceptor_->open(boost::asio::local::stream_protocol());
    acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_->bind(boost::asio::local::stream_protocol::endpoint(bind_address_));
    acceptor_->listen(backlog_);

    create_accept_stub_();

    Generics::CompositeActiveObject::activate_object();

    worker_stats_object_->activate_object();
  }

  void
  FCGIAcceptor::deactivate_object()
    /*throw(Exception, eh::Exception)*/
  {
    if(acceptor_)
    {
      boost::system::error_code ec;
      acceptor_->cancel(ec);
      acceptor_->close(ec);
    }

    Generics::CompositeActiveObject::deactivate_object();
  }

  void
  FCGIAcceptor::wait_object()
    /*throw(Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::wait_object();

    // Deactivate worker stats
    worker_stats_object_->deactivate_object();
    worker_stats_object_->wait_object();
  }

  void
  FCGIAcceptor::create_accept_stub_()
  {
    if(!acceptor_ || !acceptor_->is_open())
    {
      return;
    }

    // create stub for new connection
    Connection_var new_connection(
      new Connection(*io_service_, logger_, frontend_, state_, stats_));
    acceptor_->async_accept(
      new_connection->socket(),
      [this, new_connection](const boost::system::error_code& error)
      {
	handle_accept_(new_connection, error);
      });
  }

  void
  FCGIAcceptor::handle_accept_(
    const Connection_var& accepted_connection,
    const boost::system::error_code& error)
  {
    if(!error)
    {
      if(stats_.in())
      {
        stats_->add_fcgi_accept();
      }

      // activate stub as normal connection
      accepted_connection->activate();
    }
    else
    {
      // descriptors limit reached ? don't stop
      std::cerr << "Can't accept connection: " << error << std::endl;
    }

    if(acceptor_ && acceptor_->is_open())
    {
      create_accept_stub_();
    }
  }

  FrontendCommons::FrontendInterface*
  FCGIAcceptor::handler() noexcept
  {
    return frontend_.in();
  }

  Logging::Logger*
  FCGIAcceptor::logger() noexcept
  {
    return logger_.in();
  }
}
}
