#include <Commons/Coro/StartableAwaitable.hpp>
#include <Commons/Coro/Utils.hpp>
#include <Commons/HttpServer/HttpServer.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cerrno>
#include <condition_variable>
#include <coroutine>
#include <cstring>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace
{
  using HttpServer = AdServer::Commons::HttpServer::HttpServer;
  using namespace std::chrono_literals;

  class SuspendedRequest final
  {
  public:
    void suspend(std::coroutine_handle<> continuation)
    {
      std::lock_guard<std::mutex> lock(mutex_);
      continuation_ = continuation;
      condition_.notify_one();
    }

    bool wait() const
    {
      std::unique_lock<std::mutex> lock(mutex_);
      return condition_.wait_for(lock, 2s, [this]() { return continuation_ != nullptr; });
    }

    void resume()
    {
      std::coroutine_handle<> continuation;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        continuation = std::exchange(continuation_, {});
      }
      AdServer::Commons::resume_coroutine(continuation);
    }

  private:
    mutable std::mutex mutex_;
    mutable std::condition_variable condition_;
    std::coroutine_handle<> continuation_;
  };

  class RequestAwaiter final
  {
  public:
    explicit RequestAwaiter(std::shared_ptr<SuspendedRequest> request)
      : request_(std::move(request))
    {}

    bool await_ready() const noexcept
    {
      return false;
    }

    void await_suspend(std::coroutine_handle<> continuation) const
    {
      request_->suspend(continuation);
    }

    void await_resume() const noexcept
    {}

  private:
    std::shared_ptr<SuspendedRequest> request_;
  };

  AdServer::Commons::StartableAwaitable<HttpServer::Response>
  slow_response(std::shared_ptr<SuspendedRequest> suspended_request, HttpServer::Request)
  {
    co_await RequestAwaiter(std::move(suspended_request));
    co_return HttpServer::Response{200, "text/plain", "slow"};
  }

  AdServer::Commons::StartableAwaitable<HttpServer::Response>
  fast_response(HttpServer::Request)
  {
    co_return HttpServer::Response{200, "text/plain", "fast"};
  }

  AdServer::Commons::StartableAwaitable<HttpServer::Response>
  failing_response(HttpServer::Request)
  {
    throw std::runtime_error("handler failure");
    co_return HttpServer::Response{};
  }

  AdServer::Commons::StartableAwaitable<HttpServer::Response>
  delayed_failing_response(
    std::shared_ptr<SuspendedRequest> suspended_request,
    HttpServer::Request)
  {
    co_await RequestAwaiter(std::move(suspended_request));
    throw std::runtime_error("delayed handler failure");
    co_return HttpServer::Response{};
  }

  unsigned short free_port()
  {
    const int socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket == -1)
    {
      throw std::runtime_error(std::strerror(errno));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::bind(socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1)
    {
      ::close(socket);
      throw std::runtime_error(std::strerror(errno));
    }

    socklen_t size = sizeof(address);
    if (::getsockname(socket, reinterpret_cast<sockaddr*>(&address), &size) == -1)
    {
      ::close(socket);
      throw std::runtime_error(std::strerror(errno));
    }

    ::close(socket);
    return ntohs(address.sin_port);
  }

  std::string request(unsigned short port, std::string_view path)
  {
    const int socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket == -1)
    {
      throw std::runtime_error(std::strerror(errno));
    }

    timeval timeout{2, 0};
    ::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1)
    {
      ::close(socket);
      throw std::runtime_error(std::strerror(errno));
    }

    const std::string message =
      "GET " + std::string(path) + " HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Connection: close\r\n\r\n";
    std::size_t sent = 0;
    while (sent != message.size())
    {
      const ssize_t result = ::send(socket, message.data() + sent, message.size() - sent, 0);
      if (result <= 0)
      {
        ::close(socket);
        throw std::runtime_error(std::strerror(errno));
      }
      sent += result;
    }

    std::string response;
    char buffer[4096];
    for (;;)
    {
      const ssize_t size = ::recv(socket, buffer, sizeof(buffer), 0);
      if (size == 0)
      {
        break;
      }

      if (size < 0)
      {
        ::close(socket);
        throw std::runtime_error(std::strerror(errno));
      }
      response.append(buffer, size);
    }

    ::close(socket);
    return response;
  }

  std::string keep_alive_requests(unsigned short port)
  {
    const int socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket == -1)
    {
      throw std::runtime_error(std::strerror(errno));
    }

    timeval timeout{2, 0};
    ::setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1)
    {
      ::close(socket);
      throw std::runtime_error(std::strerror(errno));
    }

    const std::string message =
      "GET /fast HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Connection: keep-alive\r\n\r\n"
      "GET /fast HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Connection: close\r\n\r\n";
    std::size_t sent = 0;
    while (sent != message.size())
    {
      const ssize_t result = ::send(socket, message.data() + sent, message.size() - sent, 0);
      if (result <= 0)
      {
        ::close(socket);
        throw std::runtime_error(std::strerror(errno));
      }
      sent += result;
    }

    std::string response;
    char buffer[4096];
    for (;;)
    {
      const ssize_t size = ::recv(socket, buffer, sizeof(buffer), 0);
      if (size == 0)
      {
        break;
      }

      if (size < 0)
      {
        ::close(socket);
        throw std::runtime_error(std::strerror(errno));
      }
      response.append(buffer, size);
    }

    ::close(socket);
    return response;
  }

  std::size_t
  count_occurrences(std::string_view text, std::string_view value)
  {
    std::size_t count = 0;
    std::size_t position = 0;
    while ((position = text.find(value, position)) != std::string_view::npos)
    {
      ++count;
      position += value.size();
    }
    return count;
  }

  void
  send_and_disconnect(unsigned short port, std::string_view path)
  {
    const int socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket == -1)
    {
      throw std::runtime_error(std::strerror(errno));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(socket, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1)
    {
      ::close(socket);
      throw std::runtime_error(std::strerror(errno));
    }

    const std::string message =
      "GET " + std::string(path) + " HTTP/1.1\r\n"
      "Host: localhost\r\n"
      "Connection: close\r\n\r\n";
    std::size_t sent = 0;
    while (sent != message.size())
    {
      const ssize_t result = ::send(socket, message.data() + sent, message.size() - sent, 0);
      if (result <= 0)
      {
        ::close(socket);
        throw std::runtime_error(std::strerror(errno));
      }
      sent += result;
    }

    ::close(socket);
  }
}

int main()
{
  try
  {
    const unsigned short port = free_port();
    auto suspended_request = std::make_shared<SuspendedRequest>();
    auto delayed_failure = std::make_shared<SuspendedRequest>();
    auto disconnected_request = std::make_shared<SuspendedRequest>();
    auto shutdown_request = std::make_shared<SuspendedRequest>();
    AdServer::Commons::HttpServer::HttpServer_var server =
      new HttpServer("127.0.0.1", port, 1, true);
    server->add_handler(
      "/slow",
      [suspended_request](HttpServer::Request request)
      {
        return slow_response(suspended_request, std::move(request));
      });
    server->add_handler("/fast", &fast_response);
    server->add_handler("/fail", &failing_response);
    server->add_handler(
      "/delayed-fail",
      [delayed_failure](HttpServer::Request request)
      {
        return delayed_failing_response(delayed_failure, std::move(request));
      });
    server->add_handler(
      "/disconnect",
      [disconnected_request](HttpServer::Request request)
      {
        return slow_response(disconnected_request, std::move(request));
      });
    server->add_handler(
      "/shutdown",
      [shutdown_request](HttpServer::Request request)
      {
        return slow_response(shutdown_request, std::move(request));
      });
    server->activate_object();

    auto slow = std::async(std::launch::async, [port]() { return request(port, "/slow"); });
    if (!suspended_request->wait())
    {
      throw std::runtime_error("async handler did not start");
    }

    auto fast = std::async(std::launch::async, [port]() { return request(port, "/fast"); });
    const bool fast_completed = fast.wait_for(1s) == std::future_status::ready;
    suspended_request->resume();
    const std::string fast_result = fast.get();
    const std::string slow_result = slow.get();
    const std::string failure_result = request(port, "/fail");
    const std::string keep_alive_result = keep_alive_requests(port);
    auto delayed_failure_result = std::async(
      std::launch::async,
      [port]() { return request(port, "/delayed-fail"); });
    if (!delayed_failure->wait())
    {
      throw std::runtime_error("delayed failure handler did not suspend");
    }
    delayed_failure->resume();
    const std::string delayed_failure_response = delayed_failure_result.get();

    send_and_disconnect(port, "/disconnect");
    if (!disconnected_request->wait())
    {
      throw std::runtime_error("disconnected handler did not suspend");
    }
    disconnected_request->resume();
    const std::string post_disconnect_response = request(port, "/fast");

    auto shutdown_result = std::async(
      std::launch::async,
      [port]() { return request(port, "/shutdown"); });
    if (!shutdown_request->wait())
    {
      throw std::runtime_error("shutdown handler did not suspend");
    }

    server->deactivate_object();
    server->wait_object();
    server.reset();
    shutdown_request->resume();
    shutdown_result.get();

    if (!fast_completed || fast_result.find("fast") == std::string::npos ||
      slow_result.find("slow") == std::string::npos ||
      failure_result.find("500 Internal Server Error") == std::string::npos ||
      count_occurrences(keep_alive_result, "fast") != 2 ||
      post_disconnect_response.find("fast") == std::string::npos ||
      delayed_failure_response.find("500 Internal Server Error") == std::string::npos)
    {
      throw std::runtime_error("async handler blocked the only HTTP worker");
    }

    std::cout << "HttpServerAsyncTest: PASS\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "HttpServerAsyncTest: FAIL: " << ex.what() << '\n';
    return 1;
  }
}
