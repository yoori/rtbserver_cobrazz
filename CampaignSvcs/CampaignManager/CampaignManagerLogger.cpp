#include <openssl/md5.h>

#include <algorithm>
#include <functional>
#include <memory>

#include <Generics/Rand.hpp>
#include <Generics/Decimal.hpp>
#include <Generics/BitAlgs.hpp>
#include <HTTP/UrlAddress.hpp>

#include <LogCommons/ActionRequest.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <LogCommons/CcgStat.hpp>
#include <LogCommons/ChannelHitStat.hpp>
#include <LogCommons/ChannelTriggerStat.hpp>
#include <LogCommons/CreativeStat.hpp>
#include <LogCommons/PassbackStat.hpp>
#include <LogCommons/Request.hpp>
#include <LogCommons/RequestBasicChannels.hpp>
#include <LogCommons/SearchTermStat.hpp>
#include <LogCommons/SearchEngineStat.hpp>
#include <LogCommons/TagAuctionStat.hpp>
#include <LogCommons/TagRequest.hpp>
#include <LogCommons/UserAgentStat.hpp>
#include <LogCommons/UserProperties.hpp>
#include <LogCommons/TagPositionStat.hpp>
#include <LogCommons/WebStat.hpp>
#include <LogCommons/ResearchWebStat.hpp>
#include <LogCommons/ResearchProfStat.hpp>

#include <LogCommons/LogHolder.hpp>

#include <Commons/Algs.hpp>

#include "CampaignManagerLogger.hpp"

/**
 * ChannelTriggerStatLogger
 *   process_request
 * ChannelHitStatLogger
 *   process_request
 * RequestBasicChannelsLogger
 *   process_request
 *   process_ad_request
 * CreativeStat - log only virtual creative selections and requests
 *   process_ad_selection
 * WebStatLogger
 * ResearchWebStatLogger
 * RequestLogger
 *   process_ad_selection
 * ImpressionLogger
 * ClickLogger
 * ActionLogger
 * PassbackImpressionLogger
 * UserPropertiesLogger - log only requests counter (other filled in RIM)
 * TagRequest
 * SearchTermStatLogger
 * TagAuctionStatLogger
 * UserAgentStatLogger
 * TagPositionStatLogger
 *
 * Only for lost_auctions filling:
 *   CCGStat
 *   CCStat
 */
namespace
{
  const char CHANNEL_TRIGGER_STAT_LOGGER[] = "ChannelTriggerStatLogger";
  const char CHANNEL_HIT_STAT_LOGGER[] = "ChannelHitStatLogger";
  const char WEB_STAT_LOGGER[] = "WebStatLogger";
  const char CREATIVE_STAT_LOGGER[] = "CreativeStatLogger";
  const char USER_PROPERTIES_LOGGER[] = "UserPropertiesLogger";

  const char REQUEST_BASIC_CHANNELS_LOGGER[] = "RequestBasicChannelsLogger";
  const char REQUEST_LOGGER[] = "RequestLogger";
  const char IMPRESSION_LOGGER[] = "ImpressionLogger";
  const char CLICK_LOGGER[] = "ClickLogger";

  const char ADVERTISER_ACTION_LOGGER[] = "AdvertiserActionLogger";
  const char ACTION_REQUEST_LOGGER[] = "ActionRequestLogger";

  const char PASSBACK_IMPRESSION_LOGGER[] = "PassbackImpressionLogger";

  const char TAG_REQUEST_LOGGER[] = "TagRequest";
  const char TAG_POSITION_STAT_LOGGER[] = "TagPositionStat";

  const char CCG_STAT_LOGGER[] = "CcgStat";
  const char CC_STAT_LOGGER[] = "CcStat";

  const char SEARCH_TERM_STAT_LOGGER[] = "SearchTermStat";
  const char TAG_AUCTION_STAT_LOGGER[] = "TagAuctionStat";
  const char USER_AGENT_STAT_LOGGER[] = "UserAgentStat";

  typedef Generics::SimpleDecimal<uint64_t, 11, 5> ExDeliveryThresholdDecimal;

  const unsigned long MAX_WEBSTAT_SOURCE_LENGTH = 10;
}

namespace PropertyNames
{
  const Generics::StringHashAdapter COUNTRY = "CountryCode";
  const Generics::StringHashAdapter CLIENT_APP = "Client";
  const Generics::StringHashAdapter CLIENT_APP_VERSION = "ClientVersion";
  const Generics::StringHashAdapter OS_VERSION = "OsVersion";
  const Generics::StringHashAdapter BROWSER_VERSION = "BrowserVersion";
}

namespace
{
  void fill_log_revenue(
    const AdServer::CampaignSvcs::
      CampaignManagerLogger::AdSelectionInfo::Revenue& revenue,
    AdServer::LogProcessing::RequestData::Revenue& result_revenue)
    noexcept
  {
    result_revenue.rate_id = revenue.rate_id;
    result_revenue.request_revenue = revenue.request;
    result_revenue.imp_revenue = revenue.impression;
    result_revenue.click_revenue = revenue.click;
    result_revenue.action_revenue = revenue.action;
  }

  char
  adapt_user_status(AdServer::CampaignSvcs::UserStatus user_status)
  {
    if(user_status == AdServer::CampaignSvcs::US_NOEXTERNALID)
    {
      return 'N';
    }
    else if(user_status == AdServer::CampaignSvcs::US_PROBE ||
      user_status == AdServer::CampaignSvcs::US_EXTERNALPROBE)
    {
      return 'P';
    }
    else if(user_status == AdServer::CampaignSvcs::US_OPTIN ||
      user_status == AdServer::CampaignSvcs::US_TEMPORARY)
    {
      return 'I';
    }
    else if(user_status == AdServer::CampaignSvcs::US_OPTOUT)
    {
      return 'O';
    }
    else if(user_status == AdServer::CampaignSvcs::US_FOREIGN)
    {
      return 'F';
    }
    else if(user_status == AdServer::CampaignSvcs::US_BLACKLISTED)
    {
      return 'B';
    }

    return 'U';
  }
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    class CMPChannelConv:
      public std::unary_function<
        CampaignManagerLogger::CMPChannel,
        LogProcessing::RequestData::CmpChannel>
    {
    public:
      result_type operator() (const argument_type& in) const noexcept
      {
        result_type ret;
        ret.channel_id = in.channel_id;
        ret.channel_rate_id = in.channel_rate_id;
        ret.imp_revenue = in.imp_revenue;
        ret.imp_sys_revenue = in.imp_sys_revenue;
        ret.adv_imp_revenue = in.adv_imp_revenue;
        ret.click_revenue = in.click_revenue;
        ret.click_sys_revenue = in.click_sys_revenue;
        ret.adv_click_revenue = in.adv_click_revenue;
        return ret;
      }
    };

    /** CampaignManagerLogger::ChannelTriggerStatLogger */
    class CampaignManagerLogger::ChannelTriggerStatLogger:
      public AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::ChannelTriggerStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      ChannelTriggerStatLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::ChannelTriggerStatTraits>(
              flush_traits)
      {}

      void process_request(
        const CampaignManagerLogger::RequestInfo& request_info)
        /*throw(Exception)*/;

      void process_match_request(
        const MatchRequestInfo& match_request_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~ChannelTriggerStatLogger() noexcept = default;

    private:
      void add_hits_(
        CollectorT::DataT& data,
        char type,
        const CampaignManagerLogger::TriggerChannelMap& triggers)
        /*throw(eh::Exception)*/;
    };

    /** CampaignManagerLogger::ChannelHitStatLogger */
    class CampaignManagerLogger::ChannelHitStatLogger:
      public virtual AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::ChannelHitStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      ChannelHitStatLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::ChannelHitStatTraits>(flush_traits)
      {};

      void process_request(
        const CampaignManagerLogger::RequestInfo& request_info)
        /*throw(Exception)*/;

      void process_match_request(
        const MatchRequestInfo& match_request_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~ChannelHitStatLogger() noexcept = default;

    private:
      typedef std::map<unsigned long, unsigned long> HitMap;

      class CalcHits
      {
      public:
        CalcHits(
          HitMap& map,
          unsigned long flag)
          noexcept :
          map_(map),
          flag_(flag){};

        void operator()(unsigned long id);

      private:
        HitMap& map_;
        unsigned long flag_;
      };
    };

    /** CampaignManagerLogger::UserPropertiesLogger */
    class CampaignManagerLogger::UserPropertiesLogger:
      public virtual AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::UserPropertiesTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      UserPropertiesLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::UserPropertiesTraits>(flush_traits)
      {};

      void
      process_request(
        const CampaignManagerLogger::RequestInfo& request_info)
        /*throw(Exception)*/;

      void
      process_ad_request(
        const CampaignManagerLogger::RequestInfo& ri,
        const CampaignManagerLogger::AdRequestSelectionInfo&)
        /*throw(Exception)*/;

      void
      process_anon_request(
        const CampaignManagerLogger::AnonymousRequestInfo& request_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~UserPropertiesLogger() noexcept = default;

    private:
      void
      add_record_(
        const CampaignManagerLogger::AnonymousRequestInfo& request_info,
        bool is_ad_request)
        /*throw(Exception)*/;
    };

    /** RequestBasicChannelsLogger */
    class CampaignManagerLogger::RequestBasicChannelsLogger:
      public virtual AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::RequestBasicChannelsTraits,
        AdServer::LogProcessing::DistributionSavePolicy<
          AdServer::LogProcessing::RequestBasicChannelsTraits> >
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      RequestBasicChannelsLogger(
        const RequestBasicChannelsFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::RequestBasicChannelsTraits,
            AdServer::LogProcessing::DistributionSavePolicy<
              AdServer::LogProcessing::RequestBasicChannelsTraits> >(
              flush_traits,
              AdServer::LogProcessing::DistributionSavePolicy<
                AdServer::LogProcessing::RequestBasicChannelsTraits>
                (flush_traits.distrib_count)),
          inventory_users_percentage_(
            flush_traits.inventory_users_percentage),
          dump_channel_triggers_(flush_traits.dump_channel_triggers),
          adrequest_anonymize_(flush_traits.adrequest_anonymize)
      {}

      void process_request(
        const CampaignManagerLogger::RequestInfo& request_info)
        /*throw(Exception)*/;

      void process_ad_request(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo&
          ad_request_selection_info)
        /*throw(Exception)*/;

      void
      process_oo_operation(
        unsigned long colo_id,
        const Generics::Time& time,
        const Generics::Time& time_offset,
        const AdServer::Commons::UserId& user_id)
        /*throw(Exception)*/;

      void
      process_match_request(
        const CampaignManagerLogger::MatchRequestInfo&
          match_request_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~RequestBasicChannelsLogger() noexcept = default;

    private:
      bool need_process_request_(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo*
          ad_request_selection_info) const
        noexcept;

      bool need_dump_channels_(
        const CampaignManagerLogger::RequestInfo& request_info) const
        noexcept;

      void add_record_(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo*
          ad_request_selection_info)
        /*throw(Exception)*/;

      static void convert_triggers_(
        CollectorT::DataT::DataT::TriggerMatchList& res_triggers,
        const CampaignManagerLogger::TriggerChannelMap& triggers)
        noexcept;

    private:
      unsigned long inventory_users_percentage_;
      bool dump_channel_triggers_;
      bool adrequest_anonymize_;
      AdServer::Commons::UserId null_id_;
    };

    /** CampaignManagerLogger::WebStatLogger */
    class CampaignManagerLogger::WebStatLogger:
      public AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::WebStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      WebStatLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::WebStatTraits>(
              flush_traits)
      {}

      void
      process_web_operation(
        const CampaignManagerLogger::WebOperationInfo& web_op)
        /*throw(Exception)*/;

    protected:
      virtual
      ~WebStatLogger() noexcept = default;
    };

    /** CampaignManagerLogger::ResearchWebStatLogger */
    class CampaignManagerLogger::ResearchWebStatLogger:
      public AdServer::LogProcessing::LogHolderPoolData<
        AdServer::LogProcessing::ResearchWebStatTraits,
        AdServer::LogProcessing::SimpleCsvSavePolicy<
          AdServer::LogProcessing::ResearchWebStatTraits> >
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      ResearchWebStatLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPoolData<
            AdServer::LogProcessing::ResearchWebStatTraits,
            AdServer::LogProcessing::SimpleCsvSavePolicy<
              AdServer::LogProcessing::ResearchWebStatTraits> >(
                flush_traits)
      {}

      void
      process_web_operation(
        const CampaignManagerLogger::WebOperationInfo& web_op)
        /*throw(Exception)*/;

    protected:
      virtual
      ~ResearchWebStatLogger() noexcept = default;
    };

    /** CampaignManagerLogger::CreativeStatLogger */
    class CampaignManagerLogger::CreativeStatLogger:
      public virtual AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::CreativeStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      CreativeStatLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(eh::Exception)*/;

      void process_ad_request(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo&
          ad_request_selection_info)
        /*throw(Exception)*/;

      void process_passback_track(
        const CampaignManagerLogger::PassbackTrackInfo& passback_track_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~CreativeStatLogger() noexcept = default;

    private:
      typedef CollectorT::DataT::KeyT::DeliveryThresholdT DeliveryThreshold;

    private:
      CollectorT::DataT::KeyT init_no_ad_key_(
        bool fraud,
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo&
          ad_request_selection_info,
        const CampaignManagerLogger::AdSelectionInfo& ad_info)
        /*throw(eh::Exception)*/;

      CollectorT::DataT::KeyT init_no_ad_key_(
        unsigned long colo_id,
        unsigned long tag_id,
        unsigned long size_id,
        const char* country_code,
        unsigned long isp_rate_id,
        unsigned long pub_rate_id,
        unsigned long currency_exchange_id,
        bool log_as_test,
        bool fraud,
        bool walled_garden,
        bool household_based,
        UserStatus user_status,
        unsigned long geo_channel_id,
        unsigned long last_platform_channel_id)
        /*throw(eh::Exception)*/;

    private:
      const DeliveryThreshold THRESHOLD_ONE_;
      const CollectorT::DataT::DataT STAT_REQUESTS_INCREMENT_;
      const CollectorT::DataT::DataT STAT_REQUESTS_DECREMENT_;
    };

    /** RequestLogger */
    class CampaignManagerLogger::RequestLogger:
      public virtual AdServer::LogProcessing::LogHolderPoolData<
        AdServer::LogProcessing::RequestTraits,
        AdServer::LogProcessing::DistributionSavePolicy<
          AdServer::LogProcessing::RequestTraits> > // log distribution enabled
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      RequestLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(eh::Exception)*/
        : AdServer::LogProcessing::LogHolderPoolData<
            AdServer::LogProcessing::RequestTraits,
            AdServer::LogProcessing::DistributionSavePolicy<
              AdServer::LogProcessing::RequestTraits> >(
                flush_traits)
      {}

      void process_ad_request(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo&
          ad_request_selection_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~RequestLogger() noexcept = default;
    };

    /** CampaignManagerLogger::ImpressionLogger */
    class CampaignManagerLogger::ImpressionLogger:
      public AdServer::LogProcessing::LogHolderPoolData<
        AdServer::LogProcessing::ImpressionTraits,
        AdServer::LogProcessing::DistributionSavePolicy<
          AdServer::LogProcessing::ImpressionTraits> > // log distribution enabled
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      ImpressionLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(eh::Exception)*/
        : AdServer::LogProcessing::LogHolderPoolData<
            AdServer::LogProcessing::ImpressionTraits,
            AdServer::LogProcessing::DistributionSavePolicy<
              AdServer::LogProcessing::ImpressionTraits> >(
                flush_traits)
      {}

      void
      process_impression(
        const CampaignManagerLogger::ImpressionInfo& request_id)
        /*throw(Exception)*/;

    protected:
      virtual
      ~ImpressionLogger() noexcept = default;
    };

    /** CampaignManagerLogger::ClickLogger */
    class CampaignManagerLogger::ClickLogger:
      public AdServer::LogProcessing::LogHolderPoolData<
        AdServer::LogProcessing::ClickTraits,
        AdServer::LogProcessing::DistributionSavePolicy<
          AdServer::LogProcessing::ClickTraits> > // log distribution enabled
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      ClickLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(eh::Exception)*/
        : AdServer::LogProcessing::LogHolderPoolData<
            AdServer::LogProcessing::ClickTraits,
            AdServer::LogProcessing::DistributionSavePolicy<
              AdServer::LogProcessing::ClickTraits> >(
              flush_traits)
      {}

      void
      process_click(
        const CampaignManagerLogger::ClickInfo& request_id)
        /*throw(Exception)*/;

    private:
      virtual
      ~ClickLogger() noexcept = default;
    };

    /** CampaignManagerLogger::AdvertiserActionLogger */
    class CampaignManagerLogger::AdvertiserActionLogger:
      public AdServer::LogProcessing::LogHolderPoolData<
        AdServer::LogProcessing::AdvertiserActionTraits,
        AdServer::LogProcessing::DistributionSavePolicy<
          AdServer::LogProcessing::AdvertiserActionTraits> > // log distribution enabled
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      AdvertiserActionLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(eh::Exception)*/
        : AdServer::LogProcessing::LogHolderPoolData<
            AdServer::LogProcessing::AdvertiserActionTraits,
            AdServer::LogProcessing::DistributionSavePolicy<
              AdServer::LogProcessing::AdvertiserActionTraits> >(
              flush_traits)
      {}

      void
      process_action(
        const CampaignManagerLogger::AdvActionInfo& request_id)
        /*throw(Exception)*/;

    protected:
      virtual
      ~AdvertiserActionLogger() noexcept = default;
    };

    /** CampaignManagerLogger::ActionRequestLogger */
    class CampaignManagerLogger::ActionRequestLogger:
      public AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::ActionRequestTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      ActionRequestLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(eh::Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::ActionRequestTraits>(
              flush_traits)
      {}

      void
      process_action(
        const CampaignManagerLogger::AdvActionInfo& request_id)
        /*throw(Exception)*/;

    protected:
      virtual
      ~ActionRequestLogger() noexcept = default;
    };

    /** CampaignManagerLogger::PassbackImpressionLogger */
    class CampaignManagerLogger::PassbackImpressionLogger:
      public AdServer::LogProcessing::LogHolderPoolData<
        AdServer::LogProcessing::PassbackImpressionTraits,
        AdServer::LogProcessing::DistributionSavePolicy<
          AdServer::LogProcessing::PassbackImpressionTraits> >
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      PassbackImpressionLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(eh::Exception)*/
        : AdServer::LogProcessing::LogHolderPoolData<
            AdServer::LogProcessing::PassbackImpressionTraits,
            AdServer::LogProcessing::DistributionSavePolicy<
              AdServer::LogProcessing::PassbackImpressionTraits> >(
              flush_traits)
      {}

      void process_passback(
        const CampaignManagerLogger::PassbackInfo& passback_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~PassbackImpressionLogger() noexcept = default;
    };

    /** CampaignManagerLogger::PassbackStatLogger */
    class CampaignManagerLogger::PassbackStatLogger:
      public AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::PassbackStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      PassbackStatLogger(const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(eh::Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::PassbackStatTraits>(
              flush_traits)
      {}

      void process_passback_track(
        const PassbackTrackInfo& passback_track_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~PassbackStatLogger() noexcept = default;
    };

    /** TagRequestLogger */
    class CampaignManagerLogger::TagRequestLogger:
      public virtual AdServer::LogProcessing::LogHolderPoolData<
        AdServer::LogProcessing::TagRequestTraits,
        AdServer::LogProcessing::DistributionSavePolicy<
          AdServer::LogProcessing::TagRequestTraits> >
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      TagRequestLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPoolData<
            AdServer::LogProcessing::TagRequestTraits,
            AdServer::LogProcessing::DistributionSavePolicy<
              AdServer::LogProcessing::TagRequestTraits> >(flush_traits),
          EMPTY_REFERER_("-")
      {}

      void
      process_request(
        const CampaignManagerLogger::RequestInfo& request_info)
        /*throw(Exception)*/;

      void
      process_ad_request(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo&
          ad_request_selection_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~TagRequestLogger() noexcept = default;

    private:
      const std::string EMPTY_REFERER_;
    };

    /** TagPositionStatLogger */
    class CampaignManagerLogger::TagPositionStatLogger:
      public virtual AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::TagPositionStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      TagPositionStatLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::TagPositionStatTraits>(flush_traits),
          ONE_REQUEST_(1, 0, 0)
      {}

      void
      process_ad_request(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo&
          ad_request_selection_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~TagPositionStatLogger() noexcept = default;

    private:
      const CollectorT::DataT::DataT ONE_REQUEST_;
    };

    /** CcgStatLogger */
    class CampaignManagerLogger::CcgStatLogger:
      public virtual AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::CcgStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      CcgStatLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::CcgStatTraits>(
              flush_traits),
          STAT_LOST_AUCTION_ONE_(1)
      {}

      void
      process_ad_request(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo&
          ad_request_selection_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~CcgStatLogger() noexcept = default;

    private:
      const CollectorT::DataT::DataT STAT_LOST_AUCTION_ONE_;
    };

    /** CcStatLogger */
    class CampaignManagerLogger::CcStatLogger:
      public virtual AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::CcStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      CcStatLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::CcStatTraits>(
              flush_traits),
          STAT_LOST_AUCTION_ONE_(1)
      {}

      void
      process_ad_request(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo&
          ad_request_selection_info)
        /*throw(Exception)*/;

    protected:
      virtual
      ~CcStatLogger() noexcept = default;

    private:
      const CollectorT::DataT::DataT STAT_LOST_AUCTION_ONE_;
    };

    /** SearchTermStatLogger */
    class CampaignManagerLogger::SearchTermStatLogger:
      public AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::SearchTermStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      SearchTermStatLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::SearchTermStatTraits>(
              flush_traits),
          STAT_HITS_ONE_(1)
      {}

      void
      process_ad_request(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo&)
        /*throw(Exception)*/;

      void
      process_request(
        const CampaignManagerLogger::RequestInfo& request_info)
        /*throw(Exception)*/;

      void
      process_anon_request(
        const CampaignManagerLogger::AnonymousRequestInfo& request_info)
        /*throw(Exception)*/;

    private:
      virtual
      ~SearchTermStatLogger() noexcept = default;

      void
      add_record_(
        const CampaignManagerLogger::AnonymousRequestInfo& request_info)
        /*throw(Exception)*/;

      const CollectorT::DataT::DataT STAT_HITS_ONE_;
    };

    /** SearchEngineStatLogger */
    class CampaignManagerLogger::SearchEngineStatLogger:
      public AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::SearchEngineStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      SearchEngineStatLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::SearchEngineStatTraits>(
              flush_traits),
          STAT_HITS_ONE_(1, 0),
          STAT_EMPTY_PAGE_HITS_ONE_(1, 1)
      {}

      void
      process_ad_request(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo&)
        /*throw(Exception)*/;

      void
      process_request(
        const CampaignManagerLogger::RequestInfo& request_info)
        /*throw(Exception)*/;

      void
      process_anon_request(
        const CampaignManagerLogger::AnonymousRequestInfo& request_info)
        /*throw(Exception)*/;

    private:
      virtual
      ~SearchEngineStatLogger() noexcept = default;

      void
      add_record_(
        const CampaignManagerLogger::AnonymousRequestInfo& request_info)
        /*throw(Exception)*/;

      const CollectorT::DataT::DataT STAT_HITS_ONE_;
      const CollectorT::DataT::DataT STAT_EMPTY_PAGE_HITS_ONE_;
    };

    /** TagAuctionStatLogger */
    class CampaignManagerLogger::TagAuctionStatLogger:
      public virtual AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::TagAuctionStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      TagAuctionStatLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::TagAuctionStatTraits>(
              flush_traits),
          STAT_REQUEST_ONE_(1)
      {}

      void
      process_ad_request(
        const CampaignManagerLogger::RequestInfo& request_info,
        const CampaignManagerLogger::AdRequestSelectionInfo& ad_ri)
        /*throw(Exception)*/
      {
        if(!request_info.log_as_test && ad_ri.auction_type != AT_RANDOM)
        {
          CollectorT::KeyT key(ad_ri.pub_time, request_info.colo_id);
          CollectorT::DataT data;

          size_t winners = 0;
          if (!ad_ri.ad_selection_info_list.empty() &&
              !((ad_ri.ad_selection_info_list.size() == 1) &&
                (ad_ri.ad_selection_info_list.front().ad_selected == false)))
          {
            winners = ad_ri.ad_selection_info_list.size();
          }

          size_t lost = ad_ri.lost_auction_ccgs.size();

          size_t account_ccg_count = 0;
          // highest_bit_32(0) == 32
          if ((winners + lost) != 0)
          {
            account_ccg_count = 1 << Generics::BitAlgs::highest_bit_32(winners + lost);
          }


          CollectorT::DataT::KeyT inner_key(
            ad_ri.request_tag_id,
            account_ccg_count);

          data.add(inner_key, STAT_REQUEST_ONE_);
          add_record(key, data);
        }
      }

    protected:
      virtual
      ~TagAuctionStatLogger() noexcept = default;

    private:
      const CollectorT::DataT::DataT STAT_REQUEST_ONE_;
    };

    /** UserAgentStatLogger */
    class CampaignManagerLogger::UserAgentStatLogger:
      public virtual AdServer::LogProcessing::LogHolderPool<
        AdServer::LogProcessing::UserAgentStatTraits>
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      UserAgentStatLogger(
        const AdServer::LogProcessing::LogFlushTraits& flush_traits)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderPool<
            AdServer::LogProcessing::UserAgentStatTraits>(
              flush_traits)
      {}

      void
      process_request(
        const CampaignManagerLogger::RequestInfo& request_info)
        /*throw(Exception)*/
      {
        add_record_(request_info);
      }

      void
      process_anon_request(
        const CampaignManagerLogger::AnonymousRequestInfo& request_info)
        /*throw(Exception)*/
      {
        add_record_(request_info);
      }

    protected:
      virtual
      ~UserAgentStatLogger() noexcept = default;

      void
      add_record_(
        const CampaignManagerLogger::AnonymousRequestInfo& request_info)
        /*throw(Exception)*/
      {
        if(!request_info.log_as_test &&
           request_info.user_agent.in() &&
           request_info.user_status != US_FOREIGN)
        {
          CollectorT::DataT data;
          data.add(
            CollectorT::DataT::KeyT(request_info.user_agent),
            CollectorT::DataT::DataT(
              1,
              request_info.platform_channels.begin(),
              request_info.platform_channels.end(),
              request_info.platforms.begin(),
              request_info.platforms.end()));
          add_record(CollectorT::KeyT(request_info.time), data);
        }
      }
    };

    namespace
    {
      void
      user_id_hash(std::string& dst, const AdServer::Commons::UserId& uid_src)
      {
        if (!uid_src.is_null())
        {
          dst = uid_src.to_string();
          /*
          const std::string uid_str = uid_src.to_string();
          unsigned char hash[MD5_DIGEST_LENGTH];
          ::memset(hash, 0, MD5_DIGEST_LENGTH);
          MD5(reinterpret_cast<const unsigned char*>(
            uid_str.data()), uid_str.size(), hash);
          String::StringManip::base64_encode(dst, hash, sizeof(hash), false);
          */
        }
      }
    }

    /**  ProfilingResearchLogger */
    class CampaignManagerLogger::ProfilingResearchLogger:
      public virtual AdServer::LogProcessing::LogHolderLimitedDataAdd<
        AdServer::LogProcessing::ResearchProfTraits,
        AdServer::LogProcessing::SimpleCsvSavePolicy<
          AdServer::LogProcessing::ResearchProfTraits>>,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, CampaignManagerLogger::Exception);

      ProfilingResearchLogger(
        Logging::Logger* logger,
        size_t limit,
        float sampling,
        const AdServer::LogProcessing::LogFlushTraits& flush_traits,
        Commons::LogReferrer::Setting log_referrer_setting)
        /*throw(Exception)*/
        : AdServer::LogProcessing::LogHolderLimitedDataAdd<
            AdServer::LogProcessing::ResearchProfTraits,
            AdServer::LogProcessing::SimpleCsvSavePolicy<
              AdServer::LogProcessing::ResearchProfTraits>>
              (logger, limit, flush_traits),
          sampling_(static_cast<unsigned long>(
              sampling * CampaignSvcs::RANDOM_PARAM_MAX / 100.f + 0.5f)),
          log_referrer_setting_(log_referrer_setting)
      {}

      void
      process_request(
        const CampaignManagerLogger::RequestInfo& request_info)
        /*throw(Exception)*/
      {
        static const char* FUN = "ProfilingResearchLogger::process_request()";
        if (request_info.log_as_test || request_info.random >= sampling_)
        {
          return;
        }

        try
        {
          CollectorT::DataT data;
          data.time = request_info.time;
          data.app = request_info.client_app;
          data.colo_id = request_info.colo_id;
          user_id_hash(data.hid_hash, request_info.household_id);
          data.ip_addr_hash = request_info.ip_hash;
          data.referer = Commons::LogReferrer::normalize_referrer(
            request_info.referer, log_referrer_setting_, "");
          // check next
          data.device_channel_id = request_info.last_platform_channel_id;

          if(request_info.user_status == US_TEMPORARY)
          {
            // data.uid_hash = "";
            user_id_hash(data.tuid_hash, request_info.user_id);
          }
          else
          {
            user_id_hash(data.uid_hash, request_info.user_id);
            user_id_hash(data.tuid_hash, request_info.merged_user_id);
          }

          data.page_keywords = request_info.page_keywords;
          data.search_keywords = request_info.search_words;
          data.url_keywords = request_info.url_keywords;

          // In Rprof channels is triggered_channels, not history. See: ADSC-9874
          {
            ChannelIdSet tmp; // garantee unique final list.

            std::copy(
              request_info.triggered_channels.url_channels.begin(),
              request_info.triggered_channels.url_channels.end(),
              std::inserter(
                tmp,
                tmp.begin()));

            std::copy(
              request_info.triggered_channels.page_channels.begin(),
              request_info.triggered_channels.page_channels.end(),
              std::inserter(
                tmp,
                tmp.begin()));

            std::copy(
              request_info.triggered_channels.search_channels.begin(),
              request_info.triggered_channels.search_channels.end(),
              std::inserter(
                tmp,
                tmp.begin()));

            std::copy(
              request_info.triggered_channels.url_keyword_channels.begin(),
              request_info.triggered_channels.url_keyword_channels.end(),
              std::inserter(
                tmp,
                tmp.begin()));

            std::copy(
              request_info.triggered_channels.uid_channels.begin(),
              request_info.triggered_channels.uid_channels.end(),
              std::inserter(
                tmp,
                tmp.begin()));

            std::copy(
              tmp.begin(),
              tmp.end(),
              std::inserter(
                data.channel_list,
                data.channel_list.begin()));
          }

          add_record(data);
        }
        catch (const eh::Exception &ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << ex.what();
          throw Exception(ostr);
        }
      }

    protected:
      unsigned long sampling_;
      const Commons::LogReferrer::Setting log_referrer_setting_;
      virtual
      ~ProfilingResearchLogger() noexcept = default;
    };


    /** ChannelTriggerStatLogger implementation */
    void
    CampaignManagerLogger::ChannelTriggerStatLogger::add_hits_(
      CollectorT::DataT& data,
      char type,
      const CampaignManagerLogger::TriggerChannelMap& triggers)
      /*throw(eh::Exception)*/
    {
      CollectorT::DataT::DataT inner_data(1);

      data.prepare_adding(triggers.size());

      for(CampaignManagerLogger::TriggerChannelMap::
            const_iterator tr_it = triggers.begin();
          tr_it != triggers.end(); ++tr_it)
      {
        data.add(
          CollectorT::DataT::KeyT(tr_it->first, tr_it->second, type),
          inner_data);
      }
    }

    void
    CampaignManagerLogger::ChannelTriggerStatLogger::
    process_request(
      const CampaignManagerLogger::RequestInfo& request_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "ChannelTriggerStatLogger::process_request()";

      if(!request_info.log_as_test &&
           /* (request_info.user_status == US_OPTIN ||
             request_info.user_status == US_TEMPORARY ||
             request_info.user_status == US_BLACKLISTED) && */ (
           !request_info.url_triggers.empty() ||
           !request_info.discover_keyword_url_triggers.empty() ||
           !request_info.page_triggers.empty() ||
           !request_info.discover_keyword_page_triggers.empty() ||
           !request_info.search_triggers.empty() ||
           !request_info.discover_keyword_search_triggers.empty() ||
           !request_info.url_keyword_triggers.empty() ||
           !request_info.discover_keyword_url_keyword_triggers.empty()))
      {
        try
        {
          CollectorT::DataT data;

          add_hits_(data, 'U', request_info.url_triggers);
          add_hits_(data, 'U', request_info.discover_keyword_url_triggers);
          add_hits_(data, 'P', request_info.page_triggers);
          add_hits_(data, 'P', request_info.discover_keyword_page_triggers);
          add_hits_(data, 'S', request_info.search_triggers);
          add_hits_(data, 'S', request_info.discover_keyword_search_triggers);
          add_hits_(data, 'R', request_info.url_keyword_triggers);
          add_hits_(data, 'R', request_info.discover_keyword_url_keyword_triggers);

          add_record(
            CollectorT::KeyT(
              request_info.isp_time,
              request_info.colo_id),
            data);
        }
        catch (const eh::Exception &ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << ex.what();
          throw Exception(ostr);
        }
      }
    }

    void
    CampaignManagerLogger::ChannelTriggerStatLogger::
    process_match_request(
      const MatchRequestInfo& match_request_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "ChannelTriggerStatLogger::process_match_request()";

      try
      {
        CollectorT::DataT data;
        add_hits_(data, 'P', match_request_info.match_info.page_triggers);

        add_record(
          CollectorT::KeyT(
            match_request_info.time + match_request_info.isp_offset,
            match_request_info.match_info.colo_id),
          data);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }

    /** ChannelHitStatLogger implementation */
    void CampaignManagerLogger::ChannelHitStatLogger::CalcHits::operator()(
      unsigned long id)
    {
      HitMap::iterator it = map_.find(id);
      if(it == map_.end())
      {
        map_[id] = flag_;
      }
      else
      {
        it->second |= flag_;
      }
    }

    void
    CampaignManagerLogger::ChannelHitStatLogger::
    process_match_request(
      const MatchRequestInfo& match_request_info)
      /*throw(Exception)*/
    {
      // static const char* FUN = "ChannelHitStatLogger::process_match_request()";

      CollectorT::DataT data;

      data.prepare_adding(match_request_info.match_info.triggered_page_channels.size());

      for (auto ch_it = match_request_info.match_info.triggered_page_channels.begin();
        ch_it != match_request_info.match_info.triggered_page_channels.end(); ++ch_it)
      {
        data.add(
          CollectorT::DataT::KeyT(*ch_it),
          CollectorT::DataT::DataT(
            1, // hits
            0, // hits_urls
            1, // hits_kws
            0, // hits_search_kws
            0 // hits_url_kws
            ));
      }

      add_record(
        CollectorT::KeyT(
          match_request_info.time + match_request_info.isp_offset,
          match_request_info.match_info.colo_id),
        data);
    }

    void
    CampaignManagerLogger::ChannelHitStatLogger::
    process_request(
      const CampaignManagerLogger::RequestInfo& request_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "ChannelHitStatLogger::process_request()";

      if(request_info.log_as_test || (
           request_info.user_status != US_OPTIN &&
           request_info.user_status != US_TEMPORARY &&
           request_info.user_status != US_BLACKLISTED))
      {
        return;
      }

      try
      {
        enum HitFlags
        {
          HF_URL = 1,
          HF_PKW = 2,
          HF_UKW = 4,
          HF_SKW = 8
        };

        HitMap hits;

        std::for_each(
          request_info.triggered_channels.url_channels.begin(),
          request_info.triggered_channels.url_channels.end(),
          CalcHits(hits, HF_URL));
        std::for_each(
          request_info.triggered_channels.page_channels.begin(),
          request_info.triggered_channels.page_channels.end(),
          CalcHits(hits, HF_PKW));
        std::for_each(
          request_info.triggered_channels.search_channels.begin(),
          request_info.triggered_channels.search_channels.end(),
          CalcHits(hits, HF_SKW));
        std::for_each(
          request_info.triggered_channels.url_keyword_channels.begin(),
          request_info.triggered_channels.url_keyword_channels.end(),
          CalcHits(hits, HF_UKW));


        if(!hits.empty())
        {
          CollectorT::DataT data;
          data.prepare_adding(hits.size());

          for(HitMap::const_iterator it = hits.begin(); it != hits.end(); ++it)
          {
            data.add(
              CollectorT::DataT::KeyT(it->first),
              CollectorT::DataT::DataT(
                1,
                (it->second & HF_URL ? 1 : 0),
                (it->second & HF_PKW ? 1 : 0),
                (it->second & HF_SKW ? 1 : 0),
                (it->second & HF_UKW ? 1 : 0)));
          }

          add_record(
            CollectorT::KeyT(request_info.isp_time, request_info.colo_id),
            data);
        }
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    // UserPropertiesLogger impl
    void
    CampaignManagerLogger::UserPropertiesLogger::
    add_record_(
      const CampaignManagerLogger::AnonymousRequestInfo& request_info,
      bool is_ad_request)
      /*throw(Exception)*/
    {
      static const char* FUN = "UserPropertiesLogger::process_request()";

      if(request_info.log_as_test)
      {
        return;
      }

      try
      {
        CollectorT::DataT data(
          !is_ad_request ? 1 : 0 /* profiling_requests (tag not defined) */,
          is_ad_request ? 1 : 0 /* requests (defined tag) */,
          0, 0, 0, 0);

        char user_status = adapt_user_status(request_info.user_status);

        PoolObject_var pool_object = get_object();

        pool_object->add_record(
          CollectorT::KeyT(
            request_info.time,
            request_info.isp_time,
            request_info.colo_id,
            user_status,
            PropertyNames::COUNTRY,
            request_info.country_code),
          data);

        pool_object->add_record(
          CollectorT::KeyT(
            request_info.time,
            request_info.isp_time,
            request_info.colo_id,
            user_status,
            PropertyNames::CLIENT_APP_VERSION,
            request_info.client_app_version),
          data);

        pool_object->add_record(
          CollectorT::KeyT(
            request_info.time,
            request_info.isp_time,
            request_info.colo_id,
            user_status,
            PropertyNames::CLIENT_APP,
            request_info.client_app),
          data);

        pool_object->add_record(
          CollectorT::KeyT(
            request_info.time,
            request_info.isp_time,
            request_info.colo_id,
            user_status,
            PropertyNames::OS_VERSION,
            request_info.full_platform),
          data);

        pool_object->add_record(
          CollectorT::KeyT(
            request_info.time,
            request_info.isp_time,
            request_info.colo_id,
            user_status,
            PropertyNames::BROWSER_VERSION,
            request_info.web_browser),
          data);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerLogger::UserPropertiesLogger::
    process_request(
      const CampaignManagerLogger::RequestInfo& ri)
      /*throw(Exception)*/
    {
      add_record_(ri, ri.is_ad_request);
    }

    void
    CampaignManagerLogger::UserPropertiesLogger::
    process_ad_request(
      const CampaignManagerLogger::RequestInfo& ri,
      const CampaignManagerLogger::AdRequestSelectionInfo&)
      /*throw(Exception)*/
    {
      add_record_(ri, ri.is_ad_request);
    }

    void
    CampaignManagerLogger::UserPropertiesLogger::
    process_anon_request(
      const CampaignManagerLogger::AnonymousRequestInfo& ri)
      /*throw(Exception)*/
    {
      add_record_(ri, false);
    }

    /** RequestBasicChannelsLogger implementation */
    void
    CampaignManagerLogger::RequestBasicChannelsLogger::
    process_request(
      const CampaignManagerLogger::RequestInfo& request_info)
      /*throw(Exception)*/
    {
      if(need_process_request_(request_info, 0))
      {
        add_record_(request_info, 0);
      }
    }

    void
    CampaignManagerLogger::RequestBasicChannelsLogger::
    process_oo_operation(
      unsigned long colo_id,
      const Generics::Time& time,
      const Generics::Time& time_offset,
      const AdServer::Commons::UserId& user_id)
      /*throw(Exception)*/
    {
      static const char* FUN = "RequestBasicChannelsLogger::process_oo_operation()";

      try
      {
         const CollectorT::KeyT key(
          time,
          time + time_offset,
          colo_id);

        const CollectorT::DataT::DataT inner_data(
          'A',
          user_id,
          AdServer::Commons::UserId(), // temporary user id
          CollectorT::DataT::DataT::MatchOptional(),
          CollectorT::DataT::DataT::AdRequestPropsOptional());

        CollectorT::DataT data;
        data.add(inner_data);

        add_record(key, data);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerLogger::RequestBasicChannelsLogger::
    process_ad_request(
      const CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerLogger::AdRequestSelectionInfo&
        ad_request_selection_info)
      /*throw(Exception)*/
    {
      if(need_process_request_(request_info, &ad_request_selection_info))
      {
        add_record_(request_info, &ad_request_selection_info);
      }
    }

    void
    CampaignManagerLogger::RequestBasicChannelsLogger::
    process_match_request(
      const CampaignManagerLogger::MatchRequestInfo& match_request_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "RequestBasicChannelsLogger::process_match_request()";

      try
      {
        const CollectorT::KeyT key(
          match_request_info.time,
          match_request_info.time + match_request_info.isp_offset,
          match_request_info.match_info.colo_id);

        AdServer::LogProcessing::NumberArray history_channels;

        history_channels.insert(
          history_channels.end(),
          match_request_info.match_info.channels.begin(),
          match_request_info.match_info.channels.end());

        CollectorT::DataT::DataT::TriggerMatchList page_trigger_channels;

        if(dump_channel_triggers_)
        {
          convert_triggers_(page_trigger_channels, match_request_info.match_info.page_triggers);
        }

        CollectorT::DataT::DataT::Match match_request(
          history_channels,
          page_trigger_channels, // page trigger channels
          CollectorT::DataT::DataT::TriggerMatchList(), // search trigger channels
          CollectorT::DataT::DataT::TriggerMatchList(), // url trigger channels
          CollectorT::DataT::DataT::TriggerMatchList() // url keyword trigger channels
          );

        const CollectorT::DataT::DataT inner_data(
          'A',
          match_request_info.user_id,
          AdServer::Commons::UserId(), // temporary user id
          match_request,
          CollectorT::DataT::DataT::AdRequestPropsOptional());

        CollectorT::DataT data;
        data.add(inner_data);

        add_record(key, data);

        /*
           FIXME: add logging for
             match_request_info.household_id,
             match_request_info.match_info.hid_channels
         */
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    bool
    CampaignManagerLogger::RequestBasicChannelsLogger::need_process_request_(
      const CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerLogger::AdRequestSelectionInfo*
        ad_request_selection_info)
      const
      noexcept
    {
      return !request_info.log_as_test &&
        (ad_request_selection_info ||
         request_info.user_status == US_OPTIN ||
         request_info.user_status == US_BLACKLISTED ||
         (request_info.user_status == US_TEMPORARY && dump_channel_triggers_));
    }

    bool
    CampaignManagerLogger::RequestBasicChannelsLogger::need_dump_channels_(
      const CampaignManagerLogger::RequestInfo& request_info)
      const
      noexcept
    {
      return (!request_info.user_id.is_null() ?
        request_info.user_id.hash() % 100 < inventory_users_percentage_ :
        request_info.request_id.hash() % 100 < inventory_users_percentage_) &&
        !adrequest_anonymize_
        ;
    }

    void
    CampaignManagerLogger::RequestBasicChannelsLogger::
    convert_triggers_(
      CollectorT::DataT::DataT::TriggerMatchList& res_triggers,
      const CampaignManagerLogger::TriggerChannelMap& triggers)
      noexcept
    {
      for (auto tr_it = triggers.begin(); tr_it != triggers.end(); ++tr_it)
      {
        res_triggers.emplace_back(
          tr_it->second, // channel_id
          tr_it->first  // channel_trigger_id
          );
      }
    }

    void
    CampaignManagerLogger::RequestBasicChannelsLogger::
    add_record_(
      const CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerLogger::AdRequestSelectionInfo* ad_selection_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "RequestBasicChannelsLogger::add_record_()";

      try
      {
        CollectorT::KeyT key(
          request_info.time,
          request_info.isp_time,
          request_info.colo_id);

        CollectorT::DataT data;

        if(!request_info.household_id.is_null())
        {
          // household record
          CollectorT::DataT::DataT::Match match_request(
            AdServer::LogProcessing::NumberArray(
              request_info.hid_history_channels.begin(),
              request_info.hid_history_channels.end()),
            CollectorT::DataT::DataT::TriggerMatchList(), // page trigger channels
            CollectorT::DataT::DataT::TriggerMatchList(), // search trigger channels
            CollectorT::DataT::DataT::TriggerMatchList(), // url trigger channels
            CollectorT::DataT::DataT::TriggerMatchList() // url keyword trigger channels
          );

          data.add(
            CollectorT::DataT::DataT(
              'H',
              request_info.household_id,
              null_id_, // temporary user id
              match_request,
              CollectorT::DataT::DataT::AdRequestPropsOptional()));
        }

        CollectorT::DataT::DataT::AdRequestPropsOptional ad_request_opt;

        if(ad_selection_info)
        {
          CollectorT::DataT::DataT::AdSlotImpressionOptional display_ad_shown;
          CollectorT::DataT::DataT::AdBidSlotImpressionList text_ad_shown;

          RevenueDecimal cost_threshold;

          if(ad_selection_info->text_campaigns)
          {
            for(AdSelectionInfoList::const_iterator ad_it =
                  ad_selection_info->ad_selection_info_list.begin();
                ad_it != ad_selection_info->ad_selection_info_list.end();
                ++ad_it)
            {
              RevenueDecimal revenue = RevenueDecimal::div(
                ad_it->ecpm_bid,
                RevenueDecimal(false, 100000, 0));
              RevenueDecimal max_revenue_bid = RevenueDecimal::div(
                ad_it->ecpm,
                RevenueDecimal(false, 100000, 0));
              text_ad_shown.push_back(CollectorT::DataT::DataT::AdBidSlotImpression(
                revenue,
                max_revenue_bid,
                ad_it->channels));
            }

            cost_threshold = RevenueDecimal::div(
              RevenueDecimal(false, ad_selection_info->min_text_ecpm, 0),
              RevenueDecimal(false, 100000, 0));
          }
          else
          {
            if(!ad_selection_info->ad_selection_info_list.empty() &&
              ad_selection_info->ad_selection_info_list.front().ad_selected)
            {
              const AdSelectionInfo& display_ad_info =
                *(ad_selection_info->ad_selection_info_list.begin());
              RevenueDecimal revenue = RevenueDecimal::div(
                display_ad_info.ecpm,
                RevenueDecimal(false, 100000, 0));
              display_ad_shown = CollectorT::DataT::DataT::AdSlotImpression(
                revenue,
                display_ad_info.channels);
            }

            cost_threshold = RevenueDecimal::div(
              RevenueDecimal(false, ad_selection_info->min_no_adv_ecpm, 0),
              RevenueDecimal(false, 100000, 0));
          }

          ad_request_opt = CollectorT::DataT::DataT::AdRequestProps(
            LogProcessing::StringList(
              ad_selection_info->tag_sizes.begin(),
              ad_selection_info->tag_sizes.end()), // ad_request_sizes
            request_info.country_code,
            ad_selection_info->max_ads,
            cost_threshold,
            std::move(display_ad_shown),
            std::move(text_ad_shown),
            CollectorT::DataT::DataT::AdSelectPropsOptional(), /*FIXME: Add AdSelectProps*/
            ad_selection_info->auction_type
            );
        }

        AdServer::Commons::UserId user_id;
        AdServer::Commons::UserId temporary_user_id;
        if(request_info.user_status == US_TEMPORARY)
        {
          temporary_user_id = request_info.user_id;
        }
        else
        {
          user_id = request_info.user_id;
          temporary_user_id = request_info.merged_user_id;
        }

        CollectorT::DataT::DataT::TriggerMatchList page_trigger_channels;
        CollectorT::DataT::DataT::TriggerMatchList search_trigger_channels;
        CollectorT::DataT::DataT::TriggerMatchList url_trigger_channels;
        CollectorT::DataT::DataT::TriggerMatchList url_keyword_trigger_channels;

        if((ad_selection_info == 0 || !adrequest_anonymize_) &&
           dump_channel_triggers_ && (
             !user_id.is_null() || !temporary_user_id.is_null()))
        {
          convert_triggers_(page_trigger_channels, request_info.page_triggers);
          convert_triggers_(search_trigger_channels, request_info.search_triggers);
          convert_triggers_(url_trigger_channels, request_info.url_triggers);
          convert_triggers_(
            url_keyword_trigger_channels, request_info.url_keyword_triggers);
        }

        CollectorT::DataT::DataT::Match match_request(
          request_info.user_status != US_TEMPORARY && need_dump_channels_(request_info) ?
            AdServer::LogProcessing::NumberArray(
              request_info.history_channels.begin(), request_info.history_channels.end()) :
            AdServer::LogProcessing::NumberArray(),
          std::move(page_trigger_channels),
          std::move(search_trigger_channels),
          std::move(url_trigger_channels),
          std::move(url_keyword_trigger_channels));

        data.add(
          CollectorT::DataT::DataT(
            request_info.is_ad_request || request_info.client_app == "WebIndex" ?
              'A' : 'P', // workaround for ADSC-8008
            user_id,
            temporary_user_id,
            std::move(match_request),
            std::move(ad_request_opt)));

        add_record(key, data);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    /** WebStatLogger implementation */
    void
    CampaignManagerLogger::WebStatLogger::
    process_web_operation(
      const CampaignManagerLogger::WebOperationInfo& web_op)
      /*throw(Exception)*/
    {
      static const char* FUN = "WebStatLogger::process_web_operation()";

      try
      {
        CollectorT::DataT data;
        data.add(
          CollectorT::DataT::KeyT(
            web_op.ct,
            web_op.curct,
            web_op.browser,
            web_op.os,
            web_op.user_bind_src.length() <= MAX_WEBSTAT_SOURCE_LENGTH ?
              web_op.user_bind_src :
              std::string(web_op.user_bind_src, 0, MAX_WEBSTAT_SOURCE_LENGTH),
            web_op.web_operation_id,
            web_op.result,
            adapt_user_status(web_op.user_status),
            web_op.test_request,
            web_op.tag_id ?
              CollectorT::DataT::KeyT::OptionalUlong(web_op.tag_id) :
              CollectorT::DataT::KeyT::OptionalUlong(),
            web_op.cc_id ?
              CollectorT::DataT::KeyT::OptionalUlong(web_op.cc_id) :
              CollectorT::DataT::KeyT::OptionalUlong()),
          CollectorT::DataT::DataT(1));
        add_record(
          CollectorT::KeyT(web_op.time, web_op.colo_id),
          data);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    /** ResearchWebStatLogger implementation */
    void
    CampaignManagerLogger::ResearchWebStatLogger::
    process_web_operation(
      const CampaignManagerLogger::WebOperationInfo& web_op)
      /*throw(Exception)*/
    {
      static const char* FUN = "ResearchWebStatLogger::process_web_operation()";

      try
      {
        if( //(!web_op.global_request_id.is_null() || !web_op.request_ids.empty()) &&
           !web_op.test_request)
        {
          CollectorT::DataT data;
          data.time = web_op.full_time;
          data.global_request_id = web_op.global_request_id;
          data.app = web_op.app;
          data.source = web_op.source;
          data.operation = web_op.operation;
          data.result = web_op.result;
          data.referer = web_op.referer;
          data.ip_address = web_op.ip_address;
          data.external_user_id = web_op.external_user_id;
          data.user_agent = web_op.user_agent;

          if(!web_op.request_ids.empty())
          {
            for(CampaignManagerLogger::WebOperationInfo::RequestIdList::
              const_iterator request_id_it = web_op.request_ids.begin();
              request_id_it != web_op.request_ids.end();
              ++request_id_it)
            {
              data.request_id = *request_id_it;
              add_record(data);
            }
          }
          else
          {
            add_record(data);
          }
        }
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerLogger::WebOperationInfo::init_by_flags(
      const Generics::Time& time_in,
      const char* ct_in,
      const char* curt_in,
      const char* browser_in,
      const char* os_in,
      unsigned int flags)
      noexcept
    {
      full_time = time_in;
      time = time_in;
      if(!(flags & LOG_HOUR))
      {
        time = Algs::round_to_day(time);
      }
      if(flags & LOG_CT)
      {
        ct = ct_in;
      }
      if(flags & LOG_CURT)
      {
        curct = curt_in;
      }
      if(flags & LOG_BROWSER)
      {
        browser = browser_in;
      }
      if(flags & LOG_OS)
      {
        os = os_in;
      }
    }

    /** CreativeStatLogger implementation */
    CampaignManagerLogger::CreativeStatLogger::CreativeStatLogger(
      const AdServer::LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : AdServer::LogProcessing::LogHolderPool<
          AdServer::LogProcessing::CreativeStatTraits>(
            flush_traits),
        THRESHOLD_ONE_(false, 1, 0),
        STAT_REQUESTS_INCREMENT_(
          1, // requests
          0, // imps
          0, // clicks
          0, // actions
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO),
        STAT_REQUESTS_DECREMENT_(
          -1, // requests
          0, // imps
          0, // clicks
          0, // actions
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO)
    {}

    inline
    CampaignManagerLogger::CreativeStatLogger::
    CollectorT::DataT::KeyT
    CampaignManagerLogger::CreativeStatLogger::init_no_ad_key_(
      unsigned long colo_id,
      unsigned long tag_id,
      unsigned long size_id,
      const char* country_code,
      unsigned long isp_rate_id,
      unsigned long pub_rate_id,
      unsigned long currency_exchange_id,
      bool log_as_test,
      bool fraud,
      bool walled_garden,
      bool household_based,
      UserStatus user_status,
      unsigned long /*geo_channel_id*/,
      unsigned long last_platform_channel_id)
      /*throw(eh::Exception)*/
    {
      return CollectorT::DataT::KeyT(
        colo_id,
        0, // pub_account_id
        tag_id,
        size_id,
        country_code,
        0, // adv_account_id
        0, // campaign_id
        0, // ccg_id
        0, // cc_id
        0, // adv rate id
        isp_rate_id,
        pub_rate_id,
        currency_exchange_id,
        THRESHOLD_ONE_, // for records without selected ad this field isn't used
        1, // num_shown
        1, // position
        log_as_test,
        fraud,
        walled_garden,
        adapt_user_status(user_status),
        LogProcessing::OptionalValue<unsigned long>(), // geo_channel_id
        last_platform_channel_id ?
          LogProcessing::OptionalValue<unsigned long>(
            last_platform_channel_id) :
          LogProcessing::OptionalValue<unsigned long>(),
        0,
        household_based,
        -1/*FIXME: viewability*/);
    }

    inline
    CampaignManagerLogger::CreativeStatLogger::
    CollectorT::DataT::KeyT
    CampaignManagerLogger::CreativeStatLogger::init_no_ad_key_(
      bool fraud,
      const CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerLogger::AdRequestSelectionInfo&
        ad_request_selection_info,
      const CampaignManagerLogger::AdSelectionInfo& ad_info)
      /*throw(eh::Exception)*/
    {
      return init_no_ad_key_(
        request_info.colo_id,
        ad_request_selection_info.tag_id,
        ad_request_selection_info.size_id,
        request_info.country_code.c_str(),
        ad_info.isp_revenue.rate_id,
        ad_info.pub_revenue.rate_id ?
          ad_info.pub_revenue.rate_id : ad_request_selection_info.site_rate_id,
        ad_info.currency_exchange_id,
        request_info.log_as_test,
        fraud,
        ad_request_selection_info.walled_garden,
        ad_request_selection_info.household_based,
        request_info.user_status,
        !request_info.geo_channels.empty() ?
          *request_info.geo_channels.begin() : 0,
        request_info.last_platform_channel_id);
    }

    void
    CampaignManagerLogger::CreativeStatLogger::
    process_ad_request(
      const CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerLogger::AdRequestSelectionInfo&
        ad_request_selection_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "CreativeStatLogger::process_ad_request()";

      if(!ad_request_selection_info.ad_selection_info_list.empty() &&
         !ad_request_selection_info.ad_selection_info_list.front().ad_selected)
      {
        try
        {
          const CampaignManagerLogger::AdSelectionInfo& ad_info =
            *ad_request_selection_info.ad_selection_info_list.begin();

          // log only for ad requests with tid & without ad
          CollectorT::KeyT key(request_info.time, request_info.time);

          CollectorT::DataT data;

          data.add(
            init_no_ad_key_(false, request_info, ad_request_selection_info, ad_info),
            STAT_REQUESTS_INCREMENT_);

          if(request_info.fraud)
          {
            data.add(
              init_no_ad_key_(true, request_info, ad_request_selection_info, ad_info),
              STAT_REQUESTS_DECREMENT_);
          }

          add_record(key, data);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << ex.what();
          throw Exception(ostr);
        }
      }
    }

    void
    CampaignManagerLogger::CreativeStatLogger::
    process_passback_track(
      const CampaignManagerLogger::PassbackTrackInfo& passback_track_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "CreativeStatLogger::process_passback_track()";

      try
      {
        CollectorT::KeyT key(passback_track_info.time, passback_track_info.time);

        CollectorT::DataT data;

        CollectorT::DataT::KeyT inner_key = init_no_ad_key_(
          passback_track_info.colo_id,
          passback_track_info.tag_id,
          0,
          passback_track_info.country.c_str(),
          passback_track_info.colo_rate_id,
          passback_track_info.site_rate_id,
          passback_track_info.currency_exchange_id,
          passback_track_info.log_as_test,
          false, // non fraud
          false, // non walled_garden
          false, // non household based
          passback_track_info.user_status,
          0, // geo_channel_id
          0);

        data.add(inner_key, STAT_REQUESTS_INCREMENT_);

        add_record(key, data);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    /** RequestLogger implementation */
    void
    CampaignManagerLogger::RequestLogger::
    process_ad_request(
      const CampaignManagerLogger::RequestInfo& request_info,
      const AdRequestSelectionInfo& ad_ri)
      /*throw(Exception)*/
    {
      typedef CollectorT::DataT::OptionalUlong OptionalUlong;

      static const char* FUN = "RequestLogger::process_ad_request()";

      try
      {
        for(CampaignManagerLogger::AdSelectionInfoList::const_iterator ad_it =
              ad_ri.ad_selection_info_list.begin();
            ad_it != ad_ri.ad_selection_info_list.end();
            ++ad_it)
        {
          const CampaignManagerLogger::AdSelectionInfo& ad_info = *ad_it;

          if(ad_info.ad_selected)
          {
            using namespace AdServer::LogProcessing;

            RequestData::Revenue adv_revenue, adv_comm_revenue;
            RequestData::Revenue adv_payable_comm_revenue;
            RequestData::Revenue pub_revenue, pub_comm_revenue;
            RequestData::Revenue isp_revenue;

            fill_log_revenue(ad_info.adv_revenue, adv_revenue);
            fill_log_revenue(ad_info.adv_payable_comm_revenue, adv_payable_comm_revenue);
            fill_log_revenue(ad_info.adv_comm_revenue, adv_comm_revenue);
            fill_log_revenue(ad_info.pub_revenue, pub_revenue);
            fill_log_revenue(ad_info.pub_comm_revenue, pub_comm_revenue);
            fill_log_revenue(ad_info.isp_revenue, isp_revenue);

            UserPropertyList user_properties;
            user_properties.push_back(
              UserProperty(
                PropertyNames::CLIENT_APP_VERSION.text(), request_info.client_app_version));
            user_properties.push_back(
              UserProperty(
              PropertyNames::CLIENT_APP.text(), request_info.client_app));
            user_properties.push_back(
              UserProperty(
                PropertyNames::OS_VERSION.text(), request_info.full_platform));
            user_properties.push_back(
              UserProperty(
                PropertyNames::BROWSER_VERSION.text(), request_info.web_browser));

            LogProcessing::RequestData::CmpChannelList cmp_channel_list;
            std::transform(
              ad_info.cmp_channels.begin(),
              ad_info.cmp_channels.end(),
              std::back_insert_iterator<
                LogProcessing::RequestData::CmpChannelList>(cmp_channel_list),
              CMPChannelConv());

            AdServer::LogProcessing::NumberArray lost_auction_ccgs;
            std::transform(
              ad_ri.lost_auction_ccgs.begin(),
              ad_ri.lost_auction_ccgs.end(),
              std::back_inserter(lost_auction_ccgs),
              std::mem_fun_ref(&AdRequestSelectionInfo::TimedId::get_id));

            CollectorT::DataT data(
              request_info.time,
              request_info.isp_time,
              ad_ri.pub_time,
              ad_info.adv_time,
              ad_info.request_id,
              request_info.request_id, // global request id
              request_info.request_user_id.present() ?
                *request_info.request_user_id :
                request_info.user_id,
              request_info.household_id,
              ad_info.log_as_test,
              request_info.colo_id,
              ad_ri.site_id,
              ad_ri.tag_id,
              ad_ri.ext_tag_id,
              ad_ri.pub_account_id,
              request_info.country_code,
              request_info.ip_hash,
              ad_info.adv_account_id,
              ad_info.advertiser_id,
              ad_info.cc_id,
              ad_info.campaign_id,
              ad_info.ccg_id,
              CollectorT::DataT::DeliveryThresholdT(
                ExDeliveryThresholdDecimal::div(
                  ExDeliveryThresholdDecimal(
                    false, ad_info.tag_delivery_threshold, 0),
                  ExDeliveryThresholdDecimal(
                    false, TAG_DELIVERY_MAX, 0)).str()),
              ad_info.has_custom_actions,
              ad_info.currency_exchange_id,
              std::move(user_properties),
              adv_revenue,
              pub_revenue,
              isp_revenue,
              adv_comm_revenue,
              adv_payable_comm_revenue,
              pub_comm_revenue,
              AdServer::LogProcessing::NumberArray(
                ad_info.channels.begin(), ad_info.channels.end()),
              AdServer::LogProcessing::NumberArray(
                request_info.history_channels.begin(),
                request_info.history_channels.end()),
              ad_info.expression,
              std::move(cmp_channel_list),
              ad_info.ccg_keyword_id,
              ad_info.keyword_channel_id,
              false, /*keyword_page_match*/
              false, /*keyword_search_match*/
              ad_info.num_shown,
              ad_info.position,
              ad_info.enabled_notice,
              ad_info.enabled_impression_tracking,
              ad_info.enabled_action_tracking,
              request_info.disable_fraud_detection,
              ad_ri.walled_garden,
              ad_info.text_campaign ? 'T' : 'D',
              adapt_user_status(
                request_info.request_user_status.present() ?
                *request_info.request_user_status :
                request_info.user_status),
              std::move(lost_auction_ccgs),
              AdServer::LogProcessing::NumberArray(
                request_info.geo_channels.begin(),
                request_info.geo_channels.end()),
              request_info.last_platform_channel_id ?
              LogProcessing::OptionalValue<unsigned long>(
                  request_info.last_platform_channel_id) :
                LogProcessing::OptionalValue<unsigned long>(),
              ad_ri.tag_size,
              ad_ri.size_id,
              ad_ri.household_based,
              ad_ri.tag_visibility.present() ?
                OptionalUlong(*ad_ri.tag_visibility) :
                OptionalUlong(),
              ad_ri.tag_top_offset.present() ?
                OptionalUlong(*ad_ri.tag_top_offset) :
                OptionalUlong(),
              ad_ri.tag_left_offset.present() ?
                OptionalUlong(*ad_ri.tag_left_offset) :
                OptionalUlong(),
              ad_info.ctr_reset_id,
              ad_info.campaign_imps,
              request_info.referer,
              ad_info.adv_currency_rate,
              ad_info.pub_currency_rate,
              ad_info.pub_commission,
              ad_info.isp_currency_rate,
              ad_info.isp_revenue_share,
              ad_info.external_ecpm_bid,
              ad_info.position == 1 ? ad_ri.floor_cost : RevenueDecimal::ZERO,
              ad_info.ctr_algorithm_id,
              ad_info.ctr,
              (request_info.short_referer_hash.present() ?
                *request_info.short_referer_hash : 0), // FIXME: short_referer_hash for RequestsLogs
              ad_ri.auction_type,
              ad_info.conv_rate_algorithm_id,
              ad_info.conv_rate,
              ad_ri.tag_predicted_viewability,
              ad_info.model_ctrs,
              ad_info.self_service_commission,
              ad_info.adv_commission,
              ad_info.pub_cost_coef,
              ad_info.at_flags
              );

            add_record(data);
          }
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerLogger::ImpressionLogger::
    process_impression(
      const CampaignManagerLogger::ImpressionInfo& impression_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "ImpressionLogger::process_impression()";

      try
      {
        add_record(CollectorT::DataT(
          impression_info.time,
          impression_info.request_id,
          impression_info.verify_type == RVT_IMPRESSION ?
            impression_info.user_id : Commons::UserId(),
          impression_info.referrer,
          impression_info.pub_imp_revenue,
          Commons::Optional<RevenueDecimal>(), // pub sys revenue (deprecated)
          impression_info.pub_imp_revenue_type == RT_SHARE ? 'P' : 'A',
          impression_info.verify_type == RVT_IMPRESSION ? 'T' : (
            impression_info.verify_type == RVT_NOTICE ? 'N' : 'C'),
          impression_info.viewability,
          impression_info.action_name,
          impression_info.user_id_hash_mod));
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerLogger::ClickLogger::
    process_click(
      const CampaignManagerLogger::ClickInfo& click_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "ClickLogger::process_click()";

      try
      {
        CollectorT::DataT data(
          click_info.time,
          click_info.request_id,
          click_info.referer,
          click_info.user_id_hash_mod);

        add_record(data);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerLogger::AdvertiserActionLogger::
    process_action(
      const CampaignManagerLogger::AdvActionInfo& adv_action_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "AdvertiserActionLogger::process_action()";
      try
      {
        CollectorT::DataT data(
          adv_action_info.time,
          adv_action_info.user_id,
          LogProcessing::RequestId(), // FIXME: request_id
          adv_action_info.action_id,
          adv_action_info.device_channel_id,
          adv_action_info.action_request_id,
          adv_action_info.ccg_ids,
          adv_action_info.referer,
          adv_action_info.order_id,
          adv_action_info.ip_hash,
          adv_action_info.action_value);

        add_record(data);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    /* CampaignManagerLogger::ActionRequestLogger */
    void
    CampaignManagerLogger::ActionRequestLogger::
    process_action(
      const CampaignManagerLogger::AdvActionInfo& adv_action_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "ActionRequestLogger::process_action()";

      if(!adv_action_info.log_as_test &&
         adv_action_info.action_id.present())
      {
        try
        {
          CollectorT::DataT data;
          char user_status = adapt_user_status(adv_action_info.user_status);
          data.add(
            CollectorT::DataT::KeyT(
              *adv_action_info.action_id,
              adv_action_info.country,
              adv_action_info.referer,
              user_status != 'U' ? user_status : 'N' // TO CHANGE
              ),
            CollectorT::DataT::DataT(
              1,
              adv_action_info.action_value
              )
          );

          add_record(
            CollectorT::KeyT(
            adv_action_info.time,
            adv_action_info.colo_id),
            data);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << ex.what();
          throw Exception(ostr);
        }
      }
    }

    /** CampaignManagerLogger::PassbackImpressionLogger */
    void CampaignManagerLogger::PassbackImpressionLogger::process_passback(
      const CampaignManagerLogger::PassbackInfo& passback_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "PassbackImpressionLogger::process_passback()";

      try
      {
        CollectorT::DataT data(passback_info.time, passback_info.request_id,
          passback_info.user_id_hash_mod);

        add_record(data);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    /** CampaignManagerLogger::PassbackStatLogger */
    void
    CampaignManagerLogger::PassbackStatLogger::process_passback_track(
      const PassbackTrackInfo& passback_track_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "PassbackStatLogger::process_passback_track()";

      if(!passback_track_info.log_as_test)
      {
        try
        {
          CollectorT::KeyT key(passback_track_info.time, passback_track_info.colo_id);

          CollectorT::DataT data;
          data.add(
            CollectorT::DataT::KeyT(
              adapt_user_status(passback_track_info.user_status),
              passback_track_info.country,
              passback_track_info.tag_id,
              LogProcessing::OptionalUlong()/*FIXME: size_id*/),
            CollectorT::DataT::DataT(1));

          add_record(key, data);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << e.what();
          throw Exception(ostr);
        }
      }
    }

    // TagRequestLogger

    void
    CampaignManagerLogger::
    TagRequestLogger::process_request(
      const CampaignManagerLogger::RequestInfo& request_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "TagRequestLogger::process_request()";

      try
      {
        if(!request_info.log_as_test && request_info.profile_referer)
        {
          CollectorT::DataT::OptInSectionOptional opt_in;

          if(request_info.user_status == US_OPTIN ||
             request_info.user_status == US_BLACKLISTED)
          {
            opt_in = CollectorT::DataT::OptInSection(
              0, // site_id,
              request_info.user_id,
              AdServer::LogProcessing::OptionalUlong(),
              false, // ad_shown
              request_info.profile_referer,
              request_info.user_agent);
          }
          CollectorT::DataT data(
            request_info.time,
            request_info.isp_time,
            false, // FIXME: test_request
            request_info.colo_id,
            0, // tag_id
            AdServer::LogProcessing::OptionalUlong(), // FIXME: size_id
            "", // ext_tag_id
            request_info.referer.empty() ?
              EMPTY_REFERER_ : request_info.referer,
            request_info.full_referer_hash.present() ?
              AdServer::LogProcessing::OptionalUlong(*request_info.full_referer_hash) :
              AdServer::LogProcessing::OptionalUlong(),
            adapt_user_status(request_info.user_status),
            request_info.country_code,
            AdServer::Commons::RequestId(), // passback_request_id
            RevenueDecimal::ZERO, // floor_cost,
            request_info.urls,
            opt_in);

          add_record(data);
        }
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerLogger::
    TagRequestLogger::process_ad_request(
      const CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerLogger::AdRequestSelectionInfo& ad_ri)
      /*throw(Exception)*/
    {
      static const char* FUN = "TagRequestLogger::process_ad_request()";

      try
      {
        if(!request_info.log_as_test)
        {
          CollectorT::DataT::OptInSectionOptional opt_in;

          const bool ad_shown = !ad_ri.ad_selection_info_list.empty() &&
            ad_ri.ad_selection_info_list.front().ad_selected;

          if(request_info.user_status == US_OPTIN ||
             request_info.user_status == US_BLACKLISTED ||
             ad_shown)
          {
            opt_in = CollectorT::DataT::OptInSection(
              ad_ri.site_id,
              request_info.user_id,
              ad_ri.page_load_id.present() ?
                AdServer::LogProcessing::OptionalUlong(*ad_ri.page_load_id) :
                AdServer::LogProcessing::OptionalUlong(),
              ad_shown,
              request_info.profile_referer,
              request_info.user_agent);
          }

          CollectorT::DataT data(
            request_info.time,
            request_info.isp_time,
            false, // FIXME: test_request
            request_info.colo_id,
            ad_ri.tag_id,
            ad_ri.size_id,
            ad_ri.ext_tag_id,
            request_info.referer.empty() ?
              EMPTY_REFERER_ : request_info.referer,
            request_info.full_referer_hash.present() ?
              AdServer::LogProcessing::OptionalUlong(*request_info.full_referer_hash) :
              AdServer::LogProcessing::OptionalUlong(),
            adapt_user_status(request_info.user_status),
            request_info.country_code,
            ad_shown ? AdServer::Commons::RequestId() : (
              request_info.track_passback ? request_info.request_id :
                AdServer::Commons::RequestId()),
            ad_ri.floor_cost,
            request_info.urls,
            opt_in);

          add_record(data);
        }
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    // TagPositionStatLogger
    void
    CampaignManagerLogger::
    TagPositionStatLogger::process_ad_request(
      const CampaignManagerLogger::RequestInfo& ri,
      const CampaignManagerLogger::AdRequestSelectionInfo& ad_ri)
      /*throw(Exception)*/
    {
      static const char* FUN = "TagPositionStatLogger::process_ad_request()";

      try
      {
        CollectorT::KeyT key(ad_ri.pub_time, ri.colo_id);
        CollectorT::DataT add_data;
        add_data.add(
          CollectorT::DataT::KeyT(
            ad_ri.request_tag_id,
            ad_ri.tag_top_offset.present() ?
              CollectorT::DataT::KeyT::OptionalUlong(*ad_ri.tag_top_offset) :
              CollectorT::DataT::KeyT::OptionalUlong(),
            ad_ri.tag_left_offset.present() ?
              CollectorT::DataT::KeyT::OptionalUlong(*ad_ri.tag_left_offset) :
              CollectorT::DataT::KeyT::OptionalUlong(),
            ad_ri.tag_visibility.present() ?
              CollectorT::DataT::KeyT::OptionalUlong(*ad_ri.tag_visibility) :
              CollectorT::DataT::KeyT::OptionalUlong(),
            ri.log_as_test),
          ONE_REQUEST_);
        add_record(key, add_data);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerLogger::CcgStatLogger::process_ad_request(
      const CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerLogger::AdRequestSelectionInfo&
        ad_request_selection_info)
      /*throw(Exception)*/
    {
      typedef std::map<Generics::Time, CollectorT::DataT> AdvDateDataMap;

      if(!request_info.log_as_test &&
         !ad_request_selection_info.lost_auction_ccgs.empty())
      {
        AdvDateDataMap adv_date_to_data;

        for(CampaignManagerLogger::AdRequestSelectionInfo::
              TimedIdList::const_iterator lost_auction_ccg_it =
              ad_request_selection_info.lost_auction_ccgs.begin();
            lost_auction_ccg_it != ad_request_selection_info.lost_auction_ccgs.end();
            ++lost_auction_ccg_it)
        {
          adv_date_to_data[Algs::round_to_day(lost_auction_ccg_it->adv_time)].
            add(CollectorT::DataT::KeyT(lost_auction_ccg_it->id),
              STAT_LOST_AUCTION_ONE_);
        }

        for(AdvDateDataMap::const_iterator adv_tz_it =
              adv_date_to_data.begin();
            adv_tz_it != adv_date_to_data.end(); ++adv_tz_it)
        {
          add_record(
            CollectorT::KeyT(adv_tz_it->first, request_info.colo_id),
            adv_tz_it->second);
        }
      }
    }

    void
    CampaignManagerLogger::CcStatLogger::process_ad_request(
      const CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerLogger::AdRequestSelectionInfo&
        ad_request_selection_info)
      /*throw(Exception)*/
    {
      typedef std::map<Generics::Time, CollectorT::DataT> AdvDateDataMap;

      if(!request_info.log_as_test &&
         !ad_request_selection_info.lost_auction_creatives.empty())
      {
        AdvDateDataMap adv_date_to_data;

        for(CampaignManagerLogger::AdRequestSelectionInfo::
              TimedIdList::const_iterator lost_auction_cc_it =
                ad_request_selection_info.lost_auction_creatives.begin();
            lost_auction_cc_it != ad_request_selection_info.lost_auction_creatives.end();
            ++lost_auction_cc_it)
        {
          adv_date_to_data[Algs::round_to_day(lost_auction_cc_it->adv_time)].
            add(CollectorT::DataT::KeyT(lost_auction_cc_it->id),
              STAT_LOST_AUCTION_ONE_);
        }

        for(AdvDateDataMap::const_iterator adv_tz_it =
              adv_date_to_data.begin();
            adv_tz_it != adv_date_to_data.end(); ++adv_tz_it)
        {
          add_record(
            CollectorT::KeyT(adv_tz_it->first, request_info.colo_id),
            adv_tz_it->second);
        }
      }
    }

    /** SearchTermStatLogger implementation */
    void
    CampaignManagerLogger::SearchTermStatLogger::
    process_ad_request(
      const CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerLogger::AdRequestSelectionInfo&)
      /*throw(Exception)*/
    {
      add_record_(request_info);
    }

    void
    CampaignManagerLogger::SearchTermStatLogger::
    process_request(
      const CampaignManagerLogger::RequestInfo& request_info)
      /*throw(Exception)*/
    {
      add_record_(request_info);
    }

    void
    CampaignManagerLogger::SearchTermStatLogger::
    process_anon_request(
      const CampaignManagerLogger::AnonymousRequestInfo& request_info)
      /*throw(Exception)*/
    {
      add_record_(request_info);
    }

    void
    CampaignManagerLogger::SearchTermStatLogger::
    add_record_(
      const CampaignManagerLogger::AnonymousRequestInfo& request_info)
      /*throw(Exception)*/
    {
      if (!request_info.log_as_test &&
        request_info.search_words &&
        !request_info.search_words->str().empty())
      {
        CollectorT::DataT data;
        data.add(
          CollectorT::DataT::KeyT(request_info.search_words->str()),
          STAT_HITS_ONE_);
        add_record(CollectorT::KeyT(request_info.time, request_info.colo_id),
          data);
      }
    }

    /** SearchEngineStatLogger implementation */
    void
    CampaignManagerLogger::SearchEngineStatLogger::
    process_ad_request(
      const CampaignManagerLogger::RequestInfo& request_info,
      const CampaignManagerLogger::AdRequestSelectionInfo&)
      /*throw(Exception)*/
    {
      add_record_(request_info);
    }

    void
    CampaignManagerLogger::SearchEngineStatLogger::
    process_request(
      const CampaignManagerLogger::RequestInfo& request_info)
      /*throw(Exception)*/
    {
      add_record_(request_info);
    }

    void
    CampaignManagerLogger::SearchEngineStatLogger::
    process_anon_request(
      const CampaignManagerLogger::AnonymousRequestInfo& request_info)
      /*throw(Exception)*/
    {
      add_record_(request_info);
    }

    void
    CampaignManagerLogger::SearchEngineStatLogger::
    add_record_(
      const CampaignManagerLogger::AnonymousRequestInfo& request_info)
      /*throw(Exception)*/
    {
      if(!request_info.log_as_test && request_info.search_engine_id)
      {
        CollectorT::DataT data;
        data.add(
          CollectorT::DataT::KeyT(
            request_info.search_engine_id,
            request_info.search_engine_host),
          request_info.page_keywords_present ?
            STAT_HITS_ONE_ : STAT_EMPTY_PAGE_HITS_ONE_);
        add_record(CollectorT::KeyT(request_info.time, request_info.colo_id),
          data);
      }
    }

    /** CampaignManagerLogger */
    CampaignManagerLogger::CampaignManagerLogger(
      const CampaignManagerLogger::Params& params,
      Logging::Logger* logger)
      /*throw(Exception)*/
      : logger_(ReferenceCounting::add_ref(logger))
    {
      static const char* FUN = "CampaignManagerLogger::CampaignManagerLogger()";

      try
      {
        if(params.channel_trigger_stat.period != Generics::Time::ZERO)
        {
          channel_trigger_stat_logger_ =
            new ChannelTriggerStatLogger(params.channel_trigger_stat);
          add_child_log_holder(channel_trigger_stat_logger_);
        }

        channel_hit_stat_logger_ =
          new ChannelHitStatLogger(params.channel_hit_stat);
        add_child_log_holder(channel_hit_stat_logger_);

        request_basic_channels_logger_ =
          new RequestBasicChannelsLogger(params.request_basic_channels);
        add_child_log_holder(request_basic_channels_logger_);

        web_stat_logger_ = new WebStatLogger(params.web_stat);
        add_child_log_holder(web_stat_logger_);

        if(params.research_web_stat.period != Generics::Time::ZERO)
        {
          research_web_stat_logger_ = new ResearchWebStatLogger(params.research_web_stat);
          add_child_log_holder(research_web_stat_logger_);
        }

        creative_stat_logger_ =
          new CreativeStatLogger(params.creative_stat);
        add_child_log_holder(creative_stat_logger_);

        action_request_logger_ =
          new ActionRequestLogger(params.action_request);
        add_child_log_holder(action_request_logger_);

        request_logger_ = new RequestLogger(params.request);
        add_child_log_holder(request_logger_);

        impression_logger_ = new ImpressionLogger(params.impression);
        add_child_log_holder(impression_logger_);

        click_logger_ = new ClickLogger(params.click);
        add_child_log_holder(click_logger_);

        advertiser_action_logger_ =
          new AdvertiserActionLogger(params.advertiser_action);
        add_child_log_holder(advertiser_action_logger_);

        passback_impression_logger_ =
          new PassbackImpressionLogger(params.passback_impression);
        add_child_log_holder(passback_impression_logger_);

        if(params.search_term_stat.period != Generics::Time::ZERO)
        {
          search_term_stat_logger_ =
            new SearchTermStatLogger(params.search_term_stat);
          add_child_log_holder(search_term_stat_logger_);
        }

        search_engine_stat_logger_ =
          new SearchEngineStatLogger(params.search_engine_stat);
        add_child_log_holder(search_engine_stat_logger_);

        tag_auction_stat_logger_ =
          new TagAuctionStatLogger(params.tag_auction_stat);
        add_child_log_holder(tag_auction_stat_logger_);

        /*
        user_properties_logger_ =
          new UserPropertiesLogger(params.user_properties);
        add_child_log_holder(user_properties_logger_);
        */

        tag_request_logger_ = new TagRequestLogger(params.tag_request);
        add_child_log_holder(tag_request_logger_);

        tag_position_stat_logger_ = new TagPositionStatLogger(params.tag_position_stat);
        add_child_log_holder(tag_position_stat_logger_);

        if(params.ccg_stat.period != Generics::Time::ZERO)
        {
          ccg_stat_logger_ = new CcgStatLogger(params.ccg_stat);
          add_child_log_holder(ccg_stat_logger_);
        }

        if(params.cc_stat.period != Generics::Time::ZERO)
        {
          cc_stat_logger_ = new CcStatLogger(params.cc_stat);
          add_child_log_holder(cc_stat_logger_);
        }

        passback_stat_logger_ = new PassbackStatLogger(params.passback_stat);
        add_child_log_holder(passback_stat_logger_);

        user_agent_stat_logger_ = new UserAgentStatLogger(params.user_agent_stat);
        add_child_log_holder(user_agent_stat_logger_);

        if(params.prof_research.period != Generics::Time::ZERO &&
            params.profiling_log_sampling > 0)
        {
          profiling_research_logger_ = new ProfilingResearchLogger(
            logger_,
            params.profiling_research_record_limit,
            params.profiling_log_sampling,
            params.prof_research,
            params.log_referrer_setting);

          add_child_log_holder(profiling_research_logger_);
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't init loggers. Caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }

    CampaignManagerLogger::~CampaignManagerLogger() noexcept
    {}

    void
    CampaignManagerLogger::process_request(
      const CampaignManagerLogger::RequestInfo& request_info,
      unsigned long profiling_type)
      /*throw(Exception)*/
    {
      if (profiling_type & PT_PROFILING_INFO)
      {
        if(channel_trigger_stat_logger_.in())
        {
          channel_trigger_stat_logger_->process_request(request_info);
        }

        channel_hit_stat_logger_->process_request(request_info);
        request_basic_channels_logger_->process_request(request_info);
      }

      if (profiling_type & PT_OTHER)
      {
        //user_properties_logger_->process_request(request_info);
        user_agent_stat_logger_->process_request(request_info);

        if(search_term_stat_logger_.in())
        {
          search_term_stat_logger_->process_request(request_info);
        }

        search_engine_stat_logger_->process_request(request_info);
        tag_request_logger_->process_request(request_info);
      }

      if (profiling_research_logger_.in())
      {
        profiling_research_logger_->process_request(request_info);
      }
    }

    void
    CampaignManagerLogger::process_anon_request(
      const AnonymousRequestInfo& anon_request_info)
      /*throw(Exception)*/
    {
      //user_properties_logger_->process_anon_request(anon_request_info);
      user_agent_stat_logger_->process_anon_request(anon_request_info);

      if(search_term_stat_logger_.in())
      {
        search_term_stat_logger_->process_anon_request(anon_request_info);
      }

      search_engine_stat_logger_->process_anon_request(anon_request_info);
    }

    void
    CampaignManagerLogger::process_match_request(
      const MatchRequestInfo& match_request_info)
      /*throw(Exception)*/
    {
      request_basic_channels_logger_->process_match_request(match_request_info);
      channel_hit_stat_logger_->process_match_request(match_request_info);
      channel_trigger_stat_logger_->process_match_request(match_request_info);
    }

    void
    CampaignManagerLogger::process_ad_request(
      const CampaignManagerLogger::RequestInfo& ri,
      const CampaignManagerLogger::AdRequestSelectionInfo& ad_ri)
      /*throw(Exception)*/
    {
      if(channel_trigger_stat_logger_.in())
      {
        channel_trigger_stat_logger_->process_request(ri);
      }

      channel_hit_stat_logger_->process_request(ri);
      //user_properties_logger_->process_ad_request(ri, ad_ri);

      if(search_term_stat_logger_.in())
      {
        search_term_stat_logger_->process_request(ri);
      }

      search_engine_stat_logger_->process_request(ri);

      request_basic_channels_logger_->process_ad_request(ri, ad_ri);

      creative_stat_logger_->process_ad_request(ri, ad_ri);

      request_logger_->process_ad_request(ri, ad_ri);

      tag_request_logger_->process_ad_request(ri, ad_ri);

      tag_position_stat_logger_->process_ad_request(ri, ad_ri);

      if(ccg_stat_logger_.in())
      {
        ccg_stat_logger_->process_ad_request(ri, ad_ri);
      }

      if(cc_stat_logger_.in())
      {
        cc_stat_logger_->process_ad_request(ri, ad_ri);
      }

      tag_auction_stat_logger_->process_ad_request(ri, ad_ri);

      user_agent_stat_logger_->process_request(ri);

      if (profiling_research_logger_.in())
      {
        profiling_research_logger_->process_request(ri);
      }
    }

    void
    CampaignManagerLogger::process_impression(
      const ImpressionInfo& impression_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerLogger::process_impression()";

      try
      {
        impression_logger_->process_impression(impression_info);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerLogger::process_click(
      const ClickInfo& request_id)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerLogger::process_click()";

      try
      {
        click_logger_->process_click(request_id);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerLogger::process_action(
      const AdvActionInfo& adv_action_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerLogger::process_action()";

      try
      {
        advertiser_action_logger_->process_action(adv_action_info);
        action_request_logger_->process_action(adv_action_info);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }

    void
    CampaignManagerLogger::process_passback(
      const PassbackInfo& passback_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerLogger::process_passback()";

      try
      {
        passback_impression_logger_->process_passback(passback_info);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << ex.what();
        throw Exception(ostr.str());
      }
    }

    void
    CampaignManagerLogger::process_passback_track(
      const PassbackTrackInfo& passback_track_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerLogger::process_passback_track()";

      try
      {
        passback_stat_logger_->process_passback_track(
          passback_track_info);

        creative_stat_logger_->process_passback_track(
          passback_track_info);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Caught eh::Exception: " << ex.what();
        throw Exception(ostr.str());
      }
    }

    void
    CampaignManagerLogger::process_web_operation(
      const WebOperationInfo& web_op_info)
      /*throw(Exception)*/
    {
      static const char* FUN = "CampaignManagerLogger::process_web_operation()";

      try
      {
        web_stat_logger_->process_web_operation(web_op_info);

        if(research_web_stat_logger_.in())
        {
          research_web_stat_logger_->process_web_operation(web_op_info);
        }
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << FUN << ": Caught eh::Exception: " << e.what();
        throw Exception(err);
      }
    }

    void
    CampaignManagerLogger::process_oo_operation(
      unsigned long colo_id,
      const Generics::Time& time,
      const Generics::Time& time_offset,
      const AdServer::Commons::UserId& user_id,
      bool log_as_test,
      char operation)
      /*throw(Exception)*/
    {
      static const char* FN = "CampaignManagerLogger::process_oo_operation";

      try
      {
        if ((operation == 'I' || operation == 'F') && !log_as_test)
        {
          request_basic_channels_logger_->process_oo_operation(
            colo_id,
            time,
            time_offset,
            user_id);
        }
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << FN << ": Caught eh::Exception: " << e.what();
        throw Exception(err);
      }
    }

  } // namespace CampaignSvcs
} // namespace AdServer

