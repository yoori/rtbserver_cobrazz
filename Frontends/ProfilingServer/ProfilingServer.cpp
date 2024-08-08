#include <cstdlib>
#include <sstream>

#include <Commons/ConfigUtils.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/ZmqConfig.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <XMLUtility/Utility.hpp>
#include <CORBACommons/StatsImpl.hpp>

#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>

#include <Frontends/FrontendCommons/GeoInfoUtils.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include <UserInfoSvcs/UserInfoManager/ProtoConvertor.hpp>

#include "ProfilingServer.hpp"

namespace
{
  const char ASPECT[] = "ProfilingServer";
  const char PROCESS_CONTROL_OBJ_KEY[] = "ProcessControl";
  const char PROCESS_STATS_CONTROL_OBJ_KEY[] = "ProcessStatsControl";
  const char INPROC_DMP_PROFILING_INFO_ADDRESS[] = "inproc://dmp_profiling_info";
}

namespace
{
  struct ContextualChannelConverter
  {
    const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight&
    operator()(const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight& ch_weight)
      const
    {
      return ch_weight;
    }

    AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight
    operator()(const AdServer::ChannelSvcs::
      ChannelServerBase::ContentChannelAtom& contextual_channel)
      const
    {
      AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight res;
      res.channel_id = contextual_channel.id;
      res.weight = contextual_channel.weight;
      return res;
    }
  };

  struct ChannelMatch
  {
    ChannelMatch(
      unsigned long channel_id_val,
      unsigned long channel_trigger_id_val)
      : channel_id(channel_id_val),
        channel_trigger_id(channel_trigger_id_val)
    {}

    bool
    operator<(const ChannelMatch& right) const
    {
      return (channel_id < right.channel_id ||
        (channel_id == right.channel_id &&
          channel_trigger_id < right.channel_trigger_id));
    }

    unsigned long channel_id;
    unsigned long channel_trigger_id;
  };

  struct GetChannelTriggerId
  {
    ChannelMatch
    operator() (
      const AdServer::ChannelSvcs::ChannelServerBase::ChannelAtom& atom)
      noexcept
    {
      return ChannelMatch(atom.id, atom.trigger_channel_id);
    }
  };

  AdServer::CampaignSvcs::CampaignManager::ChannelTriggerMatchInfo
  convert_channel_atom(
    const AdServer::ChannelSvcs::ChannelServerBase::ChannelAtom& atom)
    noexcept
  {
    AdServer::CampaignSvcs::CampaignManager::ChannelTriggerMatchInfo out;
    out.channel_id = atom.id;
    out.channel_trigger_id = atom.trigger_channel_id;
    return out;
  }
}

namespace AdServer
{
namespace Profiling
{
  namespace
  {

    const unsigned long TIME_PRECISION = 30;
    
    struct DMPProfileHash
    {

      static const int COORD_PRECISION = 10000;
      
      DMPProfileHash(
        unsigned long time_precision,
        const DMPProfilingInfoReader& dmp_profiling_info)
        : time_precision_(time_precision),
          hash_(0)
      {
        assert(time_precision);
        calc(dmp_profiling_info);
      }

      static int
      trunc_coord(
        int coord)
      {
        return coord * COORD_PRECISION / COORD_PRECISION;
      }
      
      void
      calc(const DMPProfilingInfoReader& dmp_profiling_info)
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
  
  /*
   * ProfilingServer::ZmqRouter
   */
  ProfilingServer::ZmqRouter::ZmqRouter(
    Generics::ActiveObjectCallback* callback,
    zmq::context_t& zmq_context,
    const xsd::AdServer::Configuration::ZmqSocketType& socket_config,
    const char* inproc_address)
    /*throw(eh::Exception)*/
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

      for(auto it = socket_config.Address().begin();
          it != socket_config.Address().end(); ++it)
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

  void
  ProfilingServer::ZmqRouter::work_() noexcept
  {
    while(active())
    {
      try
      {
        zmq::message_t msg;
        if(bind_socket_.recv(&msg))
        {
          inproc_socket_.send(msg);
        }
      }
      catch(const zmq::error_t& ex)
      {
        if(ex.num() != ETERM)
        {
          Stream::Error ostr;
          ostr << "ProfilingServer::ZmqRouter::route_to_workers_: got exception: " <<
            ex.what();
          callback_->critical(ostr.str());
        }
      }
    }

    bind_socket_.close();
    inproc_socket_.close();
  }

  /*
   * ProfilingServer::ZmqWorker
   */
  ProfilingServer::ZmqWorker::ZmqWorker(
    Worker worker,
    Generics::ActiveObjectCallback* callback,
    zmq::context_t& zmq_context,
    const char* inproc_address,
    std::size_t work_threads)
    /*throw(eh::Exception)*/
    : Commons::DelegateActiveObject(
        std::bind(&ZmqWorker::do_work_, this, inproc_address),
        callback,
        work_threads),
      zmq_context_(zmq_context),
      callback_(ReferenceCounting::add_ref(callback)),
      worker_(worker)
  {}

  void
  ProfilingServer::ZmqWorker::do_work_(const char* inproc_address) noexcept
  {
    static const char* FUN = "ProfilingServer::ZmqWorker::work_()";

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

      while(active())
      {
        try
        {
          zmq::message_t msg;
          if(socket.recv(&msg))
          {
            worker_(msg);
          }
        }
        catch(const zmq::error_t& ex)
        {
          if(ex.num() != ETERM)
          {
            Stream::Error ostr;
            ostr << FUN << ": got Exception: " << ex.what();
            callback_->critical(ostr.str());
          }
        }
      }
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": socket open/close failed: " << ex.what();
      callback_->critical(ostr.str());
      std::abort();
    }
  }

  /*
   * ProfilingServer
   */
  ProfilingServer::ProfilingServer()
    /*throw(eh::Exception)*/
    : AdServer::Commons::ProcessControlVarsLoggerImpl(
        "ProfilingServer", ASPECT),
      logger_callback_holder_(logger(), "ProfilingServer", ASPECT, 0),
      campaign_managers_(logger_callback_holder_.logger(), ASPECT),
      streamer_(new DMPKafkaStreamer),
      stats_(new ProfilingServerStats(streamer_)),
      hash_time_precision_(0)
  {}

  void
  ProfilingServer::main(int& argc, char** argv)
    noexcept
  {
    static const char* FUN = "ProfilingServer::main()";

    try
    {
      XMLUtility::initialize();
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT,
        "ADS-IMPL-205") << FUN << ": Got eh::Exception: " << ex.what();
      return;
    }

    try
    {
      if(argc < 2)
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
      catch(const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << "Can't parse config file '" << argv[1] << "'. "
            << ": " << ex.what();
        throw Exception(ostr);
      }
      catch(...)
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
          filter_config =
            config_->DMPProfileFilter().get();
        
        hash_filter_ =
          new HashFilter(
            filter_config.chunk_count(),
            filter_config.chunk_size(),
            Generics::Time(filter_config.keep_time_period()),
            Generics::Time(filter_config.keep_time()));

        unsigned long time_precision =
          filter_config.time_precision();

        hash_time_precision_ =
          (time_precision / TIME_PRECISION +
            (time_precision % TIME_PRECISION || time_precision == 0 ?
              1 : 0) ) * TIME_PRECISION;
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
        "ADS-IMPL-205") << FUN <<
        ": Got ProfilingServer::Exception: " << ex.what();
    }
    catch (const eh::Exception &ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT,
        "ADS-IMPL-205") << FUN <<
        ": Got eh::Exception: " << ex.what();
    }

    XMLUtility::terminate();
  }

  void
  ProfilingServer::shutdown(CORBA::Boolean wait_for_completion)
    noexcept
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

  void
  ProfilingServer::read_config_(
    const char* filename,
    const char* argv0)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "ProfilingServer::read_config()";

    try
    {
      Config::ErrorHandler error_handler;

      try
      {
        using namespace xsd::AdServer::Configuration;

        std::unique_ptr<AdConfigurationType>
          ad_configuration = AdConfiguration(filename, error_handler);

        if(error_handler.has_errors())
        {
          std::string error_string;
          throw Exception(error_handler.text(error_string));
        }

        config_.reset(
          new ProfilingServerConfig(ad_configuration->ProfilingServerConfig()));

        if(error_handler.has_errors())
        {
          std::string error_string;
          throw Exception(error_handler.text(error_string));
        }
      }
      catch(const xml_schema::parsing& ex)
      {
        Stream::Error ostr;
        ostr << "Can't parse config file '" << filename << "'. : ";
        if(error_handler.has_errors())
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
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't read Corba Config: " << ex.what();
        throw Exception(ostr);
      }

      try
      {
        logger(Config::LoggerConfigReader::create(
          config_->Logger(), argv0));
        logger_callback_holder_.logger(logger());
      }
      catch(const Config::LoggerConfigReader::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": got LoggerConfigReader::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }
    catch(const Exception &ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got Exception. Invalid configuration: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  void
  ProfilingServer::init_coroutine_()
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

  ProfilingServer::GrpcUserBindOperationDistributor_var
  ProfilingServer::create_grpc_user_bind_operation_distributor(
    const SchedulerPtr& scheduler,
    TaskProcessor& task_processor)
  {
    GrpcUserBindOperationDistributor_var grpc_user_bind_operation_distributor;
    if (config_->UserBindGrpcClientPool().enable())
    {
      using ControllerRefList = AdServer::UserInfoSvcs::UserBindOperationDistributor::ControllerRefList;
      using ControllerRef = AdServer::UserInfoSvcs::UserBindOperationDistributor::ControllerRef;

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

  void
  ProfilingServer::init_corba_() /*throw(Exception)*/
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

      corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

      if(config_->ChannelManagerControllerRefs().present())
      {
        CORBACommons::CorbaObjectRefList channel_manager_controller_refs;
        Config::CorbaConfigReader::read_multi_corba_ref(
          config_->ChannelManagerControllerRefs().get(),
          channel_manager_controller_refs);

        channel_servers_.reset(
          new FrontendCommons::ChannelServerSessionPool(
            channel_manager_controller_refs,
            corba_client_adapter_,
            callback()));
      }

      const auto& config_grpc_client = config_->UserBindGrpcClientPool();
      const auto config_grpc_data = Config::create_pool_client_config(
        config_grpc_client);

      if(!config_->UserBindControllerGroup().empty())
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
        auto grpc_user_bind_operation_distributor = create_grpc_user_bind_operation_distributor(
          scheduler,
          manager_coro_->get_main_task_processor());

        user_bind_client_ = new FrontendCommons::UserBindClient(
          config_->UserBindControllerGroup(),
          corba_client_adapter_.in(),
          logger(),
          grpc_user_bind_operation_distributor.in());
        add_child_object(user_bind_client_);
      }

      /*user_info_client_ = new FrontendCommons::UserInfoClient(
        config_->UserInfoManagerControllerGroup(),
        corba_client_adapter_.in(),
        logger_callback_holder_.logger(),
        manager_coro_->get_main_task_processor(),
        config_grpc_data.first,
        config_grpc_data.second,
        config_grpc_client.enable());*/
      add_child_object(user_info_client_);

      CORBACommons::CorbaObjectRefList campaign_managers_refs;
      Config::CorbaConfigReader::read_multi_corba_ref(
        config_->CampaignManagerRef(),
        campaign_managers_refs);

      campaign_managers_.resolve(
        campaign_managers_refs,
        corba_client_adapter_);
    }
    catch(const eh::Exception &ex)
    {
      Stream::Error ostr;
      ostr << "ProfilingServer::init_corba(): "
        "Can't init CorbaServerAdapter: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  ProfilingServer::init_zeromq_()
    /*throw(Exception)*/
  {
    static const char* FUN = "ProfilingServer::init_zeromq_()";

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
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": got Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  ProfilingServer::process_dmp_profiling_info_(zmq::message_t& msg)
    noexcept
  {
    static const char* FUN = "ProfilingServer::process_dmp_profiling_info_()";

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

      if(profile_request)
      {
        AdServer::CampaignSvcs::CampaignManager::RequestParams request_params;
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

        if(!user_id.is_null())
        {
          rebind_user_id_(
            user_id,
            request_params,
            request_info,
            resolved_ext_user_id,
            CorbaAlgs::unpack_time(request_params.common_info.time));
        }

        if (streamer_)
        {
          streamer_->write_dmp_profiling_info(
            dmp_profiling_info,
            request_info.bind_user_ids,
            now);
        }

        // match triggers & keywords
        AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var trigger_match_result;

        trigger_match_(
          trigger_match_result.out(),
          request_params,
          request_info,
          user_id);

        // do history match
        request_params.common_info.user_id = CorbaAlgs::pack_user_id(user_id);

        AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var history_match_result;

        if(!user_id.is_null())
        {
          history_match_(
            history_match_result.out(),
            request_params,
            request_info,
            trigger_match_result.ptr());
        }

        // request campaign manager
        request_campaign_manager_(
          request_params,
          trigger_match_result.ptr(),
          history_match_result.ptr());
      }
      else
      {
        stats_->add_filtered_messages(1);
      }
    }
    catch(const CORBA::SystemException& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT) << FUN <<
        ": Caught CORBA::SystemException: " << ex;
    }
    catch(const eh::Exception& ex)
    {
      logger()->sstream(Logging::Logger::EMERGENCY, ASPECT) << FUN <<
        ": Caught eh::Exception: " << ex.what();
    }
  }

  bool
  ProfilingServer::resolve_user_id_(
    AdServer::Commons::UserId& match_user_id,
    std::string& resolved_ext_user_id,
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    RequestInfo& request_info)
    noexcept
  {
    static const char* FUN = "ProfilingServer::resolve_user_id_()";

    Generics::Time start_process_time;

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      start_process_time = Generics::Time::get_time_of_day();
    }

    match_user_id = CorbaAlgs::unpack_user_id(request_params.common_info.user_id);

    if(request_info.filter_request)
    {
      request_params.common_info.user_status = static_cast<CORBA::ULong>(
        AdServer::CampaignSvcs::US_FOREIGN);
    }
    else if(!match_user_id.is_null())
    {
      request_params.common_info.user_status = static_cast<CORBA::ULong>(
        AdServer::CampaignSvcs::US_OPTIN);
      match_user_id = CorbaAlgs::unpack_user_id(request_params.common_info.user_id);
    }
    else if(user_bind_client_)
    {
      const auto grpc_distributor = user_bind_client_->grpc_distributor();
      bool is_grpc_success = false;
      if (grpc_distributor)
      {
        try
        {
          is_grpc_success = true;

          if(!request_info.bind_request_id.empty())
          {
            // append bind user ids got by bind_request_id
            auto response = grpc_distributor->get_bind_request(
              request_info.bind_request_id,
              CorbaAlgs::unpack_time(request_params.common_info.time));
            if (!response || response->has_error())
            {
              GrpcAlgs::print_grpc_error_response(
                response,
                logger(),
                ASPECT);
              throw Exception("get_bind_request is failed");
            }

            const auto& info_proto = response->info();
            const auto& bind_user_ids = info_proto.bind_user_ids();
            request_info.bind_user_ids.insert(
              std::end(request_info.bind_user_ids),
              std::begin(bind_user_ids),
              std::end(bind_user_ids));
          }

          if(!request_info.bind_user_ids.empty())
          {
            bool min_age_reached = false;
            const auto& bind_user_ids = request_info.bind_user_ids;

            for (const auto& full_external_user_id : bind_user_ids)
            {
              if(!full_external_user_id.empty())
              {
                const bool internal_id_rule1 = (*full_external_user_id.begin() == '/');
                const bool internal_id_rule2 = (full_external_user_id.compare(0, 2, "c/") == 0);

                if(internal_id_rule1 || internal_id_rule2)
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

                auto response = grpc_distributor->get_user_id(
                  full_external_user_id,
                  match_user_id,
                  CorbaAlgs::unpack_time(request_params.common_info.time),
                  Generics::Time::ZERO,
                  false,
                  false,
                  false);
                if (!response || response->has_error())
                {
                  GrpcAlgs::print_grpc_error_response(
                    response,
                    logger(),
                    ASPECT);
                  throw Exception("get_user_id is failed");
                }

                const auto& info_proto = response->info();
                min_age_reached |= info_proto.min_age_reached();
                match_user_id = GrpcAlgs::unpack_user_id(info_proto.user_id());

                if(!match_user_id.is_null())
                {
                  resolved_ext_user_id = full_external_user_id;
                  break;
                }
              }
            }

            if(!match_user_id.is_null())
            {
              request_params.common_info.user_status = static_cast<CORBA::ULong>(
                AdServer::CampaignSvcs::US_OPTIN);
            }
            else if(min_age_reached)
            {
              // uid generation on RTB requests disabled
              request_params.common_info.user_status = static_cast<CORBA::ULong>(
                AdServer::CampaignSvcs::US_UNDEFINED);
            }
            else
            {
              request_params.common_info.user_status = static_cast<CORBA::ULong>(
                AdServer::CampaignSvcs::US_EXTERNALPROBE);
            }
          }
        }
        catch (const eh::Exception& exc)
        {
          is_grpc_success = false;
          Stream::Error stream;
          stream << FUN
                 << ": "
                 << exc.what();
          logger()->error(stream.str(), ASPECT);
        }
        catch (...)
        {
          is_grpc_success = false;
          Stream::Error stream;
          stream << FUN
                 << ": Unknown error";
          logger()->error(stream.str(), ASPECT);
        }
      }

      if (!is_grpc_success)
      {
        try
        {
          AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper =
            user_bind_client_->user_bind_mapper();

          if(!request_info.bind_request_id.empty())
          {
            // append bind user ids got by bind_request_id
            AdServer::UserInfoSvcs::UserBindServer::BindRequestInfo_var bind_request_info =
              user_bind_mapper->get_bind_request(
                request_info.bind_request_id.c_str(),
                request_params.common_info.time);

            for(CORBA::ULong bind_user_i = 0;
              bind_user_i < bind_request_info->bind_user_ids.length(); ++bind_user_i)
            {
              request_info.bind_user_ids.push_back(
                bind_request_info->bind_user_ids[bind_user_i].in());
            }
          }

          if(!request_info.bind_user_ids.empty())
          {
            bool min_age_reached = false;

            for(auto ext_user_id_it = request_info.bind_user_ids.begin();
              ext_user_id_it != request_info.bind_user_ids.end(); ++ext_user_id_it)
            {
              const std::string& full_external_user_id = *ext_user_id_it;

              if(!full_external_user_id.empty())
              {
                const bool internal_id_rule1 = (*full_external_user_id.begin() == '/');
                const bool internal_id_rule2 = (full_external_user_id.compare(0, 2, "c/") == 0);

                if(internal_id_rule1 || internal_id_rule2)
                {
                  try
                  {
                    match_user_id = AdServer::Commons::UserId(
                      String::SubString(full_external_user_id).substr(internal_id_rule1 ? 1 : 2));
                    break;
                  }
                  catch(...)
                  {}
                }

                AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo get_request_info;
                get_request_info.id << full_external_user_id;
                get_request_info.timestamp = request_params.common_info.time;
                get_request_info.silent = false;
                get_request_info.generate_user_id = false;
                get_request_info.for_set_cookie = false;
                get_request_info.create_timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
                get_request_info.current_user_id = CorbaAlgs::pack_user_id(match_user_id);

                AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo_var user_bind_info =
                  user_bind_mapper->get_user_id(get_request_info);

                min_age_reached |= user_bind_info->min_age_reached;
                match_user_id = CorbaAlgs::unpack_user_id(user_bind_info->user_id);

                if(!match_user_id.is_null())
                {
                  resolved_ext_user_id = full_external_user_id;
                  break;
                }
              }
            }

            if(!match_user_id.is_null())
            {
              request_params.common_info.user_status = static_cast<CORBA::ULong>(
                AdServer::CampaignSvcs::US_OPTIN);

              /*
              common_info.signed_user_id <<
                common_module_->user_id_controller()->sign(match_user_id).str();
              */
            }
            else if(min_age_reached)
            {
              // uid generation on RTB requests disabled
              request_params.common_info.user_status = static_cast<CORBA::ULong>(
                AdServer::CampaignSvcs::US_UNDEFINED);
            }
            else
            {
              request_params.common_info.user_status = static_cast<CORBA::ULong>(
                AdServer::CampaignSvcs::US_EXTERNALPROBE);
            }
          }
        }
        catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady& )
        {
          Stream::Error ostr;
          ostr << FUN << ": caught UserBindMapper::NotReady";

          logger()->log(ostr.str(),
            Logging::Logger::WARNING,
            ASPECT,
            "ADS-IMPL-10681");

          return false;
        }
        catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& )
        {
          Stream::Error ostr;
          ostr << FUN << ": caught UserBindMapper::ChunkNotFound";

          logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-IMPL-10681");

          return false;
        }
        catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught UserBindMapper::ImplementationException: " <<
            ex.description;

          logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-IMPL-10681");

          return false;
        }
        catch(const CORBA::SystemException& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught CORBA::SystemException: " << e;
          logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-ICON-7800");

          return false;
        }
      }
    }

    if(request_info.bind_user_ids.empty() &&
      request_params.common_info.user_status != AdServer::CampaignSvcs::US_PROBE)
    {
      request_params.common_info.user_status = static_cast<CORBA::ULong>(
        AdServer::CampaignSvcs::US_NOEXTERNALID);
    }

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      Generics::Time end_process_time = Generics::Time::get_time_of_day();
      Stream::Error ostr;
      ostr << FUN << ": user is resolving time = " <<
        (end_process_time - start_process_time);
      logger()->log(ostr.str(),
        Logging::Logger::TRACE,
        ASPECT);
    }

    return true;
  }

  void
  ProfilingServer::rebind_user_id_(
    const AdServer::Commons::UserId& user_id,
    const AdServer::CampaignSvcs::CampaignManager::RequestParams& /*request_params*/,
    const RequestInfo& request_info,
    const String::SubString& resolved_ext_user_id,
    const Generics::Time& time)
    noexcept
  {
    static const char* FUN = "ProfilingServer::rebind_user_id_()";

    if(user_bind_client_)
    {
      const auto grpc_distributor = user_bind_client_->grpc_distributor();
      bool is_grpc_success = false;
      if (grpc_distributor)
      {
        try
        {
          is_grpc_success = true;
          const auto& bind_user_ids = request_info.bind_user_ids;

          for (const auto& full_external_user_id : bind_user_ids)
          {
            if(resolved_ext_user_id != full_external_user_id &&
              !full_external_user_id.empty() &&
              *full_external_user_id.begin() != '/')
            {
              std::optional<AdServer::Commons::UserId> internal_user_id;

              if(full_external_user_id.compare(0, 2, "c/") == 0)
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

              if(!internal_user_id || user_id != *internal_user_id)
              {
                auto response = grpc_distributor->add_user_id(
                  full_external_user_id,
                  time,
                  GrpcAlgs::pack_user_id(user_id));
                if (!response || response->has_error())
                {
                  GrpcAlgs::print_grpc_error_response(
                    response,
                    logger(),
                    ASPECT);
                  throw Exception("add_user_id is failed");
                }
              }
            }
          }
        }
        catch (const eh::Exception& exc)
        {
          is_grpc_success = false;
          Stream::Error stream;
          stream << FUN
                 << ": "
                 << exc.what();
          logger()->error(stream.str(), ASPECT);
        }
        catch (...)
        {
          is_grpc_success = false;
          Stream::Error stream;
          stream << FUN
                 << ": Unknown error";
          logger()->error(stream.str(), ASPECT);
        }
      }

      if (!is_grpc_success)
      {
        try
        {
          AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper =
            user_bind_client_->user_bind_mapper();

          for(auto ext_user_id_it = request_info.bind_user_ids.begin();
            ext_user_id_it != request_info.bind_user_ids.end();
            ++ext_user_id_it)
          {
            const std::string& full_external_user_id = *ext_user_id_it;

            if(resolved_ext_user_id != full_external_user_id &&
              !full_external_user_id.empty() &&
              *full_external_user_id.begin() != '/')
            {
              std::optional<AdServer::Commons::UserId> internal_user_id;

              if(full_external_user_id.compare(0, 2, "c/") == 0)
              {
                try
                {
                  internal_user_id = AdServer::Commons::UserId(
                    String::SubString(full_external_user_id).substr(2));
                }
                catch(const eh::Exception&)
                {}
              }

              if(!internal_user_id || user_id != *internal_user_id)
              {
                AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo
                  add_user_request_info;
                add_user_request_info.id << full_external_user_id;
                add_user_request_info.user_id = CorbaAlgs::pack_user_id(user_id);
                add_user_request_info.timestamp = CorbaAlgs::pack_time(time);

                AdServer::UserInfoSvcs::UserBindServer::AddUserResponseInfo_var
                  prev_user_bind_info =
                    user_bind_mapper->add_user_id(add_user_request_info);

                (void)prev_user_bind_info;
              }
            }
          }
        }
        catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady& )
        {
          Stream::Error ostr;
          ostr << FUN << ": caught UserBindMapper::NotReady";

          logger()->log(ostr.str(),
            Logging::Logger::WARNING,
            ASPECT,
            "ADS-IMPL-10681");
        }
        catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& )
        {
          Stream::Error ostr;
          ostr << FUN << ": caught UserBindMapper::ChunkNotFound";

          logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-IMPL-10681");
        }
        catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught UserBindMapper::ImplementationException: " <<
            ex.description;

          logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-IMPL-10681");
        }
        catch(const CORBA::SystemException& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught CORBA::SystemException: " << e;
          logger()->log(ostr.str(),
            Logging::Logger::ERROR,
            ASPECT,
            "ADS-ICON-7800");
        }
      }
    }
  }

  void
  ProfilingServer::trigger_match_(
    AdServer::ChannelSvcs::ChannelServerBase::MatchResult_out trigger_matched_channels,
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    const RequestInfo& request_info,
    const AdServer::Commons::UserId& user_id)
    noexcept
  {
    static const char* FUN = "ProfilingServer::trigger_match_()";

    if(!request_info.filter_request)
    {
      try
      {
        AdServer::ChannelSvcs::ChannelServerBase::MatchQuery query;
        query.non_strict_word_match = false;
        query.non_strict_url_match = false;
        query.return_negative = false;
        query.simplify_page = true;
        query.fill_content = true;
        query.statuses[0] = 'A';
        query.statuses[1] = '\0';
        query.first_url = request_params.common_info.referer;
        
        try
        {
          std::string ref_words;
          FrontendCommons::extract_url_keywords(
            ref_words,
            String::SubString(request_params.common_info.referer),
            0 // segmentor
            );

          if (!ref_words.empty())
          {
            query.first_url_words << ref_words;
          }
        }
        catch (const eh::Exception& e)
        {
          if(logger()->log_level() >= Logging::Logger::DEBUG)
          {
            Stream::Error ostr;
            ostr << FUN << " url keywords extracting error: " << e.what();
            logger()->log(ostr.str(),
              Logging::Logger::DEBUG,
              ASPECT);
          }
        }

        // check multiline
        {
          std::ostringstream urls_ostr;
          std::ostringstream urls_words_ostr;
          bool url_word_added = false;
          for(CORBA::ULong i = 0; i < request_params.common_info.urls.length(); ++i)
          {
            urls_ostr << (i != 0 ? "\n" : "") <<
              request_params.common_info.urls[i];

            std::string url_words_res;
            FrontendCommons::extract_url_keywords(
              url_words_res,
              String::SubString(request_params.common_info.urls[i]),
              0 // segmentor
              );

            if (!url_words_res.empty())
            {
              urls_words_ostr << (url_word_added ? "\n" : "") << url_words_res;
              url_word_added = true;
            }
          }
          query.urls << urls_ostr.str();
          std::string tmp = urls_words_ostr.str();
          if (!tmp.empty())
          {
            query.urls_words << tmp;
          }
        }

        query.pwords << request_info.keywords;
        query.swords << request_info.search_words;
        query.uid = CorbaAlgs::pack_user_id(user_id);

        channel_servers_->match(query, trigger_matched_channels);

        request_params.trigger_match_result.pkw_channels.length(
          trigger_matched_channels->matched_channels.page_channels.length());
        std::transform(
          trigger_matched_channels->matched_channels.page_channels.get_buffer(),
          trigger_matched_channels->matched_channels.page_channels.get_buffer() +
            trigger_matched_channels->matched_channels.page_channels.length(),
          request_params.trigger_match_result.pkw_channels.get_buffer(),
          convert_channel_atom);
        request_params.trigger_match_result.url_channels.length(
          trigger_matched_channels->matched_channels.url_channels.length());
        std::transform(
          trigger_matched_channels->matched_channels.url_channels.get_buffer(),
          trigger_matched_channels->matched_channels.url_channels.get_buffer() +
            trigger_matched_channels->matched_channels.url_channels.length(),
          request_params.trigger_match_result.url_channels.get_buffer(),
          convert_channel_atom);
        request_params.trigger_match_result.ukw_channels.length(
          trigger_matched_channels->matched_channels.url_keyword_channels.length());
        std::transform(
          trigger_matched_channels->matched_channels.url_keyword_channels.get_buffer(),
          trigger_matched_channels->matched_channels.url_keyword_channels.get_buffer() +
            trigger_matched_channels->matched_channels.url_keyword_channels.length(),
          request_params.trigger_match_result.ukw_channels.get_buffer(),
          convert_channel_atom);
        request_params.trigger_match_result.skw_channels.length(
          trigger_matched_channels->matched_channels.search_channels.length());
        std::transform(
          trigger_matched_channels->matched_channels.search_channels.get_buffer(),
          trigger_matched_channels->matched_channels.search_channels.get_buffer() +
            trigger_matched_channels->matched_channels.search_channels.length(),
          request_params.trigger_match_result.skw_channels.get_buffer(),
          convert_channel_atom);
        CorbaAlgs::copy_sequence(
          trigger_matched_channels->matched_channels.uid_channels,
          request_params.trigger_match_result.uid_channels);

        if(request_params.common_info.user_status ==
           static_cast<CORBA::ULong>(AdServer::CampaignSvcs::US_OPTIN) &&
           (trigger_matched_channels->no_track ||
            trigger_matched_channels->no_adv))
        {
          request_params.common_info.user_status = static_cast<CORBA::ULong>(
            AdServer::CampaignSvcs::US_BLACKLISTED);
        }
      }
      catch(const FrontendCommons::ChannelServerSessionPool::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": caught ChannelServerSessionPool::Exception: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-117");
      }
    }
  }

  void
  ProfilingServer::history_match_(
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result_out,
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    const RequestInfo& request_info,
    const AdServer::ChannelSvcs::ChannelServerBase::MatchResult* trigger_matching_result)
    noexcept
  {
    static const char* FUN = "ProfilingServer::history_match_()";

    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var match_result;
    bool do_history_matching =
      request_params.common_info.user_status == AdServer::CampaignSvcs::US_OPTIN ||
      request_params.common_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;
    bool match_success = false;

    const auto grpc_distributor = user_info_client_->grpc_distributor();
    bool is_grpc_success = false;
    if (grpc_distributor)
    {
      using MatchParams = AdServer::UserInfoSvcs::Types::MatchParams;
      using UserInfo = AdServer::UserInfoSvcs::Types::UserInfo;

      try
      {
        is_grpc_success = true;

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

        if(request_info.coord_location.in())
        {
          auto& geo_data_seq = match_params.geo_data_seq;
          geo_data_seq.emplace_back(
            request_info.coord_location->latitude,
            request_info.coord_location->longitude,
            request_info.coord_location->accuracy);
        }

        if (trigger_matching_result)
        {
          using ChannelMatchSet = std::set<ChannelMatch>;

          ChannelMatchSet url_channels;
          ChannelMatchSet page_channels;
          ChannelMatchSet search_channels;
          ChannelMatchSet url_keyword_channels;

          std::transform(
            trigger_matching_result->matched_channels.url_channels.get_buffer(),
            trigger_matching_result->matched_channels.url_channels.get_buffer() +
              trigger_matching_result->matched_channels.url_channels.length(),
            std::inserter(url_channels, url_channels.end()),
            GetChannelTriggerId());

          std::transform(
            trigger_matching_result->matched_channels.page_channels.get_buffer(),
            trigger_matching_result->matched_channels.page_channels.get_buffer() +
              trigger_matching_result->matched_channels.page_channels.length(),
            std::inserter(page_channels, page_channels.end()),
            GetChannelTriggerId());

          std::transform(
            trigger_matching_result->matched_channels.search_channels.get_buffer(),
            trigger_matching_result->matched_channels.search_channels.get_buffer() +
              trigger_matching_result->matched_channels.search_channels.length(),
            std::inserter(search_channels, search_channels.end()),
            GetChannelTriggerId());

          std::transform(
            trigger_matching_result->matched_channels.url_keyword_channels.get_buffer(),
            trigger_matching_result->matched_channels.url_keyword_channels.get_buffer() +
              trigger_matching_result->matched_channels.url_keyword_channels.length(),
            std::inserter(url_keyword_channels, url_keyword_channels.end()),
            GetChannelTriggerId());

          match_params.url_channel_ids.reserve(url_channels.size());
          for (const auto& url_channel : url_channels)
          {
            match_params.url_channel_ids.emplace_back(
              url_channel.channel_id,
              url_channel.channel_trigger_id);
          }

          match_params.page_channel_ids.reserve(page_channels.size());
          for (const auto& page_channel : page_channels)
          {
            match_params.page_channel_ids.emplace_back(
              page_channel.channel_id,
              page_channel.channel_trigger_id);
          }

          match_params.search_channel_ids.reserve(search_channels.size());
          for (const auto& search_channel : search_channels)
          {
            match_params.search_channel_ids.emplace_back(
              search_channel.channel_id,
              search_channel.channel_trigger_id);
          }

          match_params.url_keyword_channel_ids.reserve(url_keyword_channels.size());
          for (const auto& url_keyword_channel : url_keyword_channels)
          {
            match_params.url_keyword_channel_ids.emplace_back(
              url_keyword_channel.channel_id,
              url_keyword_channel.channel_trigger_id);
          }
        }

        UserInfo user_info;

        if (request_params.common_info.user_id.length() != 0)
        {
          user_info.user_id.resize(
            request_params.common_info.user_id.length());
          std::memcpy(
            user_info.user_id.data(),
            request_params.common_info.user_id.get_buffer(),
            request_params.common_info.user_id.length());
        }

        user_info.last_colo_id = request_params.common_info.colo_id;
        user_info.request_colo_id = request_params.common_info.colo_id;
        user_info.current_colo_id = -1;
        user_info.temporary =
          request_params.common_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY;
        user_info.time = CorbaAlgs::unpack_time(request_params.common_info.time).tv_sec;

        auto response = grpc_distributor->match(
          user_info,
          match_params);
        if (response && response->has_info())
        {
          match_result = AdServer::UserInfoSvcs::convertor_proto_match_result(
            response->info().match_result());
          match_success = true;
        }
        else
        {
          is_grpc_success = false;
          GrpcAlgs::print_grpc_error_response(
            response,
            logger(),
            ASPECT);
        }
      }
      catch (const eh::Exception& exc)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FUN
               << ": "
               << exc.what();
        logger()->error(stream.str(), ASPECT);
      }
      catch (...)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FUN
               << ": Unknown error";
        logger()->error(stream.str(), ASPECT);
      }
    }

    AdServer::UserInfoSvcs::UserInfoMatcher_var uim_session =
      user_info_client_->user_info_session();

    if(!is_grpc_success && uim_session.in() && do_history_matching)
    {
      try
      {
        AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams match_params;
        match_params.use_empty_profile = false;
        match_params.silent_match = false;
        match_params.no_match = false;
        match_params.no_result = false;
        match_params.ret_freq_caps = false;
        match_params.provide_channel_count = false;
        match_params.provide_persistent_channels = false;
        match_params.change_last_request = true;
        match_params.filter_contextual_triggers = false;
        match_params.publishers_optin_timeout = CorbaAlgs::pack_time(Generics::Time::ZERO);
        match_params.cohort2 << request_info.cohort2;

        if(request_info.coord_location.in())
        {
          match_params.geo_data_seq.length(1);

          match_params.geo_data_seq[0].latitude =
            CorbaAlgs::pack_decimal<AdServer::CampaignSvcs::CoordDecimal>(
              request_info.coord_location->latitude);
          match_params.geo_data_seq[0].longitude =
            CorbaAlgs::pack_decimal<AdServer::CampaignSvcs::CoordDecimal>(
              request_info.coord_location->longitude);
          match_params.geo_data_seq[0].accuracy =
            CorbaAlgs::pack_decimal<AdServer::CampaignSvcs::AccuracyDecimal>(
              request_info.coord_location->accuracy);
        }

        if(trigger_matching_result)
        {
          prepare_ui_match_params_(
            match_params,
            *trigger_matching_result);
        }

        // process match request
        AdServer::UserInfoSvcs::UserInfo user_info;

        user_info.user_id = request_params.common_info.user_id;
        user_info.last_colo_id = request_params.common_info.colo_id;
        user_info.request_colo_id = request_params.common_info.colo_id;
        user_info.current_colo_id = -1;
        user_info.temporary = (
          request_params.common_info.user_status == AdServer::CampaignSvcs::US_TEMPORARY);
        user_info.time = CorbaAlgs::unpack_time(request_params.common_info.time).tv_sec;

        uim_session->match(
          user_info,
          match_params,
          match_result.out());

        match_success = true;
      }
      catch(const UserInfoSvcs::UserInfoMatcher::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": UserInfoSvcs::UserInfoMatcher::ImplementationException caught: " <<
          e.description;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-112");
      }
      catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
      {
        logger()->log(
          String::SubString("UserInfoManager not ready for matching."),
          TraceLevel::MIDDLE,
          ASPECT);
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Can't match history channels. Caught CORBA::SystemException: " <<
          ex;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-ICON-2");
      }

      if(!match_result.ptr())
      {
        match_result = get_empty_history_matching_();
      }

      /* log user info request */
      if(logger()->log_level() >= TraceLevel::MIDDLE)
      {
        const AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq& channels =
          match_result->channels;

        std::ostringstream ostr;
        ostr << FUN << ": history matched channels for uid = '" <<
          CorbaAlgs::unpack_user_id(request_params.common_info.user_id) << "': ";

        if(channels.length() == 0)
        {
          ostr << "empty";
        }
        else
        {
          CorbaAlgs::print_sequence_field(ostr,
            channels,
            &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeight::channel_id);
        }

        logger()->log(ostr.str(),
          TraceLevel::MIDDLE,
          ASPECT);
      }
    }
    else if(!uim_session.in())
    {
      logger()->log(
        String::SubString("Match with non resolved user info session."),
        TraceLevel::MIDDLE,
        ASPECT);
    }

    if(!match_result.ptr())
    {
      match_result = get_empty_history_matching_();
    }

    if(trigger_matching_result && (
         !match_success || !do_history_matching))
    {
      // fill history channels with context channels
      AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq&
        history_matched_channels = match_result->channels;
      const AdServer::ChannelSvcs::ChannelServerBase::ContentChannelAtomSeq&
        content_channels = trigger_matching_result->content_channels;

      history_matched_channels.length(content_channels.length());

      std::copy(content_channels.get_buffer(),
        content_channels.get_buffer() + content_channels.length(),
        Algs::modify_inserter(history_matched_channels.get_buffer(),
          ContextualChannelConverter()));
    }

    // fill request params
    request_params.client_create_time = match_result->create_time;
    request_params.fraud = match_result->fraud_request;

    request_params.channels.length(match_result->channels.length());
    for(CORBA::ULong i = 0; i < match_result->channels.length(); ++i)
    {
      request_params.channels[i] = match_result->channels[i].channel_id;
    }

    request_params.session_start = match_result->session_start;

    match_result_out = match_result._retn();
  }

  void
  ProfilingServer::merge_users_(
    const AdServer::Commons::UserId& target_user_id,
    const AdServer::Commons::UserId& source_user_id,
    bool source_temporary,
    const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params)
    noexcept
  {
    static const char* FUN = "ProfilingServer::merge_users_()";

    const bool REMOVE_SOURCE_USER_PROFILE = false;

    const auto grpc_distributor = user_info_client_->grpc_distributor();

    bool merge_success = false;
    bool is_grpc_success = false;
    if (grpc_distributor)
    {
      using ProfilesRequestInfo = AdServer::UserInfoSvcs::Types::ProfilesRequestInfo;
      using UserInfo = AdServer::UserInfoSvcs::Types::UserInfo;
      using MatchParams = AdServer::UserInfoSvcs::Types::MatchParams;
      using UserProfiles = AdServer::UserInfoSvcs::Types::UserProfiles;

      try
      {
        is_grpc_success = true;

        const auto merged_uid_info = GrpcAlgs::pack_user_id(
          source_user_id);

        ProfilesRequestInfo profiles_request;
        profiles_request.base_profile = true;
        profiles_request.add_profile = true;
        profiles_request.history_profile = true;
        profiles_request.freq_cap_profile = true;
        profiles_request.pref_profile = false;

        auto get_user_profile_response = grpc_distributor->get_user_profile(
          merged_uid_info,
          source_temporary,
          profiles_request);
        if (!get_user_profile_response || get_user_profile_response->has_error())
        {
          GrpcAlgs::print_grpc_error_response(
            get_user_profile_response,
            logger(),
            ASPECT);
          throw Exception("get_user_profile is failed");
        }

        const auto& get_user_profile_response_info = get_user_profile_response->info();
        const auto& user_profiles_proto = get_user_profile_response_info.user_profiles();

        if (!get_user_profile_response_info.return_value())
          return;

        if (user_profiles_proto.base_user_profile().empty()
         || user_profiles_proto.add_user_profile().empty())
          return;

        if (REMOVE_SOURCE_USER_PROFILE)
        {
          auto response = grpc_distributor->remove_user_profile(merged_uid_info);
          if (!response || response->has_error())
          {
            GrpcAlgs::print_grpc_error_response(
              response,
              logger(),
              ASPECT);
            throw Exception("remove_user_profile is failed");
          }
        }

        UserInfo user_info;
        user_info.user_id = GrpcAlgs::pack_user_id(target_user_id);
        user_info.last_colo_id = request_params.common_info.colo_id;
        user_info.request_colo_id = request_params.common_info.colo_id;
        user_info.current_colo_id = -1;
        user_info.temporary = false;
        user_info.time = CorbaAlgs::unpack_time(
          request_params.common_info.time).tv_sec;

        MatchParams match_params;
        match_params.use_empty_profile = false;
        match_params.silent_match = false;
        match_params.no_match = false;
        match_params.no_result = false;
        match_params.provide_persistent_channels = false;
        match_params.change_last_request = false;
        match_params.filter_contextual_triggers = false;
        match_params.publishers_optin_timeout = Generics::Time::ZERO;

        UserProfiles merge_user_profiles;
        merge_user_profiles.add_user_profile = user_profiles_proto.add_user_profile();
        merge_user_profiles.base_user_profile = user_profiles_proto.base_user_profile();
        merge_user_profiles.pref_profile = user_profiles_proto.pref_profile();
        merge_user_profiles.history_user_profile = user_profiles_proto.history_user_profile();
        merge_user_profiles.freq_cap = user_profiles_proto.freq_cap();

        auto merge_response = grpc_distributor->merge(
          user_info,
          match_params,
          merge_user_profiles);
        if (!merge_response || merge_response->has_error())
        {
          GrpcAlgs::print_grpc_error_response(
              merge_response,
            logger(),
            ASPECT);
          throw Exception("merge is failed");
        }
      }
      catch (const eh::Exception& exc)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FUN
                << ": "
                << exc.what();
        logger()->error(stream.str(), ASPECT);
      }
      catch (...)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FUN
               << ": Unknown error";
        logger()->error(stream.str(), ASPECT);
      }
    }

    if (is_grpc_success)
      return;

    AdServer::UserInfoSvcs::UserInfoMatcher_var uim_session =
      user_info_client_->user_info_session();

    AdServer::UserInfoSvcs::UserProfiles_var merge_user_profile;

    try
    {
      CORBACommons::UserIdInfo merged_uid_info = CorbaAlgs::pack_user_id(
        source_user_id);

      if(uim_session.in())
      {
        AdServer::UserInfoSvcs::ProfilesRequestInfo profiles_request;
        profiles_request.base_profile = true;
        profiles_request.add_profile = true;
        profiles_request.history_profile = true;
        profiles_request.freq_cap_profile = true;
        profiles_request.pref_profile = false;

        merge_success = uim_session->get_user_profile(
          merged_uid_info,
          source_temporary,
          profiles_request,
          merge_user_profile.out());

        merge_success = (
          merge_user_profile->base_user_profile.length() != 0 &&
          merge_user_profile->add_user_profile.length() != 0);
      }

      if(merge_success && REMOVE_SOURCE_USER_PROFILE)
      {
        AdServer::UserInfoSvcs::UserInfoMatcher_var
          uim_session = user_info_client_->user_info_session();

        if(uim_session.in())
        {
          uim_session->remove_user_profile(merged_uid_info);
        }
      }
    }
    catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
    {
      logger()->log(
        String::SubString("UserInfoManager not ready for merging."),
        TraceLevel::MIDDLE,
        ASPECT);
    }
    catch(const UserInfoSvcs::UserInfoManager::ImplementationException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get user profile for merging."
        "Caught UserInfoManager::ImplementationException: " <<
        ex.description;

      logger()->log(ostr.str(),
        Logging::Logger::NOTICE,
        ASPECT,
        "ADS-IMPL-111");
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get user profile for merging."
        "Caught CORBA::SystemException: " <<
        ex;

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        ASPECT,
        "ADS-ICON-2");
    }

    if(merge_success)
    {
      try
      {
        merge_success = false;

        AdServer::UserInfoSvcs::UserInfo user_info;
        user_info.user_id = CorbaAlgs::pack_user_id(target_user_id);
        user_info.last_colo_id = request_params.common_info.colo_id;
        user_info.request_colo_id = request_params.common_info.colo_id;
        user_info.current_colo_id = -1;
        user_info.temporary = false;
        user_info.time = CorbaAlgs::unpack_time(
          request_params.common_info.time).tv_sec;

        AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams match_params;
        match_params.use_empty_profile = false;
        match_params.silent_match = false;
        match_params.no_match = false;
        match_params.no_result = false;
        match_params.provide_persistent_channels = false;
        match_params.change_last_request = false;
        match_params.filter_contextual_triggers = false;
        match_params.publishers_optin_timeout = CorbaAlgs::pack_time(Generics::Time::ZERO);
        
        CORBACommons::TimestampInfo_var last_req;
        uim_session->merge(
          user_info,
          match_params,
          merge_user_profile.in(),
          merge_success,
          last_req);

        merge_success = true;
      }
      catch(const UserInfoSvcs::UserInfoMatcher::ImplementationException& e)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": caught UserInfoSvcs::UserInfoMatcher::ImplementationException: " <<
          e.description;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-IMPL-111");
      }
      catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
      {
        logger()->log(
          String::SubString("UserInfoManager not ready for merging."),
          TraceLevel::MIDDLE,
          ASPECT);
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't merge users. Caught CORBA::SystemException: " <<
          ex;

        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          ASPECT,
          "ADS-ICON-2");
      }
    }
  }

  void
  ProfilingServer::prepare_ui_match_params_(
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
    const AdServer::ChannelSvcs::ChannelServerBase::MatchResult& match_result)
    /*throw(eh::Exception)*/
  {
    typedef std::set<ChannelMatch> ChannelMatchSet;

    if(!match_result.no_track)
    {
      ChannelMatchSet url_channels;
      ChannelMatchSet page_channels;
      ChannelMatchSet search_channels;
      ChannelMatchSet url_keyword_channels;

      std::transform(
        match_result.matched_channels.url_channels.get_buffer(),
        match_result.matched_channels.url_channels.get_buffer() +
        match_result.matched_channels.url_channels.length(),
        std::inserter(url_channels, url_channels.end()),
        GetChannelTriggerId());

      std::transform(
        match_result.matched_channels.page_channels.get_buffer(),
        match_result.matched_channels.page_channels.get_buffer() +
        match_result.matched_channels.page_channels.length(),
        std::inserter(page_channels, page_channels.end()),
        GetChannelTriggerId());

      std::transform(
        match_result.matched_channels.search_channels.get_buffer(),
        match_result.matched_channels.search_channels.get_buffer() +
        match_result.matched_channels.search_channels.length(),
        std::inserter(search_channels, search_channels.end()),
        GetChannelTriggerId());

      std::transform(
        match_result.matched_channels.url_keyword_channels.get_buffer(),
        match_result.matched_channels.url_keyword_channels.get_buffer() +
        match_result.matched_channels.url_keyword_channels.length(),
        std::inserter(url_keyword_channels, url_keyword_channels.end()),
        GetChannelTriggerId());

      match_params.url_channel_ids.length(url_channels.size());
      CORBA::ULong i = 0;
      for (ChannelMatchSet::const_iterator it = url_channels.begin();
           it != url_channels.end(); ++it, ++i)
      {
        match_params.url_channel_ids[i].channel_id = it->channel_id;
        match_params.url_channel_ids[i].channel_trigger_id = it->channel_trigger_id;
      }

      match_params.page_channel_ids.length(page_channels.size());
      i = 0;
      for(ChannelMatchSet::const_iterator it = page_channels.begin();
        it != page_channels.end(); ++it, ++i)
      {
        match_params.page_channel_ids[i].channel_id = it->channel_id;
        match_params.page_channel_ids[i].channel_trigger_id = it->channel_trigger_id;
      }

      match_params.search_channel_ids.length(search_channels.size());
      i = 0;
      for(ChannelMatchSet::const_iterator it = search_channels.begin();
        it != search_channels.end(); ++it, ++i)
      {
        match_params.search_channel_ids[i].channel_id = it->channel_id;
        match_params.search_channel_ids[i].channel_trigger_id = it->channel_trigger_id;
      }

      match_params.url_keyword_channel_ids.length(url_keyword_channels.size());
      i = 0;
      for(ChannelMatchSet::const_iterator it = url_keyword_channels.begin();
        it != url_keyword_channels.end(); ++it, ++i)
      {
        match_params.url_keyword_channel_ids[i].channel_id = it->channel_id;
        match_params.url_keyword_channel_ids[i].channel_trigger_id =
          it->channel_trigger_id;
      }
    }
  }

  void
  ProfilingServer::request_campaign_manager_(
    AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
    AdServer::ChannelSvcs::ChannelServerBase::MatchResult* /*trigger_matched_channels*/,
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult* /*history_match_result*/)
    /*throw(Exception)*/
  {
    static const char* FUN = "ProfilingServer::request_campaign_manager_()";

    try
    {
      AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_var
        campaign_matching_result;

      CORBA::String_var hostname;
      campaign_managers_.get_campaign_creative(
        request_params,
        hostname,
        campaign_matching_result);
    }
    catch(const Exception&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": fail. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult*
  ProfilingServer::get_empty_history_matching_()
    /*throw(eh::Exception)*/
  {
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var res =
      new AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult();
    res->fraud_request = false;
    res->times_inited = false;
    res->last_request_time = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->create_time = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->session_start = CorbaAlgs::pack_time(Generics::Time::ZERO);
    res->colo_id = -1;
    return res._retn();
  }
}
}

int
main(int argc, char** argv)
{
  AdServer::Profiling::ProfilingServer* app = 0;

  try
  {
    app = &AdServer::Profiling::ProfilingServerApp::instance();
  }
  catch(...)
  {
    std::cerr << "main(): Critical: Got exception while "
      "creating application object.\n";
    return -1;
  }

  if(app == 0)
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
