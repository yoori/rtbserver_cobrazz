// STD
#include <cstdlib>
#include <sstream>

// UNIXCOMMONS
#include <CORBACommons/StatsImpl.hpp>
#include <XMLUtility/Utility.hpp>

// THIS
#include <Commons/ConfigUtils.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <Commons/ZmqConfig.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/ProfilingServer/ProfilingServer.hpp>
#include <UserInfoSvcs/UserInfoManager/ProtoConvertor.hpp>

namespace
{
  const char ASPECT[] = "ProfilingServer";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
  const char PROCESS_STATS_CONTROL_OBJ_KEY[] = "ProcessStatsControl";
  const char INPROC_DMP_PROFILING_INFO_ADDRESS[] = "inproc://dmp_profiling_info";
}

namespace
{
  struct ChannelMatch final
  {
    ChannelMatch(
      const unsigned long channel_id_val,
      const unsigned long channel_trigger_id_val)
      : channel_id(channel_id_val),
        channel_trigger_id(channel_trigger_id_val)
    {
    }

    bool operator<(const ChannelMatch& right) const
    {
      return (channel_id < right.channel_id ||
        (channel_id == right.channel_id &&
          channel_trigger_id < right.channel_trigger_id));
    }

    unsigned long channel_id;
    unsigned long channel_trigger_id;
  };

  struct GetChannelTriggerId final
  {
    ChannelMatch operator() (
      const AdServer::ChannelSvcs::Proto::ChannelAtom& atom) noexcept
    {
      return ChannelMatch(atom.id(), atom.trigger_channel_id());
    }
  };
}

namespace AdServer::Profiling
{
  namespace
  {
    const unsigned long TIME_PRECISION = 30;
    
    struct DMPProfileHash final
    {
      static const int COORD_PRECISION = 10000;
      
      DMPProfileHash(
        const unsigned long time_precision,
        const DMPProfilingInfoReader& dmp_profiling_info)
        : time_precision_(time_precision),
          hash_(0)
      {
        assert(time_precision);
        calc(dmp_profiling_info);
      }

      static int trunc_coord(const int coord)
      {
        return coord * COORD_PRECISION / COORD_PRECISION;
      }
      
      void calc(const DMPProfilingInfoReader& dmp_profiling_info)
      {
        Generics::Murmur64Hash hasher(hash_);
        hash_add(hasher,
          dmp_profiling_info.time() / time_precision_ * time_precision_);
        hash_add(hasher,
          String::SubString(dmp_profiling_info.source()));
        hash_add(hasher,
          String::SubString(dmp_profiling_info.external_user_id()));
        hash_add(hasher,
          String::SubString(dmp_profiling_info.bind_user_ids()));
        hash_add(hasher,
          String::SubString(dmp_profiling_info.url()));
        hash_add(hasher,
          String::SubString(dmp_profiling_info.keywords()));
        hash_add(hasher,
          trunc_coord(dmp_profiling_info.longitude()));
        hash_add(hasher,
          trunc_coord(dmp_profiling_info.latitude()));
      }

      uint64_t hash() const noexcept
      {
        return hash_;
      }

    protected:
      const unsigned long time_precision_;
      uint64_t hash_;
    };
  }

  ProfilingServer::ZmqRouter::ZmqRouter(
    Generics::ActiveObjectCallback* callback,
    zmq::context_t& zmq_context,
    const xsd::AdServer::Configuration::ZmqSocketType& socket_config,
    const char* inproc_address)
    : Commons::DelegateActiveObject(callback),
      zmq_context_(zmq_context),
      bind_socket_(
        zmq_context,
        Config::ZmqConfigReader::get_socket_type(socket_config.type())),
      inproc_socket_(zmq_context, ZMQ_PUSH),
      callback_(ReferenceCounting::add_ref(callback))
  {
    std::ostringstream addresses;

    try
    {
      Config::ZmqConfigReader::set_socket_params(socket_config, bind_socket_);

      for (auto it = socket_config.Address().begin();
           it != socket_config.Address().end();
           ++it)
      {
        const std::string address = Config::ZmqConfigReader::get_address(*it);
        bind_socket_.bind(address.c_str());
        addresses << "'" << address << "' ";
      }

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(3, 0, 0)
      const int hwm = 1;
      inproc_socket_.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
      inproc_socket_.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
#else
      const uint64_t hwm = 1;
      inproc_socket_.setsockopt(ZMQ_HWM, &hwm, sizeof(hwm));
#endif

      inproc_socket_.bind(inproc_address);
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "ProfilingServer::ZmqRouter::ZmqRouter: Failed open socket: " <<
        addresses.str() << ". Got exception:" << ex.what();
      throw Exception(ostr);
    }
  }

  void ProfilingServer::ZmqRouter::work_() noexcept
  {
    while (active())
    {
      try
      {
        zmq::message_t msg;
        if (bind_socket_.recv(&msg))
        {
          inproc_socket_.send(msg);
        }
      }
      catch (const zmq::error_t& exc)
      {
        if (exc.num() != ETERM)
        {
          Stream::Error ostr;
          ostr << "ProfilingServer::ZmqRouter::route_to_workers_: got exception: "
               << exc.what();
          callback_->critical(ostr.str());
        }
      }
    }

    bind_socket_.close();
    inproc_socket_.close();
  }

  ProfilingServer::ZmqWorker::ZmqWorker(
    Worker worker,
    Generics::ActiveObjectCallback* callback,
    zmq::context_t& zmq_context,
    const char* inproc_address,
    std::size_t work_threads)
    : Commons::DelegateActiveObject(
        std::bind(&ZmqWorker::do_work_, this, inproc_address),
        callback,
        work_threads),
      zmq_context_(zmq_context),
      callback_(ReferenceCounting::add_ref(callback)),
      worker_(worker)
  {}

  void ProfilingServer::ZmqWorker::do_work_(
    const char* inproc_address) noexcept
  {
    try
    {
      zmq::socket_t socket(zmq_context_, ZMQ_PULL);

#if ZMQ_VERSION >= ZMQ_MAKE_VERSION(3, 0, 0)
      const int hwm = 1;
      socket.setsockopt(ZMQ_RCVHWM, &hwm, sizeof(hwm));
      socket.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
#else
      const uint64_t hwm = 1;
      socket.setsockopt(ZMQ_HWM, &hwm, sizeof(hwm));
#endif

      socket.connect(inproc_address);

      while (active())
      {
        try
        {
          zmq::message_t msg;
          if(socket.recv(&msg))
          {
            worker_(msg);
          }
        }
        catch (const zmq::error_t& exc)
        {
          if (exc.num() != ETERM)
          {
            Stream::Error ostr;
            ostr << FNS
                 << "got Exception: "
                 << exc.what();
            callback_->critical(ostr.str());
          }
        }
      }
    }
    catch (eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "socket open/close failed: "
           << exc.what();
      callback_->critical(ostr.str());
      std::abort();
    }
  }

  ProfilingServer::ProfilingServer()
    : AdServer::Commons::ProcessControlVarsLoggerImpl(
        "ProfilingServer", ASPECT),
      logger_callback_holder_(logger(), "ProfilingServer", ASPECT, 0),
      streamer_(new DMPKafkaStreamer),
      stats_(new ProfilingServerStats(streamer_)),
      hash_time_precision_(0)
  {}

  void ProfilingServer::main(int& argc, char** argv) noexcept
  {
    try
    {
      XMLUtility::initialize();
    }
    catch (const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT,
        "ADS-IMPL-205") << FNS << "Got eh::Exception: " << ex.what();
      return;
    }

    try
    {
      if (argc < 2)
      {
        Stream::Error ostr;
        ostr << "config file or colocation config file is not specified\n"
             << "usage: ProfilingServer <config_file>";
        throw Exception(ostr);
      }

      try
      {
        read_config_(argv[1], argv[0]);
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error ostr;
        ostr << "Can't parse config file '" << argv[1] << "'. "
            << ": " << exc.what();
        throw Exception(ostr);
      }
      catch (...)
      {
        Stream::Error ostr;
        ostr << "Unknown Exception at parsing of config.";
        throw Exception(ostr);
      }

      if (config_->KafkaDMPProfilingStorage().present())
      {
        streamer_->register_producers(
          logger(), callback(), this,
          config_->KafkaDMPProfilingStorage().get());
       }

      if (config_->DMPProfileFilter().present())
      {
        const ::xsd::AdServer::Configuration::DMPProfileFilterType&
          filter_config = config_->DMPProfileFilter().get();
        
        hash_filter_ =
          new HashFilter(
            filter_config.chunk_count(),
            filter_config.chunk_size(),
            Generics::Time(filter_config.keep_time_period()),
            Generics::Time(filter_config.keep_time()));

        const unsigned long time_precision =
          filter_config.time_precision();

        hash_time_precision_ =
          (time_precision / TIME_PRECISION +
            (time_precision % TIME_PRECISION || time_precision == 0 ?
              1 : 0)) * TIME_PRECISION;
      }
      
      // init request info filler
      request_info_filler_.reset(
        new RequestInfoFiller(
          logger(),
          0, // colo id
          RequestInfoFiller::ExternalUserIdSet(),
          config_->bind_url_suffix().present() ?
            *config_->bind_url_suffix() : String::SubString(),
          config_->debug_on()));

      register_vars_controller();
      init_coroutine_();
      init_grpc_();
      init_corba_();
      init_zeromq_();
      activate_object();

      logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service started.";

      corba_server_adapter_->run();
      wait();

      logger()->sstream(Logging::Logger::NOTICE, ASPECT) << "service stopped.";

      XMLUtility::terminate();
    }
    catch (const Exception &ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-IMPL-205") << FNS <<
        ": Got ProfilingServer::Exception: " << ex.what();
    }
    catch (const eh::Exception &ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT,
        "ADS-IMPL-205") << FNS <<
        ": Got eh::Exception: " << ex.what();
    }

    XMLUtility::terminate();
  }

  void ProfilingServer::shutdown(CORBA::Boolean wait_for_completion) noexcept
  {
    deactivate_object();
    zmq_context_.reset();
    wait_object();

    CORBACommons::ProcessControlImpl::shutdown(wait_for_completion);
  }

  CORBACommons::IProcessControl::ALIVE_STATUS
  ProfilingServer::is_alive() noexcept
  {
    return CORBACommons::ProcessControlImpl::is_alive();
  }

  void ProfilingServer::read_config_(
    const char* filename,
    const char* argv0)
  {
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
          new ProfilingServerConfig(ad_configuration->ProfilingServerConfig()));

        if (error_handler.has_errors())
        {
          std::string error_string;
          throw Exception(error_handler.text(error_string));
        }
      }
      catch (const xml_schema::parsing& ex)
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
      catch (const eh::Exception& exc)
      {
        Stream::Error ostr;
        ostr << FNS
             << "Can't read Corba Config: "
             << exc.what();
        throw Exception(ostr);
      }

      try
      {
        logger(Config::LoggerConfigReader::create(
          config_->Logger(), argv0));
        logger_callback_holder_.logger(logger());
      }
      catch (const Config::LoggerConfigReader::Exception& exc)
      {
        Stream::Error ostr;
        ostr << FNS
             << "got LoggerConfigReader::Exception: "
             << exc.what();
        throw Exception(ostr);
      }
    }
    catch (const Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "got Exception. Invalid configuration: "
           << exc.what();
      throw Exception(ostr);
    }
  }

  void ProfilingServer::init_coroutine_()
  {
    auto task_processor_container_builder =
      Config::create_task_processor_container_builder(
        logger(),
        config_->Coroutine());
    auto init_func = [] (TaskProcessorContainer& task_processor_container) {
      return std::make_unique<ComponentsBuilder>();
    };

    manager_coro_ = new ManagerCoro(
        std::move(task_processor_container_builder),
        std::move(init_func),
        logger());
    add_child_object(manager_coro_);
  }

  void ProfilingServer::init_grpc_()
  {
    auto number_scheduler_threads = std::thread::hardware_concurrency();
    if (number_scheduler_threads == 0)
    {
      Stream::Error stream;
      stream << FNS
             << "hardware_concurrency is failed";
      throw Exception(stream);
    }

    SchedulerPtr scheduler = UServerUtils::Grpc::Common::Utils::create_scheduler(
      number_scheduler_threads,
      logger());

    auto grpc_channel_operation_pool = create_grpc_channel_operation_pool(
      scheduler,
      manager_coro_->get_main_task_processor());
    auto grpc_campaign_manager_pool = create_grpc_campaign_manager_pool(
      scheduler,
      manager_coro_->get_main_task_processor());
    auto grpc_user_bind_operation_distributor = create_grpc_user_bind_operation_distributor(
      scheduler,
      manager_coro_->get_main_task_processor());
    auto grpc_user_info_operation_distributor = create_grpc_user_info_operation_distributor(
      scheduler,
      manager_coro_->get_main_task_processor());

    if (!grpc_campaign_manager_pool)
    {
      Stream::Error stream;
      stream << FNS
             << "grpc_campaign_manager_pool is null";
      throw Exception(stream);
    }

    if (!grpc_user_bind_operation_distributor)
    {
      Stream::Error stream;
      stream << FNS
             << "grpc_user_bind_operation_distributor is null";
      throw Exception(stream);
    }

    if (!grpc_channel_operation_pool)
    {
      Stream::Error stream;
      stream << FNS
             << "grpc_channel_operation_pool is null";
      throw Exception(stream);
    }

    if (!grpc_user_info_operation_distributor)
    {
      Stream::Error stream;
      stream << FNS
             << "grpc_user_info_operation_distributor is null";
      throw Exception(stream);
    }

    grpc_container_ = std::make_shared<FrontendCommons::GrpcContainer>();
    grpc_container_->grpc_channel_operation_pool = grpc_channel_operation_pool;
    grpc_container_->grpc_campaign_manager_pool = grpc_campaign_manager_pool;
    grpc_container_->grpc_user_bind_operation_distributor = grpc_user_bind_operation_distributor;
    grpc_container_->grpc_user_info_operation_distributor = grpc_user_info_operation_distributor;
  }

  ProfilingServer::GrpcUserBindOperationDistributor_var
  ProfilingServer::create_grpc_user_bind_operation_distributor(
    const SchedulerPtr& scheduler,
    TaskProcessor& task_processor)
  {
    GrpcUserBindOperationDistributor_var grpc_user_bind_operation_distributor;
    if (config_->UserBindGrpcClientPool().enable())
    {
      using ControllerRefList = AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor::ControllerRefList;
      using ControllerRef = AdServer::UserInfoSvcs::GrpcUserBindOperationDistributor::ControllerRef;

      ControllerRefList controller_groups;
      const auto& user_bind_controller_group = config_->UserBindControllerGroup();
      for (auto cg_it = std::begin(user_bind_controller_group);
        cg_it != std::end(user_bind_controller_group);
        ++cg_it)
      {
        ControllerRef controller_ref_group;
        Config::CorbaConfigReader::read_multi_corba_ref(
          *cg_it,
          controller_ref_group);
        controller_groups.push_back(controller_ref_group);
      }

      CORBACommons::CorbaClientAdapter_var corba_client_adapter = new CORBACommons::CorbaClientAdapter();
      const auto config_grpc_data = Config::create_pool_client_config(
        config_->UserBindGrpcClientPool());

      grpc_user_bind_operation_distributor = new GrpcUserBindOperationDistributor(
        logger(),
        task_processor,
        scheduler,
        controller_groups,
        corba_client_adapter.in(),
        config_grpc_data.first,
        config_grpc_data.second,
        Generics::Time::ONE_SECOND);
    }

    return grpc_user_bind_operation_distributor;
  }

  ProfilingServer::GrpcUserInfoOperationDistributor_var
  ProfilingServer::create_grpc_user_info_operation_distributor(
    const SchedulerPtr& scheduler,
    TaskProcessor& task_processor)
  {
    GrpcUserInfoOperationDistributor_var grpc_user_info_operation_distributor;
    if (config_->UserInfoGrpcClientPool().enable())
    {
      using ControllerRefList = AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor::ControllerRefList;
      using ControllerRef = AdServer::UserInfoSvcs::GrpcUserInfoOperationDistributor::ControllerRef;

      ControllerRefList controller_groups;
      const auto& user_info_controller_group = config_->UserInfoManagerControllerGroup();
      for (auto cg_it = std::begin(user_info_controller_group);
           cg_it != std::end(user_info_controller_group);
           ++cg_it)
      {
        ControllerRef controller_ref_group;
        Config::CorbaConfigReader::read_multi_corba_ref(
          *cg_it,
          controller_ref_group);
        controller_groups.push_back(controller_ref_group);
      }

      CORBACommons::CorbaClientAdapter_var corba_client_adapter = new CORBACommons::CorbaClientAdapter();
      const auto config_grpc_data = Config::create_pool_client_config(
        config_->UserInfoGrpcClientPool());

      grpc_user_info_operation_distributor = new GrpcUserInfoOperationDistributor(
        logger(),
        task_processor,
        scheduler,
        controller_groups,
        corba_client_adapter.in(),
        config_grpc_data.first,
        config_grpc_data.second,
        Generics::Time::ONE_SECOND);
    }

    return grpc_user_info_operation_distributor;
  }

  ProfilingServer::GrpcChannelOperationPoolPtr
  ProfilingServer::create_grpc_channel_operation_pool(
    const SchedulerPtr& scheduler,
    TaskProcessor& task_processor)
  {
    using Host = std::string;
    using Port = std::size_t;
    using Endpoint = std::pair<Host, Port>;
    using Endpoints = std::vector<Endpoint>;

    std::shared_ptr<GrpcChannelOperationPool> grpc_channel_operation_pool;
    if (config_->ChannelGrpcClientPool().enable())
    {
      Endpoints endpoints;
      const auto& endpoints_config = config_->ChannelServerEndpointList().Endpoint();
      for (const auto& endpoint_config : endpoints_config)
      {
        endpoints.emplace_back(
          endpoint_config.host(),
          endpoint_config.port());
      }

      const auto config_grpc_data = Config::create_pool_client_config(
        config_->ChannelGrpcClientPool());
      grpc_channel_operation_pool = std::make_shared<GrpcChannelOperationPool>(
        logger(),
        task_processor,
        scheduler,
        endpoints,
        config_grpc_data.first,
        config_grpc_data.second,
        config_->time_duration_grpc_client_mark_bad());
    }

    return grpc_channel_operation_pool;
  }

  ProfilingServer::GrpcCampaignManagerPoolPtr
  ProfilingServer::create_grpc_campaign_manager_pool(
    const SchedulerPtr& scheduler,
    TaskProcessor& task_processor)
  {
    using Endpoints = FrontendCommons::GrpcCampaignManagerPool::Endpoints;

    std::shared_ptr<GrpcCampaignManagerPool> grpc_campaign_manager_pool;
    if (config_->CampaignGrpcClientPool().enable())
    {
      Endpoints endpoints;
      const auto& endpoints_config = config_->CampaignManagerEndpointList().Endpoint();
      for (const auto& endpoint_config : endpoints_config)
      {
        if (!endpoint_config.service_index().present())
        {
          Stream::Error stream;
          stream << FNS
                 << "Service index not exist in CampaignManagerEndpointList";
          throw Exception(stream);
        }

        const std::string host = endpoint_config.host();
        const std::size_t port = endpoint_config.port();
        const std::string service_id = *endpoint_config.service_index();
        endpoints.emplace_back(
          host,
          port,
          service_id);
      }

      const auto config_grpc_data = Config::create_pool_client_config(
        config_->CampaignGrpcClientPool());
      grpc_campaign_manager_pool = std::make_shared<GrpcCampaignManagerPool>(
        logger(),
        task_processor,
        scheduler,
        endpoints,
        config_grpc_data.first,
        config_grpc_data.second,
        config_->time_duration_grpc_client_mark_bad());
    }

    return grpc_campaign_manager_pool;
  }

  void ProfilingServer::init_corba_()
  {
    typedef CORBACommons::ProcessStatsGen<ProfilingServerStats>
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
    catch (const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "Can't init CorbaServerAdapter: "
           << exc.what();
      throw Exception(ostr);
    }
  }

  void ProfilingServer::init_zeromq_()
  {
    try
    {
      zmq_context_.reset(new zmq::context_t(config_->zmq_io_threads()));

      add_child_object(Generics::ActiveObject_var(
        new ZmqRouter(
          callback(),
          *zmq_context_,
          config_->DMPProfilingInfoSocket(),
          INPROC_DMP_PROFILING_INFO_ADDRESS)));

      add_child_object(Generics::ActiveObject_var(
        new ZmqWorker(
          std::bind(&ProfilingServer::process_dmp_profiling_info_, this, std::placeholders::_1),
          callback(),
          *zmq_context_,
          INPROC_DMP_PROFILING_INFO_ADDRESS,
          config_->work_threads())));
    }
    catch(const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "got Exception: "
           << exc.what();
      throw Exception(ostr);
    }
  }

  void ProfilingServer::process_dmp_profiling_info_(zmq::message_t& msg) noexcept
  {
    try
    {
      const std::size_t msg_size = msg.size();
      stats_->add_received_messages(1, msg_size);
      
      DMPProfilingInfoReader dmp_profiling_info(msg.data(), msg_size);
      const Generics::Time now = Generics::Time::get_time_of_day();
      bool profile_request = true;
      
      if (hash_filter_)
      {
        DMPProfileHash dmp_hash(
          hash_time_precision_,
          dmp_profiling_info);
        
        profile_request = !hash_filter_->set(
          dmp_hash.hash(),
          Generics::Time(dmp_profiling_info.time()),
          now);
      }

      if (profile_request)
      {
        FrontendCommons::GrpcCampaignManagerPool::RequestParams request_params;
        RequestInfo request_info;

        request_info_filler_->fill_by_dmp_profiling_info(
          request_params,
          request_info,
          dmp_profiling_info,
          now);
        
        AdServer::Commons::UserId user_id;
        std::string resolved_ext_user_id;

        resolve_user_id_(
          user_id,
          resolved_ext_user_id,
          request_params,
          request_info);

        if (!user_id.is_null())
        {
          rebind_user_id_(
            user_id,
            request_info,
            resolved_ext_user_id,
            request_params.common_info.time);
        }

        if (streamer_)
        {
          streamer_->write_dmp_profiling_info(
            dmp_profiling_info,
            request_info.bind_user_ids,
            now);
        }

        // match triggers & keywords
        std::unique_ptr<AdServer::ChannelSvcs::Proto::MatchResponseInfo> trigger_match_result;
        trigger_match_(
          trigger_match_result,
          request_params,
          request_info,
          user_id);

        // do history match
        request_params.common_info.user_id = user_id;

        std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult> history_match_result;
        if (!user_id.is_null())
        {
          history_match_(
            history_match_result,
            request_params,
            request_info,
            trigger_match_result.get());
        }

        // request campaign manager
        request_campaign_manager_(
          request_params);
      }
      else
      {
        stats_->add_filtered_messages(1);
      }
    }
    catch (const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT) << FNS <<
        "Caught eh::Exception: " << ex.what();
    }
    catch (...)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT) << FNS <<
        "Caught eh::Exception: Unknown error";
    }
  }

  bool ProfilingServer::resolve_user_id_(
    AdServer::Commons::UserId& match_user_id,
    std::string& resolved_ext_user_id,
    FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
    RequestInfo& request_info) noexcept
  {
    Generics::Time start_process_time;
    if (logger()->log_level() >= Logging::Logger::TRACE)
    {
      start_process_time = Generics::Time::get_time_of_day();
    }

    match_user_id = request_params.common_info.user_id;

    if (request_info.filter_request)
    {
      request_params.common_info.user_status =
        AdServer::CampaignSvcs::US_FOREIGN;
    }
    else if (!match_user_id.is_null())
    {
      request_params.common_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
      match_user_id = request_params.common_info.user_id;
    }
    else
    {
      const auto& grpc_user_bind_distributor =
        grpc_container_->grpc_user_bind_operation_distributor;

      try
      {
        if (!request_info.bind_request_id.empty())
        {
          // append bind user ids got by bind_request_id
          auto response = grpc_user_bind_distributor->get_bind_request(
            request_info.bind_request_id,
            request_params.common_info.time);
          if (!response || response->has_error())
          {
            throw Exception("get_bind_request is failed");
          }

          const auto& info_proto = response->info();
          const auto& bind_user_ids = info_proto.bind_user_ids();
          request_info.bind_user_ids.reserve(bind_user_ids.size());
          request_info.bind_user_ids.insert(
            std::end(request_info.bind_user_ids),
            std::begin(bind_user_ids),
            std::end(bind_user_ids));
        }

        if (!request_info.bind_user_ids.empty())
        {
          bool min_age_reached = false;
          const auto& bind_user_ids = request_info.bind_user_ids;

          for (const auto& full_external_user_id : bind_user_ids)
          {
            if (!full_external_user_id.empty())
            {
              const bool internal_id_rule1 = (*full_external_user_id.begin() == '/');
              const bool internal_id_rule2 = (full_external_user_id.compare(0, 2, "c/") == 0);

              if (internal_id_rule1 || internal_id_rule2)
              {
                try
                {
                  match_user_id = AdServer::Commons::UserId(
                    String::SubString(full_external_user_id).substr(internal_id_rule1 ? 1 : 2));
                  break;
                }
                catch(...)
                {
                }
              }

              auto response = grpc_user_bind_distributor->get_user_id(
                full_external_user_id,              // id
                match_user_id,                      // current_user_id
                request_params.common_info.time,    // timestamp
                Generics::Time::ZERO,               // create_timestamp
                false,                              // silent
                false,                              // generate_user_id
                false);                             // for_set_cookie
              if (!response || response->has_error())
              {
                throw Exception("get_user_id is failed");
              }

              const auto& info_proto = response->info();
              min_age_reached |= info_proto.min_age_reached();
              match_user_id = GrpcAlgs::unpack_user_id(info_proto.user_id());

              if (!match_user_id.is_null())
              {
                resolved_ext_user_id = full_external_user_id;
                break;
              }
            }
          }

          if (!match_user_id.is_null())
          {
            request_params.common_info.user_status =
              AdServer::CampaignSvcs::US_OPTIN;
          }
          else if (min_age_reached)
          {
            // uid generation on RTB requests disabled
            request_params.common_info.user_status =
              AdServer::CampaignSvcs::US_UNDEFINED;
          }
          else
          {
            request_params.common_info.user_status =
              AdServer::CampaignSvcs::US_EXTERNALPROBE;
          }
        }
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger()->error(stream.str(), ASPECT);

        return false;
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
                 << "Unknown error";
        logger()->error(stream.str(), ASPECT);

        return false;
      }
    }

    if (request_info.bind_user_ids.empty() &&
      request_params.common_info.user_status != AdServer::CampaignSvcs::US_PROBE)
    {
      request_params.common_info.user_status =
        AdServer::CampaignSvcs::US_NOEXTERNALID;
    }

    if (logger()->log_level() >= Logging::Logger::TRACE)
    {
      const Generics::Time end_process_time = Generics::Time::get_time_of_day();
      Stream::Error ostr;
      ostr << FNS
           << "user is resolving time = "
           << (end_process_time - start_process_time);
      logger()->log(ostr.str(),
        Logging::Logger::TRACE,
        ASPECT);
    }

    return true;
  }

  void ProfilingServer::rebind_user_id_(
    const AdServer::Commons::UserId& user_id,
    const RequestInfo& request_info,
    const String::SubString& resolved_ext_user_id,
    const Generics::Time& time) noexcept
  {
    try
    {
      const auto& bind_user_ids = request_info.bind_user_ids;
      for (const auto& full_external_user_id : bind_user_ids)
      {
        if (resolved_ext_user_id != full_external_user_id &&
          !full_external_user_id.empty() &&
          *full_external_user_id.begin() != '/')
        {
          std::optional<AdServer::Commons::UserId> internal_user_id;
          if (full_external_user_id.compare(0, 2, "c/") == 0)
          {
            try
            {
              internal_user_id = AdServer::Commons::UserId(
                String::SubString(full_external_user_id).substr(2));
            }
            catch(const eh::Exception&)
            {
            }
          }

          if (!internal_user_id || user_id != *internal_user_id)
          {
            const auto& grpc_user_bind_distributor =
              grpc_container_->grpc_user_bind_operation_distributor;
            auto response = grpc_user_bind_distributor->add_user_id(
              full_external_user_id,                    // id
              time,                                     // timestamp
              GrpcAlgs::pack_user_id(user_id));         // user_id
            if (!response || response->has_error())
            {
              throw Exception("add_user_id is failed");
            }
          }
        }
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << exc.what();
      logger()->error(stream.str(), ASPECT);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown error";
      logger()->error(stream.str(), ASPECT);
    }
  }

  void ProfilingServer::trigger_match_(
    std::unique_ptr<AdServer::ChannelSvcs::Proto::MatchResponseInfo>& trigger_matched_channels,
    FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
    const RequestInfo& request_info,
    const AdServer::Commons::UserId& user_id) noexcept
  {
    if (!request_info.filter_request)
    {
      try
      {
        std::string first_url_words;
        try
        {
          FrontendCommons::extract_url_keywords(
            first_url_words,
            String::SubString(request_params.common_info.referer),
            0 // segmentor
            );
        }
        catch (const eh::Exception& exc)
        {
          if (logger()->log_level() >= Logging::Logger::DEBUG)
          {
            Stream::Error ostr;
            ostr << FNS
                 << " url keywords extracting error: "
                 << exc.what();
            logger()->log(ostr.str(),
              Logging::Logger::DEBUG,
              ASPECT);
          }
        }

        std::string urls;
        urls.reserve(100);
        std::string urls_words;
        urls_words.reserve(100);
        const std::size_t common_info_urls_size =
          request_params.common_info.urls.size();
        for (std::size_t i = 0; i < common_info_urls_size; ++i)
        {
          if (i != 0)
          {
            urls += '\n';
          }
          urls += request_params.common_info.urls[i];

          std::string url_words_res;
          FrontendCommons::extract_url_keywords(
            url_words_res,
            String::SubString(request_params.common_info.urls[i]),
            0 // segmentor
            );

          if (!url_words_res.empty())
          {
            if (!urls_words.empty())
            {
              urls_words += '\n';
            }
            urls_words += url_words_res;
          }
        }

        const auto& grpc_channel_operation_pool =
          grpc_container_->grpc_channel_operation_pool;
        auto response = grpc_channel_operation_pool->match(
          {},                                        // request_id
          request_params.common_info.referer,        // first_url
          first_url_words,                           // first_url_words
          urls,                                      // urls
          urls_words,                                // urls_words
          request_info.keywords,                     // pwords
          request_info.search_words,                 // swords
          GrpcAlgs::pack_user_id(user_id),           // uid
          {'A', '\0'},                               // statuses
          false,                                     // non_strict_word_match
          false,                                     // non_strict_url_match
          false,                                     // return_negative,
          true,                                      // simplify_page,
          true);                                     // fill_content
        if (response && response->has_info())
        {
          trigger_matched_channels.reset(response->release_info());
        }
        else
        {
          throw Exception("match is failed");
        }

        const auto& trigger_page_channels =
          trigger_matched_channels->matched_channels().page_channels();
        auto& pkw_channels = request_params.trigger_match_result.pkw_channels;
        pkw_channels.reserve(trigger_page_channels.size());
        for (const auto& trigger_page_channel : trigger_page_channels)
        {
          pkw_channels.emplace_back(
            trigger_page_channel.trigger_channel_id(),
            trigger_page_channel.id());
        }

        const auto& trigger_url_channels =
          trigger_matched_channels->matched_channels().url_channels();
        auto& url_channels = request_params.trigger_match_result.url_channels;
        url_channels.reserve(trigger_url_channels.size());
        for (const auto& trigger_url_channel : trigger_url_channels)
        {
          url_channels.emplace_back(
            trigger_url_channel.trigger_channel_id(),
            trigger_url_channel.id());
        }

        const auto& trigger_url_keyword_channels =
          trigger_matched_channels->matched_channels().url_keyword_channels();
        auto& ukw_channels = request_params.trigger_match_result.ukw_channels;
        ukw_channels.reserve(trigger_url_keyword_channels.size());
        for (const auto& trigger_url_keyword_channel : trigger_url_keyword_channels)
        {
          ukw_channels.emplace_back(
            trigger_url_keyword_channel.trigger_channel_id(),
            trigger_url_keyword_channel.id());
        }

        const auto& trigger_search_channels =
          trigger_matched_channels->matched_channels().search_channels();
        auto& skw_channels = request_params.trigger_match_result.skw_channels;
        skw_channels.reserve(trigger_search_channels.size());
        for (const auto& trigger_search_channel : trigger_search_channels)
        {
          skw_channels.emplace_back(
            trigger_search_channel.trigger_channel_id(),
            trigger_search_channel.id());
        }

        const auto& trigger_uid_channels =
          trigger_matched_channels->matched_channels().uid_channels();
        auto& uid_channels = request_params.trigger_match_result.uid_channels;
        uid_channels.reserve(trigger_uid_channels.size());
        uid_channels.insert(
          std::end(uid_channels),
          std::begin(trigger_uid_channels),
          std::end(trigger_uid_channels));

        if (request_params.common_info.user_status ==
           static_cast<std::uint32_t>(AdServer::CampaignSvcs::US_OPTIN) &&
           (trigger_matched_channels->no_track() ||
            trigger_matched_channels->no_adv()))
        {
          request_params.common_info.user_status =
            AdServer::CampaignSvcs::US_BLACKLISTED;
        }
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger()->log(stream.str(),
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-117");
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << "Unknown error";
        logger()->log(stream.str(),
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-117");
      }
    }
  }

  void ProfilingServer::history_match_(
    std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>& match_result,
    FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
    const RequestInfo& request_info,
    const AdServer::ChannelSvcs::Proto::MatchResponseInfo* const trigger_matching_result) noexcept
  {
    using UserInfo = AdServer::UserInfoSvcs::Types::UserInfo;
    using MatchParams = AdServer::UserInfoSvcs::Types::MatchParams;

    const bool do_history_matching =
      request_params.common_info.user_status == AdServer::CampaignSvcs::US_OPTIN ||
      request_params.common_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;
    bool match_success = false;

    if (do_history_matching)
    {
      try
      {
        MatchParams match_params;
        match_params.use_empty_profile = false;
        match_params.silent_match = false;
        match_params.no_match = false;
        match_params.no_result = false;
        match_params.ret_freq_caps = false;
        match_params.provide_channel_count = false;
        match_params.provide_persistent_channels = false;
        match_params.change_last_request = true;
        match_params.filter_contextual_triggers = false;
        match_params.publishers_optin_timeout = Generics::Time::ZERO;
        match_params.cohort2 = request_info.cohort2;

        if (request_info.coord_location.in())
        {
          auto& geo_data_seq = match_params.geo_data_seq;
          geo_data_seq.emplace_back(
            request_info.coord_location->latitude,
            request_info.coord_location->longitude,
            request_info.coord_location->accuracy);
        }

        if (trigger_matching_result)
        {
          prepare_ui_match_params_(
            match_params,
            *trigger_matching_result);
        }

        UserInfo user_info;
        user_info.user_id = GrpcAlgs::pack_user_id(
          request_params.common_info.user_id);
        user_info.last_colo_id = request_params.common_info.colo_id;
        user_info.request_colo_id = request_params.common_info.colo_id;
        user_info.current_colo_id = -1;
        user_info.temporary =
          (request_params.common_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY);
        user_info.time = request_params.common_info.time.tv_sec;

        const auto& grpc_user_info_distributor =
          grpc_container_->grpc_user_info_operation_distributor;
        auto response = grpc_user_info_distributor->match(
          user_info,
          match_params);
        if (response && response->has_info())
        {
          auto* response_info = response->mutable_info();
          match_result.reset(response_info->release_match_result());
          match_success = true;
        }
        else
        {
          throw Exception("match is failed");
        }
      }
      catch (const eh::Exception &exc)
      {
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger()->emergency(stream.str(), ASPECT);
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << "Unknown error";
        logger()->emergency(stream.str(), ASPECT);
      }
    }

    /* log user info request */
    if (logger()->log_level() >= TraceLevel::MIDDLE)
    {
      const auto& channels = match_result->channels();

      std::ostringstream ostr;
      ostr << FNS
           << "history matched channels for uid = '"
           << request_params.common_info.user_id << "': ";

      if (channels.empty())
      {
        ostr << "empty";
      }
      else
      {
        GrpcAlgs::print_repeated_fields(
          ostr,
          ",",
          ":",
          channels,
          &AdServer::UserInfoSvcs::Proto::ChannelWeight::channel_id);
      }

      logger()->log(
        ostr.str(),
        TraceLevel::MIDDLE,
        ASPECT);
    }

    if (!match_result)
    {
      match_result = get_empty_history_matching_();
    }

    if (trigger_matching_result && (!match_success || !do_history_matching))
    {
      // fill history channels with context channels
      const auto& content_channels = trigger_matching_result->content_channels();
      auto* history_matched_channels = match_result->mutable_channels();
      history_matched_channels->Reserve(content_channels.size());
      for (const auto& content_channel : content_channels)
      {
        auto* channel_weight = history_matched_channels->Add();
        channel_weight->set_channel_id(content_channel.id());
        channel_weight->set_weight(content_channel.weight());
      }
    }

    // fill request params
    request_params.client_create_time =
      GrpcAlgs::unpack_time(match_result->create_time());
    request_params.fraud = match_result->fraud_request();

    request_params.channels.reserve(match_result->channels().size());
    for (const auto& channel : match_result->channels())
    {
      request_params.channels.emplace_back(channel.channel_id());
    }

    request_params.session_start =
      GrpcAlgs::unpack_time(match_result->session_start());
  }

  void ProfilingServer::merge_users_(
    const AdServer::Commons::UserId& target_user_id,
    const AdServer::Commons::UserId& source_user_id,
    const bool source_temporary,
    const FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params) noexcept
  {
    using ProfilesRequestInfo = AdServer::UserInfoSvcs::Types::ProfilesRequestInfo;
    using UserInfo = AdServer::UserInfoSvcs::Types::UserInfo;
    using MatchParams = AdServer::UserInfoSvcs::Types::MatchParams;
    using UserProfiles = AdServer::UserInfoSvcs::Types::UserProfiles;

    const bool REMOVE_SOURCE_USER_PROFILE = false;
    bool merge_success = false;

    UserProfiles merge_user_profiles;
    try
    {
      const auto merged_uid_info = GrpcAlgs::pack_user_id(
        source_user_id);

      ProfilesRequestInfo profiles_request;
      profiles_request.base_profile = true;
      profiles_request.add_profile = true;
      profiles_request.history_profile = true;
      profiles_request.freq_cap_profile = true;
      profiles_request.pref_profile = false;

      const auto& grpc_user_info_distributor =
        grpc_container_->grpc_user_info_operation_distributor;
      auto get_user_profile_response = grpc_user_info_distributor->get_user_profile(
        merged_uid_info,
        source_temporary,
        profiles_request);
      if (!get_user_profile_response || get_user_profile_response->has_error())
      {
        throw Exception("get_user_profile is failed");
      }

      const auto& get_user_profile_response_info = get_user_profile_response->info();
      const auto& user_profiles_proto = get_user_profile_response_info.user_profiles();

      merge_user_profiles.add_user_profile = user_profiles_proto.add_user_profile();
      merge_user_profiles.base_user_profile = user_profiles_proto.base_user_profile();
      merge_user_profiles.pref_profile = user_profiles_proto.pref_profile();
      merge_user_profiles.history_user_profile = user_profiles_proto.history_user_profile();
      merge_user_profiles.freq_cap = user_profiles_proto.freq_cap();

      merge_success = get_user_profile_response_info.return_value();

      merge_success = (
        user_profiles_proto.base_user_profile().size() != 0 &&
        user_profiles_proto.add_user_profile().size() != 0);

      if (merge_success && REMOVE_SOURCE_USER_PROFILE)
      {
        auto response = grpc_user_info_distributor->remove_user_profile(merged_uid_info);
        if (!response || response->has_error())
        {
          throw Exception("remove_user_profile is failed");
        }
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << exc.what();
      logger()->emergency(stream.str(), ASPECT);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown error";
      logger()->emergency(stream.str(), ASPECT);
    }

    if (merge_success)
    {
      try
      {
        UserInfo user_info;
        user_info.user_id = GrpcAlgs::pack_user_id(target_user_id);
        user_info.last_colo_id = request_params.common_info.colo_id;
        user_info.request_colo_id = request_params.common_info.colo_id;
        user_info.current_colo_id = -1;
        user_info.temporary = false;
        user_info.time = request_params.common_info.time.tv_sec;

        MatchParams match_params;
        match_params.use_empty_profile = false;
        match_params.silent_match = false;
        match_params.no_match = false;
        match_params.no_result = false;
        match_params.provide_persistent_channels = false;
        match_params.change_last_request = false;
        match_params.filter_contextual_triggers = false;
        match_params.publishers_optin_timeout = Generics::Time::ZERO;

        const auto& grpc_user_info_distributor =
          grpc_container_->grpc_user_info_operation_distributor;
        auto merge_response = grpc_user_info_distributor->merge(
          user_info,
          match_params,
          merge_user_profiles);
        if (!merge_response || merge_response->has_error())
        {
          throw Exception("merge is failed");
        }
      }
      catch (const eh::Exception &exc)
      {
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger()->emergency(stream.str(), ASPECT);
      }
      catch (...)
      {
        Stream::Error stream;
        stream << FNS
               << "Unknown error";
        logger()->emergency(stream.str(), ASPECT);
      }
    }
  }

  void ProfilingServer::prepare_ui_match_params_(
    AdServer::UserInfoSvcs::Types::MatchParams& match_params,
    const AdServer::ChannelSvcs::Proto::MatchResponseInfo& match_result)
  {
    using ChannelMatchSet = std::set<ChannelMatch>;

    if (!match_result.no_track())
    {
      ChannelMatchSet url_channels;
      ChannelMatchSet page_channels;
      ChannelMatchSet search_channels;
      ChannelMatchSet url_keyword_channels;

      const auto& matched_channels = match_result.matched_channels();
      std::transform(
        std::begin(matched_channels.url_channels()),
        std::end(matched_channels.url_channels()),
        std::inserter(url_channels, url_channels.end()),
        GetChannelTriggerId());

      std::transform(
        std::begin(matched_channels.page_channels()),
        std::end(matched_channels.page_channels()),
        std::inserter(page_channels, page_channels.end()),
        GetChannelTriggerId());

      std::transform(
        std::begin(matched_channels.search_channels()),
        std::end(matched_channels.search_channels()),
        std::inserter(search_channels, search_channels.end()),
        GetChannelTriggerId());

      std::transform(
        std::begin(matched_channels.url_keyword_channels()),
        std::end(matched_channels.url_keyword_channels()),
        std::inserter(url_keyword_channels, url_keyword_channels.end()),
        GetChannelTriggerId());

      auto& url_channel_ids = match_params.url_channel_ids;
      url_channel_ids.reserve(url_channels.size());
      for (const auto& url_channel : url_channels)
      {
        url_channel_ids.emplace_back(
          url_channel.channel_id,
          url_channel.channel_trigger_id);
      }

      auto& page_channel_ids = match_params.page_channel_ids;
      page_channel_ids.reserve(page_channels.size());
      for (const auto& page_channel : page_channels)
      {
        page_channel_ids.emplace_back(
          page_channel.channel_id,
          page_channel.channel_trigger_id);
      }

      auto& search_channel_ids = match_params.search_channel_ids;
      search_channel_ids.reserve(search_channels.size());
      for (const auto& search_channel : search_channels)
      {
        search_channel_ids.emplace_back(
          search_channel.channel_id,
          search_channel.channel_trigger_id);
      }

      auto& url_keyword_channel_ids = match_params.url_keyword_channel_ids;
      url_keyword_channel_ids.reserve(url_keyword_channels.size());
      for (const auto& url_keyword_channel : url_keyword_channels)
      {
        url_keyword_channel_ids.emplace_back(
          url_keyword_channel.channel_id,
          url_keyword_channel.channel_trigger_id);
      }
    }
  }

  void ProfilingServer::request_campaign_manager_(
    const FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params)
  {
    try
    {
      const auto& grpc_campaign_manager_pool =
        grpc_container_->grpc_campaign_manager_pool;
      auto response = grpc_campaign_manager_pool->get_campaign_creative(
        request_params);
      if (!response || response->has_error())
      {
        throw Exception("get_campaign_creative is failed");
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error ostr;
      ostr << FNS
           << "fail. Caught eh::Exception: "
           << exc.what();
      throw Exception(ostr);
    }
    catch (...)
    {
      Stream::Error ostr;
      ostr << FNS
           << "fail. Caught eh::Exception: Unknown error";
      throw Exception(ostr);
    }
  }

  std::unique_ptr<AdServer::UserInfoSvcs::Proto::MatchResult>
  ProfilingServer::get_empty_history_matching_()
  {
    auto result = std::make_unique<AdServer::UserInfoSvcs::Proto::MatchResult>();
    result->set_fraud_request(false);
    result->set_times_inited(false);
    result->set_last_request_time(GrpcAlgs::pack_time(Generics::Time::ZERO));
    result->set_create_time(GrpcAlgs::pack_time(Generics::Time::ZERO));
    result->set_session_start(GrpcAlgs::pack_time(Generics::Time::ZERO));
    result->set_colo_id(-1);
    return result;
  }
} // namespace AdServer::Profiling

int
main(int argc, char** argv)
{
  AdServer::Profiling::ProfilingServer* app = nullptr;
  try
  {
    app = &AdServer::Profiling::ProfilingServerApp::instance();
  }
  catch (...)
  {
    std::cerr << "main(): Critical: Got exception while "
                 "creating application object.\n";
    return EXIT_FAILURE;
  }

  if (!app)
  {
    std::cerr << "main(): Critical: got NULL application object.\n";
    return EXIT_FAILURE;
  }

  try
  {
    app->main(argc, argv);
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: "
              << ex.what();
    return EXIT_FAILURE;
  }
  catch (...)
  {
    std::cerr << "Caught eh::Exception: Unknown error";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}