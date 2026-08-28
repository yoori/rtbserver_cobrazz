
#include "CmpStat.hpp"
#include <LogCommons/BufferWriter.hpp>
#include <LogCommons/LogCommons.ipp>

namespace AdServer::LogProcessing
{

  template <> const char* CmpStatTraits::B::base_name_ = "CMPStat";
  template <> const char* CmpStatTraits::B::signature_ = "CMPStat";
  template <> const char* CmpStatTraits::B::current_version_ = "3.3";

  const CmpStatInnerKey::DeliveryThresholdT
    CmpStatInnerKey::max_delivery_threshold_value_("1.00000");

  const CmpStatInnerKey_V_2_3::DeliveryThresholdT
    CmpStatInnerKey_V_2_3::max_delivery_threshold_value_("1.00000");

  std::istream&
  operator>>(std::istream& is, CmpStatKey& key)
  {
    is >> key.sdate_;
    read_eol(is);
    is >> key.adv_sdate_;
    read_eol(is);
    is >> key.colo_id_;
    key.calc_hash_();
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CmpStatKey& key)
    /*throw(eh::Exception)*/
  {
    out << key.sdate_ << '\n' << key.adv_sdate_ << '\n' << key.colo_id_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CmpStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    is >> key.publisher_account_id_;
    is >> key.tag_id_;
    is >> key.size_id_;
    is >> key.country_code_;
    is >> key.currency_exchange_id_;
    is >> key.delivery_threshold_;
    is >> key.adv_account_id_;
    is >> key.campaign_id_;
    is >> key.ccg_id_;
    is >> key.cc_id_;
    is >> key.channel_id_;
    is >> key.channel_rate_id_;
    is >> key.fraud_;
    is >> key.walled_garden_;
    if (is)
    {
      key.invariant_();
      key.calc_hash_();
    }
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CmpStatInnerKey& key)
    /*throw(eh::Exception)*/
  {
    key.invariant_();
    out << key.publisher_account_id_ << '\t';
    out << key.tag_id_ << '\t';
    out << key.size_id_ << '\t';
    out << key.country_code_ << '\t';
    out << key.currency_exchange_id_ << '\t';
    out << key.delivery_threshold_ << '\t';
    out << key.adv_account_id_ << '\t';
    out << key.campaign_id_ << '\t';
    out << key.ccg_id_ << '\t';
    out << key.cc_id_ << '\t';
    out << key.channel_id_ << '\t';
    out << key.channel_rate_id_ << '\t';
    out << key.fraud_ << '\t';
    out << key.walled_garden_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CmpStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    is >> data.imps_;
    is >> data.clicks_;
    is >> data.cmp_amount_;
    is >> data.adv_amount_cmp_;
    is >> data.cmp_sys_amount_;
    if (is)
    {
      data.invariant_();
    }
    return is;
  }

  BufferWriter&
  operator<<(BufferWriter& out, const CmpStatInnerData& data)
    /*throw(eh::Exception)*/
  {
    data.invariant_();
    out << data.imps_ << '\t';
    out << data.clicks_ << '\t';
    out << data.cmp_amount_ << '\t';
    out << data.adv_amount_cmp_ << '\t';
    out << data.cmp_sys_amount_;
    return out;
  }

  FixedBufStream<TabCategory>&
  operator>>(FixedBufStream<TabCategory>& is, CmpStatInnerKey_V_2_3& key)
    /*throw(eh::Exception)*/
  {
    is >> key.publisher_account_id_;
    is >> key.tag_id_;
    is >> key.country_code_;
    is >> key.currency_exchange_id_;
    is >> key.delivery_threshold_;
    is >> key.adv_account_id_;
    is >> key.campaign_id_;
    is >> key.ccg_id_;
    is >> key.cc_id_;
    is >> key.channel_id_;
    is >> key.channel_rate_id_;
    is >> key.fraud_;
    is >> key.walled_garden_;
    if (is)
    {
      key.invariant_();
      key.calc_hash_();
    }
    return is;
  }

} // namespace AdServer::LogProcessing
