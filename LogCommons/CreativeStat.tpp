#pragma once

namespace AdServer::LogProcessing
{
  template <class Archive>
  void
  CreativeStatInnerKey_V_3_3::serialize(Archive& archive)
  {
    (archive
      & colo_id_
      & publisher_account_id_
      & tag_id_
      & size_id_
      & country_code_
      & adv_account_id_
      & campaign_id_
      & ccg_id_
      & cc_id_
      & ccg_rate_id_
      & colo_rate_id_
      & site_rate_id_
      & currency_exchange_id_
      & delivery_threshold_
      & num_shown_
      & position_
      & test_
      & fraud_
      & walled_garden_
      & user_status_
      & geo_channel_id_
      & device_channel_id_
      & ctr_reset_id_)
      ^ hid_profile_;
  }

  template <class Archive>
  void
  CreativeStatInnerKey::serialize(Archive& archive)
  {
    (archive
      & colo_id_
      & publisher_account_id_
      & tag_id_
      & size_id_
      & country_code_
      & adv_account_id_
      & campaign_id_
      & ccg_id_
      & cc_id_
      & ccg_rate_id_
      & colo_rate_id_
      & site_rate_id_
      & currency_exchange_id_
      & delivery_threshold_
      & num_shown_
      & position_
      & test_
      & fraud_
      & walled_garden_
      & user_status_
      & geo_channel_id_
      & device_channel_id_
      & ctr_reset_id_
      & hid_profile_)
      ^ viewability_;
  }

  template <class Archive>
  void
  CreativeStatInnerData::serialize(Archive& archive)
  {
    (archive
      & unverified_imps_
      & imps_
      & clicks_
      & actions_
      & adv_amount_
      & pub_amount_
      & isp_amount_
      & adv_comm_amount_
      & pub_comm_amount_
      & adv_payable_comm_amount_
      & pub_advcurrency_amount_)
      ^ isp_advcurrency_amount_;
  }

  template <class Functor>
  void
  CreativeStatTraits::for_each_old(Functor& functor)
    /*throw(eh::Exception)*/
  {
    functor.template operator()<CreativeStatCollector_V_3_3, true>("3.3");
  }
}
