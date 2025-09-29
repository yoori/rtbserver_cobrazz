#ifndef _CAMPAIGNCONFIG_HPP_
#define _CAMPAIGNCONFIG_HPP_

#include <string>
#include <list>
#include <set>
#include <map>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <String/StringManip.hpp>
#include <Generics/SimpleDecimal.hpp>

#include <Commons/GranularContainer.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/Algs.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignCommons.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignCommons_s.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include "NonLinkedExpressionChannel.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    typedef AdServer::Commons::TimestampValue TimestampValue;
    typedef std::set<std::string> StringSet;

    struct SizeDef:
      public ReferenceCounting::AtomicImpl
    {
    public:
      bool operator==(const SizeDef& right) const noexcept;

      std::string protocol_name;
      unsigned long size_type_id;
      unsigned long width;
      unsigned long height;
      unsigned long max_width;
      unsigned long max_height;

      Generics::Time timestamp;

    protected:
      virtual ~SizeDef() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<SizeDef>
      SizeDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, SizeDef_var>
      SizeMap;

    struct CountryDef:
       public ReferenceCounting::AtomicImpl
    {
      bool operator==(const CountryDef& right) const noexcept;

      OptionValueMap tokens;

      Generics::Time timestamp;

    protected:
      virtual ~CountryDef() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<CountryDef>
    CountryDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      std::string, CountryDef_var>
      CountryMap;

    class WebOperationDef:
      public ReferenceCounting::AtomicImpl
    {
    public:
      bool operator==(const WebOperationDef& right) const noexcept;

      std::string app;
      std::string source;
      std::string operation;
      unsigned int flags;

      Generics::Time timestamp;
    protected:
      virtual
      ~WebOperationDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<WebOperationDef>
      WebOperationDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, WebOperationDef_var>
      WebOperationMap;

    class StringDictionaryDef:
      public ReferenceCounting::AtomicImpl,
      public std::set<std::string>
    {
    public:
      bool operator==(const StringDictionaryDef& right) const noexcept;

      Generics::Time timestamp;

    protected:
      virtual
      ~StringDictionaryDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<StringDictionaryDef>
      StringDictionaryDef_var;

    struct AppFormatDef: public ReferenceCounting::AtomicImpl
    {
      AppFormatDef() noexcept {}

      std::string mime_format;
      Generics::Time timestamp;

      bool operator==(const AppFormatDef& right) const noexcept;

    protected:
      virtual ~AppFormatDef() noexcept 
      {}
    };

    typedef ReferenceCounting::SmartPtr<AppFormatDef>
      AppFormatDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      std::string, AppFormatDef_var>
      AppFormatMap;

    class CreativeOptionDef: public ReferenceCounting::AtomicImpl
    {
    public:
      CreativeOptionDef() noexcept {}

      CreativeOptionDef(
        const char* token_val,
        char type_val,
        const StringSet& token_relations_val,
        const Generics::Time& timestamp_val = Generics::Time::ZERO)
        noexcept;

      bool operator==(const CreativeOptionDef& right) const noexcept;

      std::string token;
      char type;
      StringSet token_relations;
      Generics::Time timestamp;

    protected:
      virtual
      ~CreativeOptionDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<CreativeOptionDef>
      CreativeOptionDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      long, CreativeOptionDef_var>
      CreativeOptionMap;

    typedef std::set<unsigned long> CreativeCategorySet;

    /**
     * Incapsulates information on a creative.
     */
    class CreativeDef: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      struct Size
      {
        Size()
          : up_expand_space(0), right_expand_space(0),
            down_expand_space(0), left_expand_space(0)
        {}

        bool operator==(const Size& right) const noexcept
        {
          return up_expand_space == right.up_expand_space &&
            right_expand_space == right.right_expand_space &&
            down_expand_space == right.down_expand_space &&
            left_expand_space == right.left_expand_space &&
            tokens.size() == right.tokens.size() &&
            std::equal(tokens.begin(),
              tokens.end(),
              right.tokens.begin());
        }

        unsigned long up_expand_space;
        unsigned long right_expand_space;
        unsigned long down_expand_space;
        unsigned long left_expand_space;
        OptionValueMap tokens;
      };

      typedef std::map<unsigned long, Size> SizeMap;

      struct SystemOptions
      {
        SystemOptions() noexcept;

        SystemOptions(
          long click_url_option_id,
          const char* click_url_val,
          long html_url_option_id,
          const char* html_url_val)
          /*throw(eh::Exception)*/;

        bool operator==(const SystemOptions& right) const noexcept;

        long click_url_option_id;
        std::string click_url;
        long html_url_option_id;
        std::string html_url;
      };

      CreativeDef(
        unsigned long ccid_val,
        unsigned long creative_id_val,
        unsigned long fc_id_val,
        unsigned long weight,
        const char* creative_format_val,
        const char* version_id_val,
        char status_val,
        const SystemOptions& sys_options_val,
        const OptionValueSeq& tokens_val = OptionValueSeq(),
        const CreativeCategoryIdSeq& categories_val =
          CreativeCategoryIdSeq())
        /*throw(eh::Exception)*/;

      void add_token(const char* name_val, const char* value_val)
        /*throw(eh::Exception)*/;

      bool operator==(const CreativeDef& right) const noexcept;

      unsigned long ccid;        /** Campaign creative identifier */
      unsigned long creative_id;
      unsigned long fc_id;       /** Frequency cap id for this creative */
      unsigned long weight;      /** Weight of the creative */
      std::string format;
      SizeMap sizes;
      std::string version_id;
      char status;

      SystemOptions sys_options;
      unsigned long initial_contract_id;
      unsigned long order_set_id;
      CreativeCategorySet categories;
      OptionValueMap tokens;

    private:
      virtual
      ~CreativeDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<CreativeDef> CreativeDef_var;

    typedef std::list<CreativeDef_var> CreativeList;

    typedef std::set<unsigned long> SiteIdSet;
    typedef std::set<unsigned long> ColoIdSet;
    typedef std::set<unsigned long> CampaignIdSet;
    typedef std::set<unsigned long> AccountIdSet;
    typedef std::set<unsigned long> TagIdSet;
    typedef std::set<unsigned long> UserGroupIdSet;
    typedef std::set<unsigned long> CCIdSet;

    class ContractDef: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      bool operator==(const ContractDef& right) const noexcept;

      unsigned long contract_id;

      // base contract fields
      std::string number;
      std::string date;
      std::string type;
      bool vat_included;

      // specific contract fields
      std::string ord_contract_id;
      std::string ord_ado_id;
      std::string subject_type;
      std::string action_type;
      bool agent_acting_for_publisher;
      unsigned long parent_contract_id;

      // contract sides
      std::string client_id; // inn for Russia
      std::string client_name;
      std::string client_legal_form;

      std::string contractor_id; // inn for Russia
      std::string contractor_name;
      std::string contractor_legal_form;

      TimestampValue timestamp;
    };

    typedef ReferenceCounting::SmartPtr<ContractDef> ContractDef_var;

    /**
     * Holds information on particular campaign.
     */
    class CampaignDef: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      enum CcgFlags
      {
        CCG_CREATIVE_OPTIMIZATION = 0x20
      };

      CampaignDef(): eval_status('A') {}

      bool operator==(const CampaignDef& right) const noexcept;

      char get_status() const noexcept
      {
        if(status == 'I')
        {
          return 'I';
        }
        else if(status == 'A' && eval_status == 'A')
        {
          return 'A';
        }

        // campaigns deactivated on adserver side only (eval_status is 'I' and != status)
        // always have Pending status
        return 'P';
      }

      bool creative_optimization() const noexcept
      {
        return flags & CampaignDef::CCG_CREATIVE_OPTIMIZATION;
      }

      typedef std::map<std::string, std::string> OptionMap;

      unsigned long campaign_group_id; /**< Campaign group identifier */
      unsigned long ccg_rate_id;
      char ccg_rate_type;

      unsigned long fc_id;             /**< Campaign's frequency cap id */
      unsigned long group_fc_id;       /**< Campaign group's freq cap id */
      NonLinkedExpressionChannel::Expression expression;
      NonLinkedExpressionChannel::Expression stat_expression;
      std::string country;

      /* campaign status, one of A,I,V */
      char status;
      char eval_status; // A,I filled by CampaignConfigModifier

      SiteIdSet sites;                 /**< Site ID's */
      CreativeList creatives;          /**< Campaign creatives */

      WeeklyRunIntervalSet weekly_run_intervals;

      Generics::Time min_uid_age;
      ColoIdSet colocations;

      AccountIdSet exclude_pub_accounts;
      TagDeliveryMap exclude_tags; // filled by CampaignConfigModifier
      unsigned long delivery_coef; // filled by CampaignConfigModifier

      RevenueDecimal imp_revenue;
      RevenueDecimal click_revenue;
      RevenueDecimal action_revenue;
      RevenueDecimal commision;

      char marketplace; // W - walled garden, O - OIX, A - all
      unsigned long flags;

      unsigned long account_id;
      unsigned long advertiser_id;

      char ccg_type;
      char target_type;

      CampaignDeliveryLimits campaign_delivery_limits;
      CampaignDeliveryLimits ccg_delivery_limits;
      unsigned long start_user_group_id;
      unsigned long end_user_group_id;

      RevenueDecimal max_pub_share;

      unsigned long ctr_reset_id;
      CampaignMode mode;

      unsigned long seq_set_rotate_imps;
      BidStrategy bid_strategy;
      RevenueDecimal min_ctr_goal;

      unsigned long initial_contract_id;

      TimestampValue timestamp;

    private:
      virtual
      ~CampaignDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<CampaignDef> Campaign_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, Campaign_var>
      CampaignMap;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, ContractDef_var>
      ContractMap;

    class EcpmDef: public ReferenceCounting::AtomicImpl
    {
    public:
      explicit
      EcpmDef(
        const RevenueDecimal& ecpm_val = RevenueDecimal::ZERO,
        const RevenueDecimal& ctr_val = RevenueDecimal::ZERO,
        const TimestampValue& timestamp_val = Generics::Time::ZERO)
        : ecpm(ecpm_val),
          ctr(ctr_val),
          timestamp(timestamp_val)
      {}

      bool operator==(const EcpmDef& right) const noexcept
      {
        return ecpm == right.ecpm &&
          ctr == right.ctr;
      }

      RevenueDecimal ecpm;
      RevenueDecimal ctr;
      TimestampValue timestamp;

    protected:
      virtual
      ~EcpmDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<EcpmDef> Ecpm_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, Ecpm_var> EcpmMap;

    struct TagPricingDef
    {
      bool operator==(const TagPricingDef& right) const noexcept;

      std::string country_code;
      CCGType ccg_type;
      CCGRateType ccg_rate_type;

      unsigned long site_rate_id;
      RevenueDecimal imp_revenue;
      RevenueDecimal revenue_share;
    };

    typedef std::vector<TagPricingDef> TagPricings;

    typedef std::map<std::string, OptionValueMap>
      TemplateOptionValueMap;

    class TagDef: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      struct Size
      {
        Size(): max_text_creatives(0)
        {}

        bool operator==(const Size& right) const noexcept
        {
          return max_text_creatives == right.max_text_creatives &&
            tokens.size() == right.tokens.size() &&
            std::equal(tokens.begin(),
              tokens.end(),
              right.tokens.begin());
        }

        unsigned long max_text_creatives;
        OptionValueMap tokens;
        OptionValueMap hidden_tokens;
      };

      typedef std::map<unsigned long, Size> SizeMap;

    public:
      bool operator==(const TagDef& right) const noexcept;

      unsigned long tag_id;
      unsigned long site_id;
      SizeMap sizes;
      std::string imp_track_pixel;
      std::string passback;
      std::string passback_type;

      bool allow_expandable;

      CreativeCategorySet accepted_categories;
      CreativeCategorySet rejected_categories;
      TagPricings tag_pricings;

      unsigned long flags;
      char marketplace; // W - walled garden, O - OIX, A - all
      RevenueDecimal adjustment;
      OptionValueMap tokens;
      OptionValueMap hidden_tokens;
      OptionValueMap passback_tokens;
      TemplateOptionValueMap template_tokens;

      RevenueDecimal auction_max_ecpm_share;
      RevenueDecimal auction_prop_probability_share;
      RevenueDecimal auction_random_share;
      RevenueDecimal cost_coef;

      TimestampValue tag_pricings_timestamp;
      TimestampValue timestamp;

    private:
      virtual
      ~TagDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<TagDef> TagDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, TagDef_var> TagMap;

    typedef std::list<unsigned long> SiteCreativeExclusionList;
    typedef std::list<unsigned long> CreativeCategoryIdList;
    typedef std::set<unsigned long> CreativeIdSet;

    class SiteDef: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      bool operator==(const SiteDef& right) const noexcept;

      char status;
      unsigned long freq_cap_id;

      int noads_timeout;

      unsigned long flags;
      unsigned long account_id;

      CreativeCategoryIdList approved_creative_categories;
      CreativeCategoryIdList rejected_creative_categories;

      CreativeIdSet approved_creatives;
      CreativeIdSet rejected_creatives;

      TimestampValue timestamp;

    private:
      virtual
      ~SiteDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<SiteDef> SiteDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, SiteDef_var>
      SiteMap;

    class AccountDef: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      unsigned long account_id;
      unsigned long agency_account_id;
      unsigned long internal_account_id;
      unsigned long role_id;
      std::string legal_name;
      unsigned long flags;
      unsigned long at_flags;
      char text_adserving;
      unsigned long currency_id;
      std::string country;
      Generics::Time time_offset;
      RevenueDecimal commision;
      RevenueDecimal media_handling_fee;
      RevenueDecimal budget;
      RevenueDecimal paid_amount;
      AccountIdSet walled_garden_accounts;
      AuctionRateType auction_rate;
      bool use_pub_pixels;
      std::string pub_pixel_optin;
      std::string pub_pixel_optout;
      RevenueDecimal self_service_commission;
      char status;
      char eval_status;

      TimestampValue timestamp;

    public:
      AccountDef() noexcept: eval_status('A') {}

      bool operator==(const AccountDef& right) const noexcept;

      char get_status() const noexcept
      {
        if(status == 'I')
        {
          return 'I';
        }
        else if(status == 'A' && eval_status == 'A')
        {
          return 'A';
        }

        // accounts deactivated on adserver side only (eval_status is 'I' and != status)
        // always have Pending status
        return 'P';
      }

      bool is_gross() const noexcept
      {
        return at_flags & AccountTypeFlags::GROSS;
      }

      bool invoice_commision() const noexcept
      {
        return at_flags & AccountTypeFlags::INVOICE_COMMISION;
      }

      bool is_test() const noexcept
      {
        return flags & static_cast<unsigned long>(AccountFlags::TEST);
      }

      bool use_self_budget() const noexcept
      {
        return at_flags & AccountTypeFlags::USE_SELF_BUDGET;
      }

      bool is_advertiser() const noexcept
      {
        return role_id == 1;
      }

      bool is_agency() const noexcept
      {
        return role_id == 4;
      }

    protected:
      virtual
      ~AccountDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<AccountDef>
      AccountDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, AccountDef_var>
      AccountMap;

    class CurrencyDef: public ReferenceCounting::AtomicImpl
    {
    public:
      bool operator==(const CurrencyDef& right) const noexcept;

      RevenueDecimal rate;
      unsigned long currency_id;
      unsigned long currency_exchange_id;
      unsigned long effective_date;
      unsigned long fraction_digits;
      std::string currency_code;

      TimestampValue timestamp;

    private:
      virtual
      ~CurrencyDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<CurrencyDef> CurrencyDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, CurrencyDef_var>
      CurrencyMap;

    class ColocationDef: public ReferenceCounting::AtomicImpl
    {
    public:
      ColocationDef()
        : colo_id(0),
          colo_rate_id(0),
          at_flags(0),
          account_id(0)
      {}

      bool operator==(const ColocationDef& right) const noexcept;

      unsigned long colo_id;
      std::string colo_name;
      unsigned long colo_rate_id;
      unsigned long at_flags;
      unsigned long account_id;
      RevenueDecimal revenue_share;
      ColocationAdServingType ad_serving;
      bool hid_profile;
      OptionValueMap tokens;

      TimestampValue timestamp;

    private:
      virtual
      ~ColocationDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<ColocationDef>
      ColocationDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, ColocationDef_var>
      ColocationMap;

    /**
     * Holds frequency cap data. Frequency caps associated with creatives
     * or campaigns are globally identified by fc_id.
     */
    class FreqCapDef: public ReferenceCounting::AtomicImpl
    {
    public:
      FreqCapDef() noexcept;

      FreqCapDef(
        unsigned long fc_id_val,
        unsigned long lifelimit_val,
        unsigned long period_val,
        unsigned long window_limit_val,
        unsigned long window_time_val,
        const TimestampValue& ts_val) noexcept;

      bool operator==(const FreqCapDef& cp) const noexcept;

      unsigned long fc_id;     /**< Identifier */
      unsigned long lifelimit; /**< Total number of impressions per user */
      unsigned long period;    /**< Timeout in seconds between 2 impressions */
      unsigned long window_limit; /**< Max number of impressions within windowtime */
      unsigned long window_time; /**< Window length in seconds */

      TimestampValue timestamp;

    protected:
      virtual
      ~FreqCapDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<FreqCapDef> FreqCapDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, FreqCapDef_var>
      FreqCapMap;

    struct CreativeTemplateFileDef
    {
      CreativeTemplateFileDef(
        const char* creative_format_val,
        const char* creative_size_val,
        const char* app_format_val,
        const char* mime_format_val,
        bool track_impr_val,
        AdServer::CampaignSvcs::CreativeTemplateType type_val,
        const char* template_file_val)
        noexcept;

      bool operator==(const CreativeTemplateFileDef& right) const noexcept;

      std::string creative_format;
      std::string creative_size;
      std::string app_format;
      std::string mime_format;
      bool track_impr; /**< flag, if true - track impressions */
      std::string template_file;
      AdServer::CampaignSvcs::CreativeTemplateType type;
    };

    typedef std::list<CreativeTemplateFileDef> CreativeTemplateFileList;

    class CreativeTemplateDef: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      bool operator==(const CreativeTemplateDef& right) const noexcept;

      TimestampValue timestamp;
      CreativeTemplateFileList files;
      OptionValueMap tokens;
      OptionValueMap hidden_tokens;

    private:
      virtual
      ~CreativeTemplateDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<CreativeTemplateDef> CreativeTemplateDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, CreativeTemplateDef_var>
      CreativeTemplateMap;

    class CampaignKeyword: public ReferenceCounting::AtomicImpl
    {
    public:
      CampaignKeyword() noexcept
        : ccg_keyword_id(0),
          timestamp(0)
      {}

      CampaignKeyword(
        unsigned long ccg_keyword_id_val,
        const char* original_keyword_val,
        const char* click_url_val,
        const TimestampValue& timestamp_val = Generics::Time::ZERO)
        : ccg_keyword_id(ccg_keyword_id_val),
          original_keyword(original_keyword_val),
          click_url(click_url_val),
          timestamp(timestamp_val)
      {}

      bool operator==(const CampaignKeyword& right) const noexcept;

      unsigned long ccg_keyword_id;
      std::string original_keyword;
      std::string click_url;
      TimestampValue timestamp;

    protected:
      virtual
      ~CampaignKeyword() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<CampaignKeyword> CampaignKeyword_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, CampaignKeyword_var>
      CampaignKeywordMap;

    class CategoryChannelDef: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      typedef std::map<std::string, std::string> LocalizationMap;

      CategoryChannelDef(): channel_id(0), parent_channel_id(0) {}

      CategoryChannelDef(
        unsigned long channel_id_val,
        const char* name_val,
        const char* newsgate_name_val,
        unsigned long parent_channel_id_val,
        unsigned long flags_val,
        const LocalizationMap& localizations_val,
        const TimestampValue& timestamp_val)
        noexcept;

      bool operator==(const CategoryChannelDef& right) const noexcept;

      unsigned long channel_id;
      std::string name;
      std::string newsgate_name;
      unsigned long parent_channel_id;
      unsigned long flags;
      LocalizationMap localizations;
      TimestampValue timestamp;

    private:
      virtual
      ~CategoryChannelDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<CategoryChannelDef>
      CategoryChannelDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long,
      CategoryChannelDef_var>
      CategoryChannelMap;

    struct NonLinkedExpressionChannelTimestampOps
    {
      TimestampValue timestamp(
        const NonLinkedExpressionChannel* el) const
      {
        return el->params().timestamp;
      }

      void set_timestamp(
        NonLinkedExpressionChannel* el,
        const TimestampValue& ts) const
      {
        el->params().timestamp = ts;
      }
    };

    struct ChannelMap:
      public AdServer::Commons::NoCopyGranularContainer<
        unsigned long,
        NonLinkedExpressionChannel_var,
        NonLinkedExpressionChannelTimestampOps>
    {};

    struct GlobalParamsDef
    {
      GlobalParamsDef()
        : currency_exchange_id(0),
          google_publisher_account_id(0),
          timestamp(0)
      {}

      GlobalParamsDef(
        unsigned long currency_exchange_id_val,
        const Generics::Time& fraud_user_deactivate_period_val,
        const RevenueDecimal& cost_limit_val,
        unsigned long google_publisher_account_id_val,
        const TimestampValue& timestamp_val)
        : currency_exchange_id(currency_exchange_id_val),
          fraud_user_deactivate_period(fraud_user_deactivate_period_val),
          cost_limit(cost_limit_val),
          google_publisher_account_id(google_publisher_account_id_val),
          timestamp(timestamp_val)
      {}

      unsigned long currency_exchange_id;
      Generics::Time fraud_user_deactivate_period;
      RevenueDecimal cost_limit;
      RevenueDecimal max_random_cpm;
      unsigned long google_publisher_account_id;
      TimestampValue timestamp;
    };

    class CreativeCategoryDef: public ReferenceCounting::AtomicImpl
    {
    protected:
      virtual
      ~CreativeCategoryDef() noexcept
      {}

    public:
      struct ExternalCategoryNameSet: public StringSet
      {
        bool
        operator==(const ExternalCategoryNameSet& right) const
          noexcept;
      };

      typedef std::map<AdRequestType, ExternalCategoryNameSet>
        ExternalCategoryMap;

      bool operator==(const CreativeCategoryDef& right) const noexcept;

      unsigned long cct_id;
      std::string name;
      ExternalCategoryMap external_categories;
      TimestampValue timestamp;
    };

    typedef ReferenceCounting::SmartPtr<CreativeCategoryDef>
      CreativeCategoryDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, CreativeCategoryDef_var>
      CreativeCategoryMap;

    class AdvActionDef: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      AdvActionDef() {}

      explicit
      AdvActionDef(
        unsigned long action_id_val,
        const TimestampValue& timestamp_val = Generics::Time::ZERO)
        : action_id(action_id_val),
          timestamp(timestamp_val)
      {}

      bool operator==(const AdvActionDef& right) const noexcept
      {
        return action_id == right.action_id &&
          cur_value == right.cur_value &&
          ccg_ids.size() == right.ccg_ids.size() &&
          std::equal(ccg_ids.begin(), ccg_ids.end(), right.ccg_ids.begin());
      }

      typedef std::set<unsigned long> CCGIdSet;
      unsigned long action_id;
      CCGIdSet ccg_ids;
      RevenueDecimal cur_value;
      TimestampValue timestamp;

    protected:
      virtual ~AdvActionDef() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<AdvActionDef> AdvActionDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, AdvActionDef_var>
      AdvActionMap;

    class FraudConditionDef: public ReferenceCounting::AtomicImpl
    {
    public:
      bool operator==(const FraudConditionDef& right) const noexcept;

      unsigned long id;
      char type; // I - impression, C - click
      unsigned long limit;
      Generics::Time period;
      Generics::Time timestamp;

    protected:
      virtual
      ~FraudConditionDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<FraudConditionDef> FraudConditionDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, FraudConditionDef_var>
      FraudConditionMap;

    class MarginRuleDef: public ReferenceCounting::AtomicImpl
    {
    public:
      typedef std::set<unsigned long> AccountIdSet;

      MarginRuleDef() noexcept;
      MarginRuleDef(const MarginRuleDef& init,
        bool with_conditions = true)
        noexcept;

      bool operator==(const MarginRuleDef& right) const noexcept;

      unsigned long margin_rule_id;
      unsigned long account_id;
      MarginRuleType type;
      unsigned long sort_order;
      RevenueDecimal fixed_margin;
      RevenueDecimal relative_margin;

      // conditions
      AccountIdSet isp_accounts;
      AccountIdSet publisher_accounts;
      AccountIdSet advertiser_accounts;

      char user_status; // Y,N,-

      char walled_garden; // Y,N,-

      bool display_campaigns;
      bool text_campaigns;

      bool cpm_campaigns;
      bool cpc_campaigns;
      bool cpa_campaigns;

      PriceRangeSet tag_price;
      PriceRangeSet campaign_ecpm;

      StringSet tag_size;

      Generics::Time timestamp;

    protected:
      virtual
      ~MarginRuleDef() noexcept
      {
      }

    private:
      void init_default_conditions_() noexcept;

      static bool account_set_eq_(
        const AccountIdSet& left, const AccountIdSet& right) noexcept;

      static bool pricerange_set_eq_(
        const PriceRangeSet& left, const PriceRangeSet& right) noexcept;
    };

    typedef ReferenceCounting::SmartPtr<MarginRuleDef> MarginRuleDef_var;

    typedef
      AdServer::Commons::NoCopyGranularContainer<
        unsigned long,
        MarginRuleDef_var>
      MarginRuleMap;

    struct BehavioralParameterDef
    {
      bool operator==(const BehavioralParameterDef& in) const noexcept;
      unsigned int min_visits;
      unsigned int time_from;
      unsigned int time_to;
      unsigned int weight;
      char trigger_type;
    };

    class BehavioralParameterListDef:
      public ReferenceCounting::AtomicCopyImpl
    {
    public:
      typedef std::list<BehavioralParameterDef> BehavioralParameterList;

      BehavioralParameterListDef() noexcept;

      bool operator==(const BehavioralParameterListDef& in) const noexcept;

      unsigned long threshold;
      std::list<BehavioralParameterDef> behave_params;
      Generics::Time timestamp;

    protected:
      virtual
      ~BehavioralParameterListDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<BehavioralParameterListDef>
      BehavioralParameterListDef_var;

    typedef
      AdServer::Commons::NoCopyGranularContainer<
        unsigned long,
        BehavioralParameterListDef_var>
      BehavioralParameterMap;

    typedef
      AdServer::Commons::NoCopyGranularContainer<
        std::string,
        BehavioralParameterListDef_var>
      BehavioralParameterKeyMap;

    class SimpleChannelDef: public ReferenceCounting::AtomicCopyImpl
    {
    public:
      typedef std::set<unsigned long> CategoryIdSet;

      class MatchParams: public ReferenceCounting::AtomicImpl
      {
      public:
        typedef std::vector<unsigned long> ChannelTriggerIdArray;

        ChannelTriggerIdArray page_triggers;
        ChannelTriggerIdArray search_triggers;
        ChannelTriggerIdArray url_triggers;
        ChannelTriggerIdArray url_keyword_triggers;

      protected:
        virtual
        ~MatchParams() noexcept
        {
        }
      };

      typedef ReferenceCounting::SmartPtr<MatchParams> MatchParams_var;

      bool operator==(const SimpleChannelDef& right) const noexcept;

      unsigned long channel_id;
      std::string country;
      unsigned long threshold;
      char status;
      CategoryIdSet categories;
      unsigned long behav_param_list_id;
      std::string str_behav_param_list_id;
      MatchParams_var match_params;

      Generics::Time timestamp;

    private:
      virtual
      ~SimpleChannelDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<SimpleChannelDef> SimpleChannelDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, SimpleChannelDef_var>
      SimpleChannelMap;

    class GeoChannelDef: public ReferenceCounting::AtomicImpl
    {
    public:
      struct GeoIPTarget
      {
        std::string region;
        std::string city;

        bool operator==(const GeoIPTarget& right) const noexcept;
      };

      typedef std::vector<GeoIPTarget> GeoIPTargetList;

      bool operator==(const GeoChannelDef& right) const noexcept;

      std::string country;
      GeoIPTargetList geoip_targets;

      Generics::Time timestamp;

    private:
      virtual
      ~GeoChannelDef() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<GeoChannelDef> GeoChannelDef_var;

    class GeoCoordChannelDef: public ReferenceCounting::AtomicImpl
    {
    public:
      bool operator==(const GeoCoordChannelDef& right) const noexcept;

      CoordDecimal longitude;
      CoordDecimal latitude;
      AccuracyDecimal radius;

      Generics::Time timestamp;

    private:
      virtual
      ~GeoCoordChannelDef() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<GeoCoordChannelDef>
      GeoCoordChannelDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, GeoCoordChannelDef_var>
      GeoCoordChannelMap;

    class GeoChannelMap:
      public ReferenceCounting::AtomicCopyImpl,
      public AdServer::Commons::NoCopyGranularContainer<
        unsigned long, GeoChannelDef_var>
    {
    protected:
      virtual
      ~GeoChannelMap() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<GeoChannelMap>
      GeoChannelMap_var;

    class BlockChannelDef: public ReferenceCounting::AtomicImpl
    {
    public:
      bool
      operator==(const BlockChannelDef& right) const noexcept;

      unsigned long size_id;
      Generics::Time timestamp;

    protected:
      virtual
      ~BlockChannelDef() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<BlockChannelDef>
      BlockChannelDef_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, BlockChannelDef_var>
      BlockChannelMap;

    struct SearchEngineRegExp
    {
      bool operator==(const SearchEngineRegExp& cp) const noexcept;

      std::string host_postfix;
      std::string regexp;
      std::string encoding;
      std::string post_encoding;
      unsigned long decoding_depth;
    };

    class SearchEngine: public ReferenceCounting::AtomicImpl
    {
    public:
      typedef std::list<SearchEngineRegExp> SearchEngineRegExpList;

      bool operator==(const SearchEngine& cp) const noexcept;

      SearchEngineRegExpList regexps;
      Generics::Time timestamp;

    protected:
      virtual
      ~SearchEngine() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<SearchEngine> SearchEngine_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, SearchEngine_var>
      SearchEngineMap;

    class WebBrowser: public ReferenceCounting::AtomicImpl
    {
    public:
      struct Detector
      {
        bool operator==(const Detector& cp) const noexcept;

        unsigned long priority;
        std::string marker;
        std::string regexp;
        bool regexp_required;
      };

      typedef std::list<Detector> DetectorList;

      bool operator==(const WebBrowser& cp) const noexcept;

      DetectorList detectors;
      Generics::Time timestamp;

    protected:
      virtual
      ~WebBrowser() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<WebBrowser> WebBrowser_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      std::string, WebBrowser_var>
      WebBrowserMap;

    class Platform: public ReferenceCounting::AtomicImpl
    {
    public:
      struct Detector
      {
        bool operator==(const Detector& cp) const noexcept;

        unsigned long priority;
        std::string use_name;
        std::string marker;
        std::string match_regexp;
        std::string output_regexp;
      };

      typedef std::list<Detector> DetectorList;

      bool operator==(const Platform& cp) const noexcept;

      std::string name;
      std::string type;
      DetectorList detectors;
      Generics::Time timestamp;

    protected:
      virtual
      ~Platform() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<Platform> Platform_var;

    typedef AdServer::Commons::NoCopyGranularContainer<
      unsigned long, Platform_var>
      PlatformMap;

    /** Stores configuration of the ad campaign service. */
    class CampaignConfig:
      public ReferenceCounting::AtomicCopyImpl
    {
    public:
      /** Default constructor. */
      CampaignConfig() /*throw(eh::Exception)*/;

    public:
      unsigned long server_id;
      TimestampValue master_stamp;
      TimestampValue first_load_stamp;
      TimestampValue finish_load_stamp;
      TimestampValue db_stamp;
      GlobalParamsDef global_params;

      /* internal versioning */
      AppFormatMap app_formats;
      SizeMap sizes;
      CountryMap countries;
      AccountMap accounts;
      CurrencyMap currencies;
      ColocationMap colocations;
      FreqCapMap freq_caps;
      CreativeOptionMap creative_options;
      CampaignMap campaigns;
      SiteMap sites;
      TagMap tags;
      ChannelMap expression_channels;
      SimpleChannelMap simple_channels;
      GeoChannelMap_var geo_channels;
      GeoCoordChannelMap geo_coord_channels;
      BlockChannelMap block_channels;

      CategoryChannelMap category_channels;
      CreativeTemplateMap creative_templates;
      CampaignKeywordMap campaign_keywords;
      ContractMap contracts;

      EcpmMap ecpms;

      /* db version versioning */
      AdvActionMap adv_actions;

      CreativeCategoryMap creative_categories;

      MarginRuleMap margin_rules;

      BehavioralParameterMap behav_param_lists;
      BehavioralParameterKeyMap str_behav_param_lists;

      Generics::Time detectors_timestamp;
      SearchEngineMap search_engines;
      WebBrowserMap web_browsers;
      PlatformMap platforms;

      FraudConditionMap fraud_conditions;
      WebOperationMap web_operations;

    protected:
      virtual
      ~CampaignConfig() noexcept;
    };

    typedef ReferenceCounting::SmartPtr<CampaignConfig>
      CampaignConfig_var;

    class CampaignConfigSource : public virtual ReferenceCounting::Interface
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      virtual CampaignConfig_var update(bool* need_logging = 0) /*throw(Exception)*/ = 0;
    protected:
      virtual
      ~CampaignConfigSource() noexcept
      {
      }
    };

    typedef ReferenceCounting::SmartPtr<CampaignConfigSource> CampaignConfigSource_var;
  }
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    inline
    bool CurrencyDef::operator==(const CurrencyDef& right) const noexcept
    {
      return rate == right.rate &&
        currency_id == right.currency_id &&
        currency_exchange_id == right.currency_exchange_id &&
        effective_date == right.effective_date &&
        fraction_digits == right.fraction_digits &&
        currency_code == right.currency_code;
    }

    inline
    bool SiteDef::operator==(const SiteDef& right) const noexcept
    {
      return status == right.status &&
        freq_cap_id == right.freq_cap_id &&
        noads_timeout == right.noads_timeout &&
        flags == right.flags &&
        account_id == right.account_id &&
        approved_creative_categories.size() ==
          right.approved_creative_categories.size() &&
        rejected_creative_categories.size() ==
          right.rejected_creative_categories.size() &&
        approved_creatives.size() ==
          right.approved_creatives.size() &&
        rejected_creatives.size() ==
          right.rejected_creatives.size() &&
        std::equal(approved_creative_categories.begin(),
          approved_creative_categories.end(),
          right.approved_creative_categories.begin()) &&
        std::equal(rejected_creative_categories.begin(),
          rejected_creative_categories.end(),
          right.rejected_creative_categories.begin()) &&
        std::equal(approved_creatives.begin(),
          approved_creatives.end(),
          right.approved_creatives.begin()) &&
        std::equal(rejected_creatives.begin(),
          rejected_creatives.end(),
          right.rejected_creatives.begin());
    }

    inline
    bool AccountDef::operator==(const AccountDef& right) const noexcept
    {
      return account_id == right.account_id &&
        agency_account_id == right.agency_account_id &&
        internal_account_id == right.internal_account_id &&
        role_id == right.role_id &&
        legal_name == right.legal_name &&
        flags == right.flags &&
        at_flags == right.at_flags &&
        text_adserving == right.text_adserving &&
        currency_id == right.currency_id &&
        country == right.country &&
        time_offset == right.time_offset &&
        commision == right.commision &&
        media_handling_fee == right.media_handling_fee &&
        budget == right.budget &&
        paid_amount == right.paid_amount &&
        auction_rate == right.auction_rate &&
        use_pub_pixels == right.use_pub_pixels &&
        pub_pixel_optin == right.pub_pixel_optin &&
        pub_pixel_optout == right.pub_pixel_optout &&
        self_service_commission == right.self_service_commission &&
        status == right.status &&
        eval_status == right.eval_status &&
        walled_garden_accounts.size() == right.walled_garden_accounts.size() &&
        std::equal(walled_garden_accounts.begin(),
          walled_garden_accounts.end(),
          right.walled_garden_accounts.begin());
    }

    inline
    bool CampaignKeyword::operator==(const CampaignKeyword& right) const noexcept
    {
      return ccg_keyword_id == right.ccg_keyword_id &&
        original_keyword == right.original_keyword &&
        click_url == right.click_url;
    }

    inline
    bool
    TagPricingDef::operator==(const TagPricingDef& right) const noexcept
    {
      return country_code == right.country_code &&
        ccg_type == right.ccg_type &&
        ccg_rate_type == right.ccg_rate_type &&
        site_rate_id == right.site_rate_id &&
        imp_revenue == right.imp_revenue &&
        revenue_share == right.revenue_share;
    }

    inline
    bool
    TagDef::operator==(const TagDef& right) const noexcept
    {
      return tag_id == right.tag_id &&
        site_id == right.site_id &&
        sizes.size() == right.sizes.size() &&
        imp_track_pixel == right.imp_track_pixel &&
        passback == right.passback &&
        passback_type == right.passback_type &&
        allow_expandable == right.allow_expandable &&
        flags == right.flags &&
        marketplace == right.marketplace &&
        adjustment == right.adjustment &&
        auction_max_ecpm_share == right.auction_max_ecpm_share &&
        auction_prop_probability_share == right.auction_prop_probability_share &&
        auction_random_share == right.auction_random_share &&
        cost_coef == right.cost_coef &&
        accepted_categories.size() == right.accepted_categories.size() &&
        rejected_categories.size() == right.rejected_categories.size() &&
        tag_pricings.size() == right.tag_pricings.size() &&
        template_tokens.size() == right.template_tokens.size() &&
        std::equal(sizes.begin(),
          sizes.end(),
          right.sizes.begin(),
          Algs::PairEqual()) &&
        std::equal(accepted_categories.begin(),
          accepted_categories.end(),
          right.accepted_categories.begin()) &&
        std::equal(rejected_categories.begin(),
          rejected_categories.end(),
          right.rejected_categories.begin()) &&
        std::equal(tag_pricings.begin(), tag_pricings.end(), right.tag_pricings.begin()) &&
        tokens == right.tokens &&
        hidden_tokens == right.hidden_tokens &&
        std::equal(template_tokens.begin(),
          template_tokens.end(),
          right.template_tokens.begin());
    }

    inline
    bool
    AppFormatDef::operator==(
      const AppFormatDef& right) const noexcept
    {
      return mime_format == right.mime_format;
    }

    inline
    CreativeOptionDef::CreativeOptionDef(
      const char* token_val,
      char type_val,
      const StringSet& token_relations_val,
      const Generics::Time& timestamp_val) noexcept
      : token(token_val),
        type(type_val),
        token_relations(token_relations_val),
        timestamp(timestamp_val)
    {}

    inline
    bool CreativeOptionDef::operator==(
      const CreativeOptionDef& right) const noexcept
    {
      return token == right.token &&
        type == right.type &&
        token_relations.size() == right.token_relations.size() &&
        std::equal(token_relations.begin(), token_relations.end(),
          right.token_relations.begin());
    }

    inline
    CreativeDef::SystemOptions::SystemOptions() noexcept
      : click_url_option_id(0),
        html_url_option_id(0)
    {}

    inline
    CreativeDef::SystemOptions::SystemOptions(
      long click_url_option_id_val,
      const char* click_url_val,
      long html_url_option_id_val,
      const char* html_url_val)
      /*throw(eh::Exception)*/
      : click_url_option_id(click_url_option_id_val),
        click_url(click_url_val),
        html_url_option_id(html_url_option_id_val),
        html_url(html_url_val)
    {}

    inline
    CreativeDef::CreativeDef(
      unsigned long ccid_val,
      unsigned long creative_id_val,
      unsigned long fc_id_val,
      unsigned long weight_val,
      const char* format_val,
      const char* version_id_val,
      char status_val,
      const SystemOptions& sys_options_val,
      const OptionValueSeq& tokens_val,
      const CreativeCategoryIdSeq& categories_val)
      /*throw(eh::Exception)*/
      : ccid(ccid_val),
        creative_id(creative_id_val),
        fc_id(fc_id_val),
        weight(weight_val),
        format(format_val),
        version_id(version_id_val),
        status(status_val),
        sys_options(sys_options_val),
        initial_contract_id(0)
    {
      for(CORBA::ULong token_i = 0; token_i < tokens_val.length(); ++token_i)
      {
        tokens.insert(std::make_pair(
          tokens_val[token_i].option_id,
          tokens_val[token_i].value.in()));
      }

      CorbaAlgs::convert_sequence(categories_val, categories);
    }


    //
    // FreqCapDef class
    //

    inline
    FreqCapDef::FreqCapDef()
      noexcept
      : fc_id(0),
        lifelimit(0),
        period(0),
        window_limit(0),
        window_time(0)
    {}

    inline
    FreqCapDef::FreqCapDef(
      unsigned long fc_id_val,
      unsigned long lifelimit_val,
      unsigned long period_val,
      unsigned long window_limit_val,
      unsigned long window_time_val,
      const TimestampValue& ts_val)
      noexcept
      : fc_id(fc_id_val),
        lifelimit(lifelimit_val),
        period(period_val),
        window_limit(window_limit_val),
        window_time(window_time_val),
        timestamp(ts_val)
    {}

    inline
    bool ColocationDef::operator==(const ColocationDef& right) const noexcept
    {
      return colo_id == right.colo_id &&
        colo_name == right.colo_name &&
        colo_rate_id == right.colo_rate_id &&
        at_flags == right.at_flags &&
        account_id == right.account_id &&
        revenue_share == right.revenue_share &&
        ad_serving == right.ad_serving &&
        hid_profile == right.hid_profile &&
        tokens == right.tokens;
    }

    inline
    bool
    FreqCapDef::operator==(const FreqCapDef& cp) const noexcept
    {
      return fc_id == cp.fc_id &&
        lifelimit == cp.lifelimit &&
        period == cp.period &&
        window_limit == cp.window_limit &&
        window_time == cp.window_time;
    }

    //
    // CreativeTemplateDef struct
    //
    inline
    CreativeTemplateFileDef::CreativeTemplateFileDef(
      const char* creative_format_val,
      const char* creative_size_val,
      const char* app_format_val,
      const char* mime_format_val,
      bool track_impr_val,
      AdServer::CampaignSvcs::CreativeTemplateType type_val,
      const char* template_file_val)
      noexcept
      : creative_format(creative_format_val ? creative_format_val : ""),
        creative_size(creative_size_val ? creative_size_val : ""),
        app_format(app_format_val ? app_format_val : ""),
        mime_format(mime_format_val),
        track_impr(track_impr_val),
        template_file(template_file_val ? template_file_val : ""),
        type(type_val)
    {}

    inline
    bool
    CreativeTemplateFileDef::operator==(
      const CreativeTemplateFileDef& right) const noexcept
    {
      return creative_format == right.creative_format &&
        creative_size == right.creative_size &&
        app_format == right.app_format &&
        mime_format == right.mime_format &&
        track_impr == right.track_impr &&
        template_file == right.template_file &&
        type == right.type;
    }


    inline bool
    CreativeTemplateDef::operator==(
      const CreativeTemplateDef& right) const noexcept
    {
      return files.size() == right.files.size() &&
        tokens.size() == right.tokens.size() &&
        std::equal(files.begin(), files.end(), right.files.begin()) &&
        std::equal(tokens.begin(), tokens.end(), right.tokens.begin()) &&
        hidden_tokens == right.hidden_tokens;
    }

    inline
    MarginRuleDef::MarginRuleDef() noexcept
      : margin_rule_id(0),
        account_id(0),
        type(MR_STOP),
        sort_order(0),
        fixed_margin(0),
        relative_margin(0)
    {
      init_default_conditions_();
    }

    inline
    MarginRuleDef::MarginRuleDef(
      const MarginRuleDef& init,
      bool with_conditions) noexcept
      : ReferenceCounting::Interface(init),
        ReferenceCounting::AtomicImpl(),
        margin_rule_id(init.margin_rule_id),
        account_id(init.account_id),
        type(init.type),
        sort_order(init.sort_order),
        fixed_margin(init.fixed_margin),
        relative_margin(init.relative_margin),
        timestamp(init.timestamp)
    {
      if(with_conditions)
      {
        isp_accounts = init.isp_accounts;
        publisher_accounts = init.publisher_accounts;
        advertiser_accounts = init.advertiser_accounts;
        user_status = init.user_status;
        walled_garden = init.walled_garden;
        display_campaigns = init.display_campaigns;
        text_campaigns = init.text_campaigns;
        cpm_campaigns = init.cpm_campaigns;
        cpc_campaigns = init.cpc_campaigns;
        cpa_campaigns = init.cpa_campaigns;
        tag_price = init.tag_price;
        campaign_ecpm = init.campaign_ecpm;
        tag_size = init.tag_size;
      }
      else
      {
        init_default_conditions_();
      }
    }

    inline
    void MarginRuleDef::init_default_conditions_() noexcept
    {
      user_status = '-';
      walled_garden = '-';
      display_campaigns = true;
      text_campaigns = true;
      cpm_campaigns = true;
      cpc_campaigns = true;
      cpa_campaigns = true;
    }

    inline
    bool MarginRuleDef::account_set_eq_(
      const AccountIdSet& left, const AccountIdSet& right) noexcept
    {
      return left.size() == right.size() &&
        std::equal(left.begin(), left.end(), right.begin());
    }

    inline
    bool MarginRuleDef::pricerange_set_eq_(
      const PriceRangeSet& left, const PriceRangeSet& right) noexcept
    {
      return left.size() == right.size() &&
        std::equal(left.begin(), left.end(), right.begin());
    }

    inline
    bool MarginRuleDef::operator==(const MarginRuleDef& right) const noexcept
    {
      return margin_rule_id == right.margin_rule_id &&
        account_id == right.account_id &&
        type == right.type &&
        sort_order == right.sort_order &&
        fixed_margin == right.fixed_margin &&
        relative_margin == right.relative_margin &&
        account_set_eq_(isp_accounts, right.isp_accounts) &&
        account_set_eq_(publisher_accounts, right.publisher_accounts) &&
        account_set_eq_(advertiser_accounts, right.advertiser_accounts) &&
        user_status == right.user_status &&
        walled_garden == right.walled_garden &&
        display_campaigns == right.display_campaigns &&
        text_campaigns == right.text_campaigns &&
        cpm_campaigns == right.cpm_campaigns &&
        cpc_campaigns == right.cpc_campaigns &&
        cpa_campaigns == right.cpa_campaigns &&
        pricerange_set_eq_(tag_price, right.tag_price) &&
        pricerange_set_eq_(campaign_ecpm, right.campaign_ecpm) &&
        tag_size.size() == right.tag_size.size() &&
          std::equal(tag_size.begin(), tag_size.end(), right.tag_size.begin());
    }

    inline
    bool BehavioralParameterDef::operator==(
      const BehavioralParameterDef& in) const noexcept
    {
      return min_visits == in.min_visits &&
        time_from == in.time_from &&
        time_to == in.time_to &&
        weight == in.weight &&
        trigger_type == in.trigger_type;
    }

    inline
    bool BehavioralParameterListDef::operator==(
      const BehavioralParameterListDef& in) const noexcept
    {
      return threshold == in.threshold &&
        behave_params.size() == in.behave_params.size() &&
        std::equal(behave_params.begin(),
          behave_params.end(), in.behave_params.begin());
    }

    inline
    BehavioralParameterListDef::BehavioralParameterListDef() noexcept
      : threshold(0)
    {}

    inline
    CategoryChannelDef::CategoryChannelDef(
      unsigned long channel_id_val,
      const char* name_val,
      const char* newsgate_name_val,
      unsigned long parent_channel_id_val,
      unsigned long flags_val,
      const LocalizationMap& localizations_val,
      const TimestampValue& timestamp_val)
      noexcept
      : channel_id(channel_id_val),
        name(name_val),
        newsgate_name(newsgate_name_val),
        parent_channel_id(parent_channel_id_val),
        flags(flags_val),
        localizations(localizations_val),
        timestamp(timestamp_val)
    {}

    inline
    bool CategoryChannelDef::operator==(
      const CategoryChannelDef& right) const noexcept
    {
      return channel_id == right.channel_id &&
        name == right.name &&
        newsgate_name == right.newsgate_name &&
        parent_channel_id == right.parent_channel_id &&
        flags == right.flags &&
        localizations.size() == right.localizations.size() &&
        std::equal(localizations.begin(),
          localizations.end(), right.localizations.begin(), Algs::PairEqual());
    }

    //
    // CampaignConfig class
    //

    inline
    CampaignConfig::CampaignConfig()
      /*throw(eh::Exception)*/
      : server_id(0),
        geo_channels(new GeoChannelMap())
    {}

    inline
    bool
    CreativeDef::SystemOptions::operator==(
      const CreativeDef::SystemOptions& right) const noexcept
    {
      return click_url_option_id == right.click_url_option_id &&
        click_url == right.click_url &&
        html_url_option_id == right.html_url_option_id &&
        html_url == right.html_url;
    }

    inline
    bool CreativeDef::operator==(const CreativeDef& right) const noexcept
    {
      return ccid == right.ccid &&
        creative_id == right.creative_id &&
        fc_id == right.fc_id &&
        weight == right.weight &&
        format == right.format &&
        sizes.size() == right.sizes.size() &&
        status == right.status &&
        order_set_id == right.order_set_id &&
        version_id == right.version_id &&
        initial_contract_id == right.initial_contract_id &&
        sys_options == right.sys_options &&
        categories.size() == right.categories.size() &&
        tokens.size() == right.tokens.size() &&
        std::equal(sizes.begin(), sizes.end(), right.sizes.begin(), Algs::PairEqual()) &&
        std::equal(categories.begin(), categories.end(), right.categories.begin()) &&
        std::equal(tokens.begin(), tokens.end(), right.tokens.begin(), Algs::PairEqual());
    }

    inline
    bool ContractDef::operator==(const ContractDef& right) const noexcept
    {
      return contract_id == right.contract_id &&
        number == right.number &&
        date == right.date &&
        type == right.type &&
        vat_included == right.vat_included &&
        ord_contract_id == right.ord_contract_id &&
        ord_ado_id == right.ord_ado_id &&
        subject_type == right.subject_type &&
        action_type == right.action_type &&
        agent_acting_for_publisher == right.agent_acting_for_publisher &&
        parent_contract_id == right.parent_contract_id &&
        client_id == right.client_id &&
        client_name == right.client_name &&
        client_legal_form == right.client_legal_form &&
        contractor_id == right.contractor_id &&
        contractor_name == right.contractor_name &&
        contractor_legal_form == right.contractor_legal_form;
    }

    inline
    bool CampaignDef::operator==(const CampaignDef& right) const noexcept
    {
      return campaign_group_id == right.campaign_group_id &&
        ccg_rate_id == right.ccg_rate_id &&
        ccg_rate_type == right.ccg_rate_type &&
        fc_id == right.fc_id &&
        group_fc_id == right.group_fc_id &&
        expression == right.expression &&
        stat_expression == right.stat_expression &&
        country == right.country &&
        status == right.status &&
        imp_revenue == right.imp_revenue &&
        click_revenue == right.click_revenue &&
        action_revenue == right.action_revenue &&
        commision == right.commision &&
        min_uid_age == right.min_uid_age &&
        marketplace == right.marketplace &&
        flags == right.flags &&
        account_id == right.account_id &&
        advertiser_id == right.advertiser_id &&
        ccg_type == right.ccg_type &&
        target_type == right.target_type &&
        campaign_delivery_limits == right.campaign_delivery_limits &&
        ccg_delivery_limits == right.ccg_delivery_limits &&
        start_user_group_id == right.start_user_group_id &&
        end_user_group_id == right.end_user_group_id &&
        max_pub_share == right.max_pub_share &&
        ctr_reset_id == right.ctr_reset_id && 
        mode == right.mode &&
        seq_set_rotate_imps == right.seq_set_rotate_imps &&
        delivery_coef == right.delivery_coef &&
        bid_strategy == right.bid_strategy &&
        min_ctr_goal == right.min_ctr_goal &&
        initial_contract_id == right.initial_contract_id &&
        sites.size() == right.sites.size() &&
        creatives.size() == right.creatives.size() &&
        colocations.size() == right.colocations.size() &&
        exclude_pub_accounts.size() == right.exclude_pub_accounts.size() &&
        exclude_tags.size() == right.exclude_tags.size() &&
        std::equal(sites.begin(), sites.end(), right.sites.begin()) &&
        std::equal(creatives.begin(), creatives.end(), right.creatives.begin()) &&
        std::equal(colocations.begin(),
          colocations.end(),
          right.colocations.begin()) &&
        std::equal(exclude_pub_accounts.begin(),
          exclude_pub_accounts.end(),
          right.exclude_pub_accounts.begin()) &&
        weekly_run_intervals == right.weekly_run_intervals &&
        std::equal(exclude_tags.begin(),
          exclude_tags.end(), right.exclude_tags.begin(), Algs::PairEqual());
    }

    inline
    bool FraudConditionDef::operator==(
      const FraudConditionDef& right) const noexcept
    {
      return id == right.id &&
        type == right.type &&
        limit == right.limit &&
        period == right.period;
    }

    inline
    bool SimpleChannelDef::operator==(const SimpleChannelDef& right) const
      noexcept
    {
      return channel_id == right.channel_id &&
        country == right.country &&
        threshold == right.threshold &&
        status == right.status &&
        behav_param_list_id == right.behav_param_list_id &&
        str_behav_param_list_id == right.str_behav_param_list_id &&
        categories.size() == right.categories.size() &&
        std::equal(categories.begin(), categories.end(), right.categories.begin());
    }

    inline
    bool GeoChannelDef::GeoIPTarget::operator==(
      const GeoIPTarget& right) const
      noexcept
    {
      return region == right.region && city == right.city;
    }

    inline
    bool GeoChannelDef::operator==(const GeoChannelDef& right) const
      noexcept
    {
      return country == right.country &&
        geoip_targets.size() == right.geoip_targets.size() &&
        std::equal(geoip_targets.begin(),
          geoip_targets.end(), right.geoip_targets.begin());
    }

    inline bool
    GeoCoordChannelDef::operator==(const GeoCoordChannelDef& right) const
      noexcept
    {
      return longitude == right.longitude &&
        latitude == right.latitude &&
        radius == right.radius;
    }

    inline bool
    BlockChannelDef::operator==(const BlockChannelDef& right) const noexcept
    {
      return size_id == right.size_id;
    }

    inline
    bool SearchEngine::operator==(const SearchEngine& cp) const noexcept
    {
      return regexps.size() == cp.regexps.size() &&
        std::equal(regexps.begin(), regexps.end(), cp.regexps.begin());
    }

    inline
    bool SearchEngineRegExp::operator==(const SearchEngineRegExp& cp) const
      noexcept
    {
      return host_postfix == cp.host_postfix &&
        regexp == cp.regexp &&
        encoding == cp.encoding &&
        post_encoding == cp.post_encoding &&
        decoding_depth == cp.decoding_depth;
    }

    inline
    bool WebBrowser::operator==(const WebBrowser& right) const noexcept
    {
      return detectors.size() == right.detectors.size() &&
        std::equal(detectors.begin(), detectors.end(), right.detectors.begin());
    }

    inline
    bool WebBrowser::Detector::operator==(const WebBrowser::Detector& right) const noexcept
    {
      return priority == right.priority &&
        marker == right.marker &&
        regexp == right.regexp &&
        regexp_required == right.regexp_required;
    }

    inline
    bool Platform::operator==(const Platform& right) const noexcept
    {
      return name == right.name &&
        type == right.type &&
        detectors.size() == right.detectors.size() &&
        std::equal(detectors.begin(), detectors.end(), right.detectors.begin());
    }

    inline
    bool Platform::Detector::operator==(const Platform::Detector& right) const noexcept
    {
      return priority == right.priority &&
        use_name == right.use_name &&
        marker == right.marker &&
        match_regexp == right.match_regexp &&
        output_regexp == right.output_regexp;
    }

    inline
    bool
    CreativeCategoryDef::ExternalCategoryNameSet::operator==(
      const ExternalCategoryNameSet& right) const
      noexcept
    {
      return std::equal(begin(), end(), right.begin());
    }

    inline
    bool
    CreativeCategoryDef::operator==(
      const CreativeCategoryDef& right) const noexcept
    {
      return cct_id == right.cct_id &&
        name == right.name &&
        external_categories.size() == right.external_categories.size() &&
        std::equal(external_categories.begin(),
          external_categories.end(),
          right.external_categories.begin());
    }

    inline
    bool
    StringDictionaryDef::operator==(const StringDictionaryDef& right) const noexcept
    {
      return this->size() == right.size() &&
        std::equal(begin(), end(), right.begin());
    }

    inline
    bool
    SizeDef::operator==(const SizeDef& right) const noexcept
    {
      return protocol_name == right.protocol_name &&
        size_type_id == right.size_type_id &&
        width == right.width &&
        height == right.height &&
        max_width == right.max_width &&
        max_height == right.max_height;
    }

    inline
    bool
    CountryDef::operator==(const CountryDef& right) const noexcept
    {
      return tokens == right.tokens;
    }

    inline
    bool
    WebOperationDef::operator==(const WebOperationDef& right) const noexcept
    {
      return app == right.app &&
        source == right.source &&
        operation == right.operation &&
        flags == right.flags;
    }

    inline
    CampaignConfig::~CampaignConfig() noexcept
    {}
  }
}

#endif
