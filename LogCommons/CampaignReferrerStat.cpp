
#include "CampaignReferrerStat.hpp"
#include <LogCommons/LogCommons.ipp>

namespace AdServer
{
namespace LogProcessing
{
  template <> const char* CampaignReferrerStatTraits::B::base_name_ =
    "CampaignReferrerStat";
  template <> const char* CampaignReferrerStatTraits::B::signature_ =
    "CampaignReferrerStat";
  template <> const char* CampaignReferrerStatTraits::B::current_version_ =
    "3.5";

  std::istream&
  operator>>(std::istream& is, CampaignReferrerStatKey_V_3_5& key)
    /*throw(eh::Exception)*/
  {
    is >> key.adv_sdate_;
    key.calc_hash();
    return is;
  }

  std::ostream&
  operator<<(std::ostream& os, const CampaignReferrerStatKey_V_3_5& key)
    /*throw(eh::Exception)*/
  {
    os << key.adv_sdate_;
    return os;
  }

  std::istream&
  operator>>(std::istream& is, CampaignReferrerStatInnerKey_V_3_5& key)
    /*throw(eh::Exception)*/
  {
    is >> key.ccg_id_;
    read_tab(is);
    is >> key.cc_id_;
    read_tab(is);
    is >> key.site_id_;
    read_tab(is);
    is >> key.ext_tag_id_;
    read_tab(is);
    is >> key.referer_;
    if (is)
    {
      key.invariant();
      key.calc_hash();
    }
    return is;
  }

  std::ostream&
  operator<<(std::ostream& os, const CampaignReferrerStatInnerKey_V_3_5& key)
    /*throw(eh::Exception)*/
  {
    key.invariant();
    os << key.ccg_id_ << '\t';
    os << key.cc_id_ << '\t';
    os << key.site_id_ << '\t';
    os << key.ext_tag_id_ << '\t';
    os << key.referer_;
    return os;
  }

  std::istream&
  operator>>(std::istream& is, CampaignReferrerStatInnerData_V_3_5& data)
    /*throw(eh::Exception)*/
  {
    is >> data.imps_;
    read_tab(is);
    is >> data.clicks_;
    read_tab(is);
    is >> data.video_start_;
    read_tab(is);
    is >> data.video_view_;
    read_tab(is);
    is >> data.video_q1_;
    read_tab(is);
    is >> data.video_mid_;
    read_tab(is);
    is >> data.video_q3_;
    read_tab(is);
    is >> data.video_complete_;
    read_tab(is);
    is >> data.video_skip_;
    read_tab(is);
    is >> data.video_pause_;
    read_tab(is);
    is >> data.video_mute_;
    read_tab(is);
    is >> data.video_unmute_;
    read_tab(is);
    is >> data.video_resume_;
    read_tab(is);
    is >> data.video_fullscreen_;
    read_tab(is);
    is >> data.video_error_;
    read_tab(is);
    is >> data.adv_amount_;
    read_tab(is);
    is >> data.adv_comm_amount_;
    read_tab(is);
    is >> data.adv_payable_comm_amount_;
    read_tab(is);
    is >> data.pub_amount_adv_;
    read_tab(is);
    is >> data.pub_comm_amount_adv_;
    read_tab(is);
    is >> data.isp_amount_adv_;

    return is;
  }

  std::ostream&
  operator<<(std::ostream& os, const CampaignReferrerStatInnerData_V_3_5& data)
    /*throw(eh::Exception)*/
  {
    os << data.imps_ << '\t';
    os << data.clicks_ << '\t';
    os << data.video_start_ << '\t';
    os << data.video_view_ << '\t';
    os << data.video_q1_ << '\t';
    os << data.video_mid_ << '\t';
    os << data.video_q3_ << '\t';
    os << data.video_complete_ << '\t';
    os << data.video_skip_ << '\t';
    os << data.video_pause_ << '\t';

    os << data.video_mute_ << '\t';
    os << data.video_unmute_ << '\t';
    os << data.video_resume_ << '\t';
    os << data.video_fullscreen_ << '\t';
    os << data.video_error_ << '\t';
    os << data.adv_amount_ << '\t';
    os << data.adv_comm_amount_ << '\t';
    os << data.adv_payable_comm_amount_ << '\t';
    os << data.pub_amount_adv_ << '\t';
    os << data.pub_comm_amount_adv_ << '\t';
    os << data.isp_amount_adv_;

    return os;
  }
} // namespace LogProcessing
} // namespace AdServer

