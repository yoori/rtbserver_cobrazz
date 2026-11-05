#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <Logger/ActiveObjectCallback.hpp>
#include <Logger/StreamLogger.hpp>

#include <Frontends/FCGIServer/FCGIAcceptor.hpp>

namespace
{
  void check(bool condition, const std::string& message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  std::string record(unsigned char type, std::string_view body, unsigned short id = 1)
  {
    check(body.size() <= 65535, "Test record is too large");
    const unsigned char padding = (8 - body.size() % 8) % 8;
    const unsigned char header[] = {
      1, type, static_cast<unsigned char>(id >> 8), static_cast<unsigned char>(id),
      static_cast<unsigned char>(body.size() >> 8), static_cast<unsigned char>(body.size()),
      padding, 0
    };
    std::string result(reinterpret_cast<const char*>(header), sizeof(header));
    result.append(body);
    result.append(padding, '\0');
    return result;
  }

  std::string parameter(std::string_view name, std::string_view value)
  {
    check(name.size() < 128 && value.size() < 128, "Test parameter is too large");
    std::string result;
    result.push_back(name.size());
    result.push_back(value.size());
    result.append(name);
    result.append(value);
    return result;
  }

  std::string make_request(std::string_view body, std::size_t chunk_size)
  {
    std::string begin(8, '\0');
    begin[1] = FCGI_RESPONDER;
    begin[2] = FCGI_KEEP_CONN;
    const std::string params = parameter("REQUEST_METHOD", "POST") +
      parameter("REQUEST_URI", "/fragment-test") + parameter("QUERY_STRING", "key=value") +
      parameter("HTTP_X_TEST", "header-value");
    std::string result = record(FCGI_BEGIN_REQUEST, begin) + record(FCGI_PARAMS, params) +
      record(FCGI_PARAMS, "");
    for (std::size_t offset = 0; offset < body.size(); offset += chunk_size)
    {
      result += record(FCGI_STDIN, body.substr(offset, chunk_size));
    }
    return result + record(FCGI_STDIN, "");
  }

  void check_request(const FCGI::HttpRequest& request, std::string_view body)
  {
    check(request.body() == body, "Request body differs");
    check(request.method() == FCGI::HttpRequest::RM_POST, "Request method differs");
    check(request.uri() == "/fragment-test", "Request URI differs");
    check(request.args() == "key=value", "Request query differs");
    check(request.headers().size() == 1, "Request header count differs");
    check(request.headers().front().name == "X_TEST", "Request header name differs");
    check(request.headers().front().value == "header-value", "Request header value differs");
  }

  FCGI::HttpRequestHolder::Buffer make_buffer(std::string_view data)
  {
    std::shared_ptr<char[]> buffer(new char[data.size()]);
    std::memcpy(buffer.get(), data.data(), data.size());
    return buffer;
  }

  void check_parse_error(std::string_view data, int expected)
  {
    FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
    const auto buffer = make_buffer(data);
    std::size_t consumed = 0;
    check(holder->append(buffer, 0, data.size(), consumed) == expected,
      "Malformed request returned an unexpected result");
  }

  struct BenchmarkBuffer
  {
    FCGI::HttpRequestHolder::Buffer data;
    std::size_t size;
  };

  using BenchmarkBuffers = std::vector<BenchmarkBuffer>;

  std::size_t run_parse_benchmark(const BenchmarkBuffers& buffers, std::size_t body_size)
    __attribute__((noinline));

  std::size_t run_parse_benchmark(const BenchmarkBuffers& buffers, std::size_t body_size)
  {
    std::size_t total_body_size = 0;
    for (std::size_t iteration = 0; iteration < 10000; ++iteration)
    {
      FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
      for (std::size_t i = 0; i < buffers.size(); ++i)
      {
        const auto& buffer = buffers[i];
        std::size_t consumed = 0;
        const int result = holder->append(buffer.data, 0, buffer.size, consumed);
        const int expected = i + 1 == buffers.size() ? FCGI::PARSE_OK : FCGI::PARSE_NEED_MORE;
        if (result != expected || consumed != buffer.size)
        {
          throw std::runtime_error("Benchmark parsing failed");
        }
      }

      if (holder->request().body().size() != body_size)
      {
        throw std::runtime_error("Benchmark body size differs");
      }
      total_body_size += holder->request().body().size();
    }
    return total_body_size;
  }

  void benchmark(std::string_view mode)
  {
    check(mode == "single" || mode == "split", "Expected benchmark mode: single or split");
    std::string body(8192, '\0');
    for (std::size_t i = 0; i < body.size(); ++i)
    {
      body[i] = i % 256;
    }
    const std::string wire = make_request(body, body.size());
    BenchmarkBuffers buffers;
    for (std::size_t offset = 0; offset < wire.size();)
    {
      // Split the first record header as well as the body across independently owned buffers.
      const std::size_t chunk = mode == "single" ? wire.size() : (offset == 0 ? 5 : 1024);
      const auto data = std::string_view(wire).substr(offset, chunk);
      buffers.push_back({make_buffer(data), data.size()});
      offset += data.size();
    }

    FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
    for (const auto& buffer : buffers)
    {
      std::size_t consumed = 0;
      holder->append(buffer.data, 0, buffer.size, consumed);
    }
    check_request(holder->request(), body);

    // Fixture construction and full payload comparison stay outside the profiled function.
    const auto total_body_size = run_parse_benchmark(buffers, body.size());
    check(total_body_size == 10000 * body.size(), "Benchmark total body size differs");
    std::cout << "FCGIRequestTest benchmark " << mode << ": 10000 parses, " << buffers.size()
      << " buffers/request, " << wire.size() << " wire bytes, " << body.size()
      << " body bytes, " << total_body_size << " total body bytes" << std::endl;
  }

  void test_parser()
  {
    for (const std::size_t body_size : {0, 12, 65536, 200000})
    {
      std::string body(body_size, '\0');
      for (std::size_t i = 0; i < body.size(); ++i)
      {
        body[i] = i % 256;
      }

      for (const std::size_t chunk_size : {3, 16384, 65535})
      {
        const std::string wire = make_request(body, chunk_size);
        const auto buffer = make_buffer(wire);
        FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
        std::size_t consumed = 0;
        check(holder->append(buffer, 0, wire.size(), consumed) == FCGI::PARSE_OK,
          "Fragmented STDIN rejected");
        check(consumed == wire.size(), "Incorrect consumed length");
        check_request(holder->request(), body);
      }
    }

    const std::string wire = make_request("abcdefghijkl", 3);
    const auto prefix_buffer = make_buffer(wire);
    for (std::size_t size = 0; size < wire.size(); ++size)
    {
      FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
      std::size_t consumed = 123;
      check(holder->append(prefix_buffer, 0, size, consumed) == FCGI::PARSE_NEED_MORE,
        "Incomplete request rejected at offset " + std::to_string(size));
      check(consumed == size, "Incomplete request did not consume the supplied prefix");
    }

    const std::string combined = wire + make_request("next", 2);
    const auto combined_buffer = make_buffer(combined);
    FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
    std::size_t consumed = 0;
    check(holder->append(combined_buffer, 0, combined.size(), consumed) == FCGI::PARSE_OK,
      "First request in a combined buffer rejected");
    check(consumed == wire.size(), "Next request was consumed with the first");
    check_request(holder->request(), "abcdefghijkl");

    std::string invalid = wire;
    invalid[0] = 2;
    check_parse_error(invalid, FCGI::PARSE_INVALID_HEADER);
    invalid = wire;
    invalid[19] = 2;
    check_parse_error(invalid, FCGI::PARSE_INVALID_ID);
    invalid = wire;
    invalid[1] = FCGI_PARAMS;
    check_parse_error(invalid, FCGI::PARSE_BEGIN_REQUEST_EXPECTED);
  }

  void test_incremental_parser()
  {
    const std::string body("ab\0cdefghijkl", 13);
    const std::string wire = make_request(body, body.size());
    const std::size_t body_offset = wire.find(body);
    check(body_offset != std::string::npos, "Test body missing");

    for (const std::size_t split : {std::size_t(0), body_offset + 3})
    {
      std::weak_ptr<const char[]> lifetime;
      {
        FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
        auto buffer = make_buffer(wire);
        lifetime = buffer;
        std::size_t consumed = 0;
        if (split != 0)
        {
          check(holder->append(buffer, 0, split, consumed) == FCGI::PARSE_NEED_MORE,
            "Partial owned buffer completed early");
          check(consumed == split, "Partial owned buffer was not consumed");
        }

        check(holder->append(buffer, split, wire.size() - split, consumed) == FCGI::PARSE_OK,
          "Owned buffer rejected");
        check(consumed == wire.size() - split, "Owned buffer consumed length differs");
        check(holder->request().body().data() == buffer.get() + body_offset,
          "Contiguous body was copied");
        check(holder->request().uri().data() == buffer.get() + wire.find("/fragment-test"),
          "Contiguous params were copied");
        buffer.reset();
        check(!lifetime.expired(), "Request released its backing buffer");
        check_request(holder->request(), body);
      }
      check(lifetime.expired(), "Request leaked its backing buffer");
    }

    for (std::size_t split = 1; split < wire.size(); ++split)
    {
      FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
      std::size_t consumed = 0;
      {
        const auto first = make_buffer(std::string_view(wire).substr(0, split));
        check(holder->append(first, 0, split, consumed) == FCGI::PARSE_NEED_MORE,
          "Split request completed early at " + std::to_string(split));
        check(consumed == split, "Split prefix was not consumed");
      }
      {
        const auto second = make_buffer(std::string_view(wire).substr(split));
        check(holder->append(second, 0, wire.size() - split, consumed) == FCGI::PARSE_OK,
          "Split request rejected at " + std::to_string(split));
        check(consumed == wire.size() - split, "Split suffix was not consumed");
      }
      check_request(holder->request(), body);
    }

    {
      FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
      const auto first = make_buffer(std::string_view(wire).substr(0, body_offset + 3));
      auto second = make_buffer(std::string_view(wire).substr(body_offset + 3));
      const std::weak_ptr<const char[]> lifetime = second;
      std::size_t consumed = 0;
      check(holder->append(first, 0, body_offset + 3, consumed) == FCGI::PARSE_NEED_MORE,
        "Fragmented body completed early");
      check(holder->append(second, 0, wire.size() - body_offset - 3, consumed) == FCGI::PARSE_OK,
        "Fragmented body rejected");
      second.reset();
      check(lifetime.expired(), "Flattened body retained an unnecessary input buffer");
      check_request(holder->request(), body);
    }

    {
      FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
      const std::string bytewise = make_request(std::string(2048, 'x'), 3);
      for (std::size_t i = 0; i < bytewise.size(); ++i)
      {
        const auto buffer = make_buffer(std::string_view(bytewise).substr(i, 1));
        std::size_t consumed = 0;
        const int expected = i + 1 == bytewise.size() ? FCGI::PARSE_OK : FCGI::PARSE_NEED_MORE;
        check(holder->append(buffer, 0, 1, consumed) == expected, "Bytewise parsing failed");
        check(consumed == 1, "Bytewise parser failed to advance");
      }
      check_request(holder->request(), std::string(2048, 'x'));
    }

    {
      const std::string next = make_request("next", 4);
      auto buffer = make_buffer(wire + next);
      FCGI::HttpRequestHolder_var first(new FCGI::HttpRequestHolder());
      FCGI::HttpRequestHolder_var second(new FCGI::HttpRequestHolder());
      std::size_t consumed = 0;
      check(first->append(buffer, 0, wire.size() + next.size(), consumed) == FCGI::PARSE_OK,
        "Combined buffer's first request rejected");
      check(consumed == wire.size(), "Combined buffer lost its second request");
      check(second->append(buffer, consumed, next.size(), consumed) == FCGI::PARSE_OK,
        "Combined buffer's second request rejected");
      check(consumed == next.size(), "Combined buffer's second length differs");
      buffer.reset();
      second = nullptr;
      check_request(first->request(), body);
    }
  }

  void test_parameter_fragments()
  {
    std::string begin(8, '\0');
    begin[1] = FCGI_RESPONDER;
    const std::string value(130, 'v');
    std::string params = parameter("REQUEST_METHOD", "HEAD") +
      parameter("REQUEST_URI", "/fragment-test") + parameter("HTTPS", "on") +
      parameter("REMOTE_ADDR", "127.0.0.1");
    params += std::string("\x0b\x80\x00\x00\x82", 5) + "HTTP_X_LONG" + value;
    std::string wire = record(FCGI_BEGIN_REQUEST, begin);
    for (const char byte : params)
    {
      wire += record(FCGI_PARAMS, std::string_view(&byte, 1));
    }
    wire += record(FCGI_PARAMS, "") + record(FCGI_STDIN, "");
    FCGI::HttpRequestHolder_var holder(new FCGI::HttpRequestHolder());
    const auto buffer = make_buffer(wire);
    std::size_t consumed = 0;
    check(holder->append(buffer, 0, wire.size(), consumed) == FCGI::PARSE_OK,
      "Fragmented parameter stream rejected");
    const auto& request = holder->request();
    check(request.method() == FCGI::HttpRequest::RM_GET && request.header_only(), "HEAD lost");
    check(request.uri() == "/fragment-test", "Fragmented URI differs");
    check(request.secure(), "HTTPS flag lost");
    check(request.headers().size() == 3, "Fragmented header count differs");
    check(request.headers().back().name == "X_LONG", "Long header name differs");
    check(request.headers().back().value == value, "Four-byte parameter length differs");

    const std::string malformed = record(FCGI_BEGIN_REQUEST, begin) +
      record(FCGI_PARAMS, std::string("\x80", 1)) +
      record(FCGI_PARAMS, "") + record(FCGI_STDIN, "");
    check_parse_error(malformed, FCGI::PARSE_INVALID_HEADER);
  }

  class EchoFrontend final :
    public FrontendCommons::FrontendInterface,
    public Generics::RefCountableSimpleActiveObject
  {
  public:
    bool will_handle(const String::SubString&) noexcept override;
    void init() override;
    void handle_request(
      FCGI::HttpRequestHolder_var request, FCGI::BaseHttpResponseWriter_var writer)
      noexcept override;
    void handle_request_noparams(
      FCGI::HttpRequestHolder_var request, FCGI::BaseHttpResponseWriter_var writer) override;
    void check_retained_requests();

  private:
    std::mutex mutex_;
    std::vector<std::pair<FCGI::HttpRequestHolder_var, std::string>> requests_;
  };

  bool EchoFrontend::will_handle(const String::SubString&) noexcept
  {
    return true;
  }

  void EchoFrontend::init()
  {}

  void EchoFrontend::handle_request(
    FCGI::HttpRequestHolder_var request, FCGI::BaseHttpResponseWriter_var writer) noexcept
  {
    FCGI::HttpResponse_var response(new FCGI::HttpResponse());
    response->set_status(200);
    const auto body = request->request().body();
    {
      std::lock_guard<std::mutex> lock(mutex_);
      requests_.emplace_back(request, std::string(body));
    }
    response->write(String::SubString(body.data(), body.size()));
    writer->write(std::move(response));
  }

  void EchoFrontend::handle_request_noparams(
    FCGI::HttpRequestHolder_var request, FCGI::BaseHttpResponseWriter_var writer)
  {
    handle_request(std::move(request), std::move(writer));
  }

  void EchoFrontend::check_retained_requests()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& saved : requests_)
    {
      check_request(saved.first->request(), saved.second);
    }
  }

  class Server
  {
  public:
    Server();
    ~Server();
    void check_retained_requests();
    std::string path;

  private:
    std::string directory_;
    ReferenceCounting::SmartPtr<EchoFrontend> frontend_;
    ReferenceCounting::SmartPtr<AdServer::Frontends::FCGIAcceptor> acceptor_;
  };

  Server::Server()
  {
    char directory[] = "/tmp/fcgi-request-test-XXXXXX";
    check(mkdtemp(directory) != nullptr, "Cannot create test directory");
    directory_ = directory;
    path = directory_ + "/fcgi.sock";
    Logging::Logger_var logger(new Logging::Null::Logger());
    Logging::ActiveObjectCallbackImpl_var callback(new Logging::ActiveObjectCallbackImpl(logger));
    frontend_ = new EchoFrontend();
    acceptor_ = new AdServer::Frontends::FCGIAcceptor(
      logger, frontend_, callback, String::SubString(path), 32, 2);
    acceptor_->activate_object();
  }

  Server::~Server()
  {
    acceptor_->deactivate_object();
    acceptor_->wait_object();
    unlink(path.c_str());
    rmdir(directory_.c_str());
  }

  void Server::check_retained_requests()
  {
    frontend_->check_retained_requests();
  }

  class Client
  {
  public:
    explicit Client(const std::string& path);
    ~Client();
    void send(std::string_view data);
    void expect_incomplete();
    void expect_response(std::string_view body);

  private:
    std::string read(std::size_t size);
    int fd_;
  };

  Client::Client(const std::string& path)
    : fd_(socket(AF_UNIX, SOCK_STREAM, 0))
  {
    check(fd_ != -1, "Cannot create test socket");
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::strcpy(address.sun_path, path.c_str());
    if (connect(fd_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0)
    {
      close(fd_);
      throw std::runtime_error("Cannot connect test socket");
    }
  }

  Client::~Client()
  {
    close(fd_);
  }

  void Client::send(std::string_view data)
  {
    while (!data.empty())
    {
      const auto size = ::send(fd_, data.data(), data.size(), MSG_NOSIGNAL);
      check(size > 0, "Cannot send request: " + std::string(std::strerror(errno)));
      data.remove_prefix(size);
    }
  }

  void Client::expect_incomplete()
  {
    pollfd descriptor{fd_, POLLIN, 0};
    check(poll(&descriptor, 1, 20) == 0, "Incomplete request answered or connection closed");
  }

  std::string Client::read(std::size_t size)
  {
    std::string result(size, '\0');
    std::size_t offset = 0;
    while (offset < size)
    {
      pollfd descriptor{fd_, POLLIN, 0};
      check(poll(&descriptor, 1, 2000) > 0, "Response timed out");
      const auto received = recv(fd_, result.data() + offset, size - offset, 0);
      check(received > 0, "Connection closed before response completed");
      offset += received;
    }
    return result;
  }

  void Client::expect_response(std::string_view body)
  {
    std::string output;
    for (;;)
    {
      const std::string header = read(8);
      const auto* bytes = reinterpret_cast<const unsigned char*>(header.data());
      check(bytes[0] == 1 && bytes[2] == 0 && bytes[3] == 1, "Invalid response header");
      const std::size_t size = bytes[4] * 256 + bytes[5];
      const std::string data = read(size + bytes[6]);
      if (bytes[1] == FCGI_STDOUT)
      {
        output.append(data.data(), size);
      }
      else if (bytes[1] == FCGI_END_REQUEST)
      {
        break;
      }
    }
    const auto separator = output.find("\r\n\r\n");
    check(separator != std::string::npos, "Response headers missing");
    check(output.substr(separator + 4) == body, "Echoed body differs");
  }

  void test_connection()
  {
    Server server;
    const std::string body("ab\0cdefghijkl", 13);
    const std::string wire = make_request(body, 3);
    const std::size_t splits[] = {1, 5, 8, 16, 25, wire.size() - 25};
    for (const std::size_t split : splits)
    {
      Client client(server.path);
      client.send(std::string_view(wire).substr(0, split));
      client.expect_incomplete();
      client.send(std::string_view(wire).substr(split, wire.size() - split - 1));
      client.expect_incomplete();
      client.send(std::string_view(wire).substr(wire.size() - 1));
      client.expect_response(body);
      client.send(make_request("next", 1));
      client.expect_response("next");
    }

    Client client(server.path);
    const std::string next = make_request("next", 2);
    client.send(wire + next.substr(0, 5));
    client.expect_response(body);
    client.expect_incomplete();
    client.send(next.substr(5) + wire);
    client.expect_response("next");
    client.expect_response(body);

    const std::string large_body(600000, 'x');
    const std::string large_wire = make_request(large_body, 60000);
    client.send(std::string_view(large_wire).substr(0, 524288));
    client.expect_incomplete();
    client.send(std::string_view(large_wire).substr(524288));
    client.expect_response(large_body);
    server.check_retained_requests();
  }
}

int main(int argc, char** argv)
{
  try
  {
    if (argc == 3 && std::string_view(argv[1]) == "--benchmark")
    {
      benchmark(argv[2]);
      return 0;
    }
    check(argc == 1, "Usage: FCGIRequestTest [--benchmark single|split]");
    test_parser();
    test_incremental_parser();
    test_parameter_fragments();
    test_connection();
    std::cout << "FCGIRequestTest passed" << std::endl;
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << "FCGIRequestTest failed: " << error.what() << std::endl;
    return 1;
  }
}
