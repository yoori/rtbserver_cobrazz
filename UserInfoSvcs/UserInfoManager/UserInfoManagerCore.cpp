#include <list>
#include <iostream>
#include <sstream>
#include <vector>
#include <iterator>

#include <PrivacyFilter/Filter.hpp>

#include <Generics/DirSelector.hpp>

#include <Commons/CorbaConfig.hpp>

#include <UserInfoSvcs/UserInfoCommons/Allocator.hpp>

#include "UserInfoConfigCampaignServerSource.hpp"
#include "UserInfoContainer.hpp"
#include "UserInfoManagerCore.hpp"

namespace Aspect
{
  const char USER_INFO_MANAGER[] = "UserInfoManager";
  const char DELETE_OLD_PROFILES[] = "UserInfoManager:DeleteOldProfiles";
} // namespace Aspect

namespace
{
  const unsigned long CHUNKS_RELOAD_PERIOD = 30; // 30 sec

  class CompositeActiveObjectImpl
    : public Generics::RefCountableCompositeActiveObject
  {
  };

  void
  create_path(std::string& first, const char* second)
  /*throw(eh::Exception)*/
  {
    if (second && second[0] == '/')
    {
      first = second;
      return;
    }
    if (*(first.rbegin()) != '/')
    {
      first += "/";
    }
    first += second;
  }

  AdServer::UserInfoSvcs::UserInfoConfigSourcePtr
  make_config_source(
    Logging::Logger* logger,
    const xsd::AdServer::Configuration::UserInfoManagerConfigType& config)
  {
    CORBACommons::CorbaObjectRefList campaign_server_refs;
    Config::CorbaConfigReader::read_multi_corba_ref(
      config.CampaignServerCorbaRef(),
      campaign_server_refs);

    std::vector<std::string> campaign_server_ref_strings;
    campaign_server_ref_strings.reserve(campaign_server_refs.size());
    for (const auto& campaign_server_ref : campaign_server_refs)
    {
      campaign_server_ref_strings.push_back(campaign_server_ref.object_ref);
    }

    return std::make_shared<AdServer::UserInfoSvcs::UserInfoConfigCampaignServerSource>(
      logger,
      campaign_server_ref_strings,
      config.service_index(),
      Generics::Time(config.FreqCaps().confirm_timeout()));
  }

} // namespace

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    std::string
    user_id_to_string(const AdServer::Commons::UserId& user_id)
    {
      return user_id.is_null() ? std::string("<null>") : user_id.to_string();
    }

    template<typename Container>
    void
    print_id_sequence(
      std::ostream& out,
      const char* name,
      const Container& values)
    {
      out << "  " << name << " = [";
      bool first = true;
      for (const auto& value : values)
      {
        if (!first)
        {
          out << ',';
        }
        first = false;
        out << value;
      }
      out << "]\n";
    }

    template<typename Container>
    void
    print_geo_data_sequence(
      std::ostream& out,
      const char* name,
      const Container& values)
    {
      out << "  " << name << " = [";
      bool first = true;
      for (const auto& value : values)
      {
        if (!first)
        {
          out << ',';
        }
        first = false;
        out << "{latitude=" << value.latitude <<
          ",longitude=" << value.longitude <<
          ",accuracy=" << value.accuracy << '}';
      }
      out << "]\n";
    }

    template<typename Container>
    void
    print_channel_weight_sequence(
      std::ostream& out,
      const char* name,
      const Container& values)
    {
      out << "  " << name << " = [";
      bool first = true;
      for (const auto& value : values)
      {
        if (!first)
        {
          out << ',';
        }
        first = false;
        out << value.channel_id << ':' << value.weight;
      }
      out << "]\n";
    }

    template<typename Container>
    void
    print_seq_order_sequence(
      std::ostream& out,
      const char* name,
      const Container& values)
    {
      out << "  " << name << " = [";
      bool first = true;
      for (const auto& value : values)
      {
        if (!first)
        {
          out << ',';
        }
        first = false;
        out << "{ccg_id=" << value.ccg_id <<
          ",set_id=" << value.set_id <<
          ",imps=" << value.imps << '}';
      }
      out << "]\n";
    }

    template<typename Container>
    void
    print_campaign_freq_sequence(
      std::ostream& out,
      const char* name,
      const Container& values)
    {
      out << "  " << name << " = [";
      bool first = true;
      for (const auto& value : values)
      {
        if (!first)
        {
          out << ',';
        }
        first = false;
        out << value.campaign_id << ':' << value.imps;
      }
      out << "]\n";
    }

    void
    debug_print_match_input(
      const char* function_name,
      const UserInfoManagerCore::UserInfo& user_info,
      const UserInfoManagerCore::MatchParams& match_params)
    {
      std::ostringstream out;
      out << "=== " << function_name << " input ===\n"
        << "  user_id = " << user_id_to_string(user_info.user_id) << '\n'
        << "  huser_id = " << user_id_to_string(user_info.huser_id) << '\n'
        << "  time = " << user_info.time << '\n'
        << "  request_colo_id = " << user_info.request_colo_id << '\n'
        << "  current_colo_id = " << user_info.current_colo_id << '\n'
        << "  temporary = " << user_info.temporary << '\n';

      print_id_sequence(
        out, "page_channel_ids", match_params.matched_channels.page_channels);
      print_id_sequence(
        out, "search_channel_ids", match_params.matched_channels.search_channels);
      print_id_sequence(
        out, "url_channel_ids", match_params.matched_channels.url_channels);
      print_id_sequence(
        out, "url_keyword_channel_ids",
        match_params.matched_channels.url_keyword_channels);
      print_id_sequence(
        out, "persistent_channel_ids",
        match_params.matched_channels.persistent_channels);
      out << "  cohort = " << match_params.cohort << '\n'
        << "  cohort2 = " << match_params.cohort2 << '\n';
      print_geo_data_sequence(out, "geo_data_seq", match_params.geo_data_seq);
      out << "  publishers_optin_timeout = " <<
          match_params.publishers_optin_timeout << '\n'
        << "  use_empty_profile = " << match_params.use_empty_profile << '\n'
        << "  filter_contextual_triggers = " <<
          match_params.filter_contextual_triggers << '\n'
        << "  silent_match = " << match_params.silent_match << '\n'
        << "  no_match = " << match_params.no_match << '\n'
        << "  no_result = " << match_params.no_result << '\n'
        << "  provide_channel_count = " <<
          match_params.provide_channel_count << '\n'
        << "  provide_persistent_channels = " <<
          match_params.provide_persistent_channels << '\n'
        << "  change_last_request = " <<
          match_params.change_last_request << '\n'
        << "  ret_freq_caps = " << match_params.ret_freq_caps << '\n';

      std::cout << out.str() << std::flush;
    }

    [[maybe_unused]] void
    debug_print_match_result(
      const char* function_name,
      bool result,
      const UserInfoManagerCore::MatchResult& match_result)
    {
      std::ostringstream out;
      out << "=== " << function_name << " output ===\n"
        << "  return = " << result << '\n'
        << "  adv_channel_count = " << match_result.adv_channel_count << '\n'
        << "  discover_channel_count = " <<
          match_result.discover_channel_count << '\n'
        << "  colo_id = " << match_result.colo_id << '\n'
        << "  times_inited = " << match_result.times_inited << '\n'
        << "  last_request_time = " << match_result.last_request_time << '\n'
        << "  create_time = " << match_result.create_time << '\n'
        << "  session_start = " << match_result.session_start << '\n'
        << "  fraud_request = " << match_result.fraud_request << '\n'
        << "  cohort = " << match_result.cohort << '\n'
        << "  cohort2 = " << match_result.cohort2 << '\n';
      print_geo_data_sequence(out, "geo_data_seq", match_result.geo_data_seq);
      print_id_sequence(
        out, "exclude_pubpixel_accounts",
        match_result.exclude_pubpixel_accounts);
      print_id_sequence(out, "full_freq_caps", match_result.full_freq_caps);
      print_id_sequence(
        out, "full_virtual_freq_caps",
        match_result.full_virtual_freq_caps);
      print_seq_order_sequence(out, "seq_orders", match_result.seq_orders);
      print_campaign_freq_sequence(
        out, "campaign_freqs", match_result.campaign_freqs);
      print_channel_weight_sequence(out, "channels", match_result.channels);
      print_channel_weight_sequence(
        out, "hid_channels", match_result.hid_channels);
      out << "  process_time = " << match_result.process_time << '\n';

      std::cout << out.str() << std::flush;
    }
  }

  UserInfoManagerCore::~UserInfoManagerCore() noexcept = default;

  UserInfoManagerCore::ByteArray
  convert_mem_buf(const Generics::MemBuf& mem_buf)
  {
    return UserInfoManagerCore::ByteArray(
      static_cast<const unsigned char*>(mem_buf.data()),
      static_cast<const unsigned char*>(mem_buf.data()) + mem_buf.size());
  }

  /**
   * UserInfoManagerCore::LoadingProgressProcessor
   */
  UserInfoManagerCore::LoadingProgressProcessor::LoadingProgressProcessor(
    double range) noexcept
    : range_(range), progress_(0.0)
  {
  }

  void
  UserInfoManagerCore::LoadingProgressProcessor::post_progress(
    double value) noexcept
  {
    SyncPolicy::WriteGuard lock(progress_lock_);
    progress_ += value;
  }

  std::string
  UserInfoManagerCore::LoadingProgressProcessor::get_progress_in_percents() noexcept
  {
    std::stringstream progress_str;
    SyncPolicy::ReadGuard lock(progress_lock_);
    progress_str << (progress_ / range_) * 100 << "%";
    return progress_str.str();
  }

  /**
   * UserInfoManagerCore
   */
  UserInfoManagerCore::UserInfoManagerCore(
    Generics::ActiveObjectCallback* callback, Logging::Logger* logger,
    const UserInfoManagerConfig& user_info_manager_config)
    /*throw(Exception)*/
    : UserInfoManagerCore(
        callback,
        logger,
        user_info_manager_config,
        make_config_source(logger, user_info_manager_config))
  {
  }

  UserInfoManagerCore::UserInfoManagerCore(
    Generics::ActiveObjectCallback* callback, Logging::Logger* logger,
    const UserInfoManagerConfig& user_info_manager_config,
    UserInfoConfigSourcePtr config_source)
    /*throw(Exception)*/
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_, 5)),
      user_info_manager_config_(user_info_manager_config),
      file_controller_(
        new AdServer::ProfilingCommons::SSDFileController(
          AdServer::ProfilingCommons::FileController_var(
            new AdServer::ProfilingCommons::PosixFileController(
              nullptr, // stats
              user_info_manager_config.Storage()
                .min_free_space()
                .present()
                ? *user_info_manager_config.Storage().min_free_space()
                : 0
            )
          )
        )
      ),
      user_operation_processor_(new UserOperationProcessorHolder()),
      user_info_container_(new UserInfoContainerHolder()),
      user_info_container_dependent_active_object_(new CompositeActiveObjectImpl()),
      config_source_(std::move(config_source)),
      campaignserver_ready_(false), current_day_(0), daily_users_(0),
      check_operations_callback_(new Logging::ActiveObjectCallbackImpl(
        logger_, "UserInfoManagerCore::check_operations_()",
        Aspect::USER_INFO_MANAGER, "ADS-IMPL-82")),
      loading_progress_processor_(new LoadingProgressProcessor(1.0))
  {
    static const char* FUN = "UserInfoManagerCore::UserInfoManagerCore()";

    if (user_info_manager_config.ReadWriteStats().present())
    {
      file_rw_stats_ = new FileRWStats(
        Generics::Time(user_info_manager_config.ReadWriteStats()->interval()),
        user_info_manager_config.ReadWriteStats()->times());
    }

    file_controller_ = new AdServer::ProfilingCommons::SSDFileController(
      AdServer::ProfilingCommons::FileController_var(
        new AdServer::ProfilingCommons::PosixFileController(
          file_rw_stats_)));

    try
    {
      add_child_object(task_runner_.in());
      add_child_object(scheduler_.in());
      add_child_object(user_operation_processor_.in());
      add_child_object(user_info_container_.in());
    }
    catch (const Generics::CompositeActiveObject::Exception& ex)
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
        if (user_info_manager_config_.Logging()
          .get()
          .ChannelCountStat()
          .present())
        {
          const xsd::AdServer::Configuration::UserInfoManagerLoggerType&
            xsd_logger = user_info_manager_config_.Logging()
              .get()
              .ChannelCountStat()
              .get();

          create_path(
            lft.out_dir,
            AdServer::LogProcessing::ChannelCountStatTraits::log_base_name());

          lft.period = Generics::Time(xsd_logger.flush_period().present()
            ? xsd_logger.flush_period().get()
            : 60);
        }
      }

      user_info_manager_logger_ = new UserInfoManagerLogger(lft);

      Task_var msg = new FlushLogsTask(this, 0);
      task_runner_->enqueue_task(msg);
    }
    catch (const eh::Exception& ex)
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
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    provide_channel_counters_ = false;

    try
    {
      repeat_trigger_timeout_ =
        Generics::Time(user_info_manager_config_.repeat_trigger_timeout());

      profile_lifetime_ = Generics::Time(
        user_info_manager_config_.Storage().BaseChunksConfig().expire_time());

      temp_profile_lifetime_ = Generics::Time(
        user_info_manager_config_.Storage().TempChunksConfig().expire_time());

      Task_var load_chunks_msg = new LoadChunksDataTask(this, 0);
      task_runner_->enqueue_task(load_chunks_msg);

      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->sstream(Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER) <<
          "LoadChunksDataTask was enqueued.";
      }
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't instantiate object. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    placement_colo_id_ = user_info_manager_config_.colo_id();
  }

  void
  UserInfoManagerCore::wait_object()
  /*throw(Generics::ActiveObject::Exception, eh::Exception)*/
  {
    Generics::CompositeActiveObject::wait_object();

    // deactivate (dump) user_info_container only when finished
    // all operations on it (through user_info_processor or directly)
    UserInfoContainer_var user_info_container =
      user_info_container_->get_object();
    if (user_info_container)
    {
      user_info_container->deactivate_object();
      user_info_container->wait_object();
    }

    user_info_container_dependent_active_object_->deactivate_object();
    user_info_container_dependent_active_object_->wait_object();
  }

  bool
  UserInfoManagerCore::uim_ready() noexcept
  {
    UserInfoContainerAccessor user_info_container =
      get_user_info_container_(false);

    return user_info_container.get().in() &&
      user_info_container->channels_config().in();
  }

  std::string
  UserInfoManagerCore::get_progress() noexcept
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

    return str.str();
  }

  void
  UserInfoManagerCore::calc_user_daily_stat_(
    const Generics::Time& current_time,
    const Generics::Time& request_time,
    const Generics::Time& user_time,
    const Generics::Time& time_offset) noexcept
  {
    unsigned long day =
      (current_time + time_offset).tv_sec / Generics::Time::ONE_DAY.tv_sec;
    unsigned long request_day =
      (request_time + time_offset).tv_sec / Generics::Time::ONE_DAY.tv_sec;
    unsigned long user_day =
      (user_time + time_offset).tv_sec / Generics::Time::ONE_DAY.tv_sec;
    if (request_day == day)
    {
      SyncPolicy::WriteGuard guard(daily_stat_lock_);
      if (current_day_ != day)
      {
        current_day_ = day;
        daily_users_ = 0;
      }
      if (current_day_ == request_day && user_day != request_day)
      {
        daily_users_++;
      }
    }
  }

  AdServer::Commons::Task<bool>
  UserInfoManagerCore::co_update_user_freq_caps(
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& time,
    const Generics::Uuid& request_id,
    const std::vector<unsigned long>& freq_caps,
    const std::vector<unsigned long>& uc_freq_caps,
    const std::vector<unsigned long>& virtual_freq_caps,
    const std::vector<SeqOrder>& seq_orders_seq,
    const std::vector<unsigned long>& campaign_ids_seq,
    const std::vector<unsigned long>& uc_campaign_ids_seq)
  {
    static const char* FUN = "UserInfoManagerCore::co_update_user_freq_caps()";

    try
    {
      UserInfoContainerAccessor user_info_container = get_user_info_container_();

      UserFreqCapProfile::FreqCapIdArray fcs;
      UserFreqCapProfile::FreqCapIdArray uc_fcs;
      UserFreqCapProfile::FreqCapIdArray virtual_fcs;
      UserFreqCapProfile::SeqOrderArray seq_orders;
      UserFreqCapProfile::CampaignIds campaign_ids;
      UserFreqCapProfile::CampaignIds uc_campaign_ids;
      fcs.assign(freq_caps.begin(), freq_caps.end());
      uc_fcs.assign(uc_freq_caps.begin(), uc_freq_caps.end());
      virtual_fcs.assign(virtual_freq_caps.begin(), virtual_freq_caps.end());
      campaign_ids.assign(campaign_ids_seq.begin(), campaign_ids_seq.end());
      uc_campaign_ids.assign(uc_campaign_ids_seq.begin(), uc_campaign_ids_seq.end());

      for (const auto& source_seq_order : seq_orders_seq)
      {
        UserFreqCapProfile::SeqOrder seq_order;
        seq_order.ccg_id = source_seq_order.ccg_id;
        seq_order.set_id = source_seq_order.set_id;
        seq_order.imps = source_seq_order.imps;
        seq_orders.push_back(seq_order);
      }

      co_return co_await user_info_container->co_update_freq_caps(
        user_id, time, request_id, fcs, uc_fcs, virtual_fcs, seq_orders,
        campaign_ids, uc_campaign_ids,
        AdServer::ProfilingCommons::OP_RUNTIME);
    }
    catch (const UserOperationProcessor::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't update freq caps. Caught UserOperationProcessor::NotReady: " << ex.what();
      throw UserInfoManagerCore::NotReady(ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't update freq caps. Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      throw UserInfoManagerCore::ChunkNotFound(ostr.str());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't update freq caps. Caught eh::Exception: " << ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }
  }

  AdServer::Commons::Task<bool>
  UserInfoManagerCore::co_confirm_user_freq_caps(
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& time,
    const Generics::Uuid& request_id,
    const std::set<unsigned long>& exclude_pubpixel_accounts)
  {
    static const char* FUN = "UserInfoManagerCore::co_confirm_user_freq_caps()";

    try
    {
      UserInfoContainerAccessor user_info_container = get_user_info_container_();
      co_return co_await user_info_container->co_confirm_freq_caps(
        user_id,
        time,
        request_id,
        exclude_pubpixel_accounts);
    }
    catch (const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't confirm freq caps. Caught UserInfoContainer::NotReady: " << ex.what();
      throw UserInfoManagerCore::NotReady(ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't confirm freq caps. Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      throw UserInfoManagerCore::ChunkNotFound(ostr.str());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't confirm freq caps. Caught eh::Exception: " << ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }
  }

  AdServer::Commons::Task<bool>
  UserInfoManagerCore::co_consider_publishers_optin(
    const AdServer::Commons::UserId& user_id,
    const std::set<unsigned long>& exclude_pubpixel_accounts,
    const Generics::Time& now)
  {
    static const char* FUN = "UserInfoManagerCore::co_consider_publishers_optin()";

    try
    {
      UserInfoContainerAccessor user_info_container = get_user_info_container_();
      co_return co_await user_info_container->co_consider_publishers_optin(
        user_id,
        exclude_pubpixel_accounts,
        now,
        AdServer::ProfilingCommons::OP_RUNTIME);
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't consider publisher. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      throw UserInfoManagerCore::ChunkNotFound(ostr.str());
    }
    catch (const UserInfoContainer::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't consider publisher. "
        "Caught UserInfoContainer::Exception: " << ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't consider publisher. Caught eh::Exception: " <<
        ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }
  }

  bool
  UserInfoManagerCore::remove_user_profile(const AdServer::Commons::UserId& user_id)
  {
    static const char* FUN = "UserInfoManagerCore::remove_user_profile()";

    try
    {
      UserOperationProcessorAccessor user_operation_processor =
        get_user_operation_processor_();

      return user_operation_processor->remove_user_profile(user_id);
    }
    catch (const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't remove user profile. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
      throw UserInfoManagerCore::NotReady(ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't remove user profile. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      throw UserInfoManagerCore::ChunkNotFound(ostr.str());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't remove user profile. "
        "Caught eh::Exception: " << ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }

    return 0; // never reach
  }

  Generics::Time
  UserInfoManagerCore::get_master_stamp()
  {
    static const char* FUN = "UserInfoManager::get_master_stamp()";

    try
    {
      UserInfoContainerAccessor user_info_container = get_user_info_container_();
      return user_info_container->master_stamp();
    }
    catch (const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get config master stamp. "
        "Caught UserInfoContainer::Exception: " << ex.what();
      throw UserInfoManagerCore::NotReady(ostr.str());
    }
    catch (const UserInfoContainer::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't get config master stamp. "
        "Caught UserInfoContainer::Exception: " << ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't config master stamp. "
        "Caught eh::Exception: " << ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }
  }

  AdServer::Commons::Task<bool>
  UserInfoManagerCore::co_get_user_profile(
    const AdServer::Commons::UserId& user_id, bool temporary,
    const ProfilesRequest& profile_request, UserProfiles& user_profile)
  {
    static const char* FUN = "UserInfoManager::co_get_user_profile()";

    try
    {
      UserInfoContainerAccessor user_info_container =
        get_user_info_container_();

      SmartMemBuf_var mb_base_profile_out;
      SmartMemBuf_var mb_add_profile_out;
      SmartMemBuf_var mb_history_profile_out;
      SmartMemBuf_var mb_fc_profile_out;
      SmartMemBuf_var mb_pref_profile_out;

      if (co_await user_info_container->co_get_user_profile(
        user_id, temporary,
        profile_request.base_profile ? &mb_base_profile_out : 0,
        profile_request.add_profile ? &mb_add_profile_out : 0,
        profile_request.history_profile ? &mb_history_profile_out : 0,
        profile_request.freq_cap_profile ? &mb_fc_profile_out : 0))
      {
        if (mb_base_profile_out.in())
        {
          user_profile.base_user_profile =
            convert_mem_buf(mb_base_profile_out->membuf());
        }

        if (mb_add_profile_out.in())
        {
          user_profile.add_user_profile =
            convert_mem_buf(mb_add_profile_out->membuf());
        }

        if (mb_history_profile_out.in())
        {
          user_profile.history_user_profile =
            convert_mem_buf(mb_history_profile_out->membuf());
        }

        if (mb_fc_profile_out.in())
        {
          user_profile.freq_cap = convert_mem_buf(mb_fc_profile_out->membuf());
        }

        if (mb_pref_profile_out.in())
        {
          user_profile.pref_profile =
            convert_mem_buf(mb_pref_profile_out->membuf());
        }

        co_return true;
      }

      co_return false;
    }
    catch (const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can not get user profile. "
        "Caught UserInfoContainer::Exception: " << ex.what();
      throw UserInfoManagerCore::NotReady(ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can not get user profile. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      throw UserInfoManagerCore::ChunkNotFound(ostr.str());
    }
    catch (const UserInfoContainer::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can not get user profile. "
        "Caught UserInfoContainer::Exception: " << ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can not get user profile. "
        "Caught eh::Exception: " << ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }
  }

  AdServer::Commons::Task<bool>
  UserInfoManagerCore::co_remove_user_profile(
    const AdServer::Commons::UserId& user_id)
  {
    static const char* FUN = "UserInfoManagerCore::co_remove_user_profile()";

    try
    {
      UserInfoContainerAccessor user_info_container =
        get_user_info_container_();

      co_return co_await user_info_container->co_remove_user_profile(user_id);
    }
    catch (const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN
           << ": Can not remove user profile. "
              "Caught UserInfoContainer::NotReady: "
           << ex.what();
      throw UserInfoManagerCore::NotReady(ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN
           << ": Can not remove user profile. "
              "Caught UserInfoContainer::ChunkNotFound: "
           << ex.what();
      throw UserInfoManagerCore::ChunkNotFound(ostr.str());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN
           << ": Can not remove user profile. "
              "Caught eh::Exception: "
           << ex.what();

      throw UserInfoManagerCore::Exception(ostr.str());
    }
  }

  AdServer::Commons::Task<bool>
  UserInfoManagerCore::co_merge(
    const UserInfo& user_info,
    const MatchParams& match_params,
    const UserProfiles& merge_user_profile,
    bool& merge_success,
    Generics::Time& last_request)
  {
    static const char* FUN = "UserInfoManagerCore::co_merge()";

    try
    {
      merge_success = true;

      UserId uid = user_info.user_id;
      UserId huid = user_info.huser_id;

      bool household = uid.is_null();
      UserId user_id = household ? huid : uid;

      UserInfoContainerAccessor user_info_container = get_user_info_container_();

      UserInfoContainer::RequestMatchParams request_params(
        user_id,
        user_info.time,
        String::SubString(match_params.cohort),
        String::SubString(match_params.cohort2),
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

      last_request = Generics::Time::ZERO;

      /* merge users */
      try
      {
        long placement_colo_id = user_info.current_colo_id != -1 ?
          user_info.current_colo_id :
          placement_colo_id_;

        MemBuf merge_base_profile(
          merge_user_profile.base_user_profile.data(),
          merge_user_profile.base_user_profile.size());
        MemBuf merge_add_profile(
          merge_user_profile.add_user_profile.data(),
          merge_user_profile.add_user_profile.size());
        MemBuf merge_history_profile(
          merge_user_profile.history_user_profile.data(),
          merge_user_profile.history_user_profile.size());
        MemBuf merge_freq_cap_profile(
          merge_user_profile.freq_cap.data(),
          merge_user_profile.freq_cap.size());

        UserInfoManagerLogger::HistoryOptimizationInfo ho_info;

        co_await user_info_container->co_merge(
          request_params, merge_base_profile.membuf(),
          merge_add_profile.membuf(), merge_history_profile.membuf(),
          merge_freq_cap_profile.membuf(), user_app, placement_colo_id,
          placement_colo_id, AdServer::ProfilingCommons::OP_RUNTIME,
          &ho_info);

        if (ho_info.isp_date != Generics::Time::ZERO && !household)
        {
          ho_info.colo_id = placement_colo_id_;
          user_info_manager_logger_->process_history_optimization(ho_info);
        }

        last_request = user_app.last_request;

        if (!household && !huid.is_null())
        {
          UserInfoContainer::RequestMatchParams hid_request_params(
            huid,
            user_info.time,
            String::SubString(match_params.cohort),
            String::SubString(match_params.cohort2),
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

          co_await user_info_container->co_merge(
            hid_request_params, merge_base_profile.membuf(),
            merge_add_profile.membuf(), merge_history_profile.membuf(),
            merge_freq_cap_profile.membuf(), hid_user_app, placement_colo_id,
            placement_colo_id, AdServer::ProfilingCommons::OP_RUNTIME);
        }
      }
      catch (const UserInfoContainer::ChunkNotFound& ex)
      {
        throw;
      }
      catch (const eh::Exception& ex)
      {
        merge_success = false;

        logger_->sstream(Logging::Logger::ERROR, Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-50") <<
          FUN << ": Can't do merging. Caught eh::Exception: " << ex.what();
      }
    }
    catch (const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can not merge users. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
      throw UserInfoManagerCore::NotReady(ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can not merge users. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      throw UserInfoManagerCore::ChunkNotFound(ostr.str());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can not merge user. "
        "Caught eh::Exception: " << ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }

    co_return merge_success;
  }

  AdServer::Commons::Task<bool>
  UserInfoManagerCore::co_fraud_user(
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& time)
  {
    static const char* FUN = "UserInfoManagerCore::co_fraud_user()";

    try
    {
      UserInfoContainerAccessor user_info_container = get_user_info_container_();
      co_return co_await user_info_container->co_fraud_user(user_id, time);
    }
    catch (const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't update user. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
      throw UserInfoManagerCore::NotReady(ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't update user. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      throw UserInfoManagerCore::ChunkNotFound(ostr.str());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't update user. Caught eh::Exception: " << ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }
  }

  AdServer::Commons::Task<bool>
  UserInfoManagerCore::co_match(
    const UserInfo& user_info,
    const MatchParams& match_params,
    MatchResult& match_result)
  {
    static const char* FUN = "UserInfoManagerCore::co_match()";

    //debug_print_match_input(FUN, user_info, match_params);

    try
    {
      Generics::Timer process_timer;
      process_timer.start();

      UserId user_id = user_info.user_id;

      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << "Match request: " << std::endl
             << "  User id: "
             << PrivacyFilter::filter(user_id.to_string().c_str(), "USER_ID")
             << std::endl
             << "  current colo id: " << user_info.current_colo_id << std::endl
             << "  Input search channels: ";
        for (const auto channel_id : match_params.matched_channels.search_channels)
        {
          ostr << channel_id << ',';
        }
        ostr << std::endl << "  Input page channels: ";
        for (const auto channel_id : match_params.matched_channels.page_channels)
        {
          ostr << channel_id << ',';
        }
        ostr << std::endl << "  Input url channels: ";
        for (const auto channel_id : match_params.matched_channels.url_channels)
        {
          ostr << channel_id << ',';
        }
        ostr << std::endl << "  Input url keyword channels: ";
        for (const auto channel_id : match_params.matched_channels.url_keyword_channels)
        {
          ostr << channel_id << ',';
        }

        ostr << std::endl << "  Input persistent channels: ";
        for (const auto channel_id : match_params.matched_channels.persistent_channels)
        {
          ostr << channel_id << ',';
        }
        ostr << std::endl;

        logger_->log(ostr.str(), Logging::Logger::TRACE,
                     Aspect::USER_INFO_MANAGER);
      }

      UserInfoContainerAccessor user_info_container = get_user_info_container_();

      CoordData coord_data;
      if (!match_params.geo_data_seq.empty())
      {
        coord_data.defined = true;
        coord_data.latitude = match_params.geo_data_seq[0].latitude;
        coord_data.longitude = match_params.geo_data_seq[0].longitude;
        coord_data.accuracy = match_params.geo_data_seq[0].accuracy;
      }
      else
      {
        coord_data.defined = false;
      }

      UserInfoContainer::RequestMatchParams request_params(
        user_id,
        user_info.time,
        String::SubString(match_params.cohort),
        String::SubString(match_params.cohort2),
        match_params.use_empty_profile,
        user_info.request_colo_id,
        repeat_trigger_timeout_,
        match_params.filter_contextual_triggers,
        user_info.temporary,
        match_params.silent_match,
        match_params.no_match,
        match_params.no_result,
        match_params.provide_channel_count,
        match_params.provide_persistent_channels,
        match_params.change_last_request,
        false, // household
        coord_data.defined ? &coord_data : 0);

      UserInfoContainer::UserAppearance user_app;

      AdServer::UserInfoSvcs::ChannelMatchMap result_channels(
        match_result.channels.get_allocator().resource());

      ColoUserId colo_user_id;

      long placement_colo_id = user_info.current_colo_id != -1 ?
        user_info.current_colo_id :
        placement_colo_id_;

      ProfileProperties profile_properties;

      UserInfoManagerLogger::HistoryOptimizationInfo ho_info;
      UniqueChannelsResult unique_channels_result;

      co_await user_info_container->co_match(
        request_params,
        placement_colo_id,
        placement_colo_id,
        colo_user_id,
        match_params.matched_channels,
        result_channels,
        user_app,
        profile_properties,
        AdServer::ProfilingCommons::OP_RUNTIME,
        &ho_info,
        &unique_channels_result);

      match_result.adv_channel_count = unique_channels_result.simple_channels;
      match_result.discover_channel_count =
        unique_channels_result.discover_channels;

      if (ho_info.isp_date != Generics::Time::ZERO)
      {
        ho_info.colo_id = placement_colo_id_;
        user_info_manager_logger_->process_history_optimization(ho_info);
      }

      match_result.colo_id = -1;
      match_result.times_inited = true;
      match_result.last_request_time = user_app.last_request;
      match_result.create_time = user_app.create_time;
      match_result.session_start = user_app.session_start;
      match_result.fraud_request = profile_properties.fraud_request;
      match_result.cohort = profile_properties.cohort;
      match_result.cohort2 = profile_properties.cohort2;

      match_result.geo_data_seq.reserve(
        profile_properties.geo_data_list.size());
      for (const auto& geo_data : profile_properties.geo_data_list)
      {
        match_result.geo_data_seq.push_back(GeoData{
          geo_data.latitude, geo_data.longitude, geo_data.accuracy});
      }

      if (match_params.publishers_optin_timeout != Generics::Time::ZERO)
      {
        std::list<unsigned long> publishers;

        co_await user_info_container->co_get_optin_publishers(
          user_info.user_id, match_params.publishers_optin_timeout,
          publishers);

        match_result.exclude_pubpixel_accounts.assign(
          publishers.begin(), publishers.end());
      }

      if (match_params.ret_freq_caps)
      {
        UserFreqCapProfile::FreqCapIdArray freq_caps;
        UserFreqCapProfile::FreqCapIdArray virtual_freq_caps;
        UserFreqCapProfile::SeqOrderArray seq_orders;
        UserFreqCapProfile::CampaignFreqs campaign_freqs;

        try
        {
          co_await user_info_container->co_get_full_freq_caps(
            user_info.user_id,
            user_info.time,
            freq_caps,
            virtual_freq_caps,
            seq_orders,
            campaign_freqs);

          match_result.full_freq_caps.assign(
            freq_caps.begin(), freq_caps.end());
          match_result.full_virtual_freq_caps.assign(
            virtual_freq_caps.begin(), virtual_freq_caps.end());

          for (const auto& seq_order : seq_orders)
          {
            match_result.seq_orders.push_back(
              SeqOrder{seq_order.ccg_id, seq_order.set_id, seq_order.imps});
          }

          for (const auto& campaign_freq : campaign_freqs)
          {
            match_result.campaign_freqs.push_back(
              CampaignFreq{campaign_freq.campaign_id, campaign_freq.imps});
          }
        }
        catch (const UserInfoContainer::UserIsFraud& ex)
        {
          Stream::Error ostr;
          ostr << "User '" << user_id << "' is fraud: " << ex.what();
          logger()->log(ostr.str(), Logging::Logger::INFO,
            Aspect::USER_INFO_MANAGER, "ADS-IMPL-0000");
          match_result.fraud_request = true;
        }
      }

      /* generate output params */
      match_result.channels.reserve(result_channels.size());
      for (const auto& channel : result_channels)
      {
        match_result.channels.push_back(
          ChannelWeight{channel.first, channel.second});
      }

      if (!request_params.silent_match)
      {
        calc_user_daily_stat_(
          Generics::Time::get_time_of_day(),
          request_params.current_time,
          user_app.last_request,
          user_info_container->time_offset());
      }

      process_timer.stop();
      match_result.process_time = process_timer.elapsed_time();

      //debug_print_match_result(FUN, true, match_result);
      co_return true;
    }
    catch (const UserInfoContainer::NotReady& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match user. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
      throw UserInfoManagerCore::NotReady(ostr.str());
    }
    catch (const UserInfoContainer::ChunkNotFound& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match user. "
        "Caught UserInfoContainer::ChunkNotFound: " << ex.what();
      throw UserInfoManagerCore::ChunkNotFound(ostr.str());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't match user. "
        "Caught eh::Exception: " << ex.what();
      throw UserInfoManagerCore::Exception(ostr.str());
    }

    co_return false; // never reach
  }

  void
  UserInfoManagerCore::get_controllable_chunks(
    UserInfoManagerCore::ChunkIdList& chunk_ids,
    unsigned long& common_chunks_number)
    /*throw(Exception)*/
  {
    for (AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap::
        const_iterator chunk_it = chunk_folders_.begin();
      chunk_it != chunk_folders_.end(); ++chunk_it)
    {
      chunk_ids.push_back(chunk_it->first);
    }

    common_chunks_number =
      user_info_manager_config_.Storage().common_chunks_number();
  }

  void
  UserInfoManagerCore::clear_expired(bool synch, const Generics::Time&, long)
  {
    static const char* FUN = "UserInfoManagerCore::clear_expired()";

    if (!synch)
    {
      try
      {
        Task_var delete_old_profiles_msg =
          new DeleteOldProfilesTask(0, this, false);
        task_runner_->enqueue_task(delete_old_profiles_msg);

        Task_var delete_old_temp_profiles_msg =
          new DeleteOldTemporaryProfilesTask(0, this, false);
        task_runner_->enqueue_task(delete_old_temp_profiles_msg);

        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->sstream(Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER) <<
            "tasks for expired profiles clearing was enqueued forcibly.";
        }
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't delete old user profiles. "
          "Caught eh::Exception: " << ex.what();
        throw UserInfoManagerCore::Exception(ostr.str());
      }
    }
    else
    {
      delete_old_profiles_(false);
      delete_old_temporary_profiles_(false);
    }
  }

  UserInfoManagerCore::UserInfoContainerAccessor
  UserInfoManagerCore::get_user_info_container_(bool throw_not_ready)
    /*throw(NotReady)*/
  {
    if (throw_not_ready)
    {
      SyncPolicy::ReadGuard lock(lock_);
      if (!campaignserver_ready_)
      {
        throw NotReady("UserInfoContainer is not ready");
      }
    }

    UserInfoContainerAccessor accessor = user_info_container_->get_accessor();

    if (throw_not_ready && !accessor.get().in())
    {
      throw NotReady("UserInfoContainer is not ready");
    }

    return accessor;
  }

  UserInfoManagerCore::UserOperationProcessorAccessor
  UserInfoManagerCore::get_user_operation_processor_(bool throw_not_ready)
    /*throw(NotReady)*/
  {
    UserOperationProcessorAccessor accessor = user_operation_processor_->get_accessor();

    if (throw_not_ready && !accessor.get().in())
    {
      throw NotReady("UserInfoProcessor is not ready");
    }

    return accessor;
  }

  void
  UserInfoManagerCore::flush_logs_() noexcept
  {
    static const char* FUN = "UserInfoManagerCore::flush_logs()";

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

      logger_->log(ostr.str(), Logging::Logger::ERROR,
        Aspect::USER_INFO_MANAGER, "ADS-IMPL-77");
    }

    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": logs flushed, next flush at " << next_flush.get_gm_time();
      logger_->log(ostr.str(), Logging::Logger::TRACE,
        Aspect::USER_INFO_MANAGER);
    }

    if (next_flush != Generics::Time::ZERO)
    {
      try
      {
        Task_var msg = new FlushLogsTask(this, task_runner_);
        scheduler_->schedule(msg, next_flush);
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't schedule next flush task. "
          "eh::Exception caught:" << ex.what();

        logger_->log(ostr.str(), Logging::Logger::ERROR,
          Aspect::USER_INFO_MANAGER, "ADS-IMPL-78");
      }

      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->sstream(Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER) <<
          FUN << ": flush USER_INFO_MANAGER logger for " << next_flush.get_gm_time();
      }
    }
  }

  void
  UserInfoManagerCore::delete_old_profiles_(bool reschedule) noexcept
  {
    static const char* FUN = "UserInfoManagerCore::delete_old_profiles_()";

    logger()->log(String::SubString("Cleanup old profiles task started"),
      Logging::Logger::INFO, Aspect::DELETE_OLD_PROFILES);

    try
    {
      UserInfoContainerAccessor uic = get_user_info_container_(false);

      if (uic.get().in())
      {
        uic->delete_old_profiles(profile_lifetime_);
      }
      else
      {
        logger_->log(
          String::SubString(
            "Can't do profiles cleaning - UserInfoManager isn't ready."),
          Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER);
      }
    }
    catch (const UserInfoContainer::NotReady& ex)
    {
      logger_->sstream(Logging::Logger::NOTICE, Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-51") <<
        FUN << ": Can't delete old user profiles. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
    }
    catch (const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::NOTICE, Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-51") <<
        FUN << ": Can't delete old user profiles. Caught eh::Exception: " <<
        ex.what();
    }

    if (reschedule)
    {
      try
      {
        Generics::Time now = Generics::Time::get_time_of_day();

        Task_var msg =
          new DeleteOldProfilesTask(task_runner_, this, reschedule);
        scheduler_->schedule(
          msg,
          now + std::min(std::max(profile_lifetime_ / 10, Generics::Time(1)),
            Generics::Time::ONE_DAY / 4));
      }
      catch (const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY, Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-52") <<
          FUN << ": Can't schedule task. Caught eh::Exception: " << ex.what();
      }
    }
  }

  void
  UserInfoManagerCore::delete_old_temporary_profiles_(bool reschedule) noexcept
  {
    static const char* FUN =
      "UserInfoManagerCore::delete_old_temporary_profiles_()";

    try
    {
      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->stream(Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER) <<
          FUN << ": cleanup old temporary profiles task started.";
      }

      UserInfoContainerAccessor user_info_container = get_user_info_container_(false);

      if (user_info_container.get().in())
      {
        user_info_container->delete_old_temporary_profiles(
          temp_profile_lifetime_);
      }
      else
      {
        logger_->log(
          String::SubString(
            "Can't do profiles cleaning - UserInfoManager isn't ready."),
          Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER);
      }
    }
    catch (const UserInfoContainer::NotReady& ex)
    {
      logger_->sstream(Logging::Logger::NOTICE, Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-51") <<
        FUN << ": Can't delete old user temporary profiles. "
        "Caught UserInfoContainer::NotReady: " << ex.what();
    }
    catch (const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::NOTICE, Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-51") <<
        FUN << ": Can't delete old user temporary profiles. Caught "
        "eh::Exception: " << ex.what();
    }

    if (logger_->log_level() >= Logging::Logger::INFO)
    {
      logger_->stream(Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER) <<
        FUN << ": cleanup old temporary profiles task has finished.";
    }

    if (reschedule)
    {
      try
      {
        Generics::Time now = Generics::Time::get_time_of_day();

        Task_var msg =
          new DeleteOldTemporaryProfilesTask(task_runner_, this, reschedule);
        scheduler_->schedule(
          msg,
          now + std::max(
            temp_profile_lifetime_ / 10,
            Generics::Time(1)));
      }
      catch (const eh::Exception& ex)
      {
        logger_->sstream(Logging::Logger::EMERGENCY, Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-52") <<
          FUN << ": Can't schedule task. Caught eh::Exception: " << ex.what();
      }
    }
  }

  void
  UserInfoManagerCore::load_chunk_files_() noexcept
  {
    static const char* FUN = "UserInfoManagerCore::load_chunk_files_()";

    try
    {
      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->stream(Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER)
          << "LoadChunksDataTask started.";
      }

      xsd::AdServer::Configuration::UserInfoManagerStorageType& storage_config =
        user_info_manager_config_.Storage();

      UserInfoContainer_var user_info_container = new UserInfoContainer(
        logger_, storage_config.common_chunks_number(), chunk_folders_,
        fill_level_map_traits_(storage_config.AddChunksConfig()),
        fill_level_map_traits_(storage_config.TempChunksConfig()),
        fill_level_map_traits_(storage_config.HistoryChunksConfig()),
        fill_level_map_traits_(storage_config.BaseChunksConfig()),
        fill_level_map_traits_(storage_config.FreqCapChunksConfig()),
        user_info_manager_config_.colo_id(), Generics::Time::ZERO,
        Generics::Time(
          user_info_manager_config_.history_optimization_period()),
        provide_channel_counters_,
        Generics::Time(user_info_manager_config_.session_timeout()),
        user_info_manager_config_.max_base_profile_waiters(),
        user_info_manager_config_.max_temp_profile_waiters(),
        user_info_manager_config_.max_freqcap_profile_waiters(),
        loading_progress_processor_);

      user_info_container->activate_object();

      UserOperationProcessor_var user_operation_processor;

      if (user_info_manager_config_.UserOperationsBackup().present())
      {
        user_operation_saver_ = new UserOperationSaver(
          callback_, logger(),
          user_info_manager_config_.UserOperationsBackup()->dir().c_str(),
          user_info_manager_config_.UserOperationsBackup()->file_prefix()
            .c_str(),
          storage_config.common_chunks_number(), file_controller_,
          user_info_container);

        user_info_container_dependent_active_object_->add_child_object(
          user_operation_saver_.in());

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

      logger_->log(String::SubString("Chunk maps loaded."),
        Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER);
    }
    catch (const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::EMERGENCY, Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-53") <<
        FUN << ": Can't load chunk files. Caught eh::Exception: " << ex.what();
    }

    if (!user_info_container_.in())
    {
      try
      {
        const Generics::Time tm =
          Generics::Time::get_time_of_day() + CHUNKS_RELOAD_PERIOD;

        Task_var msg = new LoadChunksDataTask(this, task_runner_);
        scheduler_->schedule(msg, tm);
      }
      catch (const eh::Exception& ex)
      {
        std::ostringstream ostr;
        ostr << FUN << ": Can't schedule chunks reloading task. "
          "Caught eh::Exception: " << ex.what();
        logger_->log(ostr.str(), Logging::Logger::EMERGENCY,
          Aspect::USER_INFO_MANAGER, "ADS-IMPL-79");
      }
    }
    else
    {
      try
      {
        Task_var update_channels_msg = new UpdateChannelsConfigTask(this, 0);
        task_runner_->enqueue_task(update_channels_msg);
      }
      catch (const eh::Exception& ex)
      {
        logger_->stream(Logging::Logger::EMERGENCY, Aspect::USER_INFO_MANAGER,
          "ADS-IMPL-52") <<
          FUN << ": Can't schedule channels updating task. "
          "Caught eh::Exception: " << ex.what();
      }
    }

    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER) <<
        "LoadChunksDataTask has finished.";
    }
  }

  void
  UserInfoManagerCore::update_channels_config_(
    UserInfoContainer* user_info_container,
    UserOperationProcessor* user_operation_processor) noexcept
  {
    static const char* FUN = "UserInfoManagerCore::update_channels_config_()";

    try
    {
      UserInfoConfigSource::Config config = config_source_->get_config();
      const bool first_config_initialization =
        user_info_container->channels_config().in() == 0;

      user_info_container->config(
        Generics::Time::ZERO,
        Generics::Time::get_time_of_day(),
        config.channels_config,
        config.freq_cap_config);

      {
        SyncPolicy::WriteGuard lock(lock_);
        campaignserver_ready_ = true;
      }

      if (first_config_initialization)
      {
        Task_var delete_old_profiles_msg =
          new DeleteOldProfilesTask(0, this, true);
        task_runner_->enqueue_task(delete_old_profiles_msg);

        Task_var delete_old_temp_profiles_msg =
          new DeleteOldTemporaryProfilesTask(0, this, true);
        task_runner_->enqueue_task(delete_old_temp_profiles_msg);

        BaseOperationRecordFetcher::ChunkIdSet chunk_ids;

        for (
          AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap::
            const_iterator chunk_it = chunk_folders_.begin();
          chunk_it != chunk_folders_.end();
          ++chunk_it)
        {
          chunk_ids.insert(chunk_it->first);
        }

        if (user_info_manager_config_.UserOperationsLoad().present())
        {
          std::string op_root =
            user_info_manager_config_.UserOperationsLoad()->dir().c_str();

          user_operation_loader_ = new InternalUserOperationLoader(
            check_operations_callback_,
            user_info_container,
            op_root.c_str(),
            user_info_manager_config_.UserOperationsLoad()
              ->unprocessed_dir()
              .c_str(),
            user_info_manager_config_.UserOperationsLoad()
              ->file_prefix()
              .c_str(),
            chunk_ids,
            Generics::Time(user_info_manager_config_.UserOperationsLoad()
              ->check_period()),
            user_info_manager_config_.UserOperationsLoad()->threads());

          add_child_object(user_operation_loader_.in());
        }

        if (user_info_manager_config_.ExternalUserOperationsLoad().present())
        {
          std::string op_root =
            user_info_manager_config_.ExternalUserOperationsLoad()->dir()
              .c_str();

          external_user_operation_loader_ = new ExternalUserOperationLoader(
            check_operations_callback_,
            user_operation_processor,
            op_root.c_str(),
            user_info_manager_config_.ExternalUserOperationsLoad()
              ->unprocessed_dir()
              .c_str(),
            user_info_manager_config_.ExternalUserOperationsLoad()
              ->file_prefix()
              .c_str(),
            chunk_ids,
            Generics::Time(
              user_info_manager_config_.ExternalUserOperationsLoad()
                ->check_period()),
            user_info_manager_config_.ExternalUserOperationsLoad()->threads());

          add_child_object(external_user_operation_loader_.in());
        }
      }
    }
    catch (const eh::Exception& ex)
    {
      logger_->stream(
        Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-ICON-10")
        << FUN
        << ": Can't update channels configuration. "
          "Caught eh::Exception: "
        << ex.what();
    }
  }

  void
  UserInfoManagerCore::update_config_() noexcept
  {
    static const char* FUN = "UserInfoManagerCore::update_config_()";

    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER) <<
        "UpdateChannelsConfigTask started.";
    }

    UserInfoContainerAccessor user_info_container =
      get_user_info_container_(false);
    UserOperationProcessorAccessor user_operation_processor =
      get_user_operation_processor_(false);
    if (user_info_container.get().in() && user_operation_processor.get().in())
    {
      // user_info_container can be null after deactivation
      update_channels_config_(
        user_info_container.get(),
        user_operation_processor.get());
    }

    try
    {
      Task_var msg = new UpdateChannelsConfigTask(this, task_runner_);

      scheduler_->schedule(
        msg,
        Generics::Time::get_time_of_day() +
          user_info_manager_config_.channels_update_period());
    }
    catch (const eh::Exception& ex)
    {
      logger_->stream(
        Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-52") <<
        FUN << ": Can't schedule channels updating task. "
        "Caught eh::Exception: " << ex.what();
    }

    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE, Aspect::USER_INFO_MANAGER) <<
        "UpdateChannelsConfigTask has finished.";
    }
  }

  void
  UserInfoManagerCore::rotate_user_operations_backup_() noexcept
  {
    static const char* FUN =
      "UserInfoManagerCore::rotate_user_operations_backup_()";

    user_operation_saver_->rotate();

    try
    {
      Task_var msg = new RotateUserOperationsBackupTask(this, task_runner_);

      scheduler_->schedule(
        msg,
        Generics::Time::get_time_of_day() +
          user_info_manager_config_.UserOperationsBackup()->rotate_period());
    }
    catch (const eh::Exception& ex)
    {
      logger_->stream(
        Logging::Logger::EMERGENCY,
        Aspect::USER_INFO_MANAGER,
        "ADS-IMPL-52") <<
        FUN << ": Can't schedule user operations backup rotate task. "
        "Caught eh::Exception: " << ex.what();
    }
  }

  AdServer::ProfilingCommons::LevelMapTraits
  UserInfoManagerCore::fill_level_map_traits_(
    const xsd::AdServer::Configuration::ChunksConfigType& chunks_config) noexcept
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

  UserStat
  UserInfoManagerCore::get_stats()
    /*throw(NotReady, eh::Exception)*/
  {
    UserStat user_stat;

    {
      UserInfoContainerAccessor user_info_container =
        get_user_info_container_();
      user_stat = user_info_container->get_stats();
    }

    {
      SyncPolicy::ReadGuard guard(daily_stat_lock_);
      user_stat.daily_users = daily_users_;
    }

    return user_stat;
  }

} // namespace AdServer::UserInfoSvcs
