#include "Http2Acceptor.hpp"
#include "BoostAsioContextRunActiveObject.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include <Stream/MemoryStream.hpp>
#include <String/SubString.hpp>

namespace
{
  const char ASPECT[] = "Http2Acceptor";
  const char RESPONSE_BODY[] = "FCGIServer HTTP/2 endpoint is enabled\n";
  const char HOST_HEADER[] = "HOST";
  const char CONTENT_TYPE_HEADER[] = "Content-Type";
  const char TEXT_PLAIN[] = "text/plain";
  const String::SubString INVALID_HTTP2_PREFACE("Invalid HTTP/2 client preface");

  const uint8_t HTTP2_PREFACE[] = {
    'P','R','I',' ','*',' ','H','T','T','P','/','2','.','0','\r','\n','\r','\n','S','M','\r','\n','\r','\n'
  };
}

namespace AdServer
{
namespace Frontends
{
  class Http2Response final
  {
  public:
    int status = 200;
    std::string content_type = "text/plain";
    std::string body = RESPONSE_BODY;
  };

  struct Http2Acceptor::Connection::StreamData final
  {
    std::string method;
    std::string path;
    std::string authority;
    bool response_sent = false;

    Http2Response response;
  };

  class Http2Acceptor::Http2ResponseWriter final: public FCGI::BaseHttpResponseWriter
  {
  public:
    explicit Http2ResponseWriter(
      const std::weak_ptr<Http2Acceptor::Connection>& connection,
      int32_t stream_id)
      : connection_(connection),
        stream_id_(stream_id)
    {}

    void write(FCGI::HttpResponse_var response) override
    {
      auto connection = connection_.lock();
      if(!connection)
      {
        return;
      }

      connection->on_backend_response_(stream_id_, std::move(response));
    }

  protected:
    ~Http2ResponseWriter() noexcept override = default;

  private:
    std::weak_ptr<Http2Acceptor::Connection> connection_;
    int32_t stream_id_;
  };

  Http2Acceptor::Connection::Connection(
    Http2Acceptor* owner,
    std::shared_ptr<boost::asio::io_service> io_service,
    unsigned long max_concurrent_streams,
    unsigned long read_buffer_size)
    : owner_(owner),
      io_service_(std::move(io_service)),
      socket_(*io_service_),
      session_(nullptr),
      read_buf_(read_buffer_size ? read_buffer_size : 64 * 1024),
      preface_received_(0),
      write_active_(false),
      close_started_(false),
      max_concurrent_streams_(max_concurrent_streams)
  {
    std::memset(read_buf_.data(), 0, read_buf_.size());
  }

  Http2Acceptor::Connection::~Connection() noexcept
  {
    if(session_)
    {
      nghttp2_session_del(session_);
    }
  }

  Http2Acceptor::Connection::SocketType&
  Http2Acceptor::Connection::socket() noexcept
  {
    return socket_;
  }

  void
  Http2Acceptor::Connection::activate()
  {
    if(!init_http2_())
    {
      close_();
      return;
    }

    submit_settings_();
    order_read_();
    flush_send_queue_();
  }

  void
  Http2Acceptor::Connection::deactivate()
  {
    auto self = shared_from_this();
    io_service_->post([self]()
    {
      self->close_();
    });
  }

  ssize_t
  Http2Acceptor::Connection::send_callback_(
    nghttp2_session* /*session*/,
    const uint8_t* data,
    size_t length,
    int /*flags*/,
    void* user_data)
  {
    auto* self = static_cast<Connection*>(user_data);
    self->send_queue_.emplace_back(
      reinterpret_cast<const char*>(data),
      reinterpret_cast<const char*>(data) + length);
    return static_cast<ssize_t>(length);
  }

  int
  Http2Acceptor::Connection::on_begin_headers_callback_(
    nghttp2_session* session,
    const nghttp2_frame* frame,
    void* user_data)
  {
    if(frame->hd.type == NGHTTP2_HEADERS && frame->headers.cat == NGHTTP2_HCAT_REQUEST)
    {
      auto data = std::make_shared<StreamData>();
      Connection* self = static_cast<Connection*>(user_data);
      if(self)
      {
        self->streams_.emplace(frame->hd.stream_id, data);
      }
      nghttp2_session_set_stream_user_data(session, frame->hd.stream_id, data.get());
    }
    return 0;
  }

  int
  Http2Acceptor::Connection::on_header_callback_(
    nghttp2_session* session,
    const nghttp2_frame* frame,
    const uint8_t* name,
    size_t namelen,
    const uint8_t* value,
    size_t valuelen,
    uint8_t /*flags*/,
    void* /*user_data*/)
  {
    if(frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST)
    {
      return 0;
    }

    auto* stream_data = static_cast<StreamData*>(
      nghttp2_session_get_stream_user_data(session, frame->hd.stream_id));

    if(!stream_data)
    {
      return 0;
    }

    if(namelen == 7 && std::memcmp(name, ":method", 7) == 0)
    {
      stream_data->method.assign(reinterpret_cast<const char*>(value), valuelen);
    }
    else if(namelen == 5 && std::memcmp(name, ":path", 5) == 0)
    {
      stream_data->path.assign(reinterpret_cast<const char*>(value), valuelen);
    }
    else if(namelen == 10 && std::memcmp(name, ":authority", 10) == 0)
    {
      stream_data->authority.assign(reinterpret_cast<const char*>(value), valuelen);
    }

    return 0;
  }

  ssize_t
  Http2Acceptor::Connection::data_source_read_callback_(
    nghttp2_session* /*session*/,
    int32_t /*stream_id*/,
    uint8_t* buf,
    size_t length,
    uint32_t* data_flags,
    nghttp2_data_source* source,
    void* /*user_data*/)
  {
    const auto* body = static_cast<const std::string*>(source->ptr);
    const size_t to_copy = std::min(length, body->size());
    std::memcpy(buf, body->data(), to_copy);
    *data_flags = NGHTTP2_DATA_FLAG_EOF;
    return static_cast<ssize_t>(to_copy);
  }

  int
  Http2Acceptor::Connection::on_frame_recv_callback_(
    nghttp2_session* session,
    const nghttp2_frame* frame,
    void* user_data)
  {
    auto* self = static_cast<Connection*>(user_data);

    if((frame->hd.type == NGHTTP2_HEADERS && frame->headers.cat == NGHTTP2_HCAT_REQUEST) ||
       frame->hd.type == NGHTTP2_DATA)
    {
      auto* stream_data = static_cast<StreamData*>(
        nghttp2_session_get_stream_user_data(session, frame->hd.stream_id));

      if(stream_data && !stream_data->response_sent &&
        (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) != 0)
      {
        self->process_request_(frame->hd.stream_id, *stream_data);
        stream_data->response_sent = true;
      }
    }

    return 0;
  }

  int
  Http2Acceptor::Connection::on_stream_close_callback_(
    nghttp2_session* /*session*/,
    int32_t stream_id,
    uint32_t /*error_code*/,
    void* user_data)
  {
    Connection* self = static_cast<Connection*>(user_data);
    if(self)
    {
      self->streams_.erase(stream_id);
    }
    return 0;
  }

  bool
  Http2Acceptor::Connection::init_http2_()
  {
    nghttp2_session_callbacks* callbacks = nullptr;

    if(nghttp2_session_callbacks_new(&callbacks) != 0)
    {
      owner_->logger_i_()->log(
        String::SubString("Can't allocate nghttp2 session callbacks"),
        Logging::Logger::ERROR,
        ASPECT);
      return false;
    }

    nghttp2_session_callbacks_set_send_callback(callbacks, send_callback_);
    nghttp2_session_callbacks_set_on_begin_headers_callback(callbacks, on_begin_headers_callback_);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback_);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback_);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback_);

    const int res = nghttp2_session_server_new(&session_, callbacks, this);
    nghttp2_session_callbacks_del(callbacks);

    if(res != 0)
    {
      Stream::Error ostr;
      ostr << "Can't initialize nghttp2 session: " << nghttp2_strerror(res);
      owner_->logger_i_()->log(ostr.str(), Logging::Logger::ERROR, ASPECT);
      return false;
    }

    return true;
  }

  void
  Http2Acceptor::Connection::submit_settings_()
  {
    std::array<nghttp2_settings_entry, 2> settings = {{
      {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, static_cast<uint32_t>(max_concurrent_streams_)},
      {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 1 << 20}
    }};

    const int res = nghttp2_submit_settings(
      session_,
      NGHTTP2_FLAG_NONE,
      settings.data(),
      settings.size());

    if(res != 0)
    {
      Stream::Error ostr;
      ostr << "Can't submit HTTP/2 settings: " << nghttp2_strerror(res);
      owner_->logger_i_()->log(ostr.str(), Logging::Logger::ERROR, ASPECT);
    }
  }

  void
  Http2Acceptor::Connection::process_request_(int32_t stream_id, StreamData& stream_data)
  {
    std::string uri = stream_data.path.empty() ? "/" : stream_data.path;
    std::string query;

    const auto pos = uri.find('?');
    if(pos != std::string::npos)
    {
      query = uri.substr(pos + 1);
      uri.resize(pos);
    }

    FCGI::HttpRequestHolder_var request_holder(new FCGI::HttpRequestHolder());
    auto& request = request_holder->request();

    request.set_method(
      stream_data.method == "POST" ?
        FCGI::HttpRequest::RM_POST :
        FCGI::HttpRequest::RM_GET);

    request.set_uri(String::SubString(uri));
    request.set_args(String::SubString(query));

    HTTP::SubHeaderList headers;
    if(!stream_data.authority.empty())
    {
      headers.push_back(
        HTTP::SubHeader(
          String::SubString(HOST_HEADER),
          String::SubString(stream_data.authority)));
    }
    request.set_headers(std::move(headers));

    FCGI::BaseHttpResponseWriter_var response_writer(
      new Http2ResponseWriter(shared_from_this(), stream_id));

    try
    {
      owner_->frontend_i_()->handle_request(request_holder, response_writer);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "HTTP/2 delegated request failed: " << ex.what();
      owner_->logger_i_()->log(ostr.str(), Logging::Logger::ERROR, ASPECT);
    }
  }

  void
  Http2Acceptor::Connection::submit_response_(int32_t stream_id, const Http2Response& response)
  {
    const std::string status_text = std::to_string(response.status);

    std::array<nghttp2_nv, 2> headers = {{
      {
        reinterpret_cast<uint8_t*>(const_cast<char*>(":status")),
        reinterpret_cast<uint8_t*>(const_cast<char*>(status_text.data())),
        sizeof(":status") - 1,
        status_text.size(),
        NGHTTP2_NV_FLAG_NONE
      },
      {
        reinterpret_cast<uint8_t*>(const_cast<char*>("content-type")),
        reinterpret_cast<uint8_t*>(const_cast<char*>(response.content_type.data())),
        sizeof("content-type") - 1,
        response.content_type.size(),
        NGHTTP2_NV_FLAG_NONE
      }
    }};

    nghttp2_data_provider provider;
    provider.source.ptr = const_cast<std::string*>(&response.body);
    provider.read_callback = data_source_read_callback_;

    const int res = nghttp2_submit_response(
      session_,
      stream_id,
      headers.data(),
      headers.size(),
      &provider);

    if(res != 0)
    {
      Stream::Error ostr;
      ostr << "Can't submit HTTP/2 response: " << nghttp2_strerror(res);
      owner_->logger_i_()->log(ostr.str(), Logging::Logger::ERROR, ASPECT);
    }
  }


  void
  Http2Acceptor::Connection::on_backend_response_(
    int32_t stream_id,
    FCGI::HttpResponse_var response)
  {
    auto self = shared_from_this();
    io_service_->post([self, stream_id, response]()
    {
      auto it = self->streams_.find(stream_id);
      if(it == self->streams_.end())
      {
        return;
      }

      const auto& stream_data = it->second;

      // map generic HttpResponse to Http2Response
      stream_data->response.status = response->status();
      stream_data->response.content_type = TEXT_PLAIN;
      for(const auto& header : response->headers())
      {
        if(header.name == String::SubString(CONTENT_TYPE_HEADER))
        {
          stream_data->response.content_type.assign(
            header.value.data(),
            header.value.size());
        }
      }
      stream_data->response.body = response->body();

      self->submit_response_(stream_id, stream_data->response);
      self->flush_send_queue_();
    });
  }

  void
  Http2Acceptor::Connection::order_read_()
  {
    auto self = shared_from_this();
    socket_.async_read_some(
      boost::asio::buffer(read_buf_.data(), read_buf_.size()),
      [self](const boost::system::error_code& error, size_t bytes_transferred)
      {
        self->handle_read_(error, bytes_transferred);
      });
  }

  void
  Http2Acceptor::Connection::handle_read_(
    const boost::system::error_code& error,
    size_t bytes_transferred)
  {
    if(error)
    {
      close_();
      return;
    }

    if(preface_received_ < sizeof(HTTP2_PREFACE))
    {
      const size_t need = sizeof(HTTP2_PREFACE) - preface_received_;
      const size_t take = std::min(need, bytes_transferred);

      if(std::memcmp(
        HTTP2_PREFACE + preface_received_,
        read_buf_.data(),
        take) != 0)
      {
        owner_->logger_i_()->log(
          INVALID_HTTP2_PREFACE,
          Logging::Logger::ERROR,
          ASPECT);
        close_();
        return;
      }

      preface_received_ += take;

      if(bytes_transferred > take)
      {
        process_http2_data_(read_buf_.data() + take, bytes_transferred - take);
      }
    }
    else
    {
      process_http2_data_(read_buf_.data(), bytes_transferred);
    }

    flush_send_queue_();
    order_read_();
  }

  void
  Http2Acceptor::Connection::process_http2_data_(const char* data, size_t size)
  {
    if(!session_ || size == 0)
    {
      return;
    }

    const ssize_t rv = nghttp2_session_mem_recv(
      session_,
      reinterpret_cast<const uint8_t*>(data),
      size);

    if(rv < 0)
    {
      Stream::Error ostr;
      ostr << "HTTP/2 frame parse error: " << nghttp2_strerror(static_cast<int>(rv));
      owner_->logger_i_()->log(ostr.str(), Logging::Logger::ERROR, ASPECT);
      close_();
      return;
    }
  }

  void
  Http2Acceptor::Connection::flush_send_queue_()
  {
    if(!session_)
    {
      return;
    }

    const int rv = nghttp2_session_send(session_);
    if(rv != 0)
    {
      Stream::Error ostr;
      ostr << "HTTP/2 send error: " << nghttp2_strerror(rv);
      owner_->logger_i_()->log(ostr.str(), Logging::Logger::ERROR, ASPECT);
      close_();
      return;
    }

    if(!write_active_ && !send_queue_.empty())
    {
      write_active_ = true;
      order_write_();
    }
  }

  void
  Http2Acceptor::Connection::order_write_()
  {
    if(send_queue_.empty())
    {
      write_active_ = false;
      return;
    }

    auto self = shared_from_this();
    boost::asio::async_write(
      socket_,
      boost::asio::buffer(send_queue_.front()),
      [self](const boost::system::error_code& error, size_t /*bytes_transferred*/)
      {
        self->handle_write_(error);
      });
  }

  void
  Http2Acceptor::Connection::handle_write_(const boost::system::error_code& error)
  {
    if(error)
    {
      close_();
      return;
    }

    if(!send_queue_.empty())
    {
      send_queue_.pop_front();
    }

    if(send_queue_.empty())
    {
      write_active_ = false;
      return;
    }

    order_write_();
  }

  void
  Http2Acceptor::Connection::close_()
  {
    std::lock_guard<std::mutex> lock(close_lock_);

    if(close_started_)
    {
      return;
    }

    close_started_ = true;

    boost::system::error_code ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
    owner_->erase_connection_(this);
  }

  Http2Acceptor::Http2Acceptor(
    Logging::Logger* logger,
    FrontendCommons::FrontendInterface* frontend,
    const String::SubString& bind_address,
    unsigned long port,
    unsigned long threads,
    unsigned long max_concurrent_streams,
    unsigned long read_buffer_size)
    : logger_(ReferenceCounting::add_ref(logger)),
      frontend_(ReferenceCounting::add_ref(frontend)),
      bind_address_(bind_address.str()),
      port_(port),
      threads_(threads ? threads : 1),
      max_concurrent_streams_(max_concurrent_streams ? max_concurrent_streams : 100),
      read_buffer_size_(read_buffer_size ? read_buffer_size : 64 * 1024),
      use_unix_socket_(false),
      io_service_(std::make_shared<boost::asio::io_service>()),
      io_work_(new boost::asio::io_service::work(*io_service_)),
      acceptor_(std::make_shared<boost::asio::ip::tcp::acceptor>(*io_service_))
  {}

  Http2Acceptor::Http2Acceptor(
    Logging::Logger* logger,
    FrontendCommons::FrontendInterface* frontend,
    const String::SubString& unix_socket_path,
    unsigned long threads,
    unsigned long max_concurrent_streams,
    unsigned long read_buffer_size)
    : logger_(ReferenceCounting::add_ref(logger)),
      frontend_(ReferenceCounting::add_ref(frontend)),
      bind_address_(unix_socket_path.str()),
      port_(0),
      threads_(threads ? threads : 1),
      max_concurrent_streams_(max_concurrent_streams ? max_concurrent_streams : 100),
      read_buffer_size_(read_buffer_size ? read_buffer_size : 64 * 1024),
      use_unix_socket_(true),
      io_service_(std::make_shared<boost::asio::io_service>()),
      io_work_(new boost::asio::io_service::work(*io_service_)),
      acceptor_(std::make_shared<boost::asio::ip::tcp::acceptor>(*io_service_))
  {}

  Http2Acceptor::~Http2Acceptor() noexcept = default;

  void
  Http2Acceptor::activate_object_()
  {
    if(use_unix_socket_)
    {
      Stream::Error ostr;
      ostr << "unix domain socket initialization is not supported by current Http2Acceptor transport";
      throw Exception(ostr);
    }

    boost::system::error_code ec;
    const auto address = boost::asio::ip::address::from_string(bind_address_, ec);
    if(ec)
    {
      Stream::Error ostr;
      ostr << "invalid HTTP/2 bind address '" << bind_address_ << "': " << ec.message();
      throw Exception(ostr);
    }

    const boost::asio::ip::tcp::endpoint endpoint(
      address,
      static_cast<unsigned short>(port_));

    acceptor_->open(endpoint.protocol());
    acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_->bind(endpoint);
    acceptor_->listen();

    create_accept_stub_();

    io_runner_.reset(
      new BoostAsioContextRunActiveObject(
        nullptr,
        io_service_,
        threads_));
    io_runner_->activate_object();
  }

  void
  Http2Acceptor::deactivate_object_()
  {
    boost::system::error_code ec;
    acceptor_->cancel(ec);
    acceptor_->close(ec);

    std::unordered_map<Connection*, Connection_var> connections_copy;
    {
      std::lock_guard<std::mutex> lock(connections_lock_);
      connections_copy.swap(connections_);
    }

    for(auto& connection_it : connections_copy)
    {
      connection_it.second->deactivate();
    }

    io_work_.reset();
    io_service_->stop();
  }

  void
  Http2Acceptor::wait_object_()
  {
    if(io_runner_)
    {
      io_runner_->deactivate_object();
      io_runner_->wait_object();
      io_runner_.reset();
    }
  }

  void
  Http2Acceptor::create_accept_stub_()
  {
    auto connection = std::make_shared<Connection>(
      this,
      io_service_,
      max_concurrent_streams_,
      read_buffer_size_);

    acceptor_->async_accept(
      connection->socket(),
      [this, connection](
        const boost::system::error_code& error)
      {
        handle_accept_(connection, error);
      });
  }

  void
  Http2Acceptor::handle_accept_(
    const Connection_var& accepted_connection,
    const boost::system::error_code& error)
  {
    if(!error)
    {
      {
        std::lock_guard<std::mutex> lock(connections_lock_);
        connections_.emplace(accepted_connection.get(), accepted_connection);
      }
      accepted_connection->activate();
    }
    else if(error != boost::asio::error::operation_aborted)
    {
      Stream::Error ostr;
      ostr << "accept failed: " << error.message();
      logger_i_()->log(ostr.str(), Logging::Logger::ERROR, ASPECT);
    }

    if(active())
    {
      create_accept_stub_();
    }
  }

  void
  Http2Acceptor::erase_connection_(Connection* connection) noexcept
  {
    std::lock_guard<std::mutex> lock(connections_lock_);
    connections_.erase(connection);
  }

  Logging::Logger*
  Http2Acceptor::logger_i_() noexcept
  {
    return logger_;
  }

  FrontendCommons::FrontendInterface*
  Http2Acceptor::frontend_i_() noexcept
  {
    return frontend_;
  }
}
}
