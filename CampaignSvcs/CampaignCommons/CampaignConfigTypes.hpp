#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <Commons/WireTypes.hpp>

namespace AdServer::CampaignSvcs
{
  namespace ChannelFlags
  {
    inline constexpr std::uint32_t HIDDEN = 0x04;
    inline constexpr std::uint32_t TARGETED = 0x10000;
  }

  using TimestampInfo = AdServer::Commons::TimestampInfo;

  using StringSeq = AdServer::Commons::StringSeq;

  struct AppFormatInfo
  {
    std::string app_format{};
    std::string mime_format{};
    TimestampInfo timestamp{};
  };

  using AppFormatSeq = std::vector<AppFormatInfo>;

  struct SizeInfo
  {
    std::uint32_t size_id{};
    std::string protocol_name{};
    std::uint32_t size_type_id{};
    std::uint32_t width{};
    std::uint32_t height{};
    TimestampInfo timestamp{};
  };

  using SizeSeq = std::vector<SizeInfo>;

  struct CreativeOptionInfo
  {
    std::int32_t option_id{};
    std::string token{};
    char type{};
    StringSeq token_relations{};
    TimestampInfo timestamp{};
  };

  using CreativeOptionSeq = std::vector<CreativeOptionInfo>;

  struct OptionValueInfo
  {
    std::int32_t option_id{};
    std::string value{};
  };

  using OptionValueSeq = std::vector<OptionValueInfo>;

  using CreativeCategoryIdSeq = std::vector<std::uint32_t>;

  struct CountryInfo
  {
    std::string country_code{};
    OptionValueSeq tokens{};
    TimestampInfo timestamp{};
  };

  using CountryInfoSeq = std::vector<CountryInfo>;

  struct ExternalCategoryInfo
  {
    std::uint32_t ad_request_type{};
    AdServer::Commons::StringSeq names{};
  };

  using ExternalCategorySeq = std::vector<ExternalCategoryInfo>;

  struct CreativeCategoryInfo
  {
    std::uint32_t creative_category_id{};
    std::uint32_t cct_id{};
    std::string name{};
    ExternalCategorySeq external_categories{};
    TimestampInfo timestamp{};
  };

  using CreativeCategorySeq = std::vector<CreativeCategoryInfo>;

  struct CreativeSizeInfo
  {
    std::uint32_t size_id{};
    std::uint32_t up_expand_space{};
    std::uint32_t right_expand_space{};
    std::uint32_t down_expand_space{};
    std::uint32_t left_expand_space{};
    OptionValueSeq tokens{};
  };

  using CreativeSizeSeq = std::vector<CreativeSizeInfo>;

  struct CreativeInfo
  {
    std::uint32_t ccid{};
    std::uint32_t creative_id{};
    std::uint32_t fc_id{};
    std::uint32_t weight{};
    CreativeSizeSeq sizes{};
    std::string creative_format{};
    OptionValueInfo click_url{};
    OptionValueInfo html_url{};
    std::uint32_t order_set_id{};
    std::uint32_t initial_contract_id{};
    CreativeCategoryIdSeq categories{};
    OptionValueSeq tokens{};
    char status{};
    std::string version_id{};
  };

  using ChannelIdSeq = std::vector<std::uint32_t>;

  using PublisherAccountIdSeq = std::vector<std::uint32_t>;

  struct ChannelWeight
  {
    std::uint32_t channel_id{};
    std::uint32_t weight{};
  };

  using ChannelWeightSeq = std::vector<ChannelWeight>;

  struct DeletedIdInfo
  {
    std::uint32_t id{};
    TimestampInfo timestamp{};
  };

  using DeletedIdSeq = std::vector<DeletedIdInfo>;

  struct DeletedStringIdInfo
  {
    std::string id{};
    TimestampInfo timestamp{};
  };

  using DeletedStringIdSeq = std::vector<DeletedStringIdInfo>;

  struct CampaignEcpmInfo
  {
    std::uint32_t ccg_id{};
    AdServer::Commons::DecimalInfo ecpm{};
    AdServer::Commons::DecimalInfo ctr{};
    TimestampInfo timestamp{};
  };

  using EcpmSeq = std::vector<CampaignEcpmInfo>;

  using ChannelCategorySeq = std::vector<std::uint32_t>;

  struct ExpressionInfo;

  using ExpressionSeq = std::vector<ExpressionInfo>;

  struct ExpressionInfo
  {
    char operation{};
    std::uint32_t channel_id{};
    ExpressionSeq sub_channels{};
  };

  struct ExpressionChannelInfo
  {
    std::uint32_t channel_id{};
    std::string name{};
    std::uint32_t account_id{};
    std::string country_code{};
    std::uint32_t flags{};
    char status{};
    char type{};
    bool is_public{};
    std::string language{};
    std::uint32_t freq_cap_id{};
    std::uint32_t parent_channel_id{};
    std::uint32_t action_id{};
    TimestampInfo timestamp{};
    std::string discover_query{};
    std::string discover_annotation{};
    std::uint32_t channel_rate_id{};
    AdServer::Commons::DecimalInfo imp_revenue{};
    AdServer::Commons::DecimalInfo click_revenue{};
    ExpressionInfo expression{};
    std::uint32_t threshold{};
  };

  using ExpressionChannelSeq = std::vector<ExpressionChannelInfo>;

  struct BehavioralParameterInfo
  {
    std::uint32_t behav_params_id{};
    std::uint32_t minimum_visits{};
    std::uint32_t time_from{};
    std::uint32_t time_to{};
    char trigger_type{};
    std::uint32_t weight{};
  };

  using BehavioralParameterSeq = std::vector<BehavioralParameterInfo>;

  struct WeeklyRunInterval
  {
    std::uint32_t min{};
    std::uint32_t max{};
  };

  using CreativeSeq = std::vector<CreativeInfo>;

  using WeeklyRunIntervalSeq = std::vector<WeeklyRunInterval>;

  using ColoIdSeq = std::vector<std::uint32_t>;

  using AccountIdSeq = std::vector<std::uint32_t>;

  using TagIdSeq = std::vector<std::uint32_t>;

  enum CreativeTemplateType
  {
    CTT_NONE,
    CTT_TEXT,
    CTT_XSLT,
  };

  struct CampaignDeliveryLimitsInfo
  {
    TimestampInfo date_start{};
    TimestampInfo date_end{};
    AdServer::Commons::OptionalDecimalInfo budget{};
    AdServer::Commons::OptionalDecimalInfo daily_budget{};
    bool imps_defined{};
    std::uint32_t imps{};
    bool clicks_defined{};
    std::uint32_t clicks{};
    char delivery_pacing{};
  };

  struct TagDeliveryInfo
  {
    std::uint32_t tag_id{};
    std::uint32_t delivery_value{};
  };

  using TagDeliverySeq = std::vector<TagDeliveryInfo>;

  using UserGroupIdSeq = std::vector<std::uint32_t>;

  using ULongSeq = std::vector<std::uint32_t>;

  struct ContractInfo
  {
    std::uint32_t contract_id{};
    std::string number{};
    std::string date{};
    std::string type{};
    bool vat_included{};
    std::string ord_contract_id{};
    std::string ord_ado_id{};
    std::string subject_type{};
    std::string action_type{};
    bool agent_acting_for_publisher{};
    std::uint32_t parent_contract_id{};
    std::string client_id{};
    std::string client_name{};
    std::string client_legal_form{};
    std::string contractor_id{};
    std::string contractor_name{};
    std::string contractor_legal_form{};
    TimestampInfo timestamp{};
  };

  using ContractSeq = std::vector<ContractInfo>;

  struct CampaignInfo
  {
    std::uint32_t campaign_id{};
    std::uint32_t campaign_group_id{};
    std::uint32_t ccg_rate_id{};
    char ccg_rate_type{};
    std::uint32_t fc_id{};
    std::uint32_t group_fc_id{};
    std::uint32_t priority{};
    std::uint32_t flags{};
    char marketplace{};
    ExpressionInfo expression{};
    ExpressionInfo stat_expression{};
    std::string country{};
    ULongSeq sites{};
    char status{};
    char eval_status{};
    WeeklyRunIntervalSeq weekly_run_intervals{};
    CreativeSeq creatives{};
    std::uint32_t account_id{};
    std::uint32_t advertiser_id{};
    AccountIdSeq exclude_pub_accounts{};
    TagDeliverySeq exclude_tags{};
    std::uint32_t delivery_coef{};
    AdServer::Commons::DecimalInfo imp_revenue{};
    AdServer::Commons::DecimalInfo click_revenue{};
    AdServer::Commons::DecimalInfo action_revenue{};
    AdServer::Commons::DecimalInfo commision{};
    char ccg_type{};
    char target_type{};
    CampaignDeliveryLimitsInfo campaign_delivery_limits{};
    CampaignDeliveryLimitsInfo ccg_delivery_limits{};
    std::uint32_t start_user_group_id{};
    std::uint32_t end_user_group_id{};
    AdServer::Commons::DecimalInfo max_pub_share{};
    std::uint32_t ctr_reset_id{};
    std::uint32_t random_imps{};
    std::uint32_t mode{};
    std::uint32_t seq_set_rotate_imps{};
    TimestampInfo min_uid_age{};
    ULongSeq colocations{};
    std::uint32_t bid_strategy{};
    AdServer::Commons::DecimalInfo min_ctr_goal{};
    TimestampInfo timestamp{};
    AdServer::Commons::DecimalInfo min_vtr_goal{};
  };

  struct TagPricingInfo
  {
    std::string country_code{};
    char ccg_type{};
    char ccg_rate_type{};
    std::uint32_t site_rate_id{};
    AdServer::Commons::DecimalInfo imp_revenue{};
    AdServer::Commons::DecimalInfo revenue_share{};
  };

  using TagPricingInfoSeq = std::vector<TagPricingInfo>;

  struct TemplateOptionsInfo
  {
    std::string template_name{};
    OptionValueSeq tokens{};
  };

  using TemplateOptionsSeq = std::vector<TemplateOptionsInfo>;

  struct TagSizeInfo
  {
    std::uint32_t size_id{};
    std::uint32_t max_text_creatives{};
    OptionValueSeq tokens{};
    OptionValueSeq hidden_tokens{};
  };

  using TagSizeSeq = std::vector<TagSizeInfo>;

  struct TagInfo
  {
    std::uint32_t tag_id{};
    std::uint32_t site_id{};
    char status{};
    TagSizeSeq sizes{};
    std::string imp_track_pixel{};
    std::string passback{};
    std::string passback_type{};
    std::uint32_t flags{};
    char marketplace{};
    AdServer::Commons::DecimalInfo adjustment{};
    TagPricingInfoSeq tag_pricings{};
    CreativeCategoryIdSeq accepted_categories{};
    CreativeCategoryIdSeq rejected_categories{};
    bool allow_expandable{};
    OptionValueSeq tokens{};
    OptionValueSeq hidden_tokens{};
    OptionValueSeq passback_tokens{};
    TemplateOptionsSeq template_tokens{};
    AdServer::Commons::DecimalInfo auction_max_ecpm_share{};
    AdServer::Commons::DecimalInfo auction_prop_probability_share{};
    AdServer::Commons::DecimalInfo auction_random_share{};
    AdServer::Commons::DecimalInfo cost_coef{};
    TimestampInfo tag_pricings_timestamp{};
    TimestampInfo timestamp{};
  };

  using CampaignIdSeq = std::vector<std::uint32_t>;

  using CCIdSeq = std::vector<std::uint32_t>;

  struct SiteInfo
  {
    std::uint32_t site_id{};
    char status{};
    std::uint32_t freq_cap_id{};
    std::uint32_t noads_timeout{};
    CreativeCategoryIdSeq approved_creative_categories{};
    CreativeCategoryIdSeq rejected_creative_categories{};
    CCIdSeq approved_creatives{};
    CCIdSeq rejected_creatives{};
    std::uint32_t flags{};
    std::uint32_t account_id{};
    TimestampInfo timestamp{};
  };

  struct ColocationInfo
  {
    std::uint32_t colo_id{};
    std::string colo_name{};
    std::uint32_t colo_rate_id{};
    std::uint32_t at_flags{};
    std::uint32_t ad_serving{};
    bool hid_profile{};
    std::uint32_t account_id{};
    AdServer::Commons::DecimalInfo revenue_share{};
    OptionValueSeq tokens{};
    TimestampInfo timestamp{};
  };

  using ColocationSeq = std::vector<ColocationInfo>;

  struct ColocationFlags
  {
    std::uint32_t colo_id{};
    std::uint32_t flags{};
    bool hid_profile{};
  };

  using ColocationFlagsSeq = std::vector<ColocationFlags>;

  struct FreqCapInfo
  {
    std::uint32_t fc_id{};
    std::uint32_t lifelimit{};
    std::uint32_t period{};
    std::uint32_t window_limit{};
    std::uint32_t window_time{};
    TimestampInfo timestamp{};
  };

  using FreqCapSeq = std::vector<FreqCapInfo>;

  struct CreativeTemplateFileInfo
  {
    std::string creative_format{};
    std::string creative_size{};
    std::string app_format{};
    std::string mime_format{};
    bool track_impr{};
    CreativeTemplateType type{};
    std::string template_file{};
  };

  using CreativeTemplateFileInfoSeq = std::vector<CreativeTemplateFileInfo>;

  struct CreativeTemplateInfo
  {
    std::uint32_t id{};
    TimestampInfo timestamp{};
    CreativeTemplateFileInfoSeq files{};
    OptionValueSeq tokens{};
    OptionValueSeq hidden_tokens{};
  };

  struct CurrencyInfo
  {
    AdServer::Commons::DecimalInfo rate{};
    std::uint32_t currency_id{};
    std::uint32_t currency_exchange_id{};
    std::uint32_t effective_date{};
    std::uint32_t fraction_digits{};
    std::string currency_code{};
    TimestampInfo timestamp{};
  };

  struct CampaignKeywordInfo
  {
    std::uint32_t ccg_keyword_id{};
    std::string original_keyword{};
    std::string click_url{};
    TimestampInfo timestamp{};
  };

  using CampaignKeywordSeq = std::vector<CampaignKeywordInfo>;

  using CCGIdSeq = std::vector<std::uint32_t>;

  struct AdvActionInfo
  {
    std::uint32_t action_id{};
    TimestampInfo timestamp{};
    CCGIdSeq ccg_ids{};
  };

  using AdvActionInfoSeq = std::vector<AdvActionInfo>;

  struct AccountInfo
  {
    std::uint32_t account_id{};
    std::uint32_t agency_account_id{};
    std::uint32_t internal_account_id{};
    std::uint32_t role_id{};
    std::string legal_name{};
    std::uint32_t flags{};
    std::uint32_t at_flags{};
    char text_adserving{};
    std::uint32_t currency_id{};
    std::string country{};
    TimestampInfo time_offset{};
    AdServer::Commons::DecimalInfo commision{};
    AdServer::Commons::DecimalInfo budget{};
    AdServer::Commons::DecimalInfo paid_amount{};
    AccountIdSeq walled_garden_accounts{};
    std::uint32_t auction_rate{};
    bool use_pub_pixels{};
    std::string pub_pixel_optin{};
    std::string pub_pixel_optout{};
    AdServer::Commons::DecimalInfo self_service_commission{};
    char status{};
    char eval_status{};
    TimestampInfo timestamp{};
  };

  using AccountInfoSeq = std::vector<AccountInfo>;

  struct CategoryChannelLocalizationInfo
  {
    std::string language{};
    std::string name{};
  };

  using CategoryChannelLocalizationSeq = std::vector<CategoryChannelLocalizationInfo>;

  struct CategoryChannelInfo
  {
    std::uint32_t channel_id{};
    std::string name{};
    std::string newsgate_name{};
    CategoryChannelLocalizationSeq localizations{};
    std::uint32_t parent_channel_id{};
    std::uint32_t flags{};
    TimestampInfo timestamp{};
  };

  using CategoryChannelSeq = std::vector<CategoryChannelInfo>;

  struct PriceRangeInfo
  {
    std::uint32_t min{};
    std::uint32_t max{};
  };

  using PriceRangeSeq = std::vector<PriceRangeInfo>;

  struct BehavParameter
  {
    std::uint32_t min_visits{};
    std::uint32_t time_from{};
    std::uint32_t time_to{};
    std::uint32_t weight{};
    char trigger_type{};
  };

  using BehavParameterSeq = std::vector<BehavParameter>;

  struct BehavParamInfo
  {
    std::uint32_t id{};
    std::uint32_t threshold{};
    TimestampInfo timestamp{};
    BehavParameterSeq bp_seq{};
  };

  using BehavParamInfoSeq = std::vector<BehavParamInfo>;

  struct KeyBehavParamInfo
  {
    std::string id{};
    std::uint32_t threshold{};
    TimestampInfo timestamp{};
    BehavParameterSeq bp_seq{};
  };

  using KeyBehavParamInfoSeq = std::vector<KeyBehavParamInfo>;

  struct BriefBehavParamInfo
  {
    std::uint32_t id{};
    std::uint32_t threshold{};
    AdServer::Commons::FixedTimestampInfo timestamp = std::string(8, '\0');
    BehavParameterSeq bp_seq{};
  };

  using BriefBehavParamInfoSeq = std::vector<BriefBehavParamInfo>;

  struct BriefKeyBehavParamInfo
  {
    std::string id{};
    std::uint32_t threshold{};
    AdServer::Commons::FixedTimestampInfo timestamp = std::string(8, '\0');
    BehavParameterSeq bp_seq{};
  };

  using BriefKeyBehavParamInfoSeq = std::vector<BriefKeyBehavParamInfo>;

  using ChannelTriggerIdSeq = std::vector<std::uint32_t>;

  struct SimpleChannelKey
  {
    std::uint32_t channel_id{};
    std::string country_code{};
    std::string language{};
    char status{};
    std::uint32_t behav_param_list_id{};
    std::string str_behav_param_list_id{};
    ChannelCategorySeq categories{};
    std::uint32_t threshold{};
    bool discover{};
    ChannelTriggerIdSeq page_triggers{};
    ChannelTriggerIdSeq search_triggers{};
    ChannelTriggerIdSeq url_triggers{};
    ChannelTriggerIdSeq url_keyword_triggers{};
    TimestampInfo timestamp{};
  };

  using SimpleChannelKeySeq = std::vector<SimpleChannelKey>;

  struct BriefSimpleChannelKey
  {
    std::uint32_t channel_id{};
    std::uint32_t behav_param_list_id{};
    std::string str_behav_param_list_id{};
    ChannelCategorySeq categories{};
    std::uint32_t threshold{};
    bool discover{};
  };

  using BriefSimpleChannelKeySeq = std::vector<BriefSimpleChannelKey>;

  struct GeoIPRegionCityInfo
  {
    std::string region{};
    std::string city{};
  };

  using GeoIPRegionCitySeq = std::vector<GeoIPRegionCityInfo>;

  struct GeoChannelInfo
  {
    std::uint32_t channel_id{};
    std::string country{};
    GeoIPRegionCitySeq geoip_targets{};
    AdServer::Commons::TimestampInfo timestamp{};
  };

  using GeoChannelSeq = std::vector<GeoChannelInfo>;

  struct GeoCoordChannelInfo
  {
    std::uint32_t channel_id{};
    AdServer::Commons::DecimalInfo longitude{};
    AdServer::Commons::DecimalInfo latitude{};
    AdServer::Commons::DecimalInfo radius{};
    AdServer::Commons::TimestampInfo timestamp{};
  };

  using GeoCoordChannelSeq = std::vector<GeoCoordChannelInfo>;

  struct BlockChannelInfo
  {
    std::uint32_t channel_id{};
    std::uint32_t size_id{};
    AdServer::Commons::TimestampInfo timestamp{};
  };

  using BlockChannelSeq = std::vector<BlockChannelInfo>;

  struct FraudConditionInfo
  {
    std::uint32_t id{};
    char type{};
    std::uint32_t limit{};
    TimestampInfo period{};
    TimestampInfo timestamp{};
  };

  using FraudConditionSeq = std::vector<FraudConditionInfo>;

  struct SearchEngineRegExpInfo
  {
    std::string host_postfix{};
    std::string regexp{};
    std::string encoding{};
    std::string post_encoding{};
    std::uint32_t decoding_depth{};
  };

  using SearchEngineRegExpSeq = std::vector<SearchEngineRegExpInfo>;

  struct SearchEngineInfo
  {
    std::uint32_t id{};
    std::string name{};
    SearchEngineRegExpSeq regexps{};
    TimestampInfo timestamp{};
  };

  using SearchEngineSeq = std::vector<SearchEngineInfo>;

  struct WebBrowserDetectorInfo
  {
    std::string marker{};
    std::string regexp{};
    bool regexp_required{};
    std::uint32_t priority{};
  };

  using WebBrowserDetectorSeq = std::vector<WebBrowserDetectorInfo>;

  struct WebBrowserInfo
  {
    std::string name{};
    WebBrowserDetectorSeq detectors{};
    TimestampInfo timestamp{};
  };

  using WebBrowserSeq = std::vector<WebBrowserInfo>;

  struct PlatformDetectorInfo
  {
    std::string use_name{};
    std::string marker{};
    std::string match_regexp{};
    std::string output_regexp{};
    std::uint32_t priority{};
  };

  using PlatformDetectorSeq = std::vector<PlatformDetectorInfo>;

  struct PlatformInfo
  {
    std::uint32_t platform_id{};
    std::string name{};
    std::string type{};
    PlatformDetectorSeq detectors{};
    TimestampInfo timestamp{};
  };

  using PlatformSeq = std::vector<PlatformInfo>;

  struct WebOperationInfo
  {
    std::uint32_t id{};
    std::string app{};
    std::string source{};
    std::string operation{};
    std::uint32_t flags{};
    TimestampInfo timestamp{};
  };

  using WebOperationSeq = std::vector<WebOperationInfo>;

  using CampaignInfoSeq = std::vector<CampaignInfo>;

  using SiteInfoSeq = std::vector<SiteInfo>;

  using TagInfoSeq = std::vector<TagInfo>;

  using ColocationInfoSeq = std::vector<ColocationInfo>;

  using CreativeTemplateInfoSeq = std::vector<CreativeTemplateInfo>;

  using CurrencyInfoSeq = std::vector<CurrencyInfo>;

  struct CampaignConfigUpdateInfo
  {
    std::uint32_t server_id{};
    TimestampInfo master_stamp{};
    TimestampInfo first_load_stamp{};
    TimestampInfo finish_load_stamp{};
    TimestampInfo current_time{};
    std::uint32_t global_freq_cap_id{};
    std::uint32_t currency_exchange_id{};
    std::uint32_t max_keyword_ecpm{};
    std::uint32_t google_publisher_account_id{};
    TimestampInfo fraud_user_deactivate_period{};
    AdServer::Commons::DecimalInfo cost_limit{};
    TimestampInfo global_params_timestamp{};
    AppFormatSeq app_formats{};
    DeletedStringIdSeq delete_app_formats{};
    SizeSeq sizes{};
    DeletedIdSeq delete_sizes{};
    CountryInfoSeq countries{};
    DeletedStringIdSeq deleted_countries{};
    AccountInfoSeq accounts{};
    DeletedIdSeq deleted_accounts{};
    CreativeOptionSeq activate_creative_options{};
    DeletedIdSeq delete_creative_options{};
    CampaignInfoSeq campaigns{};
    DeletedIdSeq deleted_campaigns{};
    EcpmSeq ecpms{};
    DeletedIdSeq deleted_ecpms{};
    SiteInfoSeq sites{};
    DeletedIdSeq deleted_sites{};
    TagInfoSeq tags{};
    DeletedIdSeq deleted_tags{};
    FreqCapSeq frequency_caps{};
    DeletedIdSeq deleted_freq_caps{};
    SimpleChannelKeySeq simple_channels{};
    DeletedIdSeq deleted_simple_channels{};
    AdServer::Commons::TimestampInfo geo_channels_timestamp{};
    GeoChannelSeq activate_geo_channels{};
    DeletedIdSeq delete_geo_channels{};
    GeoCoordChannelSeq activate_geo_coord_channels{};
    DeletedIdSeq delete_geo_coord_channels{};
    BlockChannelSeq activate_block_channels{};
    DeletedIdSeq delete_block_channels{};
    ColocationInfoSeq colocations{};
    DeletedIdSeq deleted_colocations{};
    CreativeTemplateInfoSeq creative_templates{};
    DeletedIdSeq deleted_templates{};
    CurrencyInfoSeq currencies{};
    ExpressionChannelSeq expression_channels{};
    DeletedIdSeq deleted_expression_channels{};
    CampaignKeywordSeq campaign_keywords{};
    DeletedIdSeq deleted_keywords{};
    CreativeCategorySeq creative_categories{};
    DeletedIdSeq deleted_creative_categories{};
    AdvActionInfoSeq adv_actions{};
    DeletedIdSeq deleted_adv_actions{};
    CategoryChannelSeq category_channels{};
    DeletedIdSeq deleted_category_channels{};
    BehavParamInfoSeq behav_params{};
    DeletedIdSeq deleted_behav_params{};
    KeyBehavParamInfoSeq key_behav_params{};
    DeletedStringIdSeq deleted_key_behav_params{};
    FraudConditionSeq fraud_conditions{};
    DeletedIdSeq deleted_fraud_conditions{};
    SearchEngineSeq search_engines{};
    DeletedIdSeq deleted_search_engines{};
    WebBrowserSeq web_browsers{};
    DeletedStringIdSeq deleted_web_browsers{};
    PlatformSeq platforms{};
    DeletedIdSeq deleted_platforms{};
    WebOperationSeq web_operations{};
    DeletedIdSeq delete_web_operations{};
    ContractSeq contracts{};
    DeletedIdSeq deleted_contracts{};
  };

}
