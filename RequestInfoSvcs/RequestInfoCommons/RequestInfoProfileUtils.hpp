#ifndef REQUESTINFOPROFILEUTILS_HPP_
#define REQUESTINFOPROFILEUTILS_HPP_

#include <Commons/Algs.hpp>

#include <RequestInfoSvcs/RequestInfoCommons/RequestProfile.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

namespace AdServer
{
namespace RequestInfoSvcs
{
  void
  print_request_info_profile(
    std::ostream& out,
    const RequestInfoProfileReader& reader)
    noexcept;
}
}

namespace AdServer
{
namespace RequestInfoSvcs
{
  namespace
  {
    const Table::Column REQUEST_INFO_TABLE_COLUMNS[] =
    {
      Table::Column("request_done", Table::Column::NUMBER),
      Table::Column("time", Table::Column::TEXT),
      Table::Column("isp_time", Table::Column::TEXT),
      Table::Column("pub_time", Table::Column::TEXT),
      Table::Column("adv_time", Table::Column::TEXT),
      Table::Column("user_id", Table::Column::TEXT),
      Table::Column("household_id", Table::Column::TEXT),
      Table::Column("user_status", Table::Column::TEXT),

      Table::Column("test_request", Table::Column::NUMBER),
      Table::Column("walled_garden", Table::Column::NUMBER),
      Table::Column("hid_profile", Table::Column::NUMBER),
      Table::Column("disable_fraud_detection", Table::Column::NUMBER),
      Table::Column("adv_account_id", Table::Column::NUMBER),
      Table::Column("campaign_id", Table::Column::NUMBER),
      Table::Column("ccg_id", Table::Column::NUMBER),
      Table::Column("ctr_reset_id", Table::Column::NUMBER),
      Table::Column("cc_id", Table::Column::NUMBER),
      Table::Column("has_custom_actions", Table::Column::NUMBER),
      Table::Column("pub_account_id", Table::Column::NUMBER),
      Table::Column("site_id", Table::Column::NUMBER),
      Table::Column("tag_id", Table::Column::NUMBER),
      Table::Column("size_id", Table::Column::NUMBER),
      Table::Column("colo_id", Table::Column::NUMBER),
      Table::Column("currency_exchange_id", Table::Column::NUMBER),
      Table::Column("tag_delivery_threshold", Table::Column::TEXT),
      Table::Column("num_shown", Table::Column::NUMBER),
      Table::Column("position", Table::Column::NUMBER),
      Table::Column("tag_size", Table::Column::TEXT),
      Table::Column("tag_visibility", Table::Column::TEXT),
      Table::Column("tag_top_offset", Table::Column::TEXT),
      Table::Column("tag_left_offset", Table::Column::TEXT),
      Table::Column("viewability", Table::Column::TEXT),

      Table::Column("text_campaign", Table::Column::NUMBER),

      Table::Column("adv_revenue", Table::Column::TEXT),
      Table::Column("delta_adv_revenue", Table::Column::TEXT),
      Table::Column("adv_comm_revenue", Table::Column::TEXT),
      Table::Column("adv_payable_comm_amount", Table::Column::TEXT),
      Table::Column("adv_currency_rate", Table::Column::TEXT),

      Table::Column("pub_revenue", Table::Column::TEXT),
      Table::Column("pub_comm_revenue", Table::Column::TEXT),
      Table::Column("pub_bid_cost", Table::Column::TEXT),
      Table::Column("pub_floor_cost", Table::Column::TEXT),
      Table::Column("pub_currency_rate", Table::Column::TEXT),

      Table::Column("isp_revenue", Table::Column::TEXT),
      Table::Column("isp_currency_rate", Table::Column::TEXT),
      Table::Column("isp_revenue_share", Table::Column::TEXT),

      Table::Column("ccg_keyword_id", Table::Column::NUMBER),
      Table::Column("keyword_id", Table::Column::NUMBER),
      Table::Column("channels", Table::Column::TEXT),
      Table::Column("geo_channel_id", Table::Column::NUMBER),
      Table::Column("device_channel_id", Table::Column::NUMBER),
      Table::Column("expression", Table::Column::TEXT),

      Table::Column("client_application", Table::Column::TEXT),
      Table::Column("client_app-version", Table::Column::TEXT),
      Table::Column("browser_version", Table::Column::TEXT),
      Table::Column("os_version", Table::Column::TEXT),
      Table::Column("country", Table::Column::TEXT),
      Table::Column("cmp_channels", Table::Column::TEXT),

      Table::Column("enabled_notice", Table::Column::NUMBER),
      Table::Column("enabled_impression_tracking", Table::Column::NUMBER),
      Table::Column("enabled_action_tracking", Table::Column::NUMBER),

      Table::Column("notice_received", Table::Column::NUMBER),
      Table::Column("notice_non_considered", Table::Column::NUMBER),
      Table::Column("notice_pub_revenue_type", Table::Column::TEXT),

      Table::Column("impression_time", Table::Column::TEXT),
      Table::Column("impression_verified", Table::Column::NUMBER),
      Table::Column("impression_non_considered", Table::Column::NUMBER),
      Table::Column("impression_pub_revenue_type", Table::Column::TEXT),

      Table::Column("click_time", Table::Column::TEXT),
      Table::Column("click_done", Table::Column::NUMBER),
      Table::Column("click_non_considered", Table::Column::NUMBER),

      Table::Column("action_time", Table::Column::TEXT),
      Table::Column("actions_done", Table::Column::NUMBER),
      Table::Column("actions_non_considered", Table::Column::NUMBER),

      Table::Column("fraud_time", Table::Column::TEXT),
      Table::Column("fraud", Table::Column::NUMBER),

      Table::Column("post_impression_actions", Table::Column::TEXT),

      Table::Column("campaign_freq", Table::Column::NUMBER),
      Table::Column("referer_hash", Table::Column::NUMBER),
      Table::Column("geo_channels", Table::Column::TEXT),
      Table::Column("user_channels", Table::Column::TEXT),
      Table::Column("url", Table::Column::TEXT),
      Table::Column("ip_address", Table::Column::TEXT),
      Table::Column("ctr_algorithm_id", Table::Column::TEXT),
      Table::Column("ctr", Table::Column::TEXT),
      Table::Column("model_ctrs", Table::Column::TEXT),
      Table::Column("conv_rate_algorithm_id", Table::Column::TEXT),
      Table::Column("conv_rate", Table::Column::TEXT),
      Table::Column("model_conv_rates", Table::Column::TEXT),
      Table::Column("self_service_commission", Table::Column::TEXT),
      Table::Column("adv_commission", Table::Column::TEXT),
      Table::Column("pub_commission", Table::Column::TEXT),
      Table::Column("pub_cost_coef", Table::Column::TEXT),
      Table::Column("at_flags", Table::Column::NUMBER)
    };

    std::string
    print_optional_uint(long value)
    {
      if(value < 0)
      {
        return "undefined";
      }

      return String::StringManip::IntToStr(value).str().str();
    }

    std::string
    print_revenue_reader(
      const RequestInfoRevenueReader& rev_reader)
    {
      std::ostringstream ostr;
      ostr << "rate_id = " << rev_reader.rate_id() <<
        ", imp = " << rev_reader.impression() <<
        ", click = " << rev_reader.click() <<
        ", action = " << rev_reader.action();
      return ostr.str();
    }

    template<typename SeqType>
    std::string
    array_to_string(const SeqType& cont)
    {
      std::ostringstream ostr;
      Algs::print(ostr, cont.begin(), cont.end());
      return ostr.str();
    }
  }

  std::string
  pub_revenue_type_to_string(unsigned long int_revenue_type)
  {
    AdServer::CampaignSvcs::RevenueType revenue_type =
      static_cast<AdServer::CampaignSvcs::RevenueType>(int_revenue_type);

    if(revenue_type == AdServer::CampaignSvcs::RT_ABSOLUTE)
    {
      return "absolute";
    }

    if(revenue_type == AdServer::CampaignSvcs::RT_SHARE)
    {
      return "share";
    }

    return "none";
  }

  void
  print_request_info_profile(
    std::ostream& out,
    const RequestInfoProfileReader& reader)
    noexcept
  {
    const unsigned long columns = sizeof(REQUEST_INFO_TABLE_COLUMNS) /
      sizeof(REQUEST_INFO_TABLE_COLUMNS[0]);

    Table table(columns);

    for(unsigned long i = 0; i < columns; i++)
    {
      table.column(i, REQUEST_INFO_TABLE_COLUMNS[i]);
    }

    Table::Row row(table.columns());
    row.add_field(reader.request_done());
    row.add_field(Generics::Time(reader.time()).gm_ft());
    row.add_field(Generics::Time(reader.isp_time()).gm_ft());
    row.add_field(Generics::Time(reader.pub_time()).gm_ft());
    row.add_field(Generics::Time(reader.adv_time()).gm_ft());
    row.add_field(reader.user_id());
    row.add_field(reader.household_id());
    row.add_field(reader.user_status());

    row.add_field(reader.test_request());
    row.add_field(reader.walled_garden());
    row.add_field(reader.hid_profile());
    row.add_field(reader.disable_fraud_detection());
    row.add_field(reader.adv_account_id());
    row.add_field(reader.campaign_id());
    row.add_field(reader.ccg_id());
    row.add_field(reader.ctr_reset_id());
    row.add_field(reader.cc_id());
    row.add_field(reader.has_custom_actions());
    row.add_field(reader.publisher_account_id());
    row.add_field(reader.site_id());
    row.add_field(reader.tag_id());
    row.add_field(reader.size_id());
    row.add_field(reader.colo_id());
    row.add_field(reader.currency_exchange_id());
    row.add_field(reader.tag_delivery_threshold());
    row.add_field(reader.num_shown());
    row.add_field(reader.position());
    row.add_field(reader.tag_size());
    row.add_field(print_optional_uint(reader.tag_visibility()));
    row.add_field(print_optional_uint(reader.tag_top_offset()));
    row.add_field(print_optional_uint(reader.tag_left_offset()));
    row.add_field(reader.viewability());

    row.add_field(reader.text_campaign());

    row.add_field(print_revenue_reader(reader.adv_revenue()));
    row.add_field(print_revenue_reader(reader.delta_adv_revenue()));
    row.add_field(print_revenue_reader(reader.adv_comm_revenue()));
    row.add_field(print_revenue_reader(reader.adv_payable_comm_amount()));
    row.add_field(reader.adv_currency_rate());
    row.add_field(print_revenue_reader(reader.pub_revenue()));
    row.add_field(print_revenue_reader(reader.pub_comm_revenue()));
    row.add_field(reader.pub_bid_cost());
    row.add_field(reader.pub_floor_cost());
    row.add_field(reader.pub_currency_rate());

    row.add_field(print_revenue_reader(reader.isp_revenue()));
    row.add_field(reader.isp_currency_rate());
    row.add_field(reader.isp_revenue_share());

    row.add_field(reader.ccg_keyword_id());
    row.add_field(reader.keyword_id());

    {
      std::ostringstream channels_str;
      Algs::print(channels_str,
        reader.channels().begin(), reader.channels().end());
      row.add_field(channels_str.str());
    }

    row.add_field(reader.geo_channel_id());
    row.add_field(reader.device_channel_id());
    row.add_field(reader.expression());

    row.add_field(reader.client_app());
    row.add_field(reader.client_app_version());
    row.add_field(reader.browser_version());
    row.add_field(reader.os_version());
    row.add_field(reader.country());

    std::ostringstream cmp_channels_ostr;
    for(RequestInfoProfileReader::cmp_channels_Container::const_iterator
          ch_it = reader.cmp_channels().begin();
        ch_it != reader.cmp_channels().end(); ++ch_it)
    {
      cmp_channels_ostr << (ch_it == reader.cmp_channels().begin() ? "" : " ") <<
        "[ channel_id = " << (*ch_it).channel_id() <<
        ", rate_id = " << (*ch_it).channel_rate_id() <<
        ", imp = " << (*ch_it).impression() <<
        ", sys_imp = " << (*ch_it).sys_impression() <<
        ", adv_imp = " << (*ch_it).adv_impression() <<
        ", click = " << (*ch_it).click() <<
        ", sys_click = " << (*ch_it).sys_click() <<
        ", adv_click = " << (*ch_it).adv_click() << " ]";
    }

    row.add_field(cmp_channels_ostr.str());

    row.add_field(reader.enabled_notice());
    row.add_field(reader.enabled_impression_tracking());
    row.add_field(reader.enabled_action_tracking());

    row.add_field(reader.notice_received());
    row.add_field(reader.notice_non_considered());
    row.add_field(pub_revenue_type_to_string(reader.notice_pub_revenue_type()));

    row.add_field(Generics::Time(reader.impression_time()).gm_ft());
    row.add_field(reader.impression_verified());
    row.add_field(reader.impression_non_considered());
    row.add_field(pub_revenue_type_to_string(reader.impression_pub_revenue_type()));

    row.add_field(Generics::Time(reader.click_time()).gm_ft());
    row.add_field(reader.click_done());
    row.add_field(reader.click_non_considered());

    row.add_field(Generics::Time(reader.action_time()).gm_ft());
    row.add_field(reader.actions_done());
    row.add_field(reader.actions_non_considered());

    row.add_field(Generics::Time(reader.fraud_time()).gm_ft());
    row.add_field(reader.fraud());

    row.add_field(array_to_string(reader.post_impression_actions()));

    row.add_field(reader.campaign_freq());
    row.add_field(reader.referer_hash());
    row.add_field(array_to_string(reader.geo_channels()));
    row.add_field(array_to_string(reader.user_channels()));
    row.add_field(reader.url());
    row.add_field(reader.ip_address());
    row.add_field(reader.ctr_algorithm_id());
    row.add_field(reader.ctr());
    row.add_field(array_to_string(reader.model_ctrs()));
    row.add_field(reader.conv_rate_algorithm_id());
    row.add_field(reader.conv_rate());
    row.add_field(array_to_string(reader.model_conv_rates()));
    row.add_field(reader.self_service_commission());
    row.add_field(reader.adv_commission());
    row.add_field(reader.pub_commission());
    row.add_field(reader.pub_cost_coef());
    row.add_field(reader.at_flags());

    table.add_row(row);
    table.dump(out);
  }
}
}

#endif /* REQUESTINFOPROFILEUTILS_HPP_ */
