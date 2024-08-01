#ifndef _REQUEST_ACTION_PROCESSOR_HPP_
#define _REQUEST_ACTION_PROCESSOR_HPP_

#include <list>
#include <vector>
#include <optional>

#include <Generics/Time.hpp>
#include <Commons/Algs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

#include <Commons/Containers.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

/**
 * RequestInfo
 * RequestExInfo
 * ImpressionInfo
 * AdvCustomActionInfo
 *
 * RequestActionProcessor
 * RequestContainerProcessor
 * AdvActionProcessor
 */
namespace AdServer
{
  namespace RequestInfoSvcs
  {
    static const AdServer::Commons::UserId OPTOUT_USER_ID("OOOOOOOOOOOOOOOOOOOOOA..");

    typedef AdServer::CampaignSvcs::RevenueDecimal RevenueDecimal;
    typedef AdServer::CampaignSvcs::DeliveryThresholdDecimal DeliveryThresholdDecimal;

    // RequestInfo
    struct RequestInfo
    {
      typedef std::list<unsigned long> ChannelIdList;

      /*
       * Revenue in custom currency
       */
      struct Revenue
      {
        Revenue() noexcept
          : rate_id(0),
            impression(RevenueDecimal::ZERO),
            click(RevenueDecimal::ZERO),
            action(RevenueDecimal::ZERO)
        {}

        unsigned long rate_id;

        RevenueDecimal impression;
        RevenueDecimal click;
        RevenueDecimal action;

        Revenue&
        operator=(const Revenue& init)
        {
          rate_id = init.rate_id;
          impression = init.impression;
          click = init.click;
          action = init.action;
          return *this;
        }

        Revenue&
        operator*=(const RevenueDecimal& mul) /*throw(RevenueDecimal::Overflow)*/;

        friend Revenue operator*(Revenue lhs, const RevenueDecimal& mul)
          /*throw(RevenueDecimal::Overflow)*/
        {
          lhs *= mul;
          return lhs;
        }

        Revenue&
        operator/=(const RevenueDecimal& op) /*throw(RevenueDecimal::Overflow)*/;

        friend Revenue operator/(Revenue lhs, const RevenueDecimal& divider)
          /*throw(RevenueDecimal::Overflow)*/
        {
          lhs /= divider;
          return lhs;
        }

        Revenue&
        operator+=(const Revenue& op) /*throw(RevenueDecimal::Overflow)*/;

        friend Revenue operator+(Revenue lhs, const Revenue& add)
          /*throw(RevenueDecimal::Overflow)*/
        {
          lhs += add;
          return lhs;
        }

        Revenue&
        operator-=(const Revenue& sub) /*throw(RevenueDecimal::Overflow)*/;

        friend Revenue operator-(Revenue lhs, const Revenue& sub)
          /*throw(RevenueDecimal::Overflow)*/
        {
          lhs -= sub;
          return lhs;
        }

        template<typename OStream>
        OStream&
        print(OStream& out, const char* space) const noexcept;

        static RevenueDecimal
        convert_currency(
          const RevenueDecimal& value,
          const RevenueDecimal& value_currency_rate,
          const RevenueDecimal& result_currency_rate)
        {
          if(value_currency_rate == result_currency_rate)
          {
            return value;
          }

          return RevenueDecimal::mul(
            RevenueDecimal::div(value, value_currency_rate),
            result_currency_rate,
            Generics::DMR_CEIL);
        }

        Revenue
        convert_currency(
          const RevenueDecimal& value_currency_rate,
          const RevenueDecimal& result_currency_rate)
        {
          if(value_currency_rate == result_currency_rate)
          {
            return *this;
          }

          Revenue res;
          res.impression = RevenueDecimal::mul(
            RevenueDecimal::div(impression, value_currency_rate),
            result_currency_rate,
            Generics::DMR_CEIL);
          res.click = RevenueDecimal::mul(
            RevenueDecimal::div(click, value_currency_rate),
            result_currency_rate,
            Generics::DMR_CEIL);
          res.action = RevenueDecimal::mul(
            RevenueDecimal::div(action, value_currency_rate),
            result_currency_rate,
            Generics::DMR_CEIL);

          return res;
        }
      };

      /**
       * Revenue in custom currency,
       * convertible to System currency
       */
      struct RevenueSys: public Revenue
      {
        DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

        RevenueSys() noexcept
          : currency_rate(RevenueDecimal::ZERO)
        {}

        RevenueDecimal currency_rate;

        RevenueSys&
        operator=(const Revenue& init)
        {
          rate_id = init.rate_id;
          impression = init.impression;
          click = init.click;
          action = init.action;
          return *this;
        }

        void check_sys() const /*throw(Exception)*/
        {
          if(currency_rate.is_zero())
          {
            if(impression != RevenueDecimal::ZERO ||
              click != RevenueDecimal::ZERO ||
              action != RevenueDecimal::ZERO)
            {
              throw Exception("currency rate is zero when revenue is not");
            }
          }
        }

        RevenueDecimal
        sys_impression() const /*throw(Exception, RevenueDecimal::Overflow)*/
        {
          check_sys();
          return RevenueDecimal::div(impression, currency_rate);
        }

        RevenueDecimal
        sys_impression(const RevenueDecimal& mult_val)
          const /*throw(Exception, RevenueDecimal::Overflow)*/
        {
          check_sys();
          return RevenueDecimal::div(
            RevenueDecimal::mul(impression, mult_val, Generics::DMR_FLOOR),
            currency_rate);
        }

        RevenueDecimal
        sys_click() const /*throw(Exception, RevenueDecimal::Overflow)*/
        {
          check_sys();
          return RevenueDecimal::div(click, currency_rate);
        }

        RevenueDecimal
        sys_action() const /*throw(Exception, RevenueDecimal::Overflow)*/
        {
          check_sys();
          return RevenueDecimal::div(action, currency_rate);
        }
        
        RevenueDecimal
        convert_impression(const RevenueDecimal& currency_rate)
          const /*throw(Exception, RevenueDecimal::Overflow)*/
        {
          return RevenueDecimal::mul(
            sys_impression(),
            currency_rate,
            Generics::DMR_CEIL);
        }

        RevenueDecimal
        convert_click(const RevenueDecimal& currency_rate)
          const /*throw(Exception, RevenueDecimal::Overflow)*/
        {
          return RevenueDecimal::mul(
            sys_click(),
            currency_rate,
            Generics::DMR_CEIL);
        }

        RevenueDecimal
        convert_action(const RevenueDecimal& currency_rate)
          const /*throw(Exception, RevenueDecimal::Overflow)*/
        {
          return RevenueDecimal::mul(
            sys_action(),
            currency_rate,
            Generics::DMR_CEIL);
        }

        template<typename OStream>
        OStream&
        print(OStream& out, const char* space) const noexcept;
      };

      struct ChannelRevenue
      {
        ChannelRevenue() noexcept;

        unsigned long channel_id;
        unsigned long channel_rate_id;
        RevenueDecimal impression;
        RevenueDecimal sys_impression;
        RevenueDecimal adv_impression;
        RevenueDecimal click;
        RevenueDecimal sys_click;
        RevenueDecimal adv_click;
      };

      typedef std::list<ChannelRevenue> ChannelRevenueList;

      enum RequestState
      {
        // normal saving of positive counters
        RS_NORMAL = 0,
        RS_FRAUD,
        // rollback, now actual only for request (negative counters)
        RS_MOVED,
        // rollback when data changed (negative counters)
        RS_ROLLBACK,
        // repeat saving of action with changed data (positive counters)
        // some loggers that don't process rollbacks and ignore this state for avoid
        // double counters saving
        RS_RESAVE,
        // double action, don't save stats now
        RS_DUPLICATE
      };

      RequestInfo(const AdServer::Commons::RequestId& req_id =
        AdServer::Commons::RequestId()) noexcept;

      AdServer::Commons::RequestId request_id;
      AdServer::Commons::UserId user_id;
      AdServer::Commons::UserId household_id;
      char user_status;
      RequestState fraud;

      Generics::Time time;
      Generics::Time isp_time;
      Generics::Time pub_time;
      Generics::Time adv_time;
      bool test_request;
      bool walled_garden;
      bool hid_profile;
      bool disable_fraud_detection;

      unsigned long colo_id;
      unsigned long publisher_account_id;
      unsigned long site_id;
      unsigned long tag_id;
      unsigned long size_id;
      std::string ext_tag_id;
      unsigned long adv_account_id;
      unsigned long advertiser_id;
      unsigned long campaign_id;
      unsigned long ccg_id;
      unsigned long ctr_reset_id;
      unsigned long cc_id;
      bool has_custom_actions;
      DeliveryThresholdDecimal tag_delivery_threshold;

      unsigned long currency_exchange_id;

      RevenueSys adv_revenue;
      Revenue adv_comm_revenue;
      Revenue adv_payable_comm_amount;

      RevenueSys pub_revenue;
      Revenue pub_comm_revenue;
      RevenueDecimal pub_floor_cost;
      RevenueDecimal pub_bid_cost;
      RevenueDecimal pub_commission;

      RevenueSys isp_revenue;
      RevenueDecimal isp_revenue_share;

      unsigned long num_shown;
      unsigned long position;
      std::string tag_size;
      std::optional<unsigned long> tag_visibility;
      std::optional<unsigned long> tag_top_offset;
      std::optional<unsigned long> tag_left_offset;

      std::string client_app;
      std::string client_app_version;
      std::string browser_version;
      std::string os_version;
      std::string country;
      std::string referer;

      ChannelIdList channels;
      unsigned long geo_channel_id;
      unsigned long device_channel_id;
      std::string expression;
      ChannelRevenueList cmp_channels;

      bool text_campaign;
      unsigned long ccg_keyword_id;
      unsigned long keyword_id;

      Generics::Time kw_last_page_match;
      Generics::Time kw_last_search_match;
      
      bool disabled_pub_cost_check;

      // uses only by RequestActionProcessor
      bool enabled_notice;
      bool enabled_impression_tracking;
      bool enabled_action_tracking;

      Generics::Time imp_time;
      Generics::Time click_time;
      Generics::Time action_time;

      CampaignSvcs::AuctionType auction_type;
      int viewability; // contains final viewability that should be used for rollback/save action

      unsigned long campaign_freq;
      unsigned long referer_hash;
      std::vector<unsigned long> geo_channels;
      std::vector<unsigned long> user_channels;
      std::string url;
      std::string ip_address;

      std::string ctr_algorithm_id;
      RevenueDecimal ctr;
      std::vector<RevenueDecimal> model_ctrs;

      std::string conv_rate_algorithm_id;
      RevenueDecimal conv_rate;
      std::vector<RevenueDecimal> model_conv_rates;

      RevenueDecimal self_service_commission;
      RevenueDecimal adv_commission;
      RevenueDecimal pub_cost_coef;
      unsigned long at_flags;

      Revenue delta_adv_revenue;

      template<typename OStream>
      OStream&
      print(OStream& out, const char* space) const
        noexcept;

      static const char*
      request_state_string(RequestState request_state)
        noexcept;

      template<typename OStream>
      static OStream&
      print_request_state(OStream& out, RequestState request_state)
        noexcept;
    };

    /*
    // RequestExInfo
    struct RequestExInfo: public RequestInfo
    {
      explicit
      RequestExInfo(const AdServer::Commons::RequestId& req_id =
        AdServer::Commons::RequestId())
        noexcept
        : RequestInfo(req_id)
      {}

      explicit
      RequestExInfo(const RequestInfo& request_info)
        noexcept
        : RequestInfo(request_info)
      {}

      AdServer::Commons::RequestId global_request_id;
      std::string ip_address;
      ChannelIdList user_channels;
      ChannelIdList geo_channels;
      std::string algorithm_id;
      unsigned long campaign_freq;
      std::string conv_rate_algorithm_id;
      RevenueDecimal conv_rate;
    };
    */

    // ImpressionInfo
    struct ImpressionInfo
    {
      ImpressionInfo()
        : time(0),
          verify_impression(true),
          viewability(-1)
      {}

      struct PubRevenue
      {
        bool
        operator==(const PubRevenue& right) const;

        AdServer::CampaignSvcs::RevenueType revenue_type;
        RevenueDecimal impression;
      };

      AdServer::Commons::RequestId request_id;
      Generics::Time time;
      bool verify_impression;
      std::optional<PubRevenue> pub_revenue;
      AdServer::Commons::UserId user_id;
      int viewability; // contains viewabliity from log

      std::ostream&
      print(std::ostream& out, const char* space) const
        noexcept;
    };

    // AdvCustomActionInfo
    struct AdvCustomActionInfo
    {
      AdvCustomActionInfo() noexcept
        : action_id(0)
      {}

      Generics::Time time;
      unsigned long action_id;
      AdServer::Commons::UserId action_request_id;
      std::string referer;
      std::string order_id;
      RevenueDecimal action_value;

      template<typename OStream>
      OStream& print(
        OStream& ostr, const char* space = "") const noexcept;
    };

    typedef std::list<AdvCustomActionInfo> AdvCustomActionInfoList;

    struct RequestPostActionInfo
    {
      RequestPostActionInfo()
      {}

      RequestPostActionInfo(
        const Generics::Time& time_val,
        const String::SubString& action_name_val)
        noexcept
        : time(time_val),
          action_name(action_name_val.str())
      {}

      Generics::Time time;
      std::string action_name;
    };

    typedef std::list<RequestPostActionInfo> RequestPostActionInfoList;

    /**
     * RequestActionProcessor
     */
    struct RequestActionProcessor:
      public virtual ReferenceCounting::Interface
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      struct ProcessingState
      {
        ProcessingState(
          RequestInfo::RequestState state_val = RequestInfo::RS_NORMAL)
          : state(state_val)
        {}

        RequestInfo::RequestState state;
      };

      virtual void
      process_request(
        const RequestInfo&,
        const ProcessingState&)
        /*throw(Exception)*/ = 0;

      virtual void
      process_impression(
        const RequestInfo&,
        const ImpressionInfo&,
        const ProcessingState&)
        /*throw(Exception)*/ = 0;

      virtual void
      process_click(
        const RequestInfo&,
        const ProcessingState&)
        /*throw(Exception)*/ = 0;

      virtual void
      process_action(const RequestInfo&)
        /*throw(Exception)*/ = 0;

      virtual void
      process_custom_action(
        const RequestInfo&, const AdvCustomActionInfo&)
        /*throw(Exception)*/
      {};

      virtual void
      process_request_post_action(
        const RequestInfo&, const RequestPostActionInfo&)
        /*throw(Exception)*/
      {};

    protected:
      virtual ~RequestActionProcessor() noexcept {}
    };

    typedef
      ReferenceCounting::SmartPtr<RequestActionProcessor>
      RequestActionProcessor_var;

    /**
     * RequestContainerProcessor
     */
    class RequestContainerProcessor:
      public virtual ReferenceCounting::Interface
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      enum ActionType
      {
        AT_CLICK = 0,
        AT_ACTION,
        AT_FRAUD_ROLLBACK
      };

      virtual void
      process_request(const RequestInfo& request_info)
        /*throw(Exception)*/ = 0;

      virtual void
      process_impression(const ImpressionInfo& impression_info)
        /*throw(Exception)*/ = 0;

      virtual void
      process_action(
        ActionType action_type,
        const Generics::Time& time,
        const AdServer::Commons::RequestId& request_id)
        /*throw(Exception)*/ = 0;

      virtual void
      process_custom_action(
        const AdServer::Commons::RequestId& request_id,
        const AdvCustomActionInfo& adv_custom_action_info)
        /*throw(Exception)*/ = 0;

      virtual void
      process_impression_post_action(
        const AdServer::Commons::RequestId& request_id,
        const RequestPostActionInfo& request_post_action_info)
        /*throw(Exception)*/ = 0;

    protected:
      virtual ~RequestContainerProcessor() noexcept {}
    };

    typedef
      ReferenceCounting::SmartPtr<RequestContainerProcessor>
      RequestContainerProcessor_var;

    /**
     * AdvActionProcessor
     */
    struct AdvActionProcessor:
      public virtual ReferenceCounting::Interface
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      struct AdvActionInfo
      {
        Generics::Time time;
        unsigned long ccg_id;
        AdServer::Commons::UserId user_id;

        template<typename OStream>
        OStream& print(
          OStream& ostr, const char* space = "") const noexcept;
      };

      struct AdvExActionInfo
      {
        typedef std::list<unsigned long> CCGIdList;

        Generics::Time time;
        unsigned long action_id;
        unsigned long device_channel_id;
        CCGIdList ccg_ids;
        AdServer::Commons::UserId action_request_id;
        AdServer::Commons::UserId user_id;
        std::string referer;
        std::string order_id;
        std::string ip_address;
        RevenueDecimal action_value;

        template<typename OStream>
        OStream& print(
          OStream& ostr, const char* space = "") const noexcept;
      };

      virtual void process_adv_action(
        const AdvActionInfo& adv_action_info)
        /*throw(Exception)*/ = 0;

      virtual void process_custom_action(
        const AdvExActionInfo& adv_action_info)
        /*throw(Exception)*/ = 0;

    protected:
      virtual ~AdvActionProcessor() noexcept {}
    };

    typedef
      ReferenceCounting::SmartPtr<AdvActionProcessor>
      AdvActionProcessor_var;

    /**
     * UnmergedClickProcessor
     */
    struct UnmergedClickProcessor:
      public virtual ReferenceCounting::Interface
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      struct ClickInfo
      {
        Generics::Time time;
        AdServer::Commons::RequestId request_id;
        std::string referer;

        std::ostream& print(
          std::ostream& ostr, const char* space = "") const noexcept;
      };

      virtual void process_click(
        const ClickInfo& click_info)
        /*throw(Exception)*/ = 0;

    protected:
      virtual ~UnmergedClickProcessor() noexcept {}
    };

    typedef
      ReferenceCounting::SmartPtr<UnmergedClickProcessor>
      UnmergedClickProcessor_var;

    class NullUnmergedClickProcessor:
      public virtual UnmergedClickProcessor,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      NullUnmergedClickProcessor()
        /*throw(eh::Exception)*/
      {}

      virtual void
      process_click(const ClickInfo& /*click_info*/)
        /*throw(UnmergedClickProcessor::Exception)*/
      { // DO NOTHING
      }
    };
  }
}

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    /* RequestInfo */
    inline
    RequestInfo::ChannelRevenue::ChannelRevenue() noexcept
      : channel_id(0),
        channel_rate_id(0),
        impression(0),
        adv_impression(0),
        click(0),
        adv_click(0)
    {}

    inline
    RequestInfo::RequestInfo(
      const AdServer::Commons::RequestId& req_id) noexcept
      : request_id(req_id),
        fraud(RS_NORMAL),
        test_request(false),
        walled_garden(false),
        hid_profile(false),
        disable_fraud_detection(false),
        colo_id(0),
        publisher_account_id(0),
        site_id(0),
        tag_id(0),
        size_id(0),
        adv_account_id(0),
        advertiser_id(0),
        campaign_id(0),
        ccg_id(0),
        ctr_reset_id(0),
        cc_id(0),
        has_custom_actions(false),
        tag_delivery_threshold(DeliveryThresholdDecimal::ZERO),
        currency_exchange_id(0),
        pub_revenue(adv_revenue),
        pub_floor_cost(RevenueDecimal::ZERO),
        pub_bid_cost(RevenueDecimal::ZERO),
        pub_commission(RevenueDecimal::ZERO),
        isp_revenue(adv_revenue),
        isp_revenue_share(RevenueDecimal::ZERO),
        num_shown(1),
        position(1),
        geo_channel_id(0),
        device_channel_id(0),
        text_campaign(false),
        ccg_keyword_id(0),
        keyword_id(0),
        disabled_pub_cost_check(false),
        enabled_notice(false),
        enabled_impression_tracking(false),
        enabled_action_tracking(false),
        imp_time(0),
        click_time(0),
        action_time(0),
        auction_type(CampaignSvcs::AT_MAX_ECPM),
        viewability(-1),
        campaign_freq(0),
        referer_hash(0),
        ctr(RevenueDecimal::ZERO),
        conv_rate(RevenueDecimal::ZERO),
        self_service_commission(RevenueDecimal::ZERO),
        adv_commission(RevenueDecimal::ZERO),
        pub_cost_coef(RevenueDecimal::ZERO),
        at_flags(0)
    {}

    inline
    RequestInfo::Revenue&
    RequestInfo::Revenue::operator*=(const RevenueDecimal& mul)
      /*throw(RevenueDecimal::Overflow)*/
    {
      impression = RevenueDecimal::mul(impression, mul,
        Generics::DMR_FLOOR);
      click = RevenueDecimal::mul(click, mul, Generics::DMR_FLOOR);
      action = RevenueDecimal::mul(action, mul, Generics::DMR_FLOOR);
      return *this;
    }

    inline
    RequestInfo::Revenue&
    RequestInfo::Revenue::operator/=(const RevenueDecimal& op)
      /*throw(RevenueDecimal::Overflow)*/
    {
      impression = RevenueDecimal::div(impression, op, Generics::DDR_FLOOR);
      click = RevenueDecimal::div(click, op, Generics::DDR_FLOOR);
      action = RevenueDecimal::div(action, op, Generics::DDR_FLOOR);
      return *this;
    }

    inline
    RequestInfo::Revenue&
    RequestInfo::Revenue::operator+=(const Revenue& add)
      /*throw(RevenueDecimal::Overflow)*/
    {
      impression += add.impression;
      click += add.click;
      action += add.action;
      return *this;
    }

    inline
    RequestInfo::Revenue&
    RequestInfo::Revenue::operator-=(const Revenue& sub)
      /*throw(RevenueDecimal::Overflow)*/
    {
      impression -= sub.impression;
      click -= sub.click;
      action -= sub.action;
      return *this;
    }

    template<typename OStream>
    OStream&
    RequestInfo::Revenue::print(
      OStream& out, const char* space) const
      noexcept
    {
      out << space << "rate_id: " << rate_id << std::endl <<
        space << "impression: " << impression << std::endl <<
        space << "click: " << click << std::endl <<
        space << "action: " << action << std::endl;

      return out;
    }

    template<typename OStream>
    OStream&
    RequestInfo::RevenueSys::print(
      OStream& out, const char* space) const
      noexcept
    {
      this->RequestInfo::Revenue::print(out, space);
      out << space << "currency_rate: " << currency_rate << std::endl;
      return out;
    }

    inline const char*
    RequestInfo::request_state_string(RequestState request_state)
      noexcept
    {
      if(request_state == RS_NORMAL)
      {
        return "normal";
      }
      else if(request_state == RS_FRAUD)
      {
        return "fraud";
      }
      else if(request_state == RS_MOVED)
      {
        return "moved";
      }
      else if(request_state == RS_ROLLBACK)
      {
        return "rollback";
      }
      else if(request_state == RS_DUPLICATE)
      {
        return "duplicate";
      }
      else if(request_state == RS_RESAVE)
      {
        return "resave";
      }

      return "unknown";
    }

    template<typename OStream>
    OStream&
    RequestInfo::print_request_state(
      OStream& out, RequestState request_state)
      noexcept
    {
      out << request_state_string(request_state);
      return out;
    }

    template<typename OStream>
    OStream&
    RequestInfo::print(
      OStream& out, const char* space) const
      noexcept
    {
      std::string add_space(space);
      add_space += " ";

      out << space << "request_id: " << request_id << std::endl <<
        space << "fraud: ";
      print_request_state(out, fraud) << std::endl;
      out <<
        space << "user_id: " << user_id << std::endl <<
        space << "time: " << time.gm_ft() << std::endl <<
        space << "test_request: " << test_request << std::endl <<
        space << "colo_id: " << colo_id << std::endl <<
        space << "tag_id: " << tag_id << std::endl <<
        space << "size_id: " << size_id << std::endl <<
        space << "campaign_id: " << campaign_id << std::endl <<
        space << "ccg_id: " << ccg_id << std::endl <<
        space << "cc_id: " << cc_id << std::endl <<
        space << "currency_exchange_id: " << currency_exchange_id << std::endl;

      out << space << "adv_revenue: " << std::endl;
      adv_revenue.print(out, add_space.c_str()) << std::endl;
      out << space << "pub_revenue: " << std::endl;
      pub_revenue.print(out, add_space.c_str());
      out << space << "isp_revenue: " << std::endl;
      isp_revenue.print(out, add_space.c_str());

      out << space << "pub_commission: " << pub_commission << std::endl <<
        space << "isp_revenue_share: " << isp_revenue_share << std::endl;

      out << space << "num_shown: " << num_shown << std::endl <<
        space << "position: " << position << std::endl <<
        space << "client_app: " << client_app << std::endl <<
        space << "client_app_version: " << client_app_version << std::endl <<
        space << "browser_version: " << browser_version << std::endl <<
        space << "os_version: " << os_version << std::endl <<
        space << "country: " << country << std::endl <<
        space << "channels: ";

      Algs::print(out, channels.begin(), channels.end());

      out << std::endl <<
        space << "expression: " << expression << std::endl <<
        space << "text_campaign: " << text_campaign << std::endl <<
        space << "ccg_keyword_id: " << ccg_keyword_id << std::endl <<
        space << "keyword_id: " << keyword_id << std::endl <<
        space << "kw_last_page_match: " << kw_last_page_match.get_gm_time() <<
          std::endl <<
        space << "kw_last_search_match: " << kw_last_search_match.get_gm_time() <<
          std::endl <<
        space << "enabled_impression_tracking: " << enabled_impression_tracking << std::endl <<
        space << "imp_time: " << imp_time.get_gm_time() << std::endl <<
        space << "click_time: " << click_time.get_gm_time() << std::endl <<
        space << "auction_type: " << auction_type << std::endl <<
        space << "campaign_freq: " << campaign_freq << std::endl <<
        space << "referer_hash: " << referer_hash << std::endl <<
        space << "geo_channels: ";
      Algs::print(out, geo_channels.begin(), geo_channels.end());
      out << std::endl <<
        space << "user_channels: ";
      Algs::print(out, user_channels.begin(), user_channels.end());
      out << std::endl <<
        space << "url: " << url << std::endl <<
        space << "ip_address: " << ip_address << std::endl <<
        space << "ctr_algorithm_id: " << ctr_algorithm_id << std::endl <<
        space << "ctr: " << ctr << std::endl <<
        space << "model_ctrs: ";
      Algs::print(out, model_ctrs.begin(), model_ctrs.end());
      out << std::endl <<
        space << "conv_rate_algorithm_id: " << conv_rate_algorithm_id << std::endl <<
        space << "conv_rate: " << conv_rate << std::endl <<
        space << "model_conv_rates: ";
      Algs::print(out, model_conv_rates.begin(), model_conv_rates.end());

      return out;
    }

    inline bool
    ImpressionInfo::
    PubRevenue::operator==(const PubRevenue& right)
      const
    {
      return revenue_type == right.revenue_type &&
        impression == right.impression;
    }

    inline std::ostream&
    ImpressionInfo::print(
      std::ostream& out, const char* space) const
      noexcept
    {
      out << space << "request_id: " << request_id << std::endl <<
        space << "time: " << time.gm_ft() << std::endl <<
        space << "verify_impression: " << verify_impression << std::endl <<
        space << "user_id: " << user_id << std::endl <<
        space << "pub_revenue: ";
      if(pub_revenue)
      {
        out << "revenue_type=" << pub_revenue->revenue_type <<
          ", imp=" << pub_revenue->impression;
      }
      else
      {
        out << "none";
      }
      out << std::endl;

      return out;
    }

    template<typename OStream>
    OStream&
    AdvCustomActionInfo::print(
      OStream& out, const char* space) const
      noexcept
    {
      out << space << "time: " << time.get_gm_time() << std::endl <<
        space << "action_id: " << action_id << std::endl <<
        space << "action_request_id: " << action_request_id << std::endl <<
        space << "referer: " << referer << std::endl <<
        space << "order_id: " << order_id << std::endl <<
        space << "action_value: " << action_value;
      return out;
    }

    /* AdvActionProcessor::AdvActionInfo */
    template<typename OStream>
    OStream& AdvActionProcessor::AdvActionInfo::print(
      OStream& out, const char* space) const
      noexcept
    {
      out << space << "time: " << time.get_gm_time() << std::endl <<
        space << "ccg_id: " << ccg_id << std::endl <<
        space << "user_id: " << user_id << std::endl;

      return out;
    }

    /* AdvActionProcessor::AdvExActionInfo */
    template<typename OStream>
    OStream& AdvActionProcessor::AdvExActionInfo::print(
      OStream& out, const char* space) const
      noexcept
    {
      out << space << "time: " << time.get_gm_time() << std::endl <<
        space << "action_id: " << action_id << std::endl <<
        space << "action_request_id: " << action_request_id << std::endl <<
        space << "user_id: " << user_id << std::endl <<
        space << "referer: " << referer << std::endl <<
        space << "ccg_ids: ";

      for(CCGIdList::const_iterator ccg_id_it = ccg_ids.begin();
          ccg_id_it != ccg_ids.end(); ++ccg_id_it)
      {
        if(ccg_id_it != ccg_ids.begin()) out << ", ";
        out << *ccg_id_it;
      }

      return out;
    }

    /* UnmergedClickProcessor::ClickInfo */
    inline
    std::ostream& UnmergedClickProcessor::ClickInfo::print(
      std::ostream& out, const char* space) const
      noexcept
    {
      out << space << "time: " << time.get_gm_time() << std::endl <<
        space << "request_id: " << request_id << std::endl <<
        space << "referer: " << referer << std::endl;

      return out;
    }
  }
}

#endif /*_REQUEST_ACTION_PROCESSOR_HPP_*/
