#include <array>
#include <cerrno>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <nghttp2/nghttp2.h>

#include <Frontends/FCGIServer/Http2Acceptor.hpp>

namespace
{
  using Responses = std::map<std::string, std::string>;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  struct Socket
  {
    Socket() : fd(::socket(AF_INET, SOCK_STREAM, 0))
    {
      require(fd != -1, "socket failed");
    }

    ~Socket()
    {
      ::close(fd);
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    const int fd;
  };

  unsigned short free_port()
  {
    Socket socket;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    require(::bind(socket.fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0,
      "bind failed");
    socklen_t size = sizeof(address);
    require(::getsockname(socket.fd, reinterpret_cast<sockaddr*>(&address), &size) == 0,
      "getsockname failed");
    return ntohs(address.sin_port);
  }

  class ResponseFrontend final:
    public FrontendCommons::FrontendInterface,
    public Generics::RefCountableSimpleActiveObject
  {
  public:
    explicit ResponseFrontend(const Responses& responses) : responses_(responses)
    {}

    bool will_handle(const String::SubString&) noexcept override
    {
      return true;
    }

    void init() override
    {}

    void
    handle_request(FCGI::HttpRequestHolder_var request, FCGI::BaseHttpResponseWriter_var writer)
      noexcept override
    {
      const auto path = request->request().uri().str();
      FCGI::HttpResponse_var response = new FCGI::HttpResponse;
      response->set_status(200);
      response->set_content_type("application/octet-stream");
      response->write(String::SubString(responses_.at(path)));
      writer->write(std::move(response));
      if (path == "/duplicate")
      {
        response = new FCGI::HttpResponse;
        response->set_status(500);
        response->write(String::SubString("unexpected second response"));
        writer->write(std::move(response));
      }
    }

  private:
    const Responses& responses_;
  };

  struct Server
  {
    explicit Server(const Responses& responses)
      : port(free_port()),
        logger(new Logging::Null::Logger),
        frontend(new ResponseFrontend(responses)),
        acceptor(new AdServer::Frontends::Http2Acceptor(
          logger, frontend, String::SubString("127.0.0.1"), port, 2, 100, 4096, 128 * 1024))
    {
      acceptor->activate_object();
    }

    ~Server()
    {
      acceptor->deactivate_object();
      acceptor->wait_object();
    }

    const unsigned short port;
    Logging::Logger_var logger;
    FrontendCommons::Frontend_var frontend;
    ReferenceCounting::SmartPtr<AdServer::Frontends::Http2Acceptor> acceptor;
  };

  struct Result
  {
    std::string body;
    std::string status;
    bool closed = false;
    uint32_t error = 0;
  };

  class Client
  {
  public:
    Client(unsigned short port, uint32_t window, bool automatic_window_update = true)
    {
      sockaddr_in address{};
      address.sin_family = AF_INET;
      address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      address.sin_port = htons(port);
      require(::connect(socket_.fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0,
        "connect failed");
      timeval timeout{2, 0};
      require(::setsockopt(socket_.fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0,
        "setsockopt failed");

      nghttp2_session_callbacks* raw_callbacks = nullptr;
      require(nghttp2_session_callbacks_new(&raw_callbacks) == 0, "callbacks allocation failed");
      const std::unique_ptr<nghttp2_session_callbacks, decltype(&nghttp2_session_callbacks_del)>
        callbacks(raw_callbacks, nghttp2_session_callbacks_del);
      nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks.get(), receive_data);
      nghttp2_session_callbacks_set_on_header_callback(callbacks.get(), receive_header);
      nghttp2_session_callbacks_set_on_stream_close_callback(callbacks.get(), close_stream);

      nghttp2_option* raw_options = nullptr;
      require(nghttp2_option_new(&raw_options) == 0, "options allocation failed");
      const std::unique_ptr<nghttp2_option, decltype(&nghttp2_option_del)>
        options(raw_options, nghttp2_option_del);
      nghttp2_option_set_no_auto_window_update(options.get(), !automatic_window_update);
      nghttp2_session* raw_session = nullptr;
      require(nghttp2_session_client_new2(&raw_session, callbacks.get(), this, options.get()) == 0,
        "session creation failed");
      session_.reset(raw_session);
      const nghttp2_settings_entry setting{NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, window};
      require(nghttp2_submit_settings(session_.get(), NGHTTP2_FLAG_NONE, &setting, 1) == 0,
        "settings submission failed");
    }

    int32_t request(const std::string& path)
    {
      const std::array<nghttp2_nv, 4> headers{{
        header(":method", "GET"), header(":scheme", "http"),
        header(":authority", "localhost"), header(":path", path)
      }};
      const auto id = nghttp2_submit_request(
        session_.get(), nullptr, headers.data(), headers.size(), nullptr, nullptr);
      require(id > 0, "request submission failed");
      results.emplace(id, Result{});
      return id;
    }

    void cancel(int32_t id)
    {
      require(nghttp2_submit_rst_stream(session_.get(), NGHTTP2_FLAG_NONE, id, NGHTTP2_CANCEL) == 0,
        "stream reset failed");
    }

    template<typename Predicate>
    void receive_until(Predicate done)
    {
      flush();
      while (!done())
      {
        pollfd descriptor{socket_.fd, POLLIN, 0};
        const auto ready = ::poll(&descriptor, 1, 2000);
        if (ready < 0 && errno == EINTR)
        {
          continue;
        }

        require(ready > 0, "response timed out");
        std::array<uint8_t, 8192> buffer;
        const auto size = ::recv(socket_.fd, buffer.data(), buffer.size(), 0);
        require(size > 0, "connection closed before response completion");
        require(nghttp2_session_mem_recv(session_.get(), buffer.data(), size) == size,
          "invalid HTTP/2 response");
        flush();
      }
    }

    std::map<int32_t, Result> results;

  private:
    static nghttp2_nv header(std::string_view name, std::string_view value)
    {
      return {reinterpret_cast<uint8_t*>(const_cast<char*>(name.data())),
        reinterpret_cast<uint8_t*>(const_cast<char*>(value.data())),
        name.size(), value.size(), NGHTTP2_NV_FLAG_NONE};
    }

    static int receive_data(
      nghttp2_session*, uint8_t, int32_t id, const uint8_t* data, size_t size, void* user)
    {
      static_cast<Client*>(user)->results.at(id).body.append(
        reinterpret_cast<const char*>(data), size);
      return 0;
    }

    static int receive_header(
      nghttp2_session*, const nghttp2_frame* frame, const uint8_t* name, size_t name_size,
      const uint8_t* value, size_t value_size, uint8_t, void* user)
    {
      if (std::string_view(reinterpret_cast<const char*>(name), name_size) == ":status")
      {
        static_cast<Client*>(user)->results.at(frame->hd.stream_id).status.assign(
          reinterpret_cast<const char*>(value), value_size);
      }

      return 0;
    }

    static int close_stream(nghttp2_session*, int32_t id, uint32_t error, void* user)
    {
      auto& result = static_cast<Client*>(user)->results.at(id);
      result.closed = true;
      result.error = error;
      return 0;
    }

    void flush()
    {
      const uint8_t* data = nullptr;
      ssize_t size;
      while ((size = nghttp2_session_mem_send(session_.get(), &data)) > 0)
      {
        while (size > 0)
        {
          const auto sent = ::send(socket_.fd, data, size, MSG_NOSIGNAL);
          if (sent < 0 && errno == EINTR)
          {
            continue;
          }

          require(sent > 0, "send failed");
          data += sent;
          size -= sent;
        }
      }

      require(size == 0, "HTTP/2 serialization failed");
    }

    Socket socket_;
    std::unique_ptr<nghttp2_session, decltype(&nghttp2_session_del)>
      session_{nullptr, nghttp2_session_del};
  };

  void test_responses(const Server& server, const Responses& responses, uint32_t window)
  {
    Client client(server.port, window);
    std::map<int32_t, const std::string*> expected;
    for (const auto& [path, body] : responses)
    {
      expected.emplace(client.request(path), &body);
    }

    client.receive_until([&]
    {
      for (const auto& [id, result] : client.results)
      {
        if (!result.closed)
        {
          return false;
        }
      }

      return true;
    });
    for (const auto& [id, body] : expected)
    {
      const auto& result = client.results.at(id);
      require(result.error == 0 && result.status == "200", "unexpected response status");
      if (result.body != *body)
      {
        throw std::runtime_error("response mismatch: expected " + std::to_string(body->size()) +
          " bytes, received " + std::to_string(result.body.size()));
      }
    }
  }

  void test_cancel(const Server& server, const Responses& responses)
  {
    for (const bool reset_stream : {false, true})
    {
      Client client(server.port, 1024, false);
      const auto id = client.request("/200000");
      client.receive_until([&] { return !client.results.at(id).body.empty(); });
      require(!client.results.at(id).closed, "large response ended before the flow window opened");
      if (reset_stream)
      {
        client.cancel(id);
        const auto next = client.request("/1");
        client.receive_until([&] { return client.results.at(next).closed; });
        require(client.results.at(next).body == responses.at("/1"),
          "stream reset damaged another response");
      }
    }

    test_responses(server, responses, 65535);
  }
}

int main()
{
  try
  {
    Responses responses;
    for (const std::size_t size : {0, 1, 16383, 16384, 16385, 40000, 200000})
    {
      std::string body(size, '\0');
      for (std::size_t i = 0; i < size; ++i)
      {
        body[i] = static_cast<char>(i % 251);
      }

      responses.emplace("/" + std::to_string(size), std::move(body));
    }

    responses.emplace("/duplicate", responses.at("/40000"));
    Server server(responses);
    test_responses(server, responses, 65535);
    test_responses(server, responses, 1024);
    test_cancel(server, responses);
    std::cout << "Http2ResponseTest: PASS\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "Http2ResponseTest: " << ex.what() << '\n';
    return 1;
  }
}
