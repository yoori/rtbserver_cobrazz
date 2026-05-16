#pragma once

#include <eh/Exception.hpp>
#include <Generics/DirSelector.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include <Logger/SimpleLogger.hpp>
#include <Logger/FileLogger.hpp>
#include <Logger/Syslog.hpp>
#include <Logger/DistributorLogger.hpp>
#include <Commons/Oracle/ConnectionDescription.hpp>
#include <Commons/Grpc/GrpcClient.hpp>

#include <xsd/AdServerCommons/AdServerCommons.hpp>

namespace Config
{
  class LoggerConfigReader
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static
    Logging::Logger_var
    create(const xsd::AdServer::Configuration::ErrorLoggerType&
      xml_logger_config, const char* argv0 = 0) /*throw(Exception)*/;
  };

  void
  read_db_connection(
    AdServer::Commons::Oracle::ConnectionDescription& conn,
    const xsd::AdServer::Configuration::DBConnectionType& db_conn_config)
    noexcept;

  AdServer::Grpc::BatchingOptions
  read_xsd_grpc_options(
    const xsd::AdServer::Configuration::GrpcBatchingOptionsType& config);
}

namespace Config
{
  inline
  Logging::Logger_var
  LoggerConfigReader::create(
    const xsd::AdServer::Configuration::ErrorLoggerType& xml_logger_config,
    const char* argv0) /*throw(Exception)*/
  {
    static const char SYSLOG_PREFIX[] = "FOROS.";

    if (xml_logger_config.filename().empty())
    {
      throw Exception("LoggerConfigReader::create(): empty file name");
    }

    const std::string& filename = xml_logger_config.filename();

    try
    {
      ReferenceCounting::Deque<Logging::QLogger_var> loggers;
      for (xsd::AdServer::Configuration::ErrorLoggerType::Suffix_sequence::
        const_iterator it = xml_logger_config.Suffix().begin();
        it != xml_logger_config.Suffix().end(); ++it)
      {
        Logging::File::Policies::PolicyList log_policies;

        std::string log_file_name = filename + it->name();
        if (it->size_span().present())
        {
          log_policies.push_back(
            new Logging::File::Policies::SizeSpanPolicy(
              it->size_span().get()));
        }

        if (it->time_span().present())
        {
          log_policies.push_back(
            new Logging::File::Policies::TimeSpanPolicy(
              Generics::Time(it->time_span().get())));
        }

        Logging::File::Config config(
          log_file_name.c_str(),
          log_policies,
          xml_logger_config.log_level() > it->max_log_level() ?
            it->max_log_level() : xml_logger_config.log_level());

        Logging::QLogger_var file_logger(
          new Logging::File::Logger(std::move(config)));

        if (it->min_log_level().present())
        {
          loggers.push_back(Logging::QLogger_var(
            new Logging::SeveritySelectorLogger(file_logger,
              it->min_log_level().get())));
        }
        else
        {
          loggers.push_back(Logging::QLogger_var(
            new Logging::SeveritySelectorLogger(
              it->max_log_level(), file_logger)));
        }
      }

      if (xml_logger_config.SysLog().present())
      {
        Logging::Logger_var sys_logger(
          new Logging::Syslog::Logger(Logging::Syslog::Config(
            xml_logger_config.SysLog().get().log_level(),
            argv0 ? (std::string(SYSLOG_PREFIX) +
              Generics::DirSelect::file_name(argv0)).c_str() : "")));

        if (loggers.empty())
        {
          return sys_logger;
        }

        loggers.push_back(Logging::QLogger_var(
          new Logging::SeveritySelectorLogger(sys_logger,
          Logging::Logger::EMERGENCY,
          Logging::Logger::NOTICE)));
      }

      if (loggers.size() == 1)
      {
        return loggers[0];
      }

      return Logging::Logger_var(
        new Logging::DistributorLogger(loggers.begin(), loggers.end()));
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LoggerConfigReader::create(): Can't init logger. "
        "Caught eh::Exception. : " << ex.what();
      throw Exception(ostr);
    }
  }

  inline
  void
  read_db_connection(
    AdServer::Commons::Oracle::ConnectionDescription& conn,
    const xsd::AdServer::Configuration::DBConnectionType& db_conn_config)
    noexcept
  {
    conn.db = db_conn_config.db();
    conn.user_name = db_conn_config.user();
    conn.password = db_conn_config.password();
    conn.schema = db_conn_config.schema().present() ?
      db_conn_config.schema()->c_str() : "";
    conn.statement_timeout = db_conn_config.statement_timeout().present() ?
      Generics::Time(*db_conn_config.statement_timeout()) :
      Generics::Time::ZERO;
  }

  inline
  AdServer::Grpc::BatchingOptions
  read_xsd_grpc_options(
    const xsd::AdServer::Configuration::GrpcBatchingOptionsType& config)
  {
    AdServer::Grpc::BatchingOptions options;
    options.channels_number = config.channels_number();
    options.max_batch_size = config.max_batch_size();
    if (config.max_inflight().present())
    {
      options.max_inflight = *config.max_inflight() > 0 ?
        std::optional<std::size_t>(*config.max_inflight()) :
        std::nullopt;
    }
    options.error_on_inflight_reaching = config.error_on_inflight_reaching();
    if (config.max_outstanding_requests().present())
    {
      options.max_outstanding_requests = *config.max_outstanding_requests() > 0 ?
        std::optional<std::size_t>(*config.max_outstanding_requests()) :
        std::nullopt;
    }
    options.workers_number = config.workers_number();
    options.hot_buckets_count = config.hot_buckets_count();
    if (config.max_batch_delay_us().present())
    {
      options.max_batch_delay = *config.max_batch_delay_us() > 0 ?
        std::optional<Generics::Time>(
          Generics::Time(
            *config.max_batch_delay_us() / Generics::Time::USEC_MAX,
            *config.max_batch_delay_us() % Generics::Time::USEC_MAX)) :
        std::nullopt;
    }

    if (config.max_queue_wait_us().present())
    {
      options.max_queue_wait = *config.max_queue_wait_us() > 0 ?
        std::optional<Generics::Time>(
          Generics::Time(
            *config.max_queue_wait_us() / Generics::Time::USEC_MAX,
            *config.max_queue_wait_us() % Generics::Time::USEC_MAX)) :
        std::nullopt;
    }

    if (config.stream_start_timeout_us().present())
    {
      options.stream_start_timeout = *config.stream_start_timeout_us() > 0 ?
        std::optional<Generics::Time>(
          Generics::Time(
            *config.stream_start_timeout_us() / Generics::Time::USEC_MAX,
            *config.stream_start_timeout_us() % Generics::Time::USEC_MAX)) :
        std::nullopt;
    }

    options.enable_grpc_compression = config.enable_grpc_compression();
    options.use_local_subchannel_pool = config.use_local_subchannel_pool();
    options.reconnect_period = Generics::Time(config.reconnect_period());
    return options;
  }
}
