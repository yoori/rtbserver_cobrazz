#include <csignal>
#include <iostream>
#include <unistd.h>

#include <Generics/AppUtils.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/StreamLogger.hpp>

#include <Frontends/FCGIServer/Http2Acceptor.hpp>
#include <Frontends/FrontendCommons/FrontendInterface.hpp>

namespace
{
  volatile std::sig_atomic_t g_stop = 0;

  void
  signal_handler(int /*signum*/)
  {
    g_stop = 1;
  }
}

namespace AdServer::Utils
{
  class EchoFrontendImpl final:
    public FrontendCommons::FrontendInterface,
    public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    bool
    will_handle(const String::SubString& /*uri*/) noexcept override
    {
      return true;
    }

    void
    handle_request(
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer) noexcept override
    {
      FCGI::HttpResponse_var response(new FCGI::HttpResponse());
      response->set_status(200);
      response->set_content_type_nocopy(String::SubString("text/plain; charset=utf-8"));

      const auto& request = request_holder->request();
      Stream::Error out;
      out << "method=" << (request.method() == FCGI::HttpRequest::RM_POST ? "POST" : "GET")
          << "\nuri=" << request.uri()
          << "\nargs=" << request.args()
          << "\nbody=" << request.body()
          << "\n";

      response->write(out.str());
      response_writer->write(std::move(response));
    }

    void
    init() override
    {}

  protected:
    ~EchoFrontendImpl() noexcept override = default;
  };
}


int
main(int argc, char** argv)
{
  Generics::AppUtils::CheckOption opt_help;
  Generics::AppUtils::StringOption opt_bind("127.0.0.1");
  Generics::AppUtils::Option<unsigned long> opt_port(8080);

  Generics::AppUtils::Args args(-1);
  args.add(Generics::AppUtils::equal_name("help") || Generics::AppUtils::short_name("h"), opt_help);
  args.add(Generics::AppUtils::equal_name("bind") || Generics::AppUtils::short_name("b"), opt_bind);
  args.add(Generics::AppUtils::equal_name("port") || Generics::AppUtils::short_name("p"), opt_port);
  args.parse(argc - 1, argv + 1);

  if (opt_help.enabled())
  {
    std::cout << "Http2EchoServer options:\n"
      "  --bind|-b <address>   Bind address (default: 127.0.0.1)\n"
      "  --port|-p <port>      TCP port (default: 8080)\n";
    return 0;
  }

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  Logging::Logger_var logger(new Logging::OStream::Logger(Logging::OStream::Config(std::cerr)));
  FrontendCommons::Frontend_var frontend(new AdServer::Utils::EchoFrontendImpl());

  ReferenceCounting::SmartPtr<AdServer::Frontends::Http2Acceptor> acceptor(
    new AdServer::Frontends::Http2Acceptor(
      logger,
      frontend,
      *opt_bind,
      *opt_port,
      2,      // threads
      256,    // max_concurrent_streams
      0       // read_buffer_size
      ));

  acceptor->activate_object();

  while (!g_stop)
  {
    ::sleep(1);
  }

  acceptor->deactivate_object();
  acceptor->wait_object();
  return 0;
}
