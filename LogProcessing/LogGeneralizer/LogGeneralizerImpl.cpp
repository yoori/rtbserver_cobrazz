
#include "LogGeneralizerImpl.hpp"
#include "LogProcessorImpl.hpp"
#include "DbConnectionFactory.hpp"

#include "ErrorCode.hpp"

#include <Commons/CorbaAlgs.hpp>
#include <Commons/ConfigUtils.hpp>
#include <Commons/PathManip.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer {
namespace LogProcessing {

const char DEFERRED_DIR[] = "Deferred";

namespace {

//const char ASPECT[] = "LogGeneralizerImpl";

const Generics::Time XSEARCH_REQUEST_TIMEOUT(600);

} // namespace

const char *LOG_GEN_DB_ERR_CODE_0    = "ADS-DB-1000";
const char *LOG_GEN_DB_ERR_CODE_1    = "ADS-DB-1001";
const char *LOG_GEN_DB_ERR_CODE_2    = "ADS-DB-1002";
const char *LOG_GEN_IMPL_ERR_CODE_0  = "ADS-IMPL-1000";
const char *LOG_GEN_IMPL_ERR_CODE_1  = "ADS-IMPL-1001";
const char *LOG_GEN_IMPL_ERR_CODE_2  = "ADS-IMPL-1002";
const char *LOG_GEN_IMPL_ERR_CODE_3  = "ADS-IMPL-1003";
const char *LOG_GEN_IMPL_ERR_CODE_4  = "ADS-IMPL-1004";
const char *LOG_GEN_IMPL_ERR_CODE_5  = "ADS-IMPL-1005";
const char *LOG_GEN_IMPL_ERR_CODE_6  = "ADS-IMPL-1006";
const char *LOG_GEN_IMPL_ERR_CODE_7  = "ADS-IMPL-1007";
const char *LOG_GEN_IMPL_ERR_CODE_8  = "ADS-IMPL-1008";
const char *LOG_GEN_IMPL_ERR_CODE_9  = "ADS-IMPL-1009";
const char *LOG_GEN_IMPL_ERR_CODE_10 = "ADS-IMPL-1010";
const char *LOG_GEN_IMPL_ERR_CODE_11 = "ADS-IMPL-1011";

LogGeneralizerImpl::LogGeneralizerImpl(
  Generics::ActiveObjectCallback *callback,
  Logging::Logger *logger,
  const LogGeneralizerConfigType &config,
  ProcStatImpl_var &proc_stat_impl
)
  /*throw(Exception, eh::Exception)*/
:
  callback_(ReferenceCounting::add_ref(callback)),
  logger_(ReferenceCounting::add_ref(logger)),
  scheduler_(new Generics::Planner(callback_)),
  in_logs_dir_(config.input_logs_dir()),
  out_logs_dir_(config.output_logs_dir()),
  proc_stat_impl_(proc_stat_impl),
  log_generalizer_stat_map_bundle_(new LogGeneralizerStatMapBundle),
  fr_interrupter_(new FileReceiverInterrupter())
{
  if (config.db_dump_timeout().present())
  {
    Configuration::db_dump_timeout = config.db_dump_timeout().get();
  }
  if (*in_logs_dir_.rbegin() != '/')
  {
    in_logs_dir_ += "/";
  }
  if (*out_logs_dir_.rbegin() != '/')
  {
    out_logs_dir_ += "/";
  }
  const auto archive = config.archive();
  if (archive.present())
  {
    switch (*archive)
    {
      case xsd::AdServer::Configuration::ArchiveType::value::no_compression:
        break;
      case xsd::AdServer::Configuration::ArchiveType::value::gzip_default:
        archive_params_ = AdServer::LogProcessing::Archive::gzip_default_compression_params;
        break;
      case xsd::AdServer::Configuration::ArchiveType::value::gzip_best_compression:
        archive_params_ = AdServer::LogProcessing::Archive::gzip_best_compression_params;
        break;
      case xsd::AdServer::Configuration::ArchiveType::value::gzip_best_speed:
        archive_params_ = AdServer::LogProcessing::Archive::gzip_best_speed_params;
        break;
      case xsd::AdServer::Configuration::ArchiveType::value::bz2_default:
        archive_params_ = AdServer::LogProcessing::Archive::bzip2_default_compression_params;
        break;
      default:
      {
        Stream::Error stream;
        stream << FNS
               << "Unknown archive type";
        throw Exception(stream);
      }
    }
  }

  if (config.DBConnection().present())
  {
    db_conn_factory_composite_ = new CompositeActiveObjectImpl;

    pg_conn_factory_impl_ = new PostgresConnectionFactoryImpl;
    const char* pg_conn =
      config.DBConnection().get().Postgres().connection_string().c_str();
    pg_conn_factory_impl_->initialize(pg_conn);
    db_conn_factory_composite_->add_child_object(pg_conn_factory_impl_);

    add_child_object(db_conn_factory_composite_);
  }

  if (config.XSearchStatParams().present())
  {
    xsearch_stat_url_ = config.XSearchStatParams().get().url();
    http_interface_ = HTTP::CreateSyncHttp(&XSEARCH_REQUEST_TIMEOUT,
      &XSEARCH_REQUEST_TIMEOUT, &XSEARCH_REQUEST_TIMEOUT);
  }

  {
    // Two part to reduce build time
    apply_log_proc_config_(config.LogProcessing(), pg_conn_factory_impl_);

    apply_log_proc_config_part2_(config.LogProcessing());
  }

  add_child_object(fr_interrupter_);
  add_child_object(task_runner_);
  add_child_object(scheduler_);
}

LogGeneralizerImpl::~LogGeneralizerImpl()
  noexcept
{
  deactivate_object();
  wait_object();
  scheduler_->clear();
  task_runner_->clear();
  ProcessingContexts::clear();
}

void
LogGeneralizerImpl::deactivate_object()
  /*throw(Generics::CompositeActiveObject::Exception, eh::Exception)*/
{
  // Order below is important
  Generics::CompositeActiveObject::deactivate_object();
  ProcessingContexts::deactivation();
}

unsigned
LogGeneralizerImpl::calc_required_number_of_threads(
  const LogProcessingType &config
)
{
  unsigned num_threads = 0;

  num_threads += calc_max_number_of_threads(config.CCGStat());
  num_threads += calc_max_number_of_threads(config.CCStat());
  num_threads += calc_max_number_of_threads(config.ColoUsers());
  num_threads += calc_max_number_of_threads(config.TagAuctionStat());

  num_threads += calc_max_number_of_threads(config.CMPStat()) +
    (db_enabled() ? 1 : 0);
  num_threads += calc_max_number_of_threads(config.CreativeStat()) +
    (db_enabled() ? 1 : 0);

  num_threads += calc_max_number_of_threads(config.WebStat());

  num_threads += calc_max_number_of_threads(config.ActionRequest());
  num_threads += calc_max_number_of_threads(config.AdvertiserUserStat());
  num_threads += calc_max_number_of_threads(config.CCGKeywordStat());
  num_threads += calc_max_number_of_threads(config.CCGUserStat());
  num_threads += calc_max_number_of_threads(config.CCUserStat());
//  CampaignStat logs are processed by CampaignUserStat log processor
  num_threads += calc_max_number_of_threads(config.CampaignUserStat());
  num_threads += calc_max_number_of_threads(config.ChannelCountStat());
  num_threads +=
    calc_max_number_of_threads(config.ChannelInventoryEstimationStat());
  num_threads += calc_max_number_of_threads(config.ColoUpdateStat());
  num_threads += calc_max_number_of_threads(config.ColoUserStat());
  num_threads += calc_max_number_of_threads(config.ExpressionPerformance());
  num_threads += calc_max_number_of_threads(config.GlobalColoUserStat());
  num_threads += calc_max_number_of_threads(config.PageLoadsDailyStat());
  num_threads += calc_max_number_of_threads(config.PassbackStat());
  num_threads += calc_max_number_of_threads(config.SiteReferrerStat());
//  SiteStat logs are processed by SiteUserStat log processor
  num_threads += calc_max_number_of_threads(config.SiteUserStat());

  num_threads += calc_max_number_of_threads(config.ActionStat());

  num_threads += calc_max_number_of_threads(config.CCGSelectionFailureStat());
  num_threads += calc_max_number_of_threads(config.ChannelHitStat());
  num_threads += calc_max_number_of_threads(config.ChannelImpInventory());
  num_threads += calc_max_number_of_threads(config.ChannelInventory());
  num_threads += calc_max_number_of_threads(config.ChannelOverlapUserStat());
  num_threads += calc_max_number_of_threads(config.ChannelPerformance());
  num_threads += calc_max_number_of_threads(config.ChannelPriceRange());
  num_threads += calc_max_number_of_threads(config.ChannelTriggerImpStat());
  num_threads += calc_max_number_of_threads(config.ChannelTriggerStat());
  num_threads += calc_max_number_of_threads(config.DeviceChannelCountStat());
  num_threads += calc_max_number_of_threads(config.SearchEngineStat());
  num_threads += calc_max_number_of_threads(config.TagPositionStat());
  num_threads += calc_max_number_of_threads(config.UserAgentStat());
  num_threads += calc_max_number_of_threads(config.UserProperties());
  num_threads += calc_max_number_of_threads(config.CampaignReferrerStat());

  num_threads += calc_max_number_of_threads(config.SearchTermStat());

  return num_threads;
}

void LogGeneralizerImpl::change_db_state(bool enable)
{
  if (!db_enabled())
  {
    return;
  }
  try
  {
    Stream::Error es;
    if (enable)
    {
      db_conn_factory_composite_->activate_object();
      es << "DB state set to: ENABLED";
    }
    else
    {
      db_conn_factory_composite_->deactivate_object();
      db_conn_factory_composite_->wait_object();
      es << "DB state set to: DISABLED";
    }
    logger_->notice(es.str(), ASPECT);
  }
  catch (const eh::Exception &ex)
  {
    Stream::Error es;
    es << "Cannot set state: Caught eh::Exception: " << ex.what();
    throw Exception(es);
  }
}

void
LogGeneralizerImpl::check_logs(const LogProcThreadInfo_var& context) noexcept
{
  try
  {
    context->log_processor->check_and_load();
  }
  catch (const eh::Exception& ex)
  {
    Stream::Error es;
    es << FNT << ": an eh::Exception has been caught while "
      "processing log files: " << ex.what();
    logger_->error(es.str(), ASPECT, LOG_GEN_IMPL_ERR_CODE_11);
  }
}

void
LogGeneralizerImpl::move_deferred_logs(
  const char* log_base_name,
  const FileReceiver_var& file_rcvr,
  const FileReceiver_var& def_file_rcvr
)
  noexcept
{
  {
    Stream::Error es;
    es << __FUNCTION__ << ": Enter";
    logger_->trace(es.str(), ASPECT, 1);
  }
  try
  {
    std::string in_dir(in_logs_dir_);
    ((in_dir += log_base_name) += "/");

    std::string deferred_in_dir(in_dir);
    deferred_in_dir += DEFERRED_DIR;

    def_file_rcvr->fetch_files(deferred_in_dir.c_str(), log_base_name);

    FileReceiver::FileGuard_var file;
    std::string tmp;

    while ((file = def_file_rcvr->get_eldest(tmp)))
    {
      file_rcvr->move(file->full_path().c_str());
    }
  }
  catch (const eh::Exception &ex)
  {
    Stream::Error es;
    es << FNS << "an eh::Exception has been caught while "
      "processing log files: " << ex.what();
    logger_->error(es.str(), ASPECT, LOG_GEN_IMPL_ERR_CODE_11); // FIXME
  }
  {
    Stream::Error es;
    es << __FUNCTION__ << ": Exit";
    logger_->trace(es.str(), ASPECT, 1);
  }
}

void
LogGeneralizerImpl::clear_expired_hits_data() noexcept
{
  {
    Stream::Error es;
    es << FNS << "Enter";
    logger_->trace(es.str(), ASPECT, 2);
  }
  try
  {
    hits_filter_->clear_expired(Generics::Time::get_time_of_day() -
      Generics::Time::ONE_DAY * (days_to_keep_ - 1));
  }
  catch (const eh::Exception &ex)
  {
    Stream::Error es;
    es << FNS << "an eh::Exception has been caught "
      "while reporting a DB dump timeout. : " << ex.what();
    logger_->error(es.str(), ASPECT, LOG_GEN_IMPL_ERR_CODE_4);
  }
  {
    Stream::Error es;
    es << FNS << "Exit";
    logger_->trace(es.str(), ASPECT, 2);
  }
}

void
LogGeneralizerImpl::schedule_clear_expired_msg() noexcept
{
  {
    Stream::Error es;
    es << FNS << "Enter";
    logger_->trace(es.str(), ASPECT, 2);
  }
  try
  {
    Msg_var msg = new ClearExpiredHitsFilterDataMsg(this);
    Generics::Time sched_time =
      Generics::Time::get_time_of_day() + Generics::Time::ONE_HOUR;
    scheduler_->schedule(msg, sched_time);
  }
  catch (const eh::Exception &ex)
  {
    Stream::Error es;
    es << FNS << "an eh::Exception has been caught "
      "while rescheduling the msg: " << ex.what();
    callback_->critical(es.str());
  }
  {
    Stream::Error es;
    es << FNS << "Exit";
    logger_->trace(es.str(), ASPECT, 2);
  }
}

template <typename LogTraits>
void
LogGeneralizerImpl::disable_upload_() noexcept
{
  const LogProcThreadInfo_var& context(ProcessingContexts::get<LogTraits>());
  if (context)
  {
    LogProcThreadInfo::ConditionalGuardT guard(
      context->upload_event, context->upload_event_mutex);
    context->upload_enabled = false;
    while (context->available_dump_task_count != context->MAX_DUMP_TASK_COUNT)
    {
      guard.wait();
    }
  }
}

template <typename LogTraits>
void
LogGeneralizerImpl::enable_upload_() noexcept
{
  const LogProcThreadInfo_var& context(ProcessingContexts::get<LogTraits>());
  if (context)
  {
    LogProcThreadInfo::GuardT guard(context->upload_event_mutex);
    context->upload_enabled = true;
    context->upload_event.broadcast();
  }
}

void LogGeneralizerImpl::stop_stat_upload(CORBA::ULong client_id)
{
  static const char FUNC[] = "AdServer::LogProcessing::LogGeneralizerImpl::"
                             "stop_stat_upload";
  if (!db_enabled())
  {
    typedef AdServer::LogProcessing::LogGeneralizer::NotSupported NotSupported;
    Stream::Error es;
    es << FUNC << ": Operation is not supported";
    CORBACommons::throw_desc<NotSupported>(es.str());
  }

  Guard_ disable_upload_guard(start_stop_upload_lock_);

  disable_upload_<CustomCreativeStatExtTraits>();
  disable_upload_<DeferredCreativeStatExtTraits>();
  disable_upload_<CustomCmpStatExtTraits>();
  disable_upload_<DeferredCmpStatExtTraits>();

  LogGeneralizerStatMapBundle &lgsm_bundle = *log_generalizer_stat_map_bundle_;

  LogGeneralizerStatMapBundle::WriteGuardT lgsm_guard(lgsm_bundle.lock);

  LogGeneralizerStatMap::value_type::second_type& lgs_value =
    lgsm_bundle.map[client_id];
  if (!lgs_value)
  {
    lgs_value = new LogGeneralizerStatValue;
  }
  lgs_value->upload_stopped = true;
}

void
LogGeneralizerImpl::start_stat_upload(
  CORBA::ULong client_id,
  CORBA::Boolean clear
)
{
  static const char FUNC[] = "AdServer::LogProcessing::LogGeneralizerImpl::"
                             "start_stat_upload";
  if (!db_enabled())
  {
    typedef AdServer::LogProcessing::LogGeneralizer::NotSupported NotSupported;
    Stream::Error es;
    es << FUNC << ": Operation is not supported";
    CORBACommons::throw_desc<NotSupported>(es.str());
  }

  Guard_ enable_upload_guard(start_stop_upload_lock_);

  LogGeneralizerStatMapBundle &lgsm_bundle = *log_generalizer_stat_map_bundle_;

  LogGeneralizerStatMapBundle::ReadGuardT lgsm_guard(lgsm_bundle.lock);

  LogGeneralizerStatMap &lgs_map = lgsm_bundle.map;

  LogGeneralizerStatMap::iterator lgsm_it = lgs_map.find(client_id);
  if (lgsm_it == lgs_map.end())
  {
    typedef AdServer::LogProcessing::LogGeneralizer::CollectionNotStarted
      CollectionNotStarted;
    Stream::Error es;
    es << FUNC << ": Data collection is not started";
    CORBACommons::throw_desc<CollectionNotStarted>(es.str());
  }

  {
    LogGeneralizerStatValue &lgs_value = *lgsm_it->second;
    LogGeneralizerStatValue::GuardT lgsv_guard(lgs_value.lock);

    lgs_value.upload_stopped = false;
    if (clear)
    {
      lgs_value.map.clear();
      lgs_value.start_clear_timestamp = Generics::Time::get_time_of_day();
    }
  }

  for (LogGeneralizerStatMap::const_iterator lgsm_it = lgs_map.begin();
    lgsm_it != lgs_map.end(); ++lgsm_it)
  {
    if (lgsm_it->second->upload_stopped)
    {
      return;
    }
  }

  enable_upload_<CustomCreativeStatExtTraits>();
  enable_upload_<DeferredCreativeStatExtTraits>();
  enable_upload_<CustomCmpStatExtTraits>();
  enable_upload_<DeferredCmpStatExtTraits>();
}

StatInfo*
LogGeneralizerImpl::get_stat_info(
  CORBA::ULong client_id,
  CORBA::Boolean clear
)
{
  static const char FUNC[] = "AdServer::LogProcessing::LogGeneralizerImpl::"
                             "get_stat_info";
  if (!db_enabled())
  {
    typedef AdServer::LogProcessing::LogGeneralizer::NotSupported NotSupported;
    Stream::Error es;
    es << FUNC << ": Operation is not supported";
    CORBACommons::throw_desc<NotSupported>(es.str());
  }

  LogGeneralizerStatMapBundle &lgsm_bundle = *log_generalizer_stat_map_bundle_;

  LogGeneralizerStatMapBundle::ReadGuardT lgsm_guard(lgsm_bundle.lock);

  const LogGeneralizerStatMap &lgs_map = lgsm_bundle.map;
  LogGeneralizerStatMap::const_iterator lgsm_it = lgs_map.find(client_id);
  if (lgsm_it == lgs_map.end())
  {
    typedef AdServer::LogProcessing::LogGeneralizer::CollectionNotStarted
      CollectionNotStarted;
    Stream::Error es;
    es << FUNC << ": Data collection is not started";
    CORBACommons::throw_desc<CollectionNotStarted>(es.str());
  }

  LogGeneralizerStatValue &lgs_value = *lgsm_it->second;

  LogGeneralizerStatValue::GuardT lgsv_guard(lgs_value.lock);

  StatInfo_var stat_info = new StatInfo;
  CampaignStatSeq &cmp_stats = stat_info->campaign_stats;
  cmp_stats.length(lgs_value.map.size());
  unsigned i = 0;

  for (CampaignStatMap::const_iterator cmp_map_it = lgs_value.map.begin();
    cmp_map_it != lgs_value.map.end(); ++cmp_map_it, ++i)
  {
    CampaignStatInfo &cmp_stat_info = cmp_stats[i];

    cmp_stat_info.sdate = CorbaAlgs::pack_time(cmp_map_it->first.sdate());
    cmp_stat_info.adv_sdate =
      CorbaAlgs::pack_time(cmp_map_it->first.adv_sdate());
    cmp_stat_info.adv_account_id = cmp_map_it->first.adv_account_id();
    cmp_stat_info.campaign_id = cmp_map_it->first.campaign_id();
    cmp_stat_info.ccg_id = cmp_map_it->first.ccg_id();
    cmp_stat_info.adv_account_amount =
      CorbaAlgs::pack_decimal(cmp_map_it->second.adv_account_amount);
    cmp_stat_info.adv_amount =
      CorbaAlgs::pack_decimal(cmp_map_it->second.adv_amount);
    cmp_stat_info.adv_comm_amount =
      CorbaAlgs::pack_decimal(cmp_map_it->second.adv_comm_amount);
    cmp_stat_info.adv_payable_comm_amount =
      CorbaAlgs::pack_decimal(cmp_map_it->second.adv_payable_comm_amount);

    CreativeStatSeq &crtv_stats = cmp_stat_info.creative_stats;
    crtv_stats.length(cmp_map_it->second.creative_stats.size());
    typedef CampaignStatValueDef::CreativeStatMap CreativeStatMapT;
    const CreativeStatMapT &crtv_map = cmp_map_it->second.creative_stats;
    unsigned j = 0;
    for (CreativeStatMapT::const_iterator crtv_it = crtv_map.begin();
      crtv_it != crtv_map.end(); ++crtv_it, ++j)
    {
      crtv_stats[j].cc_id = crtv_it->first;
      crtv_stats[j].requests = crtv_it->second.requests();
      crtv_stats[j].impressions = crtv_it->second.imps();
      crtv_stats[j].clicks = crtv_it->second.clicks();
      crtv_stats[j].actions = crtv_it->second.actions();
    }

    RequestStatsHourlyExtStatSeq &rshe_stats = cmp_stat_info.request_stats_hourly_ext_stats;
    rshe_stats.length(cmp_map_it->second.request_stats_hourly_ext_stats.size());
    typedef CampaignStatValueDef::RequestStatsHourlyExtStatMap RequestStatsHourlyExtStatMapT;
    const RequestStatsHourlyExtStatMapT &rshe_map = cmp_map_it->second.request_stats_hourly_ext_stats;
    unsigned m = 0;
    for (RequestStatsHourlyExtStatMapT::const_iterator rshe_it = rshe_map.begin();
      rshe_it != rshe_map.end(); ++rshe_it, ++m)
    {
      rshe_stats[m].cc_id = rshe_it->first;
      rshe_stats[m].requests = rshe_it->second.requests();
      rshe_stats[m].impressions = rshe_it->second.imps();
      rshe_stats[m].clicks = rshe_it->second.clicks();
      rshe_stats[m].actions = rshe_it->second.actions();
      rshe_stats[m].undup_imps = rshe_it->second.undup_imps();
      rshe_stats[m].undup_clicks = rshe_it->second.undup_clicks();
      rshe_stats[m].ym_confirmed_clicks = rshe_it->second.ym_confirmed_clicks();
      rshe_stats[m].ym_bounced_clicks = rshe_it->second.ym_bounced_clicks();
      rshe_stats[m].ym_robots_clicks = rshe_it->second.ym_robots_clicks();
    }

    PublisherAmountSeq &pub_amounts = cmp_stat_info.publisher_amounts;
    pub_amounts.length(cmp_map_it->second.publisher_amounts.size());
    typedef CampaignStatValueDef::PublisherAmountMap
      PublisherAmountMapT;
    const PublisherAmountMapT &pub_map = cmp_map_it->second.publisher_amounts;
    unsigned k = 0;
    for (PublisherAmountMapT::const_iterator pub_it = pub_map.begin();
      pub_it != pub_map.end(); ++pub_it, ++k)
    {
      pub_amounts[k].publisher_account_id = pub_it->first;
      pub_amounts[k].adv_amount = CorbaAlgs::pack_decimal(pub_it->second);
    }

    TagAmountSeq &tag_amounts = cmp_stat_info.tag_amounts;
    tag_amounts.length(cmp_map_it->second.tag_amounts.size());
    typedef CampaignStatValueDef::TagAmountMap TagAmountMapT;
    const TagAmountMapT &tag_map = cmp_map_it->second.tag_amounts;
    unsigned l = 0;
    for (TagAmountMapT::const_iterator tag_it = tag_map.begin();
      tag_it != tag_map.end(); ++tag_it, ++l)
    {
      tag_amounts[l].tag_id = tag_it->first;
      tag_amounts[l].pub_isp_amount =
        CorbaAlgs::pack_decimal(tag_it->second.pub_isp_amount());
      tag_amounts[l].adv_amount =
        CorbaAlgs::pack_decimal(tag_it->second.adv_amount());
      tag_amounts[l].adv_comm_amount =
        CorbaAlgs::pack_decimal(tag_it->second.adv_comm_amount());
    }

    // fill ctr reset stats
    CtrResetStatSeq& ctr_reset_stat_seq = cmp_stat_info.ctr_reset_stats;
    ctr_reset_stat_seq.length(cmp_map_it->second.ctr_reset_stats.size());
    CORBA::ULong ctr_reset_stat_i = 0;
    for(CampaignStatValueDef::CtrResetStatMap::
          const_iterator ctr_reset_stat_it =
            cmp_map_it->second.ctr_reset_stats.begin();
        ctr_reset_stat_it != cmp_map_it->second.ctr_reset_stats.end();
        ++ctr_reset_stat_it, ++ctr_reset_stat_i)
    {
      ctr_reset_stat_seq[ctr_reset_stat_i].ctr_reset_id =
        ctr_reset_stat_it->first;
      ctr_reset_stat_seq[ctr_reset_stat_i].impressions =
        ctr_reset_stat_it->second.imps();
    }
  }

  if (clear)
  {
    lgs_value.map.clear();
  }

  return stat_info._retn();
}

void
LogGeneralizerImpl::MoveLogsTask::execute() noexcept
{
  log_generalizer_->move_deferred_logs(stat_proc_info_->LOG_TYPE.c_str(),
    stat_proc_info_->log_processor->get_file_receiver(),
    deferred_stat_proc_info_->log_processor->get_file_receiver());
  try
  {
    PlannerTask_var check_logs_task(
      new CheckDeferrableLogsTask(log_generalizer_, stat_proc_info_));
    check_logs_task->deliver();

    PlannerTask_var check_deferred_logs_task(
      new CheckLogsTask(log_generalizer_, deferred_stat_proc_info_));
    check_deferred_logs_task->deliver();
  }
  catch (const eh::Exception &ex)
  {
    Stream::Error es;
    es << FNS << "an eh::Exception has been caught while "
      "scheduling the task(s): " << ex.what();
    log_generalizer_->callback_->critical(es.str());
  }
}

} // namespace LogProcessing
} // namespace AdServer

