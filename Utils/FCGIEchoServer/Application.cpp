#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <sstream>
#include <thread>

#include <Generics/AppUtils.hpp>
#include <Logger/Logger.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <Frontends/FrontendCommons/FrontendInterface.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <Frontends/FCGIServer/FCGIAcceptor.hpp>

namespace
{
  std::atomic<bool> stop_requested{false};

  void
  signal_handler(int)
  {
    stop_requested.store(true);
  }

  class EchoFrontend final:
    public FrontendCommons::FrontendInterface,
    public ReferenceCounting::AtomicImpl
  {
  public:
    bool
    will_handle(const String::SubString&) noexcept override
    {
      return true;
    }

    void
    handle_request(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer)
      noexcept override
    {
      const auto& request = request_holder->request();

      std::ostringstream response_body;
      response_body << "method=" << request.method() << "\n";
      response_body << "uri=" << request.uri() << "\n";
      response_body << "args=" << request.args() << "\n";
      response_body << "body_size=" << request.body().size() << "\n";
      response_body << "headers=" << request.headers().size() << "\n";
      response_body << "params=" << request.params().size() << "\n";
      response_body << "\n" << request.body() << "\n";

      FCGI::HttpResponse_var response(new FCGI::HttpResponse());
      const String::SubString content_type("text/plain");
      response->set_content_type(content_type);

      auto& output = response->get_output_stream();
      const auto payload = response_body.str();
      output.write(payload.data(), payload.size());

      response_writer->write(200, response);
    }

    void
    init() override
    {
    }

    void
    shutdown() noexcept override
    {
    }

  protected:
    ~EchoFrontend() noexcept override = default;
  };
}

int
main(int argc, char** argv)
{
  Generics::AppUtils::StringOption bind_address("/tmp/fcgi_echo.sock");
  Generics::AppUtils::Option<unsigned long> backlog(1024);
  Generics::AppUtils::Option<unsigned long> accept_threads(8);

  Generics::AppUtils::Args args(-1);
  args.add(Generics::AppUtils::equal_name("bind") || Generics::AppUtils::short_name("b"), bind_address);
  args.add(Generics::AppUtils::equal_name("backlog"), backlog);
  args.add(Generics::AppUtils::equal_name("accept-threads"), accept_threads);
  args.parse(argc - 1, argv + 1);

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  Logging::Logger_var logger(new Logging::Null::Logger());
  Logging::ActiveObjectCallbackImpl_var callback(
    new Logging::ActiveObjectCallbackImpl(
      logger,
      "FCGIEchoServer::main",
      "FCGIEchoServer",
      ""));

  FrontendCommons::Frontend_var frontend(new EchoFrontend());

  ReferenceCounting::SmartPtr<AdServer::Frontends::FCGIAcceptor> acceptor(
    new AdServer::Frontends::FCGIAcceptor(
      logger,
      frontend,
      callback,
      *bind_address,
      *backlog,
      *accept_threads));

  frontend->init();
  acceptor->activate_object();

  std::cerr << "FCGIEchoServer listen unix socket: " << *bind_address << std::endl;

  while(!stop_requested.load())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }

  acceptor->deactivate_object();
  acceptor->wait_object();
  frontend->shutdown();

  return 0;
}
