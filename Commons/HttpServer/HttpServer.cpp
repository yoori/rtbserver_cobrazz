#include <Commons/HttpServer/HttpServer.hpp>
#include <Commons/ThreadName.hpp>

#include <chrono>
#include <algorithm>
#include <utility>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace AdServer::Commons::HttpServer
{
  namespace
  {
    namespace asio = boost::asio;
    namespace beast = boost::beast;
    namespace http = beast::http;
    using tcp = asio::ip::tcp;

    std::string
    extract_path(const std::string& target)
    {
      const auto pos = target.find('?');
      return pos == std::string::npos ? target : target.substr(0, pos);
    }
  }

  struct HttpServer::Impl
  {
    using WorkGuard = asio::executor_work_guard<asio::io_context::executor_type>;

    asio::io_context io_context;
    std::unique_ptr<tcp::acceptor> acceptor;
    std::unique_ptr<WorkGuard> work_guard;
  };

  class Session final:
    public std::enable_shared_from_this<Session>
  {
  public:
    Session(tcp::socket&& socket, HttpServer* server)
      : stream_(std::move(socket)),
        server_(server)
    {}

    void start()
    {
      stream_.expires_after(std::chrono::seconds(30));
      http::async_read(
        stream_,
        buffer_,
        request_,
        [self = shared_from_this()](const boost::system::error_code& ec, std::size_t)
        {
          if (!ec)
          {
            self->write_response_();
          }
        });
    }

  private:
    void write_response_()
    {
      HttpServer::Request app_request;
      app_request.method = std::string(request_.method_string());
      app_request.target = std::string(request_.target());
      app_request.path = extract_path(app_request.target);
      app_request.body = request_.body();

      const auto app_response = server_->handle_request_(app_request);
      auto response = std::make_shared<http::response<http::string_body>>(
        static_cast<http::status>(app_response.status),
        request_.version());
      response->set(http::field::server, "AdServer");
      response->set(http::field::content_type, app_response.content_type);
      response->keep_alive(false);
      response->body() = app_response.body;
      response->prepare_payload();

      http::async_write(
        stream_,
        *response,
        [
          self = shared_from_this(),
          response
        ](const boost::system::error_code&, std::size_t)
        {
          boost::system::error_code ec;
          self->stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        });
    }

  private:
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    HttpServer* const server_;
  };

  HttpServer::HttpServer(std::string host, unsigned short port, unsigned long threads)
    : host_(std::move(host)),
      port_(port),
      threads_(std::max<unsigned long>(1, threads)),
      impl_(new Impl())
  {}

  HttpServer::~HttpServer() noexcept = default;

  void
  HttpServer::add_handler(const std::string& path, Handler handler)
  {
    std::lock_guard<std::mutex> lock(handlers_lock_);
    handlers_[path] = std::move(handler);
  }

  void
  HttpServer::activate_object_()
  {
    impl_->io_context.restart();

    boost::system::error_code ec;
    const auto address =
      (host_.empty() || host_ == "*") ?
        asio::ip::address_v4::any() :
        asio::ip::make_address(host_, ec);
    if (ec)
    {
      throw Exception(ec.message());
    }

    tcp::endpoint endpoint(address, port_);
    impl_->acceptor.reset(new tcp::acceptor(impl_->io_context));
    impl_->acceptor->open(endpoint.protocol(), ec);
    if (ec)
    {
      throw Exception(ec.message());
    }

    impl_->acceptor->set_option(tcp::acceptor::reuse_address(true), ec);
    if (ec)
    {
      throw Exception(ec.message());
    }

    impl_->acceptor->bind(endpoint, ec);
    if (ec)
    {
      throw Exception(ec.message());
    }

    impl_->acceptor->listen(asio::socket_base::max_listen_connections, ec);
    if (ec)
    {
      throw Exception(ec.message());
    }

    impl_->work_guard.reset(new Impl::WorkGuard(impl_->io_context.get_executor()));
    do_accept_();
    worker_threads_.reserve(threads_);
    for (unsigned long i = 0; i < threads_; ++i)
    {
      worker_threads_.emplace_back([this]() {
        AdServer::Commons::set_current_thread_name("http-server");
        impl_->io_context.run();
      });
    }
  }

  void
  HttpServer::deactivate_object_()
  {
    boost::system::error_code ec;
    if (impl_->acceptor)
    {
      impl_->acceptor->close(ec);
    }
    impl_->work_guard.reset();
    impl_->io_context.stop();
  }

  void
  HttpServer::wait_object_()
  {
    for (auto& thread : worker_threads_)
    {
      if (thread.joinable())
      {
        thread.join();
      }
    }
    worker_threads_.clear();
  }

  void
  HttpServer::do_accept_()
  {
    auto socket = std::make_shared<tcp::socket>(impl_->io_context);
    impl_->acceptor->async_accept(
      *socket,
      [
        this,
        socket
      ](const boost::system::error_code& ec)
      {
        if (!ec)
        {
          std::make_shared<Session>(std::move(*socket), this)->start();
        }

        if (active() && ec != asio::error::operation_aborted && ec != asio::error::bad_descriptor)
        {
          do_accept_();
        }
      });
  }

  HttpServer::Response
  HttpServer::handle_request_(const Request& request) noexcept
  {
    Handler handler;
    {
      std::lock_guard<std::mutex> lock(handlers_lock_);
      const auto it = handlers_.find(request.path);
      if (it != handlers_.end())
      {
        handler = it->second;
      }
    }

    if (!handler)
    {
      return {
        404,
        "application/json",
        "{\"error\":\"not found\"}\n"
      };
    }

    try
    {
      return handler(request);
    }
    catch (...)
    {
      return {
        500,
        "application/json",
        "{\"error\":\"internal error\"}\n"
      };
    }
  }
}
