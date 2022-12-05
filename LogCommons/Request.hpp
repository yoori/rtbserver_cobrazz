#ifndef AD_SERVER_LOG_PROCESSING_REQUEST_HPP
#define AD_SERVER_LOG_PROCESSING_REQUEST_HPP


#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <LogCommons/CsvUtils.hpp>
#include <LogCommons/GenericLogCsvSaverImpl.hpp>
#include <LogCommons/Compatibility/Request_Base.hpp>
#include <LogCommons/Compatibility/Request_v361.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer {
namespace LogProcessing {

typedef std::pair<StringIO<Aux_::ConvertSpacesSeparators, '='>,
  StringIO<Aux_::ConvertSpacesSeparators, ','> > UserProperty;
typedef std::list<UserProperty> UserPropertyList;

class RequestData
{
public:
  DECLARE_EXCEPTION(InvalidSystemRevenue, eh::DescriptiveException);

  typedef RequestData_V_3_6_1::DeliveryThresholdT DeliveryThresholdT;
  typedef RequestData_V_3_6_1::DeviceChannelIdOptional
    DeviceChannelIdOptional;

  typedef RequestData_V_3_6_1::CmpChannel CmpChannel;

  typedef RequestData_V_3_6_1::CmpChannelList CmpChannelList;
  typedef std::vector<CmpChannel> CmpChannelArray;

  typedef RequestData_V_3_6_1::Revenue Revenue;

  typedef OptionalValue<unsigned long> OptionalUlong;

  typedef Revenue::FixedNum FixedNum;

  typedef AdServer::LogProcessing::FixedNumberList FixedNumberList;

  RequestData() noexcept {}

  RequestData(const RequestData& init) noexcept
    : holder_(init.holder_)
  {}

  RequestData(RequestData&& init) noexcept
  {
    holder_.swap(init.holder_);
  }

  RequestData(
    const SecondsTimestamp& time,
    const SecondsTimestamp& isp_time,
    const SecondsTimestamp& pub_time,
    const SecondsTimestamp& adv_time,
    const RequestId& request_id,
    const RequestId& global_request_id,
    const UserId& user_id,
    const UserId& household_id,
    bool test_request,
    unsigned long colo_id,
    unsigned long site_id,
    unsigned long tag_id,
    const std::string& ext_tag_id,
    unsigned long publisher_account_id,
    const std::string& country_code,
    const std::string& ip_address,
    unsigned long adv_account_id,
    unsigned long advertiser_id,
    unsigned long cc_id,
    unsigned long cmp_id,
    unsigned long ccg_id,
    const DeliveryThresholdT& delivery_threshold,
    bool has_custom_actions,
    unsigned long currency_exchange_id,
    UserPropertyList user_props,
    const Revenue& adv_revenue,
    const Revenue& pub_revenue,
    const Revenue& isp_revenue,
    const Revenue& adv_comm_revenue,
    const Revenue& adv_payable_comm_amount,
    const Revenue& pub_comm_revenue,
    NumberArray channel_list,
    NumberArray history_channel_list,
    const std::string& channel_expression,
    CmpChannelList cmp_channel_list,
    unsigned long ccg_keyword_id,
    unsigned long keyword_id,
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
    const std::string& tag_size,
    const OptionalUlong& size_id,
    bool hid_profile,
    const OptionalUlong& tag_visibility,
    const OptionalUlong& tag_top_offset,
    const OptionalUlong& tag_left_offset,
    unsigned long ctr_reset_id,
    unsigned long campaign_freq,
    const std::string& referer,
    const FixedNum& adv_currency_rate,
    const FixedNum& pub_currency_rate,
    const FixedNum& pub_commission,
    const FixedNum& isp_currency_rate,
    const FixedNum& isp_revenue_share,
    const FixedNum& ecpm,
    const FixedNum& floor_cost,
    const std::string& ctr_algorithm_id,
    const FixedNum& ctr,
    unsigned long full_referer_hash,
    const CampaignSvcs::AuctionType auction_type,
    const std::string& conv_rate_algorithm_id,
    const FixedNum& conv_rate,
    long tag_predicted_viewability,
    const FixedNumberList& model_ctrs,
    const FixedNum& self_service_commission,
    const FixedNum& adv_commission,
    const FixedNum& pub_cost_coef,
    unsigned long at_flags)
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
        model_ctrs,
        self_service_commission,
        adv_commission,
        pub_cost_coef,
        at_flags
      ))
  {}

  RequestData(const RequestData_V_3_6_1& data)
    : holder_(new DataHolder(
        data.time(),
        data.isp_time(),
        data.pub_time(),
        data.adv_time(),
        data.request_id(),
        data.global_request_id(),
        data.user_id(),
        data.household_id(),
        data.test_request(),
        data.colo_id(),
        data.site_id(),
        data.tag_id(),
        data.ext_tag_id(),
        data.publisher_account_id(),
        data.country_code(),
        data.ip_address(),
        data.adv_account_id(),
        data.advertiser_id(),
        data.cc_id(),
        data.cmp_id(),
        data.ccg_id(),
        data.delivery_threshold(),
        data.has_custom_actions(),
        data.currency_exchange_id(),
        data.user_props(),
        data.adv_revenue(),
        data.pub_revenue(),
        data.isp_revenue(),
        data.adv_comm_revenue(),
        data.adv_payable_comm_amount(),
        data.pub_comm_revenue(),
        data.channel_list(),
        data.history_channel_list(),
        data.channel_expression(),
        data.cmp_channel_list(),
        data.ccg_keyword_id(),
        data.keyword_id(),
        data.keyword_page_match(),
        data.keyword_search_match(),
        data.num_shown(),
        data.position(),
        data.enabled_notice(),
        data.enabled_impression_tracking(),
        data.enabled_action_tracking(),
        data.disable_fraud_detection(),
        data.walled_garden(),
        data.ccg_type(),
        data.user_status(),
        data.lost_auction_ccgs(),
        data.geo_channels(),
        data.device_channel_id(),
        data.tag_size(),
        data.size_id(),
        data.hid_profile(),
        data.tag_visibility(),
        data.tag_top_offset(),
        data.tag_left_offset(),
        data.ctr_reset_id(),
        data.campaign_freq(),
        data.referer(),
        data.adv_currency_rate(),
        data.pub_currency_rate(),
        data.pub_commission(),
        data.isp_currency_rate(),
        data.isp_revenue_share(),
        data.ecpm(),
        data.floor_cost(),
        data.ctr_algorithm_id(),
        data.ctr(),
        data.full_referer_hash(),
        data.auction_type(),
        data.conv_rate_algorithm_id(),
        data.conv_rate(),
        data.tag_predicted_viewability(),
        data.model_ctrs(),
        FixedNum::ZERO, // self_service_commission
        FixedNum::ZERO, // adv_commission
        FixedNum::ZERO, // pub_cost_coef
        0 // at_flags
      ))
  {}

  bool operator==(const RequestData& data) const
  {
    if (this == &data || holder_.in() == data.holder_.in())
    {
      return true;
    }
    return *holder_ == *data.holder_;
  }

  Generics::Time time() const
  {
    return holder_->time.time();
  }

  Generics::Time isp_time() const
  {
    return holder_->isp_time.time();
  }

  Generics::Time pub_time() const
  {
    return holder_->pub_time.time();
  }

  Generics::Time adv_time() const
  {
    return holder_->adv_time.time();
  }

  const RequestId& request_id() const
  {
    return holder_->request_id;
  }

  const RequestId& global_request_id() const
  {
    return holder_->global_request_id;
  }

  const UserId& user_id() const
  {
    return holder_->user_id;
  }

  const UserId& household_id() const
  {
    return holder_->household_id;
  }

  bool test_request() const
  {
    return holder_->test_request;
  }

  unsigned long colo_id() const
  {
    return holder_->colo_id;
  }

  unsigned long site_id() const
  {
    return holder_->site_id;
  }

  unsigned long tag_id() const
  {
    return holder_->tag_id;
  }

  const std::string& ext_tag_id() const
  {
    return holder_->ext_tag_id.get();
  }

  unsigned long publisher_account_id() const
  {
    return holder_->publisher_account_id;
  }

  const std::string& country_code() const
  {
    return holder_->country_code.get();
  }

  const std::string& ip_address() const
  {
    return holder_->ip_address.get();
  }

  unsigned long adv_account_id() const
  {
    return holder_->adv_account_id;
  }

  unsigned long advertiser_id() const
  {
    return holder_->advertiser_id;
  }

  unsigned long cc_id() const
  {
    return holder_->cc_id;
  }

  unsigned long cmp_id() const
  {
    return holder_->cmp_id;
  }

  unsigned long ccg_id() const
  {
    return holder_->ccg_id;
  }

  const DeliveryThresholdT& delivery_threshold() const
  {
    return holder_->delivery_threshold;
  }

  bool has_custom_actions() const
  {
    return holder_->has_custom_actions;
  }

  unsigned long currency_exchange_id() const
  {
    return holder_->currency_exchange_id;
  }

  const UserPropertyList& user_props() const
  {
    return holder_->user_props;
  }

  const Revenue& adv_revenue() const
  {
    return holder_->adv_revenue;
  }

  const Revenue& pub_revenue() const
  {
    return holder_->pub_revenue;
  }

  const Revenue& isp_revenue() const
  {
    return holder_->isp_revenue;
  }

  const Revenue& adv_comm_revenue() const
  {
    return holder_->adv_comm_revenue;
  }

  const Revenue& adv_payable_comm_amount() const
  {
    return holder_->adv_payable_comm_amount;
  }

  const Revenue& pub_comm_revenue() const
  {
    return holder_->pub_comm_revenue;
  }

  const NumberArray&
  channel_list() const
  {
    return holder_->channel_list;
  }

  const NumberArray&
  history_channel_list() const
  {
    return holder_->history_channel_list;
  }

  const std::string& channel_expression() const
  {
    return holder_->channel_expression.get();
  }

  const CmpChannelList& cmp_channel_list() const
  {
    return holder_->cmp_channel_list;
  }

  unsigned long ccg_keyword_id() const
  {
    return holder_->ccg_keyword_id;
  }

  unsigned long keyword_id() const
  {
    return holder_->keyword_id;
  }

  bool keyword_page_match() const
  {
    return holder_->keyword_page_match;
  }

  bool keyword_search_match() const
  {
    return holder_->keyword_search_match;
  }

  unsigned long num_shown() const
  {
    return holder_->num_shown;
  }

  unsigned long position() const
  {
    return holder_->position;
  }

  bool enabled_notice() const
  {
    return holder_->enabled_notice;
  }

  bool enabled_impression_tracking() const
  {
    return holder_->enabled_impression_tracking;
  }

  bool enabled_action_tracking() const
  {
    return holder_->enabled_action_tracking;
  }

  bool disable_fraud_detection() const
  {
    return holder_->disable_fraud_detection;
  }

  bool walled_garden() const
  {
    return holder_->walled_garden;
  }

  char ccg_type() const
  {
    return holder_->ccg_type;
  }

  char user_status() const
  {
    return holder_->user_status;
  }

  const NumberArray&
  lost_auction_ccgs() const
  {
    return holder_->lost_auction_ccgs;
  }

  const NumberArray&
  geo_channels() const
  {
    return holder_->geo_channels;
  }

  const DeviceChannelIdOptional& device_channel_id() const
  {
    return holder_->device_channel_id;
  }

  const std::string& tag_size() const
  {
    return holder_->tag_size;
  }

  const OptionalUlong& size_id() const noexcept
  {
    return holder_->size_id;
  }

  bool hid_profile() const noexcept
  {
    return holder_->hid_profile;
  }

  const OptionalUlong& tag_visibility() const
  {
    return holder_->tag_visibility;
  }

  const OptionalUlong& tag_top_offset() const
  {
    return holder_->tag_top_offset;
  }

  const OptionalUlong& tag_left_offset() const
  {
    return holder_->tag_left_offset;
  }

  unsigned long ctr_reset_id() const
  {
    return holder_->ctr_reset_id;
  }

  unsigned long campaign_freq() const
  {
    return holder_->campaign_freq;
  }

  const std::string& referer() const
  {
    return holder_->referer.get();
  }

  const FixedNum& adv_currency_rate() const
  {
    return holder_->adv_currency_rate;
  }

  const FixedNum& pub_currency_rate() const
  {
    return holder_->pub_currency_rate;
  }

  const FixedNum& pub_commission() const
  {
    return holder_->pub_commission;
  }

  const FixedNum& isp_currency_rate() const
  {
    return holder_->isp_currency_rate;
  }

  const FixedNum& isp_revenue_share() const
  {
    return holder_->isp_revenue_share;
  }

  const FixedNum& ecpm() const
  {
    return holder_->ecpm;
  }

  const FixedNum& floor_cost() const
  {
    return holder_->floor_cost;
  }

  const std::string& ctr_algorithm_id() const
  {
    return holder_->ctr_algorithm_id.get();
  }

  const FixedNum& ctr() const
  {
    return holder_->ctr;
  }

  unsigned long full_referer_hash() const
  {
    return holder_->full_referer_hash;
  }

  unsigned long distrib_hash() const
  {
    return request_distribution_hash(holder_->request_id, holder_->user_id);
  }

  CampaignSvcs::AuctionType auction_type() const
  {
    return holder_->auction_type;
  }

  const std::string& conv_rate_algorithm_id() const
  {
    return holder_->conv_rate_algorithm_id.get();
  }

  const FixedNum& conv_rate() const
  {
    return holder_->conv_rate;
  }

  bool disabled_pub_cost_check() const
  {
    return false;
  }

  long tag_predicted_viewability() const
  {
    return holder_->tag_predicted_viewability;
  }

  const FixedNumberList& model_ctrs() const
  {
    return holder_->model_ctrs;
  }

  const FixedNum&
  self_service_commission() const
  {
    return holder_->self_service_commission;
  }

  const FixedNum&
  adv_commission() const
  {
    return holder_->adv_commission;
  }

  const FixedNum&
  pub_cost_coef() const
  {
    return holder_->pub_cost_coef;
  }

  unsigned long
  at_flags() const
  {
    return holder_->at_flags;
  }

private:
  struct DataHolder: public ReferenceCounting::AtomicImpl
  {
    DataHolder() {}

    DataHolder(
      const SecondsTimestamp& time_val,
      const SecondsTimestamp& isp_time_val,
      const SecondsTimestamp& pub_time_val,
      const SecondsTimestamp& adv_time_val,
      const RequestId& request_id_val,
      const RequestId& global_request_id_val,
      const UserId& user_id_val,
      const UserId& household_id_val,
      bool test_request_val,
      unsigned long colo_id_val,
      unsigned long site_id_val,
      unsigned long tag_id_val,
      const StringIoWrapperOptional& ext_tag_id_val,
      unsigned long publisher_account_id_val,
      const std::string& country_code_val,
      const StringIoWrapperOptional& ip_address_val,
      unsigned long adv_account_id_val,
      unsigned long advertiser_id_val,
      unsigned long cc_id_val,
      unsigned long cmp_id_val,
      unsigned long ccg_id_val,
      const DeliveryThresholdT& delivery_threshold_val,
      bool has_custom_actions_val,
      unsigned long currency_exchange_id_val,
      UserPropertyList user_props_val,
      const Revenue& adv_revenue_val,
      const Revenue& pub_revenue_val,
      const Revenue& isp_revenue_val,
      const Revenue& adv_comm_revenue_val,
      const Revenue& adv_payable_comm_amount_val,
      const Revenue& pub_comm_revenue_val,
      NumberArray channel_list_val,
      NumberArray history_channel_list_val,
      const std::string& channel_expression_val,
      CmpChannelList cmp_channel_list_val,
      unsigned long ccg_keyword_id_val,
      unsigned long keyword_id_val,
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
      const std::string& tag_size_val,
      const OptionalUlong& size_id_val,
      bool hid_profile_val,
      const OptionalUlong& tag_visibility_val,
      const OptionalUlong& tag_top_offset_val,
      const OptionalUlong& tag_left_offset_val,
      unsigned long ctr_reset_id_val,
      unsigned long campaign_freq_val,
      const StringIoWrapperOptional& referer_val,
      const FixedNum& adv_currency_rate_val,
      const FixedNum& pub_currency_rate_val,
      const FixedNum& pub_commission_val,
      const FixedNum& isp_currency_rate_val,
      const FixedNum& isp_revenue_share_val,
      const FixedNum& ecpm_val,
      const FixedNum& floor_cost_val,
      const StringIoWrapperOptional& ctr_algorithm_id_val,
      const FixedNum& ctr_val,
      unsigned long full_referer_hash_val,
      const CampaignSvcs::AuctionType auction_type_val,
      const std::string& conv_rate_algorithm_id_val,
      const FixedNum& conv_rate_val,
      long tag_predicted_viewability_val,
      const FixedNumberList& model_ctrs_val,
      const FixedNum& self_service_commission_val,
      const FixedNum& adv_commission_val,
      const FixedNum& pub_cost_coef_val,
      unsigned long at_flags_val)
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
        model_ctrs(model_ctrs_val),
        self_service_commission(self_service_commission_val),
        adv_commission(adv_commission_val),
        pub_cost_coef(pub_cost_coef_val),
        at_flags(at_flags_val)
    {}

    // compatibility c-tor
    DataHolder(
      const SecondsTimestamp& time_val,
      const SecondsTimestamp& isp_time_val,
      const SecondsTimestamp& pub_time_val,
      const SecondsTimestamp& adv_time_val,
      const RequestId& request_id_val,
      const RequestId& global_request_id_val,
      const UserId& user_id_val,
      const UserId& household_id_val,
      bool test_request_val,
      unsigned long colo_id_val,
      unsigned long site_id_val,
      unsigned long tag_id_val,
      const StringIoWrapperOptional& ext_tag_id_val,
      unsigned long publisher_account_id_val,
      const std::string& country_code_val,
      const StringIoWrapperOptional& ip_address_val,
      unsigned long adv_account_id_val,
      unsigned long advertiser_id_val,
      unsigned long cc_id_val,
      unsigned long cmp_id_val,
      unsigned long ccg_id_val,
      const DeliveryThresholdT& delivery_threshold_val,
      bool has_custom_actions_val,
      unsigned long currency_exchange_id_val,
      UserPropertyList user_props_val,
      const Revenue& adv_revenue_val,
      const Revenue& pub_revenue_val,
      const Revenue& isp_revenue_val,
      const Revenue& adv_comm_revenue_val,
      const Revenue& adv_payable_comm_amount_val,
      const Revenue& pub_comm_revenue_val,
      const NumberList& channel_list_val,
      const NumberList& history_channel_list_val,
      const std::string& channel_expression_val,
      const CmpChannelList& cmp_channel_list_val,
      unsigned long ccg_keyword_id_val,
      unsigned long keyword_id_val,
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
      const NumberList& lost_auction_ccgs_val,
      const NumberList& geo_channels_val,
      const DeviceChannelIdOptional& device_channel_id_val,
      const std::string& tag_size_val,
      const OptionalUlong& size_id_val,
      bool hid_profile_val,
      const OptionalUlong& tag_visibility_val,
      const OptionalUlong& tag_top_offset_val,
      const OptionalUlong& tag_left_offset_val,
      unsigned long ctr_reset_id_val,
      unsigned long campaign_freq_val,
      const StringIoWrapperOptional& referer_val,
      const FixedNum& adv_currency_rate_val,
      const FixedNum& pub_currency_rate_val,
      const FixedNum& pub_commission_val,
      const FixedNum& isp_currency_rate_val,
      const FixedNum& isp_revenue_share_val,
      const FixedNum& ecpm_val,
      const FixedNum& floor_cost_val,
      const StringIoWrapperOptional& ctr_algorithm_id_val,
      const FixedNum& ctr_val,
      unsigned long full_referer_hash_val,
      const CampaignSvcs::AuctionType auction_type_val,
      const std::string& conv_rate_algorithm_id_val,
      const FixedNum& conv_rate_val,
      long tag_predicted_viewability_val,
      const FixedNumberList& model_ctrs_val,
      const FixedNum& self_service_commission_val,
      const FixedNum& adv_commission_val,
      const FixedNum& pub_cost_coef_val,
      unsigned long at_flags_val)
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
        user_props(user_props_val),
        adv_revenue(adv_revenue_val),
        pub_revenue(pub_revenue_val),
        isp_revenue(isp_revenue_val),
        adv_comm_revenue(adv_comm_revenue_val),
        adv_payable_comm_amount(adv_payable_comm_amount_val),
        pub_comm_revenue(pub_comm_revenue_val),
        channel_list(channel_list_val.begin(), channel_list_val.end()),
        history_channel_list(history_channel_list_val.begin(), history_channel_list_val.end()),
        channel_expression(channel_expression_val),
        cmp_channel_list(cmp_channel_list_val),
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
        lost_auction_ccgs(lost_auction_ccgs_val.begin(), lost_auction_ccgs_val.end()),
        geo_channels(geo_channels_val.begin(), geo_channels_val.end()),
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
        model_ctrs(model_ctrs_val),
        self_service_commission(self_service_commission_val),
        adv_commission(adv_commission_val),
        pub_cost_coef(pub_cost_coef_val),
        at_flags(at_flags_val)
    {}

    bool
    operator==(const DataHolder& data) const
    {
      return time == data.time &&
        isp_time == data.isp_time &&
        pub_time == data.pub_time &&
        adv_time == data.adv_time &&
        request_id == data.request_id &&
        global_request_id == data.global_request_id &&
        user_id == data.user_id &&
        household_id == data.household_id &&
        test_request == data.test_request &&
        colo_id == data.colo_id &&
        site_id == data.site_id &&
        tag_id == data.tag_id &&
        ext_tag_id.get() == data.ext_tag_id.get() &&
        publisher_account_id == data.publisher_account_id &&
        country_code.get() == data.country_code.get() &&
        ip_address.get() == data.ip_address.get() &&
        adv_account_id == data.adv_account_id &&
        advertiser_id == data.advertiser_id &&
        cc_id == data.cc_id &&
        cmp_id == data.cmp_id &&
        ccg_id == data.ccg_id &&
        delivery_threshold == data.delivery_threshold &&
        has_custom_actions == data.has_custom_actions &&
        currency_exchange_id == data.currency_exchange_id &&
        user_props == data.user_props &&
        adv_revenue == data.adv_revenue &&
        pub_revenue == data.pub_revenue &&
        isp_revenue == data.isp_revenue &&
        adv_comm_revenue == data.adv_comm_revenue &&
        adv_payable_comm_amount == data.adv_payable_comm_amount &&
        pub_comm_revenue == data.pub_comm_revenue &&
        channel_list == data.channel_list &&
        history_channel_list == data.history_channel_list &&
        channel_expression.get() == data.channel_expression.get() &&
        cmp_channel_list == data.cmp_channel_list &&
        ccg_keyword_id == data.ccg_keyword_id &&
        keyword_id == data.keyword_id &&
        keyword_page_match == data.keyword_page_match &&
        keyword_search_match == data.keyword_search_match &&
        num_shown == data.num_shown &&
        position == data.position &&
        enabled_notice == data.enabled_notice &&
        enabled_impression_tracking == data.enabled_impression_tracking &&
        enabled_action_tracking == data.enabled_action_tracking &&
        disable_fraud_detection == data.disable_fraud_detection &&
        walled_garden == data.walled_garden &&
        ccg_type == data.ccg_type &&
        user_status == data.user_status &&
        lost_auction_ccgs == data.lost_auction_ccgs &&
        geo_channels == data.geo_channels &&
        device_channel_id == data.device_channel_id &&
        tag_size == data.tag_size &&
        size_id == data.size_id &&
        hid_profile == data.hid_profile &&
        tag_visibility == data.tag_visibility &&
        tag_top_offset == data.tag_top_offset &&
        tag_left_offset == data.tag_left_offset &&
        ctr_reset_id == data.ctr_reset_id &&
        campaign_freq == data.campaign_freq &&
        referer.get() == data.referer.get() &&
        adv_currency_rate == data.adv_currency_rate &&
        pub_currency_rate == data.pub_currency_rate &&
        pub_commission == data.pub_commission &&
        isp_currency_rate == data.isp_currency_rate &&
        isp_revenue_share == data.isp_revenue_share &&
        ecpm == data.ecpm &&
        floor_cost == data.floor_cost &&
        ctr_algorithm_id.get() == data.ctr_algorithm_id.get() &&
        ctr == data.ctr &&
        full_referer_hash == data.full_referer_hash &&
        auction_type == data.auction_type &&
        conv_rate_algorithm_id == data.conv_rate_algorithm_id &&
        conv_rate == data.conv_rate &&
        tag_predicted_viewability == data.tag_predicted_viewability &&
        model_ctrs == data.model_ctrs &&
        self_service_commission == data.self_service_commission &&
        adv_commission == data.adv_commission &&
        pub_cost_coef == data.pub_cost_coef &&
        at_flags == data.at_flags;
    }

    template <class ARCHIVE_>
    void serialize(ARCHIVE_& ar)
    {
      ar & time;
      ar & isp_time;
      ar & pub_time;
      ar & adv_time;
      ar & request_id;
      ar & global_request_id;
      ar & user_id;
      ar & household_id;
      ar & test_request;
      ar & colo_id;
      ar & site_id;
      ar & tag_id;
      ar & ext_tag_id;
      ar & publisher_account_id;
      ar & country_code;
      ar & ip_address;
      ar & adv_account_id;
      ar & advertiser_id;
      ar & cc_id;
      ar & cmp_id;
      ar & ccg_id;
      ar & delivery_threshold;
      ar & has_custom_actions;
      ar & currency_exchange_id;
      ar & user_props;
      ar & adv_revenue;
      ar & pub_revenue;
      ar & isp_revenue;
      ar & adv_comm_revenue;
      ar & adv_payable_comm_amount;
      ar & pub_comm_revenue;
      ar & channel_list;
      ar & history_channel_list;
      ar & channel_expression;
      ar & cmp_channel_list;
      ar & ccg_keyword_id;
      ar & keyword_id;
      ar & keyword_page_match;
      ar & keyword_search_match;
      ar & num_shown;
      ar & position;
      ar & enabled_notice;
      ar & enabled_impression_tracking;
      ar & enabled_action_tracking;
      ar & disable_fraud_detection;
      ar & walled_garden;
      ar & ccg_type;
      ar & user_status;
      ar & lost_auction_ccgs;
      ar & geo_channels;
      ar & device_channel_id;
      ar & tag_size;
      ar & size_id;
      ar & hid_profile;
      ar & tag_visibility;
      ar & tag_top_offset;
      ar & tag_left_offset;
      ar & ctr_reset_id;
      ar & campaign_freq;
      ar & referer;
      ar & adv_currency_rate;
      ar & pub_currency_rate;
      ar & pub_commission;
      ar & isp_currency_rate;
      ar & isp_revenue_share;
      ar & ecpm;
      ar & floor_cost;
      ar & ctr_algorithm_id;
      ar & ctr;
      ar & full_referer_hash;
      ar & auction_type;
      ar & conv_rate_algorithm_id;
      ar & conv_rate;
      ar & tag_predicted_viewability;
      ar & model_ctrs;
      ar & self_service_commission;
      ar & adv_commission;
      ar & pub_cost_coef;
      ar ^ at_flags;
    }

    void invariant() const /*throw(ConstraintViolation)*/
    {
      if (!ccg_type_is_valid())
      {
        Stream::Error es;
        es << "RequestData::DataHolder::invariant(): ccg_type "
           "has invalid value '" << ccg_type << '\'';
        throw ConstraintViolation(es);
      }
      if (!is_valid_user_status(user_status))
      {
        Stream::Error es;
        es << "RequestData::DataHolder::invariant(): user_status "
           "has invalid value '" << user_status << '\'';
        throw ConstraintViolation(es);
      }
      if (!delivery_threshold_is_valid())
      {
        Stream::Error es;
        es << "RequestData::DataHolder::invariant(): delivery_threshold "
           "has invalid value '" << delivery_threshold << '\'';
        throw ConstraintViolation(es);
      }
    }

    SecondsTimestamp time;
    SecondsTimestamp isp_time;
    SecondsTimestamp pub_time;
    SecondsTimestamp adv_time;
    RequestIdIoWrapper request_id;
    RequestIdIoWrapper global_request_id;
    UserIdIoWrapper user_id;
    UserIdIoWrapper household_id;
    bool test_request;
    unsigned long colo_id;
    unsigned long site_id;
    unsigned long tag_id;

    StringIoWrapperOptional ext_tag_id;

    unsigned long publisher_account_id;
    EmptyHolder<std::string> country_code;
    StringIoWrapperOptional ip_address;
    unsigned long adv_account_id;
    unsigned long advertiser_id;
    unsigned long cc_id;
    unsigned long cmp_id;
    unsigned long ccg_id;
    DeliveryThresholdT delivery_threshold;
    bool has_custom_actions;
    unsigned long currency_exchange_id;
    UserPropertyList user_props;
    Revenue adv_revenue;
    Revenue pub_revenue;
    Revenue isp_revenue;
    Revenue adv_comm_revenue;
    Revenue adv_payable_comm_amount;
    Revenue pub_comm_revenue;

    NumberArray channel_list;
    NumberArray history_channel_list;
    EmptyHolder<Aux_::StringIoWrapper> channel_expression;
    CmpChannelList cmp_channel_list;

    unsigned long ccg_keyword_id;
    unsigned long keyword_id;
    bool keyword_page_match;
    bool keyword_search_match;
    unsigned long num_shown;
    unsigned long position;

    bool enabled_notice;
    bool enabled_impression_tracking;
    bool enabled_action_tracking;
    bool disable_fraud_detection;
    bool walled_garden;
    char ccg_type;
    char user_status;
    NumberArray lost_auction_ccgs;
    NumberArray geo_channels;
    DeviceChannelIdOptional device_channel_id;
    Aux_::StringIoWrapper tag_size;
    OptionalUlong size_id;
    bool hid_profile;

    OptionalUlong tag_visibility;
    OptionalUlong tag_top_offset;
    OptionalUlong tag_left_offset;

    unsigned long ctr_reset_id;
    unsigned long campaign_freq;

    StringIoWrapperOptional referer;

    FixedNum adv_currency_rate;
    FixedNum pub_currency_rate;
    FixedNum pub_commission;
    FixedNum isp_currency_rate;
    FixedNum isp_revenue_share;
    FixedNum ecpm;
    FixedNum floor_cost;

    StringIoWrapperOptional ctr_algorithm_id;
    FixedNum ctr;

    unsigned long full_referer_hash;

    CampaignSvcs::AuctionType auction_type;
    StringIoWrapperOptional conv_rate_algorithm_id;
    FixedNum conv_rate;

    long tag_predicted_viewability;

    FixedNumberList model_ctrs;
    FixedNum self_service_commission;
    FixedNum adv_commission;
    FixedNum pub_cost_coef;
    unsigned long at_flags;

private:
    virtual ~DataHolder() noexcept {}

    bool ccg_type_is_valid() const
    {
      return ccg_type == 'D' || ccg_type == 'T';
    }

    bool delivery_threshold_is_valid() const
    {
      return delivery_threshold >= DeliveryThresholdT::ZERO &&
        delivery_threshold <= max_delivery_threshold_value_;
    }

    static const DeliveryThresholdT max_delivery_threshold_value_;
  };

  typedef ReferenceCounting::AssertPtr<DataHolder>::Ptr DataHolder_var;

  friend
  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, RequestData& data)
    /*throw(eh::Exception)*/;


  friend
  std::ostream&
  operator<<(std::ostream& os, const RequestData& data)
    /*throw(eh::Exception)*/;

  DataHolder_var holder_;
};

FixedBufStream<CommaCategory>&
operator>>(FixedBufStream<CommaCategory>& is, UserProperty& property) /*throw(eh::Exception)*/;

typedef SeqCollector<RequestData_V_3_5_4, true> RequestCollector_V_3_5_4;

typedef SeqCollector<RequestData_V_3_6, true> RequestCollector_V_3_6;

typedef SeqCollector<RequestData_V_3_6_1, true> RequestCollector_V_3_6_1;

  typedef SeqCollector<RequestData, true> RequestCollector;

struct RequestTraits: LogDefaultTraits<RequestCollector, false, false>
{
  template <class T>
  static void for_each_old(T& obj) /*throw(eh::Exception)*/
  {
    obj.template support<RequestCollector_V_3_5_4, false>("3.5.4");
    obj.template support<RequestCollector_V_3_6, false>("3.6");
    obj.template support<RequestCollector_V_3_6_1, false>("3.6.1");
    obj.template support<RequestCollector_V_3_6_1, false>("3.7");
  }

  static
  const RequestCollector
  convert_collector(const RequestCollector_V_3_6_1& old_collector)
  {
    RequestCollector collector;
    collector.insert(collector.end(), old_collector.begin(), old_collector.end());
    return collector;
  }

  static
  const RequestCollector
  convert_collector(const RequestCollector_V_3_6& old_collector)
  {
    RequestCollector_V_3_6_1 collector_v_3_6_1;
    collector_v_3_6_1.insert(collector_v_3_6_1.end(), old_collector.begin(), old_collector.end());
    return convert_collector(collector_v_3_6_1);
  }

  static
  const RequestCollector
  convert_collector(const RequestCollector_V_3_5_4& old_collector)
  {
    RequestCollector_V_3_6 collector_v_3_6;
    collector_v_3_6.insert(collector_v_3_6.end(), old_collector.begin(), old_collector.end());
    return convert_collector(collector_v_3_6);
  }

  typedef GenericLogIoHelperImpl<RequestTraits> IoHelperType;
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_REQUEST_HPP */
