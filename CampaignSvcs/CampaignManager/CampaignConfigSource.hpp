
#ifndef _CAMPAIGNCONFIGSOURCE_HPP_
#define _CAMPAIGNCONFIGSOURCE_HPP_

#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>
#include <Generics/FileCache.hpp>

#include <CampaignSvcs/CampaignServer/CampaignServerPool.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManager_s.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include "DomainParser.hpp"
#include "CampaignConfig.hpp"
#include "CampaignManagerDeclarations.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    using namespace AdServer::CampaignSvcs::AdInstances;

    class CampaignConfigSource:
      public Generics::SimpleActiveObject,
      public ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(Interrupted, Exception);

      CampaignConfigSource(
        Logging::Logger* logger,
        DomainParser* domain_parser,
        const CORBACommons::CorbaObjectRefList& campaign_server_refs,
        const char* campaigns_types,
        const char* creative_file_dir,
        const char* template_file_dir,
        const std::string& service_index,
        const CreativeInstantiateRuleMap& creative_rules,
        bool drop_https_safe = false)
        /*throw(Exception)*/;

      CampaignConfig_var
      update(const CampaignConfig* old_config) /*throw(Exception)*/;

    private:
      struct TraceLevel
      {
        enum
        {
          LOW = Logging::Logger::TRACE,
          MIDDLE,
          HIGH
        };
      };

      typedef std::map<std::string, StringSet> SizeAppFormatSet;

      typedef std::map<std::string, SizeAppFormatSet>
        CreativeFormatTemplateMap_;

      struct ConfigUpdateLinks
      {
        struct EcpmHolder
        {
          RevenueDecimal ecpm;
          RevenueDecimal ctr;
        };

        typedef std::map<unsigned long, unsigned long> IdMap;
        typedef std::map<unsigned long, EcpmHolder> EcpmMap;
        typedef std::map<std::string, CreativeCategoryIdSet>
          DomainExcludeCategoryMap;
        typedef std::map<unsigned long, OptionValueMap>
          IdOptionValueMap;

        typedef std::map<unsigned long, OptionValueMap>
          CreativeSizeOptionValueMap;
        typedef std::map<unsigned long, CreativeSizeOptionValueMap>
          IdCreativeSizeOptionValueMap;

        typedef std::map<std::string, OptionValueMap>
          TemplateOptionValueMap;
        typedef std::map<std::string, OptionValueMap>
          CountryOptionValueMap;
        typedef std::map<unsigned long, TemplateOptionValueMap>
          IdTemplateOptionValueMap;

        typedef std::map<unsigned long, OptionValueMap>
          TagSizeOptionValueMap;
        typedef std::map<unsigned long, TagSizeOptionValueMap>
          IdTagSizeOptionValueMap;

        typedef ReferenceCounting::SmartPtr<RCOptionTokenValueMap>
          OptionTokenValueMap_var;
        struct TemplateOptionsLink
        {
          OptionValueMap unlinked_tokens;
          OptionTokenValueMap_var tokens_ref;
          OptionValueMap unlinked_hidden_tokens;
          OptionTokenValueMap_var hidden_tokens_ref;
        };

        typedef std::map<unsigned long, TemplateOptionsLink>
          TemplateOptionsLinkMap;

        typedef std::list<unsigned long> IdList;
        typedef std::map<unsigned long, IdList> BlockChannelMap;

        IdMap account_currencies;
        IdMap account_agencies;

        IdMap campaign_accounts;
        IdMap campaign_advertisers;

        IdMap site_accounts;
        IdMap tag_sites;

        IdMap colocation_accounts;
        IdMap keyword_campaigns;

        IdMap margin_rule_accounts;
        EcpmMap campaign_ecpms;

        DomainExcludeCategoryMap domain_category_exclusions;

        IdOptionValueMap creative_option_values;
        IdCreativeSizeOptionValueMap creative_size_option_values;
        IdOptionValueMap tag_option_values;
        IdOptionValueMap tag_hidden_option_values;
        IdOptionValueMap tag_passback_option_values;
        IdTemplateOptionValueMap tag_template_option_values;
        IdTagSizeOptionValueMap tag_size_option_values;
        IdTagSizeOptionValueMap tag_size_option_hidden_values;
        TemplateOptionsLinkMap template_option_values;
        IdOptionValueMap colocation_option_values;
        CountryOptionValueMap country_option_values;

        ExpressionChannelHolderMap platform_channels;
        BlockChannelMap block_channels;
      };

    private:
      virtual
      ~CampaignConfigSource() noexcept
      {}

      Creative* adapt_creative_info_(
        const Campaign* campaign,
        const AdServer::CampaignSvcs::CreativeInfo& creative_info)
        /*throw(Exception, eh::Exception)*/;

      const char* adapt_creative_file_path(
        const char* file_path,
        std::string& result_file_path)
        noexcept;

      const char* adopt_template_path(
        const char* template_dir,
        std::string& result_template_path)
        noexcept;

      static void
      fill_tag_pricings_(Tag* tag) noexcept;

      void apply_config_update_(
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links,
        const CampaignConfigUpdateInfo& update_info,
        const CampaignConfig* old_config)
        /*throw(Exception)*/;

      void
      apply_sizes_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config)
        /*throw(Exception)*/;
        
      void
      apply_app_formats_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config)
        /*throw(Exception)*/;

      void apply_creative_categories_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception)*/;

      void apply_currency_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config)
        /*throw(Exception)*/;

      void apply_frequency_caps_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config)
        /*throw(Exception)*/;

      void apply_creative_templates_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception)*/;

      void apply_account_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception)*/;

      void apply_creative_options_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config)
        /*throw(Exception)*/;

      void apply_campaigns_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception)*/;

      void apply_expression_channels_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception)*/;

      void apply_sites_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception)*/;

      void apply_tags_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception)*/;

      void apply_colocations_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception)*/;

      void apply_countries_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception)*/;

      void apply_campaign_keywords_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception, eh::Exception)*/;

      void apply_category_channels_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config)
        /*throw(Exception, eh::Exception)*/;

      void apply_simple_channels_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config)
        /*throw(Exception, eh::Exception)*/;

      void apply_margin_rules_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception, eh::Exception)*/;

      void apply_ccg_keyword_update_(
        const CampaignConfigUpdateInfo& update_info,
        CampaignConfig& new_config,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception, eh::Exception)*/;

      void apply_geo_channel_update_(
        CampaignConfig& new_config,
        const CampaignConfigUpdateInfo& update_info,
        const CampaignConfig* old_config)
        noexcept;

      void apply_platform_update_(
        CampaignConfig& new_config,
        const CampaignConfigUpdateInfo& update_info)
        noexcept;

      void apply_web_app_update_(
        CampaignConfig& new_config,
        const CampaignConfigUpdateInfo& update_info)
        noexcept;

      void
      apply_block_channel_update_(
        const CampaignConfigUpdateInfo& update_info,
        ConfigUpdateLinks& config_update_links)
        /*throw(Exception, eh::Exception)*/;

      void link_config_update_(
        const ConfigUpdateLinks& config_update_links,
        CampaignConfig& new_config)
        /*throw(Exception)*/;

      void link_template_update_(
        CampaignConfig& new_config,
        const ConfigUpdateLinks& config_update_links)
        noexcept;

      void link_account_update_(
        CampaignConfig& new_config,
        const ConfigUpdateLinks& config_update_links)
        noexcept;

      void link_campaign_update_(
        CampaignConfig& new_config,
        const ConfigUpdateLinks& config_update_links,
        const CreativeFormatTemplateMap_& creative_format_template_map)
        noexcept;

      void fill_platform_channel_priorities_(
        CampaignConfig& new_config,
        const ConfigUpdateLinks& config_update_links) noexcept;

      void link_block_channel_update_(
        CampaignConfig& new_config,
        const ConfigUpdateLinks& config_update_links)
        noexcept;

      void check_category_channels_(CampaignConfig& new_config) noexcept;

      void fill_campaign_action_markers_(CampaignConfig& new_config) noexcept;

      void enrich_channels_by_geo_channels_(CampaignConfig& new_config) noexcept;

      void
      enrich_channels_by_geo_coord_channels_(
        CampaignConfig& new_config) noexcept;

      void enrich_channels_by_platform_channels_(CampaignConfig& new_config) noexcept;

      unsigned long
      filter_not_exist_fc_(unsigned long fc_id, const FreqCapMap& freq_caps_map)
        noexcept;

      static bool
      link_option_values_(
        OptionTokenValueMap& option_token_values,
        const CampaignConfig& campaign_config,
        const OptionValueMap& option_values);

      static bool
      link_option_values_(
        OptionTokenValueMap& option_token_values,
        const CampaignConfig& campaign_config,
        const ConfigUpdateLinks::IdOptionValueMap& option_values,
        unsigned long id);

      static void
      link_tag_size_option_values_(
        Tag& tag,
        OptionTokenValueMap Tag::Size::* tokens_field,
        const CampaignConfig& new_config,
        unsigned long tag_id,
        const ConfigUpdateLinks::IdTagSizeOptionValueMap& tag_size_option_values)
        noexcept;

      /* dynamic changes */
      void apply_campaign_limitations_(
        CampaignConfig& config,
        const Generics::Time& now)
        noexcept;

      void check_creative_files_option_(CampaignConfig& new_config)
        noexcept;

      void check_creative_template_files_(CampaignConfig& new_config)
        noexcept;

      void preinstantiate_creative_tokens_(CampaignConfig& new_config) noexcept;

      void fill_default_tokens_(
        const Campaign& for_campaign,
        const Creative* creative,
        TokenValueMap& system_tokens)
        /*throw(eh::Exception)*/;

      void
      fill_clickurl_tokens_(
        const Campaign& campaign,
        TokenValueMap& system_tokens)
        /*throw(eh::Exception)*/;

      static void link_url_categories_(
        Creative::CategorySet& target_creative_categories,
        const String::SubString& url,
        const ConfigUpdateLinks::DomainExcludeCategoryMap&
          domain_category_exclusions)
        noexcept;

    private:
      Logging::Logger_var logger_;
      DomainParser_var domain_parser_;
      CORBACommons::CorbaClientAdapter_var corba_client_adapter_;

      const std::string campaigns_types_;
      const std::string creative_file_dir_;
      const std::string template_file_dir_;
      const unsigned SERVICE_INDEX_;
      const CreativeInstantiateRuleMap creative_rules_;
      const bool drop_https_safe_;

      CampaignServerPoolPtr campaign_servers_;
      Generics::FileAccessCacheManager file_access_manager_;
    };

    typedef ReferenceCounting::SmartPtr<CampaignConfigSource>
      CampaignConfigSource_var;
  }
}

#endif
