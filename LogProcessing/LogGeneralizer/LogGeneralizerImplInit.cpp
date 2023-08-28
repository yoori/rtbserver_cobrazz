#include "LogGeneralizerImpl.hpp"
#include "LogProcessorImpl.hpp"

namespace AdServer {
namespace LogProcessing {

typedef ProcTraits<
  GenericLogProcessorImpl<CcgStatExtTraits, LogVersionManager3<
    CcgStatExtTraits> >, CcgStatProcessor>
    CcgStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<CcStatExtTraits, LogVersionManager3<
    CcStatExtTraits> >, CcStatProcessor>
    CcStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<CampaignUserStatExtTraits, LogVersionManager3<
    CampaignUserStatExtTraits> >, CampaignUserStatProcessor>
    CampaignUserStatProcTraits;

typedef ProcTraits<ColoUsersProcessor> ColoUsersProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<PageLoadsDailyStatExtTraits, LogVersionManager3<
    PageLoadsDailyStatExtTraits> >, PageLoadsDailyStatProcessor>
    PageLoadsDailyStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<SiteUserStatExtTraits, LogVersionManager3<
    SiteUserStatExtTraits> >, SiteUserStatProcessor>
    SiteUserStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<TagAuctionStatExtTraits, LogVersionManager3<
    TagAuctionStatExtTraits> >, TagAuctionStatProcessor>
    TagAuctionStatProcTraits;

struct CmpStatProcTraits: ProcTraits<CmpStatProcessor>
{
  typedef GenericLogProcessorImpl<CustomCmpStatExtTraits,
    LogVersionManager2<CustomCmpStatExtTraits> >
      DeferrableLogProcessorType;

  typedef CustomLogProcessorImpl<DeferredCmpStatExtTraits>
    DeferredLogProcessorType;

  typedef GenericLogProcessorImpl<CustomCmpStatExtTraits,
    LogVersionManager9<CustomCmpStatExtTraits> >
      DeferrableLogPgCsvProcessorType;

  typedef CustomLogProcessorImpl<DeferredCmpStatExtTraits,
    LogVersionManager9<DeferredCmpStatExtTraits> >
      DeferredLogPgCsvProcessorType;
};

struct CreativeStatProcTraits: ProcTraits<CreativeStatProcessor>
{
  typedef GenericLogProcessorImpl<CustomCreativeStatExtTraits,
    LogVersionManager2<CustomCreativeStatExtTraits> >
      DeferrableLogProcessorType;

  typedef CustomLogProcessorImpl<DeferredCreativeStatExtTraits>
    DeferredLogProcessorType;

  typedef GenericLogProcessorImpl<CustomCreativeStatExtTraits,
    LogVersionManager9<CustomCreativeStatExtTraits> >
      DeferrableLogPgCsvProcessorType;

  typedef CustomLogProcessorImpl<DeferredCreativeStatExtTraits,
    LogVersionManager9<DeferredCreativeStatExtTraits> >
      DeferredLogPgCsvProcessorType;
};

/// Write CSV when db_enabled()
typedef ProcTraits<
  RequestStatsHourlyExtStatProcessor2, RequestStatsHourlyExtStatProcessor>
    RequestStatsHourlyExtStatProcTraits;

typedef ProcTraits<
  ChannelOverlapUserStatProcessor2, ChannelOverlapUserStatProcessor>
    ChannelOverlapUserStatProcTraits;

typedef ProcTraits<ChannelPerformanceProcessor2, ChannelPerformanceProcessor>
  ChannelPerformanceProcTraits;

typedef ProcTraits<ChannelPriceRangeProcessor2, ChannelPriceRangeProcessor>
  ChannelPriceRangeProcTraits;

typedef ProcTraits<
  ChannelTriggerImpStatProcessor2, ChannelTriggerImpStatProcessor>
    ChannelTriggerImpStatProcTraits;

typedef ProcTraits<ChannelTriggerStatProcessor2, ChannelTriggerStatProcessor>
  ChannelTriggerStatProcTraits;

typedef ProcTraits<
  DeviceChannelCountStatProcessor2, DeviceChannelCountStatProcessor>
    DeviceChannelCountStatProcTraits;

typedef ProcTraits<SearchEngineStatProcessor2, SearchEngineStatProcessor>
  SearchEngineStatProcTraits;

typedef ProcTraits<SearchTermStatProcessor2, SearchTermStatProcessor>
  SearchTermStatProcTraits;

typedef ProcTraits<TagPositionStatProcessor2, TagPositionStatProcessor>
  TagPositionStatProcTraits;

typedef ProcTraits<UserAgentStatProcessor2, UserAgentStatProcessor>
  UserAgentStatProcTraits;

typedef ProcTraits<UserPropertiesProcessor2, UserPropertiesProcessor>
  UserPropertiesProcTraits;

typedef ProcTraits<
  CcgSelectionFailureStatProcessor2, CcgSelectionFailureStatProcessor>
    CcgSelectionFailureStatProcTraits;

template <class LOG_PROC_TRAITS_>
void
LogGeneralizerImpl::init_deferrable_log_proc_info(
  const PostgresConnectionFactoryImpl_var& pg_conn_factory,
  const LogProcessingParamsDeferrableCSVTypeOptional& log_proc_params
)
  /*throw(eh::Exception)*/
{
  if (!log_proc_params.present())
  {
    return;
  }

  CollectorBundleParams bundle_params =
  {
    log_proc_params.get().max_size(),
  };

  typedef LOG_PROC_TRAITS_ ProcTraits;

  typedef typename ProcTraits::LogProcessorType::Traits Traits;

  typedef typename ProcTraits::DeferrableLogProcessorType::Traits
    DeferrableTraits;

  typedef typename ProcTraits::DeferredLogProcessorType::Traits DeferredTraits;

  const char* log_base_name = Traits::log_base_name();

  std::string in_dir = in_logs_dir_;
  in_dir += log_base_name;

  std::string out_dir = out_logs_dir_;
  out_dir += log_base_name;

  LogProcThreadInfo_var context(
    ProcessingContexts::create<DeferrableTraits>(
      log_proc_params, in_dir, logger_,
        task_runner_, scheduler_, callback_, proc_stat_impl_));

  std::string def_in_dir = in_dir;
  (def_in_dir += "/") += DEFERRED_DIR;

  LogProcThreadInfo_var deferred_context(
    ProcessingContexts::create<DeferredTraits>(
      log_proc_params, def_in_dir, logger_,
        task_runner_, scheduler_, callback_, proc_stat_impl_));

  static const char UPLOAD_TYPE_POSTGRES[]     = "postgres";
  static const char UPLOAD_TYPE_POSTGRES_CSV[] = "postgres_csv";

  if (log_proc_params.get().upload_type() == UPLOAD_TYPE_POSTGRES)
  {
    typedef typename ProcTraits::DeferrableLogProcessorType LogProcessor;
    typedef typename ProcTraits::DeferredLogProcessorType DefLogProcessor;

    context->log_processor =
      new LogProcessor(
        in_dir, new typename LogProcessor::LogVersionManagerT(
          context, out_dir, log_generalizer_stat_map_bundle_, pg_conn_factory,
            bundle_params),
        context->logger,
        proc_stat_impl_, fr_interrupter_);

    deferred_context->log_processor =
      new DefLogProcessor(
        deferred_context, def_in_dir, out_dir, pg_conn_factory, bundle_params,
          proc_stat_impl_, fr_interrupter_, log_generalizer_stat_map_bundle_);
  }
  else if (log_proc_params.get().upload_type() == UPLOAD_TYPE_POSTGRES_CSV)
  {
    typedef typename ProcTraits::DeferrableLogPgCsvProcessorType LogProcessor;
    typedef typename ProcTraits::DeferredLogPgCsvProcessorType
      DefLogProcessor;

    context->log_processor =
      new LogProcessor(
        in_dir, new typename LogProcessor::LogVersionManagerT(
          context, out_dir, log_generalizer_stat_map_bundle_, pg_conn_factory,
            bundle_params),
        context->logger,
        proc_stat_impl_, fr_interrupter_);

    deferred_context->log_processor =
      new DefLogProcessor(
        deferred_context, def_in_dir, out_dir, pg_conn_factory, bundle_params,
          proc_stat_impl_, fr_interrupter_, log_generalizer_stat_map_bundle_);
  }
  else
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Unknown or unsupported upload_type: " <<
      log_proc_params.get().upload_type();
    throw Exception(es);
  }

  Task_var
    move_deferred_logs_task(new MoveLogsTask(this, context, deferred_context));

  move_deferred_logs_task->deliver();
}

void
LogGeneralizerImpl::apply_log_proc_config_(
  const LogProcessingType &config,
  PostgresConnectionFactoryImpl_var& pg_conn_factory)
  /*throw(eh::Exception)*/
{
  unsigned num_threads = calc_required_number_of_threads(config);

  task_runner_ = new Generics::TaskRunner(
    callback_,
    num_threads,
    0, // stack_size
    0, // max_pending_tasks
    num_threads // start_threads, workaround for problem, that task runner don't
    // extend thread pool before activation
    );

  if (config.CCGUserStat().present())
  { // Depends on CCGUserStat
    init_log_proc_info_<CcgStatProcTraits>(config.CCGStat());
  }
  else if (config.CCGStat().present())
  {
    Stream::Error es;
    es << FNS << "CCGStat processing requires CCGUserStat";
    throw Exception(es);
  }

  if (config.CCUserStat().present())
  { // Depends on CCUserStat
    init_log_proc_info_<CcStatProcTraits>(config.CCStat());
  }
  else if (config.CCStat().present())
  {
    Stream::Error es;
    es << FNS << "CCStat processing requires CCUserStat";
    throw Exception(es);
  }

  if (config.CampaignStat().present())
  {
    if (!config.CampaignUserStat().present())
    {
      Stream::Error es;
      es << FNS << "CampaignStat processing requires CampaignUserStat";
      throw Exception(es);
    }
    init_log_proc_info_<CampaignUserStatProcTraits>(config.CampaignUserStat());
  }

  init_log_proc_info_<PageLoadsDailyStatProcTraits>(
    config.PageLoadsDailyStat());

  if (config.SiteStat().present())
  {
    if (!config.SiteUserStat().present())
    {
      Stream::Error es;
      es << FNS << "SiteStat processing requires SiteUserStat";
      throw Exception(es);
    }
    init_log_proc_info_<SiteUserStatProcTraits>(config.SiteUserStat());
  }

  init_log_proc_info_<TagAuctionStatProcTraits>(config.TagAuctionStat());

  if (db_enabled())
  {
    init_deferrable_log_proc_info<CmpStatProcTraits>(pg_conn_factory,
      config.CMPStat());

    init_deferrable_log_proc_info<CreativeStatProcTraits>(pg_conn_factory,
      config.CreativeStat());
  }
  else
  {
    init_log_proc_info_<CmpStatProcTraits>(config.CMPStat());

    init_log_proc_info_<CreativeStatProcTraits>(config.CreativeStat());
  }

  /// Write in CSV when db_enabled()
  init_log_proc_info_<RequestStatsHourlyExtStatProcTraits>(
    config.RequestStatsHourlyExtStat());

  init_log_proc_info_<ChannelOverlapUserStatProcTraits>(
    config.ChannelOverlapUserStat());

  init_log_proc_info_<ChannelPerformanceProcTraits>(
    config.ChannelPerformance());

  init_log_proc_info_<ChannelPriceRangeProcTraits>(
    config.ChannelPriceRange());

  init_log_proc_info_<ChannelTriggerImpStatProcTraits>(
    config.ChannelTriggerImpStat());

  init_log_proc_info_<ChannelTriggerStatProcTraits>(
    config.ChannelTriggerStat());

  init_log_proc_info_<DeviceChannelCountStatProcTraits>(
    config.DeviceChannelCountStat());

  init_log_proc_info_<SearchEngineStatProcTraits>(config.SearchEngineStat());

  init_log_proc_info_<TagPositionStatProcTraits>(config.TagPositionStat());

  init_log_proc_info_<UserAgentStatProcTraits>(config.UserAgentStat());

  init_hf_log_proc_info_<SearchTermStatProcTraits>(config.SearchTermStat());

  init_log_proc_info_<UserPropertiesProcTraits>(config.UserProperties());

  init_log_proc_info_<CcgSelectionFailureStatProcTraits>(
    config.CCGSelectionFailureStat());
  /// End write in CSV when db_enabled()

  if (config.ColoUserStat().present() && config.GlobalColoUserStat().present())
  { // Depends on ColoUserStat and GlobalColoUserStat
    init_log_proc_info_<ColoUsersProcTraits>(config.ColoUsers());
  }
  else if (config.ColoUsers().present())
  {
    Stream::Error es;
    es << FNS << "ColoUsers processing requires ColoUserStat and "
      "GlobalColoUserStat";
    throw Exception(es);
  }
}

} // namespace LogProcessing
} // namespace AdServer

