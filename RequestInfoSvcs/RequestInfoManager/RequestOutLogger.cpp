/**
 * @file RequestOutLogger.cpp
 */
#include <HTTP/UrlAddress.hpp>

#include <LogCommons/UserProperties.hpp>
#include <LogCommons/CreativeStat.hpp>
#include <LogCommons/ChannelPerformance.hpp>
#include <LogCommons/ExpressionPerformance.hpp>
#include <LogCommons/CcgKeywordStat.hpp>
#include <LogCommons/CcgStat.hpp>
#include <LogCommons/ActionStat.hpp>
#include <LogCommons/PassbackStat.hpp>
#include <LogCommons/CmpStat.hpp>
#include <LogCommons/ChannelImpInventory.hpp>
#include <LogCommons/SiteUserStat.hpp>
#include <LogCommons/SiteReferrerStat.hpp>
#include <LogCommons/PageLoadsDailyStat.hpp>
#include <LogCommons/CampaignUserStat.hpp>
#include <LogCommons/CcgUserStat.hpp>
#include <LogCommons/CcUserStat.hpp>
#include <LogCommons/TagPositionStat.hpp>
#include <LogCommons/CampaignReferrerStat.hpp>
#include <LogCommons/BidCostStat.hpp>

//#include <LogCommons/ResearchBidStat.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <LogCommons/Request.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>
#include <LogCommons/ResearchLogs.hpp>

#include "RequestOutLogger.hpp"

namespace
{
  const char USER_PROPERTIES_LOGGER[] = "UserPropertiesLogger";
  const char CREATIVE_STAT_LOGGER[] = "CreativeStatLogger";
  const char CHANNEL_PERFORMANCE_LOGGER[] = "ChannelPerformanceLogger";
  const char EXPRESSION_PERFORMANCE_LOGGER[] = "ExpressionPerformanceLogger";
  const char CCG_KEYWORD_STAT_LOGGER[] = "CCGKeywordStatLogger";
  const char CMP_STAT_LOGGER[] = "CmpStatLogger";
  const char CHANNEL_IMP_INVENTORY_LOGGER[] = "ChannelImpInventory";
  const char TAG_POSITION_STAT_LOGGER[] = "TagPositionStat";

  const char CCG_USER_STAT_LOGGER[] = "CCGUserStatLogger";
  const char CC_USER_STAT_LOGGER[] = "CCUserStatLogger";
  const char CAMPAIGN_USER_STAT_LOGGER[] = "CampaignUserStatLogger";
  const char ADVERTISER_USER_STAT_LOGGER[] = "AdvertiserUserStatLogger";

  const char ACTION_STAT_LOGGER[] = "ActionStatLogger";

  const char PASSBACK_STAT_LOGGER[] = "PassbackStatLogger";

  const char SITE_STAT_LOGGER[] = "SiteUserStatLogger";

  const char SITE_REFERRER_STAT_LOGGER[] = "SiteReferrerStatLogger";
  const char CAMPAIGN_REFERRER_STAT_LOGGER[] = "CampaignReferrerStatLogger";
  const char BID_COST_STAT_LOGGER[] = "BidCostStatLogger";

  const char PAGE_LOADS_DAILY_STAT_LOGGER[] = "PageLoadsDailyStatLogger";

  namespace PropertyName
  {
    const char COUNTRY[] = "CountryCode";
    const char CLIENT_VERSION[] = "ClientVersion";
    const char OS_VERSION[] = "OsVersion";
    const char BROWSER_VERSION[] = "BrowserVersion";
    const char CLIENT_APP[] = "Client";
  }
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    const RevenueDecimal CPM_MULTILPIER = RevenueDecimal (1000, 0);
    const RevenueDecimal REVENUE_ONE(false, 1, 0);
  }

  //
  // RequestLoggerBase
  //
  class RequestLoggerBase:
    public virtual RequestActionProcessor,
    public virtual AdServer::LogProcessing::LogHolder
  {};

  typedef ReferenceCounting::SmartPtr<RequestLoggerBase>
    RequestLoggerBase_var;

  //
  // CampaignReachLoggerBase
  //
  class CampaignReachLoggerBase:
    public virtual CampaignReachProcessor,
    public virtual AdServer::LogProcessing::LogHolder
  {
  protected:
    virtual ~CampaignReachLoggerBase() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<CampaignReachLoggerBase>
    CampaignReachLoggerBase_var;

  //
  // SiteReachLoggerBase
  //
  class SiteReachLoggerBase:
    public virtual SiteReachProcessor,
    public virtual AdServer::LogProcessing::LogHolder
  {
  protected:
    virtual ~SiteReachLoggerBase() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<SiteReachLoggerBase>
    SiteReachLoggerBase_var;

  //
  // CampaignReachLoggerAdapter
  //
  template<typename LogTraitsType>
  class CampaignReachLoggerAdapter:
    public CampaignReachLoggerBase,
    public AdServer::LogProcessing::LogHolderPool<LogTraitsType>,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    typedef AdServer::LogProcessing::LogHolderPool<LogTraitsType>
      LogHolder;

    CampaignReachLoggerAdapter(
      const LogProcessing::LogFlushTraits& flush_traits,
      unsigned long colo_id)
      /*throw(eh::Exception)*/
      : LogHolder(flush_traits), colo_id_(colo_id)
    {}

  protected:
    struct NullValueAdapter
    {
      template<typename ValueType>
      ValueType
      operator()(const ValueType& value) const
      {
        return value;
      }

      template<typename ValueType>
      ValueType
      operator()(const ValueType& value, unsigned long) const
      {
        return value;
      }
    };

    template<typename KeyType>
    struct ColoIdAdapter
    {
      KeyType
      operator()(const Generics::Time& date, unsigned long colo_id) const
      {
        return KeyType(date, colo_id);
      }
    };

  protected:
    virtual ~CampaignReachLoggerAdapter() noexcept
    {}

    template<typename KeyAdapterType, typename CounterAdapterType>
    void
    add_reach_records_(
      const IdAppearanceList& appearances,
      const KeyAdapterType& key_adapter,
      const CounterAdapterType& counter_adapter)
      /*throw(eh::Exception)*/
    {
      if(!appearances.empty())
      {
        for(IdAppearanceList::const_iterator it = appearances.begin();
            it != appearances.end(); ++it)
        {
          typedef typename LogHolder::CollectorT::DataT DataT;
          typename DataT::KeyT inner_key(it->id, it->last_appearance_date);
          DataT data;
          data.add(inner_key,
            typename DataT::DataT(counter_adapter(it->counter)));
          this->add_record(key_adapter(it->date, colo_id_), data);
        }
      }
    }

  private:
    const unsigned long colo_id_;
  };

  //
  // RequestLoggerAdapter
  //
  template<typename LogTraitsType>
  class RequestLoggerAdapter:
    public RequestLoggerBase,
    public AdServer::LogProcessing::LogHolderPool<LogTraitsType>,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    RequestLoggerAdapter(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : AdServer::LogProcessing::LogHolderPool<LogTraitsType>(flush_traits)
    {}

    virtual const char* name() noexcept = 0;

    virtual void
    process_request(
      const RequestInfo& request_info,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      try
      {
        process_request_impl(request_info, processing_state);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << name() << "::process_request(): "
          "eh::Exception caught: " << ex.what();
        throw RequestActionProcessor::Exception(ostr);
      }
    }

    virtual void
    process_impression(
      const RequestInfo& request_info,
      const ImpressionInfo&,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      try
      {
        process_impression_impl(request_info, processing_state);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << name() <<
          "::process_impression(): eh::Exception caught: " <<
          ex.what();
        throw RequestActionProcessor::Exception(ostr);
      }
    }

    virtual void
    process_click(
      const RequestInfo& request_info,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      try
      {
        process_click_impl(request_info, processing_state);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << name() <<
          "::process_click(): eh::Exception caught: " <<
          ex.what();
        throw RequestActionProcessor::Exception(ostr);
      }
    }

    virtual void
    process_action(const RequestInfo& request_info)
      /*throw(RequestActionProcessor::Exception)*/
    {
      try
      {
        process_action_impl(request_info);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << name() <<
          "::process_action(): eh::Exception caught: " <<
          ex.what();
        throw RequestActionProcessor::Exception(ostr);
      }
    }

    virtual void
    process_custom_action(
      const RequestInfo& request_info,
      const AdvCustomActionInfo& adv_custom_action_info)
      /*throw(RequestActionProcessor::Exception)*/
    {
      try
      {
        process_custom_action_impl(request_info, adv_custom_action_info);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << name() <<
          "::process_action(): eh::Exception caught: " <<
          ex.what();
        throw RequestActionProcessor::Exception(ostr);
      }
    }

    virtual void
    process_request_post_action(
      const RequestInfo& request_info,
      const RequestPostActionInfo& request_post_action_info)
      /*throw(RequestActionProcessor::Exception)*/
    {
      try
      {
        process_request_post_action_impl(request_info, request_post_action_info);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << name() <<
          "::process_action(): eh::Exception caught: " <<
          ex.what();
        throw RequestActionProcessor::Exception(ostr);
      }
    }

  protected:
    virtual ~RequestLoggerAdapter() noexcept
    {}

    virtual void
    process_request_impl(
      const RequestInfo& request_info,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/ = 0;

    virtual void
    process_impression_impl(
      const RequestInfo& request_info,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/ = 0;

    virtual void
    process_click_impl(
      const RequestInfo& request_info,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/ = 0;

    virtual void
    process_action_impl(const RequestInfo& request_info)
      /*throw(eh::Exception)*/ = 0;

    virtual void
    process_request_post_action_impl(
      const RequestInfo& /*request_info*/,
      const RequestPostActionInfo& /*request_post_action_info*/)
      /*throw(RequestActionProcessor::Exception)*/
    {}

    virtual void
    process_custom_action_impl(
      const RequestInfo& /*request_info*/,
      const AdvCustomActionInfo& /*adv_custom_action_info*/)
      /*throw(eh::Exception)*/
    {};
  };

  /**
   * UserPropertiesLogger
   */
  class UserPropertiesLogger:
    public RequestLoggerAdapter<
      AdServer::LogProcessing::UserPropertiesTraits>
  {
  public:
    UserPropertiesLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : RequestLoggerAdapter<
          AdServer::LogProcessing::UserPropertiesTraits>(flush_traits),
        STAT_UNVERIFIED_IMP_ONE_(0, 0, 1 /*imps_unverified*/, 0, 0, 0),
        STAT_VERIFIED_IMP_ONE_(0, 0, 0, 1 /*imps_verified*/, 0, 0),
        STAT_CLICK_ONE_(0, 0, 0, 0, 1, 0),
        STAT_ACTION_ONE_(0, 0, 0, 0, 0, 1),
        STAT_NEGATIVE_UNVERIFIED_IMP_ONE_(0, 0, -1 /*imps_unverified*/, 0, 0, 0)
    {}

    virtual const char* name() noexcept
    {
      return USER_PROPERTIES_LOGGER;
    }

    virtual void
    process_request_impl(
      const RequestInfo& request_info,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      // process moved, resave sequence for correct user status
      if(processing_state.state == RequestInfo::RS_NORMAL ||
        processing_state.state == RequestInfo::RS_RESAVE)
      {
        log_record_(request_info, STAT_UNVERIFIED_IMP_ONE_);
      }
      else if(processing_state.state == RequestInfo::RS_MOVED ||
        processing_state.state == RequestInfo::RS_ROLLBACK)
      {
        log_record_(request_info, STAT_NEGATIVE_UNVERIFIED_IMP_ONE_);
      }
    }

    virtual void
    process_impression_impl(
      const RequestInfo& request_info,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      // don't process resave for impression, it can't change user status
      if(processing_state.state == RequestInfo::RS_NORMAL)
      {
        log_record_(request_info, STAT_VERIFIED_IMP_ONE_);
      }
    }

    virtual void
    process_click_impl(
      const RequestInfo& request_info,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      if(processing_state.state == RequestInfo::RS_NORMAL)
      {
        log_record_(request_info, STAT_CLICK_ONE_);
      }
    }

    virtual void
    process_action_impl(const RequestInfo& request_info)
      /*throw(eh::Exception)*/
    {
      log_record_(request_info, STAT_ACTION_ONE_);
    }

    void log_record_(
      const RequestInfo& request_info,
      const CollectorT::DataT& data)
    {
      if(!request_info.test_request)
      {
        using namespace PropertyName;

        add_record(
          CollectorT::KeyT(
            request_info.time, request_info.isp_time, request_info.colo_id,
            request_info.user_status, COUNTRY, request_info.country),
          data);
        add_record(
          CollectorT::KeyT(
            request_info.time, request_info.isp_time, request_info.colo_id,
            request_info.user_status, CLIENT_VERSION,
            request_info.client_app_version),
          data);
        add_record(
          CollectorT::KeyT(
            request_info.time, request_info.isp_time, request_info.colo_id,
            request_info.user_status, OS_VERSION, request_info.os_version),
          data);
        add_record(
          CollectorT::KeyT(
            request_info.time, request_info.isp_time, request_info.colo_id,
            request_info.user_status, BROWSER_VERSION,
            request_info.browser_version),
          data);
        add_record(
          CollectorT::KeyT(
            request_info.time, request_info.isp_time, request_info.colo_id,
            request_info.user_status, CLIENT_APP, request_info.client_app),
          data);
      }
    }

  protected:
    virtual ~UserPropertiesLogger() noexcept {}

  private:
    const CollectorT::DataT STAT_UNVERIFIED_IMP_ONE_;
    const CollectorT::DataT STAT_VERIFIED_IMP_ONE_;
    const CollectorT::DataT STAT_CLICK_ONE_;
    const CollectorT::DataT STAT_ACTION_ONE_;
    const CollectorT::DataT STAT_NEGATIVE_UNVERIFIED_IMP_ONE_;
  };

  /**
   * CreativeStatLogger
   */
  class CreativeStatLogger:
    public RequestLoggerAdapter<
      AdServer::LogProcessing::CreativeStatTraits>
  {
  public:
    CreativeStatLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : RequestLoggerAdapter<
          AdServer::LogProcessing::CreativeStatTraits>(flush_traits),
        STAT_REQUEST_ONE_(
          1, 0, 0, 0,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO),
        STAT_REQUEST_NEGATIVE_ONE_(
          -1, 0, 0, 0,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO,
          CollectorT::DataT::DataT::FixedNum::ZERO)
    {};

    virtual const char* name() noexcept
    {
      return CREATIVE_STAT_LOGGER;
    }

    virtual void
    process_request_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      if(processing_state.state == RequestInfo::RS_NORMAL ||
        processing_state.state == RequestInfo::RS_RESAVE)
      {
        log_record_(
          ri,
          STAT_REQUEST_ONE_);
      }
      else if(processing_state.state == RequestInfo::RS_FRAUD ||
        processing_state.state == RequestInfo::RS_MOVED ||
        processing_state.state == RequestInfo::RS_ROLLBACK)
      {
        log_record_(
          ri,
          STAT_REQUEST_NEGATIVE_ONE_);
      }
    }

    virtual void
    process_impression_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      if(processing_state.state == RequestInfo::RS_NORMAL ||
        processing_state.state == RequestInfo::RS_RESAVE)
      {
        log_record_(
          ri,
          CollectorT::DataT::DataT(
            0, 1, 0, 0,
            ri.adv_revenue.impression,
            ri.pub_revenue.impression,
            ri.isp_revenue.impression,
            ri.adv_comm_revenue.impression,
            ri.pub_comm_revenue.impression,
            ri.adv_payable_comm_amount.impression,
            ri.pub_revenue.convert_impression(ri.adv_revenue.currency_rate),
            ri.isp_revenue.convert_impression(ri.adv_revenue.currency_rate)));
      }
      else if(processing_state.state == RequestInfo::RS_FRAUD ||
        processing_state.state == RequestInfo::RS_MOVED ||
        processing_state.state == RequestInfo::RS_ROLLBACK)
      {
        log_record_(
          ri,
          CollectorT::DataT::DataT(
            0, -1, 0, 0,
            RevenueDecimal(ri.adv_revenue.impression).negate(),
            RevenueDecimal(ri.pub_revenue.impression).negate(),
            RevenueDecimal(ri.isp_revenue.impression).negate(),
            RevenueDecimal(ri.adv_comm_revenue.impression).negate(),
            RevenueDecimal(ri.pub_comm_revenue.impression).negate(),
            RevenueDecimal(ri.adv_payable_comm_amount.impression).negate(),
            RevenueDecimal(ri.pub_revenue.convert_impression(ri.adv_revenue.currency_rate)).negate(),
            RevenueDecimal(ri.isp_revenue.convert_impression(ri.adv_revenue.currency_rate)).negate()));
      }
    }

    virtual void
    process_click_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      if(processing_state.state == RequestInfo::RS_NORMAL ||
        processing_state.state == RequestInfo::RS_RESAVE)
      {
        log_record_(
          ri,
          CollectorT::DataT::DataT(
            0, 0, 1, 0,
            ri.adv_revenue.click,
            ri.pub_revenue.click,
            ri.isp_revenue.click,
            ri.adv_comm_revenue.click,
            ri.pub_comm_revenue.click,
            ri.adv_payable_comm_amount.click,
            ri.pub_revenue.convert_click(ri.adv_revenue.currency_rate),
            ri.isp_revenue.convert_click(ri.adv_revenue.currency_rate)));
      }
      else if(processing_state.state == RequestInfo::RS_FRAUD ||
        processing_state.state == RequestInfo::RS_MOVED ||
        processing_state.state == RequestInfo::RS_ROLLBACK)
      {
        log_record_(
          ri,
          CollectorT::DataT::DataT(
            0, 0, -1, 0,
            RevenueDecimal(ri.adv_revenue.click).negate(),
            RevenueDecimal(ri.pub_revenue.click).negate(),
            RevenueDecimal(ri.isp_revenue.click).negate(),
            RevenueDecimal(ri.adv_comm_revenue.click).negate(),
            RevenueDecimal(ri.pub_comm_revenue.click).negate(),
            RevenueDecimal(ri.adv_payable_comm_amount.click).negate(),
            RevenueDecimal(ri.pub_revenue.convert_click(ri.adv_revenue.currency_rate)).negate(),
            RevenueDecimal(ri.isp_revenue.convert_click(ri.adv_revenue.currency_rate)).negate()));
      }
    }

    virtual void
    process_action_impl(const RequestInfo& ri)
      /*throw(eh::Exception)*/
    {
      log_record_(
        ri,
        CollectorT::DataT::DataT(
          0, 0, 0, 1,
          ri.adv_revenue.action,
          ri.pub_revenue.action,
          ri.isp_revenue.action,
          ri.adv_comm_revenue.action,
          ri.pub_comm_revenue.action,
          ri.adv_payable_comm_amount.action,
          ri.pub_revenue.convert_action(ri.adv_revenue.currency_rate),
          ri.isp_revenue.convert_action(ri.adv_revenue.currency_rate)));
    }

    void log_record_(
      const RequestInfo& ri,
      const CollectorT::DataT::DataT& data)
    {
      CollectorT::KeyT key(ri.time, ri.adv_time);
      CollectorT::DataT add_data;
      CollectorT::DataT::KeyT inner_key(
        ri.colo_id,
        ri.publisher_account_id,
        ri.tag_id,
        ri.size_id ? LogProcessing::OptionalUlong(ri.size_id) : LogProcessing::OptionalUlong(),
        ri.country,
        ri.adv_account_id,
        ri.campaign_id,
        ri.ccg_id,
        ri.cc_id,
        ri.adv_revenue.rate_id,
        ri.isp_revenue.rate_id,
        ri.pub_revenue.rate_id,
        ri.currency_exchange_id,
        ri.tag_delivery_threshold,
        ri.num_shown,
        ri.position,
        ri.test_request,
        ri.fraud == RequestInfo::RS_FRAUD,
        ri.walled_garden,
        ri.user_status,
        ri.geo_channel_id ?
          LogProcessing::OptionalValue<unsigned long>(ri.geo_channel_id) :
          LogProcessing::OptionalValue<unsigned long>(),
        ri.device_channel_id ?
          LogProcessing::OptionalValue<unsigned long>(ri.device_channel_id) :
          LogProcessing::OptionalValue<unsigned long>(),
        ri.ctr_reset_id,
        ri.hid_profile,
        ri.viewability);

      add_data.add(inner_key, data);
      add_record(key, add_data);
    }

  protected:
    virtual ~CreativeStatLogger() noexcept {}

  private:
    const CollectorT::DataT::DataT STAT_REQUEST_ONE_;
    const CollectorT::DataT::DataT STAT_REQUEST_NEGATIVE_ONE_;
  };

  /**
   * ChannelPerformanceLogger
   */
  class ChannelPerformanceLogger:
    public RequestLoggerAdapter<
      AdServer::LogProcessing::ChannelPerformanceTraits>
  {
  public:
    ChannelPerformanceLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : RequestLoggerAdapter<
          AdServer::LogProcessing::ChannelPerformanceTraits>(flush_traits)
    {}

    virtual const char* name() noexcept
    {
      return CHANNEL_PERFORMANCE_LOGGER;
    }

    virtual void process_request_impl(
      const RequestInfo& /*ri*/,
      const ProcessingState&)
      /*throw(eh::Exception)*/
    {}

    virtual void
    process_impression_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      log_record_(
        ri,
        processing_state,
        CollectorT::DataT::DataT(
          0, // requests (ExpressionMatcher fill it)
          1, // imps
          0,
          0,
          ri.adv_revenue.sys_impression()));
    }

    virtual void
    process_click_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      log_record_(
        ri,
        processing_state,
        CollectorT::DataT::DataT(
          0, // requests
          0, // imps
          1, // clicks
          0,
          ri.adv_revenue.sys_click()));
    }

    virtual void
    process_action_impl(const RequestInfo& ri)
      /*throw(eh::Exception)*/
    {
      log_record_(
        ri,
        ProcessingState(),
        CollectorT::DataT::DataT(
          0, // requests
          0, // imps
          0, // clicks
          1, // actions
          ri.adv_revenue.sys_action()));
    }

    void log_record_(
      const RequestInfo& ri,
      const ProcessingState& processing_state,
      const CollectorT::DataT::DataT& data)
    {
      if(!ri.test_request &&
         processing_state.state == RequestInfo::RS_NORMAL &&
         !ri.channels.empty())
      {
        CollectorT::KeyT key(ri.isp_time, ri.colo_id);
        CollectorT::DataT add_data;

        for(RequestInfo::ChannelIdList::const_iterator ch_it =
              ri.channels.begin();
            ch_it != ri.channels.end(); ++ch_it)
        {
          CollectorT::DataT::KeyT inner_key(*ch_it, ri.ccg_id,
            Commons::ImmutableString(ri.tag_size));
          add_data.add(inner_key, data);
        }

        add_record(key, add_data);
      }
    }
  };

  /**
   * ChannelImpInventoryLogger
   */
  class ChannelImpInventoryLogger:
    public RequestLoggerAdapter<
      AdServer::LogProcessing::ChannelImpInventoryTraits>
  {
  public:
    ChannelImpInventoryLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : RequestLoggerAdapter<
          AdServer::LogProcessing::ChannelImpInventoryTraits>(flush_traits),
        ZERO_COUNTER_(
          RevenueDecimal::ZERO,
          RevenueDecimal::ZERO,
          RevenueDecimal::ZERO),
        ONE_IMP_COUNTER_(
          RevenueDecimal(1),
          RevenueDecimal::ZERO,
          RevenueDecimal::ZERO)
    {}

    virtual const char* name() noexcept
    {
      return CHANNEL_IMP_INVENTORY_LOGGER;
    }

    virtual void
    process_request_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      // log imps by unconfirmed impressions (specific for ChannelImpInventory)
      log_record_(
        ri,
        processing_state,
        LogProcessing::ChannelImpInventoryCollector::DataT::DataT(
          0,
          0,
          RevenueDecimal::ZERO, // no revenue
          RevenueDecimal::ZERO,
          ONE_IMP_COUNTER_,
          ZERO_COUNTER_,
          ZERO_COUNTER_));
    }

    virtual void
    process_impression_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      RevenueDecimal sys_imp_revenue = ri.adv_revenue.sys_impression();

      if(sys_imp_revenue != RevenueDecimal::ZERO)
      {
        log_record_(
          ri,
          processing_state,
          LogProcessing::ChannelImpInventoryCollector::DataT::DataT(
            0,
            0,
            ri.adv_revenue.sys_impression(), // impression cost (system currency)
            RevenueDecimal::ZERO,
            ZERO_COUNTER_,
            ZERO_COUNTER_,
            ZERO_COUNTER_));
      }
    }

    virtual void
    process_click_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      log_record_(
        ri,
        processing_state,
        LogProcessing::ChannelImpInventoryCollector::DataT::DataT(
          1, // click
          0,
          ri.adv_revenue.sys_click(), // click cost (system currency)
          RevenueDecimal::ZERO,
          ZERO_COUNTER_,
          ZERO_COUNTER_,
          ZERO_COUNTER_));
    }

    virtual void
    process_action_impl(const RequestInfo& ri)
      /*throw(eh::Exception)*/
    {
      log_record_(
        ri,
        ProcessingState(),
        LogProcessing::ChannelImpInventoryCollector::DataT::DataT(
          0,
          1, // action
          ri.adv_revenue.sys_action(), // action cost (system currency)
          RevenueDecimal::ZERO,
          ZERO_COUNTER_,
          ZERO_COUNTER_,
          ZERO_COUNTER_));
    }

    void log_record_(
      const RequestInfo& ri,
      const ProcessingState& processing_state,
      const CollectorT::DataT::DataT& data)
    {
      typedef CollectorT::DataT::KeyT::CCGType CCGType;

      if(!ri.test_request &&
         processing_state.state == RequestInfo::RS_NORMAL &&
         !ri.channels.empty())
      {
        const CollectorT::KeyT key(ri.isp_time, ri.colo_id);
        CollectorT::DataT add_data;

        for(RequestInfo::ChannelIdList::const_iterator ch_it =
              ri.channels.begin();
            ch_it != ri.channels.end(); ++ch_it)
        {
          CollectorT::DataT::KeyT inner_key(
            *ch_it, ri.text_campaign ? CCGType::TEXT : CCGType::DISPLAY);
          add_data.add(inner_key, data);
        }

        add_record(key, add_data);
      }
    }

  private:
    const LogProcessing::ChannelImpInventoryCollector::
      DataT::DataT::Counter ZERO_COUNTER_;
    const LogProcessing::ChannelImpInventoryCollector::
      DataT::DataT::Counter ONE_IMP_COUNTER_;
  };

  /**
   * TagPositionStatLogger
   */
  class TagPositionStatLogger:
    public RequestLoggerAdapter<
      AdServer::LogProcessing::TagPositionStatTraits>
  {
  public:
    TagPositionStatLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : RequestLoggerAdapter<
          AdServer::LogProcessing::TagPositionStatTraits>(flush_traits),
        ONE_IMPRESSION_(
          0, // requests
          1, // impressions
          0  // clicks
          ),
        ONE_CLICK_(
          0, // requests
          0, // impressions
          1  // clicks
          )
    {}

    virtual const char* name() noexcept
    {
      return TAG_POSITION_STAT_LOGGER;
    }

    virtual void
    process_request_impl(
      const RequestInfo&,
      const ProcessingState&)
      /*throw(eh::Exception)*/
    {}

    virtual void
    process_impression_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      if(ri.position == 1)
      {
        log_record_(ri, processing_state, ONE_IMPRESSION_);
      }
    }

    virtual void
    process_click_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      log_record_(ri, processing_state, ONE_CLICK_);
    }

    virtual void
    process_action_impl(const RequestInfo&)
      /*throw(eh::Exception)*/
    {}

  protected:
    void log_record_(
      const RequestInfo& ri,
      const ProcessingState& processing_state,
      const CollectorT::DataT::DataT& data)
    {
      typedef CollectorT::DataT::KeyT::OptionalUlong OptionalUlong;

      if(processing_state.state == RequestInfo::RS_NORMAL)
      {
        CollectorT::KeyT key(ri.pub_time, ri.colo_id);
        CollectorT::DataT add_data;
        add_data.add(
          CollectorT::DataT::KeyT(
            ri.tag_id,
            ri.tag_top_offset.present() ?
              OptionalUlong(*ri.tag_top_offset) :
              OptionalUlong(),
            ri.tag_left_offset.present() ?
              OptionalUlong(*ri.tag_left_offset) :
              OptionalUlong(),
            ri.tag_visibility.present() ?
              OptionalUlong(*ri.tag_visibility) :
              OptionalUlong(),
            ri.test_request),
          data);
        add_record(key, add_data);
      }
    }

  private:
    const LogProcessing::TagPositionStatCollector::
      DataT::DataT ONE_IMPRESSION_;
    const LogProcessing::TagPositionStatCollector::
      DataT::DataT ONE_CLICK_;
  };

  /**
   * ExpressionPerformanceLogger
   */
  class ExpressionPerformanceLogger:
    public RequestLoggerAdapter<
      AdServer::LogProcessing::ExpressionPerformanceTraits>
  {
  public:
    ExpressionPerformanceLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : RequestLoggerAdapter<
          AdServer::LogProcessing::ExpressionPerformanceTraits>(flush_traits),
        STAT_IMP_ONE_(1, 0, 0),
        STAT_CLICK_ONE_(0, 1, 0),
        STAT_ACTION_ONE_(0, 0, 1)
    {}

    virtual const char* name() noexcept
    {
      return EXPRESSION_PERFORMANCE_LOGGER;
    }

    virtual void process_request_impl(
      const RequestInfo&,
      const ProcessingState&)
      /*throw(eh::Exception)*/
    {}

    virtual void
    process_impression_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      log_record_(ri, processing_state, STAT_IMP_ONE_);
    }

    virtual void
    process_click_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      log_record_(ri, processing_state, STAT_CLICK_ONE_);
    }

    virtual void
    process_action_impl(
      const RequestInfo& ri)
      /*throw(eh::Exception)*/
    {
      log_record_(ri, ProcessingState(), STAT_ACTION_ONE_);
    }

    void log_record_(
      const RequestInfo& ri,
      const ProcessingState& processing_state,
      const CollectorT::DataT::DataT& data)
    {
      if(!ri.expression.empty() &&
         !ri.test_request &&
         processing_state.state == RequestInfo::RS_NORMAL)
      {
        CollectorT::KeyT key(ri.time, ri.colo_id);
        CollectorT::DataT add_data;
        CollectorT::DataT::KeyT inner_key(ri.cc_id, ri.expression);
        add_data.add(inner_key, data);
        add_record(key, add_data);
      }
    }

  private:
    const CollectorT::DataT::DataT STAT_IMP_ONE_;
    const CollectorT::DataT::DataT STAT_CLICK_ONE_;
    const CollectorT::DataT::DataT STAT_ACTION_ONE_;
  };

  /**
   * CCGKeywordStatLogger
   */
  class CCGKeywordStatLogger:
    public RequestLoggerAdapter<
      AdServer::LogProcessing::CcgKeywordStatTraits>
  {
  public:
    CCGKeywordStatLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : RequestLoggerAdapter<
          AdServer::LogProcessing::CcgKeywordStatTraits>(flush_traits)
    {}

    virtual const char* name() noexcept
    {
      return CCG_KEYWORD_STAT_LOGGER;
    }

    virtual void process_request_impl(
      const RequestInfo&,
      const ProcessingState&)
      /*throw(eh::Exception)*/
    {}

    virtual void process_impression_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      if(ri.ccg_keyword_id)
      {
        log_record_(
          ri,
          processing_state,
          CollectorT::DataT::DataT(
            1,
            0,
            ri.adv_revenue.impression,
            ri.adv_comm_revenue.impression,
            ri.pub_revenue.convert_impression(ri.adv_revenue.currency_rate)));
      }
    }

    virtual void
    process_click_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      if(ri.ccg_keyword_id)
      {
        log_record_(
          ri,
          processing_state,
          CollectorT::DataT::DataT(
            0,
            1,
            ri.adv_revenue.click,
            ri.adv_comm_revenue.click,
            ri.pub_revenue.convert_click(ri.adv_revenue.currency_rate)));
      }
    }

    virtual void
    process_action_impl(const RequestInfo&)
      /*throw(eh::Exception)*/
    {}

    void log_record_(
      const RequestInfo& ri,
      const ProcessingState& processing_state,
      const CollectorT::DataT::DataT& data)
    {
      if(!ri.test_request &&
        processing_state.state == RequestInfo::RS_NORMAL)
      {
        CollectorT::KeyT key(ri.time, ri.colo_id);
        CollectorT::DataT add_data;
        CollectorT::DataT::KeyT inner_key(
          ri.ccg_keyword_id, ri.currency_exchange_id, ri.cc_id);
        add_data.add(inner_key, data);
        add_record(key, add_data);
      }
    }
  };

  /**
   * CmpStatLogger
   */
  class CmpStatLogger:
    public RequestLoggerAdapter<
      AdServer::LogProcessing::CmpStatTraits>
  {
  public:
    CmpStatLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : RequestLoggerAdapter<
          AdServer::LogProcessing::CmpStatTraits>(flush_traits)
    {}

    virtual const char* name() noexcept
    {
      return CMP_STAT_LOGGER;
    }

    virtual void
    process_request_impl(
      const RequestInfo&,
      const ProcessingState&)
      /*throw(eh::Exception)*/
    {}

    virtual void
    process_impression_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      if(processing_state.state == RequestInfo::RS_NORMAL ||
        processing_state.state == RequestInfo::RS_RESAVE)
      {
        for(RequestInfo::ChannelRevenueList::const_iterator ch_it =
              ri.cmp_channels.begin();
            ch_it != ri.cmp_channels.end();
            ++ch_it)
        {
          log_record_(ri, processing_state, *ch_it,
            CollectorT::DataT::DataT(
              1,
              0,
              ch_it->impression,
              ch_it->adv_impression,
              ch_it->sys_impression));
        }
      }
      else if(processing_state.state == RequestInfo::RS_FRAUD ||
        processing_state.state == RequestInfo::RS_MOVED ||
        processing_state.state == RequestInfo::RS_ROLLBACK)
      {
        for(RequestInfo::ChannelRevenueList::const_iterator ch_it =
              ri.cmp_channels.begin();
            ch_it != ri.cmp_channels.end();
            ++ch_it)
        {
          log_record_(ri, processing_state, *ch_it,
            CollectorT::DataT::DataT(
              -1,
              0,
              RevenueDecimal(ch_it->impression).negate(),
              RevenueDecimal(ch_it->adv_impression).negate(),
              RevenueDecimal(ch_it->sys_impression).negate()));
        }
      }
    }

    virtual void
    process_click_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(eh::Exception)*/
    {
      if(processing_state.state == RequestInfo::RS_NORMAL ||
        processing_state.state == RequestInfo::RS_RESAVE)
      {
        for(RequestInfo::ChannelRevenueList::const_iterator ch_it =
              ri.cmp_channels.begin();
            ch_it != ri.cmp_channels.end();
            ++ch_it)
        {
          log_record_(ri, processing_state, *ch_it,
            CollectorT::DataT::DataT(
              0,
              1,
              ch_it->click,
              ch_it->adv_click,
              ch_it->sys_click));
      }
      }
      else if(processing_state.state == RequestInfo::RS_FRAUD ||
        processing_state.state == RequestInfo::RS_MOVED ||
        processing_state.state == RequestInfo::RS_ROLLBACK)
      {
        for(RequestInfo::ChannelRevenueList::const_iterator ch_it =
              ri.cmp_channels.begin();
            ch_it != ri.cmp_channels.end();
            ++ch_it)
        {
          log_record_(ri, processing_state, *ch_it,
            CollectorT::DataT::DataT(
              0,
              -1,
              RevenueDecimal(ch_it->click).negate(),
              RevenueDecimal(ch_it->adv_click).negate(),
              RevenueDecimal(ch_it->sys_click).negate()));
        }
      }
    }

    virtual void
    process_action_impl(const RequestInfo&)
      /*throw(eh::Exception)*/
    {}

    void log_record_(
      const RequestInfo& ri,
      const ProcessingState& processing_state,
      const RequestInfo::ChannelRevenue& ch_revenue,
      const CollectorT::DataT::DataT& data)
    {
      if(!ri.test_request)
      {
        CollectorT::KeyT key(
          ri.time,
          ri.adv_time,
          ri.colo_id);
        CollectorT::DataT::KeyT inner_key(
          ri.publisher_account_id,
          ri.tag_id,
          ri.size_id,
          ri.country,
          ri.currency_exchange_id,
          ri.tag_delivery_threshold,
          ri.adv_account_id,
          ri.campaign_id,
          ri.ccg_id,
          ri.cc_id,
          ch_revenue.channel_id,
          ch_revenue.channel_rate_id,
          processing_state.state != RequestInfo::RS_NORMAL,
          ri.walled_garden);
        CollectorT::DataT add_data;
        add_data.add(inner_key, data);
        add_record(key, add_data);
      }
    }
  };

  /**
   * ActionStatLogger
   */
  class ActionStatLogger:
    public RequestLoggerAdapter<
      AdServer::LogProcessing::ActionStatTraits>
  {
  public:
    ActionStatLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : RequestLoggerAdapter<
          AdServer::LogProcessing::ActionStatTraits>(flush_traits)
    {}

    virtual const char* name() noexcept
    {
      return ACTION_STAT_LOGGER;
    }

    virtual void process_request_impl(
      const RequestInfo& /*ri*/,
      const ProcessingState&)
      /*throw(eh::Exception)*/
    {}

    virtual void process_impression_impl(
      const RequestInfo& /*ri*/,
      const ProcessingState&)
      /*throw(eh::Exception)*/
    {}

    virtual void process_click_impl(
      const RequestInfo&,
      const ProcessingState&)
      /*throw(eh::Exception)*/
    {}

    virtual void
    process_action_impl(const RequestInfo&)
      /*throw(eh::Exception)*/
    {}

    virtual void process_custom_action_impl(
      const RequestInfo& ri,
      const AdvCustomActionInfo& adv_custom_action_info)
      /*throw(eh::Exception)*/
    {
      if(!ri.test_request)
      {
        CollectorT::KeyT key(adv_custom_action_info.time, ri.colo_id);
        CollectorT::DataT data;

        CollectorT::DataT::KeyT inner_key(
          adv_custom_action_info.action_request_id,
          ri.request_id,
          ri.cc_id);

        if(ri.click_time != Generics::Time::ZERO)
        {
          CollectorT::DataT::DataT inner_data(
            adv_custom_action_info.action_id,
            ri.tag_id,
            adv_custom_action_info.order_id,
            ri.country,
            adv_custom_action_info.referer,
            ri.imp_time,
            ri.click_time,
            adv_custom_action_info.action_value,
            ri.device_channel_id);

          data.add(inner_key, inner_data);
        }
        else
        {
          CollectorT::DataT::DataT inner_data(
            adv_custom_action_info.action_id,
            ri.tag_id,
            adv_custom_action_info.order_id,
            ri.country,
            adv_custom_action_info.referer,
            ri.imp_time,
            adv_custom_action_info.action_value,
            ri.device_channel_id);

          data.add(inner_key, inner_data);
        }

        add_record(key, data);
      }
    }
  };

  /**
   * CCGUserStatLogger
   */
  class CCGUserStatLogger:
    public CampaignReachLoggerAdapter<
      AdServer::LogProcessing::CcgUserStatTraits>
  {
  public:
    CCGUserStatLogger(
      const LogProcessing::LogFlushTraits& flush_traits,
      unsigned long colo_id)
      /*throw(eh::Exception)*/
      : CampaignReachLoggerAdapter<
          AdServer::LogProcessing::CcgUserStatTraits>(flush_traits, colo_id)
    {}

    virtual const char*
    name() noexcept
    {
      return CCG_USER_STAT_LOGGER;
    }

    virtual void
    process_reach(const ReachInfo& ri)
      /*throw(CampaignReachProcessor::Exception)*/
    {
      add_reach_records_(
        ri.ccgs,
        ColoIdAdapter<CollectorT::KeyT>(),
        NullValueAdapter());
    }

  protected:
    virtual
    ~CCGUserStatLogger() noexcept
    {}
  };

  /**
   * CCUserStatLogger
   */
  class CCUserStatLogger:
    public virtual CampaignReachLoggerAdapter<
      AdServer::LogProcessing::CcUserStatTraits>
  {
  public:
    CCUserStatLogger(
      const LogProcessing::LogFlushTraits& flush_traits,
      unsigned long colo_id)
      /*throw(eh::Exception)*/
      : CampaignReachLoggerAdapter<
          AdServer::LogProcessing::CcUserStatTraits>(flush_traits, colo_id)
    {}

    virtual const char*
    name() noexcept
    {
      return CC_USER_STAT_LOGGER;
    }

    virtual void
    process_reach(const ReachInfo& ri)
      /*throw(CampaignReachProcessor::Exception)*/
    {
      add_reach_records_(
        ri.creatives,
        ColoIdAdapter<CollectorT::KeyT>(),
        NullValueAdapter());
    }

    protected:
      virtual
      ~CCUserStatLogger() noexcept
      {}
  };

  /**
   * CampaignUserStatLogger
   */
  class CampaignUserStatLogger:
    public virtual CampaignReachLoggerAdapter<
      AdServer::LogProcessing::CampaignUserStatTraits>
  {
  public:
    CampaignUserStatLogger(
      const LogProcessing::LogFlushTraits& flush_traits,
      unsigned long colo_id)
      /*throw(eh::Exception)*/
      : CampaignReachLoggerAdapter<
          AdServer::LogProcessing::CampaignUserStatTraits>(flush_traits, colo_id)
    {}

    virtual const char*
    name() noexcept
    {
      return CAMPAIGN_USER_STAT_LOGGER;
    }

    virtual void
    process_reach(const ReachInfo& ri)
      /*throw(CampaignReachProcessor::Exception)*/
    {
      add_reach_records_(
        ri.campaigns,
        ColoIdAdapter<CollectorT::KeyT>(),
        NullValueAdapter());
    }

  protected:
    virtual
    ~CampaignUserStatLogger() noexcept
    {}
  };

  /**
   * AdvertiserUserStatLogger
   */
  class AdvertiserUserStatLogger:
    public CampaignReachLoggerAdapter<
      AdServer::LogProcessing::AdvertiserUserStatTraits>
  {
  public:
    AdvertiserUserStatLogger(
      const LogProcessing::LogFlushTraits& flush_traits,
      unsigned long colo_id)
      /*throw(eh::Exception)*/
      : CampaignReachLoggerAdapter<
          AdServer::LogProcessing::AdvertiserUserStatTraits>(flush_traits, colo_id)
    {}

    virtual const char*
    name() noexcept
    {
      return ADVERTISER_USER_STAT_LOGGER;
    }

    virtual void
    process_reach(const ReachInfo& ri)
      /*throw(CampaignReachProcessor::Exception)*/
    {
      add_reach_records_(
        ri.advertisers,
        NullValueAdapter(),
        UniqueUsersAdapter());
      add_reach_records_(
        ri.text_advertisers,
        NullValueAdapter(),
        TextUniqueUsersAdapter());
      add_reach_records_(
        ri.display_advertisers,
        NullValueAdapter(),
        DisplayUniqueUsersAdapter());
    }

  protected:
    virtual
    ~AdvertiserUserStatLogger() noexcept
    {}

  protected:
    struct UniqueUsersAdapter
    {
      CollectorT::DataT::DataT
      operator()(unsigned long counter) const
      {
        return CollectorT::DataT::DataT(counter, 0, 0);
      }
    };

    struct TextUniqueUsersAdapter
    {
      CollectorT::DataT::DataT
      operator()(unsigned long counter) const
      {
        return CollectorT::DataT::DataT(0, counter, 0);
      }
    };

    struct DisplayUniqueUsersAdapter
    {
      CollectorT::DataT::DataT
      operator()(unsigned long counter) const
      {
        return CollectorT::DataT::DataT(0, 0, counter);
      }
    };
  };

  /** PassbackStatLogger */
  class PassbackStatLogger:
    public virtual PassbackProcessor,
    public virtual AdServer::LogProcessing::LogHolderPool<
      AdServer::LogProcessing::PassbackStatTraits>,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    PassbackStatLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : AdServer::LogProcessing::LogHolderPool<
          AdServer::LogProcessing::PassbackStatTraits>(flush_traits)
    {}

    virtual void process_passback(const PassbackInfo& pi)
      /*throw(PassbackProcessor::Exception)*/
    {
      static const char* FUN = "PassbackStatLogger::process_passback()";

      try
      {
        CollectorT::KeyT key(pi.time, pi.colo_id);

        CollectorT::DataT data;
        data.add(
          CollectorT::DataT::KeyT(pi.user_status, pi.country,
            pi.tag_id,
            pi.size_id ? LogProcessing::OptionalUlong(pi.size_id) :
              LogProcessing::OptionalUlong()),
          CollectorT::DataT::DataT(1));
        add_record(key, data);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught eh::Exception: " << ex.what();
        throw PassbackProcessor::Exception(ostr);
      }
    }

  protected:
    virtual ~PassbackStatLogger() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<PassbackStatLogger>
    PassbackStatLogger_var;

  /**
   * SiteUserStatLogger
   */
  class SiteUserStatLogger:
    public virtual SiteReachLoggerBase,
    public virtual AdServer::LogProcessing::LogHolderPool<
      AdServer::LogProcessing::SiteUserStatTraits>,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    SiteUserStatLogger(
      const LogProcessing::LogFlushTraits& flush_traits,
      unsigned long colo_id)
      /*throw(eh::Exception)*/
      : AdServer::LogProcessing::LogHolderPool<
          AdServer::LogProcessing::SiteUserStatTraits>(flush_traits),
        colo_id_(colo_id)
    {}

    virtual void
    process_site_reach(const SiteReachInfo& ri)
      /*throw(SiteReachProcessor::Exception)*/
    {
      static const char* FUN = "SiteUserStatLogger::process_site_reach()";

      try
      {
        for(IdAppearanceList::const_iterator site_app_it =
              ri.appearance_list.begin();
            site_app_it != ri.appearance_list.end(); ++site_app_it)
        {
          CollectorT::KeyT key(
            site_app_it->date, // isp timezone
            colo_id_);

          CollectorT::DataT data;

          CollectorT::DataT::KeyT inner_key(
            site_app_it->id, site_app_it->last_appearance_date);
          data.add(inner_key, CollectorT::DataT::DataT(site_app_it->counter));

          add_record(key, data);
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught eh::Exception: " << ex.what();
        throw SiteReachProcessor::Exception(ostr);
      }
    }

  private:
    unsigned long colo_id_;
  };

  typedef ReferenceCounting::SmartPtr<SiteUserStatLogger>
    SiteUserStatLogger_var;

  /**
   * SiteRefererStatLogger
   */
  class SiteRefererStatLogger:
    public TagRequestProcessor,
    public RequestActionProcessor,
    public PassbackProcessor,
    /*
    public AdServer::LogProcessing::LogHolderPool<
      AdServer::LogProcessing::SiteReferrerStatTraits>,
    */
    public AdServer::LogProcessing::LogHolderPortioned<
      AdServer::LogProcessing::SiteReferrerStatTraits>,
    public virtual ReferenceCounting::AtomicImpl
  {
    static const LogProcessing::FixedNumber&
    fzero_() { return LogProcessing::FixedNumber::ZERO; }

  public:
    SiteRefererStatLogger(
      const LogProcessing::LogFlushTraits& flush_traits,
      Commons::LogReferrer::Setting site_referrer_stats_log_referrer_setting,
      Generics::TaskRunner* task_runner)
      /*throw(eh::Exception)*/
      : /*
        AdServer::LogProcessing::LogHolderPool<
          AdServer::LogProcessing::SiteReferrerStatTraits>(flush_traits),
        */
        AdServer::LogProcessing::LogHolderPortioned<
          AdServer::LogProcessing::SiteReferrerStatTraits>(
            flush_traits,
            AdServer::LogProcessing::DefaultSavePolicy<AdServer::LogProcessing::SiteReferrerStatTraits>(),
            task_runner,
            8 // threads
            ),
        log_referrer_setting_(site_referrer_stats_log_referrer_setting),
        ONE_AD_SHOWN_REQUEST_(
          1, // requests
          0, 0, 0, 0, 0,
          0, // no_bid_count
          fzero_(), // floor_won_cost
          fzero_(), // floor_lost_cost
          fzero_(), // floor_no_bid_cost
          fzero_(), // bid_won_amount
          fzero_(), // bid_lost_amount
          fzero_()),
        ONE_CLICK_(0, 0, 1, 0, 0, 0, 0, fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_()),
        ONE_PASSBACK_(0, 0, 0, 1, 0, 0, 0, fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())
    {}

    virtual void
    process_tag_request(
      const TagRequestInfo& tag_request_info)
      /*throw(TagRequestProcessor::Exception)*/
    {
      static const char* FUN = "SiteReferrerStatLogger::process_tag_request()";

      try
      {
        CollectorT::DataT data;
        data.add(
          CollectorT::DataT::KeyT(
            tag_request_info.user_status,
            tag_request_info.tag_id,
            tag_request_info.ext_tag_id,
            normalize_referer_(tag_request_info.referer)),
          tag_request_info.ad_shown ?
            ONE_AD_SHOWN_REQUEST_ :
          CollectorT::DataT::DataT(
              1,
              0,
              0,
              0,
              0, // bids_won_count
              0, // bids_lost_count
              1, // no_bid_count
              LogProcessing::FixedNumber::ZERO, // floor_won_cost
              LogProcessing::FixedNumber::ZERO, // floor_lost_cost
              tag_request_info.floor_cost, // floor_no_bid_cost
              LogProcessing::FixedNumber::ZERO, // bid_won_amount
              LogProcessing::FixedNumber::ZERO, // bid_lost_amount
              LogProcessing::FixedNumber::ZERO));

        add_record(
          CollectorT::KeyT(tag_request_info.isp_time, tag_request_info.colo_id),
          data);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw TagRequestProcessor::Exception(ostr);
      }
    }

    virtual void
    process_request(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      if(ri.position == 1 || ri.pub_revenue.impression != RevenueDecimal::ZERO)
      {
        // save impression only on first position creative confirmation
        // other positions we process for collect only full cost
        add_request_info_record_(
          ri,
          processing_state,
          CollectorT::DataT::DataT(
            0,
            0,
            0,
            0,
            0, // bids_won_count
            ri.position == 1 ? 1 : 0, // bids_lost_count
            0, // no_bid_count
            LogProcessing::FixedNumber::ZERO, // floor_won_cost
            ri.pub_floor_cost, // floor_lost_cost
            LogProcessing::FixedNumber::ZERO, // floor_no_bid_cost
            LogProcessing::FixedNumber::ZERO, // bid_won_amount
            ri.pub_bid_cost, // bid_lost_amount
            LogProcessing::FixedNumber::ZERO  // cost
            ));
      }
    }

    virtual void
    process_impression(
      const RequestInfo& ri,
      const ImpressionInfo&,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      if(ri.position == 1 || ri.pub_revenue.impression != RevenueDecimal::ZERO)
      {
        // save impression only on first creative confirmation
        add_request_info_record_(
          ri,
          processing_state,
          CollectorT::DataT::DataT(
            0,
            ri.position == 1 ? 1 : 0,
            0,
            0,
            ri.position == 1 ? 1 : 0, // bids_won_count
            ri.position == 1 ? -1 : 0, // bids_lost_count
            0, // no_bid_count
            ri.pub_floor_cost, // floor_won_cost
            LogProcessing::FixedNumber(ri.pub_floor_cost).negate(), // floor_lost_cost
            LogProcessing::FixedNumber::ZERO, // floor_no_bid_cost
            ri.pub_bid_cost, // bid_won_amount
            LogProcessing::FixedNumber(ri.pub_bid_cost).negate(), // bid_lost_amount
            ri.pub_revenue.impression
            ));
      }
    }

    virtual void
    process_click(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      add_request_info_record_(ri, processing_state, ONE_CLICK_);
    }

    virtual void
    process_action(const RequestInfo&)
      /*throw(RequestActionProcessor::Exception)*/
    {}

    virtual void
    process_passback(const PassbackInfo& pi)
      /*throw(PassbackProcessor::Exception)*/
    {
      static const char* FUN = "SiteReferrerStatLogger::process_passback()";

      try
      {
        CollectorT::DataT data;
        data.add(
          CollectorT::DataT::KeyT(
            pi.user_status,
            pi.tag_id,
            pi.ext_tag_id,
            normalize_referer_(pi.referer)),
          ONE_PASSBACK_);

        add_record(
          CollectorT::KeyT(pi.time, pi.colo_id),
          data);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw PassbackProcessor::Exception(ostr);
      }
    }

  private:
    std::string
    normalize_referer_(const String::SubString& referer)
      const
    {
      return Commons::LogReferrer::normalize_referrer(
        referer,
        log_referrer_setting_,
        "-", // in case of empty referer
        true); // truncate path for DB
    }

    void
    add_request_info_record_(
      const RequestInfo& ri,
      const ProcessingState& processing_state,
      const CollectorT::DataT::DataT& inner_data)
      /*throw(RequestActionProcessor::Exception)*/
    {
      static const char* FUN = "SiteReferrerStatLogger::add_request_info_record_()";

      if(!ri.test_request && processing_state.state == RequestInfo::RS_NORMAL)
      {
        try
        {
          CollectorT::DataT data;

          data.add(
            CollectorT::DataT::KeyT(
              ri.user_status,
              ri.tag_id,
              ri.ext_tag_id,
              normalize_referer_(ri.referer)),
            inner_data);

          add_record(CollectorT::KeyT(ri.isp_time, ri.colo_id), data);
        }
        catch (const eh::Exception &ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << ex.what();
          throw RequestActionProcessor::Exception(ostr);
        }
      }
    }

  private:
    const Commons::LogReferrer::Setting log_referrer_setting_;
    const CollectorT::DataT::DataT ONE_AD_SHOWN_REQUEST_;
    const CollectorT::DataT::DataT ONE_CLICK_;
    const CollectorT::DataT::DataT ONE_PASSBACK_;
  };

  /**
   * CampaignReferrerStatLogger
   */
  class CampaignReferrerStatLogger:
    public RequestLoggerAdapter<
      AdServer::LogProcessing::CampaignReferrerStatTraits>,
    public virtual ReferenceCounting::AtomicImpl
  {
    static const LogProcessing::FixedNumber&
    fzero_() { return LogProcessing::FixedNumber::ZERO; }

  public:
    CampaignReferrerStatLogger(
      const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : RequestLoggerAdapter<
          AdServer::LogProcessing::CampaignReferrerStatTraits>(flush_traits)
    {
      action_data_.insert(
        std::make_pair("vstart", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("vview", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("vq1", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("vmid", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("vq3", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("vcomplete", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("vskip", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("vpause", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("vmute", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("vunmute", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("vresume", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("vfullscreen", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
      action_data_.insert(
        std::make_pair("verror", CollectorT::DataT::DataT(
          0, // imps
          0, // clicks
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
          fzero_(), fzero_(), fzero_(), fzero_(), fzero_(), fzero_())));
    }

    virtual const char*
    name() noexcept
    {
      return CAMPAIGN_REFERRER_STAT_LOGGER;
    }

    virtual void
    process_request_impl(
      const RequestInfo& /*ri*/,
      const ProcessingState& /*processing_state*/)
      /*throw(RequestActionProcessor::Exception)*/
    {}

    virtual void
    process_impression_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      add_request_info_record_(
        ri,
        processing_state,
        CollectorT::DataT::DataT(
          1, // imps
          0, // clicks
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          ri.adv_revenue.impression, // adv_amount
          ri.adv_comm_revenue.impression, // adv_comm_amount
          ri.adv_payable_comm_amount.impression, // adv_payable_comm_amount
          ri.pub_revenue.convert_impression(ri.adv_revenue.currency_rate), // pub_amount_adv
          RequestInfo::Revenue::convert_currency(
            ri.pub_comm_revenue.impression,
            ri.pub_revenue.currency_rate,
            ri.adv_revenue.currency_rate), // pub_comm_amount_adv
          RequestInfo::Revenue::convert_currency(
            ri.isp_revenue.impression,
            ri.isp_revenue.currency_rate,
            ri.adv_revenue.currency_rate) // isp_amount_adv
          ));
    }

    virtual void
    process_click_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      add_request_info_record_(
        ri,
        processing_state,
        CollectorT::DataT::DataT(
          0, // imps
          1, // clicks
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          ri.adv_revenue.click, // adv_amount
          ri.adv_comm_revenue.click, // adv_comm_amount
          ri.adv_payable_comm_amount.impression, // adv_payable_comm_amount
          ri.pub_revenue.convert_click(ri.adv_revenue.currency_rate), // pub_amount_adv
          RequestInfo::Revenue::convert_currency(
            ri.pub_comm_revenue.click,
            ri.pub_revenue.currency_rate,
            ri.adv_revenue.currency_rate), // pub_comm_amount_adv
          RequestInfo::Revenue::convert_currency(
            ri.isp_revenue.click,
            ri.isp_revenue.currency_rate,
            ri.adv_revenue.currency_rate) // isp_amount_adv
          ));
    }

    virtual void
    process_action_impl(const RequestInfo&)
      /*throw(RequestActionProcessor::Exception)*/
    {}

    virtual void
    process_request_post_action_impl(
      const RequestInfo& ri,
      const RequestPostActionInfo& request_post_action_info)
      /*throw(RequestActionProcessor::Exception)*/
    {
      auto it = action_data_.find(request_post_action_info.action_name);
      if(it != action_data_.end())
      {
        add_request_info_record_(
          ri,
          ProcessingState(RequestInfo::RS_NORMAL),
          it->second);
      }
    }

  protected:
    virtual
    ~CampaignReferrerStatLogger() noexcept
    {}

  private:
    typedef Generics::GnuHashTable<
      Generics::StringHashAdapter,
      CollectorT::DataT::DataT> ActionNameDataMap;

  private:
    static std::string
    normalize_referer_(const String::SubString& referer)
    {
      try
      {
        HTTP::BrowserAddress addr(referer);
        if(!addr.host().empty())
        {
          return addr.host().str();
        }
      }
      catch(const eh::Exception&)
      {}

      return std::string();
    }

    void
    add_request_info_record_(
      const RequestInfo& ri,
      const ProcessingState& processing_state,
      const CollectorT::DataT::DataT& inner_data)
      /*throw(RequestActionProcessor::Exception)*/
    {
      static const char* FUN = "CampaignReferrerStatLogger::add_request_info_record_()";

      if(!ri.test_request && processing_state.state == RequestInfo::RS_NORMAL)
      {
        try
        {
          CollectorT::DataT data;

          data.add(
            CollectorT::DataT::KeyT(
              ri.ccg_id,
              ri.cc_id,
              ri.site_id,
              ri.ext_tag_id,
              normalize_referer_(ri.referer)),
            inner_data);

          add_record(CollectorT::KeyT(ri.adv_time), data);
        }
        catch (const eh::Exception &ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << ex.what();
          throw RequestActionProcessor::Exception(ostr);
        }
      }
    }

  private:
    ActionNameDataMap action_data_;
  };

  /**
   * BidCostStatLogger
   */
  class BidCostStatLogger:
    public RequestLoggerAdapter<
      AdServer::LogProcessing::BidCostStatTraits>,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    BidCostStatLogger(
      const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : RequestLoggerAdapter<
          AdServer::LogProcessing::BidCostStatTraits>(flush_traits)
    {
    }

    virtual const char*
    name() noexcept
    {
      return BID_COST_STAT_LOGGER;
    }

    virtual void
    process_request_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      add_request_info_record_(
        ri,
        processing_state,
        CollectorT::DataT::DataT(
          1, // unverified_imps
          0, // imps
          0 // clicks
          ),
        // use as cost pub_revenue.impression before auction correction &
        // as it sent to SSP
        //RevenueDecimal::mul(ri.pub_bid_cost, REVENUE_ONE + ri.pub_cost_coef, Generics::DMR_FLOOR)
        ri.pub_bid_cost
        );
    }

    virtual void
    process_impression_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      /*
      RevenueDecimal req_bid_cost = RevenueDecimal::mul(
        ri.pub_bid_cost, REVENUE_ONE + ri.pub_cost_coef, Generics::DMR_FLOOR);
      RevenueDecimal imp_cost = RevenueDecimal::mul(
        ri.pub_revenue.impression,
        REVENUE_ONE + ri.pub_cost_coef, Generics::DMR_FLOOR);
      */
      RevenueDecimal req_bid_cost = ri.pub_bid_cost;
      RevenueDecimal imp_cost = ri.pub_revenue.impression;

      if(req_bid_cost != imp_cost)
      {
        // rollback req_bid_cost & save bid with imp_cost
        add_request_info_record_(
          ri,
          processing_state,
          CollectorT::DataT::DataT(
            -1, // unverified_imps
            0, // imps
            0 // clicks
            ),
          req_bid_cost);

        add_request_info_record_(
          ri,
          processing_state,
          CollectorT::DataT::DataT(
            1, // unverified_imps
            0, // imps
            0 // clicks
            ),
          imp_cost);
      }

      add_request_info_record_(
        ri,
        processing_state,
        CollectorT::DataT::DataT(
          0, // unverified_imps
          1, // imps
          0 // clicks
          ),
        imp_cost);
    }

    virtual void
    process_click_impl(
      const RequestInfo& ri,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      add_request_info_record_(
        ri,
        processing_state,
        CollectorT::DataT::DataT(
          0, // unverified_imps
          0, // imps
          1 // clicks
          ),
        REVENUE_ONE);
    }

    virtual void
    process_action_impl(const RequestInfo&)
      /*throw(RequestActionProcessor::Exception)*/
    {}

    virtual void
    process_request_post_action_impl(
      const RequestInfo&,
      const RequestPostActionInfo&)
      /*throw(RequestActionProcessor::Exception)*/
    {
    }

  protected:
    virtual
    ~BidCostStatLogger() noexcept
    {}

  private:
    static std::string
    normalize_referer_(const String::SubString& referer)
    {
      try
      {
        HTTP::BrowserAddress addr(referer);
        if(!addr.host().empty())
        {
          return addr.host().str();
        }
      }
      catch(const eh::Exception&)
      {}

      return std::string();
    }

    void
    add_request_info_record_(
      const RequestInfo& ri,
      const ProcessingState& processing_state,
      const CollectorT::DataT::DataT& inner_data,
      const RevenueDecimal& cost)
      /*throw(RequestActionProcessor::Exception)*/
    {
      static const char* FUN = "BidCostStatLogger::add_request_info_record_()";

      if(!ri.test_request && processing_state.state == RequestInfo::RS_NORMAL)
      {
        try
        {
          CollectorT::DataT data;

          data.add(
            CollectorT::DataT::KeyT(
              ri.tag_id,
              ri.ext_tag_id,
              normalize_referer_(ri.referer),
              RevenueDecimal(cost).ceil(6)
              ),
            inner_data);

          add_record(CollectorT::KeyT(ri.adv_time), data);
        }
        catch (const eh::Exception &ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << ex.what();
          throw RequestActionProcessor::Exception(ostr);
        }
      }
    }
  };

  /**
   * PageLoadsDailyStatLogger
   */
  class PageLoadsDailyStatLogger:
    public virtual TagRequestGroupProcessor,
    public virtual AdServer::LogProcessing::LogHolderPool<
      AdServer::LogProcessing::PageLoadsDailyStatTraits>,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    PageLoadsDailyStatLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : AdServer::LogProcessing::LogHolderPool<
          AdServer::LogProcessing::PageLoadsDailyStatTraits>(flush_traits),
        ONE_PAGE_LOAD_(1, 0),
        ONE_PAGE_LOAD_ROLLBACK_(-1, 0),
        ONE_UTILIZED_PAGE_LOAD_(1, 1),
        ONE_UTILIZED_PAGE_LOAD_ROLLBACK_(-1, -1)
    {}

    virtual void
    process_tag_request_group(
      const TagRequestGroupInfo& tag_request_group_info)
      /*throw(TagRequestGroupProcessor::Exception)*/
    {
      static const char* FUN = "PageLoadsDailyStatLogger::process_tag_request_group()";

      try
      {
        CollectorT::DataT data;

        data.add(
          CollectorT::DataT::KeyT(
            tag_request_group_info.site_id,
            tag_request_group_info.country,
            tag_request_group_info.tags.begin(),
            tag_request_group_info.tags.end()),
          tag_request_group_info.rollback ?
            (tag_request_group_info.ad_shown ?
              ONE_UTILIZED_PAGE_LOAD_ROLLBACK_ : ONE_PAGE_LOAD_ROLLBACK_) :
            (tag_request_group_info.ad_shown ?
              ONE_UTILIZED_PAGE_LOAD_ : ONE_PAGE_LOAD_));

        add_record(
          CollectorT::KeyT(tag_request_group_info.time, tag_request_group_info.colo_id),
          data);
      }
      catch (const eh::Exception &ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw TagRequestGroupProcessor::Exception(ostr);
      }
    }

  private:
    const CollectorT::DataT::DataT ONE_PAGE_LOAD_;
    const CollectorT::DataT::DataT ONE_PAGE_LOAD_ROLLBACK_;
    const CollectorT::DataT::DataT ONE_UTILIZED_PAGE_LOAD_;
    const CollectorT::DataT::DataT ONE_UTILIZED_PAGE_LOAD_ROLLBACK_;
  };

  /**
   * ResearchActionLogger
   */
  class ResearchActionLogger:
    public virtual AdvActionProcessor,
    public virtual AdServer::LogProcessing::LogHolderPoolData<
      AdServer::LogProcessing::ResearchActionTraits,
      AdServer::LogProcessing::SimpleCsvSavePolicy<
        AdServer::LogProcessing::ResearchActionTraits> >,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    ResearchActionLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : AdServer::LogProcessing::LogHolderPoolData<
          AdServer::LogProcessing::ResearchActionTraits,
          AdServer::LogProcessing::SimpleCsvSavePolicy<
            AdServer::LogProcessing::ResearchActionTraits>
          >(flush_traits)
    {}

    virtual void
    process_adv_action(
      const AdvActionInfo& /*adv_action_info*/)
      /*throw(AdvActionProcessor::Exception)*/
    {}

    virtual void
    process_custom_action(
      const AdvExActionInfo& adv_custom_action_info)
      /*throw(AdvActionProcessor::Exception)*/
    {
      static const char* FUN = "ResearchActionLogger::process_custom_action()";

      try
      {
        const AdvExActionInfo& info = adv_custom_action_info;

        add_record(
          CollectorT::DataT(
            info.time,
            info.user_id,
            LogProcessing::RequestId(), // FIXME: request_id
            info.action_id,
            info.device_channel_id,
            info.action_request_id,
            info.ccg_ids,
            info.referer,
            info.order_id,
            info.ip_address,
            info.action_value
          )
        );
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw AdvActionProcessor::Exception(ostr);
      }
    }
  };

  class NullResearchActionLogger:
    public virtual AdvActionProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    NullResearchActionLogger()
      /*throw(eh::Exception)*/
    {}

    virtual void
    process_adv_action(
      const AdvActionInfo& /*adv_action_info*/)
      /*throw(AdvActionProcessor::Exception)*/
    { // DO NOTHING
    }

    virtual void
    process_custom_action(
      const AdvExActionInfo& /*adv_custom_action_info*/)
      /*throw(AdvActionProcessor::Exception)*/
    { // DO NOTHING
    }
  };

  /**
   * ResearchBidLogger
   */
  class ResearchBidLogger:
    public virtual RequestContainerProcessor,
    public virtual AdServer::LogProcessing::LogHolderPoolData<
      AdServer::LogProcessing::ResearchBidTraits,
      AdServer::LogProcessing::SimpleCsvSavePolicy<
        AdServer::LogProcessing::ResearchBidTraits> >,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    ResearchBidLogger(
      const LogProcessing::LogFlushTraits& flush_traits,
      Commons::LogReferrer::Setting site_referrer_stats_log_referrer_setting)
      /*throw(eh::Exception)*/
      : AdServer::LogProcessing::LogHolderPoolData<
          AdServer::LogProcessing::ResearchBidTraits,
          AdServer::LogProcessing::SimpleCsvSavePolicy<
            AdServer::LogProcessing::ResearchBidTraits>
          >(flush_traits),
        log_referrer_setting_(site_referrer_stats_log_referrer_setting)
    {}

    virtual void
    process_request(const RequestInfo& request_info)
      /*throw(RequestContainerProcessor::Exception)*/
    {
      static const char* FUN = "ResearchBidLogger::process_request()";

      if(!request_info.test_request)
      {
        try
        {
          const RequestInfo& info = request_info;

          CollectorT::DataT data;

          data.time = info.time;
          data.request_id = info.request_id;
          //data.global_request_id = info.global_request_id;
          data.user_id = info.user_id;
          data.household_id = info.household_id;
          data.tag_id = info.tag_id;
          data.ext_tag_id = info.ext_tag_id;
          data.ip_address = info.ip_address;
          data.cc_id = info.cc_id;
          data.channel_list = info.channels;
          data.history_channel_list.assign(
            info.user_channels.begin(), info.user_channels.end());
          data.geo_channels.assign(
            info.geo_channels.begin(), info.geo_channels.end());
          data.device_channel_id = info.device_channel_id;
          data.referer = Commons::LogReferrer::normalize_referrer(
            info.referer, log_referrer_setting_, "");
          data.algorithm_id = info.ctr_algorithm_id;
          data.size_id = info.size_id;
          data.colo_id = info.colo_id;
          data.predicted_ctr = info.ctr;
          data.campaign_freq = info.campaign_freq;
          data.conv_rate_algorithm_id = info.conv_rate_algorithm_id;
          data.predicted_conv_rate = info.conv_rate;
          if(info.pub_revenue.currency_rate != RevenueDecimal::ZERO)
          {
            data.bid_cost = RevenueDecimal::div(
              RevenueDecimal::mul(info.pub_bid_cost,
                CPM_MULTILPIER, Generics::DMR_FLOOR),
              info.pub_revenue.currency_rate, Generics::DDR_FLOOR);
            data.floor_cost = RevenueDecimal::div(
              RevenueDecimal::mul(info.pub_floor_cost,
                CPM_MULTILPIER, Generics::DMR_FLOOR),
              info.pub_revenue.currency_rate, Generics::DDR_FLOOR);
          }
          else
          {
            data.bid_cost = RevenueDecimal::ZERO;
            data.floor_cost = RevenueDecimal::ZERO;
          }

          add_record(data);
        }
        catch (const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << ex.what();
          throw RequestContainerProcessor::Exception(ostr);
        }
      }
    }

    virtual void
    process_impression(const ImpressionInfo& /*impression_info*/)
      /*throw(RequestContainerProcessor::Exception)*/
    {}

    virtual void
    process_action(
      ActionType /*action_type*/,
      const Generics::Time& /*time*/,
      const AdServer::Commons::RequestId& /*request_id*/)
      /*throw(RequestContainerProcessor::Exception)*/
    {}

    virtual void
    process_custom_action(
      const AdServer::Commons::RequestId& /*request_id*/,
      const AdvCustomActionInfo& /*adv_custom_action_info*/)
      /*throw(RequestContainerProcessor::Exception)*/
    {}

    virtual void
    process_impression_post_action(
      const AdServer::Commons::RequestId&,
      const RequestPostActionInfo&)
      /*throw(RequestContainerProcessor::Exception)*/
    {};

  private:
    const Commons::LogReferrer::Setting log_referrer_setting_;
  };

  class NullResearchBidLogger:
    public virtual RequestContainerProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    NullResearchBidLogger()
      /*throw(eh::Exception)*/
    {}

    virtual void
    process_request(const RequestInfo& /*request_info*/)
      /*throw(RequestContainerProcessor::Exception)*/
    {}

    virtual void
    process_impression(const ImpressionInfo& /*impression_info*/)
      /*throw(RequestContainerProcessor::Exception)*/
    {}

    virtual void
    process_action(
      ActionType /*action_type*/,
      const Generics::Time& /*time*/,
      const AdServer::Commons::RequestId& /*request_id*/)
      /*throw(RequestContainerProcessor::Exception)*/
    {}

    virtual void
    process_custom_action(
      const AdServer::Commons::RequestId& /*request_id*/,
      const AdvCustomActionInfo& /*adv_custom_action_info*/)
      /*throw(RequestContainerProcessor::Exception)*/
    {}

    virtual void
    process_impression_post_action(
      const AdServer::Commons::RequestId&,
      const RequestPostActionInfo&)
      /*throw(RequestContainerProcessor::Exception)*/
    {};
  };

  /**
   * ResearchImpressionLogger
   */
  class ResearchImpressionLogger:
    public virtual RequestActionProcessor,
    public virtual AdServer::LogProcessing::LogHolderPoolData<
      AdServer::LogProcessing::ResearchImpressionTraits,
      AdServer::LogProcessing::SimpleCsvSavePolicy<
        AdServer::LogProcessing::ResearchImpressionTraits> >,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    ResearchImpressionLogger(
      const LogProcessing::LogFlushTraits& flush_traits,
      Commons::LogReferrer::Setting site_referrer_stats_log_referrer_setting)
      /*throw(eh::Exception)*/
      : AdServer::LogProcessing::LogHolderPoolData<
          AdServer::LogProcessing::ResearchImpressionTraits,
          AdServer::LogProcessing::SimpleCsvSavePolicy<
            AdServer::LogProcessing::ResearchImpressionTraits>
          >(flush_traits),
        log_referrer_setting_(site_referrer_stats_log_referrer_setting)
    {}

    virtual void
    process_impression(
      const RequestInfo& request_info,
      const ImpressionInfo&,
      const ProcessingState& processing_state)
      /*throw(RequestActionProcessor::Exception)*/
    {
      static const char* FUN =
        "ResearchImpressionLogger::process_impression()";

      if(!request_info.test_request &&
        processing_state.state == RequestInfo::RS_NORMAL)
      {
        try
        {
          const RequestInfo& info = request_info;

          CollectorT::DataT data;

          data.time = info.time;
          data.request_id = info.request_id;
          //data.global_request_id = info.global_request_id;
          data.user_id = info.user_id;
          data.household_id = info.household_id;
          data.publisher_account_id = info.publisher_account_id;
          data.tag_id = info.tag_id;
          data.ext_tag_id = info.ext_tag_id;
          data.ip_address = info.ip_address;
          data.campaign_id = info.campaign_id;
          data.ccg_id = info.ccg_id;
          data.cc_id = info.cc_id;
          data.channel_list = info.channels;
          data.history_channel_list.assign(
            info.user_channels.begin(), info.user_channels.end());
          data.geo_channels.assign(
            info.geo_channels.begin(), info.geo_channels.end());
          data.device_channel_id = info.device_channel_id;
          data.referer = Commons::LogReferrer::normalize_referrer(
            info.referer, log_referrer_setting_, "");
          data.algorithm_id = info.ctr_algorithm_id;
          data.size_id = info.size_id;
          data.colo_id = info.colo_id;
          data.predicted_ctr = info.ctr;
          data.campaign_freq = info.campaign_freq;
          data.conv_rate_algorithm_id = info.conv_rate_algorithm_id;
          data.predicted_conv_rate = info.conv_rate;
          data.tag_predicted_viewability = info.viewability;

          if(info.pub_revenue.currency_rate != RevenueDecimal::ZERO)
          {
            data.bid_cost = RevenueDecimal::div(
              RevenueDecimal::mul(info.pub_bid_cost,
                CPM_MULTILPIER, Generics::DMR_FLOOR),
              info.pub_revenue.currency_rate, Generics::DDR_FLOOR);
            data.floor_cost = RevenueDecimal::div(
              RevenueDecimal::mul(info.pub_floor_cost,
                CPM_MULTILPIER, Generics::DMR_FLOOR),
              info.pub_revenue.currency_rate, Generics::DDR_FLOOR);
          }
          else
          {
            data.bid_cost = RevenueDecimal::ZERO;
            data.floor_cost = RevenueDecimal::ZERO;
          }

          data.win_price = info.pub_revenue.sys_impression(CPM_MULTILPIER);

          add_record(data);
        }
        catch (const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": eh::Exception caught: " << ex.what();
          throw RequestContainerProcessor::Exception(ostr);
        }
      }
    }

    virtual void
    process_request(
      const RequestInfo&,
      const ProcessingState&)
      /*throw(RequestActionProcessor::Exception)*/
    {}

    virtual void
    process_click(
      const RequestInfo&,
      const ProcessingState&)
      /*throw(RequestActionProcessor::Exception)*/
    {}

    virtual void
    process_action(const RequestInfo&)
      /*throw(RequestActionProcessor::Exception)*/
    {}

  private:
    const Commons::LogReferrer::Setting log_referrer_setting_;
  };

  /**
   * ResearchClickLogger
   */
  class ResearchClickLogger:
    public virtual UnmergedClickProcessor,
    public virtual AdServer::LogProcessing::LogHolderPoolData<
      AdServer::LogProcessing::ResearchClickTraits,
      AdServer::LogProcessing::SimpleCsvSavePolicy<
        AdServer::LogProcessing::ResearchClickTraits> >,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    ResearchClickLogger(const LogProcessing::LogFlushTraits& flush_traits)
      /*throw(eh::Exception)*/
      : AdServer::LogProcessing::LogHolderPoolData<
          AdServer::LogProcessing::ResearchClickTraits,
          AdServer::LogProcessing::SimpleCsvSavePolicy<
            AdServer::LogProcessing::ResearchClickTraits>
          >(flush_traits)
    {}

    virtual void
    process_click(const ClickInfo& click_info)
      /*throw(UnmergedClickProcessor::Exception)*/
    {
      static const char* FUN = "ResearchClickLogger::process_click()";

      try
      {
        const ClickInfo& info = click_info;

        CollectorT::DataT data;

        data.time = info.time;
        data.request_id = info.request_id;
        data.referrer = info.referer;

        add_record(data);
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw UnmergedClickProcessor::Exception(ostr);
      }
    }
  };

  /**
   * RequestOutLogger
   */
  RequestOutLogger::RequestOutLogger(
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
    const LogProcessing::LogFlushTraits& adv_user_stat_flush,
    const LogProcessing::LogFlushTraits& site_referer_stat_flush,
    const LogProcessing::LogFlushTraits& page_loads_daily_stat_flush,
    const LogProcessing::LogFlushTraits& tag_position_stat_flush,
    const LogProcessing::LogFlushTraits& campaign_referrer_stat_stat_flush,
    const LogProcessing::LogFlushTraits* research_action_flush,
    const LogProcessing::LogFlushTraits* research_bid_flush,
    const LogProcessing::LogFlushTraits* research_impression_flush,
    const LogProcessing::LogFlushTraits* research_click_flush,
    const LogProcessing::LogFlushTraits* bid_cost_stat_flush,
    Commons::LogReferrer::Setting site_referrer_stats_log_referrer_setting,
    unsigned long colo_id)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      dump_task_runner_(new Generics::TaskRunner(callback, 16))
  {
    // request loggers
    add_request_logger_(RequestLoggerBase_var(
      new UserPropertiesLogger(user_properties_flush)).in());

    add_request_logger_(RequestLoggerBase_var(
      new CreativeStatLogger(creative_stat_flush)).in());

    add_request_logger_(RequestLoggerBase_var(
      new ChannelPerformanceLogger(channel_performance_flush)).in());

    add_request_logger_(RequestLoggerBase_var(
      new ExpressionPerformanceLogger(expression_performance_flush)).in());

    add_request_logger_(RequestLoggerBase_var(
      new CCGKeywordStatLogger(ccg_keyword_stat_flush)).in());

    add_request_logger_(RequestLoggerBase_var(
      new CmpStatLogger(cmp_stat_flush)).in());

    add_request_logger_(RequestLoggerBase_var(
      new ActionStatLogger(action_stat_flush)).in());

    add_request_logger_(RequestLoggerBase_var(
      new ChannelImpInventoryLogger(channel_imp_inventory_flush)).in());

    add_request_logger_(RequestLoggerBase_var(
      new TagPositionStatLogger(tag_position_stat_flush)).in());

    add_request_logger_(RequestLoggerBase_var(
      new CampaignReferrerStatLogger(campaign_referrer_stat_stat_flush)).in());

    if(bid_cost_stat_flush)
    {
      add_request_logger_(RequestLoggerBase_var(
        new BidCostStatLogger(*bid_cost_stat_flush)).in());
    }

    // reach loggers
    add_reach_logger_(CampaignReachLoggerBase_var(
      new CCGUserStatLogger(ccg_user_stat_flush, colo_id)).in());

    add_reach_logger_(CampaignReachLoggerBase_var(
      new CCUserStatLogger(cc_user_stat_flush, colo_id)).in());

    add_reach_logger_(CampaignReachLoggerBase_var(
      new CampaignUserStatLogger(campaign_user_stat_flush, colo_id)).in());

    add_reach_logger_(CampaignReachLoggerBase_var(
      new AdvertiserUserStatLogger(adv_user_stat_flush, colo_id)).in());

    ReferenceCounting::SmartPtr<SiteRefererStatLogger>
      site_referer_logger = new SiteRefererStatLogger(
        site_referer_stat_flush,
        site_referrer_stats_log_referrer_setting,
        dump_task_runner_);

    // passback loggers
    add_passback_logger_(PassbackStatLogger_var(
      new PassbackStatLogger(passback_stat_flush)).in());

    add_passback_logger_(site_referer_logger.in());

    // site reach loggers
    add_site_reach_logger_(SiteUserStatLogger_var(
      new SiteUserStatLogger(site_stat_flush, colo_id)).in());

    site_referer_logger_ = site_referer_logger;

    add_request_logger_(site_referer_logger.in());

    ReferenceCounting::SmartPtr<PageLoadsDailyStatLogger>
      page_loads_daily_logger = new PageLoadsDailyStatLogger(
        page_loads_daily_stat_flush);

    page_loads_daily_logger_ = page_loads_daily_logger;

    add_child_log_holder(page_loads_daily_logger);

    if (research_action_flush)
    {
      ReferenceCounting::SmartPtr<ResearchActionLogger>
        research_action_logger =
          new ResearchActionLogger(*research_action_flush);

      research_action_logger_ = research_action_logger;

      add_child_log_holder(research_action_logger);
    }
    else
    {
      research_action_logger_ = new NullResearchActionLogger;
    }

    if (research_bid_flush)
    {
      ReferenceCounting::SmartPtr<ResearchBidLogger> research_bid_logger =
        new ResearchBidLogger(*research_bid_flush, site_referrer_stats_log_referrer_setting);

      research_bid_logger_ = research_bid_logger;

      add_child_log_holder(research_bid_logger);
    }
    else
    {
      research_bid_logger_ = new NullResearchBidLogger;
    }

    // ResearchImpression logger
    if (research_impression_flush)
    {
      ReferenceCounting::SmartPtr<ResearchImpressionLogger>
        research_impression_logger = new ResearchImpressionLogger(
          *research_impression_flush,
          site_referrer_stats_log_referrer_setting);

      request_loggers_.push_back(research_impression_logger);

      add_child_log_holder(research_impression_logger);
    }

    if (research_click_flush)
    {
      ReferenceCounting::SmartPtr<ResearchClickLogger> research_click_logger =
        new ResearchClickLogger(*research_click_flush);

      research_click_logger_ = research_click_logger;

      add_child_log_holder(research_click_logger);
    }
    else
    {
      research_click_logger_ = new NullUnmergedClickProcessor;
    }

    dump_task_runner_->activate_object();
  }

  RequestOutLogger::~RequestOutLogger() noexcept
  {
    request_loggers_.clear();
    reach_loggers_.clear();
    passback_loggers_.clear();
    site_reach_loggers_.clear();
    site_referer_logger_ = TagRequestProcessor_var();
    page_loads_daily_logger_ = TagRequestGroupProcessor_var();
    research_action_logger_ = AdvActionProcessor_var();
    research_bid_logger_ = RequestContainerProcessor_var();
    research_click_logger_ = UnmergedClickProcessor_var();

    // deactivate only here because logholder's can dump on destroy
    dump_task_runner_->deactivate_object();
    dump_task_runner_->wait_object();
  }

  Generics::Time
  RequestOutLogger::flush_if_required(const Generics::Time& now)
  {
    std::cout << "[" << Generics::Time::get_time_of_day().gm_ft() << "] RequestOutLogger::flush_if_required()" <<
      std::endl;
    return LogProcessing::CompositeLogHolder::flush_if_required(now);
  }

  void
  RequestOutLogger::process_request(
    const RequestInfo& request_info,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = request_loggers_.begin();
        it != request_loggers_.end(); ++it)
    {
      (*it)->process_request(request_info, processing_state);
    }
  }

  void
  RequestOutLogger::process_impression(
    const RequestInfo& request_info,
    const ImpressionInfo& impression_info,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = request_loggers_.begin();
        it != request_loggers_.end(); ++it)
    {
      (*it)->process_impression(request_info, impression_info, processing_state);
    }
  }

  void
  RequestOutLogger::process_click(
    const RequestInfo& request_info,
    const ProcessingState& processing_state)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = request_loggers_.begin();
        it != request_loggers_.end(); ++it)
    {
      (*it)->process_click(request_info, processing_state);
    }
  }

  void RequestOutLogger::process_action(
    const RequestInfo& request_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = request_loggers_.begin();
        it != request_loggers_.end(); ++it)
    {
      (*it)->process_action(request_info);
    }
  }

  void RequestOutLogger::process_custom_action(
    const RequestInfo& request_info,
    const AdvCustomActionInfo& adv_custom_action_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = request_loggers_.begin();
        it != request_loggers_.end(); ++it)
    {
      (*it)->process_custom_action(request_info, adv_custom_action_info);
    }
  }

  void
  RequestOutLogger::process_request_post_action(
    const RequestInfo& request_info,
    const RequestPostActionInfo& request_post_action_info)
    /*throw(RequestActionProcessor::Exception)*/
  {
    for(RequestActionProcessorList::iterator it = request_loggers_.begin();
        it != request_loggers_.end(); ++it)
    {
      (*it)->process_request_post_action(
        request_info,
        request_post_action_info);
    }
  }

  void RequestOutLogger::process_reach(const ReachInfo& ri)
    /*throw(CampaignReachProcessor::Exception)*/
  {
    for(CampaignReachProcessorList::iterator it = reach_loggers_.begin();
        it != reach_loggers_.end(); ++it)
    {
      (*it)->process_reach(ri);
    }
  }

  void RequestOutLogger::process_site_reach(
    const SiteReachProcessor::SiteReachInfo& site_reach_info)
    /*throw(SiteReachProcessor::Exception)*/
  {
    for(SiteReachProcessorList::iterator it =
          site_reach_loggers_.begin();
        it != site_reach_loggers_.end(); ++it)
    {
      (*it)->process_site_reach(site_reach_info);
    }
  }

  void RequestOutLogger::process_passback(const PassbackInfo& pi)
    /*throw(PassbackProcessor::Exception)*/
  {
    for(PassbackProcessorList::iterator it =
          passback_loggers_.begin();
        it != passback_loggers_.end(); ++it)
    {
      (*it)->process_passback(pi);
    }
  }

  void RequestOutLogger::process_tag_request(
    const TagRequestInfo& tag_request_info)
    /*throw(TagRequestProcessor::Exception)*/
  {
    if (tag_request_info.tag_id)
    {
      site_referer_logger_->process_tag_request(tag_request_info);
    }
  }

  void
  RequestOutLogger::process_tag_request_group(
    const TagRequestGroupInfo& tag_request_group_info)
    /*throw(TagRequestGroupProcessor::Exception)*/
  {
    page_loads_daily_logger_->process_tag_request_group(
      tag_request_group_info);
  }

  void
  RequestOutLogger::process_adv_action(
    const AdvActionInfo& adv_action_info)
    /*throw(AdvActionProcessor::Exception)*/
  {
    research_action_logger_->process_adv_action(adv_action_info);
  }

  void
  RequestOutLogger::process_custom_action(
    const AdvExActionInfo& adv_custom_action_info)
    /*throw(AdvActionProcessor::Exception)*/
  {
    research_action_logger_->process_custom_action(adv_custom_action_info);
  }

  void
  RequestOutLogger::process_request(
    const RequestInfo& request_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    research_bid_logger_->process_request(request_info);
  }

  void
  RequestOutLogger::process_impression(
    const ImpressionInfo& impression_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    research_bid_logger_->process_impression(impression_info);
  }

  void
  RequestOutLogger::process_action(
    ActionType action_type,
    const Generics::Time& time,
    const AdServer::Commons::RequestId& request_id)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    research_bid_logger_->process_action(action_type, time, request_id);
  }

  void
  RequestOutLogger::process_custom_action(
    const AdServer::Commons::RequestId& request_id,
    const AdvCustomActionInfo& adv_custom_action_info)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    research_bid_logger_->process_custom_action(request_id,
      adv_custom_action_info);
  }

  void
  RequestOutLogger::process_click(const ClickInfo& click_info)
    /*throw(UnmergedClickProcessor::Exception)*/
  {
    research_click_logger_->process_click(click_info);
  }

  template<typename LoggerType>
  void
  RequestOutLogger::add_request_logger_(
    LoggerType* request_logger)
    /*throw(eh::Exception)*/
  {
    request_loggers_.push_back(
      ReferenceCounting::add_ref(request_logger));

    add_child_log_holder(request_logger);
  }

  template<typename LoggerType>
  void
  RequestOutLogger::add_reach_logger_(
    LoggerType* reach_logger)
    /*throw(eh::Exception)*/
  {
    reach_loggers_.push_back(
      ReferenceCounting::add_ref(reach_logger));

    add_child_log_holder(reach_logger);
  }

  template<typename LoggerType>
  void
  RequestOutLogger::add_passback_logger_(
    LoggerType* passback_logger)
    /*throw(eh::Exception)*/
  {
    passback_loggers_.push_back(
      ReferenceCounting::add_ref(passback_logger));

    add_child_log_holder(passback_logger);
  }

  template<typename LoggerType>
  void
  RequestOutLogger::add_site_reach_logger_(
    LoggerType* site_reach_logger)
    /*throw(eh::Exception)*/
  {
    site_reach_loggers_.push_back(
      ReferenceCounting::add_ref(site_reach_logger));

    add_child_log_holder(site_reach_logger);
  }
}
}
