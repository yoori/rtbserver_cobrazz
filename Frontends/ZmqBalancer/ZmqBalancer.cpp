#include <cstdlib>
#include <iostream>

#include <XMLUtility/Utility.hpp>
#include <CORBACommons/StatsImpl.hpp>

#include <Commons/ConfigUtils.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/DelegateTaskGoal.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ZmqConfig.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Frontends/ProfilingServer/DMPProfilingInfo.hpp>

#include "ZmqBalancer.hpp"

namespace
{
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
  const char PROCESS_STATS_CONTROL_OBJ_KEY[] = "ProcessStatsControl";
  const char ROUTER_SOCKETNAME[] = "inproc://balancer";
}

namespace AdServer
{
/*
 * ZmqBalancer_::ZmqWorker
 */
ZmqBalancer_::ZmqWorker::ZmqWorker(
  Generics::ActiveObjectCallback* callback,
  zmq::context_t& context,
  ZmqStreamStats* stats,
  const SocketConfig& config,
  unsigned long worker_number,
  unsigned long worker_threads,
  const char* address,
  const char* router_socketname)
 : AdServer::Commons::DelegateActiveObject(
      std::bind(&ZmqWorker::work_, this),
      callback, worker_threads),
   callback_(ReferenceCounting::add_ref(callback)),
   context_(context),
   stats_(ReferenceCounting::add_ref(stats)),
   config_(config),
   worker_number_(worker_number),
   address_(address),
   router_socketname_(router_socketname)
{}

void
ZmqBalancer_::ZmqWorker::work_() noexcept
{
  // Dealer socket
  zmq::socket_t dealer(context_, ZMQ_DEALER);
  char identity[40];
  Config::ZmqConfigReader::set_socket_params(config_, dealer);
  String::StringManip::int_to_str(worker_number_, identity, sizeof(identity));
  dealer.setsockopt(ZMQ_IDENTITY, identity, strlen(identity));
  dealer.connect(router_socketname_.c_str());

  // Connect socket
  zmq::socket_t worker(
    context_,
    Config::ZmqConfigReader::get_socket_type(config_.type()));
  Config::ZmqConfigReader::set_socket_params(config_, worker);
  worker.connect(address_.c_str());
  
  const int flags = (config_.non_block() ? ZMQ_NOBLOCK : 0);

  // Work cycle
  while (active())
  {
    try
    {
      zmq::message_t msg;
      if(dealer.recv(&msg))
      {
        const std::size_t msg_size = msg.size();
        
        if (worker.send(msg, flags))
        {
          stats_->add_sent_messages(1, msg_size);
        }
        else
        {
          stats_->add_dropped_messages(1);
        }
      }
    }
    catch (const zmq::error_t& ex)
    {
      if (ex.num() != ETERM)
      {
        Stream::Error ostr;
        ostr << "ZmqBalancer_::ZmqWorker::work_: got exception: " <<
          ex.what();
        callback_->critical(ostr.str());
      }
    }
  }

  // Close ZMQ sockets
  dealer.close();
  worker.close();
}

/*
 * ZmqBalancer_::ZmqClient
 */
ZmqBalancer_::ZmqClient::ZmqClient(
  Generics::ActiveObjectCallback* callback,
  zmq::context_t& context,
  ZmqStreamStats* stats,
  const SocketConfig& config,
  unsigned long workers_count,
  const char* router_socketname)
  : AdServer::Commons::DelegateActiveObject(
      std::bind(&ZmqClient::work_, this),
      callback),
    callback_(ReferenceCounting::add_ref(callback)),
    context_(context),
    stats_(ReferenceCounting::add_ref(stats)),
    config_(config),
    workers_count_(workers_count),
    router_socketname_(router_socketname)
{}

void
ZmqBalancer_::ZmqClient::work_() noexcept
{
  // Broker socket
  zmq::socket_t broker(context_, ZMQ_ROUTER);
  Config::ZmqConfigReader::set_socket_params(config_, broker);
  broker.bind(router_socketname_.c_str());

  // Client socket
  zmq::socket_t client(
    context_,
    Config::ZmqConfigReader::get_socket_type(config_.type()));
    
  Config::ZmqConfigReader::set_socket_params(config_, client);

  for(auto it = config_.Address().begin(); it != config_.Address().end(); ++it)
  {
    client.bind(Config::ZmqConfigReader::get_address(*it).c_str());
  }

  // Work cycle
  while (active())
  {
    try
    {
      zmq::message_t msg;
      if(client.recv(&msg))
      {
        const std::size_t msg_size = msg.size();

        stats_->add_received_messages(1, msg_size);

        AdServer::Profiling::DMPProfilingInfoReader
          dmp_profiling_info(msg.data(), msg_size);

        size_t worker_number =
          AdServer::Commons::external_id_distribution_hash(
            String::SubString(
              dmp_profiling_info.external_user_id())) %
          workers_count_;

        char identity[40];
        String::StringManip::int_to_str(worker_number, identity, sizeof(identity));

        if (broker.send(identity, strlen(identity), ZMQ_SNDMORE))
        {
          if (!broker.send(msg))
          {
            stats_->add_dropped_messages(1);
          }
        }
        else
        {
          stats_->add_dropped_messages(1);
        }
      }
    }
    catch (const zmq::error_t& ex)
    {
      if (ex.num() != ETERM)
      {
        Stream::Error ostr;
        ostr << "ZmqBalancer_::ZmqClient::work_: got exception: " <<
          ex.what();
        callback_->critical(ostr.str());
      }
    }
  }

  // Close ZMQ sockets
  client.close();
  broker.close();
}

/*
 * ZmqBalancer_
 */
ZmqBalancer_::ZmqBalancer_(const char* aspect) /*throw(eh::Exception)*/
  : AdServer::Commons::ProcessControlVarsLoggerImpl(
      "ZmqBalancer_", aspect),
    ASPECT_(aspect),
    stats_(new ZmqStreamStats()),
    task_runner_(new Generics::TaskRunner(callback(), 1)),
    scheduler_(new Generics::Planner(callback()))
{
  add_child_object(task_runner_);
  add_child_object(scheduler_);
}

ReferenceCounting::QualPtr<ZmqBalancer_>
ZmqBalancer_::create(char** argv) /*throw(eh::Exception)*/
{
  static const char* FUN = "ZmqBalancer_::create()";
  Config::ErrorHandler error_handler;
  ZmqBalancer_var app;

  try
  {
    using namespace xsd::AdServer::Configuration;

    std::unique_ptr<AdConfigurationType>
      ad_configuration = AdConfiguration(argv[1], error_handler);

    if (error_handler.has_errors())
    {
      std::string error_string;
      throw Exception(error_handler.text(error_string));
    }

    app = new ZmqBalancer_(ad_configuration->ZmqBalancerConfig().app_name().c_str());

    app->config_.reset(
      new ZmqBalancerConfig(ad_configuration->ZmqBalancerConfig()));

    if (error_handler.has_errors())
    {
      std::string error_string;
      throw Exception(error_handler.text(error_string));
    }
  }
  catch (const xml_schema::parsing &ex)
  {
    Stream::Error ostr;
    ostr << "Can't parse config file '" << argv[1] << "'. : ";
    if (error_handler.has_errors())
    {
      std::string error_string;
      ostr << error_handler.text(error_string);
    }
    throw Exception(ostr);
  }

  try
  {
    Config::CorbaConfigReader::read_config(
      app->config_->CorbaConfig(),
      app->corba_config_);
  }
  catch(const eh::Exception &ex)
  {
    Stream::Error ostr;
    ostr << FUN << ": Can't read Corba Config. : " << ex.what();
    throw Exception(ostr);
  }

  try
  {
    app->logger(Config::LoggerConfigReader::create(
      app->config_->Logger(), argv[0]));
  }
  catch (const Config::LoggerConfigReader::Exception &ex)
  {
    Stream::Error ostr;
    ostr << FUN << ": got LoggerConfigReader::Exception: " << ex.what();
    throw Exception(ostr);
  }

  return app;
}

void
ZmqBalancer_::main() noexcept
{
  try
  {
    register_vars_controller();
    init_corba_();
    init_zeromq_();
    activate_object();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT_.c_str()) << "service started.";
    corba_server_adapter_->run();
    wait();
    logger()->sstream(Logging::Logger::NOTICE, ASPECT_.c_str()) << "service stopped.";
  }
  catch (const Exception &ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY,
      ASPECT_.c_str(),
      "ADS-IMPL-205") <<
      "ZmqBalancer_::main(): Got ZmqBalancer_::Exception: " << ex.what();
  }
  catch (const eh::Exception &ex)
  {
    logger()->sstream(Logging::Logger::EMERGENCY, ASPECT_.c_str(),
      "ADS-IMPL-205") <<
      "ZmqBalancer_::main(): Got eh::Exception: " << ex.what();
  }
}

void
ZmqBalancer_::init_zeromq_() /*throw(Exception)*/
{
  try
  {
    zmq_context_.reset(new zmq::context_t(config_->zmq_io_threads()));

    unsigned long br_i = 0;

    for(auto br_it = config_->BalancingRoute().begin();
        br_it != config_->BalancingRoute().end(); ++br_it, ++br_i)
    {
      std::ostringstream router_socketname_ostr;
      router_socketname_ostr << ROUTER_SOCKETNAME;
      router_socketname_ostr << br_i;

      const std::string router_socketname = router_socketname_ostr.str();

      const ZmqBalancerConfig::BalancingRoute_type& route_config = *br_it;

      // Initialize clients
      ZmqClient_var client = new ZmqClient(
        callback(),
        *zmq_context_,
        stats_,
        route_config.BindSocket(),
        route_config.ConnectSocket().Address().size(),
        router_socketname.c_str());
      add_child_object(client);
    
      // Initialize workers
      {
        const SocketConfig& connect_config = route_config.ConnectSocket();
        unsigned long worker_number(0);
        for(auto it = connect_config.Address().begin();
          it != connect_config.Address().end(); ++it)
        {
          ZmqWorker_var worker = new ZmqWorker(
            callback(),
            *zmq_context_,
            stats_,
            connect_config,
            worker_number++,
            route_config.work_threads(),
            Config::ZmqConfigReader::get_address(*it).c_str(),
            router_socketname.c_str());

          add_child_object(worker);
        }
      }
    }  
  }
  catch (const eh::Exception& e)
  {
    Stream::Error ostr;
    ostr << "ZmqBalancer_::init_zeromq_(): Got exception: " << e.what();
    throw Exception(ostr);
  }
}

void
ZmqBalancer_::read_config_(
  const char *filename,
  const char* argv0)
  /*throw(Exception, eh::Exception)*/
{
  static const char* FUN = "ZmqBalancer_::read_config()";

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
        new ZmqBalancerConfig(ad_configuration->ZmqBalancerConfig()));

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
      Config::CorbaConfigReader::read_config(
        config_->CorbaConfig(),
        corba_config_);
    }
    catch(const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't read Corba Config. : " << ex.what();
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
ZmqBalancer_::shutdown(CORBA::Boolean wait_for_completion)
  noexcept
{
  {
    SyncPolicy::WriteGuard guard(shutdown_lock_);
    deactivate_object();
    zmq_context_.reset();
    wait_object();
  }

  CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
}

void
ZmqBalancer_::init_corba_() /*throw(Exception)*/
{
  typedef CORBACommons::ProcessStatsGen<ZmqStreamStats>
    ProcessStatsImpl;

  try
  {
    corba_server_adapter_ =
      new CORBACommons::CorbaServerAdapter(corba_config_);
    shutdowner_ = corba_server_adapter_->shutdowner();

    corba_server_adapter_->add_binding(PROCESS_CONTROL_OBJ_KEY, this);

    CORBACommons::POA_ProcessStatsControl_var proc_stat_ctrl =
      new ProcessStatsImpl(stats_);

    corba_server_adapter_->add_binding(
      PROCESS_STATS_CONTROL_OBJ_KEY, proc_stat_ctrl.in());
  }
  catch (const eh::Exception &ex)
  {
    Stream::Error ostr;
    ostr << "ZmqBalancer_::init_corba(): "
        << "Can't init CorbaServerAdapter. : " << ex.what();
    throw Exception(ostr);
  }
}
} // AdServer

int
main(int argc, char** argv)
{
  int ret_code = 0;

  try
  {
    if (argc < 2)
    {
      const char *usage = "usage: ZmqBalancer <config_file>";

      Stream::Error ostr;
      ostr << "config file or colocation config file is not specified\n" <<
        usage;
      throw std::runtime_error(ostr.str().data());
    }

    XMLUtility::initialize();
    AdServer::ZmqBalancer_var app = AdServer::ZmqBalancer_::create(argv);
    app->main();
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    ret_code = -1;
  }

  XMLUtility::terminate();
  return ret_code;
}
