#ifndef REQUESTINFOSVCS_REQUESTOUTLOGGER_HPP
#define REQUESTINFOSVCS_REQUESTOUTLOGGER_HPP

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <eh/Exception.hpp>

#include <Logger/Logger.hpp>
#include <Generics/ActiveObject.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <LogCommons/LogHolder.hpp>
#include <Commons/LogReferrerUtils.hpp>

#include "PassbackContainer.hpp"
#include "RequestInfoContainer.hpp"
#include "UserCampaignReachContainer.hpp"
#include "UserSiteReachContainer.hpp"
#include "TagRequestGroupProcessor.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    class RequestOutLogger:
      public virtual RequestActionProcessor,
      public virtual RequestContainerProcessor,
      public virtual CampaignReachProcessor,
      public virtual SiteReachProcessor,
      public virtual PassbackProcessor,
      public virtual TagRequestProcessor,
      public virtual TagRequestGroupProcessor,
      public virtual AdvActionProcessor,
      public virtual UnmergedClickProcessor,
      public virtual LogProcessing::CompositeLogHolder
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      RequestOutLogger(
        Logging::Logger* logger,
        Generics::ActiveObjectCallback* callback,
        const LogProcessing::LogFlushTraits& creative_stat_flush,
        const LogProcessing::LogFlushTraits& user_properties_flush,
        const LogProcessing::LogFlushTraits& channel_performance_flush,
        const LogProcessing::LogFlushTraits& expression_performance_flush,
        const LogProcessing::LogFlushTraits& ccg_keyword_stat_flush,
        const LogProcessing::LogFlushTraits& cmp_stat_flush,
        const LogProcessing::LogFlushTraits& action_stat_flush,
        const LogProcessing::LogFlushTraits& ccg_user_stat_flush,
        const LogProcessing::LogFlushTraits& cc_user_stat_flush,
        const LogProcessing::LogFlushTraits& campaign_user_stat_flush,
        const LogProcessing::LogFlushTraits& passback_stat_flush,
        const LogProcessing::LogFlushTraits& channel_imp_inventory_flush,
        const LogProcessing::LogFlushTraits& site_stat_flush,
        const LogProcessing::LogFlushTraits& user_adv_stat_flush,
        const LogProcessing::LogFlushTraits& site_referer_stat_flush,
        const LogProcessing::LogFlushTraits& page_load_daily_stat_flush,
        const LogProcessing::LogFlushTraits& tag_position_stat_flush,
        const LogProcessing::LogFlushTraits& campaign_referrer_stat_stat_flush,
        const LogProcessing::LogFlushTraits* research_action_flush,
        const LogProcessing::LogFlushTraits* research_bid_flush,
        const LogProcessing::LogFlushTraits* research_impression_flush,
        const LogProcessing::LogFlushTraits* research_click_flush,
        const LogProcessing::LogFlushTraits* bid_cost_stat_flush,
        Commons::LogReferrer::Setting site_referrer_stats_log_referrer_setting,
        unsigned long colo_id)
        /*throw(Exception)*/;

      virtual Generics::Time flush_if_required(
        const Generics::Time& now = Generics::Time::get_time_of_day());

      // RequestActionProcessor
      virtual void
      process_request(
        const RequestInfo&,
        const ProcessingState&)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_impression(
        const RequestInfo&,
        const ImpressionInfo&,
        const ProcessingState&)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_click(
        const RequestInfo&,
        const ProcessingState&)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_action(const RequestInfo&)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_custom_action(
        const RequestInfo&, const AdvCustomActionInfo&)
        /*throw(RequestActionProcessor::Exception)*/;

      virtual void
      process_request_post_action(
        const RequestInfo&, const RequestPostActionInfo&)
        /*throw(RequestActionProcessor::Exception)*/;

      // RequestContainerProcessor
      virtual void
      process_request(const RequestInfo& request_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_impression(const ImpressionInfo& impression_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_action(
        ActionType action_type,
        const Generics::Time& time,
        const AdServer::Commons::RequestId& request_id)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_custom_action(
        const AdServer::Commons::RequestId& request_id,
        const AdvCustomActionInfo& adv_custom_action_info)
        /*throw(RequestContainerProcessor::Exception)*/;

      virtual void
      process_impression_post_action(
        const AdServer::Commons::RequestId&,
        const RequestPostActionInfo&)
        /*throw(RequestContainerProcessor::Exception)*/
      {}

      // CampaignReachProcessor
      virtual void
      process_reach(const ReachInfo&)
        /*throw(CampaignReachProcessor::Exception)*/;

      // SiteReachProcessor
      virtual void
      process_site_reach(const SiteReachInfo& reach_info)
        /*throw(SiteReachProcessor::Exception)*/;

      // PassbackProcessor
      virtual void
      process_passback(const PassbackInfo&)
        /*throw(PassbackProcessor::Exception)*/;

      // TagRequestProcessor
      virtual void
      process_tag_request(
        const TagRequestInfo& tag_request_info)
        /*throw(TagRequestProcessor::Exception)*/;

      // TagRequestGroupProcessor
      virtual void
      process_tag_request_group(
        const TagRequestGroupInfo& tag_request_group_info)
        /*throw(TagRequestGroupProcessor::Exception)*/;

      // AdvActionProcessor
      virtual void
      process_adv_action(
        const AdvActionInfo& adv_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

      virtual void
      process_custom_action(
        const AdvExActionInfo& adv_custom_action_info)
        /*throw(AdvActionProcessor::Exception)*/;

      // UnmergedClickProcessor
      virtual void
      process_click(const ClickInfo&)
        /*throw(UnmergedClickProcessor::Exception)*/;

    protected:
      typedef std::list<RequestActionProcessor_var>
        RequestActionProcessorList;
      typedef std::list<CampaignReachProcessor_var>
        CampaignReachProcessorList;
      typedef std::list<PassbackProcessor_var>
        PassbackProcessorList;
      typedef std::list<SiteReachProcessor_var>
        SiteReachProcessorList;

    protected:
      virtual ~RequestOutLogger() noexcept;

    private:
      template<typename LoggerType>
      void
      add_request_logger_(LoggerType* log_flush)
        /*throw(eh::Exception)*/;

      template<typename LoggerType>
      void
      add_reach_logger_(LoggerType* reach_logger)
        /*throw(eh::Exception)*/;

      template<typename LoggerType>
      void
      add_passback_logger_(LoggerType* passback_logger)
        /*throw(eh::Exception)*/;

      template<typename LoggerType>
      void
      add_site_reach_logger_(LoggerType* site_reach_logger)
        /*throw(eh::Exception)*/;

    private:
      Logging::Logger_var logger_;
      Generics::TaskRunner_var dump_task_runner_;

      RequestActionProcessorList request_loggers_;
      CampaignReachProcessorList reach_loggers_;
      PassbackProcessorList passback_loggers_;
      SiteReachProcessorList site_reach_loggers_;
      TagRequestProcessor_var site_referer_logger_;
      TagRequestGroupProcessor_var page_loads_daily_logger_;
      AdvActionProcessor_var research_action_logger_;
      RequestContainerProcessor_var research_bid_logger_;
      UnmergedClickProcessor_var research_click_logger_;
    };

    typedef ReferenceCounting::SmartPtr<RequestOutLogger>
      RequestOutLogger_var;
  }
}

#endif /*REQUESTINFOSVCS_REQUESTOUTLOGGER_HPP*/
