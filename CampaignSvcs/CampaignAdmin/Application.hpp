
#ifndef _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_ADMIN_APPLICATION_HPP_
#define _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_ADMIN_APPLICATION_HPP_

#include <list>
#include <string>
#include <sstream>

#include <eh/Exception.hpp>
#include <Generics/Singleton.hpp>

#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>
#include <CampaignSvcs/CampaignServer/CampaignServer.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>

#include <UtilCommons/Table.hpp>

/**
 * Class which requests campaign manager and outputs result into std output.
 */
class Application
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

public:
  Application() /*throw(Exception, eh::Exception)*/;

  virtual ~Application() noexcept;

/**
 * Parses arguments and issues corresponding request to CampaignManager.
 * @param argc Number of arguments passed to utility process
 * @param argv Arguments passed to utility process
 */
  int run(int& argc, char** argv)
    /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/;

  static Application* instance() /*throw(eh::Exception)*/;

  struct Argument
  {
    std::string name;
    std::string value;
    Table::Relation relation;

    Argument(const char* nm = 0,
      const char* vl = 0,
      Table::Relation rel = Table::RL_EQ)
      /*throw(eh::Exception)*/;
  };

  typedef std::list<Argument> ArgList;

  enum
  {
    command_help  = 0,
    command_globals,
    command_creative_categories,
    command_sites,
    command_colocations,
    command_countries,
    command_creative_templates,
    command_campaign,
    command_tags,
    command_currency,
    command_creative,
    command_freq_caps,
    command_creative_option,
    command_adv_action,
    command_category_channel,
    command_geo_channel,
    command_geo_coord_channel,
    command_web_operations,
    command_keyword,
    command_account,
    command_fraud_condition,
    command_select,
    command_preview,
    command_expression_channel,
    command_simple_channel,
    command_simple_channel_cs,
    command_trace_index,
    command_trace_weight,
    command_behave_param,
    command_behave_keys,
    command_search_engines,
    command_colocation_flags,
    command_detectors,
    command_stat,
    command_stat_ccg,
    command_stat_account,
    command_stat_campaign,
    command_update_stat,
    command_decode,
    command_app_format,
    command_size,
    command_bill_accounts,
    command_bill_campaigns,
    command_bill_ccgs,
    command_max,
  };


  int help() noexcept;

  int show(int aspect)
    /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/;

  int select()
    /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/;

  int preview()
    /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/;

  int trace_campaign_weight()
    /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/;

  int trace_campaign_index()
    /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/;

  int decode()
    /*throw(InvalidArgument, Exception, eh::Exception, CORBA::Exception)*/;

  /* CampaignServer commands */
  void simple_channel()
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  void simple_channel_cs()
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  int bill_stats(int command_index)
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  void behav_param(int command_index)
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  void expression_channels()
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  void fraud_conditions()
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  void search_engines(bool use_update, bool deleted)
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  void detectors(bool web, bool platform)
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  void colocation_flags()
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

  // stat utils
  void campaign_stat() /*throw(Exception, InvalidArgument)*/;

  void stat(bool account) /*throw(Exception, InvalidArgument)*/;

  void update_stat() /*throw(Exception)*/;

  static Table* setup_table_(
    size_t columns,
    const Table::Column legend[])
    noexcept;
private:
  typedef std::map<unsigned long, const AdServer::CampaignSvcs::SizeInfo*>
    SizeMap;

private:
  int parse_args_(int argc, char** argv) /*throw(InvalidArgument)*/;

  void init_server_ref_() /*throw(InvalidArgument)*/;

  void init_manager_ref_() /*throw(InvalidArgument)*/;

  template<typename VALUE>
  void add_behav_value_(Table* table, const VALUE& info) noexcept;

  static void print_custom_options(
    AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
    std::string& str,
    const AdServer::CampaignSvcs::OptionValueSeq& tokens,
    const char* prefix,
    bool expand_options);

  static void describe_account(
    AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
    Table::Row& row,
    const AdServer::CampaignSvcs::AccountInfo& acc_info);

  static void describe_creative_category(
    AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
    Table::Row& row,
    const AdServer::CampaignSvcs::CreativeCategoryInfo& creative_category_info);

  static void describe_campaign(
    AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
    Table::Row& row,
    const AdServer::CampaignSvcs::
      CampaignManager::AdaptedCampaignInfo& campaign_info,
    AdServer::CampaignSvcs::ExpressionChannelHolderMap& channels,
    bool expand);

  static void describe_tag(
    AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
    const SizeMap& sizes,
    Table::Row& row,
    const AdServer::CampaignSvcs::CampaignManager::AdaptedTagInfo& tag_info,
    const char* prefix,
    bool expand_options);

  static void describe_creative(
    AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
    const SizeMap& sizes,
    Table::Row& row,
    const AdServer::CampaignSvcs::CampaignManager::AdaptedCampaignInfo&
      adapted_campaign_info,
    const AdServer::CampaignSvcs::CreativeInfo& creative,
    bool expand,
    bool expand_options,
    const char* prefix);

  static void describe_site(
    AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
    Table::Row& row,
    const AdServer::CampaignSvcs::SiteInfo& site_info,
    bool expand);

  static void describe_expression_channel(
    Table::Row& row,
    const AdServer::CampaignSvcs::ExpressionChannelBase* channel)
    /*throw(eh::Exception)*/;

  static void describe_colocation(
    AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
    Table::Row& row,
    const AdServer::CampaignSvcs::ColocationInfo& colocation_info,
    const char* prefix,
    bool expand_options);

  static void describe_country(
    AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
    Table::Row& row,
    const AdServer::CampaignSvcs::CountryInfo& country_info,
    const char* prefix,
    bool expand_options);

  static void describe_geo_channel(
    Table::Row& row,
    const AdServer::CampaignSvcs::GeoChannelInfo& geo_channel_info);

  static void describe_geo_coord_channel(
    Table::Row& row,
    const AdServer::CampaignSvcs::GeoCoordChannelInfo& geo_channel_info);

  static const AdServer::CampaignSvcs::AccountInfo*
  get_account(
    const AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
    unsigned long account_id)
    /*throw(eh::Exception, CORBA::Exception)*/;

  static std::string
  creatives(const AdServer::CampaignSvcs::CreativeSeq& creatives)
    /*throw(eh::Exception)*/;

  static std::string
  campaign_sites(
    const AdServer::CampaignSvcs::CampaignInfo& campaign,
    const AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config)
    /*throw(eh::Exception)*/;

  static std::string
  campaign_weekly_run(
    const AdServer::CampaignSvcs::CampaignInfo& campaign)
    /*throw(eh::Exception)*/;

  static std::string
  campaign_exclude_urls(
    const AdServer::CampaignSvcs::CampaignInfo& campaign)
    /*throw(eh::Exception)*/;

  static std::string decode_campaign_flags(
    unsigned long flags) noexcept;

  static std::string decode_account_type_flags(
    unsigned long flags) noexcept;

  static std::string
  decode_bid_strategy(
    unsigned long bid_strategy) noexcept;

  static void fill_channels_(
    AdServer::CampaignSvcs::ExpressionChannelHolderMap& channel_holders,
    const AdServer::CampaignSvcs::ExpressionChannelSeq& channels)
    noexcept;

  static void print_search_engines_(
    const AdServer::CampaignSvcs::SearchEngineSeq& engines,
    Table::Filters& filters) noexcept;

  static void print_colocation_flags_(
    const AdServer::CampaignSvcs::ColocationFlagsSeq& colocations)
    noexcept;

  static void init_config_settings(
    AdServer::CampaignSvcs::CampaignGetConfigSettings& config_settings)
    noexcept;

  static AdServer::CampaignSvcs::FreqCapInfo get_freq_cap(
    const AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config,
    unsigned long fc_id)
    /*throw(eh::Exception, CORBA::Exception)*/;

  static void fill_sizes_(
    SizeMap& sizes,
    const AdServer::CampaignSvcs::CampaignManager::CampaignConfig* config)
    noexcept;

private:
  AdServer::CampaignSvcs::CampaignManager_var manager_;
  AdServer::CampaignSvcs::CampaignServer_var server_;
  CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
  Table::Filters filters_;
  Table::Sorter sorter_;
};

typedef std::unique_ptr<Table> TablePtr;

/**
 * Defines singleton class.
 */
typedef Generics::Singleton<Application> AppSingleton;

/* Inlines */
inline
Application*
Application::instance() /*throw(eh::Exception)*/
{
  return &AppSingleton::instance();
}

inline
Application::Argument::Argument(
  const char* nm,
  const char* vl,
  Table::Relation rel)
  /*throw(eh::Exception)*/
  : name(nm ? nm : ""),
    value(vl ? vl : ""),
    relation(rel)
{}

template<typename SeqType>
std::string sequence_to_string(const SeqType& seq)
{
  std::ostringstream ostr;
  CorbaAlgs::print_sequence(ostr, seq);
  return ostr.str();
}

#endif // _AD_SERVER_CAMPAIGN_SERVICE_CAMPAIGN_ADMIN_APPLICATION_HPP_
