
#include <chrono>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include <Commons/ErrorHandler.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/PidFileGuard.hpp>
#include <Commons/SignalActiveObject.hpp>

#include <XMLUtility/Utility.cpp>

#include "FCGIServer.hpp"
#include "FrontendsPool.hpp"
#include "FCGIAcceptor.hpp"
#include "Http2Acceptor.hpp"

namespace
{
  const char ASPECT[] = "FCGIServer";

  const auto STARTUP_STARTED_AT = std::chrono::steady_clock::now();

  void
  trace_startup(const char* label)
  {
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - STARTUP_STARTED_AT);
    std::cerr << "FCGI_STARTUP "
      << (elapsed.count() / 1000) << "."
      << (elapsed.count() % 1000) << " "
      << label << std::endl;
  }

  std::string
  escape_json_string(const std::string& value)
  {
    std::string result;
    result.reserve(value.size());

    for (const char ch : value)
    {
      switch (ch)
      {
      case '\\':
        result += "\\\\";
        break;
      case '"':
        result += "\\\"";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        result += ch;
        break;
      }
    }

    return result;
  }

  struct JsonStatsWriter
  {
    explicit
    JsonStatsWriter(std::ostream& out) noexcept
      : out(out)
    {}

    void
    operator()(const std::size_t)
    {}

    template<typename Type>
    void
    operator()(
      const Generics::Values::Key& key,
      const Type& value)
    {
      append_key_(key);
      out << value;
    }

    void
    operator()(
      const Generics::Values::Key& key,
      const std::string& value)
    {
      append_key_(key);
      out << '"' << escape_json_string(value) << '"';
    }

    std::ostream& out;
    bool first = true;

    void
    append_key_(const Generics::Values::Key& key)
    {
      append_key_(key.text());
    }

    void
    append_key_(const std::string& key)
    {
      if (first)
      {
        first = false;
      }
      else
      {
        out << ',';
      }

      out << '"' << escape_json_string(key) << "\":";
    }
  };

  struct JsonMetricValueWriter : boost::static_visitor<>
  {
    explicit JsonMetricValueWriter(std::ostream& out) noexcept
      : out(out)
    {}

    void operator()(double value) const
    {
      out << value;
    }

    void operator()(long value) const
    {
      out << value;
    }

    void operator()(const std::string& value) const
    {
      out << '"' << escape_json_string(value) << '"';
    }

    std::ostream& out;
  };

  struct GrpcErrorStat
  {
    std::string time;
    std::string endpoint;
    long code = 0;
    std::string message;
    std::string source;
  };

  bool
  parse_last_error_metric(
    const std::string& name,
    std::string& client_name,
    std::string& field)
  {
    static const std::string suffix = "_last_error_";
    const auto pos = name.find(suffix);
    if (pos == std::string::npos)
    {
      return false;
    }

    client_name = name.substr(0, pos);
    field = name.substr(pos + suffix.size());
    return !client_name.empty() && !field.empty();
  }

  template<typename Value>
  void
  set_error_field(
    GrpcErrorStat& error,
    const std::string& field,
    const Value& value)
  {
    (void)error;
    (void)field;
    (void)value;
  }

  template<>
  void
  set_error_field<std::string>(
    GrpcErrorStat& error,
    const std::string& field,
    const std::string& value)
  {
    if (field == "time")
    {
      error.time = value;
    }
    else if (field == "endpoint")
    {
      error.endpoint = value;
    }
    else if (field == "message")
    {
      error.message = value;
    }
    else if (field == "source")
    {
      error.source = value;
    }
  }

  template<>
  void
  set_error_field<long>(
    GrpcErrorStat& error,
    const std::string& field,
    const long& value)
  {
    if (field == "code")
    {
      error.code = value;
    }
  }

  struct GrpcErrorFieldVisitor : boost::static_visitor<>
  {
    GrpcErrorFieldVisitor(GrpcErrorStat& error, const std::string& field)
      : error(error),
        field(field)
    {}

    template<typename Value>
    void operator()(const Value& value) const
    {
      set_error_field(error, field, value);
    }

    GrpcErrorStat& error;
    const std::string& field;
  };

  std::string
  dump_stats_json(
    const AdServer::StatHolder_var& stats,
    const Generics::CompositeMetricsProvider_var& metrics)
  {
    Generics::Values_var values = stats->dump_stats();
    std::map<std::string, GrpcErrorStat> errors;

    std::ostringstream out;
    out << '{';
    JsonStatsWriter writer(out);
    values->enumerate_all(writer);
    if (metrics)
    {
      for (const auto& metric : metrics->get_values())
      {
        std::string client_name;
        std::string field;
        if (parse_last_error_metric(metric.first, client_name, field))
        {
          auto& error = errors[client_name];
          boost::apply_visitor(
            GrpcErrorFieldVisitor(error, field),
            metric.second);
          continue;
        }

        writer.append_key_(metric.first);
        boost::apply_visitor(JsonMetricValueWriter(out), metric.second);
      }
    }
    writer.append_key_(std::string("errors"));
    out << '{';
    bool first_error = true;
    for (const auto& [client_name, error] : errors)
    {
      if (first_error)
      {
        first_error = false;
      }
      else
      {
        out << ',';
      }
      out << '"' << escape_json_string(client_name) << "\":{";
      out << "\"time\":\"" << escape_json_string(error.time) << "\",";
      out << "\"endpoint\":\"" << escape_json_string(error.endpoint) << "\",";
      out << "\"code\":" << error.code << ',';
      out << "\"message\":\"" << escape_json_string(error.message) << "\",";
      out << "\"source\":\"" << escape_json_string(error.source) << "\"";
      out << '}';
    }
    out << '}';
    out << "}\n";
    return out.str();
  }
}

namespace AdServer::Frontends
{
  FCGIServer::FCGIServer() /*throw(eh::Exception)*/
    : Logging::LoggerCallbackHolder(
        Logging::Logger_var(new Logging::OStream::Logger(
          Logging::OStream::Config(std::cerr))),
        "FCGIServer", ASPECT, 0),
      stats_(new StatHolder()), // to remove ?
      composite_metrics_provider_(new Generics::CompositeMetricsProvider())
  {}

  void
  FCGIServer::read_config_(
    const char *filename,
    const char* argv0)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "FCGIServer::read_config()";

    try
    {
      Config::ErrorHandler error_handler;

      try
      {
        using namespace xsd::AdServer::Configuration;

        std::unique_ptr<AdConfigurationType>
          ad_configuration = AdConfiguration(filename, error_handler);

        if (error_handler.has_errors())
        {
          std::string error_string;
          throw Exception(error_handler.text(error_string));
        }

        config_.reset(
          new FCGIServerConfig(ad_configuration->FCGIServerConfig()));

        if (error_handler.has_errors())
        {
          std::string error_string;
          throw Exception(error_handler.text(error_string));
        }
      }
      catch (const xml_schema::parsing &ex)
      {
        Stream::Error ostr;
        ostr << "Can't parse config file '" << filename << "'. : ";
        if (error_handler.has_errors())
        {
          std::string error_string;
          ostr << error_handler.text(error_string);
        }
        throw Exception(ostr);
      }

      try
      {
        logger(Config::LoggerConfigReader::create(
                 config_->Logger(), argv0));
      }
      catch (const Config::LoggerConfigReader::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": got LoggerConfigReader::Exception: " << ex.what();
        throw Exception(ostr);
      }

      if(config_->Monitoring().present())
      {
        http_server_ = new AdServer::Commons::HttpServer::HttpServer(
          "0.0.0.0",
          config_->Monitoring()->port(),
          4);

        StatHolder_var stats = stats_;
        Generics::CompositeMetricsProvider_var metrics =
          composite_metrics_provider_;
        http_server_->add_handler(
          "/stats",
          [stats, metrics](
            const AdServer::Commons::HttpServer::HttpServer::Request&)
          {
            return AdServer::Commons::HttpServer::HttpServer::Response{
              200,
              "application/json",
              dump_stats_json(stats, metrics)
            };
          });

        add_child_object(http_server_);
      }
    }
    catch (const Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got Exception. Invalid configuration: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  void
  FCGIServer::init_fcgi_() /*throw(Exception)*/
  {
    static const char* FUN = "FCGIServer::init_fcgi_()";
    trace_startup("init_fcgi begin");

    try
    {
      FrontendsPool::ModuleIdArray modules;

      for(auto module_it = config_->Module().begin();
        module_it != config_->Module().end(); ++module_it)
      {
        if(module_it->name() == "bidding")
        {
          modules.push_back(FrontendsPool::M_BIDDING);
        }
        else if(module_it->name() == "pubpixel")
        {
          modules.push_back(FrontendsPool::M_PUBPIXEL);
        }
        else if(module_it->name() == "content")
        {
          modules.push_back(FrontendsPool::M_CONTENT);
        }
        else if(module_it->name() == "directory")
        {
          modules.push_back(FrontendsPool::M_DIRECTORY);
        }
        else if(module_it->name() == "webstat")
        {
          modules.push_back(FrontendsPool::M_WEBSTAT);
        }
        else if(module_it->name() == "action")
        {
          modules.push_back(FrontendsPool::M_ACTION);
        }
        else if(module_it->name() == "userbind")
        {
          modules.push_back(FrontendsPool::M_USERBIND);
        }
        else if(module_it->name() == "passback")
        {
          modules.push_back(FrontendsPool::M_PASSBACK);
        }
        else if(module_it->name() == "passbackpixel")
        {
          modules.push_back(FrontendsPool::M_PASSBACKPIXEL);
        }
        else if(module_it->name() == "optout")
        {
          modules.push_back(FrontendsPool::M_OPTOUT);
        }
        else if(module_it->name() == "nullad")
        {
          modules.push_back(FrontendsPool::M_NULLAD);
        }
        else if(module_it->name() == "adinst")
        {
          modules.push_back(FrontendsPool::M_ADINST);
        }
        else if(module_it->name() == "click")
        {
          modules.push_back(FrontendsPool::M_CLICK);
        }
        else if(module_it->name() == "imprtrack")
        {
          modules.push_back(FrontendsPool::M_IMPRTRACK);
        }
        else if(module_it->name() == "ad")
        {
          modules.push_back(FrontendsPool::M_AD);
        }
        else
        {
          Stream::Error ostr;
          ostr << "unknown module name '" << module_it->name() << "'";
          throw Exception(ostr);
        }
      }

      // pass CompositeMetricsProvider here
      trace_startup("create FrontendsPool begin");
      FrontendCommons::Frontend_var frontend_pool = new FrontendsPool(
        config_->fe_config().data(),
        modules,
        logger(),
        stats_,
        composite_metrics_provider_,
        config_->grpc_coalesce_threads(),
        config_->service_index());
      trace_startup("create FrontendsPool end");

      trace_startup("FrontendsPool init begin");
      frontend_pool->init();
      trace_startup("FrontendsPool init end");

      frontend_pool_ = frontend_pool;
      add_child_object(frontend_pool.in());

      for(auto bind_it = config_->BindSocket().begin(); bind_it != config_->BindSocket().end();
        ++bind_it)
      {
        trace_startup("create FCGIAcceptor begin");
        add_child_object(
          Generics::ActiveObject_var(
            new FCGIAcceptor(
              logger(),
              frontend_pool,
              callback(),
              bind_it->bind(), // bind_it->bind().data(),
              bind_it->backlog(),
              bind_it->accept_threads())));
        trace_startup("create FCGIAcceptor end");
      }

      if(config_->Http2Endpoint().present())
      {
        const auto& http2_endpoint = config_->Http2Endpoint().get();

        trace_startup("create Http2Acceptor begin");
        add_child_object(
          Generics::ActiveObject_var(
            new Http2Acceptor(
              logger(),
              frontend_pool,
              http2_endpoint.bind(),
              http2_endpoint.port(),
              http2_endpoint.threads(),
              http2_endpoint.max_concurrent_streams(),
              http2_endpoint.read_buffer_size(),
              http2_endpoint.max_request_size())));
        trace_startup("create Http2Acceptor end");
      }

    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  FCGIServer::main(int& argc, char** argv) noexcept
  {
    static const char* FUN = "FCGIServer::main()";

    try
    {
      trace_startup("XMLUtility initialize begin");
      XMLUtility::initialize();
      trace_startup("XMLUtility initialize end");
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-205") << FUN << ": Got eh::Exception: " << ex.what();
      return;
    }

    std::unique_ptr<AdServer::Commons::PidFileGuard> pid_file_guard;

    try
    {
      if (argc < 2)
      {
        Stream::Error ostr;
        ostr << "config file or colocation config file is not specified\n"
          "usage: FCGIServer <config_file>";
        throw Exception(ostr);
      }

      try
      {
        trace_startup("read_config begin");
        read_config_(argv[1], argv[0]);
        trace_startup("read_config end");
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << "Can't parse config file '" << argv[1] << "': " <<
          ex.what();
        throw Exception(ostr);
      }
      catch(...)
      {
        Stream::Error ostr;
        ostr << "Unknown Exception at parsing of config.";
        throw Exception(ostr);
      }

      pid_file_guard = std::make_unique<AdServer::Commons::PidFileGuard>(
        std::string(config_->pid_file()));
      init_fcgi_();
      AdServer::Commons::SignalActiveObject signal_active_object;
      trace_startup("activate_object begin");
      activate_object();
      trace_startup("activate_object end");
      logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";
      trace_startup("service started");

      signal_active_object.wait_object();
      deactivate_object();
      wait_object();
      logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";
      XMLUtility::terminate();
    }
    catch (const Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Got BiddingFCGIServerApp_::Exception: " <<
        e.what();
      logger()->log(
        ostr.str(),
        Logging::Logger::CRITICAL,
        ASPECT,
        "ADS-IMPL-150");
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Got eh::Exception: " << e.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-150");
    }
    catch (...)
    {
      Stream::Error ostr;
      ostr << FUN << ": Got unknown exception";
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-150");
    }
  }
} // namespace AdServer::Frontends

int
main(int argc, char** argv)
{
  AdServer::Frontends::FCGIServer* app = 0;

  try
  {
    app = &AdServer::Frontends::FCGIServerApp::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  if (app == 0)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return -1;
  }

  try
  {
    app->main(argc, argv);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    return -1;
  }

  return 0;
}
