#ifndef AD_SERVER_LOG_PROCESSING_CAMPAIGN_REFERRER_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_CAMPAIGN_REFERRER_STAT_HPP

#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

namespace AdServer
{
namespace LogProcessing
{
  class CampaignReferrerStatInnerKey_V_3_5
  {
  public:
    CampaignReferrerStatInnerKey_V_3_5()
      : ccg_id_(),
        cc_id_(),
        site_id_(),
        ext_tag_id_(),
        referer_(),
        hash_()
    {}

    CampaignReferrerStatInnerKey_V_3_5(
      unsigned long ccg_id,
      unsigned long cc_id,
      unsigned long site_id,
      const String::SubString& ext_tag_id,
      const String::SubString& referer
      )
      /*throw(eh::Exception)*/
      : ccg_id_(ccg_id),
        cc_id_(cc_id),
        site_id_(site_id),
        ext_tag_id_(ext_tag_id.str()),
        referer_(referer.str()),
        hash_()
    {
      calc_hash();
    }

    bool
    operator==(const CampaignReferrerStatInnerKey_V_3_5& rhs) const
    {
      if (&rhs == this)
      {
        return true;
      }
      return ccg_id_ == rhs.ccg_id_ &&
        cc_id_ == rhs.cc_id_ &&
        site_id_ == rhs.site_id_ &&
        ext_tag_id_.get() == rhs.ext_tag_id_.get() &&
        referer_.get() == rhs.referer_.get();
    }

    unsigned long ccg_id() const
    {
      return ccg_id_;
    }

    unsigned long cc_id() const
    {
      return cc_id_;
    }

    unsigned long site_id() const
    {
      return site_id_;
    }

    const std::string& ext_tag_id() const
    {
      return ext_tag_id_.get();
    }

    const std::string& referer() const
    {
      return referer_.get();
    }

    size_t hash() const
    {
      return hash_;
    }

    friend
    std::istream&
    operator>>(std::istream& is, CampaignReferrerStatInnerKey_V_3_5& key)
      /*throw(eh::Exception)*/;

    friend
    std::ostream&
    operator<<(std::ostream& os, const CampaignReferrerStatInnerKey_V_3_5& key)
      /*throw(eh::Exception)*/;

  private:
    void calc_hash()
    {
      Generics::Murmur64Hash hasher(hash_);
      hash_add(hasher, ccg_id_);
      hash_add(hasher, cc_id_);
      hash_add(hasher, site_id_);
      hash_add(hasher, ext_tag_id_.get());
      hash_add(hasher, referer_.get());
    }

    void invariant() const /*throw(eh::Exception)*/
    {}

    unsigned long ccg_id_;
    unsigned long cc_id_;
    unsigned long site_id_;
    EmptyHolder<Aux_::StringIoWrapper> ext_tag_id_;
    EmptyHolder<Aux_::StringIoWrapper> referer_;
    size_t hash_;
  };

  class CampaignReferrerStatInnerData_V_3_5
  {
  public:
    CampaignReferrerStatInnerData_V_3_5()
      : imps_(),
        clicks_(),
        video_start_(),
        video_view_(),
        video_q1_(),
        video_mid_(),
        video_q3_(),
        video_complete_(),
        video_skip_(),
        video_pause_(),
        video_mute_(),
        video_unmute_(),
        video_resume_(),
        video_fullscreen_(),
        video_error_(),
        adv_amount_(),
        adv_comm_amount_(),
        adv_payable_comm_amount_(),
        pub_amount_adv_(),
        pub_comm_amount_adv_(),
        isp_amount_adv_()
    {}

    CampaignReferrerStatInnerData_V_3_5(
      unsigned long imps,
      unsigned long clicks,
      unsigned long video_start,
      unsigned long video_view,
      unsigned long video_q1,
      unsigned long video_mid,
      unsigned long video_q3,
      unsigned long video_complete,
      unsigned long video_skip,
      unsigned long video_pause,
      unsigned long video_mute,
      unsigned long video_unmute,
      unsigned long video_resume,
      unsigned long video_fullscreen,
      unsigned long video_error,
      const FixedNumber& adv_amount,
      const FixedNumber& adv_comm_amount,
      const FixedNumber& adv_payable_comm_amount,
      const FixedNumber& pub_amount_adv,
      const FixedNumber& pub_comm_amount_adv,
      const FixedNumber& isp_amount_adv)
      /*throw(eh::Exception)*/
      : imps_(imps),
        clicks_(clicks),
        video_start_(video_start),
        video_view_(video_view),
        video_q1_(video_q1),
        video_mid_(video_mid),
        video_q3_(video_q3),
        video_complete_(video_complete),
        video_skip_(video_skip),
        video_pause_(video_pause),
        video_mute_(video_mute),
        video_unmute_(video_unmute),
        video_resume_(video_resume),
        video_fullscreen_(video_fullscreen),
        video_error_(video_error),
        adv_amount_(adv_amount),
        adv_comm_amount_(adv_comm_amount),
        adv_payable_comm_amount_(adv_payable_comm_amount),
        pub_amount_adv_(pub_amount_adv),
        pub_comm_amount_adv_(pub_comm_amount_adv),
        isp_amount_adv_(isp_amount_adv)
    {}

    bool
    operator==(const CampaignReferrerStatInnerData_V_3_5& rhs) const
    {
      return imps_ == rhs.imps_ &&
        clicks_ == rhs.clicks_ &&
        video_start_ == rhs.video_start_ &&
        video_view_ == rhs.video_view_ &&
        video_q1_ == rhs.video_q1_ &&
        video_mid_ == rhs.video_mid_ &&
        video_q3_ == rhs.video_q3_ &&
        video_complete_ == rhs.video_complete_ &&
        video_skip_ == rhs.video_skip_ &&
        video_pause_ == rhs.video_pause_ &&
        video_mute_ == rhs.video_mute_ &&
        video_unmute_ == rhs.video_unmute_ &&
        video_resume_ == rhs.video_resume_ &&
        video_fullscreen_ == rhs.video_fullscreen_ &&
        video_error_ == rhs.video_error_ &&
        adv_amount_ == rhs.adv_amount_ &&
        adv_comm_amount_ == rhs.adv_comm_amount_ &&
        adv_payable_comm_amount_ == rhs.adv_payable_comm_amount_ &&
        pub_amount_adv_ == rhs.pub_amount_adv_ &&
        pub_comm_amount_adv_ == rhs.pub_comm_amount_adv_ &&
        isp_amount_adv_ == rhs.isp_amount_adv_;
    }

    CampaignReferrerStatInnerData_V_3_5&
    operator+=(const CampaignReferrerStatInnerData_V_3_5& rhs)
      /*throw(eh::Exception)*/
    {
      imps_ += rhs.imps_;
      clicks_ += rhs.clicks_;
      video_start_ += rhs.video_start_;
      video_view_ += rhs.video_view_;
      video_q1_ += rhs.video_q1_;
      video_mid_ += rhs.video_mid_;
      video_q3_ += rhs.video_q3_;
      video_complete_ += rhs.video_complete_;
      video_skip_ += rhs.video_skip_;
      video_pause_ += rhs.video_pause_;
      video_mute_ += rhs.video_mute_;
      video_unmute_ += rhs.video_unmute_;
      video_resume_ += rhs.video_resume_;
      video_fullscreen_ += rhs.video_fullscreen_;
      video_error_ += rhs.video_error_;
      adv_amount_ += rhs.adv_amount_;
      adv_comm_amount_ += rhs.adv_comm_amount_;
      adv_payable_comm_amount_ += rhs.adv_payable_comm_amount_;
      pub_amount_adv_ += rhs.pub_amount_adv_;
      pub_comm_amount_adv_ += rhs.pub_comm_amount_adv_;
      isp_amount_adv_ += rhs.isp_amount_adv_;

      return *this;
    }

    unsigned long
    imps() const
    {
      return imps_;
    }

    unsigned long
    clicks() const
    {
      return clicks_;
    }

    unsigned long
    video_start() const
    {
      return video_start_;
    }

    unsigned long
    video_view() const
    {
      return video_view_;
    }

    unsigned long
    video_q1() const
    {
      return video_q1_;
    }

    unsigned long
    video_mid() const
    {
      return video_mid_;
    }

    long
    video_q3() const
    {
      return video_q3_;
    }

    unsigned long
    video_complete() const
    {
      return video_complete_;
    }

    unsigned long
    video_skip() const
    {
      return video_skip_;
    }

    unsigned long
    video_pause() const
    {
      return video_pause_;
    }

    unsigned long
    video_mute() const
    {
      return video_mute_;
    }

    unsigned long
    video_unmute() const
    {
      return video_unmute_;
    }

    unsigned long
    video_resume() const
    {
      return video_resume_;
    }

    unsigned long
    video_fullscreen() const
    {
      return video_fullscreen_;
    }

    unsigned long
    video_error() const
    {
      return video_error_;
    }

    const FixedNumber&
    adv_amount() const
    {
      return adv_amount_;
    }

    const FixedNumber&
    adv_comm_amount() const
    {
      return adv_comm_amount_;
    }

    const FixedNumber&
    adv_payable_comm_amount() const
    {
      return adv_payable_comm_amount_;
    }

    const FixedNumber&
    pub_amount_adv() const
    {
      return pub_amount_adv_;
    }

    const FixedNumber&
    pub_comm_amount_adv() const
    {
      return pub_comm_amount_adv_;
    }

    const FixedNumber&
    isp_amount_adv() const
    {
      return isp_amount_adv_;
    }

    friend std::istream&
    operator>>(std::istream& is, CampaignReferrerStatInnerData_V_3_5& data)
      /*throw(eh::Exception)*/;

    friend std::ostream&
    operator<<(std::ostream& os, const CampaignReferrerStatInnerData_V_3_5& data)
      /*throw(eh::Exception)*/;

  private:
    unsigned long imps_;
    unsigned long clicks_;
    unsigned long video_start_;
    unsigned long video_view_;
    unsigned long video_q1_;
    unsigned long video_mid_;
    unsigned long video_q3_;
    unsigned long video_complete_;
    unsigned long video_skip_;
    unsigned long video_pause_;
    unsigned long video_mute_;
    unsigned long video_unmute_;
    unsigned long video_resume_;
    unsigned long video_fullscreen_;
    unsigned long video_error_;
    FixedNumber adv_amount_;
    FixedNumber adv_comm_amount_;
    FixedNumber adv_payable_comm_amount_;
    FixedNumber pub_amount_adv_;
    FixedNumber pub_comm_amount_adv_;
    FixedNumber isp_amount_adv_;
  };

  struct CampaignReferrerStatKey_V_3_5
  {
    CampaignReferrerStatKey_V_3_5()
      : adv_sdate_(), hash_()
    {}

    CampaignReferrerStatKey_V_3_5(
      const DayTimestamp& adv_sdate
      )
      : adv_sdate_(adv_sdate),
        hash_()
    {
      calc_hash();
    }

    bool
    operator==(const CampaignReferrerStatKey_V_3_5& rhs) const
    {
      if (&rhs == this)
      {
        return true;
      }
      return adv_sdate_ == rhs.adv_sdate_;
    }

  public:
    const DayTimestamp& adv_sdate() const
    {
      return adv_sdate_;
    }

    size_t hash() const
    {
      return hash_;
    }

    friend std::istream&
    operator>>(std::istream& is, CampaignReferrerStatKey_V_3_5& key)
      /*throw(eh::Exception)*/;

    friend std::ostream&
    operator<<(std::ostream& os, const CampaignReferrerStatKey_V_3_5& key)
      /*throw(eh::Exception)*/;

  private:
    void calc_hash()
    {
      Generics::Murmur64Hash hasher(hash_);
      adv_sdate_.hash_add(hasher);
    }

    DayTimestamp adv_sdate_;
    size_t hash_;
  };

  typedef NestedStatCollector<CampaignReferrerStatKey_V_3_5,
    CampaignReferrerStatInnerKey_V_3_5, CampaignReferrerStatInnerData_V_3_5>::Type
      CampaignReferrerStatCollector_V_3_5;

  typedef CampaignReferrerStatInnerKey_V_3_5 CampaignReferrerStatInnerKey;
  typedef CampaignReferrerStatInnerData_V_3_5 CampaignReferrerStatInnerData;
  typedef CampaignReferrerStatKey_V_3_5 CampaignReferrerStatKey;
  typedef CampaignReferrerStatCollector_V_3_5 CampaignReferrerStatCollector;

  struct CampaignReferrerStatTraits: LogDefaultTraits<CampaignReferrerStatCollector>
  {
    template <typename Functor>
    static void
    for_each_old(Functor& /*f*/) /*throw(eh::Exception)*/
    {}
  };

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CAMPAIGN_REFERRER_STAT_HPP */

