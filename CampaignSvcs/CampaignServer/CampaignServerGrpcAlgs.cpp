#include "CampaignServerGrpcAlgs.hpp"

#include <stdexcept>

namespace AdServer::CampaignSvcs::CampaignGrpc
{
  namespace
  {
    void unpack_bytes(const std::string& source, std::string& target, std::size_t size,
      bool optional = false)
    {
      if (source.size() != size && !(optional && source.empty()))
      {
        throw std::invalid_argument("Invalid packed time/decimal width");
      }

      target = source;
    }
  }

  void pack(const AppFormatInfo& source, rpc::AppFormatInfo& target)
  {
    target.Clear();
    target.set_app_format(source.app_format);
    target.set_mime_format(source.mime_format);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::AppFormatInfo& source, AppFormatInfo& target)
  {
    target.app_format = source.app_format();
    target.mime_format = source.mime_format();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const SizeInfo& source, rpc::SizeInfo& target)
  {
    target.Clear();
    target.set_size_id(source.size_id);
    target.set_protocol_name(source.protocol_name);
    target.set_size_type_id(source.size_type_id);
    target.set_width(source.width);
    target.set_height(source.height);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::SizeInfo& source, SizeInfo& target)
  {
    target.size_id = source.size_id();
    target.protocol_name = source.protocol_name();
    target.size_type_id = source.size_type_id();
    target.width = source.width();
    target.height = source.height();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const CreativeOptionInfo& source, rpc::CreativeOptionInfo& target)
  {
    target.Clear();
    target.set_option_id(source.option_id);
    target.set_token(source.token);
    target.set_type(static_cast<unsigned char>(source.type));

    for (std::uint32_t i = 0; i < source.token_relations.size(); ++i)
    {
      target.add_token_relations(source.token_relations[i]);
    }

    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::CreativeOptionInfo& source, CreativeOptionInfo& target)
  {
    target.option_id = source.option_id();
    target.token = source.token();

    if (source.type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.type = source.type();
    target.token_relations.resize(source.token_relations_size());
    for (std::uint32_t i = 0; i < target.token_relations.size(); ++i)
    {
      target.token_relations[i] = source.token_relations(i);
    }

    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const OptionValueInfo& source, rpc::OptionValueInfo& target)
  {
    target.Clear();
    target.set_option_id(source.option_id);
    target.set_value(source.value);
  }

  void unpack(const rpc::OptionValueInfo& source, OptionValueInfo& target)
  {
    target.option_id = source.option_id();
    target.value = source.value();
  }

  void pack(const CountryInfo& source, rpc::CountryInfo& target)
  {
    target.Clear();
    target.set_country_code(source.country_code);

    for (std::uint32_t i = 0; i < source.tokens.size(); ++i)
    {
      pack(source.tokens[i], *target.add_tokens());
    }

    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::CountryInfo& source, CountryInfo& target)
  {
    target.country_code = source.country_code();
    target.tokens.resize(source.tokens_size());
    for (std::uint32_t i = 0; i < target.tokens.size(); ++i)
    {
      unpack(source.tokens(i), target.tokens[i]);
    }

    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const ExternalCategoryInfo& source, rpc::ExternalCategoryInfo& target)
  {
    target.Clear();
    target.set_ad_request_type(source.ad_request_type);

    for (std::uint32_t i = 0; i < source.names.size(); ++i)
    {
      target.add_names(source.names[i]);
    }
  }

  void unpack(const rpc::ExternalCategoryInfo& source, ExternalCategoryInfo& target)
  {
    target.ad_request_type = source.ad_request_type();
    target.names.resize(source.names_size());
    for (std::uint32_t i = 0; i < target.names.size(); ++i)
    {
      target.names[i] = source.names(i);
    }
  }

  void pack(const CreativeCategoryInfo& source, rpc::CreativeCategoryInfo& target)
  {
    target.Clear();
    target.set_creative_category_id(source.creative_category_id);
    target.set_cct_id(source.cct_id);
    target.set_name(source.name);

    for (std::uint32_t i = 0; i < source.external_categories.size(); ++i)
    {
      pack(source.external_categories[i], *target.add_external_categories());
    }

    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::CreativeCategoryInfo& source, CreativeCategoryInfo& target)
  {
    target.creative_category_id = source.creative_category_id();
    target.cct_id = source.cct_id();
    target.name = source.name();
    target.external_categories.resize(source.external_categories_size());
    for (std::uint32_t i = 0; i < target.external_categories.size(); ++i)
    {
      unpack(source.external_categories(i), target.external_categories[i]);
    }

    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const CreativeSizeInfo& source, rpc::CreativeSizeInfo& target)
  {
    target.Clear();
    target.set_size_id(source.size_id);
    target.set_up_expand_space(source.up_expand_space);
    target.set_right_expand_space(source.right_expand_space);
    target.set_down_expand_space(source.down_expand_space);
    target.set_left_expand_space(source.left_expand_space);

    for (std::uint32_t i = 0; i < source.tokens.size(); ++i)
    {
      pack(source.tokens[i], *target.add_tokens());
    }
  }

  void unpack(const rpc::CreativeSizeInfo& source, CreativeSizeInfo& target)
  {
    target.size_id = source.size_id();
    target.up_expand_space = source.up_expand_space();
    target.right_expand_space = source.right_expand_space();
    target.down_expand_space = source.down_expand_space();
    target.left_expand_space = source.left_expand_space();
    target.tokens.resize(source.tokens_size());
    for (std::uint32_t i = 0; i < target.tokens.size(); ++i)
    {
      unpack(source.tokens(i), target.tokens[i]);
    }
  }

  void pack(const CreativeInfo& source, rpc::CreativeInfo& target)
  {
    target.Clear();
    target.set_ccid(source.ccid);
    target.set_creative_id(source.creative_id);
    target.set_fc_id(source.fc_id);
    target.set_weight(source.weight);

    for (std::uint32_t i = 0; i < source.sizes.size(); ++i)
    {
      pack(source.sizes[i], *target.add_sizes());
    }

    target.set_creative_format(source.creative_format);
    pack(source.click_url, *target.mutable_click_url());
    pack(source.html_url, *target.mutable_html_url());
    target.set_order_set_id(source.order_set_id);
    target.set_initial_contract_id(source.initial_contract_id);

    for (std::uint32_t i = 0; i < source.categories.size(); ++i)
    {
      target.add_categories(source.categories[i]);
    }

    for (std::uint32_t i = 0; i < source.tokens.size(); ++i)
    {
      pack(source.tokens[i], *target.add_tokens());
    }

    target.set_status(static_cast<unsigned char>(source.status));
    target.set_version_id(source.version_id);
  }

  void unpack(const rpc::CreativeInfo& source, CreativeInfo& target)
  {
    target.ccid = source.ccid();
    target.creative_id = source.creative_id();
    target.fc_id = source.fc_id();
    target.weight = source.weight();
    target.sizes.resize(source.sizes_size());
    for (std::uint32_t i = 0; i < target.sizes.size(); ++i)
    {
      unpack(source.sizes(i), target.sizes[i]);
    }

    target.creative_format = source.creative_format();
    unpack(source.click_url(), target.click_url);
    unpack(source.html_url(), target.html_url);
    target.order_set_id = source.order_set_id();
    target.initial_contract_id = source.initial_contract_id();
    target.categories.resize(source.categories_size());
    for (std::uint32_t i = 0; i < target.categories.size(); ++i)
    {
      target.categories[i] = source.categories(i);
    }

    target.tokens.resize(source.tokens_size());
    for (std::uint32_t i = 0; i < target.tokens.size(); ++i)
    {
      unpack(source.tokens(i), target.tokens[i]);
    }

    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.status = source.status();
    target.version_id = source.version_id();
  }

  void pack(const ChannelWeight& source, rpc::ChannelWeight& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);
    target.set_weight(source.weight);
  }

  void unpack(const rpc::ChannelWeight& source, ChannelWeight& target)
  {
    target.channel_id = source.channel_id();
    target.weight = source.weight();
  }

  void pack(const DeletedIdInfo& source, rpc::DeletedIdInfo& target)
  {
    target.Clear();
    target.set_id(source.id);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::DeletedIdInfo& source, DeletedIdInfo& target)
  {
    target.id = source.id();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const DeletedStringIdInfo& source, rpc::DeletedStringIdInfo& target)
  {
    target.Clear();
    target.set_id(source.id);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::DeletedStringIdInfo& source, DeletedStringIdInfo& target)
  {
    target.id = source.id();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const CampaignEcpmInfo& source, rpc::CampaignEcpmInfo& target)
  {
    target.Clear();
    target.set_ccg_id(source.ccg_id);
    target.set_ecpm(source.ecpm);
    target.set_ctr(source.ctr);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::CampaignEcpmInfo& source, CampaignEcpmInfo& target)
  {
    target.ccg_id = source.ccg_id();
    unpack_bytes(source.ecpm(), target.ecpm, 9);
    unpack_bytes(source.ctr(), target.ctr, 9);
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const ExpressionInfo& source, rpc::ExpressionInfo& target)
  {
    target.Clear();
    target.set_operation(static_cast<unsigned char>(source.operation));
    target.set_channel_id(source.channel_id);

    for (std::uint32_t i = 0; i < source.sub_channels.size(); ++i)
    {
      pack(source.sub_channels[i], *target.add_sub_channels());
    }
  }

  void unpack(const rpc::ExpressionInfo& source, ExpressionInfo& target)
  {
    if (source.operation() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.operation = source.operation();
    target.channel_id = source.channel_id();
    target.sub_channels.resize(source.sub_channels_size());
    for (std::uint32_t i = 0; i < target.sub_channels.size(); ++i)
    {
      unpack(source.sub_channels(i), target.sub_channels[i]);
    }
  }

  void pack(const ExpressionChannelInfo& source, rpc::ExpressionChannelInfo& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);
    target.set_name(source.name);
    target.set_account_id(source.account_id);
    target.set_country_code(source.country_code);
    target.set_flags(source.flags);
    target.set_status(static_cast<unsigned char>(source.status));
    target.set_type(static_cast<unsigned char>(source.type));
    target.set_is_public(source.is_public);
    target.set_language(source.language);
    target.set_freq_cap_id(source.freq_cap_id);
    target.set_parent_channel_id(source.parent_channel_id);
    target.set_action_id(source.action_id);
    target.set_timestamp(source.timestamp);
    target.set_discover_query(source.discover_query);
    target.set_discover_annotation(source.discover_annotation);
    target.set_channel_rate_id(source.channel_rate_id);
    target.set_imp_revenue(source.imp_revenue);
    target.set_click_revenue(source.click_revenue);
    pack(source.expression, *target.mutable_expression());
    target.set_threshold(source.threshold);
  }

  void unpack(const rpc::ExpressionChannelInfo& source, ExpressionChannelInfo& target)
  {
    target.channel_id = source.channel_id();
    target.name = source.name();
    target.account_id = source.account_id();
    target.country_code = source.country_code();
    target.flags = source.flags();

    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.status = source.status();

    if (source.type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.type = source.type();
    target.is_public = source.is_public();
    target.language = source.language();
    target.freq_cap_id = source.freq_cap_id();
    target.parent_channel_id = source.parent_channel_id();
    target.action_id = source.action_id();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    target.discover_query = source.discover_query();
    target.discover_annotation = source.discover_annotation();
    target.channel_rate_id = source.channel_rate_id();
    unpack_bytes(source.imp_revenue(), target.imp_revenue, 9, !target.channel_rate_id);
    unpack_bytes(source.click_revenue(), target.click_revenue, 9, !target.channel_rate_id);
    unpack(source.expression(), target.expression);
    target.threshold = source.threshold();
  }

  void pack(const BehavioralParameterInfo& source, rpc::BehavioralParameterInfo& target)
  {
    target.Clear();
    target.set_behav_params_id(source.behav_params_id);
    target.set_minimum_visits(source.minimum_visits);
    target.set_time_from(source.time_from);
    target.set_time_to(source.time_to);
    target.set_trigger_type(static_cast<unsigned char>(source.trigger_type));
    target.set_weight(source.weight);
  }

  void unpack(const rpc::BehavioralParameterInfo& source, BehavioralParameterInfo& target)
  {
    target.behav_params_id = source.behav_params_id();
    target.minimum_visits = source.minimum_visits();
    target.time_from = source.time_from();
    target.time_to = source.time_to();

    if (source.trigger_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.trigger_type = source.trigger_type();
    target.weight = source.weight();
  }

  void pack(const WeeklyRunInterval& source, rpc::WeeklyRunInterval& target)
  {
    target.Clear();
    target.set_min(source.min);
    target.set_max(source.max);
  }

  void unpack(const rpc::WeeklyRunInterval& source, WeeklyRunInterval& target)
  {
    target.min = source.min();
    target.max = source.max();
  }

  void pack(const CampaignDeliveryLimitsInfo& source, rpc::CampaignDeliveryLimitsInfo& target)
  {
    target.Clear();
    target.set_date_start(source.date_start);
    target.set_date_end(source.date_end);
    target.set_budget(source.budget);
    target.set_daily_budget(source.daily_budget);
    target.set_imps_defined(source.imps_defined);
    target.set_imps(source.imps);
    target.set_clicks_defined(source.clicks_defined);
    target.set_clicks(source.clicks);
    target.set_delivery_pacing(static_cast<unsigned char>(source.delivery_pacing));
  }

  void unpack(const rpc::CampaignDeliveryLimitsInfo& source, CampaignDeliveryLimitsInfo& target)
  {
    unpack_bytes(source.date_start(), target.date_start, 8);
    unpack_bytes(source.date_end(), target.date_end, 8);
    unpack_bytes(source.budget(), target.budget, 9, true);
    unpack_bytes(source.daily_budget(), target.daily_budget, 9, true);
    target.imps_defined = source.imps_defined();
    target.imps = source.imps();
    target.clicks_defined = source.clicks_defined();
    target.clicks = source.clicks();

    if (source.delivery_pacing() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.delivery_pacing = source.delivery_pacing();
  }

  void pack(const TagDeliveryInfo& source, rpc::TagDeliveryInfo& target)
  {
    target.Clear();
    target.set_tag_id(source.tag_id);
    target.set_delivery_value(source.delivery_value);
  }

  void unpack(const rpc::TagDeliveryInfo& source, TagDeliveryInfo& target)
  {
    target.tag_id = source.tag_id();
    target.delivery_value = source.delivery_value();
  }

  void pack(const ContractInfo& source, rpc::ContractInfo& target)
  {
    target.Clear();
    target.set_contract_id(source.contract_id);
    target.set_number(source.number);
    target.set_date(source.date);
    target.set_type(source.type);
    target.set_vat_included(source.vat_included);
    target.set_ord_contract_id(source.ord_contract_id);
    target.set_ord_ado_id(source.ord_ado_id);
    target.set_subject_type(source.subject_type);
    target.set_action_type(source.action_type);
    target.set_agent_acting_for_publisher(source.agent_acting_for_publisher);
    target.set_parent_contract_id(source.parent_contract_id);
    target.set_client_id(source.client_id);
    target.set_client_name(source.client_name);
    target.set_client_legal_form(source.client_legal_form);
    target.set_contractor_id(source.contractor_id);
    target.set_contractor_name(source.contractor_name);
    target.set_contractor_legal_form(source.contractor_legal_form);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::ContractInfo& source, ContractInfo& target)
  {
    target.contract_id = source.contract_id();
    target.number = source.number();
    target.date = source.date();
    target.type = source.type();
    target.vat_included = source.vat_included();
    target.ord_contract_id = source.ord_contract_id();
    target.ord_ado_id = source.ord_ado_id();
    target.subject_type = source.subject_type();
    target.action_type = source.action_type();
    target.agent_acting_for_publisher = source.agent_acting_for_publisher();
    target.parent_contract_id = source.parent_contract_id();
    target.client_id = source.client_id();
    target.client_name = source.client_name();
    target.client_legal_form = source.client_legal_form();
    target.contractor_id = source.contractor_id();
    target.contractor_name = source.contractor_name();
    target.contractor_legal_form = source.contractor_legal_form();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const CampaignInfo& source, rpc::CampaignInfo& target)
  {
    target.Clear();
    target.set_campaign_id(source.campaign_id);
    target.set_campaign_group_id(source.campaign_group_id);
    target.set_ccg_rate_id(source.ccg_rate_id);
    target.set_ccg_rate_type(static_cast<unsigned char>(source.ccg_rate_type));
    target.set_fc_id(source.fc_id);
    target.set_group_fc_id(source.group_fc_id);
    target.set_priority(source.priority);
    target.set_flags(source.flags);
    target.set_marketplace(static_cast<unsigned char>(source.marketplace));
    pack(source.expression, *target.mutable_expression());
    pack(source.stat_expression, *target.mutable_stat_expression());
    target.set_country(source.country);

    for (std::uint32_t i = 0; i < source.sites.size(); ++i)
    {
      target.add_sites(source.sites[i]);
    }

    target.set_status(static_cast<unsigned char>(source.status));
    target.set_eval_status(static_cast<unsigned char>(source.eval_status));

    for (std::uint32_t i = 0; i < source.weekly_run_intervals.size(); ++i)
    {
      pack(source.weekly_run_intervals[i], *target.add_weekly_run_intervals());
    }

    for (std::uint32_t i = 0; i < source.creatives.size(); ++i)
    {
      pack(source.creatives[i], *target.add_creatives());
    }

    target.set_account_id(source.account_id);
    target.set_advertiser_id(source.advertiser_id);

    for (std::uint32_t i = 0; i < source.exclude_pub_accounts.size(); ++i)
    {
      target.add_exclude_pub_accounts(source.exclude_pub_accounts[i]);
    }

    for (std::uint32_t i = 0; i < source.exclude_tags.size(); ++i)
    {
      pack(source.exclude_tags[i], *target.add_exclude_tags());
    }

    target.set_delivery_coef(source.delivery_coef);
    target.set_imp_revenue(source.imp_revenue);
    target.set_click_revenue(source.click_revenue);
    target.set_action_revenue(source.action_revenue);
    target.set_commision(source.commision);
    target.set_ccg_type(static_cast<unsigned char>(source.ccg_type));
    target.set_target_type(static_cast<unsigned char>(source.target_type));
    pack(source.campaign_delivery_limits, *target.mutable_campaign_delivery_limits());
    pack(source.ccg_delivery_limits, *target.mutable_ccg_delivery_limits());
    target.set_start_user_group_id(source.start_user_group_id);
    target.set_end_user_group_id(source.end_user_group_id);
    target.set_max_pub_share(source.max_pub_share);
    target.set_ctr_reset_id(source.ctr_reset_id);
    target.set_random_imps(source.random_imps);
    target.set_mode(source.mode);
    target.set_seq_set_rotate_imps(source.seq_set_rotate_imps);
    target.set_min_uid_age(source.min_uid_age);

    for (std::uint32_t i = 0; i < source.colocations.size(); ++i)
    {
      target.add_colocations(source.colocations[i]);
    }

    target.set_bid_strategy(source.bid_strategy);
    target.set_min_ctr_goal(source.min_ctr_goal);
    target.set_timestamp(source.timestamp);
    target.set_min_vtr_goal(source.min_vtr_goal);
  }

  void unpack(const rpc::CampaignInfo& source, CampaignInfo& target)
  {
    target.campaign_id = source.campaign_id();
    target.campaign_group_id = source.campaign_group_id();
    target.ccg_rate_id = source.ccg_rate_id();

    if (source.ccg_rate_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.ccg_rate_type = source.ccg_rate_type();
    target.fc_id = source.fc_id();
    target.group_fc_id = source.group_fc_id();
    target.priority = source.priority();
    target.flags = source.flags();

    if (source.marketplace() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.marketplace = source.marketplace();
    unpack(source.expression(), target.expression);
    unpack(source.stat_expression(), target.stat_expression);
    target.country = source.country();
    target.sites.resize(source.sites_size());
    for (std::uint32_t i = 0; i < target.sites.size(); ++i)
    {
      target.sites[i] = source.sites(i);
    }

    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.status = source.status();

    if (source.eval_status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.eval_status = source.eval_status();
    target.weekly_run_intervals.resize(source.weekly_run_intervals_size());
    for (std::uint32_t i = 0; i < target.weekly_run_intervals.size(); ++i)
    {
      unpack(source.weekly_run_intervals(i), target.weekly_run_intervals[i]);
    }

    target.creatives.resize(source.creatives_size());
    for (std::uint32_t i = 0; i < target.creatives.size(); ++i)
    {
      unpack(source.creatives(i), target.creatives[i]);
    }

    target.account_id = source.account_id();
    target.advertiser_id = source.advertiser_id();
    target.exclude_pub_accounts.resize(source.exclude_pub_accounts_size());
    for (std::uint32_t i = 0; i < target.exclude_pub_accounts.size(); ++i)
    {
      target.exclude_pub_accounts[i] = source.exclude_pub_accounts(i);
    }

    target.exclude_tags.resize(source.exclude_tags_size());
    for (std::uint32_t i = 0; i < target.exclude_tags.size(); ++i)
    {
      unpack(source.exclude_tags(i), target.exclude_tags[i]);
    }

    target.delivery_coef = source.delivery_coef();
    unpack_bytes(source.imp_revenue(), target.imp_revenue, 9);
    unpack_bytes(source.click_revenue(), target.click_revenue, 9);
    unpack_bytes(source.action_revenue(), target.action_revenue, 9);
    unpack_bytes(source.commision(), target.commision, 9);

    if (source.ccg_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.ccg_type = source.ccg_type();

    if (source.target_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.target_type = source.target_type();
    unpack(source.campaign_delivery_limits(), target.campaign_delivery_limits);
    unpack(source.ccg_delivery_limits(), target.ccg_delivery_limits);
    target.start_user_group_id = source.start_user_group_id();
    target.end_user_group_id = source.end_user_group_id();
    unpack_bytes(source.max_pub_share(), target.max_pub_share, 9);
    target.ctr_reset_id = source.ctr_reset_id();
    target.random_imps = source.random_imps();
    target.mode = source.mode();
    target.seq_set_rotate_imps = source.seq_set_rotate_imps();
    unpack_bytes(source.min_uid_age(), target.min_uid_age, 8);
    target.colocations.resize(source.colocations_size());
    for (std::uint32_t i = 0; i < target.colocations.size(); ++i)
    {
      target.colocations[i] = source.colocations(i);
    }

    target.bid_strategy = source.bid_strategy();
    unpack_bytes(source.min_ctr_goal(), target.min_ctr_goal, 9);
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    unpack_bytes(source.min_vtr_goal(), target.min_vtr_goal, 9);
  }

  void pack(const TagPricingInfo& source, rpc::TagPricingInfo& target)
  {
    target.Clear();
    target.set_country_code(source.country_code);
    target.set_ccg_type(static_cast<unsigned char>(source.ccg_type));
    target.set_ccg_rate_type(static_cast<unsigned char>(source.ccg_rate_type));
    target.set_site_rate_id(source.site_rate_id);
    target.set_imp_revenue(source.imp_revenue);
    target.set_revenue_share(source.revenue_share);
  }

  void unpack(const rpc::TagPricingInfo& source, TagPricingInfo& target)
  {
    target.country_code = source.country_code();

    if (source.ccg_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.ccg_type = source.ccg_type();

    if (source.ccg_rate_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.ccg_rate_type = source.ccg_rate_type();
    target.site_rate_id = source.site_rate_id();
    unpack_bytes(source.imp_revenue(), target.imp_revenue, 9);
    unpack_bytes(source.revenue_share(), target.revenue_share, 9);
  }

  void pack(const TemplateOptionsInfo& source, rpc::TemplateOptionsInfo& target)
  {
    target.Clear();
    target.set_template_name(source.template_name);

    for (std::uint32_t i = 0; i < source.tokens.size(); ++i)
    {
      pack(source.tokens[i], *target.add_tokens());
    }
  }

  void unpack(const rpc::TemplateOptionsInfo& source, TemplateOptionsInfo& target)
  {
    target.template_name = source.template_name();
    target.tokens.resize(source.tokens_size());
    for (std::uint32_t i = 0; i < target.tokens.size(); ++i)
    {
      unpack(source.tokens(i), target.tokens[i]);
    }
  }

  void pack(const TagSizeInfo& source, rpc::TagSizeInfo& target)
  {
    target.Clear();
    target.set_size_id(source.size_id);
    target.set_max_text_creatives(source.max_text_creatives);

    for (std::uint32_t i = 0; i < source.tokens.size(); ++i)
    {
      pack(source.tokens[i], *target.add_tokens());
    }

    for (std::uint32_t i = 0; i < source.hidden_tokens.size(); ++i)
    {
      pack(source.hidden_tokens[i], *target.add_hidden_tokens());
    }
  }

  void unpack(const rpc::TagSizeInfo& source, TagSizeInfo& target)
  {
    target.size_id = source.size_id();
    target.max_text_creatives = source.max_text_creatives();
    target.tokens.resize(source.tokens_size());
    for (std::uint32_t i = 0; i < target.tokens.size(); ++i)
    {
      unpack(source.tokens(i), target.tokens[i]);
    }

    target.hidden_tokens.resize(source.hidden_tokens_size());
    for (std::uint32_t i = 0; i < target.hidden_tokens.size(); ++i)
    {
      unpack(source.hidden_tokens(i), target.hidden_tokens[i]);
    }
  }

  void pack(const TagInfo& source, rpc::TagInfo& target)
  {
    target.Clear();
    target.set_tag_id(source.tag_id);
    target.set_site_id(source.site_id);
    target.set_status(static_cast<unsigned char>(source.status));

    for (std::uint32_t i = 0; i < source.sizes.size(); ++i)
    {
      pack(source.sizes[i], *target.add_sizes());
    }

    target.set_imp_track_pixel(source.imp_track_pixel);
    target.set_passback(source.passback);
    target.set_passback_type(source.passback_type);
    target.set_flags(source.flags);
    target.set_marketplace(static_cast<unsigned char>(source.marketplace));
    target.set_adjustment(source.adjustment);

    for (std::uint32_t i = 0; i < source.tag_pricings.size(); ++i)
    {
      pack(source.tag_pricings[i], *target.add_tag_pricings());
    }

    for (std::uint32_t i = 0; i < source.accepted_categories.size(); ++i)
    {
      target.add_accepted_categories(source.accepted_categories[i]);
    }

    for (std::uint32_t i = 0; i < source.rejected_categories.size(); ++i)
    {
      target.add_rejected_categories(source.rejected_categories[i]);
    }

    target.set_allow_expandable(source.allow_expandable);

    for (std::uint32_t i = 0; i < source.tokens.size(); ++i)
    {
      pack(source.tokens[i], *target.add_tokens());
    }

    for (std::uint32_t i = 0; i < source.hidden_tokens.size(); ++i)
    {
      pack(source.hidden_tokens[i], *target.add_hidden_tokens());
    }

    for (std::uint32_t i = 0; i < source.passback_tokens.size(); ++i)
    {
      pack(source.passback_tokens[i], *target.add_passback_tokens());
    }

    for (std::uint32_t i = 0; i < source.template_tokens.size(); ++i)
    {
      pack(source.template_tokens[i], *target.add_template_tokens());
    }

    target.set_auction_max_ecpm_share(source.auction_max_ecpm_share);
    target.set_auction_prop_probability_share(source.auction_prop_probability_share);
    target.set_auction_random_share(source.auction_random_share);
    target.set_cost_coef(source.cost_coef);
    target.set_tag_pricings_timestamp(source.tag_pricings_timestamp);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::TagInfo& source, TagInfo& target)
  {
    target.tag_id = source.tag_id();
    target.site_id = source.site_id();

    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.status = source.status();
    target.sizes.resize(source.sizes_size());
    for (std::uint32_t i = 0; i < target.sizes.size(); ++i)
    {
      unpack(source.sizes(i), target.sizes[i]);
    }

    target.imp_track_pixel = source.imp_track_pixel();
    target.passback = source.passback();
    target.passback_type = source.passback_type();
    target.flags = source.flags();

    if (source.marketplace() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.marketplace = source.marketplace();
    unpack_bytes(source.adjustment(), target.adjustment, 9);
    target.tag_pricings.resize(source.tag_pricings_size());
    for (std::uint32_t i = 0; i < target.tag_pricings.size(); ++i)
    {
      unpack(source.tag_pricings(i), target.tag_pricings[i]);
    }

    target.accepted_categories.resize(source.accepted_categories_size());
    for (std::uint32_t i = 0; i < target.accepted_categories.size(); ++i)
    {
      target.accepted_categories[i] = source.accepted_categories(i);
    }

    target.rejected_categories.resize(source.rejected_categories_size());
    for (std::uint32_t i = 0; i < target.rejected_categories.size(); ++i)
    {
      target.rejected_categories[i] = source.rejected_categories(i);
    }

    target.allow_expandable = source.allow_expandable();
    target.tokens.resize(source.tokens_size());
    for (std::uint32_t i = 0; i < target.tokens.size(); ++i)
    {
      unpack(source.tokens(i), target.tokens[i]);
    }

    target.hidden_tokens.resize(source.hidden_tokens_size());
    for (std::uint32_t i = 0; i < target.hidden_tokens.size(); ++i)
    {
      unpack(source.hidden_tokens(i), target.hidden_tokens[i]);
    }

    target.passback_tokens.resize(source.passback_tokens_size());
    for (std::uint32_t i = 0; i < target.passback_tokens.size(); ++i)
    {
      unpack(source.passback_tokens(i), target.passback_tokens[i]);
    }

    target.template_tokens.resize(source.template_tokens_size());
    for (std::uint32_t i = 0; i < target.template_tokens.size(); ++i)
    {
      unpack(source.template_tokens(i), target.template_tokens[i]);
    }

    unpack_bytes(source.auction_max_ecpm_share(), target.auction_max_ecpm_share, 9);
    unpack_bytes(source.auction_prop_probability_share(), target.auction_prop_probability_share, 9);
    unpack_bytes(source.auction_random_share(), target.auction_random_share, 9);
    unpack_bytes(source.cost_coef(), target.cost_coef, 9);
    unpack_bytes(source.tag_pricings_timestamp(), target.tag_pricings_timestamp, 8);
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const SiteInfo& source, rpc::SiteInfo& target)
  {
    target.Clear();
    target.set_site_id(source.site_id);
    target.set_status(static_cast<unsigned char>(source.status));
    target.set_freq_cap_id(source.freq_cap_id);
    target.set_noads_timeout(source.noads_timeout);

    for (std::uint32_t i = 0; i < source.approved_creative_categories.size(); ++i)
    {
      target.add_approved_creative_categories(source.approved_creative_categories[i]);
    }

    for (std::uint32_t i = 0; i < source.rejected_creative_categories.size(); ++i)
    {
      target.add_rejected_creative_categories(source.rejected_creative_categories[i]);
    }

    for (std::uint32_t i = 0; i < source.approved_creatives.size(); ++i)
    {
      target.add_approved_creatives(source.approved_creatives[i]);
    }

    for (std::uint32_t i = 0; i < source.rejected_creatives.size(); ++i)
    {
      target.add_rejected_creatives(source.rejected_creatives[i]);
    }

    target.set_flags(source.flags);
    target.set_account_id(source.account_id);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::SiteInfo& source, SiteInfo& target)
  {
    target.site_id = source.site_id();

    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.status = source.status();
    target.freq_cap_id = source.freq_cap_id();
    target.noads_timeout = source.noads_timeout();
    target.approved_creative_categories.resize(source.approved_creative_categories_size());
    for (std::uint32_t i = 0; i < target.approved_creative_categories.size(); ++i)
    {
      target.approved_creative_categories[i] = source.approved_creative_categories(i);
    }

    target.rejected_creative_categories.resize(source.rejected_creative_categories_size());
    for (std::uint32_t i = 0; i < target.rejected_creative_categories.size(); ++i)
    {
      target.rejected_creative_categories[i] = source.rejected_creative_categories(i);
    }

    target.approved_creatives.resize(source.approved_creatives_size());
    for (std::uint32_t i = 0; i < target.approved_creatives.size(); ++i)
    {
      target.approved_creatives[i] = source.approved_creatives(i);
    }

    target.rejected_creatives.resize(source.rejected_creatives_size());
    for (std::uint32_t i = 0; i < target.rejected_creatives.size(); ++i)
    {
      target.rejected_creatives[i] = source.rejected_creatives(i);
    }

    target.flags = source.flags();
    target.account_id = source.account_id();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const ColocationInfo& source, rpc::ColocationInfo& target)
  {
    target.Clear();
    target.set_colo_id(source.colo_id);
    target.set_colo_name(source.colo_name);
    target.set_colo_rate_id(source.colo_rate_id);
    target.set_at_flags(source.at_flags);
    target.set_ad_serving(source.ad_serving);
    target.set_hid_profile(source.hid_profile);
    target.set_account_id(source.account_id);
    target.set_revenue_share(source.revenue_share);

    for (std::uint32_t i = 0; i < source.tokens.size(); ++i)
    {
      pack(source.tokens[i], *target.add_tokens());
    }

    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::ColocationInfo& source, ColocationInfo& target)
  {
    target.colo_id = source.colo_id();
    target.colo_name = source.colo_name();
    target.colo_rate_id = source.colo_rate_id();
    target.at_flags = source.at_flags();
    target.ad_serving = source.ad_serving();
    target.hid_profile = source.hid_profile();
    target.account_id = source.account_id();
    unpack_bytes(source.revenue_share(), target.revenue_share, 9);
    target.tokens.resize(source.tokens_size());
    for (std::uint32_t i = 0; i < target.tokens.size(); ++i)
    {
      unpack(source.tokens(i), target.tokens[i]);
    }

    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const ColocationFlags& source, rpc::ColocationFlags& target)
  {
    target.Clear();
    target.set_colo_id(source.colo_id);
    target.set_flags(source.flags);
    target.set_hid_profile(source.hid_profile);
  }

  void unpack(const rpc::ColocationFlags& source, ColocationFlags& target)
  {
    target.colo_id = source.colo_id();
    target.flags = source.flags();
    target.hid_profile = source.hid_profile();
  }

  void pack(const FreqCapInfo& source, rpc::FreqCapInfo& target)
  {
    target.Clear();
    target.set_fc_id(source.fc_id);
    target.set_lifelimit(source.lifelimit);
    target.set_period(source.period);
    target.set_window_limit(source.window_limit);
    target.set_window_time(source.window_time);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::FreqCapInfo& source, FreqCapInfo& target)
  {
    target.fc_id = source.fc_id();
    target.lifelimit = source.lifelimit();
    target.period = source.period();
    target.window_limit = source.window_limit();
    target.window_time = source.window_time();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const CreativeTemplateFileInfo& source, rpc::CreativeTemplateFileInfo& target)
  {
    target.Clear();
    target.set_creative_format(source.creative_format);
    target.set_creative_size(source.creative_size);
    target.set_app_format(source.app_format);
    target.set_mime_format(source.mime_format);
    target.set_track_impr(source.track_impr);
    target.set_type(static_cast<rpc::CreativeTemplateType>(source.type));
    target.set_template_file(source.template_file);
  }

  void unpack(const rpc::CreativeTemplateFileInfo& source, CreativeTemplateFileInfo& target)
  {
    target.creative_format = source.creative_format();
    target.creative_size = source.creative_size();
    target.app_format = source.app_format();
    target.mime_format = source.mime_format();
    target.track_impr = source.track_impr();

    if (!rpc::CreativeTemplateType_IsValid(source.type()))
    {
      throw std::invalid_argument("Invalid campaign enum");
    }

    target.type = static_cast<CreativeTemplateType>(source.type());
    target.template_file = source.template_file();
  }

  void pack(const CreativeTemplateInfo& source, rpc::CreativeTemplateInfo& target)
  {
    target.Clear();
    target.set_id(source.id);
    target.set_timestamp(source.timestamp);

    for (std::uint32_t i = 0; i < source.files.size(); ++i)
    {
      pack(source.files[i], *target.add_files());
    }

    for (std::uint32_t i = 0; i < source.tokens.size(); ++i)
    {
      pack(source.tokens[i], *target.add_tokens());
    }

    for (std::uint32_t i = 0; i < source.hidden_tokens.size(); ++i)
    {
      pack(source.hidden_tokens[i], *target.add_hidden_tokens());
    }
  }

  void unpack(const rpc::CreativeTemplateInfo& source, CreativeTemplateInfo& target)
  {
    target.id = source.id();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    target.files.resize(source.files_size());
    for (std::uint32_t i = 0; i < target.files.size(); ++i)
    {
      unpack(source.files(i), target.files[i]);
    }

    target.tokens.resize(source.tokens_size());
    for (std::uint32_t i = 0; i < target.tokens.size(); ++i)
    {
      unpack(source.tokens(i), target.tokens[i]);
    }

    target.hidden_tokens.resize(source.hidden_tokens_size());
    for (std::uint32_t i = 0; i < target.hidden_tokens.size(); ++i)
    {
      unpack(source.hidden_tokens(i), target.hidden_tokens[i]);
    }
  }

  void pack(const CurrencyInfo& source, rpc::CurrencyInfo& target)
  {
    target.Clear();
    target.set_rate(source.rate);
    target.set_currency_id(source.currency_id);
    target.set_currency_exchange_id(source.currency_exchange_id);
    target.set_effective_date(source.effective_date);
    target.set_fraction_digits(source.fraction_digits);
    target.set_currency_code(source.currency_code);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::CurrencyInfo& source, CurrencyInfo& target)
  {
    unpack_bytes(source.rate(), target.rate, 9);
    target.currency_id = source.currency_id();
    target.currency_exchange_id = source.currency_exchange_id();
    target.effective_date = source.effective_date();
    target.fraction_digits = source.fraction_digits();
    target.currency_code = source.currency_code();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const CampaignKeywordInfo& source, rpc::CampaignKeywordInfo& target)
  {
    target.Clear();
    target.set_ccg_keyword_id(source.ccg_keyword_id);
    target.set_original_keyword(source.original_keyword);
    target.set_click_url(source.click_url);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::CampaignKeywordInfo& source, CampaignKeywordInfo& target)
  {
    target.ccg_keyword_id = source.ccg_keyword_id();
    target.original_keyword = source.original_keyword();
    target.click_url = source.click_url();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const AdvActionInfo& source, rpc::AdvActionInfo& target)
  {
    target.Clear();
    target.set_action_id(source.action_id);
    target.set_timestamp(source.timestamp);

    for (std::uint32_t i = 0; i < source.ccg_ids.size(); ++i)
    {
      target.add_ccg_ids(source.ccg_ids[i]);
    }
  }

  void unpack(const rpc::AdvActionInfo& source, AdvActionInfo& target)
  {
    target.action_id = source.action_id();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    target.ccg_ids.resize(source.ccg_ids_size());
    for (std::uint32_t i = 0; i < target.ccg_ids.size(); ++i)
    {
      target.ccg_ids[i] = source.ccg_ids(i);
    }
  }

  void pack(const AccountInfo& source, rpc::AccountInfo& target)
  {
    target.Clear();
    target.set_account_id(source.account_id);
    target.set_agency_account_id(source.agency_account_id);
    target.set_internal_account_id(source.internal_account_id);
    target.set_role_id(source.role_id);
    target.set_legal_name(source.legal_name);
    target.set_flags(source.flags);
    target.set_at_flags(source.at_flags);
    target.set_text_adserving(static_cast<unsigned char>(source.text_adserving));
    target.set_currency_id(source.currency_id);
    target.set_country(source.country);
    target.set_time_offset(source.time_offset);
    target.set_commision(source.commision);
    target.set_budget(source.budget);
    target.set_paid_amount(source.paid_amount);

    for (std::uint32_t i = 0; i < source.walled_garden_accounts.size(); ++i)
    {
      target.add_walled_garden_accounts(source.walled_garden_accounts[i]);
    }

    target.set_auction_rate(source.auction_rate);
    target.set_use_pub_pixels(source.use_pub_pixels);
    target.set_pub_pixel_optin(source.pub_pixel_optin);
    target.set_pub_pixel_optout(source.pub_pixel_optout);
    target.set_self_service_commission(source.self_service_commission);
    target.set_status(static_cast<unsigned char>(source.status));
    target.set_eval_status(static_cast<unsigned char>(source.eval_status));
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::AccountInfo& source, AccountInfo& target)
  {
    target.account_id = source.account_id();
    target.agency_account_id = source.agency_account_id();
    target.internal_account_id = source.internal_account_id();
    target.role_id = source.role_id();
    target.legal_name = source.legal_name();
    target.flags = source.flags();
    target.at_flags = source.at_flags();

    if (source.text_adserving() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.text_adserving = source.text_adserving();
    target.currency_id = source.currency_id();
    target.country = source.country();
    unpack_bytes(source.time_offset(), target.time_offset, 8);
    unpack_bytes(source.commision(), target.commision, 9);
    unpack_bytes(source.budget(), target.budget, 9);
    unpack_bytes(source.paid_amount(), target.paid_amount, 9);
    target.walled_garden_accounts.resize(source.walled_garden_accounts_size());
    for (std::uint32_t i = 0; i < target.walled_garden_accounts.size(); ++i)
    {
      target.walled_garden_accounts[i] = source.walled_garden_accounts(i);
    }

    target.auction_rate = source.auction_rate();
    target.use_pub_pixels = source.use_pub_pixels();
    target.pub_pixel_optin = source.pub_pixel_optin();
    target.pub_pixel_optout = source.pub_pixel_optout();
    unpack_bytes(source.self_service_commission(), target.self_service_commission, 9);

    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.status = source.status();

    if (source.eval_status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.eval_status = source.eval_status();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(
    const CategoryChannelLocalizationInfo& source, rpc::CategoryChannelLocalizationInfo& target)
  {
    target.Clear();
    target.set_language(source.language);
    target.set_name(source.name);
  }

  void unpack(
    const rpc::CategoryChannelLocalizationInfo& source, CategoryChannelLocalizationInfo& target)
  {
    target.language = source.language();
    target.name = source.name();
  }

  void pack(const CategoryChannelInfo& source, rpc::CategoryChannelInfo& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);
    target.set_name(source.name);
    target.set_newsgate_name(source.newsgate_name);

    for (std::uint32_t i = 0; i < source.localizations.size(); ++i)
    {
      pack(source.localizations[i], *target.add_localizations());
    }

    target.set_parent_channel_id(source.parent_channel_id);
    target.set_flags(source.flags);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::CategoryChannelInfo& source, CategoryChannelInfo& target)
  {
    target.channel_id = source.channel_id();
    target.name = source.name();
    target.newsgate_name = source.newsgate_name();
    target.localizations.resize(source.localizations_size());
    for (std::uint32_t i = 0; i < target.localizations.size(); ++i)
    {
      unpack(source.localizations(i), target.localizations[i]);
    }

    target.parent_channel_id = source.parent_channel_id();
    target.flags = source.flags();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const PriceRangeInfo& source, rpc::PriceRange& target)
  {
    target.Clear();
    target.set_min(source.min);
    target.set_max(source.max);
  }

  void unpack(const rpc::PriceRange& source, PriceRangeInfo& target)
  {
    target.min = source.min();
    target.max = source.max();
  }

  void pack(const BehavParameter& source, rpc::BehavParameter& target)
  {
    target.Clear();
    target.set_min_visits(source.min_visits);
    target.set_time_from(source.time_from);
    target.set_time_to(source.time_to);
    target.set_weight(source.weight);
    target.set_trigger_type(static_cast<unsigned char>(source.trigger_type));
  }

  void unpack(const rpc::BehavParameter& source, BehavParameter& target)
  {
    target.min_visits = source.min_visits();
    target.time_from = source.time_from();
    target.time_to = source.time_to();
    target.weight = source.weight();

    if (source.trigger_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.trigger_type = source.trigger_type();
  }

  void pack(const BehavParamInfo& source, rpc::BehavParamInfo& target)
  {
    target.Clear();
    target.set_id(source.id);
    target.set_threshold(source.threshold);
    target.set_timestamp(source.timestamp);

    for (std::uint32_t i = 0; i < source.bp_seq.size(); ++i)
    {
      pack(source.bp_seq[i], *target.add_bp_seq());
    }
  }

  void unpack(const rpc::BehavParamInfo& source, BehavParamInfo& target)
  {
    target.id = source.id();
    target.threshold = source.threshold();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    target.bp_seq.resize(source.bp_seq_size());
    for (std::uint32_t i = 0; i < target.bp_seq.size(); ++i)
    {
      unpack(source.bp_seq(i), target.bp_seq[i]);
    }
  }

  void pack(const KeyBehavParamInfo& source, rpc::KeyBehavParamInfo& target)
  {
    target.Clear();
    target.set_id(source.id);
    target.set_threshold(source.threshold);
    target.set_timestamp(source.timestamp);

    for (std::uint32_t i = 0; i < source.bp_seq.size(); ++i)
    {
      pack(source.bp_seq[i], *target.add_bp_seq());
    }
  }

  void unpack(const rpc::KeyBehavParamInfo& source, KeyBehavParamInfo& target)
  {
    target.id = source.id();
    target.threshold = source.threshold();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    target.bp_seq.resize(source.bp_seq_size());
    for (std::uint32_t i = 0; i < target.bp_seq.size(); ++i)
    {
      unpack(source.bp_seq(i), target.bp_seq[i]);
    }
  }

  void pack(const BriefBehavParamInfo& source, rpc::BriefBehavParamInfo& target)
  {
    target.Clear();
    target.set_id(source.id);
    target.set_threshold(source.threshold);
    target.set_timestamp(source.timestamp);

    for (std::uint32_t i = 0; i < source.bp_seq.size(); ++i)
    {
      pack(source.bp_seq[i], *target.add_bp_seq());
    }
  }

  void unpack(const rpc::BriefBehavParamInfo& source, BriefBehavParamInfo& target)
  {
    target.id = source.id();
    target.threshold = source.threshold();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    target.bp_seq.resize(source.bp_seq_size());
    for (std::uint32_t i = 0; i < target.bp_seq.size(); ++i)
    {
      unpack(source.bp_seq(i), target.bp_seq[i]);
    }
  }

  void pack(const BriefKeyBehavParamInfo& source, rpc::BriefKeyBehavParamInfo& target)
  {
    target.Clear();
    target.set_id(source.id);
    target.set_threshold(source.threshold);
    target.set_timestamp(source.timestamp);

    for (std::uint32_t i = 0; i < source.bp_seq.size(); ++i)
    {
      pack(source.bp_seq[i], *target.add_bp_seq());
    }
  }

  void unpack(const rpc::BriefKeyBehavParamInfo& source, BriefKeyBehavParamInfo& target)
  {
    target.id = source.id();
    target.threshold = source.threshold();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    target.bp_seq.resize(source.bp_seq_size());
    for (std::uint32_t i = 0; i < target.bp_seq.size(); ++i)
    {
      unpack(source.bp_seq(i), target.bp_seq[i]);
    }
  }

  void pack(const SimpleChannelKey& source, rpc::SimpleChannelKey& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);
    target.set_country_code(source.country_code);
    target.set_language(source.language);
    target.set_status(static_cast<unsigned char>(source.status));
    target.set_behav_param_list_id(source.behav_param_list_id);
    target.set_str_behav_param_list_id(source.str_behav_param_list_id);

    for (std::uint32_t i = 0; i < source.categories.size(); ++i)
    {
      target.add_categories(source.categories[i]);
    }

    target.set_threshold(source.threshold);
    target.set_discover(source.discover);

    for (std::uint32_t i = 0; i < source.page_triggers.size(); ++i)
    {
      target.add_page_triggers(source.page_triggers[i]);
    }

    for (std::uint32_t i = 0; i < source.search_triggers.size(); ++i)
    {
      target.add_search_triggers(source.search_triggers[i]);
    }

    for (std::uint32_t i = 0; i < source.url_triggers.size(); ++i)
    {
      target.add_url_triggers(source.url_triggers[i]);
    }

    for (std::uint32_t i = 0; i < source.url_keyword_triggers.size(); ++i)
    {
      target.add_url_keyword_triggers(source.url_keyword_triggers[i]);
    }

    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::SimpleChannelKey& source, SimpleChannelKey& target)
  {
    target.channel_id = source.channel_id();
    target.country_code = source.country_code();
    target.language = source.language();

    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.status = source.status();
    target.behav_param_list_id = source.behav_param_list_id();
    target.str_behav_param_list_id = source.str_behav_param_list_id();
    target.categories.resize(source.categories_size());
    for (std::uint32_t i = 0; i < target.categories.size(); ++i)
    {
      target.categories[i] = source.categories(i);
    }

    target.threshold = source.threshold();
    target.discover = source.discover();
    target.page_triggers.resize(source.page_triggers_size());
    for (std::uint32_t i = 0; i < target.page_triggers.size(); ++i)
    {
      target.page_triggers[i] = source.page_triggers(i);
    }

    target.search_triggers.resize(source.search_triggers_size());
    for (std::uint32_t i = 0; i < target.search_triggers.size(); ++i)
    {
      target.search_triggers[i] = source.search_triggers(i);
    }

    target.url_triggers.resize(source.url_triggers_size());
    for (std::uint32_t i = 0; i < target.url_triggers.size(); ++i)
    {
      target.url_triggers[i] = source.url_triggers(i);
    }

    target.url_keyword_triggers.resize(source.url_keyword_triggers_size());
    for (std::uint32_t i = 0; i < target.url_keyword_triggers.size(); ++i)
    {
      target.url_keyword_triggers[i] = source.url_keyword_triggers(i);
    }

    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const BriefSimpleChannelKey& source, rpc::BriefSimpleChannelKey& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);
    target.set_behav_param_list_id(source.behav_param_list_id);
    target.set_str_behav_param_list_id(source.str_behav_param_list_id);

    for (std::uint32_t i = 0; i < source.categories.size(); ++i)
    {
      target.add_categories(source.categories[i]);
    }

    target.set_threshold(source.threshold);
    target.set_discover(source.discover);
  }

  void unpack(const rpc::BriefSimpleChannelKey& source, BriefSimpleChannelKey& target)
  {
    target.channel_id = source.channel_id();
    target.behav_param_list_id = source.behav_param_list_id();
    target.str_behav_param_list_id = source.str_behav_param_list_id();
    target.categories.resize(source.categories_size());
    for (std::uint32_t i = 0; i < target.categories.size(); ++i)
    {
      target.categories[i] = source.categories(i);
    }

    target.threshold = source.threshold();
    target.discover = source.discover();
  }

  void pack(const GeoIPRegionCityInfo& source, rpc::GeoIPRegionCityInfo& target)
  {
    target.Clear();
    target.set_region(source.region);
    target.set_city(source.city);
  }

  void unpack(const rpc::GeoIPRegionCityInfo& source, GeoIPRegionCityInfo& target)
  {
    target.region = source.region();
    target.city = source.city();
  }

  void pack(const GeoChannelInfo& source, rpc::GeoChannelInfo& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);
    target.set_country(source.country);

    for (std::uint32_t i = 0; i < source.geoip_targets.size(); ++i)
    {
      pack(source.geoip_targets[i], *target.add_geoip_targets());
    }

    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::GeoChannelInfo& source, GeoChannelInfo& target)
  {
    target.channel_id = source.channel_id();
    target.country = source.country();
    target.geoip_targets.resize(source.geoip_targets_size());
    for (std::uint32_t i = 0; i < target.geoip_targets.size(); ++i)
    {
      unpack(source.geoip_targets(i), target.geoip_targets[i]);
    }

    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const GeoCoordChannelInfo& source, rpc::GeoCoordChannelInfo& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);
    target.set_longitude(source.longitude);
    target.set_latitude(source.latitude);
    target.set_radius(source.radius);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::GeoCoordChannelInfo& source, GeoCoordChannelInfo& target)
  {
    target.channel_id = source.channel_id();
    unpack_bytes(source.longitude(), target.longitude, 9);
    unpack_bytes(source.latitude(), target.latitude, 9);
    unpack_bytes(source.radius(), target.radius, 9);
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const BlockChannelInfo& source, rpc::BlockChannelInfo& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);
    target.set_size_id(source.size_id);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::BlockChannelInfo& source, BlockChannelInfo& target)
  {
    target.channel_id = source.channel_id();
    target.size_id = source.size_id();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const FraudConditionInfo& source, rpc::FraudConditionInfo& target)
  {
    target.Clear();
    target.set_id(source.id);
    target.set_type(static_cast<unsigned char>(source.type));
    target.set_limit(source.limit);
    target.set_period(source.period);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::FraudConditionInfo& source, FraudConditionInfo& target)
  {
    target.id = source.id();

    if (source.type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.type = source.type();
    target.limit = source.limit();
    unpack_bytes(source.period(), target.period, 8);
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const SearchEngineRegExpInfo& source, rpc::SearchEngineRegExpInfo& target)
  {
    target.Clear();
    target.set_host_postfix(source.host_postfix);
    target.set_regexp(source.regexp);
    target.set_encoding(source.encoding);
    target.set_post_encoding(source.post_encoding);
    target.set_decoding_depth(source.decoding_depth);
  }

  void unpack(const rpc::SearchEngineRegExpInfo& source, SearchEngineRegExpInfo& target)
  {
    target.host_postfix = source.host_postfix();
    target.regexp = source.regexp();
    target.encoding = source.encoding();
    target.post_encoding = source.post_encoding();
    target.decoding_depth = source.decoding_depth();
  }

  void pack(const SearchEngineInfo& source, rpc::SearchEngineInfo& target)
  {
    target.Clear();
    target.set_id(source.id);
    target.set_name(source.name);

    for (std::uint32_t i = 0; i < source.regexps.size(); ++i)
    {
      pack(source.regexps[i], *target.add_regexps());
    }

    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::SearchEngineInfo& source, SearchEngineInfo& target)
  {
    target.id = source.id();
    target.name = source.name();
    target.regexps.resize(source.regexps_size());
    for (std::uint32_t i = 0; i < target.regexps.size(); ++i)
    {
      unpack(source.regexps(i), target.regexps[i]);
    }

    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const WebBrowserDetectorInfo& source, rpc::WebBrowserDetectorInfo& target)
  {
    target.Clear();
    target.set_marker(source.marker);
    target.set_regexp(source.regexp);
    target.set_regexp_required(source.regexp_required);
    target.set_priority(source.priority);
  }

  void unpack(const rpc::WebBrowserDetectorInfo& source, WebBrowserDetectorInfo& target)
  {
    target.marker = source.marker();
    target.regexp = source.regexp();
    target.regexp_required = source.regexp_required();
    target.priority = source.priority();
  }

  void pack(const WebBrowserInfo& source, rpc::WebBrowserInfo& target)
  {
    target.Clear();
    target.set_name(source.name);

    for (std::uint32_t i = 0; i < source.detectors.size(); ++i)
    {
      pack(source.detectors[i], *target.add_detectors());
    }

    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::WebBrowserInfo& source, WebBrowserInfo& target)
  {
    target.name = source.name();
    target.detectors.resize(source.detectors_size());
    for (std::uint32_t i = 0; i < target.detectors.size(); ++i)
    {
      unpack(source.detectors(i), target.detectors[i]);
    }

    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const PlatformDetectorInfo& source, rpc::PlatformDetectorInfo& target)
  {
    target.Clear();
    target.set_use_name(source.use_name);
    target.set_marker(source.marker);
    target.set_match_regexp(source.match_regexp);
    target.set_output_regexp(source.output_regexp);
    target.set_priority(source.priority);
  }

  void unpack(const rpc::PlatformDetectorInfo& source, PlatformDetectorInfo& target)
  {
    target.use_name = source.use_name();
    target.marker = source.marker();
    target.match_regexp = source.match_regexp();
    target.output_regexp = source.output_regexp();
    target.priority = source.priority();
  }

  void pack(const PlatformInfo& source, rpc::PlatformInfo& target)
  {
    target.Clear();
    target.set_platform_id(source.platform_id);
    target.set_name(source.name);
    target.set_type(source.type);

    for (std::uint32_t i = 0; i < source.detectors.size(); ++i)
    {
      pack(source.detectors[i], *target.add_detectors());
    }

    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::PlatformInfo& source, PlatformInfo& target)
  {
    target.platform_id = source.platform_id();
    target.name = source.name();
    target.type = source.type();
    target.detectors.resize(source.detectors_size());
    for (std::uint32_t i = 0; i < target.detectors.size(); ++i)
    {
      unpack(source.detectors(i), target.detectors[i]);
    }

    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const WebOperationInfo& source, rpc::WebOperationInfo& target)
  {
    target.Clear();
    target.set_id(source.id);
    target.set_app(source.app);
    target.set_source(source.source);
    target.set_operation(source.operation);
    target.set_flags(source.flags);
    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::WebOperationInfo& source, WebOperationInfo& target)
  {
    target.id = source.id();
    target.app = source.app();
    target.source = source.source();
    target.operation = source.operation();
    target.flags = source.flags();
    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const CampaignConfigUpdateInfo& source, rpc::CampaignConfigUpdateInfo& target)
  {
    target.Clear();
    target.set_server_id(source.server_id);
    target.set_master_stamp(source.master_stamp);
    target.set_first_load_stamp(source.first_load_stamp);
    target.set_finish_load_stamp(source.finish_load_stamp);
    target.set_current_time(source.current_time);
    target.set_global_freq_cap_id(source.global_freq_cap_id);
    target.set_currency_exchange_id(source.currency_exchange_id);
    target.set_max_keyword_ecpm(source.max_keyword_ecpm);
    target.set_google_publisher_account_id(source.google_publisher_account_id);
    target.set_fraud_user_deactivate_period(source.fraud_user_deactivate_period);
    target.set_cost_limit(source.cost_limit);
    target.set_global_params_timestamp(source.global_params_timestamp);

    for (std::uint32_t i = 0; i < source.app_formats.size(); ++i)
    {
      pack(source.app_formats[i], *target.add_app_formats());
    }

    for (std::uint32_t i = 0; i < source.delete_app_formats.size(); ++i)
    {
      pack(source.delete_app_formats[i], *target.add_delete_app_formats());
    }

    for (std::uint32_t i = 0; i < source.sizes.size(); ++i)
    {
      pack(source.sizes[i], *target.add_sizes());
    }

    for (std::uint32_t i = 0; i < source.delete_sizes.size(); ++i)
    {
      pack(source.delete_sizes[i], *target.add_delete_sizes());
    }

    for (std::uint32_t i = 0; i < source.countries.size(); ++i)
    {
      pack(source.countries[i], *target.add_countries());
    }

    for (std::uint32_t i = 0; i < source.deleted_countries.size(); ++i)
    {
      pack(source.deleted_countries[i], *target.add_deleted_countries());
    }

    for (std::uint32_t i = 0; i < source.accounts.size(); ++i)
    {
      pack(source.accounts[i], *target.add_accounts());
    }

    for (std::uint32_t i = 0; i < source.deleted_accounts.size(); ++i)
    {
      pack(source.deleted_accounts[i], *target.add_deleted_accounts());
    }

    for (std::uint32_t i = 0; i < source.activate_creative_options.size(); ++i)
    {
      pack(source.activate_creative_options[i], *target.add_activate_creative_options());
    }

    for (std::uint32_t i = 0; i < source.delete_creative_options.size(); ++i)
    {
      pack(source.delete_creative_options[i], *target.add_delete_creative_options());
    }

    for (std::uint32_t i = 0; i < source.campaigns.size(); ++i)
    {
      pack(source.campaigns[i], *target.add_campaigns());
    }

    for (std::uint32_t i = 0; i < source.deleted_campaigns.size(); ++i)
    {
      pack(source.deleted_campaigns[i], *target.add_deleted_campaigns());
    }

    for (std::uint32_t i = 0; i < source.ecpms.size(); ++i)
    {
      pack(source.ecpms[i], *target.add_ecpms());
    }

    for (std::uint32_t i = 0; i < source.deleted_ecpms.size(); ++i)
    {
      pack(source.deleted_ecpms[i], *target.add_deleted_ecpms());
    }

    for (std::uint32_t i = 0; i < source.sites.size(); ++i)
    {
      pack(source.sites[i], *target.add_sites());
    }

    for (std::uint32_t i = 0; i < source.deleted_sites.size(); ++i)
    {
      pack(source.deleted_sites[i], *target.add_deleted_sites());
    }

    for (std::uint32_t i = 0; i < source.tags.size(); ++i)
    {
      pack(source.tags[i], *target.add_tags());
    }

    for (std::uint32_t i = 0; i < source.deleted_tags.size(); ++i)
    {
      pack(source.deleted_tags[i], *target.add_deleted_tags());
    }

    for (std::uint32_t i = 0; i < source.frequency_caps.size(); ++i)
    {
      pack(source.frequency_caps[i], *target.add_frequency_caps());
    }

    for (std::uint32_t i = 0; i < source.deleted_freq_caps.size(); ++i)
    {
      pack(source.deleted_freq_caps[i], *target.add_deleted_freq_caps());
    }

    for (std::uint32_t i = 0; i < source.simple_channels.size(); ++i)
    {
      pack(source.simple_channels[i], *target.add_simple_channels());
    }

    for (std::uint32_t i = 0; i < source.deleted_simple_channels.size(); ++i)
    {
      pack(source.deleted_simple_channels[i], *target.add_deleted_simple_channels());
    }

    target.set_geo_channels_timestamp(source.geo_channels_timestamp);

    for (std::uint32_t i = 0; i < source.activate_geo_channels.size(); ++i)
    {
      pack(source.activate_geo_channels[i], *target.add_activate_geo_channels());
    }

    for (std::uint32_t i = 0; i < source.delete_geo_channels.size(); ++i)
    {
      pack(source.delete_geo_channels[i], *target.add_delete_geo_channels());
    }

    for (std::uint32_t i = 0; i < source.activate_geo_coord_channels.size(); ++i)
    {
      pack(source.activate_geo_coord_channels[i], *target.add_activate_geo_coord_channels());
    }

    for (std::uint32_t i = 0; i < source.delete_geo_coord_channels.size(); ++i)
    {
      pack(source.delete_geo_coord_channels[i], *target.add_delete_geo_coord_channels());
    }

    for (std::uint32_t i = 0; i < source.activate_block_channels.size(); ++i)
    {
      pack(source.activate_block_channels[i], *target.add_activate_block_channels());
    }

    for (std::uint32_t i = 0; i < source.delete_block_channels.size(); ++i)
    {
      pack(source.delete_block_channels[i], *target.add_delete_block_channels());
    }

    for (std::uint32_t i = 0; i < source.colocations.size(); ++i)
    {
      pack(source.colocations[i], *target.add_colocations());
    }

    for (std::uint32_t i = 0; i < source.deleted_colocations.size(); ++i)
    {
      pack(source.deleted_colocations[i], *target.add_deleted_colocations());
    }

    for (std::uint32_t i = 0; i < source.creative_templates.size(); ++i)
    {
      pack(source.creative_templates[i], *target.add_creative_templates());
    }

    for (std::uint32_t i = 0; i < source.deleted_templates.size(); ++i)
    {
      pack(source.deleted_templates[i], *target.add_deleted_templates());
    }

    for (std::uint32_t i = 0; i < source.currencies.size(); ++i)
    {
      pack(source.currencies[i], *target.add_currencies());
    }

    for (std::uint32_t i = 0; i < source.expression_channels.size(); ++i)
    {
      pack(source.expression_channels[i], *target.add_expression_channels());
    }

    for (std::uint32_t i = 0; i < source.deleted_expression_channels.size(); ++i)
    {
      pack(source.deleted_expression_channels[i], *target.add_deleted_expression_channels());
    }

    for (std::uint32_t i = 0; i < source.campaign_keywords.size(); ++i)
    {
      pack(source.campaign_keywords[i], *target.add_campaign_keywords());
    }

    for (std::uint32_t i = 0; i < source.deleted_keywords.size(); ++i)
    {
      pack(source.deleted_keywords[i], *target.add_deleted_keywords());
    }

    for (std::uint32_t i = 0; i < source.creative_categories.size(); ++i)
    {
      pack(source.creative_categories[i], *target.add_creative_categories());
    }

    for (std::uint32_t i = 0; i < source.deleted_creative_categories.size(); ++i)
    {
      pack(source.deleted_creative_categories[i], *target.add_deleted_creative_categories());
    }

    for (std::uint32_t i = 0; i < source.adv_actions.size(); ++i)
    {
      pack(source.adv_actions[i], *target.add_adv_actions());
    }

    for (std::uint32_t i = 0; i < source.deleted_adv_actions.size(); ++i)
    {
      pack(source.deleted_adv_actions[i], *target.add_deleted_adv_actions());
    }

    for (std::uint32_t i = 0; i < source.category_channels.size(); ++i)
    {
      pack(source.category_channels[i], *target.add_category_channels());
    }

    for (std::uint32_t i = 0; i < source.deleted_category_channels.size(); ++i)
    {
      pack(source.deleted_category_channels[i], *target.add_deleted_category_channels());
    }

    for (std::uint32_t i = 0; i < source.behav_params.size(); ++i)
    {
      pack(source.behav_params[i], *target.add_behav_params());
    }

    for (std::uint32_t i = 0; i < source.deleted_behav_params.size(); ++i)
    {
      pack(source.deleted_behav_params[i], *target.add_deleted_behav_params());
    }

    for (std::uint32_t i = 0; i < source.key_behav_params.size(); ++i)
    {
      pack(source.key_behav_params[i], *target.add_key_behav_params());
    }

    for (std::uint32_t i = 0; i < source.deleted_key_behav_params.size(); ++i)
    {
      pack(source.deleted_key_behav_params[i], *target.add_deleted_key_behav_params());
    }

    for (std::uint32_t i = 0; i < source.fraud_conditions.size(); ++i)
    {
      pack(source.fraud_conditions[i], *target.add_fraud_conditions());
    }

    for (std::uint32_t i = 0; i < source.deleted_fraud_conditions.size(); ++i)
    {
      pack(source.deleted_fraud_conditions[i], *target.add_deleted_fraud_conditions());
    }

    for (std::uint32_t i = 0; i < source.search_engines.size(); ++i)
    {
      pack(source.search_engines[i], *target.add_search_engines());
    }

    for (std::uint32_t i = 0; i < source.deleted_search_engines.size(); ++i)
    {
      pack(source.deleted_search_engines[i], *target.add_deleted_search_engines());
    }

    for (std::uint32_t i = 0; i < source.web_browsers.size(); ++i)
    {
      pack(source.web_browsers[i], *target.add_web_browsers());
    }

    for (std::uint32_t i = 0; i < source.deleted_web_browsers.size(); ++i)
    {
      pack(source.deleted_web_browsers[i], *target.add_deleted_web_browsers());
    }

    for (std::uint32_t i = 0; i < source.platforms.size(); ++i)
    {
      pack(source.platforms[i], *target.add_platforms());
    }

    for (std::uint32_t i = 0; i < source.deleted_platforms.size(); ++i)
    {
      pack(source.deleted_platforms[i], *target.add_deleted_platforms());
    }

    for (std::uint32_t i = 0; i < source.web_operations.size(); ++i)
    {
      pack(source.web_operations[i], *target.add_web_operations());
    }

    for (std::uint32_t i = 0; i < source.delete_web_operations.size(); ++i)
    {
      pack(source.delete_web_operations[i], *target.add_delete_web_operations());
    }

    for (std::uint32_t i = 0; i < source.contracts.size(); ++i)
    {
      pack(source.contracts[i], *target.add_contracts());
    }

    for (std::uint32_t i = 0; i < source.deleted_contracts.size(); ++i)
    {
      pack(source.deleted_contracts[i], *target.add_deleted_contracts());
    }
  }

  void unpack(const rpc::CampaignConfigUpdateInfo& source, CampaignConfigUpdateInfo& target)
  {
    target.server_id = source.server_id();
    unpack_bytes(source.master_stamp(), target.master_stamp, 8);
    unpack_bytes(source.first_load_stamp(), target.first_load_stamp, 8);
    unpack_bytes(source.finish_load_stamp(), target.finish_load_stamp, 8);
    unpack_bytes(source.current_time(), target.current_time, 8);
    target.global_freq_cap_id = source.global_freq_cap_id();
    target.currency_exchange_id = source.currency_exchange_id();
    target.max_keyword_ecpm = source.max_keyword_ecpm();
    target.google_publisher_account_id = source.google_publisher_account_id();
    unpack_bytes(source.fraud_user_deactivate_period(), target.fraud_user_deactivate_period, 8);
    unpack_bytes(source.cost_limit(), target.cost_limit, 9);
    unpack_bytes(source.global_params_timestamp(), target.global_params_timestamp, 8);
    target.app_formats.resize(source.app_formats_size());
    for (std::uint32_t i = 0; i < target.app_formats.size(); ++i)
    {
      unpack(source.app_formats(i), target.app_formats[i]);
    }

    target.delete_app_formats.resize(source.delete_app_formats_size());
    for (std::uint32_t i = 0; i < target.delete_app_formats.size(); ++i)
    {
      unpack(source.delete_app_formats(i), target.delete_app_formats[i]);
    }

    target.sizes.resize(source.sizes_size());
    for (std::uint32_t i = 0; i < target.sizes.size(); ++i)
    {
      unpack(source.sizes(i), target.sizes[i]);
    }

    target.delete_sizes.resize(source.delete_sizes_size());
    for (std::uint32_t i = 0; i < target.delete_sizes.size(); ++i)
    {
      unpack(source.delete_sizes(i), target.delete_sizes[i]);
    }

    target.countries.resize(source.countries_size());
    for (std::uint32_t i = 0; i < target.countries.size(); ++i)
    {
      unpack(source.countries(i), target.countries[i]);
    }

    target.deleted_countries.resize(source.deleted_countries_size());
    for (std::uint32_t i = 0; i < target.deleted_countries.size(); ++i)
    {
      unpack(source.deleted_countries(i), target.deleted_countries[i]);
    }

    target.accounts.resize(source.accounts_size());
    for (std::uint32_t i = 0; i < target.accounts.size(); ++i)
    {
      unpack(source.accounts(i), target.accounts[i]);
    }

    target.deleted_accounts.resize(source.deleted_accounts_size());
    for (std::uint32_t i = 0; i < target.deleted_accounts.size(); ++i)
    {
      unpack(source.deleted_accounts(i), target.deleted_accounts[i]);
    }

    target.activate_creative_options.resize(source.activate_creative_options_size());
    for (std::uint32_t i = 0; i < target.activate_creative_options.size(); ++i)
    {
      unpack(source.activate_creative_options(i), target.activate_creative_options[i]);
    }

    target.delete_creative_options.resize(source.delete_creative_options_size());
    for (std::uint32_t i = 0; i < target.delete_creative_options.size(); ++i)
    {
      unpack(source.delete_creative_options(i), target.delete_creative_options[i]);
    }

    target.campaigns.resize(source.campaigns_size());
    for (std::uint32_t i = 0; i < target.campaigns.size(); ++i)
    {
      unpack(source.campaigns(i), target.campaigns[i]);
    }

    target.deleted_campaigns.resize(source.deleted_campaigns_size());
    for (std::uint32_t i = 0; i < target.deleted_campaigns.size(); ++i)
    {
      unpack(source.deleted_campaigns(i), target.deleted_campaigns[i]);
    }

    target.ecpms.resize(source.ecpms_size());
    for (std::uint32_t i = 0; i < target.ecpms.size(); ++i)
    {
      unpack(source.ecpms(i), target.ecpms[i]);
    }

    target.deleted_ecpms.resize(source.deleted_ecpms_size());
    for (std::uint32_t i = 0; i < target.deleted_ecpms.size(); ++i)
    {
      unpack(source.deleted_ecpms(i), target.deleted_ecpms[i]);
    }

    target.sites.resize(source.sites_size());
    for (std::uint32_t i = 0; i < target.sites.size(); ++i)
    {
      unpack(source.sites(i), target.sites[i]);
    }

    target.deleted_sites.resize(source.deleted_sites_size());
    for (std::uint32_t i = 0; i < target.deleted_sites.size(); ++i)
    {
      unpack(source.deleted_sites(i), target.deleted_sites[i]);
    }

    target.tags.resize(source.tags_size());
    for (std::uint32_t i = 0; i < target.tags.size(); ++i)
    {
      unpack(source.tags(i), target.tags[i]);
    }

    target.deleted_tags.resize(source.deleted_tags_size());
    for (std::uint32_t i = 0; i < target.deleted_tags.size(); ++i)
    {
      unpack(source.deleted_tags(i), target.deleted_tags[i]);
    }

    target.frequency_caps.resize(source.frequency_caps_size());
    for (std::uint32_t i = 0; i < target.frequency_caps.size(); ++i)
    {
      unpack(source.frequency_caps(i), target.frequency_caps[i]);
    }

    target.deleted_freq_caps.resize(source.deleted_freq_caps_size());
    for (std::uint32_t i = 0; i < target.deleted_freq_caps.size(); ++i)
    {
      unpack(source.deleted_freq_caps(i), target.deleted_freq_caps[i]);
    }

    target.simple_channels.resize(source.simple_channels_size());
    for (std::uint32_t i = 0; i < target.simple_channels.size(); ++i)
    {
      unpack(source.simple_channels(i), target.simple_channels[i]);
    }

    target.deleted_simple_channels.resize(source.deleted_simple_channels_size());
    for (std::uint32_t i = 0; i < target.deleted_simple_channels.size(); ++i)
    {
      unpack(source.deleted_simple_channels(i), target.deleted_simple_channels[i]);
    }

    unpack_bytes(source.geo_channels_timestamp(), target.geo_channels_timestamp, 8);
    target.activate_geo_channels.resize(source.activate_geo_channels_size());
    for (std::uint32_t i = 0; i < target.activate_geo_channels.size(); ++i)
    {
      unpack(source.activate_geo_channels(i), target.activate_geo_channels[i]);
    }

    target.delete_geo_channels.resize(source.delete_geo_channels_size());
    for (std::uint32_t i = 0; i < target.delete_geo_channels.size(); ++i)
    {
      unpack(source.delete_geo_channels(i), target.delete_geo_channels[i]);
    }

    target.activate_geo_coord_channels.resize(source.activate_geo_coord_channels_size());
    for (std::uint32_t i = 0; i < target.activate_geo_coord_channels.size(); ++i)
    {
      unpack(source.activate_geo_coord_channels(i), target.activate_geo_coord_channels[i]);
    }

    target.delete_geo_coord_channels.resize(source.delete_geo_coord_channels_size());
    for (std::uint32_t i = 0; i < target.delete_geo_coord_channels.size(); ++i)
    {
      unpack(source.delete_geo_coord_channels(i), target.delete_geo_coord_channels[i]);
    }

    target.activate_block_channels.resize(source.activate_block_channels_size());
    for (std::uint32_t i = 0; i < target.activate_block_channels.size(); ++i)
    {
      unpack(source.activate_block_channels(i), target.activate_block_channels[i]);
    }

    target.delete_block_channels.resize(source.delete_block_channels_size());
    for (std::uint32_t i = 0; i < target.delete_block_channels.size(); ++i)
    {
      unpack(source.delete_block_channels(i), target.delete_block_channels[i]);
    }

    target.colocations.resize(source.colocations_size());
    for (std::uint32_t i = 0; i < target.colocations.size(); ++i)
    {
      unpack(source.colocations(i), target.colocations[i]);
    }

    target.deleted_colocations.resize(source.deleted_colocations_size());
    for (std::uint32_t i = 0; i < target.deleted_colocations.size(); ++i)
    {
      unpack(source.deleted_colocations(i), target.deleted_colocations[i]);
    }

    target.creative_templates.resize(source.creative_templates_size());
    for (std::uint32_t i = 0; i < target.creative_templates.size(); ++i)
    {
      unpack(source.creative_templates(i), target.creative_templates[i]);
    }

    target.deleted_templates.resize(source.deleted_templates_size());
    for (std::uint32_t i = 0; i < target.deleted_templates.size(); ++i)
    {
      unpack(source.deleted_templates(i), target.deleted_templates[i]);
    }

    target.currencies.resize(source.currencies_size());
    for (std::uint32_t i = 0; i < target.currencies.size(); ++i)
    {
      unpack(source.currencies(i), target.currencies[i]);
    }

    target.expression_channels.resize(source.expression_channels_size());
    for (std::uint32_t i = 0; i < target.expression_channels.size(); ++i)
    {
      unpack(source.expression_channels(i), target.expression_channels[i]);
    }

    target.deleted_expression_channels.resize(source.deleted_expression_channels_size());
    for (std::uint32_t i = 0; i < target.deleted_expression_channels.size(); ++i)
    {
      unpack(source.deleted_expression_channels(i), target.deleted_expression_channels[i]);
    }

    target.campaign_keywords.resize(source.campaign_keywords_size());
    for (std::uint32_t i = 0; i < target.campaign_keywords.size(); ++i)
    {
      unpack(source.campaign_keywords(i), target.campaign_keywords[i]);
    }

    target.deleted_keywords.resize(source.deleted_keywords_size());
    for (std::uint32_t i = 0; i < target.deleted_keywords.size(); ++i)
    {
      unpack(source.deleted_keywords(i), target.deleted_keywords[i]);
    }

    target.creative_categories.resize(source.creative_categories_size());
    for (std::uint32_t i = 0; i < target.creative_categories.size(); ++i)
    {
      unpack(source.creative_categories(i), target.creative_categories[i]);
    }

    target.deleted_creative_categories.resize(source.deleted_creative_categories_size());
    for (std::uint32_t i = 0; i < target.deleted_creative_categories.size(); ++i)
    {
      unpack(source.deleted_creative_categories(i), target.deleted_creative_categories[i]);
    }

    target.adv_actions.resize(source.adv_actions_size());
    for (std::uint32_t i = 0; i < target.adv_actions.size(); ++i)
    {
      unpack(source.adv_actions(i), target.adv_actions[i]);
    }

    target.deleted_adv_actions.resize(source.deleted_adv_actions_size());
    for (std::uint32_t i = 0; i < target.deleted_adv_actions.size(); ++i)
    {
      unpack(source.deleted_adv_actions(i), target.deleted_adv_actions[i]);
    }

    target.category_channels.resize(source.category_channels_size());
    for (std::uint32_t i = 0; i < target.category_channels.size(); ++i)
    {
      unpack(source.category_channels(i), target.category_channels[i]);
    }

    target.deleted_category_channels.resize(source.deleted_category_channels_size());
    for (std::uint32_t i = 0; i < target.deleted_category_channels.size(); ++i)
    {
      unpack(source.deleted_category_channels(i), target.deleted_category_channels[i]);
    }

    target.behav_params.resize(source.behav_params_size());
    for (std::uint32_t i = 0; i < target.behav_params.size(); ++i)
    {
      unpack(source.behav_params(i), target.behav_params[i]);
    }

    target.deleted_behav_params.resize(source.deleted_behav_params_size());
    for (std::uint32_t i = 0; i < target.deleted_behav_params.size(); ++i)
    {
      unpack(source.deleted_behav_params(i), target.deleted_behav_params[i]);
    }

    target.key_behav_params.resize(source.key_behav_params_size());
    for (std::uint32_t i = 0; i < target.key_behav_params.size(); ++i)
    {
      unpack(source.key_behav_params(i), target.key_behav_params[i]);
    }

    target.deleted_key_behav_params.resize(source.deleted_key_behav_params_size());
    for (std::uint32_t i = 0; i < target.deleted_key_behav_params.size(); ++i)
    {
      unpack(source.deleted_key_behav_params(i), target.deleted_key_behav_params[i]);
    }

    target.fraud_conditions.resize(source.fraud_conditions_size());
    for (std::uint32_t i = 0; i < target.fraud_conditions.size(); ++i)
    {
      unpack(source.fraud_conditions(i), target.fraud_conditions[i]);
    }

    target.deleted_fraud_conditions.resize(source.deleted_fraud_conditions_size());
    for (std::uint32_t i = 0; i < target.deleted_fraud_conditions.size(); ++i)
    {
      unpack(source.deleted_fraud_conditions(i), target.deleted_fraud_conditions[i]);
    }

    target.search_engines.resize(source.search_engines_size());
    for (std::uint32_t i = 0; i < target.search_engines.size(); ++i)
    {
      unpack(source.search_engines(i), target.search_engines[i]);
    }

    target.deleted_search_engines.resize(source.deleted_search_engines_size());
    for (std::uint32_t i = 0; i < target.deleted_search_engines.size(); ++i)
    {
      unpack(source.deleted_search_engines(i), target.deleted_search_engines[i]);
    }

    target.web_browsers.resize(source.web_browsers_size());
    for (std::uint32_t i = 0; i < target.web_browsers.size(); ++i)
    {
      unpack(source.web_browsers(i), target.web_browsers[i]);
    }

    target.deleted_web_browsers.resize(source.deleted_web_browsers_size());
    for (std::uint32_t i = 0; i < target.deleted_web_browsers.size(); ++i)
    {
      unpack(source.deleted_web_browsers(i), target.deleted_web_browsers[i]);
    }

    target.platforms.resize(source.platforms_size());
    for (std::uint32_t i = 0; i < target.platforms.size(); ++i)
    {
      unpack(source.platforms(i), target.platforms[i]);
    }

    target.deleted_platforms.resize(source.deleted_platforms_size());
    for (std::uint32_t i = 0; i < target.deleted_platforms.size(); ++i)
    {
      unpack(source.deleted_platforms(i), target.deleted_platforms[i]);
    }

    target.web_operations.resize(source.web_operations_size());
    for (std::uint32_t i = 0; i < target.web_operations.size(); ++i)
    {
      unpack(source.web_operations(i), target.web_operations[i]);
    }

    target.delete_web_operations.resize(source.delete_web_operations_size());
    for (std::uint32_t i = 0; i < target.delete_web_operations.size(); ++i)
    {
      unpack(source.delete_web_operations(i), target.delete_web_operations[i]);
    }

    target.contracts.resize(source.contracts_size());
    for (std::uint32_t i = 0; i < target.contracts.size(); ++i)
    {
      unpack(source.contracts(i), target.contracts[i]);
    }

    target.deleted_contracts.resize(source.deleted_contracts_size());
    for (std::uint32_t i = 0; i < target.deleted_contracts.size(); ++i)
    {
      unpack(source.deleted_contracts(i), target.deleted_contracts[i]);
    }
  }

  void pack(const CreativeStatInfo& source, rpc::CreativeStatInfo& target)
  {
    target.Clear();
    target.set_cc_id(source.cc_id);
    target.set_impressions(source.impressions);
    target.set_clicks(source.clicks);
    target.set_actions(source.actions);
  }

  void unpack(const rpc::CreativeStatInfo& source, CreativeStatInfo& target)
  {
    target.cc_id = source.cc_id();
    unpack_bytes(source.impressions(), target.impressions, 17);
    unpack_bytes(source.clicks(), target.clicks, 17);
    unpack_bytes(source.actions(), target.actions, 17);
  }

  void pack(const PublisherCCGStatInfo& source, rpc::PublisherCCGStatInfo& target)
  {
    target.Clear();
    target.set_pub_account_id(source.pub_account_id);
    target.set_amount(source.amount);
  }

  void unpack(const rpc::PublisherCCGStatInfo& source, PublisherCCGStatInfo& target)
  {
    target.pub_account_id = source.pub_account_id();
    unpack_bytes(source.amount(), target.amount, 9);
  }

  void pack(const TagCCGStatInfo& source, rpc::TagCCGStatInfo& target)
  {
    target.Clear();
    target.set_tag_id(source.tag_id);
    target.set_cur_pub_isp_amount(source.cur_pub_isp_amount);
    target.set_cur_adv_amount(source.cur_adv_amount);
    target.set_cur_adv_comm_amount(source.cur_adv_comm_amount);
    target.set_prev_pub_isp_amount(source.prev_pub_isp_amount);
    target.set_prev_adv_amount(source.prev_adv_amount);
    target.set_prev_adv_comm_amount(source.prev_adv_comm_amount);
  }

  void unpack(const rpc::TagCCGStatInfo& source, TagCCGStatInfo& target)
  {
    target.tag_id = source.tag_id();
    unpack_bytes(source.cur_pub_isp_amount(), target.cur_pub_isp_amount, 9);
    unpack_bytes(source.cur_adv_amount(), target.cur_adv_amount, 9);
    unpack_bytes(source.cur_adv_comm_amount(), target.cur_adv_comm_amount, 9);
    unpack_bytes(source.prev_pub_isp_amount(), target.prev_pub_isp_amount, 9);
    unpack_bytes(source.prev_adv_amount(), target.prev_adv_amount, 9);
    unpack_bytes(source.prev_adv_comm_amount(), target.prev_adv_comm_amount, 9);
  }

  void pack(const CtrResetStatInfo& source, rpc::CtrResetStatInfo& target)
  {
    target.Clear();
    target.set_ctr_reset_id(source.ctr_reset_id);
    target.set_impressions(source.impressions);
  }

  void unpack(const rpc::CtrResetStatInfo& source, CtrResetStatInfo& target)
  {
    target.ctr_reset_id = source.ctr_reset_id();
    unpack_bytes(source.impressions(), target.impressions, 17);
  }

  void pack(const CCGStatInfo& source, rpc::CCGStatInfo& target)
  {
    target.Clear();
    target.set_ccg_id(source.ccg_id);
    target.set_impressions(source.impressions);
    target.set_clicks(source.clicks);
    target.set_actions(source.actions);
    target.set_amount(source.amount);
    target.set_comm_amount(source.comm_amount);
    target.set_daily_amount(source.daily_amount);
    target.set_daily_comm_amount(source.daily_comm_amount);
    target.set_prev_hour_amount(source.prev_hour_amount);
    target.set_prev_hour_comm_amount(source.prev_hour_comm_amount);
    target.set_current_hour_amount(source.current_hour_amount);
    target.set_current_hour_comm_amount(source.current_hour_comm_amount);

    for (std::uint32_t i = 0; i < source.creatives.size(); ++i)
    {
      pack(source.creatives[i], *target.add_creatives());
    }

    for (std::uint32_t i = 0; i < source.publishers.size(); ++i)
    {
      pack(source.publishers[i], *target.add_publishers());
    }

    for (std::uint32_t i = 0; i < source.tags.size(); ++i)
    {
      pack(source.tags[i], *target.add_tags());
    }

    for (std::uint32_t i = 0; i < source.ctr_resets.size(); ++i)
    {
      pack(source.ctr_resets[i], *target.add_ctr_resets());
    }
  }

  void unpack(const rpc::CCGStatInfo& source, CCGStatInfo& target)
  {
    target.ccg_id = source.ccg_id();
    unpack_bytes(source.impressions(), target.impressions, 17);
    unpack_bytes(source.clicks(), target.clicks, 17);
    unpack_bytes(source.actions(), target.actions, 17);
    unpack_bytes(source.amount(), target.amount, 9);
    unpack_bytes(source.comm_amount(), target.comm_amount, 9);
    unpack_bytes(source.daily_amount(), target.daily_amount, 9);
    unpack_bytes(source.daily_comm_amount(), target.daily_comm_amount, 9);
    unpack_bytes(source.prev_hour_amount(), target.prev_hour_amount, 9);
    unpack_bytes(source.prev_hour_comm_amount(), target.prev_hour_comm_amount, 9);
    unpack_bytes(source.current_hour_amount(), target.current_hour_amount, 9);
    unpack_bytes(source.current_hour_comm_amount(), target.current_hour_comm_amount, 9);
    target.creatives.resize(source.creatives_size());
    for (std::uint32_t i = 0; i < target.creatives.size(); ++i)
    {
      unpack(source.creatives(i), target.creatives[i]);
    }

    target.publishers.resize(source.publishers_size());
    for (std::uint32_t i = 0; i < target.publishers.size(); ++i)
    {
      unpack(source.publishers(i), target.publishers[i]);
    }

    target.tags.resize(source.tags_size());
    for (std::uint32_t i = 0; i < target.tags.size(); ++i)
    {
      unpack(source.tags(i), target.tags[i]);
    }

    target.ctr_resets.resize(source.ctr_resets_size());
    for (std::uint32_t i = 0; i < target.ctr_resets.size(); ++i)
    {
      unpack(source.ctr_resets(i), target.ctr_resets[i]);
    }
  }

  void pack(const CampaignStatInfo& source, rpc::CampaignStatInfo& target)
  {
    target.Clear();
    target.set_campaign_id(source.campaign_id);
    target.set_amount(source.amount);
    target.set_comm_amount(source.comm_amount);
    target.set_daily_amount(source.daily_amount);
    target.set_daily_comm_amount(source.daily_comm_amount);

    for (std::uint32_t i = 0; i < source.ccgs.size(); ++i)
    {
      pack(source.ccgs[i], *target.add_ccgs());
    }
  }

  void unpack(const rpc::CampaignStatInfo& source, CampaignStatInfo& target)
  {
    target.campaign_id = source.campaign_id();
    unpack_bytes(source.amount(), target.amount, 9);
    unpack_bytes(source.comm_amount(), target.comm_amount, 9);
    unpack_bytes(source.daily_amount(), target.daily_amount, 9);
    unpack_bytes(source.daily_comm_amount(), target.daily_comm_amount, 9);
    target.ccgs.resize(source.ccgs_size());
    for (std::uint32_t i = 0; i < target.ccgs.size(); ++i)
    {
      unpack(source.ccgs(i), target.ccgs[i]);
    }
  }

  void pack(const AmountStatInfo& source, rpc::AmountStatInfo& target)
  {
    target.Clear();
    target.set_id(source.id);
    target.set_amount(source.amount);
    target.set_comm_amount(source.comm_amount);
    target.set_daily_amount(source.daily_amount);
    target.set_daily_comm_amount(source.daily_comm_amount);
  }

  void unpack(const rpc::AmountStatInfo& source, AmountStatInfo& target)
  {
    target.id = source.id();
    unpack_bytes(source.amount(), target.amount, 9);
    unpack_bytes(source.comm_amount(), target.comm_amount, 9);
    unpack_bytes(source.daily_amount(), target.daily_amount, 9);
    unpack_bytes(source.daily_comm_amount(), target.daily_comm_amount, 9);
  }

  void pack(const StatInfo& source, rpc::StatInfo& target)
  {
    target.Clear();
    for (std::uint32_t i = 0; i < source.accounts.size(); ++i)
    {
      pack(source.accounts[i], *target.add_accounts());
    }

    for (std::uint32_t i = 0; i < source.campaigns.size(); ++i)
    {
      pack(source.campaigns[i], *target.add_campaigns());
    }
  }

  void unpack(const rpc::StatInfo& source, StatInfo& target)
  {
    target.accounts.resize(source.accounts_size());
    for (std::uint32_t i = 0; i < target.accounts.size(); ++i)
    {
      unpack(source.accounts(i), target.accounts[i]);
    }

    target.campaigns.resize(source.campaigns_size());
    for (std::uint32_t i = 0; i < target.campaigns.size(); ++i)
    {
      unpack(source.campaigns(i), target.campaigns[i]);
    }
  }

  void pack(const DayAmount& source, rpc::DayAmount& target)
  {
    target.Clear();
    target.set_day(source.day);
    target.set_amount(source.amount);
  }

  void unpack(const rpc::DayAmount& source, DayAmount& target)
  {
    unpack_bytes(source.day(), target.day, 8);
    unpack_bytes(source.amount(), target.amount, 9);
  }

  void pack(const AmountDistributionInfo& source, rpc::AmountDistributionInfo& target)
  {
    target.Clear();
    pack(source.prev_days_amount, *target.mutable_prev_days_amount());

    for (std::uint32_t i = 0; i < source.day_amounts.size(); ++i)
    {
      pack(source.day_amounts[i], *target.add_day_amounts());
    }
  }

  void unpack(const rpc::AmountDistributionInfo& source, AmountDistributionInfo& target)
  {
    unpack(source.prev_days_amount(), target.prev_days_amount);
    target.day_amounts.resize(source.day_amounts_size());
    for (std::uint32_t i = 0; i < target.day_amounts.size(); ++i)
    {
      unpack(source.day_amounts(i), target.day_amounts[i]);
    }
  }

  void pack(const AccountAmountDistributionInfo& source, rpc::AccountAmountDistributionInfo& target)
  {
    target.Clear();
    target.set_account_id(source.account_id);
    pack(source.amount_distribution, *target.mutable_amount_distribution());
  }

  void unpack(
    const rpc::AccountAmountDistributionInfo& source, AccountAmountDistributionInfo& target)
  {
    target.account_id = source.account_id();
    unpack(source.amount_distribution(), target.amount_distribution);
  }

  void pack(const DayAmountCount& source, rpc::DayAmountCount& target)
  {
    target.Clear();
    target.set_day(source.day);
    target.set_amount(source.amount);
    target.set_imps(source.imps);
    target.set_clicks(source.clicks);
  }

  void unpack(const rpc::DayAmountCount& source, DayAmountCount& target)
  {
    unpack_bytes(source.day(), target.day, 8);
    unpack_bytes(source.amount(), target.amount, 9);
    unpack_bytes(source.imps(), target.imps, 17);
    unpack_bytes(source.clicks(), target.clicks, 17);
  }

  void pack(const AmountCountDistributionInfo& source, rpc::AmountCountDistributionInfo& target)
  {
    target.Clear();
    pack(source.prev_days_amount_count, *target.mutable_prev_days_amount_count());

    for (std::uint32_t i = 0; i < source.day_amount_counts.size(); ++i)
    {
      pack(source.day_amount_counts[i], *target.add_day_amount_counts());
    }
  }

  void unpack(const rpc::AmountCountDistributionInfo& source, AmountCountDistributionInfo& target)
  {
    unpack(source.prev_days_amount_count(), target.prev_days_amount_count);
    target.day_amount_counts.resize(source.day_amount_counts_size());
    for (std::uint32_t i = 0; i < target.day_amount_counts.size(); ++i)
    {
      unpack(source.day_amount_counts(i), target.day_amount_counts[i]);
    }
  }

  void pack(
    const CampaignAmountDistributionInfo& source, rpc::CampaignAmountDistributionInfo& target)
  {
    target.Clear();
    target.set_campaign_id(source.campaign_id);
    pack(source.amount_count_distribution, *target.mutable_amount_count_distribution());
  }

  void unpack(
    const rpc::CampaignAmountDistributionInfo& source, CampaignAmountDistributionInfo& target)
  {
    target.campaign_id = source.campaign_id();
    unpack(source.amount_count_distribution(), target.amount_count_distribution);
  }

  void pack(const CCGAmountDistributionInfo& source, rpc::CCGAmountDistributionInfo& target)
  {
    target.Clear();
    target.set_ccg_id(source.ccg_id);
    pack(source.amount_count_distribution, *target.mutable_amount_count_distribution());
  }

  void unpack(const rpc::CCGAmountDistributionInfo& source, CCGAmountDistributionInfo& target)
  {
    target.ccg_id = source.ccg_id();
    unpack(source.amount_count_distribution(), target.amount_count_distribution);
  }

  void pack(const BillStatInfo& source, rpc::BillStatInfo& target)
  {
    target.Clear();
    for (std::uint32_t i = 0; i < source.accounts.size(); ++i)
    {
      pack(source.accounts[i], *target.add_accounts());
    }

    for (std::uint32_t i = 0; i < source.campaigns.size(); ++i)
    {
      pack(source.campaigns[i], *target.add_campaigns());
    }

    for (std::uint32_t i = 0; i < source.ccgs.size(); ++i)
    {
      pack(source.ccgs[i], *target.add_ccgs());
    }
  }

  void unpack(const rpc::BillStatInfo& source, BillStatInfo& target)
  {
    target.accounts.resize(source.accounts_size());
    for (std::uint32_t i = 0; i < target.accounts.size(); ++i)
    {
      unpack(source.accounts(i), target.accounts[i]);
    }

    target.campaigns.resize(source.campaigns_size());
    for (std::uint32_t i = 0; i < target.campaigns.size(); ++i)
    {
      unpack(source.campaigns(i), target.campaigns[i]);
    }

    target.ccgs.resize(source.ccgs_size());
    for (std::uint32_t i = 0; i < target.ccgs.size(); ++i)
    {
      unpack(source.ccgs(i), target.ccgs[i]);
    }
  }

  void pack(const DiscoverChannel& source, rpc::DiscoverChannel& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);
    target.set_query(source.query);
    target.set_annotation(source.annotation);
    target.set_language(source.language);

    for (std::uint32_t i = 0; i < source.categories.size(); ++i)
    {
      target.add_categories(source.categories[i]);
    }

    target.set_timestamp(source.timestamp);
  }

  void unpack(const rpc::DiscoverChannel& source, DiscoverChannel& target)
  {
    target.channel_id = source.channel_id();
    target.query = source.query();
    target.annotation = source.annotation();
    target.language = source.language();
    target.categories.resize(source.categories_size());
    for (std::uint32_t i = 0; i < target.categories.size(); ++i)
    {
      target.categories[i] = source.categories(i);
    }

    unpack_bytes(source.timestamp(), target.timestamp, 8);
  }

  void pack(const SimpleChannelAnswer& source, rpc::SimpleChannelAnswer& target)
  {
    target.Clear();
    for (std::uint32_t i = 0; i < source.simple_channels.size(); ++i)
    {
      pack(source.simple_channels[i], *target.add_simple_channels());
    }

    for (std::uint32_t i = 0; i < source.behav_params.size(); ++i)
    {
      pack(source.behav_params[i], *target.add_behav_params());
    }

    for (std::uint32_t i = 0; i < source.key_behav_params.size(); ++i)
    {
      pack(source.key_behav_params[i], *target.add_key_behav_params());
    }

    target.set_timezone_offset(source.timezone_offset);
    target.set_master_stamp(source.master_stamp);
    target.set_cost_limit(source.cost_limit);
  }

  void unpack(const rpc::SimpleChannelAnswer& source, SimpleChannelAnswer& target)
  {
    target.simple_channels.resize(source.simple_channels_size());
    for (std::uint32_t i = 0; i < target.simple_channels.size(); ++i)
    {
      unpack(source.simple_channels(i), target.simple_channels[i]);
    }

    target.behav_params.resize(source.behav_params_size());
    for (std::uint32_t i = 0; i < target.behav_params.size(); ++i)
    {
      unpack(source.behav_params(i), target.behav_params[i]);
    }

    target.key_behav_params.resize(source.key_behav_params_size());
    for (std::uint32_t i = 0; i < target.key_behav_params.size(); ++i)
    {
      unpack(source.key_behav_params(i), target.key_behav_params[i]);
    }

    unpack_bytes(source.timezone_offset(), target.timezone_offset, 8);
    unpack_bytes(source.master_stamp(), target.master_stamp, 8);
    unpack_bytes(source.cost_limit(), target.cost_limit, 9);
  }

  void pack(const BriefSimpleChannelAnswer& source, rpc::BriefSimpleChannelAnswer& target)
  {
    target.Clear();
    for (std::uint32_t i = 0; i < source.simple_channels.size(); ++i)
    {
      pack(source.simple_channels[i], *target.add_simple_channels());
    }

    for (std::uint32_t i = 0; i < source.behav_params.size(); ++i)
    {
      pack(source.behav_params[i], *target.add_behav_params());
    }

    for (std::uint32_t i = 0; i < source.key_behav_params.size(); ++i)
    {
      pack(source.key_behav_params[i], *target.add_key_behav_params());
    }

    target.set_timezone_offset(source.timezone_offset);
    target.set_master_stamp(source.master_stamp);
  }

  void unpack(const rpc::BriefSimpleChannelAnswer& source, BriefSimpleChannelAnswer& target)
  {
    target.simple_channels.resize(source.simple_channels_size());
    for (std::uint32_t i = 0; i < target.simple_channels.size(); ++i)
    {
      unpack(source.simple_channels(i), target.simple_channels[i]);
    }

    target.behav_params.resize(source.behav_params_size());
    for (std::uint32_t i = 0; i < target.behav_params.size(); ++i)
    {
      unpack(source.behav_params(i), target.behav_params[i]);
    }

    target.key_behav_params.resize(source.key_behav_params_size());
    for (std::uint32_t i = 0; i < target.key_behav_params.size(); ++i)
    {
      unpack(source.key_behav_params(i), target.key_behav_params[i]);
    }

    unpack_bytes(source.timezone_offset(), target.timezone_offset, 8);
    unpack_bytes(source.master_stamp(), target.master_stamp, 8);
  }

  void pack(const BehaveInfo& source, rpc::BehaveInfo& target)
  {
    target.Clear();
    target.set_is_context(source.is_context);
    target.set_trigger_type(static_cast<unsigned char>(source.trigger_type));
    target.set_weight(source.weight);
  }

  void unpack(const rpc::BehaveInfo& source, BehaveInfo& target)
  {
    target.is_context = source.is_context();

    if (source.trigger_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.trigger_type = source.trigger_type();
    target.weight = source.weight();
  }

  void pack(const CSSimpleChannel& source, rpc::CSSimpleChannel& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);
    target.set_country_code(source.country_code);
    target.set_language(source.language);
    target.set_channel_type(static_cast<unsigned char>(source.channel_type));
    target.set_status(static_cast<unsigned char>(source.status));

    for (std::uint32_t i = 0; i < source.behave_info.size(); ++i)
    {
      pack(source.behave_info[i], *target.add_behave_info());
    }
  }

  void unpack(const rpc::CSSimpleChannel& source, CSSimpleChannel& target)
  {
    target.channel_id = source.channel_id();
    target.country_code = source.country_code();
    target.language = source.language();

    if (source.channel_type() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.channel_type = source.channel_type();

    if (source.status() > 255)
    {
      throw std::invalid_argument("Invalid campaign character");
    }

    target.status = source.status();
    target.behave_info.resize(source.behave_info_size());
    for (std::uint32_t i = 0; i < target.behave_info.size(); ++i)
    {
      unpack(source.behave_info(i), target.behave_info[i]);
    }
  }

  void pack(const ChannelServerChannelAnswer& source, rpc::ChannelServerChannelAnswer& target)
  {
    target.Clear();
    for (std::uint32_t i = 0; i < source.simple_channels.size(); ++i)
    {
      pack(source.simple_channels[i], *target.add_simple_channels());
    }

    target.set_cost_limit(source.cost_limit);
  }

  void unpack(const rpc::ChannelServerChannelAnswer& source, ChannelServerChannelAnswer& target)
  {
    target.simple_channels.resize(source.simple_channels_size());
    for (std::uint32_t i = 0; i < target.simple_channels.size(); ++i)
    {
      unpack(source.simple_channels(i), target.simple_channels[i]);
    }

    unpack_bytes(source.cost_limit(), target.cost_limit, 9);
  }

  void pack(const CampaignGetConfigSettings& source, rpc::CampaignGetConfigSettings& target)
  {
    target.Clear();
    target.set_timestamp(source.timestamp);
    target.set_server_id(source.server_id);
    target.set_portion(source.portion);
    target.set_portions_number(source.portions_number);
    target.set_colo_id(source.colo_id);
    target.set_version(source.version);
    target.set_campaign_statuses(source.campaign_statuses);
    target.set_channel_statuses(source.channel_statuses);
    target.set_country(source.country);
    target.set_no_deleted(source.no_deleted);
    target.set_provide_only_tags(source.provide_only_tags);
    target.set_provide_channel_triggers(source.provide_channel_triggers);
    target.set_geo_channels_timestamp(source.geo_channels_timestamp);
  }

  void unpack(const rpc::CampaignGetConfigSettings& source, CampaignGetConfigSettings& target)
  {
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    target.server_id = source.server_id();
    target.portion = source.portion();
    target.portions_number = source.portions_number();
    target.colo_id = source.colo_id();
    target.version = source.version();
    target.campaign_statuses = source.campaign_statuses();
    target.channel_statuses = source.channel_statuses();
    target.country = source.country();
    target.no_deleted = source.no_deleted();
    target.provide_only_tags = source.provide_only_tags();
    target.provide_channel_triggers = source.provide_channel_triggers();
    unpack_bytes(source.geo_channels_timestamp(), target.geo_channels_timestamp, 8);
  }

  void pack(const FraudConditionConfig& source, rpc::FraudConditionConfig& target)
  {
    target.Clear();
    for (std::uint32_t i = 0; i < source.rules.size(); ++i)
    {
      pack(source.rules[i], *target.add_rules());
    }

    target.set_deactivate_period(source.deactivate_period);
  }

  void unpack(const rpc::FraudConditionConfig& source, FraudConditionConfig& target)
  {
    target.rules.resize(source.rules_size());
    for (std::uint32_t i = 0; i < target.rules.size(); ++i)
    {
      unpack(source.rules(i), target.rules[i]);
    }

    unpack_bytes(source.deactivate_period(), target.deactivate_period, 8);
  }

  void pack(const DetectorsConfig& source, rpc::DetectorsConfig& target)
  {
    target.Clear();
    target.set_timestamp(source.timestamp);

    for (std::uint32_t i = 0; i < source.engines.size(); ++i)
    {
      pack(source.engines[i], *target.add_engines());
    }

    for (std::uint32_t i = 0; i < source.web_browsers.size(); ++i)
    {
      pack(source.web_browsers[i], *target.add_web_browsers());
    }

    for (std::uint32_t i = 0; i < source.platforms.size(); ++i)
    {
      pack(source.platforms[i], *target.add_platforms());
    }
  }

  void unpack(const rpc::DetectorsConfig& source, DetectorsConfig& target)
  {
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    target.engines.resize(source.engines_size());
    for (std::uint32_t i = 0; i < target.engines.size(); ++i)
    {
      unpack(source.engines(i), target.engines[i]);
    }

    target.web_browsers.resize(source.web_browsers_size());
    for (std::uint32_t i = 0; i < target.web_browsers.size(); ++i)
    {
      unpack(source.web_browsers(i), target.web_browsers[i]);
    }

    target.platforms.resize(source.platforms_size());
    for (std::uint32_t i = 0; i < target.platforms.size(); ++i)
    {
      unpack(source.platforms(i), target.platforms[i]);
    }
  }

  void pack(const FreqCapConfigInfo& source, rpc::FreqCapConfigInfo& target)
  {
    target.Clear();
    for (std::uint32_t i = 0; i < source.freq_caps.size(); ++i)
    {
      pack(source.freq_caps[i], *target.add_freq_caps());
    }

    for (std::uint32_t i = 0; i < source.campaign_ids.size(); ++i)
    {
      target.add_campaign_ids(source.campaign_ids[i]);
    }
  }

  void unpack(const rpc::FreqCapConfigInfo& source, FreqCapConfigInfo& target)
  {
    target.freq_caps.resize(source.freq_caps_size());
    for (std::uint32_t i = 0; i < target.freq_caps.size(); ++i)
    {
      unpack(source.freq_caps(i), target.freq_caps[i]);
    }

    target.campaign_ids.resize(source.campaign_ids_size());
    for (std::uint32_t i = 0; i < target.campaign_ids.size(); ++i)
    {
      target.campaign_ids[i] = source.campaign_ids(i);
    }
  }

  void pack(const ExpressionChannelCCGInfo& source, rpc::ExpressionChannelCCGInfo& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);

    for (std::uint32_t i = 0; i < source.ccgs.size(); ++i)
    {
      target.add_ccgs(source.ccgs[i]);
    }
  }

  void unpack(const rpc::ExpressionChannelCCGInfo& source, ExpressionChannelCCGInfo& target)
  {
    target.channel_id = source.channel_id();
    target.ccgs.resize(source.ccgs_size());
    for (std::uint32_t i = 0; i < target.ccgs.size(); ++i)
    {
      target.ccgs[i] = source.ccgs(i);
    }
  }

  void pack(const ChannelTriggersInfo& source, rpc::ChannelTriggersInfo& target)
  {
    target.Clear();
    target.set_channel_id(source.channel_id);
    target.set_page_min_visits(source.page_min_visits);
    target.set_search_min_visits(source.search_min_visits);
    target.set_url_min_visits(source.url_min_visits);
    target.set_url_keyword_min_visits(source.url_keyword_min_visits);
    target.set_page_time_to(source.page_time_to);
    target.set_search_time_to(source.search_time_to);
    target.set_url_time_to(source.url_time_to);
    target.set_url_keyword_time_to(source.url_keyword_time_to);

    for (std::uint32_t i = 0; i < source.page_triggers.size(); ++i)
    {
      target.add_page_triggers(source.page_triggers[i]);
    }

    for (std::uint32_t i = 0; i < source.search_triggers.size(); ++i)
    {
      target.add_search_triggers(source.search_triggers[i]);
    }

    for (std::uint32_t i = 0; i < source.url_triggers.size(); ++i)
    {
      target.add_url_triggers(source.url_triggers[i]);
    }

    for (std::uint32_t i = 0; i < source.url_keyword_triggers.size(); ++i)
    {
      target.add_url_keyword_triggers(source.url_keyword_triggers[i]);
    }
  }

  void unpack(const rpc::ChannelTriggersInfo& source, ChannelTriggersInfo& target)
  {
    target.channel_id = source.channel_id();
    target.page_min_visits = source.page_min_visits();
    target.search_min_visits = source.search_min_visits();
    target.url_min_visits = source.url_min_visits();
    target.url_keyword_min_visits = source.url_keyword_min_visits();
    target.page_time_to = source.page_time_to();
    target.search_time_to = source.search_time_to();
    target.url_time_to = source.url_time_to();
    target.url_keyword_time_to = source.url_keyword_time_to();
    target.page_triggers.resize(source.page_triggers_size());
    for (std::uint32_t i = 0; i < target.page_triggers.size(); ++i)
    {
      target.page_triggers[i] = source.page_triggers(i);
    }

    target.search_triggers.resize(source.search_triggers_size());
    for (std::uint32_t i = 0; i < target.search_triggers.size(); ++i)
    {
      target.search_triggers[i] = source.search_triggers(i);
    }

    target.url_triggers.resize(source.url_triggers_size());
    for (std::uint32_t i = 0; i < target.url_triggers.size(); ++i)
    {
      target.url_triggers[i] = source.url_triggers(i);
    }

    target.url_keyword_triggers.resize(source.url_keyword_triggers_size());
    for (std::uint32_t i = 0; i < target.url_keyword_triggers.size(); ++i)
    {
      target.url_keyword_triggers[i] = source.url_keyword_triggers(i);
    }
  }

  void pack(const ExpressionChannelsInfo& source, rpc::ExpressionChannelsInfo& target)
  {
    target.Clear();
    target.set_timestamp(source.timestamp);
    target.set_first_load_stamp(source.first_load_stamp);

    for (std::uint32_t i = 0; i < source.expression_channels.size(); ++i)
    {
      pack(source.expression_channels[i], *target.add_expression_channels());
    }

    for (std::uint32_t i = 0; i < source.expression_channel_ccgs.size(); ++i)
    {
      pack(source.expression_channel_ccgs[i], *target.add_expression_channel_ccgs());
    }

    for (std::uint32_t i = 0; i < source.overlap_channel_ids.size(); ++i)
    {
      target.add_overlap_channel_ids(source.overlap_channel_ids[i]);
    }

    for (std::uint32_t i = 0; i < source.activate_channel_triggers.size(); ++i)
    {
      pack(source.activate_channel_triggers[i], *target.add_activate_channel_triggers());
    }

    for (std::uint32_t i = 0; i < source.delete_simple_channels.size(); ++i)
    {
      pack(source.delete_simple_channels[i], *target.add_delete_simple_channels());
    }

    for (std::uint32_t i = 0; i < source.activate_actions.size(); ++i)
    {
      pack(source.activate_actions[i], *target.add_activate_actions());
    }

    for (std::uint32_t i = 0; i < source.delete_actions.size(); ++i)
    {
      pack(source.delete_actions[i], *target.add_delete_actions());
    }
  }

  void unpack(const rpc::ExpressionChannelsInfo& source, ExpressionChannelsInfo& target)
  {
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    unpack_bytes(source.first_load_stamp(), target.first_load_stamp, 8);
    target.expression_channels.resize(source.expression_channels_size());
    for (std::uint32_t i = 0; i < target.expression_channels.size(); ++i)
    {
      unpack(source.expression_channels(i), target.expression_channels[i]);
    }

    target.expression_channel_ccgs.resize(source.expression_channel_ccgs_size());
    for (std::uint32_t i = 0; i < target.expression_channel_ccgs.size(); ++i)
    {
      unpack(source.expression_channel_ccgs(i), target.expression_channel_ccgs[i]);
    }

    target.overlap_channel_ids.resize(source.overlap_channel_ids_size());
    for (std::uint32_t i = 0; i < target.overlap_channel_ids.size(); ++i)
    {
      target.overlap_channel_ids[i] = source.overlap_channel_ids(i);
    }

    target.activate_channel_triggers.resize(source.activate_channel_triggers_size());
    for (std::uint32_t i = 0; i < target.activate_channel_triggers.size(); ++i)
    {
      unpack(source.activate_channel_triggers(i), target.activate_channel_triggers[i]);
    }

    target.delete_simple_channels.resize(source.delete_simple_channels_size());
    for (std::uint32_t i = 0; i < target.delete_simple_channels.size(); ++i)
    {
      unpack(source.delete_simple_channels(i), target.delete_simple_channels[i]);
    }

    target.activate_actions.resize(source.activate_actions_size());
    for (std::uint32_t i = 0; i < target.activate_actions.size(); ++i)
    {
      unpack(source.activate_actions(i), target.activate_actions[i]);
    }

    target.delete_actions.resize(source.delete_actions_size());
    for (std::uint32_t i = 0; i < target.delete_actions.size(); ++i)
    {
      unpack(source.delete_actions(i), target.delete_actions[i]);
    }
  }

  void pack(const AccountDeliveryInfo& source, rpc::AccountDeliveryInfo& target)
  {
    target.Clear();
    target.set_account_id(source.account_id);
    target.set_time_offset(source.time_offset);
    target.set_active(source.active);
    target.set_budget(source.budget);
  }

  void unpack(const rpc::AccountDeliveryInfo& source, AccountDeliveryInfo& target)
  {
    target.account_id = source.account_id();
    unpack_bytes(source.time_offset(), target.time_offset, 8);
    target.active = source.active();
    unpack_bytes(source.budget(), target.budget, 9);
  }

  void pack(const CampaignDeliveryInfo& source, rpc::CampaignDeliveryInfo& target)
  {
    target.Clear();
    target.set_campaign_id(source.campaign_id);
    target.set_time_offset(source.time_offset);
    target.set_active(source.active);
    pack(source.delivery_limits, *target.mutable_delivery_limits());
  }

  void unpack(const rpc::CampaignDeliveryInfo& source, CampaignDeliveryInfo& target)
  {
    target.campaign_id = source.campaign_id();
    unpack_bytes(source.time_offset(), target.time_offset, 8);
    target.active = source.active();
    unpack(source.delivery_limits(), target.delivery_limits);
  }

  void pack(const CCGDeliveryInfo& source, rpc::CCGDeliveryInfo& target)
  {
    target.Clear();
    target.set_ccg_id(source.ccg_id);
    target.set_campaign_id(source.campaign_id);
    target.set_time_offset(source.time_offset);
    target.set_active(source.active);
    pack(source.delivery_limits, *target.mutable_delivery_limits());
    target.set_imp_amount(source.imp_amount);
    target.set_click_amount(source.click_amount);
  }

  void unpack(const rpc::CCGDeliveryInfo& source, CCGDeliveryInfo& target)
  {
    target.ccg_id = source.ccg_id();
    target.campaign_id = source.campaign_id();
    unpack_bytes(source.time_offset(), target.time_offset, 8);
    target.active = source.active();
    unpack(source.delivery_limits(), target.delivery_limits);
    unpack_bytes(source.imp_amount(), target.imp_amount, 9);
    unpack_bytes(source.click_amount(), target.click_amount, 9);
  }

  void pack(
    const CampaignServer::GetExpressionChannelsInfo& source, rpc::GetExpressionChannelsInfo& target)
  {
    target.Clear();
    target.set_timestamp(source.timestamp);
    target.set_portion(source.portion);
    target.set_portions_number(source.portions_number);
    target.set_channel_types(source.channel_types);
    target.set_channel_statuses(source.channel_statuses);
    target.set_provide_ccg_links(source.provide_ccg_links);
    target.set_provide_channel_triggers(source.provide_channel_triggers);
    target.set_provide_overlap_channel_ids(source.provide_overlap_channel_ids);
  }

  void unpack(
    const rpc::GetExpressionChannelsInfo& source, CampaignServer::GetExpressionChannelsInfo& target)
  {
    unpack_bytes(source.timestamp(), target.timestamp, 8);
    target.portion = source.portion();
    target.portions_number = source.portions_number();
    target.channel_types = source.channel_types();
    target.channel_statuses = source.channel_statuses();
    target.provide_ccg_links = source.provide_ccg_links();
    target.provide_channel_triggers = source.provide_channel_triggers();
    target.provide_overlap_channel_ids = source.provide_overlap_channel_ids();
  }

  void pack(const CampaignServer::GetSimpleChannelsInfo& source, rpc::GetSimpleChannelsInfo& target)
  {
    target.Clear();
    target.set_portion(source.portion);
    target.set_portions_number(source.portions_number);
    target.set_channel_statuses(source.channel_statuses);
  }

  void unpack(
    const rpc::GetSimpleChannelsInfo& source, CampaignServer::GetSimpleChannelsInfo& target)
  {
    target.portion = source.portion();
    target.portions_number = source.portions_number();
    target.channel_statuses = source.channel_statuses();
  }

  void pack(const CampaignServer::PassbackInfo& source, rpc::PassbackInfo& target)
  {
    target.Clear();
    target.set_passback_type(source.passback_type);
    target.set_passback_code(source.passback_code);
    target.set_passback_url(source.passback_url);
    target.set_width(source.width);
    target.set_height(source.height);
    target.set_size(source.size);
    target.set_mime_format(source.mime_format);
    target.set_active(source.active);
  }

  void unpack(const rpc::PassbackInfo& source, CampaignServer::PassbackInfo& target)
  {
    target.passback_type = source.passback_type();
    target.passback_code = source.passback_code();
    target.passback_url = source.passback_url();
    target.width = source.width();
    target.height = source.height();
    target.size = source.size();
    target.mime_format = source.mime_format();
    target.active = source.active();
  }

  void pack(const CampaignServer::ColocationPropInfo& source, rpc::ColocationPropInfo& target)
  {
    target.Clear();
    target.set_found(source.found);
    target.set_colo_id(source.colo_id);
    target.set_time_offset(source.time_offset);
  }

  void unpack(const rpc::ColocationPropInfo& source, CampaignServer::ColocationPropInfo& target)
  {
    target.found = source.found();
    target.colo_id = source.colo_id();
    unpack_bytes(source.time_offset(), target.time_offset, 8);
  }

  void pack(
    const CampaignServer::DeliveryLimitConfigInfo& source, rpc::DeliveryLimitConfigInfo& target)
  {
    target.Clear();
    for (std::uint32_t i = 0; i < source.accounts.size(); ++i)
    {
      pack(source.accounts[i], *target.add_accounts());
    }

    for (std::uint32_t i = 0; i < source.campaigns.size(); ++i)
    {
      pack(source.campaigns[i], *target.add_campaigns());
    }

    for (std::uint32_t i = 0; i < source.ccgs.size(); ++i)
    {
      pack(source.ccgs[i], *target.add_ccgs());
    }
  }

  void unpack(
    const rpc::DeliveryLimitConfigInfo& source, CampaignServer::DeliveryLimitConfigInfo& target)
  {
    target.accounts.resize(source.accounts_size());
    for (std::uint32_t i = 0; i < target.accounts.size(); ++i)
    {
      unpack(source.accounts(i), target.accounts[i]);
    }

    target.campaigns.resize(source.campaigns_size());
    for (std::uint32_t i = 0; i < target.campaigns.size(); ++i)
    {
      unpack(source.campaigns(i), target.campaigns[i]);
    }

    target.ccgs.resize(source.ccgs_size());
    for (std::uint32_t i = 0; i < target.ccgs.size(); ++i)
    {
      unpack(source.ccgs(i), target.ccgs[i]);
    }
  }

}
