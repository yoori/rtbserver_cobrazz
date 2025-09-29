#ifndef _CAMPAIGNCONFIG_HPP_
#define _CAMPAIGNCONFIG_HPP_

#include <list>
#include <vector>
#include <set>
#include <map>

#include <eh/Exception.hpp>
#include <HTTP/UrlAddress.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <String/SubString.hpp>
#include <Generics/LastPtr.hpp>

#include <Commons/StringHolder.hpp>
#include <Commons/AtomicInt.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannelIndex.hpp>

#include "CreativeTemplate.hpp"
#include "CreativeTemplateArgs.hpp"
#include "GeoChannelIndex.hpp"
#include "AvailableAndMinCTRSetter.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  namespace AdInstances
  {
    typedef Generics::Time Timestamp;
    typedef std::set<std::string> StringSet;
    typedef std::list<std::string> StringList;
    typedef std::set<unsigned long> AccountIdSet;
    typedef std::list<unsigned long> AccountIdList;
    //typedef std::vector<unsigned long> AccountIdArray;
    typedef std::map<unsigned long, std::string> PlatformMap;

    enum CreativeCategoryType
    {
      CCT_VISUAL = 0,
      CCT_CONTENT = 1,
      CCT_TAG = 2
    };

    struct AppFormatDef
    {
      AppFormatDef() noexcept
      {}

      AppFormatDef(
        const char* mime_format_val,
        const Timestamp& timestamp_val) noexcept
        : mime_format(mime_format_val),
          timestamp(timestamp_val)
      {}

      std::string mime_format;
      Timestamp timestamp;
    };

    typedef std::map<std::string, AppFormatDef> AppFormatMap;

    class Size: public virtual ReferenceCounting::AtomicImpl
    {
    public:
      unsigned long size_id;
      std::string protocol_name;
      unsigned long size_type_id;
      unsigned long width;
      unsigned long height;
      Timestamp timestamp;

    protected:
      virtual
      ~Size() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Size> Size_var;
    typedef ReferenceCounting::SmartPtr<const Size> ConstSize_var;
    typedef std::map<unsigned long, Size_var> SizeMap;

    struct CreativeOptionDef
    {
      CreativeOptionDef() {}

      std::string token;
      char type;
      StringSet token_relations;
      Timestamp timestamp;

      BaseTokenProcessor_var token_processor;
    };

    typedef std::map<long, CreativeOptionDef> CreativeOptionMap;

    struct CreativeCategory
    {
      typedef std::map<AdRequestType, StringSet> ExternalCategoryMap;

      Timestamp timestamp;
      std::string name;
      ExternalCategoryMap external_categories;
      CreativeCategoryType cct_id;
      // we can't differentiate url exclusion categories with other
      // exclude_domain isn't empty if name is valid url
      std::string exclude_domain;
    };

    class Currency: public virtual ReferenceCounting::AtomicImpl
    {
    public:
      RevenueDecimal rate;
      unsigned long currency_id;
      unsigned long currency_exchange_id;
      unsigned long effective_date;
      unsigned long fraction;
      std::string currency_code;
      Timestamp timestamp;

      RevenueDecimal
      to_system_currency(
        const RevenueDecimal& value,
        Generics::DecimalDivRemainder ddr = Generics::DDR_FLOOR) const
      {
        return RevenueDecimal::div(value, rate, ddr);
      }

      ExtRevenueDecimal
      to_system_currency(const ExtRevenueDecimal& value) const
      {
        ExtRevenueDecimal ext_rate;
        narrow_decimal(ext_rate, rate);
        return ExtRevenueDecimal::div(value, ext_rate);
      }

      RevenueDecimal
      from_system_currency(const RevenueDecimal& value) const
      {
        return RevenueDecimal::mul(value, rate, Generics::DMR_FLOOR);
      }

      RevenueDecimal convert(
        const Currency* from_currency,
        const RevenueDecimal& value) const
      {
        if(rate != from_currency->rate)
        {
          return from_system_currency(
            from_currency->to_system_currency(value));
        }

        return value;
      }

    protected:
      virtual
      ~Currency() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Currency> Currency_var;
    typedef ReferenceCounting::SmartPtr<const Currency> ConstCurrency_var;
    typedef std::map<unsigned long, Currency_var> CurrencyMap;
    typedef Generics::GnuHashTable<Generics::StringHashAdapter, Currency_var>
      CurrencyCodeMap;

    /**
     * Holds frequency cap data. Frequency caps associated with creatives
     * or campaigns are globally identified by fc_id.
     */
    class FreqCap
    {
    public:
      FreqCap(
        unsigned long fc_id_val,
        const Timestamp& timestamp_val,
        unsigned long lifelimit_val,
        const Generics::Time& period_val,
        unsigned long window_limit_val,
        const Generics::Time& window_time_val) noexcept;

    public:
      unsigned long fc_id;
      Timestamp timestamp;
      unsigned long lifelimit; /**< Total number of impressions per user */
      const Generics::Time period; /**< Timeout between 2 impressions */
      unsigned long window_limit; /**< Max number of impressions within windowtime */
      const Generics::Time window_time; /**< Window length */
    };

    typedef std::map<unsigned long, FreqCap> FreqCapMap;

    class AccountDef: public virtual ReferenceCounting::AtomicImpl
    {
    public:
      enum TextAdserving
      {
        TA_ONE,
        TA_ADVERTISER_ONE,
        TA_ALL
      };

      AccountDef()
        : available_(1)
      {}

      TextAdserving get_text_adserving() const
      {
        if(text_adserving == 'O')
        {
          return TA_ONE;
        }
        else if(text_adserving == 'A')
        {
          return TA_ADVERTISER_ONE;
        }

        return TA_ALL;
      }

      bool cost_is_gross() const noexcept
      {
        return at_flags & AccountTypeFlags::GROSS;
      }

      bool invoice_commision() const noexcept
      {
        return at_flags & AccountTypeFlags::INVOICE_COMMISION;
      }

      bool use_self_budget() const noexcept
      {
        return !agency_account.in() ||
          (at_flags & AccountTypeFlags::USE_SELF_BUDGET);
      }

      bool agency_profit_by_pub_amount() const noexcept
      {
        return at_flags & AccountTypeFlags::AGENCY_PROFIT_BY_PUB_AMOUNT;
      }

      bool is_active() const noexcept
      {
        return status == 'A' && eval_status == 'A' &&
          (!agency_account.in() ||
           (agency_account->status == 'A' &&
           agency_account->eval_status == 'A'));
      }

      RevenueDecimal
      get_self_service_commission() const noexcept
      {
        const AccountDef* commision_account =
          agency_account ? agency_account.in() : this;

        return commision_account->self_service_commission;
      }

      RevenueDecimal
      adv_commission() const noexcept
      {
        return commision;
      }
      
      // adapt bid cost
      RevenueDecimal
      adapt_cost(
        const RevenueDecimal& cost,
        const RevenueDecimal& ccg_commision) const noexcept
      {
        RevenueDecimal res_cost = cost;

        const AccountDef* commision_account =
          agency_account ? agency_account.in() : this; // flags & self_service_commission account

        /*
        if(!commision_account->cost_is_gross() &&
          commision_account->auction_rate == AR_GROSS)
        {
          // +commision
          res_cost = RevenueDecimal::div(
            cost,
            REVENUE_ONE - commision);
        }
        else if(commision_account->cost_is_gross() &&
          commision_account->auction_rate == AR_NET)
        {
          // -commision
          res_cost = RevenueDecimal::mul(
            cost,
            REVENUE_ONE - commision,
            Generics::DMR_FLOOR);
        }
        */

        if(media_handling_fee != RevenueDecimal::ZERO)
        {
          // decrease cost for media handling fee (REQ-3944)
          res_cost = RevenueDecimal::mul(
            res_cost,
            REVENUE_ONE - media_handling_fee,
            Generics::DMR_FLOOR);
        }

        // apply ccg level commission
        res_cost = RevenueDecimal::mul(
          cost,
          REVENUE_ONE - ccg_commision,
          Generics::DMR_FLOOR);

        RevenueDecimal adv_commission = commision; // get from advertiser

        // apply margin division model (apply account commission)
        if(!commision_account->agency_profit_by_pub_amount())
        {
          // schema #1 (fix price)
          res_cost = RevenueDecimal::mul(
            res_cost,
            REVENUE_ONE - adv_commission,
            Generics::DMR_FLOOR);
        }
        else
        {
          res_cost = RevenueDecimal::div(
            res_cost,
            REVENUE_ONE + adv_commission);
        }

        res_cost = RevenueDecimal::div(
          res_cost,
          REVENUE_ONE + commision_account->self_service_commission);

        return res_cost;
      }

      // revert adapt_cost changes
      // used for get stats amount in dynamic cost case (ccg keywords)
      RevenueDecimal revert_cost(
        const RevenueDecimal& cost,
        const RevenueDecimal& commision) const noexcept
      {
        RevenueDecimal res_cost = cost;

        if(media_handling_fee != RevenueDecimal::ZERO)
        {
          assert(media_handling_fee != REVENUE_ONE);

          res_cost = RevenueDecimal::div(
            res_cost,
            REVENUE_ONE - media_handling_fee);
        }

        const AccountDef* commision_account =
          agency_account ? agency_account.in() : this;

        if(!commision_account->cost_is_gross() &&
          commision_account->auction_rate == AR_GROSS)
        {
          return RevenueDecimal::mul(
            res_cost,
            REVENUE_ONE - commision,
            Generics::DMR_FLOOR);
        }

        if(commision_account->cost_is_gross() &&
          commision_account->auction_rate == AR_NET)
        {
          return RevenueDecimal::div(
            res_cost,
            REVENUE_ONE - commision);
        }

        return res_cost;
      }

      bool
      is_available() const noexcept;

      unsigned long
      bill_account_id() const noexcept
      {
        return use_self_budget() ? account_id : agency_account->account_id;
      }

      unsigned long
      not_bill_account_id() const noexcept
      {
        if(use_self_budget())
        {
          return agency_account ? agency_account->account_id : 0;
        }

        return account_id;
      }

      void
      set_available(bool available_val) const noexcept;

      unsigned long account_id;
      ReferenceCounting::SmartPtr<AccountDef> agency_account;
      unsigned long internal_account_id;
      std::string legal_name;
      unsigned long flags;
      unsigned long at_flags;
      char text_adserving;
      ConstCurrency_var currency;
      std::string country;
      Generics::Time time_offset;
      RevenueDecimal commision;
      RevenueDecimal media_handling_fee;
      RevenueDecimal budget;
      RevenueDecimal paid_amount;
      char status;
      char eval_status;
      AccountIdSet walled_garden_accounts;
      AuctionRateType auction_rate;
      bool use_pub_pixels;
      std::string pub_pixel_optin;
      std::string pub_pixel_optout;
      RevenueDecimal self_service_commission;

      Timestamp timestamp;

    protected:
      volatile mutable sig_atomic_t available_;

    protected:
      virtual
      ~AccountDef() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<AccountDef> Account_var;
    typedef ReferenceCounting::SmartPtr<const AccountDef> ConstAccount_var;
    typedef std::vector<Account_var> AccountList;
    typedef std::map<unsigned long, Account_var> AccountMap;

    typedef std::set<unsigned long> CreativeCategoryIdSet;

    struct
    CompareAccountByID : public std::binary_function <Account_var, Account_var, bool>
    {
      bool operator()(
        const Account_var& a1,
        const Account_var& a2) const
      {
        return (a1->account_id < a2->account_id);
      }
    };

    /**
     * Encapsulates information on a site.
     */
    class Site: public virtual ReferenceCounting::AtomicImpl
    {
    public:
      enum ExclustioFlags
      {
        SITE_EXCLUSION_FLAG = 0x100,
        TAG_EXCLUSION_FLAG = 0x400
      };

      typedef std::set<unsigned long> CreativeIdSet;

    public:
      unsigned long site_id;
      unsigned long freq_cap_id;
      unsigned long noads_timeout;
      unsigned long flags;
      Account_var account;
      char status;

      Timestamp timestamp;

      CreativeCategoryIdSet approved_creative_categories;
      CreativeCategoryIdSet rejected_creative_categories;
      CreativeIdSet approved_creatives;
      CreativeIdSet rejected_creatives;

    public:
      bool site_exclusion() const noexcept
      {
        return account->at_flags & SITE_EXCLUSION_FLAG;
      }

      bool tag_exclusion() const noexcept
      {
        return account->at_flags & TAG_EXCLUSION_FLAG;
      }

    protected:
      virtual
      ~Site() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Site> Site_var;
    typedef std::map<unsigned long, Site_var> SiteMap;
    typedef std::set<unsigned long> CreativeCategoryIdSet;

    /** Tag */
    class Tag: public virtual ReferenceCounting::AtomicImpl
    {
    public:
      struct TagPricing
      {
        TagPricing()
          : site_rate_id(0),
            cpm(RevenueDecimal::ZERO),
            imp_revenue(0),
            revenue_share(RevenueDecimal::ZERO)
        {}

        unsigned long site_rate_id;
        RevenueDecimal cpm;
        RevenueDecimal imp_revenue;
        RevenueDecimal revenue_share;

        static const TagPricing DEFAULT;
      };

      struct TagPricingKey
      {
        TagPricingKey(
          const char* country_code_val,
          CCGType ccg_type_val,
          CCGRateType ccg_rate_type_val)
          : country_code(country_code_val),
            ccg_type(ccg_type_val),
            ccg_rate_type(ccg_rate_type_val)
        {}

        bool
        operator<(const TagPricingKey& right) const
        {
          return country_code < right.country_code ||
            (country_code == right.country_code && (
              ccg_type < right.ccg_type ||
              (ccg_type == right.ccg_type &&
                ccg_rate_type < right.ccg_rate_type)));
        }

        const std::string country_code;
        const CCGType ccg_type;
        const CCGRateType ccg_rate_type;
      };

      class Size: public ReferenceCounting::AtomicCopyImpl
      {
      public:
        Size(): max_text_creatives(0)
        {}

        Size_var size;
        unsigned long max_text_creatives;
        OptionTokenValueMap tokens;
        OptionTokenValueMap hidden_tokens;

      protected:
        virtual
        ~Size() noexcept = default;
      };

      typedef std::map<TagPricingKey, TagPricing> TagPricings;
      typedef std::map<std::string, TagPricing> CountryTagPricingMap;
      typedef std::map<std::string, const TagPricing*> CountryTagPricingPtrMap;
      typedef std::map<std::string, OptionTokenValueMap>
        TemplateOptionTokenValueMap;
      typedef ReferenceCounting::SmartPtr<Size> Size_var;
      typedef ReferenceCounting::ConstPtr<Size> ConstSize_var;
      typedef std::map<unsigned long, ConstSize_var> SizeMap;

      const TagPricing*
      select_tag_pricing(
        const char* country_code,
        CCGType ccg_type,
        CCGRateType ccg_rate_type) const noexcept;

      // select max cpm linked to country
      const TagPricing*
      select_country_tag_pricing(
        const char* country_code) const noexcept;

      const TagPricing*
      select_no_impression_tag_pricing(
        const char* country_code) const noexcept;

      bool is_test() const noexcept
      {
        return (site->account->flags & static_cast<unsigned long>(AccountFlags::TEST)) != 0;
      }

      unsigned long tag_id;
      Site_var site;

      SizeMap sizes;

      std::string imp_track_pixel;
      std::string passback;
      std::string passback_type;
      unsigned long flags;
      char marketplace;
      RevenueDecimal adjustment;

      TagPricings tag_pricings;
      CountryTagPricingMap country_tag_pricings;
      CountryTagPricingPtrMap no_imp_tag_pricings;

      CreativeCategoryIdSet accepted_categories;
      CreativeCategoryIdSet rejected_categories;
      bool allow_expandable;
      unsigned long min_visibility;

      RevenueDecimal auction_max_ecpm_share;
      RevenueDecimal auction_prop_probability_share;
      RevenueDecimal auction_random_share;
      RevenueDecimal pub_max_random_cpm;
      RevenueDecimal max_random_cpm;
      RevenueDecimal cost_coef;

      OptionTokenValueMap tokens;
      OptionTokenValueMap hidden_tokens;
      OptionTokenValueMap passback_tokens;
      TemplateOptionTokenValueMap template_tokens;
      bool skip_min_ecpm;

      Timestamp tag_pricings_timestamp;
      Timestamp timestamp;

      // duplicate rejected_categories in other view
      //   for fast ccg keyword exclusion detection
      StringSet exclude_creative_domains;
      StringSet tag_pricing_countries;

    protected:
      virtual
      ~Tag() noexcept = default;

    private:
      const TagPricing*
      select_tag_pricing_(
        const char* country_code,
        CCGType ccg_type,
        CCGRateType ccg_rate_type) const noexcept;
    };

    typedef ReferenceCounting::SmartPtr<Tag> Tag_var;
    typedef std::map<unsigned long, Tag_var> TagMap;

    /** Stores configuration of the ad campaign service. */

    /**
     * Encapsulates information on a country.
     */
    class Country: public ReferenceCounting::AtomicImpl
    {
    public:
      OptionTokenValueMap tokens;
      Timestamp timestamp;

    private:
      virtual
      ~Country() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Country> Country_var;


    /**
     * Encapsulates information on a colocation.
     */
    class Colocation: public ReferenceCounting::AtomicImpl
    {
    public:
      bool is_test() const noexcept
      {
        return (at_flags & static_cast<unsigned long>(AccountFlags::TEST)) ? true : false;
      }

      unsigned long colo_id;
      unsigned long colo_rate_id;
      std::string colo_name;
      unsigned long at_flags;
      ConstAccount_var account;
      RevenueDecimal revenue_share;
      ColocationAdServingType ad_serving;
      bool hid_profile;
      OptionTokenValueMap tokens;
      Timestamp timestamp;

    private:
      virtual
      ~Colocation() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Colocation> Colocation_var;

    struct Campaign;

    class Contract: public ReferenceCounting::AtomicImpl
    {
    public:
      unsigned long contract_id;

      std::string number;
      std::string date;
      std::string type;
      bool vat_included;

      std::string ord_contract_id;
      std::string ord_ado_id;
      std::string subject_type;
      std::string action_type;
      bool agent_acting_for_publisher;
      ReferenceCounting::SmartPtr<Contract> parent_contract;

      std::string client_id;
      std::string client_name;
      std::string client_legal_form;

      std::string contractor_id;
      std::string contractor_name;
      std::string contractor_legal_form;

      Timestamp timestamp;
    };

    typedef ReferenceCounting::SmartPtr<Contract>
      Contract_var;

    /**
     * Encapsulates information on a creative.
     */
    class Creative: public ReferenceCounting::AtomicImpl
    {
    public:
      typedef std::vector<unsigned long> CategoryIdArray;
      typedef std::set<unsigned long> CategorySet;

      struct Size
      {
        Size_var size;
        unsigned long up_expand_space;
        unsigned long right_expand_space;
        unsigned long down_expand_space;
        unsigned long left_expand_space;
        bool expandable;
        OptionTokenValueMap tokens;
        StringSet available_appformats;
      };

      typedef std::unordered_map<unsigned long, Size> SizeMap;

      Creative(
        const Campaign* campaign_val,
        unsigned long ccid_val,
        unsigned long creative_id_val,
        unsigned long fc_id_val,
        unsigned long weight_val,
        const char* creative_format_val,
        const char* version_id_val,
        const OptionValue& click_url_val,
        const char* click_url_domain_val,
        const char* short_click_url_val,
        const CategorySet& categories_val)
        /*throw(eh::Exception)*/;

      const Campaign* campaign;
      unsigned long ccid;    /**< Campaign creative identifier */
      unsigned long creative_id;
      unsigned long fc_id;   /**< Frequency cap id for this creative */
      unsigned long weight;  /**< Weight of the creative */

      SizeMap sizes;
      std::string creative_format; /**< Creative format*/
      std::string version_id;

      unsigned long order_set_id;

      char status; //status of creative can be 'W' - wait or 'A' - accepted

      std::string click_url_domain;
      std::string short_click_url;
      OptionValue click_url;

      OptionTokenValueMap tokens;

      CategorySet categories;
      CategorySet click_categories;

      // categories divided by type - optimization for CTR calculation
      CategoryIdArray content_categories;
      CategoryIdArray visual_categories;

      bool defined_content_category;
      HTTP::BrowserAddress destination_url;
      unsigned long video_duration;
      Commons::Optional<unsigned long> video_skip_offset;
      bool https_safe_flag;
      std::string erid;
      Contract_var initial_contract;

    protected:
      virtual
      ~Creative() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<Creative>
      Creative_var;

    typedef std::list<Creative_var> CreativeList;
    typedef std::list<const Creative*> OptCreativePtrList;

    typedef std::map<unsigned long, Creative_var>
      CreativeMap;
    typedef Generics::GnuHashTable<
      Generics::NumericHashAdapter<unsigned long>, Creative_var>
      CcidMap;

    typedef std::set<unsigned long> ColoIdSet;

    typedef std::set<unsigned long> SiteIdSet;

    struct Campaign:
      public AvailableAndMinCTRSetter,
      public virtual ReferenceCounting::AtomicImpl
    {
    public:
      typedef std::set<unsigned long> OrderSetIdSet;
      typedef std::unordered_set<unsigned long> SizeIdSet;

      Campaign() noexcept;

      bool targeted() const noexcept;

      bool track_actions() const noexcept;

      bool exclude_clickurl() const noexcept;

      bool include_specific_sites() const noexcept;

      bool is_test() const noexcept;

      bool channel_based() const noexcept;

      bool keyword_based() const noexcept;

      bool is_text() const noexcept;

      bool use_ctr() const noexcept
      {
        return ccg_rate_type == CR_CPC;
      }

      bool is_active() const noexcept
      {
        return (status == 'A' || status == 'V') &&
          eval_status == 'A' &&
          account->is_active() && advertiser->is_active();
      }

      bool
      is_available() const noexcept;

      void add_creative(Creative* new_creative) noexcept;

      const CreativeList& get_creatives() const noexcept;

      void
      set_min_ctr_goal(const RevenueDecimal& goal_ctr)
        const noexcept;

      unsigned long
      int_min_ctr_goal() const noexcept;

      RevenueDecimal
      min_ctr_goal() const noexcept;

      std::string campaign_group_name;
      std::string campaign_name;

      unsigned long campaign_id;       /**< Campaign identifier */
      unsigned long campaign_group_id; /**< Campaign group identifier */
      Timestamp timestamp;
      ConstAccount_var account;
      ConstAccount_var advertiser;

      unsigned long fc_id;             /**< Campaign's frequency cap id */
      unsigned long group_fc_id;       /**< Campaign group's freq cap id */

      unsigned long ccg_rate_id;
      CCGRateType ccg_rate_type;

      unsigned long flags;
      char marketplace;

      char status; /** campaign status, one of A,I,V */
      char eval_status;

      WeeklyRunIntervalSet weekly_run_intervals;  /** Daily run */

      ExpressionChannel_var channel;
      ExpressionChannel_var stat_channel;
      ExpressionChannelBase_var fast_channel;

      std::string country;
      SiteIdSet sites;                 /**< Sites */

      ColoIdSet colocations;
      AccountIdSet exclude_pub_accounts;
      TagDeliveryMap exclude_tags;
      unsigned long delivery_coef;

      RevenueDecimal imp_revenue;
      RevenueDecimal click_revenue;
      RevenueDecimal click_sys_revenue;
      RevenueDecimal action_revenue;
      RevenueDecimal commision;

      CCGType ccg_type;
      char targeting_type;
      unsigned long start_user_group_id;
      unsigned long end_user_group_id;
      unsigned long ctr_reset_id;
      CampaignMode mode;
      Generics::Time min_uid_age;
      unsigned long seq_set_rotate_imps;

      CampaignDeliveryLimits campaign_delivery_limits;
      CampaignDeliveryLimits ccg_delivery_limits;

      RevenueDecimal max_pub_share;
      BidStrategy bid_strategy;

      RevenueDecimal ecpm_; // for find all ecpm usages
      RevenueDecimal ctr;

      bool has_custom_actions;
      bool ctr_modifiable;

      /* optimization */
      SizeIdSet opt_available_sizes;
      CreativeList creatives;          /**< Campaign creatives */
      OrderSetIdSet opt_order_sets;
      RevenueDecimal base_min_ctr_goal;

    protected:
      typedef Sync::Policy::PosixSpinThread GoalCTRSyncPolicy;

    protected:
      volatile mutable sig_atomic_t available_;
      mutable Algs::AtomicInt int_min_ctr_goal_;

    protected:
      virtual
      ~Campaign() noexcept = default;

      virtual void
      set_available(bool available_val, const RevenueDecimal& goal_ctr)
        const noexcept;
    };

    typedef ReferenceCounting::SmartPtr<Campaign>
      Campaign_var;

    typedef ReferenceCounting::SmartPtr<const Campaign>
      ConstCampaign_var;

    typedef Generics::GnuHashSet<Generics::StringHashAdapter> KeywordSet;

    struct CampaignKeywordBase
    {
      unsigned long ccg_keyword_id;
      std::string click_url;
      std::string original_keyword;
    };

    typedef std::map<unsigned long, CampaignKeywordBase>
      CCGKeywordPostClickInfoMap;

    struct AdvActionDef
    {
      typedef std::list<unsigned long> CCGIdList;

      unsigned long action_id;
      RevenueDecimal cur_value;
      Timestamp timestamp;
      CCGIdList ccg_ids;
    };

    typedef std::map<unsigned long, AdvActionDef> AdvActionMap;

    class CategoryChannel: public virtual ReferenceCounting::AtomicImpl
    {
    public:
      typedef std::map<std::string, std::string> LocalizationMap;

      unsigned long channel_id;
      std::string name;
      std::string newsgate_name;
      unsigned long parent_channel_id;
      LocalizationMap localizations;
      unsigned long flags;
      Timestamp timestamp;

    protected:
      virtual
      ~CategoryChannel() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<CategoryChannel> CategoryChannel_var;

    class CategoryChannelNode;
    typedef ReferenceCounting::SmartPtr<CategoryChannelNode> CategoryChannelNode_var;

    /* CategoryChannelNodeMap: category name -> category node */
    typedef std::multimap<std::string, CategoryChannelNode_var> CategoryChannelNodeMap;

    class CategoryChannelNode: public virtual ReferenceCounting::AtomicImpl
    {
    public:
      unsigned long channel_id;
      std::string name;
      CategoryChannel::LocalizationMap localizations;
      unsigned long flags;
      CategoryChannelNodeMap child_category_channels;

    protected:
      virtual
      ~CategoryChannelNode() noexcept = default;
    };

    class SimpleChannelCategories: public ReferenceCounting::AtomicImpl
    {
    public:
      typedef std::vector<unsigned long> CategoryIdList;
      CategoryIdList categories;

    protected:
      virtual
      ~SimpleChannelCategories() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<SimpleChannelCategories>
      SimpleChannelCategories_var;

    struct PubPixelAccountKey
    {
      PubPixelAccountKey()
        : country(""),
          user_status(US_UNDEFINED)
      {
        calc_hash();
      }

      PubPixelAccountKey(const char* country_val, UserStatus user_status_val)
        noexcept
        : country(country_val),
          user_status(user_status_val)
      {
        calc_hash();
      }

      unsigned long
      hash() const noexcept
      {
        return hash_;
      }

      bool
      operator==(const PubPixelAccountKey& right) const noexcept
      {
        return country == right.country &&
          user_status == right.user_status;
      }

      const std::string country;
      const UserStatus user_status;

    private:
      void calc_hash()
      {
        Generics::Murmur64Hash hasher(hash_);
        hash_add(hasher, country);
        hash_add(hasher, static_cast<unsigned long>(user_status));
      }

    private:
      size_t hash_;
    };

    typedef std::set<Account_var, CompareAccountByID> AccountSet;

    typedef Generics::GnuHashTable<PubPixelAccountKey, AccountSet>
      PubPixelAccountMap;

    class WebOperation: public ReferenceCounting::AtomicImpl
    {
    public:
      unsigned int id;
      std::string app;
      std::string source;
      std::string operation;
      unsigned long flags;

    protected:
      virtual
      ~WebOperation() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<WebOperation>
      WebOperation_var;

    struct WebOperationKey
    {
      WebOperationKey() noexcept{};

      WebOperationKey(
        const char* app_val,
        const char* source_val,
        const char* operation_val) noexcept
        : app(app_val),
          source(source_val),
          operation(operation_val)
      {
        calc_hash_();
      }

      bool
      operator==(const WebOperationKey& right) const noexcept
      {
        return app == right.app &&
          source == right.source &&
          operation == right.operation;
      }

      unsigned long
      hash() const noexcept
      {
        return hash_;
      }

      const String::SubString app;
      const String::SubString source;
      const String::SubString operation;
    private:
      void calc_hash_()
      {
        Generics::Murmur64Hash hasher(hash_);
        hash_add(hasher, app);
        hash_add(hasher, source);
        hash_add(hasher, operation);
      }
      size_t hash_;
    };

    typedef Generics::GnuHashTable<WebOperationKey, WebOperation_var>
      WebOperationHash;

    struct IdTagKey
    {
      IdTagKey(
        unsigned long id_val,
        const char* size_val)
        : id(id_val),
          size(size_val)
      {
        calc_hash();
      }

      bool
      operator==(const IdTagKey& right) const
      {
        return id == right.id && size == right.size;
      }

      unsigned long
      hash() const noexcept
      {
        return hash_;
      }

      const unsigned long id;
      const std::string size;

    private:
      void calc_hash()
      {
        Generics::Murmur64Hash hasher(hash_);
        hash_add(hasher, id);
        hash_add(hasher, size);
      }

    private:
      size_t hash_;
    };

    class CampaignConfig: public Generics::Last<ReferenceCounting::AtomicImpl>
    {
    public:
      struct PlatformChannelHolder
      {
        PlatformChannelHolder(
          unsigned long priority_val,
          const String::SubString& norm_name_val)
          noexcept;

        unsigned long priority;
        std::string norm_name;
      };

      typedef std::list<Campaign_var> CampaignList;

      typedef std::map<unsigned long, CreativeCategory> CreativeCategoryMap;

      typedef std::unordered_map<unsigned long, Campaign_var> CampaignMap;
      typedef std::unordered_map<unsigned long, Colocation_var> ColocationMap;
      typedef std::map<std::string, Country_var> CountryMap;
      typedef std::map<unsigned long, CategoryChannel_var> CategoryChannelMap;
      typedef std::map<unsigned long, SimpleChannelCategories_var> SimpleChannelMap;
      typedef ExpressionChannelHolderMap ChannelMap;

      typedef std::unordered_map<unsigned long, PlatformChannelHolder> PlatformChannelPriorityMap;
      typedef std::unordered_map<std::string, Tag_var> SizeTagMap;
      typedef std::map<AdRequestType, Account_var> AdRequestTypeAccountMap;
      typedef std::map<AdRequestType, SizeTagMap> AdRequestTypeTagMap;
      typedef Generics::GnuHashTable<IdTagKey, std::vector<Tag_var> > IdTagMap;

      typedef std::list<ExpressionChannelHolder_var> ExpressionChannelHolderList;
      typedef std::map<unsigned long, ExpressionChannelHolderList> BlockChannelMap;

      typedef Generics::GnuHashTable<Generics::StringHashAdapter, CreativeCategoryIdSet>
        ExternalCategoryNameMap;
      typedef std::map<AdRequestType, ExternalCategoryNameMap>
        ExternalCategoryMap;
      typedef std::unordered_map<unsigned long, Contract_var> ContractMap;

    public:
      CampaignConfig() noexcept;

    public:
      CurrencyMap currencies;
      AccountMap accounts;
      SiteMap sites;
      TagMap tags;
      CampaignMap campaigns;

      /*
       * Campaign with invalid account, etc MUST be moved to this list,
       * before erasing from campaigns, because because they can
       * required to process a click (see post_click_info_map)
       */
      CampaignList inconsistent_campaigns;

      Timestamp master_stamp;
      Timestamp first_load_stamp;
      Timestamp finish_load_stamp;
      Timestamp global_params_timestamp;
      unsigned long currency_exchange_id;
      Generics::Time fraud_user_deactivate_period;
      RevenueDecimal cost_limit;
      unsigned long google_publisher_account_id;

      AppFormatMap app_formats;
      SizeMap sizes;
      CreativeOptionMap creative_options;
      TokenProcessorMap token_processors;
      BaseTokenProcessor_var default_click_token_processor;

      FreqCapMap freq_caps;
      CreativeTemplateMap creative_templates;

      ChannelMap expression_channels;
      ChannelMap discover_channels;

      CreativeCategoryMap creative_categories;

      ColocationMap colocations;
      CountryMap countries;
      AdvActionMap adv_actions;
      CategoryChannelMap category_channels;
      SimpleChannelMap simple_channels;
      BlockChannelMap block_channels;

      CategoryChannelNodeMap category_channel_nodes;
      StringSet all_template_appformats;
      CCGKeywordPostClickInfoMap ccg_keyword_click_info_map;
      CreativeMap creatives; // creative_id => creative
      CcidMap campaign_creatives; // cc_id => creative
      CurrencyCodeMap currency_codes;

      // force loading of geo channels, if channels isn't changed
      Generics::Time geo_channels_timestamp;
      GeoChannelIndex_var geo_channels;
      GeoCoordChannelIndex_var geo_coord_channels;
      ExpressionChannelIndex_var platform_channels;
      PlatformChannelPriorityMap platform_channel_priorities;

      PlatformMap platforms;

      WebOperationHash web_operations;

      PubPixelAccountMap pub_pixel_accounts;
      AdRequestTypeAccountMap external_system_accounts;
      IdTagMap account_tags;
      IdTagMap site_tags;

      ExternalCategoryMap external_creative_categories;
      ContractMap contracts;

    protected:
      virtual
      ~CampaignConfig() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<const CampaignConfig>
      ConstCampaignConfig_var;

    typedef ReferenceCounting::SmartPtr<CampaignConfig>
      CampaignConfig_var;

    typedef ReferenceCounting::QualPtr<CampaignConfig>
      QCampaignConfig_var;
  } /* CampaignConfig */
} /* CampaignSvcs */
} /* AdServer */


namespace AdServer
{
namespace CampaignSvcs
{
  namespace AdInstances
  {
    namespace
    {
      const RevenueDecimal CTR_MULTIPLIER = RevenueDecimal(false, 100000, 0);
    }
    
    /** FreqCap class */
    inline
    FreqCap::FreqCap(
      unsigned long fc_id_val,
      const Timestamp& timestamp_val,
      unsigned long lifelimit_val,
      const Generics::Time& period_val,
      unsigned long window_limit_val,
      const Generics::Time& window_time_val)
      noexcept
      : fc_id(fc_id_val),
        timestamp(timestamp_val),
        lifelimit(lifelimit_val),
        period(period_val),
        window_limit(window_limit_val),
        window_time(window_time_val)
    {}

    // AccountDef impl
    inline bool
    AccountDef::is_available() const noexcept
    {
      return available_;
    }

    inline void
    AccountDef::set_available(bool available_val) const noexcept
    {
      available_ = available_val;
    }

    /** Campaign */
    inline
    Campaign::Campaign() noexcept
      : ctr_modifiable(false),
        base_min_ctr_goal(RevenueDecimal::ZERO),
        available_(1),
        int_min_ctr_goal_(0)
    {}

    inline
    bool Campaign::targeted() const noexcept
    {
      return channel.in();
    }

    inline
    bool Campaign::track_actions() const noexcept
    {
      return (flags & CampaignFlags::TRACK_ACTIONS) ? true : false;
    }

    inline
    bool Campaign::include_specific_sites() const noexcept
    {
      return (flags & CampaignFlags::INCLUDE_SPECIFIC_SITES) ? true : false;
    }

    inline
    bool Campaign::is_test() const noexcept
    {
      return (account->flags & static_cast<unsigned long>(AccountFlags::TEST)) ? true : false;
    }

    inline
    bool Campaign::channel_based() const noexcept
    {
      return targeting_type == 'C';
    }

    inline
    bool Campaign::keyword_based() const noexcept
    {
      return targeting_type == 'K';
    }

    inline
    bool Campaign::is_text() const noexcept
    {
      return ccg_type == CT_TEXT;
    }

    inline bool
    Campaign::is_available() const noexcept
    {
      /*
      std::cerr << "is_available: ccg_id = " << campaign_id <<
        ", available = " << available_ << std::endl;
      */
      return available_;
    }

    inline
    void Campaign::add_creative(Creative* new_creative) noexcept
    {
      creatives.push_back(ReferenceCounting::add_ref(new_creative));
      if(new_creative->status == 'A' || new_creative->status == 'W')
      {
        opt_order_sets.insert(new_creative->order_set_id);
        for(Creative::SizeMap::const_iterator size_it =
              new_creative->sizes.begin();
            size_it != new_creative->sizes.end(); ++size_it)
        {
          opt_available_sizes.insert(size_it->first);
        }
      }
    }

    inline
    const CreativeList&
    Campaign::get_creatives() const noexcept
    {
      return creatives;
    }

    inline void
    Campaign::set_available(bool available_val, const RevenueDecimal& goal_ctr)
      const noexcept
    {
      Algs::AtomicInt mem_barrier(0);
      available_ = available_val;      
      set_min_ctr_goal(goal_ctr);
      mem_barrier += 1;
    }

    inline void
    Campaign::set_min_ctr_goal(const RevenueDecimal& goal_ctr)
      const noexcept
    {
      if(ctr_modifiable)
      {
        int_min_ctr_goal_ = RevenueDecimal::mul(
          std::max(base_min_ctr_goal, goal_ctr),
          CTR_MULTIPLIER,
          Generics::DMR_FLOOR).floor(0).integer<unsigned long>();
      }
    }

    inline unsigned long
    Campaign::int_min_ctr_goal() const noexcept
    {
      return int_min_ctr_goal_;
    }

    inline RevenueDecimal
    Campaign::min_ctr_goal() const noexcept
    {
      return RevenueDecimal::div(
        RevenueDecimal(false, static_cast<int>(int_min_ctr_goal_), 0),
        CTR_MULTIPLIER,
        Generics::DDR_FLOOR);
    }

    inline
    CampaignConfig::PlatformChannelHolder::PlatformChannelHolder(
      unsigned long priority_val,
      const String::SubString& norm_name_val)
      noexcept
      : priority(priority_val),
        norm_name(norm_name_val.str())
    {
      String::AsciiStringManip::to_lower(norm_name);
    }

    /** CampaignConfig class */
    inline
    CampaignConfig::CampaignConfig() noexcept
      : master_stamp(0)
    {}
  }
}
}

#endif /*_CAMPAIGNCONFIG_HPP_*/
