#include <algorithm>
#include <charconv>
#include <iostream>
#include <limits>
#include <string_view>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/io_service.hpp>

#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>

#include "FCGIAcceptor.hpp"

namespace
{
  constexpr std::size_t FCGI_RESPONSE_BUFFER_SIZE = 128 * 1024;

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

  constexpr std::string_view CRLF = "\r\n";
  constexpr std::string_view STATUS_HEADER = "Status: ";
  constexpr std::string_view HEADER_SEPARATOR = ": ";
  constexpr std::string_view SET_COOKIE_HEADER = "Set-Cookie: ";
  constexpr std::string_view CONTENT_LENGTH_HEADER = "Content-Length: ";
  constexpr std::string_view STATUS_200 = "OK";
  constexpr std::string_view STATUS_204 = "No Content";
  constexpr std::string_view STATUS_301 = "Moved Permanently";
  constexpr std::string_view STATUS_302 = "Found";
  constexpr std::string_view STATUS_303 = "See Other";
  constexpr std::string_view STATUS_307 = "Temporary Redirect";
  constexpr std::string_view STATUS_400 = "Bad Request";
  constexpr std::string_view STATUS_403 = "Forbidden";
  constexpr std::string_view STATUS_404 = "Not Found";
  constexpr std::string_view STATUS_500 = "Internal Server Error";

  std::string_view
  status_text_ref(int status) noexcept
  {
    switch (status)
    {
      case 200: return STATUS_200;
      case 204: return STATUS_204;
      case 301: return STATUS_301;
      case 302: return STATUS_302;
      case 303: return STATUS_303;
      case 307: return STATUS_307;
      case 400: return STATUS_400;
      case 403: return STATUS_403;
      case 404: return STATUS_404;
      case 500: return STATUS_500;
    }

    return std::string_view();
  }

  template<typename Number>
  std::string_view
  to_chars_ref(char* buf, std::size_t size, Number value) noexcept
  {
    const std::to_chars_result result = std::to_chars(buf, buf + size, value);
    return std::string_view(buf, result.ptr - buf);
  }

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
    deactivate_i_();

    void
    handle_read_(
      const boost::system::error_code& error,
      size_t bytes_transferred);

    void
    handle_write_(const boost::system::error_code& error);

    void
    send_response(std::unique_ptr<char[]>&& response_buf, std::size_t response_size)
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

      std::unique_ptr<char[]> response_buf;
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
    boost::asio::io_service::strand strand_;

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
        auto prepared_response = make_fcgi_response_(response_ptr);
        conn_->send_response(
          std::move(prepared_response.response_buf),
          prepared_response.response_size);
      }
    }

  private:
    struct PreparedResponse
    {
      std::unique_ptr<char[]> response_buf;
      std::size_t response_size = 0;
    };

    static std::size_t
    fcgi_record_size_(std::size_t payload_size) noexcept
    {
      return
        sizeof(FCGI_Header) +
        payload_size +
        ((8 - (payload_size % 8)) % 8);
    }

    static PreparedResponse
    make_fcgi_response_(const FCGI::HttpResponse_var& response)
    {
      const int status = response->status() == 0 ? 200 : response->status();
      char status_line_buf[std::numeric_limits<int>::digits10 + 3];
      std::string_view status_line = to_chars_ref(
        status_line_buf,
        sizeof(status_line_buf) - 1,
        status);
      status_line_buf[status_line.size()] = ' ';
      status_line = std::string_view(status_line_buf, status_line.size() + 1);

      const std::string_view status_text = status_text_ref(status);
      const std::string_view status_text_out =
        status_text.empty() ? status_line : status_text;

      std::size_t status_payload_size =
        STATUS_HEADER.size() +
        status_line.size() +
        status_text_out.size() +
        CRLF.size();

      std::size_t headers_payload_size = 0;
      for(const auto& header : response->headers())
      {
        headers_payload_size +=
          header.name.size() +
          HEADER_SEPARATOR.size() +
          header.value.size() +
          CRLF.size();
      }

      for(const auto& cookie : response->cookies())
      {
        headers_payload_size +=
          SET_COOKIE_HEADER.size() +
          cookie.size() +
          CRLF.size();
      }
      char content_length_buf[std::numeric_limits<std::size_t>::digits10 + 2];
      const std::string_view content_length = to_chars_ref(
        content_length_buf,
        sizeof(content_length_buf),
        response->body().size());
      headers_payload_size +=
        CONTENT_LENGTH_HEADER.size() +
        content_length.size() +
        CRLF.size() +
        CRLF.size();

      const std::string& body = response->body();
      std::size_t total_response_size =
        fcgi_record_size_(status_payload_size) +
        fcgi_record_size_(headers_payload_size) +
        fcgi_record_size_(0) +
        fcgi_record_size_(sizeof(FCGI_EndRequestBody));

      for(std::size_t body_offset = 0; body_offset < body.size();)
      {
        const std::size_t chunk_size = std::min<std::size_t>(
          body.size() - body_offset,
          std::numeric_limits<std::uint16_t>::max());
        total_response_size += fcgi_record_size_(chunk_size);
        body_offset += chunk_size;
      }

      PreparedResponse result;
      const std::size_t response_buf_size =
        std::max<std::size_t>(FCGI_RESPONSE_BUFFER_SIZE, total_response_size);
      result.response_buf.reset(new char[response_buf_size]);

      std::size_t offset = 0;
      {
        tinyfcgi::message status_msg(
          1,
          result.response_buf.get() + offset,
          response_buf_size - offset);
        status_msg.append(FCGI_STDOUT, STATUS_HEADER)
          .append(FCGI_STDOUT, status_line)
          .append(FCGI_STDOUT, status_text_out)
          .append(FCGI_STDOUT, CRLF)
          .clear_padding();
        offset += status_msg.size();
      }

      {
        tinyfcgi::message headers_msg(
          1,
          result.response_buf.get() + offset,
          response_buf_size - offset);
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
            .append(FCGI_STDOUT, std::string_view(cookie))
            .append(FCGI_STDOUT, CRLF);
        }
        headers_msg.append(FCGI_STDOUT, CONTENT_LENGTH_HEADER)
          .append(FCGI_STDOUT, content_length)
          .append(FCGI_STDOUT, CRLF)
          .append(FCGI_STDOUT, CRLF)
          .clear_padding();
        offset += headers_msg.size();
      }

      for(std::size_t body_offset = 0; body_offset < body.size();)
      {
        tinyfcgi::message body_msg(
          1,
          result.response_buf.get() + offset,
          response_buf_size - offset);
        const std::size_t chunk_size = std::min<std::size_t>(
          body.size() - body_offset,
          std::numeric_limits<std::uint16_t>::max());
        body_msg.append(
          FCGI_STDOUT,
          String::SubString(body.data() + body_offset, chunk_size))
          .clear_padding();
        offset += body_msg.size();
        body_offset += chunk_size;
      }

      {
        tinyfcgi::message stdout_end_msg(
          1,
          result.response_buf.get() + offset,
          response_buf_size - offset);
        stdout_end_msg.end_stream(FCGI_STDOUT);
        offset += stdout_end_msg.size();
      }

      {
        tinyfcgi::message end_msg(
          1,
          result.response_buf.get() + offset,
          response_buf_size - offset);
        end_msg.end_request(0, FCGI_REQUEST_COMPLETE);
        offset += end_msg.size();
      }

      result.response_size = offset;
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
    response_buf.swap(init.response_buf);
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
      strand_(io_service),
      socket_(io_service_),
      active_(0),
      read_ordered_(0),
      write_ordered_(0)
  {
    rbuf_.resize(READ_BUF_SIZE_);
  }

  FCGIAcceptor::Connection::~Connection() noexcept
  {
    if(active_.exchange(0) != 0)
    {
      boost::system::error_code ignored_error;
      socket_.close(ignored_error);
      if(stats_.in())
      {
        stats_->complete_fcgi_connection();
      }
    }
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
    auto self = shared_from_this();
    strand_.post(
      [self]()
      {
        if(self->active_.exchange(1) == 0 && self->stats_.in())
        {
          self->stats_->add_fcgi_connection();
        }
        self->order_read_();
      }
    );
  }

  void
  FCGIAcceptor::Connection::deactivate()
  {
    auto self = shared_from_this();
    strand_.post(
      [self]()
      {
        self->deactivate_i_();
      }
    );
  }

  void
  FCGIAcceptor::Connection::deactivate_i_()
  {
    if(active_.exchange(0) != 0)
    {
      boost::system::error_code ignored_error;
      socket_.close(ignored_error);
      {
        WriteBufSyncPolicy::WriteGuard lock(send_bufs_lock_);
        SendBufPtrArray().swap(send_bufs_);
      }
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
        deactivate_i_();
        return;
      }

      order_write_();
      order_read_();
    }
    else
    {
      // destroy & close on ref count == 0
      deactivate_i_();
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
      deactivate_i_();
    }
  }

  void
  FCGIAcceptor::Connection::order_read_()
  {
    if(++read_ordered_ == 1)
    {
      if(active_.load(std::memory_order_acquire) == 0)
      {
        --read_ordered_;
        return;
      }

      auto self = shared_from_this();
      socket_.async_read_some(
        boost::asio::buffer(&rbuf_[0], READ_BUF_SIZE_),
        boost::asio::bind_executor(
          strand_,
          [self](
            const boost::system::error_code& error,
            size_t bytes_transferred)
          {
            self->handle_read_(error, bytes_transferred);
          }));
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
      if(active_.load(std::memory_order_acquire) == 0)
      {
        {
          WriteBufSyncPolicy::WriteGuard lock(send_bufs_lock_);
          SendBufPtrArray().swap(send_bufs_);
        }
        --write_ordered_;
        return;
      }

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
            boost::asio::bind_executor(
              strand_,
              [self](
                const boost::system::error_code& error,
                size_t /*bytes_transferred*/)
              {
                self->handle_write_(error);
              }));
        }
        else
        {
          boost::asio::async_write(
            socket_,
            (*ordered_send_bufs_.begin())->bufs,
            boost::asio::bind_executor(
              strand_,
              [self](
                const boost::system::error_code& error,
                size_t /*bytes_transferred*/)
              {
                self->handle_write_(error);
              }));
          
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
      return false;
    }

    return true;
  }

  void
  FCGIAcceptor::Connection::send_response(
    std::unique_ptr<char[]>&& response_buf,
    std::size_t response_size)
    noexcept
  {
    std::unique_ptr<SendBuf> send_buf(new SendBuf());
    send_buf->response_buf = std::move(response_buf);
    send_buf->bufs.emplace_back(send_buf->response_buf.get(), response_size);

    {
      WriteBufSyncPolicy::WriteGuard lock(send_bufs_lock_);
      send_bufs_.emplace_back(std::move(send_buf));
    }

    auto self = shared_from_this();
    strand_.post(
      [self]()
      {
        self->order_write_();
      }
    );
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
