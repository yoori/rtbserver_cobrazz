
#include "Request.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{
  RequestData::DataHolder::DataHolder(
    const SecondsTimestamp& time_val,
    const SecondsTimestamp& isp_time_val,
    const SecondsTimestamp& pub_time_val,
    const SecondsTimestamp& adv_time_val,
    const RequestId& request_id_val,
    const RequestId& global_request_id_val,
    const UserId& user_id_val,
    const UserId& household_id_val,
    bool test_request_val,
    std::uint32_t colo_id_val,
    std::uint32_t site_id_val,
    std::uint32_t tag_id_val,
    std::string_view ext_tag_id_val,
    std::uint32_t publisher_account_id_val,
    std::string_view country_code_val,
    std::string_view ip_address_val,
    std::uint32_t adv_account_id_val,
    std::uint32_t advertiser_id_val,
    std::uint32_t cc_id_val,
    std::uint32_t cmp_id_val,
    std::uint32_t ccg_id_val,
    const DeliveryThresholdT& delivery_threshold_val,
    bool has_custom_actions_val,
    std::uint32_t currency_exchange_id_val,
    UserPropertyList user_props_val,
    const Revenue& adv_revenue_val,
    const Revenue& pub_revenue_val,
    const Revenue& isp_revenue_val,
    const Revenue& adv_comm_revenue_val,
    const Revenue& adv_payable_comm_amount_val,
    const Revenue& pub_comm_revenue_val,
    NumberArray channel_list_val,
    NumberArray history_channel_list_val,
    std::string_view channel_expression_val,
    CmpChannelList cmp_channel_list_val,
    std::uint32_t ccg_keyword_id_val,
    std::uint32_t keyword_id_val,
    bool keyword_page_match_val,
    bool keyword_search_match_val,
    unsigned long num_shown_val,
    unsigned long position_val,
    bool enabled_notice_val,
    bool enabled_impression_tracking_val,
    bool enabled_action_tracking_val,
    bool disable_fraud_detection_val,
    bool walled_garden_val,
    char ccg_type_val,
    char user_status_val,
    NumberArray lost_auction_ccgs_val,
    NumberArray geo_channels_val,
    const DeviceChannelIdOptional& device_channel_id_val,
    std::string_view tag_size_val,
    const OptionalUInt32& size_id_val,
    bool hid_profile_val,
    const OptionalUInt32& tag_visibility_val,
    const OptionalUInt32& tag_top_offset_val,
    const OptionalUInt32& tag_left_offset_val,
    std::uint32_t ctr_reset_id_val,
    unsigned long campaign_freq_val,
    std::string_view referer_val,
    const FixedNum& adv_currency_rate_val,
    const FixedNum& pub_currency_rate_val,
    const FixedNum& pub_commission_val,
    const FixedNum& isp_currency_rate_val,
    const FixedNum& isp_revenue_share_val,
    const FixedNum& ecpm_val,
    const FixedNum& floor_cost_val,
    std::string_view ctr_algorithm_id_val,
    const FixedNum& ctr_val,
    unsigned long full_referer_hash_val,
    const CampaignSvcs::AuctionType auction_type_val,
    std::string_view conv_rate_algorithm_id_val,
    const FixedNum& conv_rate_val,
    long tag_predicted_viewability_val,
    FixedNumberList model_ctrs_val,
    const FixedNum& self_service_commission_val,
    const FixedNum& adv_commission_val,
    const FixedNum& pub_cost_coef_val,
    unsigned long at_flags_val,
    std::string_view additional_info_val,
    const StringIoWrapperOptional& page_keywords_val,
    std::shared_ptr<const std::vector<std::string>> expected_post_actions_val)
    : time(time_val),
      isp_time(isp_time_val),
      pub_time(pub_time_val),
      adv_time(adv_time_val),
      request_id(request_id_val),
      global_request_id(global_request_id_val),
      user_id(user_id_val),
      household_id(household_id_val),
      test_request(test_request_val),
      colo_id(colo_id_val),
      site_id(site_id_val),
      tag_id(tag_id_val),
      ext_tag_id(ext_tag_id_val),
      publisher_account_id(publisher_account_id_val),
      country_code(country_code_val),
      ip_address(ip_address_val),
      adv_account_id(adv_account_id_val),
      advertiser_id(advertiser_id_val),
      cc_id(cc_id_val),
      cmp_id(cmp_id_val),
      ccg_id(ccg_id_val),
      delivery_threshold(delivery_threshold_val),
      has_custom_actions(has_custom_actions_val),
      currency_exchange_id(currency_exchange_id_val),
      user_props(std::move(user_props_val)),
      adv_revenue(adv_revenue_val),
      pub_revenue(pub_revenue_val),
      isp_revenue(isp_revenue_val),
      adv_comm_revenue(adv_comm_revenue_val),
      adv_payable_comm_amount(adv_payable_comm_amount_val),
      pub_comm_revenue(pub_comm_revenue_val),
      channel_list(std::move(channel_list_val)),
      history_channel_list(std::move(history_channel_list_val)),
      channel_expression(channel_expression_val),
      cmp_channel_list(std::move(cmp_channel_list_val)),
      ccg_keyword_id(ccg_keyword_id_val),
      keyword_id(keyword_id_val),
      keyword_page_match(keyword_page_match_val),
      keyword_search_match(keyword_search_match_val),
      num_shown(num_shown_val),
      position(position_val),
      enabled_notice(enabled_notice_val),
      enabled_impression_tracking(enabled_impression_tracking_val),
      enabled_action_tracking(enabled_action_tracking_val),
      disable_fraud_detection(disable_fraud_detection_val),
      walled_garden(walled_garden_val),
      ccg_type(ccg_type_val),
      user_status(user_status_val),
      lost_auction_ccgs(std::move(lost_auction_ccgs_val)),
      geo_channels(std::move(geo_channels_val)),
      device_channel_id(device_channel_id_val),
      tag_size(tag_size_val),
      size_id(size_id_val),
      hid_profile(hid_profile_val),
      tag_visibility(tag_visibility_val),
      tag_top_offset(tag_top_offset_val),
      tag_left_offset(tag_left_offset_val),
      ctr_reset_id(ctr_reset_id_val),
      campaign_freq(campaign_freq_val),
      referer(referer_val),
      adv_currency_rate(adv_currency_rate_val),
      pub_currency_rate(pub_currency_rate_val),
      pub_commission(pub_commission_val),
      isp_currency_rate(isp_currency_rate_val),
      isp_revenue_share(isp_revenue_share_val),
      ecpm(ecpm_val),
      floor_cost(floor_cost_val),
      ctr_algorithm_id(ctr_algorithm_id_val),
      ctr(ctr_val),
      full_referer_hash(full_referer_hash_val),
      auction_type(auction_type_val),
      conv_rate_algorithm_id(conv_rate_algorithm_id_val),
      conv_rate(conv_rate_val),
      tag_predicted_viewability(tag_predicted_viewability_val),
      model_ctrs(std::move(model_ctrs_val)),
      self_service_commission(self_service_commission_val),
      adv_commission(adv_commission_val),
      pub_cost_coef(pub_cost_coef_val),
      at_flags(at_flags_val),
      additional_info(additional_info_val),
      page_keywords(page_keywords_val),
      expected_post_actions(expected_post_actions_val ?
        std::move(expected_post_actions_val) : empty_expected_post_actions())
  {}

  RequestData::RequestData(
    const SecondsTimestamp& time,
    const SecondsTimestamp& isp_time,
    const SecondsTimestamp& pub_time,
    const SecondsTimestamp& adv_time,
    const RequestId& request_id,
    const RequestId& global_request_id,
    const UserId& user_id,
    const UserId& household_id,
    bool test_request,
    std::uint32_t colo_id,
    std::uint32_t site_id,
    std::uint32_t tag_id,
    std::string_view ext_tag_id,
    std::uint32_t publisher_account_id,
    std::string_view country_code,
    std::string_view ip_address,
    std::uint32_t adv_account_id,
    std::uint32_t advertiser_id,
    std::uint32_t cc_id,
    std::uint32_t cmp_id,
    std::uint32_t ccg_id,
    const DeliveryThresholdT& delivery_threshold,
    bool has_custom_actions,
    std::uint32_t currency_exchange_id,
    UserPropertyList user_props,
    const Revenue& adv_revenue,
    const Revenue& pub_revenue,
    const Revenue& isp_revenue,
    const Revenue& adv_comm_revenue,
    const Revenue& adv_payable_comm_amount,
    const Revenue& pub_comm_revenue,
    NumberArray channel_list,
    NumberArray history_channel_list,
    std::string_view channel_expression,
    CmpChannelList cmp_channel_list,
    std::uint32_t ccg_keyword_id,
    std::uint32_t keyword_id,
    bool keyword_page_match,
    bool keyword_search_match,
    unsigned long num_shown,
    unsigned long position,
    bool enabled_notice,
    bool enabled_impression_tracking,
    bool enabled_action_tracking,
    bool disable_fraud_detection,
    bool walled_garden,
    char ccg_type,
    char user_status,
    NumberArray lost_auction_ccgs,
    NumberArray geo_channels,
    const DeviceChannelIdOptional& device_channel_id,
    std::string_view tag_size,
    const OptionalUInt32& size_id,
    bool hid_profile,
    const OptionalUInt32& tag_visibility,
    const OptionalUInt32& tag_top_offset,
    const OptionalUInt32& tag_left_offset,
    std::uint32_t ctr_reset_id,
    unsigned long campaign_freq,
    std::string_view referer,
    const FixedNum& adv_currency_rate,
    const FixedNum& pub_currency_rate,
    const FixedNum& pub_commission,
    const FixedNum& isp_currency_rate,
    const FixedNum& isp_revenue_share,
    const FixedNum& ecpm,
    const FixedNum& floor_cost,
    std::string_view ctr_algorithm_id,
    const FixedNum& ctr,
    unsigned long full_referer_hash,
    const CampaignSvcs::AuctionType auction_type,
    std::string_view conv_rate_algorithm_id,
    const FixedNum& conv_rate,
    long tag_predicted_viewability,
    FixedNumberList model_ctrs,
    const FixedNum& self_service_commission,
    const FixedNum& adv_commission,
    const FixedNum& pub_cost_coef,
    unsigned long at_flags,
    std::string_view additional_info,
    const std::string& page_keywords,
    std::shared_ptr<const std::vector<std::string>> expected_post_actions)
    : holder_(new DataHolder(
        time,
        isp_time,
        pub_time,
        adv_time,
        request_id,
        global_request_id,
        user_id,
        household_id,
        test_request,
        colo_id,
        site_id,
        tag_id,
        ext_tag_id,
        publisher_account_id,
        country_code,
        ip_address,
        adv_account_id,
        advertiser_id,
        cc_id,
        cmp_id,
        ccg_id,
        delivery_threshold,
        has_custom_actions,
        currency_exchange_id,
        std::move(user_props),
        adv_revenue,
        pub_revenue,
        isp_revenue,
        adv_comm_revenue,
        adv_payable_comm_amount,
        pub_comm_revenue,
        std::move(channel_list),
        std::move(history_channel_list),
        channel_expression,
        std::move(cmp_channel_list),
        ccg_keyword_id,
        keyword_id,
        keyword_page_match,
        keyword_search_match,
        num_shown,
        position,
        enabled_notice,
        enabled_impression_tracking,
        enabled_action_tracking,
        disable_fraud_detection,
        walled_garden,
        ccg_type,
        user_status,
        std::move(lost_auction_ccgs),
        std::move(geo_channels),
        device_channel_id,
        tag_size,
        size_id,
        hid_profile,
        tag_visibility,
        tag_top_offset,
        tag_left_offset,
        ctr_reset_id,
        campaign_freq,
        referer,
        adv_currency_rate,
        pub_currency_rate,
        pub_commission,
        isp_currency_rate,
        isp_revenue_share,
        ecpm,
        floor_cost,
        ctr_algorithm_id,
        ctr,
        full_referer_hash,
        auction_type,
        conv_rate_algorithm_id,
        conv_rate,
        tag_predicted_viewability,
        std::move(model_ctrs),
        self_service_commission,
        adv_commission,
        pub_cost_coef,
        at_flags,
        additional_info,
        page_keywords,
        std::move(expected_post_actions)
      ))
  {}


  template <> const char* RequestTraits::B::base_name_ = "Request";
  template <> const char* RequestTraits::B::signature_ = "Request";
  template <> const char* RequestTraits::B::current_version_ = "3.7.4";

  namespace {

  const char CC_SEP1 = ',';
  const char CC_SEP2 = ':';

  const char UP_SEP1 = ',';
  const char UP_SEP2 = '=';

  } // namespace

  const RequestData::DeliveryThresholdT
    RequestData::DataHolder::max_delivery_threshold_value_("1.00000");

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestData& data)
    /*throw(eh::Exception)*/
  {
    data.holder_ = new RequestData::DataHolder;
    TokenizerInputArchive</*Aux_::OwnInvariants*/> ia(is);
    data.holder_->serialize_v_3_7_2(ia);
    ia & data.holder_->page_keywords;
    StringArray expected_post_actions;
    ia ^ expected_post_actions;
    data.holder_->expected_post_actions = expected_post_actions.empty() ?
      empty_expected_post_actions() :
      std::make_shared<const StringArray>(std::move(expected_post_actions));
    data.holder_->invariant();
    return is;
  }

  void
  RequestData::read_v_3_7_2_(FixedBufStream<TabCategory>& is)
  {
    holder_ = new DataHolder;
    TokenizerInputArchive<> ia(is);
    holder_->serialize_v_3_7_2(ia);
    holder_->invariant();
  }

  void
  RequestData::read_v_3_7_3_(FixedBufStream<TabCategory>& is)
  {
    holder_ = new DataHolder;
    TokenizerInputArchive<> ia(is);
    holder_->serialize_v_3_7_3(ia);
    holder_->invariant();
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestData_V_3_7_2& data)
    /*throw(eh::Exception)*/
  {
    data.read_(is);
    return is;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestData_V_3_7_3& data)
    /*throw(eh::Exception)*/
  {
    data.read_(is);
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const RequestData& data)
    /*throw(eh::Exception)*/
  {
    BufferTabOutputArchive archive(out);
    data.holder_->invariant();
    data.holder_->serialize_v_3_7_2(archive);
    archive & data.holder_->page_keywords;
    archive ^ *data.holder_->expected_post_actions;
    return out;
  }

} // namespace AdServer::LogProcessing
