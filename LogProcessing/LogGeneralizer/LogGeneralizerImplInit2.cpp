#include "LogGeneralizerImpl.hpp"
#include "LogProcessorImpl.hpp"

namespace AdServer {
namespace LogProcessing {

typedef ProcTraits<WebStatProcessor2, WebStatProcessor> WebStatProcTraits;

typedef ProcTraits<ActionRequestProcessor2, ActionRequestProcessor>
  ActionRequestProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<AdvertiserUserStatExtTraits, LogVersionManager3<
    AdvertiserUserStatExtTraits> >, AdvertiserUserStatProcessor>
    AdvertiserUserStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<CcgKeywordStatExtTraits, LogVersionManager3<
    CcgKeywordStatExtTraits> >, CcgKeywordStatProcessor>
    CcgKeywordStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<CcgUserStatExtTraits, LogVersionManager3<
    CcgUserStatExtTraits> >, CcgUserStatProcessor>
    CcgUserStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<CcUserStatExtTraits, LogVersionManager3<
    CcUserStatExtTraits> >, CcUserStatProcessor>
    CcUserStatProcTraits;

typedef ProcTraits<ChannelCountStatProcessor2, ChannelCountStatProcessor>
  ChannelCountStatProcTraits;

typedef ProcTraits<ChannelHitStatProcessor2, ChannelHitStatProcessor>
  ChannelHitStatProcTraits;

typedef ProcTraits<ChannelImpInventoryProcessor2, ChannelImpInventoryProcessor>
  ChannelImpInventoryProcTraits;

typedef ProcTraits<ChannelInventoryProcessor2, ChannelInventoryProcessor>
  ChannelInventoryProcTraits;

typedef ProcTraits<ChannelInventoryEstimationStatProcessor2,
  ChannelInventoryEstimationStatProcessor>
  ChannelInventoryEstimationStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<ColoUpdateStatExtTraits, LogVersionManager3<
    ColoUpdateStatExtTraits> >, ColoUpdateStatProcessor>
    ColoUpdateStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<ColoUserStatExtTraits, LogVersionManager3<
    ColoUserStatExtTraits> >, ColoUserStatProcessor>
    ColoUserStatProcTraits;

typedef ProcTraits<ExpressionPerformanceProcessor2,
  ExpressionPerformanceProcessor> ExpressionPerformanceProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<GlobalColoUserStatExtTraits, LogVersionManager3<
    GlobalColoUserStatExtTraits> >, GlobalColoUserStatProcessor>
    GlobalColoUserStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<PassbackStatExtTraits, LogVersionManager3<
    PassbackStatExtTraits> >, PassbackStatProcessor>
    PassbackStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<SiteReferrerStatExtTraits, LogVersionManager3<
    SiteReferrerStatExtTraits> >, SiteReferrerStatProcessor>
    SiteReferrerStatProcTraits;

typedef ProcTraits<
  GenericLogProcessorImpl<ActionStatExtTraits, LogVersionManager3<
    ActionStatExtTraits> >, ActionStatProcessor>
    ActionStatProcTraits;


typedef ProcTraits<
  GenericLogProcessorImpl<CampaignReferrerStatExtTraits, LogVersionManager3<
    CampaignReferrerStatExtTraits> >, CampaignReferrerStatProcessor>
    CampaignReferrerStatProcTraits;

void
LogGeneralizerImpl::apply_log_proc_config_part2_(
  const LogProcessingType &config
)
  /*throw(eh::Exception)*/
{
  init_log_proc_info_<WebStatProcTraits>(config.WebStat());

  init_log_proc_info_<ActionRequestProcTraits>(config.ActionRequest());

  init_log_proc_info_<AdvertiserUserStatProcTraits>(
    config.AdvertiserUserStat());

  init_log_proc_info_<CcgKeywordStatProcTraits>(config.CCGKeywordStat());

  init_log_proc_info_<CcgUserStatProcTraits>(
    config.CCGUserStat()); // Must be initialized before CCGStat

  init_log_proc_info_<CcUserStatProcTraits>(
    config.CCUserStat()); // Must be initialized before CCStat

  init_log_proc_info_<ChannelCountStatProcTraits>(config.ChannelCountStat());

  init_log_proc_info_<ChannelHitStatProcTraits>(config.ChannelHitStat());

  init_log_proc_info_<ChannelImpInventoryProcTraits>(
    config.ChannelImpInventory());

  init_log_proc_info_<ChannelInventoryProcTraits>(config.ChannelInventory());

  init_log_proc_info_<ChannelInventoryEstimationStatProcTraits>(
    config.ChannelInventoryEstimationStat()
  );

  init_log_proc_info_<ColoUpdateStatProcTraits>(config.ColoUpdateStat());

  init_log_proc_info_<ColoUserStatProcTraits>(
    config.ColoUserStat()); // Must be initialized before ColoUsers

  init_log_proc_info_<ExpressionPerformanceProcTraits>(
    config.ExpressionPerformance());

  init_log_proc_info_<GlobalColoUserStatProcTraits>(
    config.GlobalColoUserStat()); // Must be initialized before ColoUsers

  init_log_proc_info_<PassbackStatProcTraits>(config.PassbackStat());

  init_log_proc_info_<SiteReferrerStatProcTraits>(config.SiteReferrerStat());

  init_log_proc_info_<ActionStatProcTraits>(config.ActionStat());

  init_log_proc_info_<CampaignReferrerStatProcTraits>(config.CampaignReferrerStat());
}

} // namespace LogProcessing
} // namespace AdServer

