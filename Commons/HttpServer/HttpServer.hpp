#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace AdServer::Commons::HttpServer
{
  class HttpServer final:
    public Generics::RefCountableSimpleActiveObject
  {
  public:
    struct Request
    {
      std::string method;
      std::string target;
      std::string path;
      std::string body;
    };

    struct Response
    {
      unsigned int status = 200;
      std::string content_type = "text/plain";
      std::string body;
    };

    using Handler = std::function<Response(const Request&)>;

    HttpServer(std::string host, unsigned short port, unsigned long threads);

    ~HttpServer() noexcept override;

    void add_handler(const std::string& path, Handler handler);

  private:
    void activate_object_() override;
    void deactivate_object_() override;
    void wait_object_() override;

    void do_accept_();
    Response handle_request_(const Request& request) noexcept;

  private:
    friend class Session;

    const std::string host_;
    const unsigned short port_;
    const unsigned long threads_;

    std::mutex handlers_lock_;
    std::map<std::string, Handler> handlers_;

    std::vector<std::thread> worker_threads_;

    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

  using HttpServer_var = ReferenceCounting::SmartPtr<HttpServer>;
}
