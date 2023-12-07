#include <list>
#include <vector>
#include <iterator>

#include <PrivacyFilter/Filter.hpp>

#include <Generics/DirSelector.hpp>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/FreqCapManip.hpp>

#include <UserInfoSvcs/UserInfoCommons/Allocator.hpp>

#include "UserInfoContainer.hpp"
#include "UserInfoManagerImpl.hpp"
#include "UServerUtils/MetricsRAII.hpp"

namespace Aspect
{
  const char USER_INFO_MANAGER[] = "UserInfoManager";
  const char DELETE_OLD_PROFILES[] = "UserInfoManager:DeleteOldProfiles";
}

namespace
{
  const unsigned long CHUNKS_RELOAD_PERIOD = 30; // 30 sec

  class CompositeActiveObjectImpl:
    public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
  {};

  struct SimpleChannelProperties
  {
    SimpleChannelProperties(
      unsigned long channel_id_val,
      bool discover_val,
      unsigned long behav_param_list_id_val,
      const char* str_behav_param_list_id_val)
      : channel_id(channel_id_val),
        discover(discover_val),
        behav_param_list_id(behav_param_list_id_val),
        str_behav_param_list_id(str_behav_param_list_id_val)
    {}

    unsigned long channel_id;
    bool discover;
    unsigned long behav_param_list_id;
    std::string str_behav_param_list_id;
  };

  typedef std::list<
    SimpleChannelProperties,
    Generics::TAlloc::ThreadPool<SimpleChannelProperties, 256> >
    SimpleChannelPropertiesList;

  struct BehavIdTypeKey
  {
    BehavIdTypeKey(unsigned long id_val, char type_val)
      : id(id_val),
        type(type_val)
    {}

    bool operator <(const BehavIdTypeKey& right) const
    {
      return ((id < right.id) || (id == right.id && type < right.type));
    }

    unsigned long id;
    char type;
  };

  struct StrBehavIdTypeKey
  {
    StrBehavIdTypeKey(const char* id_val, char type_val)
      : id(id_val),
        type(type_val)

    {}

    bool operator <(const StrBehavIdTypeKey& right) const
    {
      return ((id < right.id) || (id == right.id && type < right.type));
    }

    std::string id;
    char type;
  };

  void create_path(std::string& first, const char* second)
    /*throw(eh::Exception)*/
  {
    if(second && second[0] == '/')
    {
      first = second;
      return;
    }
    if(*(first.rbegin()) != '/')
    {
      first += "/";
    }
    first += second;
  }
}

namespace AdServer
{
namespace UserInfoSvcs
{
  void
  convert_mem_buf(
    const Generics::MemBuf& mem_buf,
    CORBACommons::OctSeq& oct_seq)
    /*throw(eh::Exception)*/
  {
    try
    {
      oct_seq.length(mem_buf.size());
      ::memcpy(oct_seq.get_buffer(), mem_buf.data(), mem_buf.size());
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << "convert_mem_buf(): Caught CORBA::SystemException: " << ex;
      throw UserInfoManagerImpl::Exception(ostr);
    }
  }

  /**
   * UserInfoManagerImpl::LoadingProgressProcessor
   */
  UserInfoManagerImpl::
  LoadingProgressProcessor::LoadingProgressProcessor(double range) noexcept
   : range_(range),
     progress_(0.0)
  {}

  void
  UserInfoManagerImpl::
  LoadingProgressProcessor::post_progress(double value) noexcept
  {
    SyncPolicy::WriteGuard lock(progress_lock_);
    progress_ += value;
  }

  std::string
  UserInfoManagerImpl::
  LoadingProgressProcessor::get_progress_in_percents() noexcept
  {
    std::stringstream progress_str;
    SyncPolicy::ReadGuard lock(progress_lock_);
    progress_str << (progress_ / range_) * 100 << "%";
    return progress_str.str();
  }

  /**
   * UserInfoManagerImpl
   */
  UserInfoManagerImpl::UserInfoManagerImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const UserInfoManagerConfig& user_info_manager_config,
    Generics::CompositeMetricsProvider* composite_metrics_provider)
    /*throw(Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_, 5)),
      user_info_manager_config_(user_info_manager_config),
      file_controller_(new AdServer::ProfilingCommons::SSDFileController(
        AdServer::ProfilingCommons::FileController_var(
          new AdServer::ProfilingCommons::PosixFileController(
            nullptr, // stats
            user_info_manager_config.Storage().min_free_space().present() ?
              *user_info_manager_config.Storage().min_free_space() : 0)))),
      user_operation_processor_(new UserOperationProcessorHolder()),
      user_info_container_(new UserInfoContainerHolder()),
      user_info_container_dependent_active_object_(new CompositeActiveObjectImpl()),
      campaignserver_ready_(false),
      current_day_(0),
      daily_users_(0),
      check_operations_callback_(
        new Logging::ActiveObjectCallbackImpl(
          logger_,
          "UserInfoManagerImpl::check_operations_()",
          Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-82")),
      loading_progress_processor_(new LoadingProgressProcessor(1.0)),
      composite_metrics_provider_(ReferenceCounting::add_ref(composite_metrics_provider))

  {
    static const char* FUN = "UserInfoManagerImpl::UserInfoManagerImpl()";

    if (user_info_manager_config.ReadWriteStats().present())
    {
      file_rw_stats_ = new FileRWStats(
        Generics::Time(user_info_manager_config.ReadWriteStats()->interval()),
        user_info_manager_config.ReadWriteStats()->times());
    }

    file_controller_ = new AdServer::ProfilingCommons::SSDFileController(
      AdServer::ProfilingCommons::FileController_var(
        new AdServer::ProfilingCommons::PosixFileController(file_rw_stats_)));

    try
    {
      corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't init CorbaClientAdapter - eh::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }

    try
    {
      add_child_object(task_runner_);
      add_child_object(scheduler_);
      add_child_object(user_operation_processor_);
      add_child_object(user_info_container_);
    }
    catch(const Generics::CompositeActiveObject::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": CompositeActiveObject::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      AdServer::LogProcessing::LogFlushTraits lft;

      lft.period = Generics::Time(60); // TO FIX
      lft.out_dir = user_info_manager_config_.root_dir();

      if (user_info_manager_config_.Logging().present())
      {
        if (user_info_manager_config_.Logging().get().ChannelCountStat().present())
        {
          const xsd::AdServer::Configuration::UserInfoManagerLoggerType& xsd_logger =
            user_info_manager_config_.Logging().get().ChannelCountStat().get();

          create_path(
            lft.out_dir,
            AdServer::LogProcessing::ChannelCountStatTraits::log_base_name());

          lft.period = Generics::Time(
            xsd_logger.flush_period().present() ?
            xsd_logger.flush_period().get() : 60);
        }
      }

      user_info_manager_logger_ = new UserInfoManagerLogger(lft);

      Task_var msg = new FlushLogsTask(this, 0);
      task_runner_->enqueue_task(msg);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
        chunk_folders_,
        user_info_manager_config_.Storage().chunks_root().c_str(),
        "UserChunk");
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      provide_channel_counters_ = false;

      if(user_info_manager_config_.StatsDumper().present() &&
         user_info_manager_config_.StatsDumper().get().
         provide_channel_counters().present())
      {
        provide_channel_counters_ = user_info_manager_config_.StatsDumper().
          get().provide_channel_counters().get();
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      repeat_trigger_timeout_ =
        Generics::Time(user_info_manager_config_.repeat_trigger_timeout());

      profile_lifetime_ = Generics::Time(
        user_info_manager_config_.Storage().BaseChunksConfig().expire_time());

      temp_profile_lifetime_ = Generics::Time(
        user_info_manager_config_.Storage().TempChunksConfig().expire_time());

      opt_uie_receive_criteria_.common_chunks_number =
        user_info_manager_config_.Storage().common_chunks_number();
      opt_uie_receive_criteria_.max_response_plain_size = 10*1024*1024;

      opt_uie_receive_criteria_.chunk_ids.length(chunk_folders_.size());
      CORBA::ULong i = 0;
      for(AdServer::ProfilingCommons::ProfileMapFactory::
            ChunkPathMap::const_iterator chunk_it = chunk_folders_.begin();
          chunk_it != chunk_folders_.end(); ++chunk_it, ++i)
      {
        opt_uie_receive_criteria_.chunk_ids[i] = chunk_it->first;
      }

      Task_var load_chunks_msg = new LoadChunksDataTask(this, 0);
      task_runner_->enqueue_task(load_chunks_msg);

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->sstream(Logging::Logger::TRACE,
          Aspect::USER_INFO_MANAGER) <<
          "LoadChunksDataTask was enqueued.";
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't instantiate object. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    try
    {
      CORBACommons::CorbaObjectRefList campaign_server_refs;

      Config::CorbaConfigReader::read_multi_corba_ref(
        user_info_manager_config_.CampaignServerCorbaRef(),
        campaign_server_refs);

      /* init campaign servers pool */
      campaign_servers_ = resolve_campaign_servers_(
        campaign_server_refs);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": catch eh::Exception: " << e.what();
      throw Exception(ostr);
    }

    try
    {
      if(user_info_manager_config_.UserInfoExchangerParameters().present())
      {
        uie_presents_ = true;

        CORBACommons::CorbaObjectRef corba_object_ref;

        const xsd::AdServer::Configuration::UserInfoExchangerParametersType&
          user_info_exchanger_params =
            *user_info_manager_config_.UserInfoExchangerParameters();

        Config::CorbaConfigReader::read_corba_ref(
          user_info_exchanger_params.UserInfoExchangerRef(),
          corba_object_ref);

        exchange_customer_id_ = user_info_exchanger_params.customer_id();

        {
          std::ostringstream ostr;
          ostr << user_info_manager_config_.colo_id();
          exchange_provider_id_ = ostr.str();
        }

        CORBA::Object_var obj =
          corba_client_adapter_->resolve_object(corba_object_ref);

        user_info_exchanger_ = UserInfoExchanger::_narrow(obj.in());

        if (CORBA::is_nil(user_info_exchanger_.in()))
        {
          throw Exception(
            "Can't resolve UserInfoExchanger - _narrow return nil reference");
        }

        Task_var get_last_colo_profile_msg =
          new GetLastColoProfilesTask(this, 0);
        task_runner_->enqueue_task(get_last_colo_profile_msg);

        if(logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->sstream(Logging::Logger::TRACE,
            Aspect::USER_INFO_MANAGER) <<
            "GetLastColoProfilesTask was enqueued.";
        }
      }
      else
      {
        uie_presents_ = false;
      }

      placement_colo_id_ = user_info_manager_config_.colo_id();
    }
    catch(const CORBA::SystemException& e)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": catch CORBA::SystemException when reading UserInfoExchanger reference: " <<
        e;
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Caught eh::Exception when reading UserInfoExchanger reference: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  void
  UserInfoManagerImpl::wait_object()
    /*throw(Generics::ActiveObject::Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::wait_object();

    // deactivate (dump) user_info_container only when finished
    // all operations on it (through user_info_processor or directly)
    UserInfoContainer_var user_info_container = user_info_container_->get_object();
    if(user_info_container)
    {
      user_info_container->deactivate_object();
      user_info_container->wait_object();
    }

    user_info_container_dependent_active_object_->deactivate_object();
    user_info_container_dependent_active_object_->wait_object();
  }

  CORBA::Boolean
  UserInfoManagerImpl::uim_ready() noexcept
  {
    UserInfoContainerAccessor user_info_container =
      get_user_info_container_(false);

    return user_info_container.get().in() &&
      user_info_container->channels_config().in();
  }

  char*
  UserInfoManagerImpl::get_progress() noexcept
  {
    UserInfoContainerAccessor user_info_container = get_user_info_container_(false);

    std::stringstream str;
    if (!user_info_container.get().in())
    {
      str << "chunks: " << loading_progress_processor_->get_progress_in_percents();
    }
    else if (!user_info_container->channels_config().in())
    {
      str << "channels loading...";
    }

    return CORBA::string_dup(str.str().c_str());
  }

  UserInfoManagerImpl::CampaignServerPoolPtr
  UserInfoManagerImpl::resolve_campaign_servers_(
    const CORBACommons::CorbaObjectRefList& campaign_server_refs)
    /*throw(Exception, eh::Exception)*/
  {
    static const char* FUN = "UserInfoManagerImpl::resolve_campaign_servers_()";

    try
    {
      CampaignServerPoolConfig pool_config(corba_client_adapter_.in());
      pool_config.timeout = Generics::Time(10); // 10 sec

      std::copy(
        campaign_server_refs.begin(),
        campaign_server_refs.end(),
        std::back_inserter(pool_config.iors_list));

      return CampaignServerPoolPtr(new CampaignServerPool(
        pool_config, CORBACommons::ChoosePolicyType::PT_PERSISTENT));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void UserInfoManagerImpl::calc_user_daily_stat_(
    const Generics::Time& current_time,
    const Generics::Time& request_time,
    const Generics::Time& user_time,
    const Generics::Time& time_offset)
    noexcept
  {
    unsigned long day = (current_time + time_offset).tv_sec /
      Generics::Time::ONE_DAY.tv_sec;
    unsigned long request_day = (request_time + time_offset).tv_sec /
      Generics::Time::ONE_DAY.tv_sec;
    unsigned long user_day = (user_time + time_offset).tv_sec /
      Generics::Time::ONE_DAY.tv_sec;
    if(request_day == day)
    {
      SyncPolicy::WriteGuard guard(daily_stat_lock_);
      if(current_day_ != day)
      {
        current_day_ = day;
        daily_users_ = 0;
      }
      if(current_day_ == request_day && user_day != request_day)
      {
        daily_users_++;
      }
    }
  }

  void
  UserInfoManagerImpl::update_user_freq_caps(
    const CORBACommons::UserIdInfo& user_id_info,
    const CORBACommons::TimestampInfo& time,
    const CORBACommons::RequestIdInfo& request_id,
    const FreqCapIdSeq& freq_caps,
    const FreqCapIdSeq& uc_freq_caps,
    const FreqCapIdSeq& virtual_freq_caps,
    const AdServer::UserInfoSvcs::UserInfoManager::SeqOrderSeq& seq_orders_seq,
    const CampaignIdSeq& campaign_ids_seq,
    const CampaignIdSeq& uc_campaign_ids_seq)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerImpl::post_match()";

    try
    {
      UserOperationProcessorAccessor user_operation_processor =
        get_user_operation_processor_();

      Generics::Time now(CorbaAlgs::unpack_time(time));

      UserFreqCapProfile::FreqCapIdList fcs;
      UserFreqCapProfile::FreqCapIdList uc_fcs;
      UserFreqCapProfile::FreqCapIdList virtual_fcs;
      UserFreqCapProfile::SeqOrderList seq_orders;
      UserFreqCapProfile::CampaignIds campaign_ids;
      UserFreqCapProfile::CampaignIds uc_campaign_ids;
      CorbaAlgs::convert_sequence(freq_caps, fcs);
      CorbaAlgs::convert_sequence(uc_freq_caps, uc_fcs);
      CorbaAlgs::convert_sequence(virtual_freq_caps, virtual_fcs);
      CorbaAlgs::convert_sequence(campaign_ids_seq, campaign_ids);
      CorbaAlgs::convert_sequence(uc_campaign_ids_seq, uc_campaign_ids);

      for(CORBA::ULong i = 0; i < seq_orders_seq.length(); ++i)
      {
        UserFreqCapProfile::SeqOrder seq_order;
        seq_order.ccg_id = seq_orders_seq[i].ccg_id;
        seq_order.set_id = seq_orders_seq[i].set_id;
        seq_order.imps = seq_orders_seq[i].imps;
        seq_orders.push_back(seq_order);
      }

      user_operation_processor->update_freq_caps(
        CorbaAlgs::unpack_user_id(user_id_info),
        now,
        CorbaAlgs::unpack_request_id(request_id),
        fcs,
        uc_fcs,
        virtual_fcs,
        seq_orders,
        campaign_ids,
        uc_campaign_ids,
        AdServer::ProfilingCommons::OP_RUNTIME);
    }
    catch(const UserOperationProcessor::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match. "
        "Caught UserOperationProcessor::NotReady: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::NotReady>(
          ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ChunkNotFound>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match. Caught eh::Exception: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
  }

  void UserInfoManagerImpl::confirm_user_freq_caps(
    const CORBACommons::UserIdInfo& user_id_info,
    const CORBACommons::TimestampInfo& time,
    const CORBACommons::RequestIdInfo& request_id_info,
    const CORBACommons::IdSeq& exclude_pubpixel_accounts)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerImpl::post_match()";

    try
    {
      UserOperationProcessorAccessor user_operation_processor =
        get_user_operation_processor_();

      std::set<unsigned long> exclude_pubpixel_accs;
      for(CORBA::ULong i = 0; i < exclude_pubpixel_accounts.length(); ++i)
      {
        exclude_pubpixel_accs.insert(exclude_pubpixel_accounts[i]);
      }

      user_operation_processor->confirm_freq_caps(
        CorbaAlgs::unpack_user_id(user_id_info),
        CorbaAlgs::unpack_time(time),
        CorbaAlgs::unpack_request_id(request_id_info),
        exclude_pubpixel_accs);
    }
    catch(const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::NotReady>(
          ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ChunkNotFound>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match. Caught eh::Exception: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
  }

  void
  UserInfoManagerImpl::consider_publishers_optin(
    const CORBACommons::UserIdInfo& user_id_info,
    const CORBACommons::IdSeq& exclude_pubpixel_accounts,
    const CORBACommons::TimestampInfo& now)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerImpl::consider_publishers_optin()";

    try
    {
      UserOperationProcessorAccessor user_operation_processor =
        get_user_operation_processor_();

      std::set<unsigned long> exclude_pubpixel_accs;
      for(CORBA::ULong i = 0; i < exclude_pubpixel_accounts.length(); ++i)
      {
        exclude_pubpixel_accs.insert(exclude_pubpixel_accounts[i]);
      }

      user_operation_processor->consider_publishers_optin(
        CorbaAlgs::unpack_user_id(user_id_info),
        exclude_pubpixel_accs,
        CorbaAlgs::unpack_time(now),
        AdServer::ProfilingCommons::OP_RUNTIME);
    }
    catch (const UserOperationProcessor::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't consider publisher. "
        "Caught UserOperationProcessor::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ChunkNotFound>(
          ostr.str());
    }
    catch(const UserOperationProcessor::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't consider publisher. "
        "Caught UserInfoContainer::Exception: " <<
        ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't consider publisher. "
        "Caught eh::Exception: " <<
        ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
  }

  CORBA::Boolean
  UserInfoManagerImpl::remove_user_profile(
    const CORBACommons::UserIdInfo& user_id_info)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerImpl::remove_user_profile()";

    try
    {
      UserOperationProcessorAccessor user_operation_processor =
        get_user_operation_processor_();

      return user_operation_processor->remove_user_profile(
        CorbaAlgs::unpack_user_id(user_id_info));
    }
    catch(const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't remove user profile. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::NotReady>(
          ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't remove user profile. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ChunkNotFound>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't remove user profile. "
        "Caught eh::Exception: " <<
        ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  void
  UserInfoManagerImpl::get_master_stamp(
    CORBACommons::TimestampInfo_out master_stamp)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    static const char* FUN = "UserInfoManager::get_master_stamp()";

    try
    {
      UserInfoContainerAccessor user_info_container = get_user_info_container_();

      master_stamp = new CORBACommons::TimestampInfo;
      *master_stamp =
        CorbaAlgs::pack_time(user_info_container->master_stamp());
    }
    catch(const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't get config master stamp. "
        "Caught UserInfoContainer::Exception: " <<
        ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::NotReady>(
          ostr.str());
    }
    catch(const UserInfoContainer::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get config master stamp. "
        "Caught UserInfoContainer::Exception: " <<
        ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't config master stamp. "
        "Caught eh::Exception: " <<
        ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
  }

  CORBA::Boolean
  UserInfoManagerImpl::get_user_profile(
    const CORBACommons::UserIdInfo& user_id_info,
    CORBA::Boolean temporary,
    const AdServer::UserInfoSvcs::ProfilesRequestInfo& profile_request,
    AdServer::UserInfoSvcs::UserProfiles_out user_profile)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManager::get_user_profile()";

    metrics_raii raii_tmp(composite_metrics_provider_, "UserInfoManagerImpl::get_user_profile");
    try
    {
      UserInfoContainerAccessor user_info_container = get_user_info_container_();

      SmartMemBuf_var mb_base_profile_out;
      SmartMemBuf_var mb_add_profile_out;
      SmartMemBuf_var mb_history_profile_out;
      SmartMemBuf_var mb_fc_profile_out;
      SmartMemBuf_var mb_pref_profile_out;

      if(user_info_container->get_user_profile(
           CorbaAlgs::unpack_user_id(user_id_info),
           temporary,
           profile_request.base_profile ? &mb_base_profile_out : 0,
           profile_request.add_profile ? &mb_add_profile_out : 0,
           profile_request.history_profile ? &mb_history_profile_out : 0,
           profile_request.freq_cap_profile ? &mb_fc_profile_out : 0))
      {
        user_profile = new AdServer::UserInfoSvcs::UserProfiles();

        if (mb_base_profile_out.in())
        {
          convert_mem_buf(mb_base_profile_out->membuf(),
            user_profile->base_user_profile);
        }

        if (mb_add_profile_out.in())
        {
          convert_mem_buf(mb_add_profile_out->membuf(),
            user_profile->add_user_profile);
        }

        if (mb_history_profile_out.in())
        {
          convert_mem_buf(mb_history_profile_out->membuf(),
            user_profile->history_user_profile);
        }

        if(mb_fc_profile_out.in())
        {
          convert_mem_buf(
            mb_fc_profile_out->membuf(),
            user_profile->freq_cap);
        }

        if(mb_pref_profile_out.in())
        {
          convert_mem_buf(mb_pref_profile_out->membuf(),
            user_profile->pref_profile);
        }

        return true;
      }

      user_profile = new AdServer::UserInfoSvcs::UserProfiles();

      return false;
    }
    catch(const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't get user profile. Caught UserInfoContainer::Exception: " <<
        ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::NotReady>(
          ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get user profile. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ChunkNotFound>(
          ostr.str());
    }
    catch(const UserInfoContainer::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get user profile. "
        "Caught UserInfoContainer::Exception: " <<
        ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get user profile. "
        "Caught eh::Exception: " <<
        ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  CORBA::Boolean
  UserInfoManagerImpl::merge(
    const AdServer::UserInfoSvcs::UserInfo& user_info,
    const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
    const AdServer::UserInfoSvcs::UserProfiles& merge_user_profile,
    CORBA::Boolean_out merge_success,
    CORBACommons::TimestampInfo_out last_request)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerImpl::merge()";

    try
    {
      merge_success = true;

      UserId uid = CorbaAlgs::unpack_user_id(user_info.user_id);
      UserId huid = CorbaAlgs::unpack_user_id(user_info.huser_id);

      bool household = uid.is_null();
      UserId user_id = household ? huid : uid;

      UserOperationProcessorAccessor user_operation_processor =
        get_user_operation_processor_();

      UserInfoContainer::RequestMatchParams
        request_params(
          user_id,
          Generics::Time(user_info.time),
          String::SubString(match_params.cohort.in()),
          String::SubString(match_params.cohort2.in()),
          match_params.use_empty_profile,
          user_info.current_colo_id,
          repeat_trigger_timeout_,
          match_params.filter_contextual_triggers,
          user_info.temporary,
          match_params.silent_match,
          false, // no match
          false, // no result
          false, // provide channel count
          false, // provide persistent channels
          match_params.change_last_request,
          household,
          0);

      UserInfoContainer::UserAppearance user_app;

      last_request = new CORBACommons::TimestampInfo;
      *last_request = CorbaAlgs::pack_time(Generics::Time::ZERO);

      /* merge users */
      try
      {
        long placement_colo_id =
          user_info.current_colo_id != -1 ? user_info.current_colo_id : placement_colo_id_;

        MemBuf merge_base_profile(
          merge_user_profile.base_user_profile.get_buffer(),
          merge_user_profile.base_user_profile.length());
        MemBuf merge_add_profile(
          merge_user_profile.add_user_profile.get_buffer(),
          merge_user_profile.add_user_profile.length());
        MemBuf merge_history_profile(
          merge_user_profile.history_user_profile.get_buffer(),
          merge_user_profile.history_user_profile.length());
        MemBuf merge_freq_cap_profile(
          merge_user_profile.freq_cap.get_buffer(),
          merge_user_profile.freq_cap.length());

        UserInfoManagerLogger::HistoryOptimizationInfo ho_info;

        user_operation_processor->merge(
          request_params,
          merge_base_profile.membuf(),
          merge_add_profile.membuf(),
          merge_history_profile.membuf(),
          merge_freq_cap_profile.membuf(),
          user_app,
          uie_presents_ ? user_info.last_colo_id : placement_colo_id,
          placement_colo_id,
          AdServer::ProfilingCommons::OP_RUNTIME,
          &ho_info);

        if (ho_info.isp_date != Generics::Time::ZERO && !household)
        {
          ho_info.colo_id = placement_colo_id_;
          user_info_manager_logger_->process_history_optimization(ho_info);
        }

        *last_request = CorbaAlgs::pack_time(user_app.last_request);

        if (!household && !huid.is_null())
        {
          UserInfoContainer::RequestMatchParams
            hid_request_params(
              huid,
              Generics::Time(user_info.time),
              String::SubString(match_params.cohort.in()),
              String::SubString(match_params.cohort2.in()),
              match_params.use_empty_profile,
              user_info.current_colo_id,
              repeat_trigger_timeout_,
              match_params.filter_contextual_triggers,
              user_info.temporary,
              match_params.silent_match,
              false, // no match
              false, // no result
              false, // provide channel count
              false, // provide persistent channels
              match_params.change_last_request,
              true, // household
              0);

          UserInfoContainer::UserAppearance hid_user_app;

          user_operation_processor->merge(
            hid_request_params,
            merge_base_profile.membuf(),
            merge_add_profile.membuf(),
            merge_history_profile.membuf(),
            merge_freq_cap_profile.membuf(),
            hid_user_app,
            placement_colo_id,
            placement_colo_id,
            AdServer::ProfilingCommons::OP_RUNTIME);
        }
      }
      catch (const UserInfoContainer::ChunkNotFound& ex)
      {
        throw;
      }
      catch(const eh::Exception& ex)
      {
        merge_success = false;

        logger_->sstream(Logging::Logger::ERROR,
          Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-50") << FUN <<
          ": Can't do merging. Caught eh::Exception: " << ex.what();
      }
    }
    catch(const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can not merge users. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::NotReady>(
          ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can not merge users. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ChunkNotFound>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can not merge user. "
        "Caught eh::Exception: " << ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }

    return merge_success;
  }

  CORBA::Boolean
  UserInfoManagerImpl::fraud_user(
    const CORBACommons::UserIdInfo& user_id_info,
    const CORBACommons::TimestampInfo& time)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerImpl::fraud_user()";

    try
    {
      UserOperationProcessorAccessor user_operation_processor =
        get_user_operation_processor_();

      user_operation_processor->fraud_user(
        CorbaAlgs::unpack_user_id(user_id_info),
        CorbaAlgs::unpack_time(time));
    }
    catch(const UserOperationProcessor::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't update user. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::NotReady>(
          ostr.str());
    }
    catch (const UserOperationProcessor::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't update user. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ChunkNotFound>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't update user. Caught eh::Exception: " <<
        ex.what();

      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }

    return true;
  }

  CORBA::Boolean
  UserInfoManagerImpl::match(
    const AdServer::UserInfoSvcs::UserInfo& user_info,
    const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
    AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady,
      AdServer::UserInfoSvcs::UserInfoManager::ImplementationException,
      AdServer::UserInfoSvcs::UserInfoManager::ChunkNotFound)*/
  {
    static const char* FUN = "UserInfoManagerImpl::match()";

    metrics_raii raii_tmp(composite_metrics_provider_, "UserInfoManagerImpl::match");
    try
    {
      Generics::Timer process_timer;
      process_timer.start();

      UserId uid = CorbaAlgs::unpack_user_id(user_info.user_id);
      UserId huid = CorbaAlgs::unpack_user_id(user_info.huser_id);

      bool household = uid.is_null();

      UserId user_id = household ? huid : uid;

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << "Match request: " << std::endl <<
          "  User id: " <<
          PrivacyFilter::filter(user_id.to_string().c_str(), "USER_ID") << std::endl <<
          "  current colo id: " << user_info.current_colo_id << std::endl <<
          "  Input search channels: ";
        CorbaAlgs::print_sequence_fields(
          ostr,
          match_params.search_channel_ids,
          &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_id,
          &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_trigger_id);
        ostr << std::endl << "  Input page channels: ";
        CorbaAlgs::print_sequence_fields(
          ostr,
          match_params.page_channel_ids,
          &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_id,
          &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_trigger_id);
        ostr << std::endl << "  Input url channels: ";
        CorbaAlgs::print_sequence_fields(
          ostr,
          match_params.url_channel_ids,
          &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_id,
          &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_trigger_id);
        ostr << std::endl << "  Input url keyword channels: ";
        CorbaAlgs::print_sequence_fields(
          ostr,
          match_params.url_keyword_channel_ids,
          &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_id,
          &AdServer::UserInfoSvcs::UserInfoMatcher::ChannelTriggerMatch::channel_trigger_id);

        ostr << std::endl << "  Input persistent channels: ";
        CorbaAlgs::print_sequence(ostr, match_params.persistent_channel_ids);
        ostr << std::endl;

        logger_->log(ostr.str(),
          Logging::Logger::TRACE,
          Aspect::USER_INFO_MANAGER);
      }

      UserOperationProcessorAccessor user_operation_processor =
        get_user_operation_processor_();
      UserInfoContainerAccessor user_info_container =
        get_user_info_container_();

      CoordData coord_data;
      if (match_params.geo_data_seq.length() != 0)
      {
        coord_data.defined = true;
        coord_data.latitude =
          CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::CoordDecimal>(
            match_params.geo_data_seq[0].latitude);
        coord_data.longitude =
          CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::CoordDecimal>(
            match_params.geo_data_seq[0].longitude);
        coord_data.accuracy =
          CorbaAlgs::unpack_decimal<AdServer::CampaignSvcs::AccuracyDecimal>(
            match_params.geo_data_seq[0].accuracy);
      }
      else
      {
        coord_data.defined = false;
      }

      UserInfoContainer::RequestMatchParams
        request_params(
          user_id,
          Generics::Time(user_info.time),
          String::SubString(match_params.cohort.in()),
          String::SubString(match_params.cohort2.in()),
          match_params.use_empty_profile,
          user_info.request_colo_id,
          repeat_trigger_timeout_,
          match_params.filter_contextual_triggers,
          user_info.temporary,
          match_params.silent_match,
          match_params.no_match,
          match_params.no_result,
          !household ? match_params.provide_channel_count : false,
          !household ? match_params.provide_persistent_channels : false,
          match_params.change_last_request,
          household,
          coord_data.defined ? &coord_data : 0);

      UserInfoContainer::UserAppearance user_app;

      AdServer::UserInfoSvcs::ChannelMatchMap result_channels;
      AdServer::UserInfoSvcs::ChannelMatchPack matched_channels;

      matched_channels.page_channels.reserve(match_params.page_channel_ids.length());
      for (CORBA::ULong i = 0; i < match_params.page_channel_ids.length(); ++i)
      {
        matched_channels.page_channels.push_back(
          ChannelMatch(
            match_params.page_channel_ids[i].channel_id,
            match_params.page_channel_ids[i].channel_trigger_id));
      }

      matched_channels.search_channels.reserve(match_params.search_channel_ids.length());
      for (CORBA::ULong i = 0; i < match_params.search_channel_ids.length(); ++i)
      {
        matched_channels.search_channels.push_back(
          ChannelMatch(
            match_params.search_channel_ids[i].channel_id,
            match_params.search_channel_ids[i].channel_trigger_id));
      }

      matched_channels.url_channels.reserve(match_params.url_channel_ids.length());
      for (CORBA::ULong i = 0; i < match_params.url_channel_ids.length(); ++i)
      {
        matched_channels.url_channels.push_back(
          ChannelMatch(
            match_params.url_channel_ids[i].channel_id,
            match_params.url_channel_ids[i].channel_trigger_id));
      }

      matched_channels.url_keyword_channels.reserve(
        match_params.url_keyword_channel_ids.length());
      for (CORBA::ULong i = 0; i < match_params.url_keyword_channel_ids.length(); ++i)
      {
        matched_channels.url_keyword_channels.push_back(
          ChannelMatch(
            match_params.url_keyword_channel_ids[i].channel_id,
            match_params.url_keyword_channel_ids[i].channel_trigger_id));
      }

      matched_channels.persistent_channels.reserve(match_params.persistent_channel_ids.length());
      for (CORBA::ULong i = 0; i < match_params.persistent_channel_ids.length(); ++i)
      {
        matched_channels.persistent_channels.push_back(match_params.persistent_channel_ids[i]);
      }

      ColoUserId colo_user_id;

      long placement_colo_id =
        user_info.current_colo_id != -1 ? user_info.current_colo_id : placement_colo_id_;

      ProfileProperties profile_properties;

      UserInfoManagerLogger::HistoryOptimizationInfo ho_info;
      UniqueChannelsResult unique_channels_result;

      user_operation_processor->match(
        request_params,
        uie_presents_ ? user_info.last_colo_id : placement_colo_id,
        placement_colo_id,
        colo_user_id,
        matched_channels,
        result_channels,
        user_app,
        profile_properties,
        AdServer::ProfilingCommons::OP_RUNTIME,
        &ho_info,
        &unique_channels_result);
      
      match_result = new AdServer::UserInfoSvcs::UserInfoManager::MatchResult();

      if (!household)
      {
        match_result->adv_channel_count =
          unique_channels_result.simple_channels;
        match_result->discover_channel_count =
          unique_channels_result.discover_channels;

        if (ho_info.isp_date != Generics::Time::ZERO)
        {
          ho_info.colo_id = placement_colo_id_;
          user_info_manager_logger_->process_history_optimization(ho_info);
        }

        match_result->colo_id = -1;
        match_result->times_inited = true;
        match_result->last_request_time = CorbaAlgs::pack_time(user_app.last_request);
        match_result->create_time = CorbaAlgs::pack_time(user_app.create_time);
        match_result->session_start = CorbaAlgs::pack_time(user_app.session_start);
        match_result->fraud_request = profile_properties.fraud_request;
        match_result->cohort = profile_properties.cohort.c_str();
        match_result->cohort2 = profile_properties.cohort2.c_str();

        CORBA::ULong i = 0;
        match_result->geo_data_seq.length(profile_properties.geo_data_list.size());
        for (GeoDataResultList::const_iterator it =
               profile_properties.geo_data_list.begin();
             it != profile_properties.geo_data_list.end(); ++it, ++i)
        {
          match_result->geo_data_seq[i].latitude =
            CorbaAlgs::pack_decimal<AdServer::CampaignSvcs::CoordDecimal>(
              it->latitude);
          match_result->geo_data_seq[i].longitude =
            CorbaAlgs::pack_decimal<AdServer::CampaignSvcs::CoordDecimal>(
              it->longitude);
          match_result->geo_data_seq[i].accuracy =
            CorbaAlgs::pack_decimal<AdServer::CampaignSvcs::AccuracyDecimal>(
              it->accuracy);
        }
        
        if (uie_presents_)
        {
          if (user_info.last_colo_id != placement_colo_id)
          {
            match_result->colo_id = placement_colo_id;
          }

          if (colo_user_id.need_profile)
          {
            SyncPolicy::WriteGuard guard(colo_lock_);

            long sz = colo_user_id.colo_id;

            if (sz >= 0)
            {
              if (colo_profiles_vector_.size() <= (unsigned long)sz)
              {
                colo_profiles_vector_.resize(sz + 1);
              }

              colo_profiles_vector_[sz].user_id.push_back(colo_user_id.user_id);
            }
          }
        }

        Generics::Time publisher_optin_timeout(
          CorbaAlgs::unpack_time(match_params.publishers_optin_timeout));
        if (publisher_optin_timeout != Generics::Time::ZERO)
        {
          std::list<unsigned long> publishers;

          user_info_container->get_optin_publishers(
            CorbaAlgs::unpack_user_id(user_info.user_id),
            publisher_optin_timeout,
            publishers);

          CorbaAlgs::fill_sequence(
            publishers.begin(),
            publishers.end(),
            match_result->exclude_pubpixel_accounts);
        }

        if(match_params.ret_freq_caps)
        {
          UserFreqCapProfile::FreqCapIdList freq_caps;
          UserFreqCapProfile::FreqCapIdList virtual_freq_caps;
          UserFreqCapProfile::SeqOrderList seq_orders;
          UserFreqCapProfile::CampaignFreqs campaign_freqs;

          try
          {
            user_info_container->get_full_freq_caps(
              CorbaAlgs::unpack_user_id(user_info.user_id),
              Generics::Time(user_info.time),
              freq_caps,
              virtual_freq_caps,
              seq_orders,
              campaign_freqs);

            CorbaAlgs::fill_sequence(
              freq_caps.begin(),
              freq_caps.end(),
              match_result->full_freq_caps);

            CorbaAlgs::fill_sequence(
              virtual_freq_caps.begin(),
              virtual_freq_caps.end(),
              match_result->full_virtual_freq_caps);

            match_result->seq_orders.length(seq_orders.size());
            CORBA::ULong seq_order_i = 0;

            for(UserFreqCapProfile::SeqOrderList::const_iterator it =
                  seq_orders.begin();
                it != seq_orders.end(); ++it, ++seq_order_i)
            {
              match_result->seq_orders[seq_order_i].ccg_id = it->ccg_id;
              match_result->seq_orders[seq_order_i].set_id = it->set_id;
              match_result->seq_orders[seq_order_i].imps = it->imps;
            }

            match_result->campaign_freqs.length(campaign_freqs.size());
            CORBA::ULong campaign_freq_i = 0;

            for(auto it = campaign_freqs.begin();
                it != campaign_freqs.end(); ++it, ++campaign_freq_i)
            {
              match_result->campaign_freqs[campaign_freq_i].campaign_id = it->campaign_id;
              match_result->campaign_freqs[campaign_freq_i].imps = it->imps;
            }
          }
          catch(const UserInfoContainer::UserIsFraud& ex)
          {
            Stream::Error ostr;
            ostr << "User '" << uid << "' is fraud: " << ex.what();
            logger()->log(
              ostr.str(),
              Logging::Logger::INFO,
              Aspect::USER_INFO_MANAGER,
              "ADS-IMPL-0000");
            match_result->fraud_request = true;
          }
        }

        /* generate output params */
        match_result->channels.length(result_channels.size());

        unsigned long j = 0;
        for (ChannelMatchMap::const_iterator it = result_channels.begin();
             it != result_channels.end(); ++it, ++j)
        {
          match_result->channels[j].channel_id = it->first;
          match_result->channels[j].weight = it->second;
        }

        if(!request_params.silent_match)
        {
          calc_user_daily_stat_(
            Generics::Time::get_time_of_day(),
            request_params.current_time,
            user_app.last_request,
            user_info_container->time_offset());
        }
      }
      else
      {
        /* generate output params if household */
        match_result->hid_channels.length(result_channels.size());

        unsigned long j = 0;
        for (ChannelMatchMap::const_iterator it = result_channels.begin();
             it != result_channels.end(); ++it, ++j)
        {
          match_result->hid_channels[j].channel_id = it->first;
          match_result->hid_channels[j].weight = it->second;
        }
      }

      if (!household && !huid.is_null())
      {
        UserInfoContainer::RequestMatchParams
          hid_request_params(
            huid,
            Generics::Time(user_info.time),
            String::SubString(match_params.cohort.in()),
            String::SubString(match_params.cohort2.in()),
            match_params.use_empty_profile,
            user_info.request_colo_id,
            repeat_trigger_timeout_,
            match_params.filter_contextual_triggers,
            false,                           // temporary,
            match_params.silent_match,
            match_params.no_match,
            match_params.no_result,
            false,
            false,
            match_params.change_last_request,
            true,                           // household
            coord_data.defined ? &coord_data : 0);

        AdServer::UserInfoSvcs::ChannelMatchMap hid_result_channels;

        user_operation_processor->match(
          hid_request_params,
          placement_colo_id,
          placement_colo_id,
          colo_user_id,
          matched_channels,
          hid_result_channels,
          user_app,
          profile_properties,
          AdServer::ProfilingCommons::OP_RUNTIME,
          0, // history optimization info
          0 // unique channels result
          );

        match_result->hid_channels.length(hid_result_channels.size());

        unsigned long j = 0;
        for (ChannelMatchMap::const_iterator it = hid_result_channels.begin();
             it != hid_result_channels.end(); ++it, ++j)
        {
          match_result->hid_channels[j].channel_id = it->first;
          match_result->hid_channels[j].weight = it->second;
        }
      }

      process_timer.stop();
      match_result->process_time = CorbaAlgs::pack_time(process_timer.elapsed_time());

      return true;
    }
    catch(const UserOperationProcessor::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match user. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::NotReady>(
          ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match user. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ChunkNotFound>(
          ostr.str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match user. "
        "Caught eh::Exception: " << ex.what();
      CORBACommons::throw_desc<
        UserInfoSvcs::UserInfoManager::ImplementationException>(
          ostr.str());
    }
    return 0; // never reach
  }

  void
  UserInfoManagerImpl::get_controllable_chunks(
    UserInfoManagerImpl::ChunkIdList& chunk_ids,
    unsigned long& common_chunks_number)
    /*throw(Exception)*/
  {
    for(AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap::
          const_iterator chunk_it = chunk_folders_.begin();
        chunk_it != chunk_folders_.end(); ++chunk_it)
    {
      chunk_ids.push_back(chunk_it->first);
    }

    common_chunks_number =
      user_info_manager_config_.Storage().common_chunks_number();
  }

  void
  UserInfoManagerImpl::clear_expired(
    CORBA::Boolean synch,
    const CORBACommons::TimestampInfo& cleanup_time,
    CORBA::Long portion)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::ImplementationException)*/
  {
    static const char* FUN = "UserInfoManagerImpl::clear_expired()";

    if(!synch)
    {
      try
      {
        Task_var delete_old_profiles_msg =
          new DeleteOldProfilesTask(0, this, false);
        task_runner_->enqueue_task(delete_old_profiles_msg);

        Task_var delete_old_temp_profiles_msg =
          new DeleteOldTemporaryProfilesTask(0, this, false);
        task_runner_->enqueue_task(delete_old_temp_profiles_msg);

        if(user_info_manager_config_.UserProfilesCleanup().present())
        {
          Task_var all_users_processing_msg =
            new AllUsersProcessingTask(
              0, this, false, 0, CorbaAlgs::unpack_time(cleanup_time), portion);
          task_runner_->enqueue_task(all_users_processing_msg);
        }

        if(logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->sstream(Logging::Logger::TRACE,
            Aspect::USER_INFO_MANAGER) <<
            "tasks for expired profiles clearing was enqueued forcibly.";
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't delete old user profiles. Caught eh::Exception: " <<
          ex.what();
        CORBACommons::throw_desc<
          UserInfoSvcs::UserInfoManager::ImplementationException>(
            ostr.str());
      }
    }
    else
    {
      delete_old_profiles_(false);
      delete_old_temporary_profiles_(false);
      all_users_process_step_(
        false, 0, true, CorbaAlgs::unpack_time(cleanup_time), portion);
    }
  }

  void
  UserInfoManagerImpl::all_users_process_step_(
    bool reschedule,
    UserInfoContainer::AllUsersProcessingState* processing_state,
    bool fast_mode,
    const Generics::Time& cleanup_time,
    long portion)
    noexcept
  {
    static const char* FUN = "UserInfoManagerImpl::all_users_process_step_()";

    UserInfoContainerAccessor user_info_container = get_user_info_container_(false);

    Generics::Time next_ex_time;
    UserInfoContainer::AllUsersProcessingState_var use_processing_state;

    if(user_info_container.get().in())
    {
      try
      {
        use_processing_state =
          ReferenceCounting::add_ref(processing_state);

        if(!use_processing_state.in())
        {
          Generics::Time content_cleanup_time;
          unsigned long process_portion;

          if (!user_info_manager_config_.UserProfilesCleanup().present())
          {
            content_cleanup_time =
              (cleanup_time != Generics::Time(-1)) ?
              cleanup_time : Generics::Time::ZERO;

            process_portion = static_cast<unsigned long>(portion);
          }
          else
          {
            content_cleanup_time =
              (cleanup_time == Generics::Time(-1)) ?
              Generics::Time(user_info_manager_config_.UserProfilesCleanup()->
                             content_cleanup_time()) :
              cleanup_time;

            process_portion = (portion == -1) ?
              user_info_manager_config_.UserProfilesCleanup()->process_portion() :
              static_cast<unsigned long>(portion);
          }

          use_processing_state = user_info_container->start_all_users_processing(
            content_cleanup_time,
            fast_mode ? -1 : process_portion,
            provide_channel_counters_);
        }

        if(fast_mode)
        {
          do
          {
            next_ex_time =
              user_info_container->continue_all_users_processing(
                *use_processing_state,
                this);
          }
          while(next_ex_time != Generics::Time::ZERO);
        }
        else
        {
          next_ex_time =
            user_info_container->continue_all_users_processing(
              *use_processing_state,
              this);
        }
      }
      catch(const eh::Exception& e)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-81") <<
          FUN << ": Can't make daily processing: eh::Exception: " << e.what();
      }
    }
    else
    {
      next_ex_time = Generics::Time::get_time_of_day() + 10;
    }

    try
    {
      if(next_ex_time != Generics::Time::ZERO)
      {
        Task_var msg = new AllUsersProcessingTask(
          task_runner_,
          this,
          reschedule,
          use_processing_state);
        scheduler_->schedule(msg, next_ex_time);
      }
      else
      {
        // TO DO: publish channel counters
        if(reschedule)
        {
          assert(user_info_manager_config_.UserProfilesCleanup().present());

          // schedule for next day
          Generics::Time now = Generics::Time::get_time_of_day();
          Generics::Time next_tm = now.get_gm_time().get_date() + Generics::Time(
            user_info_manager_config_.UserProfilesCleanup()->start_time(),
            "%H:%M");

          if(next_tm < now)
          {
            next_tm += Generics::Time::ONE_DAY;
          }

          Task_var msg = new AllUsersProcessingTask(
            task_runner_, this, reschedule, 0);
          scheduler_->schedule(msg, next_tm);

          if(logger_->log_level() >= Logging::Logger::INFO)
          {
            Stream::Error ostr;
            ostr <<
              FUN << ": Cleanup profiles content task scheduled for '" <<
              next_tm.get_gm_time().format("%F %T") <<
              "'";

            if(use_processing_state)
            {
              ostr << " start time of prev task '" <<
                use_processing_state->start_time.get_gm_time().format("%F %T") << "'";
            }

            logger()->log(
              ostr.str(),
              Logging::Logger::INFO,
              Aspect::USER_INFO_MANAGER);
          }
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-52") <<
        FUN << ": Can't schedule task. Caught eh::Exception: " << ex.what();
    }
  }

  UserInfoManagerImpl::UserInfoContainerAccessor
  UserInfoManagerImpl::get_user_info_container_(
    bool throw_not_ready)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady)*/
  {
    if(throw_not_ready)
    {
      SyncPolicy::ReadGuard lock(lock_);
      if(!campaignserver_ready_)
      {
        AdServer::UserInfoSvcs::UserInfoManager::NotReady exc;
        exc.description = "UserInfoContainer is not ready";
        throw exc;
      }
    }
    
    UserInfoContainerAccessor accessor =
      user_info_container_->get_accessor();

    if(throw_not_ready && !accessor.get().in())
    {
      AdServer::UserInfoSvcs::UserInfoManager::NotReady exc;
      exc.description = "UserInfoContainer is not ready";
      throw exc;
    }

    return accessor;
  }

  UserInfoManagerImpl::UserOperationProcessorAccessor
  UserInfoManagerImpl::get_user_operation_processor_(
    bool throw_not_ready)
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady)*/
  {
    UserOperationProcessorAccessor accessor =
      user_operation_processor_->get_accessor();

    if(throw_not_ready && !accessor.get().in())
    {
      AdServer::UserInfoSvcs::UserInfoManager::NotReady exc;
      exc.description = "UserInfoProcessor is not ready";
      throw exc;
    }

    return accessor;
  }

  void UserInfoManagerImpl::flush_logs_() noexcept
  {
    static const char* FUN = "UserInfoManagerImpl::flush_logs()";

    const Generics::Time LOGS_DUMP_ERROR_RESCHEDULE_PERIOD(2);

    Generics::Time next_flush;

    try
    {
      next_flush = user_info_manager_logger_->flush_if_required(
        Generics::Time::get_time_of_day());
    }
    catch (const eh::Exception& ex)
    {
      next_flush = Generics::Time::get_time_of_day() +
        LOGS_DUMP_ERROR_RESCHEDULE_PERIOD;

      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught on flush logs:" << ex.what();

      logger_->log(ostr.str(),
        Logging::Logger::ERROR,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-77");
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": logs flushed, next flush at " <<
        next_flush.get_gm_time();

      logger_->log(
        ostr.str(),
        Logging::Logger::TRACE,
        Aspect::USER_INFO_MANAGER);
    }

    if(next_flush != Generics::Time::ZERO)
    {
      try
      {
        Task_var msg = new FlushLogsTask(this, task_runner_);
        scheduler_->schedule(msg, next_flush);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't schedule next flush task. "
          "eh::Exception caught:" << ex.what();

        logger_->log(
          ostr.str(),
          Logging::Logger::ERROR,
          Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-78");
      }

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->sstream(
          Logging::Logger::TRACE,
          Aspect::USER_INFO_MANAGER) << FUN <<
          ": flush USER_INFO_MANAGER logger for " <<
          next_flush.get_gm_time();
      }
    }
  }

  void
  UserInfoManagerImpl::delete_old_profiles_(bool reschedule) noexcept
  {
    static const char* FUN = "UserInfoManagerImpl::delete_old_profiles_()";

    logger()->log(
      String::SubString("Cleanup old profiles task started"),
      Logging::Logger::INFO,
      Aspect::DELETE_OLD_PROFILES);

    try
    {
      UserInfoContainerAccessor uic = get_user_info_container_(false);

      if(uic.get().in())
      {
        uic->delete_old_profiles(profile_lifetime_);
      }
      else
      {
        logger_->log(
          String::SubString(
            "Can't do profiles cleaning - UserInfoManager isn't ready."),
          Logging::Logger::TRACE,
          Aspect::USER_INFO_MANAGER);
      }
    }
    catch(const UserInfoContainer::NotReady& ex)
    {
      logger_->sstream(Logging::Logger::NOTICE,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-51") << FUN <<
        ": Can't delete old user profiles. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::NOTICE,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-51") << FUN <<
        ": Can't delete old user profiles. Caught eh::Exception: " <<
        ex.what();
    }

    if(reschedule)
    {
      try
      {
        Generics::Time now = Generics::Time::get_time_of_day();

        Task_var msg = new DeleteOldProfilesTask(
          task_runner_, this, reschedule);
        scheduler_->schedule(
          msg,
          now + std::min(
            std::max(profile_lifetime_ / 10, Generics::Time(1)),
            Generics::Time::ONE_DAY / 4));
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-52") <<
          FUN << ": Can't schedule task. Caught eh::Exception: " << ex.what();
      }
    }
  }

  void
  UserInfoManagerImpl::delete_old_temporary_profiles_(bool reschedule) noexcept
  {
    static const char* FUN = "UserInfoManagerImpl::delete_old_temporary_profiles_()";

    try
    {
      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->stream(Logging::Logger::TRACE,
          Aspect::USER_INFO_MANAGER) <<
          FUN << ": cleanup old temporary profiles task started.";
      }

      UserInfoContainerAccessor user_info_container = get_user_info_container_(false);

      if(user_info_container.get().in())
      {
        user_info_container->delete_old_temporary_profiles(temp_profile_lifetime_);
      }
      else
      {
        logger_->log(
          String::SubString(
            "Can't do profiles cleaning - UserInfoManager isn't ready."),
          Logging::Logger::TRACE,
          Aspect::USER_INFO_MANAGER);
      }
    }
    catch(const UserInfoContainer::NotReady& ex)
    {
      logger_->sstream(Logging::Logger::NOTICE,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-51") << FUN <<
        ": Can't delete old user temporary profiles. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::NOTICE,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-51") << FUN <<
        ": Can't delete old user temporary profiles. Caught eh::Exception: " <<
        ex.what();
    }

    if (logger_->log_level() >= Logging::Logger::INFO)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::USER_INFO_MANAGER) << FUN <<
        ": cleanup old temporary profiles task has finished.";
    }

    if(reschedule)
    {
      try
      {
        Generics::Time now = Generics::Time::get_time_of_day();

        Task_var msg =
          new DeleteOldTemporaryProfilesTask(task_runner_, this, reschedule);
        scheduler_->schedule(msg,
          now + std::max(temp_profile_lifetime_ / 10, Generics::Time(1)));
      }
      catch(const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY,
          Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-52") <<
          FUN << ": Can't schedule task. Caught eh::Exception: " << ex.what();
      }
    }
  }

  void
  UserInfoManagerImpl::load_chunk_files_() noexcept
  {
    static const char* FUN = "UserInfoManagerImpl::load_chunk_files_()";

    try
    {
      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->stream(Logging::Logger::TRACE,
          Aspect::USER_INFO_MANAGER) << "LoadChunksDataTask started.";
      }

      xsd::AdServer::Configuration::UserInfoManagerStorageType&
        storage_config = user_info_manager_config_.Storage();

      UserInfoContainer_var user_info_container =
        new UserInfoContainer(
          logger_,
          storage_config.common_chunks_number(),
          chunk_folders_,
          storage_config.is_level_enable(),
          fill_level_map_traits_(storage_config.AddChunksConfig()),
          fill_level_map_traits_(storage_config.TempChunksConfig()),
          fill_level_map_traits_(storage_config.HistoryChunksConfig()),
          fill_level_map_traits_(storage_config.BaseChunksConfig()),
          fill_level_map_traits_(storage_config.FreqCapChunksConfig()),
          storage_config.is_rocksdb_enable(),
          fill_rocksdb_map_params_(storage_config.AddChunksRocksDBConfig()),
          fill_rocksdb_map_params_(storage_config.TempChunksRocksDBConfig()),
          fill_rocksdb_map_params_(storage_config.HistoryChunksRocksDBConfig()),
          fill_rocksdb_map_params_(storage_config.BaseChunksRocksDBConfig()),
          fill_rocksdb_map_params_(storage_config.FreqCapChunksRocksDBConfig()),
          user_info_manager_config_.colo_id(),
          user_info_manager_config_.UserInfoExchangerParameters().present() ?
            Generics::Time(user_info_manager_config_.
              UserInfoExchangerParameters()->colo_request_timeout()) :
            Generics::Time::ZERO,
          Generics::Time(user_info_manager_config_.history_optimization_period()),
          provide_channel_counters_,
          Generics::Time(user_info_manager_config_.session_timeout()),
          user_info_manager_config_.max_base_profile_waiters(),
          user_info_manager_config_.max_temp_profile_waiters(),
          user_info_manager_config_.max_freqcap_profile_waiters(),
          loading_progress_processor_);

      user_info_container->activate_object();

      UserOperationProcessor_var user_operation_processor;

      if(user_info_manager_config_.UserOperationsBackup().present())
      {
        user_operation_saver_ = new UserOperationSaver(
          callback_,
          logger(),
          user_info_manager_config_.UserOperationsBackup()->dir().c_str(),
          user_info_manager_config_.UserOperationsBackup()->file_prefix().c_str(),
          storage_config.common_chunks_number(),
          file_controller_,
          user_info_container);

        user_info_container_dependent_active_object_->add_child_object(
          user_operation_saver_);

        user_operation_processor = user_operation_saver_;

        Task_var msg = new RotateUserOperationsBackupTask(this, 0);
        task_runner_->enqueue_task(msg);
      }
      else
      {
        user_operation_processor = user_info_container;
      }

      user_info_container_dependent_active_object_->activate_object();

      {
        *user_info_container_ = user_info_container;
        *user_operation_processor_ = user_operation_processor;
      }

      logger_->log(
        String::SubString("Chunk maps loaded."),
        Logging::Logger::TRACE,
        Aspect::USER_INFO_MANAGER);
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-53") <<
        FUN << ": Can't load chunk files. Caught eh::Exception: " <<
        ex.what();
    }

    if (!user_info_container_.in())
    {
      try
      {
        Generics::Time tm =
          Generics::Time::get_time_of_day() + CHUNKS_RELOAD_PERIOD;

        Task_var msg = new LoadChunksDataTask(this, task_runner_);
        scheduler_->schedule(msg, tm);
      }
      catch(const eh::Exception& ex)
      {
        std::ostringstream ostr;
        ostr << FUN << ": Can't schedule chunks reloading task. "
          "Caught eh::Exception: " << ex.what();

        logger_->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-79");
      }
    }
    else
    {
      try
      {
        Task_var update_channels_msg = new UpdateChannelsConfigTask(this, 0);
        task_runner_->enqueue_task(update_channels_msg);
      }
      catch(const eh::Exception& ex)
      {
        logger_->stream(Logging::Logger::EMERGENCY,
          Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-52") << FUN <<
          ": Can't schedule channels updating task. "
          "Caught eh::Exception: " << ex.what();
      }
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::USER_INFO_MANAGER) <<
        "LoadChunksDataTask has finished.";
    }
  }

  void
  UserInfoManagerImpl::get_last_colo_profiles_()
    noexcept
  {
    static const char* FUN = "UserInfoManagerImpl::get_last_colo_profiles_()";

    try
    {
      UserOperationProcessorAccessor user_operation_processor =
        get_user_operation_processor_();
      UserInfoContainerAccessor user_info_container =
        get_user_info_container_(false);

      if(!user_info_container.get().in() || !user_operation_processor.get().in())
      {
        return;
      }

      if(logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->stream(Logging::Logger::TRACE,
          Aspect::USER_INFO_MANAGER) <<
          "GetLastColoProfilesTask started.";
      }

      ColoUsersRequestSeq users_request;

      {
        SyncPolicy::WriteGuard guard(colo_lock_);

        users_request.length(colo_profiles_vector_.size());
        CORBA::ULong user_i = 0;

        for (unsigned int i = 0; i < colo_profiles_vector_.size(); ++i)
        {
          if (!colo_profiles_vector_[i].user_id.empty())
          {
            users_request[user_i].colo_id = i;
            UserIdSeq& users_req = users_request[user_i].users;

            CorbaAlgs::fill_sequence(
              colo_profiles_vector_[i].user_id.begin(),
              colo_profiles_vector_[i].user_id.end(),
              users_req);

            ++user_i;
          }
        }

        users_request.length(user_i);
        colo_profiles_vector_.clear();
      }

      if (users_request.length() != 0)
      {
        user_info_exchanger_->register_users_request(
          exchange_customer_id_.c_str(),
          users_request);
      }

      UserProfileSeq_var user_profiles_out;

      user_info_exchanger_->receive_users(
        exchange_customer_id_.c_str(),
        user_profiles_out,
        opt_uie_receive_criteria_);

      const UserProfileSeq& profiles = user_profiles_out.in();

      for (CORBA::ULong i = 0; i < profiles.length(); ++i)
      {
        MemBuf base_profile_mb(
          profiles[i].plain_profile.get_buffer(),
          profiles[i].plain_profile.length());
        MemBuf history_profile_mb(
          profiles[i].plain_history_profile.get_buffer(),
          profiles[i].plain_history_profile.length());

        UserInfoManagerLogger::HistoryOptimizationInfo ho_info;

        user_operation_processor->exchange_merge(
          UserId(profiles[i].user_id),
          base_profile_mb.membuf(),
          history_profile_mb.membuf(),
          &ho_info);

        if (ho_info.isp_date != Generics::Time::ZERO)
        {
          ho_info.colo_id = placement_colo_id_;
          user_info_manager_logger_->process_history_optimization(ho_info);
        }
      }

      UserIdSeq_var users_out;

      user_info_exchanger_->get_users_requests(
        exchange_provider_id_.c_str(),
        users_out);

      const UserIdSeq& ids = users_out.in();
      UserProfileSeq user_profiles;

      for(unsigned long i = 0; i < ids.length(); ++i)
      {
        const char* user_id = ids[i];

        if(user_info_container->dispose_user(UserId(user_id)))
        {
          CORBA::ULong len = user_profiles.length();
          user_profiles.length(len + 1);
          AdServer::UserInfoSvcs::UserProfile& result_user = user_profiles[len];

          SmartMemBuf_var mb_base_profile;
          SmartMemBuf_var mb_history_profile;

          user_info_container->get_user_profile(
            UserId(user_id),
            false,
            &mb_base_profile,
            0,
            &mb_history_profile);

          result_user.user_id = user_id;
          result_user.colo_id = user_info_manager_config_.colo_id(); // provider id

          if (mb_base_profile.in())
          {
            convert_mem_buf(
              mb_base_profile->membuf(),
              result_user.plain_profile);
          }

          if (mb_history_profile.in())
          {
            convert_mem_buf(
              mb_history_profile->membuf(),
              result_user.plain_history_profile);
          }
        }
      }

      if(user_profiles.length())
      {
        user_info_exchanger_->send_users(
          exchange_provider_id_.c_str(),
          user_profiles);
      }
    }
    catch(const AdServer::UserInfoSvcs::UserInfoManager::NotReady&)
    {}
    catch(const UserInfoExchanger::ImplementationException& exc)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-54") <<
        FUN << ": Caught AdServer::UserInfoSvcs::UserInfoExchanger::"
        "ImplementationException: " << exc.description;
    }
    catch(const UserInfoContainer::NotReady& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-54") <<
        FUN << ": Caught AdServer::UserInfoSvcs::UserInfoExchanger::"
        "ImplementationException: " << ex.what();
    }
    catch(const UserInfoContainer::ChunkNotFound& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-54") <<
        FUN << ": Caught AdServer::UserInfoSvcs::UserInfoExchanger::"
        "ChunkNotFound: " << ex.what();
    }
    catch(const CORBA::SystemException& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-54") <<
        FUN << ": Caught CORBA::SystemException: " << ex;
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-54") <<
        FUN << ": Caught eh::Exception: " << ex.what();
    }

    try
    {
      Generics::Time tm = Generics::Time::get_time_of_day() +
        user_info_manager_config_.UserInfoExchangerParameters()->
          set_get_profiles_period();

      Task_var msg =
        new GetLastColoProfilesTask(this, task_runner_);
      scheduler_->schedule(msg, tm);
    }
    catch(const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-52") <<
        FUN << ": Can't schedule task. Caught eh::Exception: " << ex.what();
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::USER_INFO_MANAGER) <<
        "GetLastColoProfilesTask has finished.";
    }
  }

  void
  UserInfoManagerImpl::update_channels_config_(
    UserInfoContainer* user_info_container,
    UserOperationProcessor* user_operation_processor)
    noexcept
  {
    static const char* FUN = "UserInfoManagerImpl::update_channels_config_()";
    const unsigned long PORTIONS_NUMBER = 20;

    try
    {
      for (;;)
      {
        CampaignServerPool::ObjectHandlerType campaign_server =
          campaign_servers_->get_object<CampaignServerPool::Exception>(
          logger_,
          Logging::Logger::EMERGENCY,
          Aspect::USER_INFO_MANAGER,
          "ADS_ICON-10",
          user_info_manager_config_.service_index(),
          user_info_manager_config_.service_index());

        try
        {
          Generics::Time time_offset;
          Generics::Time master_stamp;

          ChannelDictionary_var channels_config = new ChannelDictionary();
          FreqCapConfig_var freq_cap_config = new FreqCapConfig();

          {
            typedef std::map<BehavIdTypeKey, ChannelIntervalsPack_var>
              BehavIdChannelIntervalMap;
            BehavIdChannelIntervalMap behav_channel_interval_map;

            typedef std::map<StrBehavIdTypeKey, ChannelIntervalsPack_var>
              StrBehavIdChannelIntervalMap;
            StrBehavIdChannelIntervalMap str_behav_channel_interval_map;

            SimpleChannelPropertiesList channel_properties_list;

            CampaignSvcs::CampaignServer::GetSimpleChannelsInfo settings;
            settings.portions_number = PORTIONS_NUMBER;
            settings.channel_statuses = "AW";

            unsigned long res_length = 0;

            for (unsigned long portion = 0; portion < PORTIONS_NUMBER; ++portion)
            {
              settings.portion = portion;
              AdServer::CampaignSvcs::BriefSimpleChannelAnswer_var channels_to_load =
                campaign_server->brief_simple_channels(settings);

              res_length += channels_to_load->simple_channels.length();

              if (portion == 0)
              {
                master_stamp =
                  CorbaAlgs::unpack_time(channels_to_load->master_stamp);
                time_offset =
                  CorbaAlgs::unpack_time(channels_to_load->timezone_offset);
              }

              for (CORBA::ULong i = 0; i < channels_to_load->simple_channels.length(); ++i)
              {
                const CampaignSvcs::BriefSimpleChannelKey& channel =
                  channels_to_load->simple_channels[i];

                channel_properties_list.push_back(
                  SimpleChannelProperties(
                    channel.channel_id,
                    channel.discover,
                    channel.behav_param_list_id,
                    channel.str_behav_param_list_id));

                CorbaAlgs::convert_sequence(
                  channel.categories,
                  channels_config->channel_categories[channel.channel_id]);

                channels_config->channel_features.insert(
                  std::make_pair(channel.channel_id,
                                 ChannelFeatures(
                                   channel.discover,
                                   channel.threshold)));
              }

              for (CORBA::ULong i = 0; i < channels_to_load->behav_params.length(); ++i)
              {
                const CampaignSvcs::BriefBehavParamInfo& bpi =
                  channels_to_load->behav_params[i];

                if (bpi.bp_seq.length() != 0)
                {
                  ChannelIntervalsPack_var page_cip = new ChannelIntervalsPack();
                  ChannelIntervalsPack_var search_cip = new ChannelIntervalsPack();
                  ChannelIntervalsPack_var url_cip = new ChannelIntervalsPack();
                  ChannelIntervalsPack_var url_keyword_cip = new ChannelIntervalsPack();
                  ChannelIntervalsPack_var audience_cip = new ChannelIntervalsPack();

                  for (CORBA::ULong j = 0; j < bpi.bp_seq.length(); ++j)
                  {
                    const CampaignSvcs::BehavParameter& bp = bpi.bp_seq[j];

                    ChannelIntervalsPack_var target_cip;
                    bool unknown_trigger_type = false;            // Temporary fix

                    switch (bp.trigger_type)
                    {
                    case PAGE_CHANNEL: target_cip = page_cip; break;
                    case SEARCH_CHANNEL: target_cip = search_cip; break;
                    case URL_CHANNEL: target_cip = url_cip; break;
                    case URL_KEYWORD_CHANNEL : target_cip = url_keyword_cip; break;
                    case AUDIENCE_CHANNEL : target_cip = audience_cip; break;
                    default: unknown_trigger_type = true; break;
                    }

                    if (!unknown_trigger_type)
                    {
                      if (bp.trigger_type == AUDIENCE_CHANNEL)
                      {
                        if (bp.time_from != 0 ||
                            bp.time_to == 0 ||
                            bp.min_visits != 1)
                        {
                          logger_->stream(Logging::Logger::WARNING,
                                          Aspect::USER_INFO_MANAGER,
                                          "ADS-IMPL-75") << FUN <<
                            ": Audience channel with BEHAV_PARAMS_LIST_ID = " <<
                            bpi.id << " is incorrect.";
                        }
                        else
                        {
                          ChannelInterval ci(
                          Generics::Time(bp.time_from),
                          Generics::Time(bp.time_to),
                          bp.min_visits,
                          bp.weight);

                          push_channel_interval_(target_cip, ci);
                        }
                      }
                      else if (bp.time_from == 0 && bp.time_to == 0)
                      {
                        if (bp.min_visits == 1)
                        {
                          target_cip->contextual = true;
                          target_cip->zero_channel = true;
                          target_cip->weight += bp.weight;
                        }
                        else
                        {
                          logger_->stream(Logging::Logger::WARNING,
                                          Aspect::USER_INFO_MANAGER,
                                          "ADS-IMPL-75") << FUN <<
                            ": Channel with BEHAV_PARAMS_LIST_ID = " <<
                            bpi.id << " is incorrect.";
                        }
                      }
                      else if (bp.time_from < bp.time_to)
                      {
                        ChannelInterval ci(
                          Generics::Time(bp.time_from),
                          Generics::Time(bp.time_to),
                          bp.min_visits,
                          bp.weight);

                        if (bp.time_from == 0 && bp.min_visits == 1)
                        {
                          target_cip->contextual = true;
                        }

                        push_channel_interval_(target_cip, ci);
                      }
                      else
                      {
                        logger_->stream(Logging::Logger::WARNING,
                                        Aspect::USER_INFO_MANAGER,
                                        "ADS-IMPL-74") << FUN <<
                          ": Channel with BEHAV_PARAMS_LIST_ID = " <<
                          bpi.id << " is incorrect.";
                      }
                    }
                  }

                  behav_channel_interval_map[BehavIdTypeKey(bpi.id, PAGE_CHANNEL)] =
                    page_cip;
                  behav_channel_interval_map[BehavIdTypeKey(bpi.id, URL_CHANNEL)] =
                    url_cip;
                  behav_channel_interval_map[BehavIdTypeKey(bpi.id, URL_KEYWORD_CHANNEL)] =
                    url_keyword_cip;
                  behav_channel_interval_map[BehavIdTypeKey(bpi.id, SEARCH_CHANNEL)] =
                    search_cip;
                  behav_channel_interval_map[BehavIdTypeKey(bpi.id, AUDIENCE_CHANNEL)] =
                    audience_cip;
                }
              }

              for (CORBA::ULong i = 0; i < channels_to_load->key_behav_params.length(); ++i)
              {
                const CampaignSvcs::BriefKeyBehavParamInfo& kbpi =
                  channels_to_load->key_behav_params[i];

                if (kbpi.bp_seq.length() != 0)
                {
                  ChannelIntervalsPack_var page_cip = new ChannelIntervalsPack();
                  ChannelIntervalsPack_var search_cip = new ChannelIntervalsPack();
                  ChannelIntervalsPack_var url_cip = new ChannelIntervalsPack();
                  ChannelIntervalsPack_var url_keyword_cip = new ChannelIntervalsPack();
                  ChannelIntervalsPack_var audience_cip = new ChannelIntervalsPack();

                  for (CORBA::ULong j = 0; j < kbpi.bp_seq.length(); ++j)
                  {
                    const CampaignSvcs::BehavParameter& bp = kbpi.bp_seq[j];

                    ChannelIntervalsPack_var target_cip;
                    bool unknown_trigger_type = false;            // Temporary fix

                    switch (bp.trigger_type)
                    {
                    case PAGE_CHANNEL: target_cip = page_cip; break;
                    case SEARCH_CHANNEL: target_cip = search_cip; break;
                    case URL_CHANNEL: target_cip = url_cip; break;
                    case URL_KEYWORD_CHANNEL : target_cip = url_keyword_cip; break;
                    case AUDIENCE_CHANNEL : target_cip = audience_cip; break;
                    default: unknown_trigger_type = true; break;
                    }

                    if (!unknown_trigger_type)
                    {
                      if (bp.trigger_type == AUDIENCE_CHANNEL)
                      {
                        if (bp.time_from != 0 ||
                            bp.time_to == 0 ||
                            bp.min_visits != 1)
                        {
                          logger_->stream(Logging::Logger::WARNING,
                                          Aspect::USER_INFO_MANAGER,
                                          "ADS-IMPL-75") << FUN <<
                            ": Audience channel with BEHAV_PARAMS_LIST_ID = " <<
                            kbpi.id << " is incorrect.";
                        }
                        else
                        {
                          ChannelInterval ci(
                          Generics::Time(bp.time_from),
                          Generics::Time(bp.time_to),
                          bp.min_visits,
                          bp.weight);

                          push_channel_interval_(target_cip, ci);
                        }
                      }
                      else if (bp.time_from == 0 && bp.time_to == 0)
                      {
                        if (bp.min_visits == 1)
                        {
                          target_cip->contextual = true;
                          target_cip->zero_channel = true;
                          target_cip->weight += bp.weight;
                        }
                        else
                        {
                          logger_->stream(Logging::Logger::WARNING,
                                          Aspect::USER_INFO_MANAGER,
                                          "ADS-IMPL-75") << FUN <<
                            ": Channel with BEHAV_PARAMS_LIST_ID = " <<
                            kbpi.id << " is incorrect.";
                        }
                      }
                      else if (bp.time_from < bp.time_to)
                      {
                        ChannelInterval ci(
                          Generics::Time(bp.time_from),
                          Generics::Time(bp.time_to),
                          bp.min_visits,
                          bp.weight);

                        if (bp.time_from == 0 && bp.min_visits == 1)
                        {
                          target_cip->contextual = true;
                        }

                        push_channel_interval_(target_cip.in(), ci);
                      }
                      else
                      {
                        logger_->stream(Logging::Logger::WARNING,
                                        Aspect::USER_INFO_MANAGER,
                                        "ADS-IMPL-74") << FUN <<
                          ": Channel with BEHAV_PARAMS_LIST_ID = " <<
                          kbpi.id << " is incorrect.";
                      }
                    }
                  }

                  str_behav_channel_interval_map[StrBehavIdTypeKey(kbpi.id, PAGE_CHANNEL)] =
                    page_cip;
                  str_behav_channel_interval_map[StrBehavIdTypeKey(kbpi.id, URL_CHANNEL)] =
                    url_cip;
                  str_behav_channel_interval_map[StrBehavIdTypeKey(kbpi.id, URL_KEYWORD_CHANNEL)] =
                    url_keyword_cip;
                  str_behav_channel_interval_map[StrBehavIdTypeKey(kbpi.id, SEARCH_CHANNEL)] =
                    search_cip;
                  str_behav_channel_interval_map[StrBehavIdTypeKey(kbpi.id, AUDIENCE_CHANNEL)] =
                    audience_cip;
                }
              }
            }

            for (SimpleChannelPropertiesList::const_iterator it =
                   channel_properties_list.begin();
                 it != channel_properties_list.end(); ++it)
            {
              const SimpleChannelProperties& key = *it;

              if (key.behav_param_list_id != 0)
              {
                BehavIdChannelIntervalMap::const_iterator it =
                  behav_channel_interval_map.find(
                    BehavIdTypeKey(key.behav_param_list_id,
                                   PAGE_CHANNEL));

                if (it != behav_channel_interval_map.end())
                {
                  channels_config->page_channels[key.channel_id] = it->second;
                }

                it = behav_channel_interval_map.find(
                  BehavIdTypeKey(key.behav_param_list_id,
                                 SEARCH_CHANNEL));

                if (it != behav_channel_interval_map.end())
                {
                  channels_config->search_channels[key.channel_id] = it->second;
                }

                it = behav_channel_interval_map.find(
                  BehavIdTypeKey(key.behav_param_list_id,
                                 URL_CHANNEL));

                if (it != behav_channel_interval_map.end())
                {
                  channels_config->url_channels[key.channel_id] = it->second;
                }

                it = behav_channel_interval_map.find(
                  BehavIdTypeKey(key.behav_param_list_id,
                                 URL_KEYWORD_CHANNEL));

                if (it != behav_channel_interval_map.end())
                {
                  channels_config->url_keyword_channels[key.channel_id] = it->second;
                }

                it = behav_channel_interval_map.find(
                  BehavIdTypeKey(key.behav_param_list_id,
                                 AUDIENCE_CHANNEL));

                if (it != behav_channel_interval_map.end())
                {
                  channels_config->audience_channels[key.channel_id] = it->second;
                }
              }
              else if (!key.str_behav_param_list_id.empty())
              {
                StrBehavIdChannelIntervalMap::const_iterator it =
                  str_behav_channel_interval_map.find(
                    StrBehavIdTypeKey(key.str_behav_param_list_id.c_str(),
                                      PAGE_CHANNEL));

                if (it != str_behav_channel_interval_map.end())
                {
                  channels_config->page_channels[key.channel_id] = it->second;
                }

                it = str_behav_channel_interval_map.find(
                  StrBehavIdTypeKey(key.str_behav_param_list_id.c_str(),
                                    SEARCH_CHANNEL));

                if (it != str_behav_channel_interval_map.end())
                {
                  channels_config->search_channels[key.channel_id] = it->second;
                }

                it = str_behav_channel_interval_map.find(
                  StrBehavIdTypeKey(key.str_behav_param_list_id.c_str(),
                                    URL_CHANNEL));

                if (it != str_behav_channel_interval_map.end())
                {
                  channels_config->url_channels[key.channel_id] = it->second;
                }

                it = str_behav_channel_interval_map.find(
                  StrBehavIdTypeKey(key.str_behav_param_list_id.c_str(),
                                    URL_KEYWORD_CHANNEL));

                if (it != str_behav_channel_interval_map.end())
                {
                  channels_config->url_keyword_channels[key.channel_id] = it->second;
                }

                it = str_behav_channel_interval_map.find(
                  StrBehavIdTypeKey(key.str_behav_param_list_id.c_str(),
                                    AUDIENCE_CHANNEL));

                if (it != str_behav_channel_interval_map.end())
                {
                  channels_config->audience_channels[key.channel_id] = it->second;
                }
              }
            }

            if(res_length == 0)
            {
              logger_->log(
                String::SubString(
                  "Campaign server returns empty channel set."),
                Logging::Logger::WARNING,
                Aspect::USER_INFO_MANAGER,
                "ADS-IMPL-55");
            }
          }

          AdServer::CampaignSvcs::FreqCapConfigInfo_var freq_cap_config_info =
            campaign_server->freq_caps();

          apply_freq_caps_(*freq_cap_config, *freq_cap_config_info);

          const bool first_config_initialization =
            user_info_container->channels_config().in() == 0;

          user_info_container->config(
            time_offset,
            master_stamp,
            channels_config,
            freq_cap_config);

          {
            SyncPolicy::WriteGuard lock(lock_);
            campaignserver_ready_ = true;
          }

          if(first_config_initialization)
          {
            // start cleanup tasks loop
            Task_var delete_old_profiles_msg =
              new DeleteOldProfilesTask(0, this, true);
            task_runner_->enqueue_task(delete_old_profiles_msg);

            Task_var delete_old_temp_profiles_msg =
              new DeleteOldTemporaryProfilesTask(0, this, true);
            task_runner_->enqueue_task(delete_old_temp_profiles_msg);

            if(user_info_manager_config_.UserProfilesCleanup().present())
            {
              Task_var all_users_processing_msg =
                new AllUsersProcessingTask(0, this, true, 0);
              task_runner_->enqueue_task(all_users_processing_msg);
            }

            BaseOperationRecordFetcher::ChunkIdSet chunk_ids;
            
            for(AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap::
                  const_iterator chunk_it = chunk_folders_.begin();
                chunk_it != chunk_folders_.end(); ++chunk_it)
            {
              chunk_ids.insert(chunk_it->first);
            }
            
            // init user operations loading
            if(user_info_manager_config_.UserOperationsLoad().present())
            {
              std::string op_root =
                user_info_manager_config_.UserOperationsLoad()->dir().c_str();

              user_operation_loader_ = new InternalUserOperationLoader(
                check_operations_callback_,
                user_info_container,
                op_root.c_str(),
                user_info_manager_config_.UserOperationsLoad()->unprocessed_dir().c_str(),
                user_info_manager_config_.UserOperationsLoad()->file_prefix().c_str(),
                chunk_ids,
                Generics::Time(
                  user_info_manager_config_.UserOperationsLoad()->check_period()),
                user_info_manager_config_.UserOperationsLoad()->threads());

              add_child_object(user_operation_loader_);
            }

            // init external user operations loading
            if(user_info_manager_config_.ExternalUserOperationsLoad().present())
            {
              std::string op_root =
                user_info_manager_config_.ExternalUserOperationsLoad()->dir().c_str();

              external_user_operation_loader_ = new ExternalUserOperationLoader(
                check_operations_callback_,
                user_operation_processor,
                op_root.c_str(),
                user_info_manager_config_.ExternalUserOperationsLoad()->
                  unprocessed_dir().c_str(),
                user_info_manager_config_.ExternalUserOperationsLoad()->
                  file_prefix().c_str(),
                chunk_ids,
                Generics::Time(
                  user_info_manager_config_.ExternalUserOperationsLoad()->check_period()),
                  user_info_manager_config_.ExternalUserOperationsLoad()->threads());
              
              add_child_object(external_user_operation_loader_);
            }
          }
          break;
        }
        catch(const AdServer::CampaignSvcs::CampaignServer::NotReady& ex)
        {
          campaign_server.release_bad(
            String::SubString("Campaign Server is not ready"));
          logger_->sstream(Logging::Logger::NOTICE,
            Aspect::USER_INFO_MANAGER, "ADS-ICON-10") << FUN <<
            ": Can't update channels configuration. "
            "Campaign Server is not ready. "
            "Caught CampaignServer::NotReady: " << ex.description;
        }
        catch(const AdServer::CampaignSvcs::CampaignServer::ImplementationException& exc)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't update channels configuration. "
            "Caught CampaignServer::ImplementationException: " <<
            exc.description;
          campaign_server.release_bad(ostr.str());
          logger_->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::USER_INFO_MANAGER,
            "ADS-IMPL-56");
        }
        catch(const CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": Can't update channels configuration. "
          "Caught CORBA::SystemException: " << ex;
          campaign_server.release_bad(ostr.str());
          logger_->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::USER_INFO_MANAGER,
            "ADS-ICON-10");
        }
      }
    }
    catch (const eh::Exception& ex)
    {
      logger_->stream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-ICON-10") << FUN <<
        ": Can't update channels configuration. "
        "Caught eh::Exception: " << ex.what();
    }
  }

  void
  UserInfoManagerImpl::update_config_()
    noexcept
  {
    static const char* FUN = "UserInfoManagerImpl::update_config_()";

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::USER_INFO_MANAGER) <<
        "UpdateChannelsConfigTask started.";
    }

    UserInfoContainerAccessor user_info_container =
      get_user_info_container_(false);
    UserOperationProcessorAccessor user_operation_processor =
      get_user_operation_processor_(false);
    if(user_info_container.get().in() && user_operation_processor.get().in())
    {
      // user_info_container can be null after deactivation
      update_channels_config_(
        user_info_container.get(),
        user_operation_processor.get());
    }

    try
    {
      Task_var msg = new UpdateChannelsConfigTask(this, task_runner_);

      scheduler_->schedule(msg,
        Generics::Time::get_time_of_day() +
        user_info_manager_config_.channels_update_period());
    }
    catch(const eh::Exception& ex)
    {
      logger_->stream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-52") << FUN <<
        ": Can't schedule channels updating task. "
        "Caught eh::Exception: " << ex.what();
    }

    if(logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE,
        Aspect::USER_INFO_MANAGER) <<
        "UpdateChannelsConfigTask has finished.";
    }
  }

  void
  UserInfoManagerImpl::rotate_user_operations_backup_()
    noexcept
  {
    static const char* FUN =
      "UserInfoManagerImpl::rotate_user_operations_backup_()";

    user_operation_saver_->rotate();

    try
    {
      Task_var msg = new RotateUserOperationsBackupTask(
        this, task_runner_);

      scheduler_->schedule(msg,
        Generics::Time::get_time_of_day() +
          user_info_manager_config_.UserOperationsBackup()->rotate_period());
    }
    catch(const eh::Exception& ex)
    {
      logger_->stream(Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-52") << FUN <<
        ": Can't schedule user operations backup rotate task. "
        "Caught eh::Exception: " << ex.what();
    }
  }

  void
  UserInfoManagerImpl::apply_freq_caps_(
    FreqCapConfig& freq_cap_config,
    const AdServer::CampaignSvcs::FreqCapConfigInfo& freq_cap_config_info)
    noexcept
  {
    freq_cap_config.confirm_timeout =
      Generics::Time(user_info_manager_config_.FreqCaps().confirm_timeout());

    // fill default frequency cap: fc that will be used
    // in case when <fc id> not found
    //
    for(CORBA::ULong i = 0; i < freq_cap_config_info.freq_caps.length(); ++i)
    {
      const AdServer::CampaignSvcs::FreqCapInfo& fc =
        freq_cap_config_info.freq_caps[i];
      Commons::FreqCap freq_cap;
      freq_cap.fc_id = fc.fc_id;
      freq_cap.lifelimit = fc.lifelimit;
      freq_cap.period = Generics::Time(fc.period);
      freq_cap.window_limit = fc.window_limit;
      freq_cap.window_time = Generics::Time(fc.window_time);
      freq_cap_config.freq_caps.insert(
        std::make_pair(freq_cap.fc_id, freq_cap));
    }

    CorbaAlgs::convert_sequence(
      freq_cap_config_info.campaign_ids, freq_cap_config.campaign_ids);
  }

  void UserInfoManagerImpl::push_channel_interval_(
    ChannelIntervalsPack* cip,
    const ChannelInterval& ci)
    noexcept
  {
    if (ci.time_to < Generics::Time::ONE_DAY)
    {
      cip->short_intervals.insert(ci);
    }
    else if (ci.time_to >= Generics::Time::ONE_DAY &&
             ci.time_from == Generics::Time::ZERO)
    {
      cip->today_long_intervals.insert(ci);
    }
    else if (ci.time_to > ci.time_from &&
             ci.time_from >= Generics::Time::ONE_DAY)
    {
      cip->long_intervals.insert(ci);
    }
  }

  AdServer::ProfilingCommons::LevelMapTraits
  UserInfoManagerImpl::fill_level_map_traits_(
    const xsd::AdServer::Configuration::ChunksConfigType& chunks_config)
    noexcept
  {
    return AdServer::ProfilingCommons::LevelMapTraits(
      AdServer::ProfilingCommons::LevelMapTraits::NONBLOCK_RUNTIME,
      chunks_config.rw_buffer_size(),
      chunks_config.rwlevel_max_size(),
      chunks_config.max_undumped_size(),
      chunks_config.max_levels0(),
      Generics::Time(chunks_config.expire_time()),
      file_controller_);
  }

  AdServer::ProfilingCommons::RocksDB::RocksDBParams
  UserInfoManagerImpl::fill_rocksdb_map_params_(
    const xsd::AdServer::Configuration::ChunksRocksDBConfigType& chunks_config) noexcept
  {std::cout << "TTTTTTTTTTTTTTTTTTTT" << std::endl;
    using RocksDBCompactionStyleType = ::xsd::AdServer::Configuration::RocksDBCompactionStyleType;
    const auto compaction_style_config = chunks_config.compaction_style();
    rocksdb::CompactionStyle compaction_style = rocksdb::kCompactionStyleLevel;
    if (compaction_style_config == RocksDBCompactionStyleType::value::kCompactionStyleLevel)
    {
      compaction_style = rocksdb::kCompactionStyleLevel;
    }
    else if (compaction_style_config == RocksDBCompactionStyleType::value::kCompactionStyleFIFO)
    {
      compaction_style = rocksdb::kCompactionStyleFIFO;
    }

    return AdServer::ProfilingCommons::RocksDB::RocksDBParams(
      chunks_config.block_cache_size_mb(),
      chunks_config.expire_time(),
      compaction_style,
      chunks_config.number_background_threads());
  }

  UserStat
  UserInfoManagerImpl::get_stats()
    /*throw(AdServer::UserInfoSvcs::UserInfoManager::NotReady, eh::Exception)*/
  {
    UserStat user_stat;

    {
      UserInfoContainerAccessor user_info_container = get_user_info_container_();
      user_stat = user_info_container->get_stats();
    }

    {
      SyncPolicy::ReadGuard guard(daily_stat_lock_);
      user_stat.daily_users = daily_users_;
    }

    return user_stat;
  }
} /*UserInfoSvcs*/
} /*AdServer*/
