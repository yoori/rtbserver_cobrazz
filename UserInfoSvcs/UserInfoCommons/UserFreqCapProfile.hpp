#ifndef USERINFOCOMMONS_FREQCAPPROFILE_HPP
#define USERINFOCOMMONS_FREQCAPPROFILE_HPP

#include <Generics/MemBuf.hpp>
#include <Commons/UserInfoManip.hpp>
#include "Allocator.hpp"
#include "FreqCapConfig.hpp"

#include <UserInfoSvcs/UserInfoCommons/UserFreqCapProfileDescription.hpp>

namespace AdServer
{
namespace UserInfoSvcs
{
  static const unsigned long
  CURRENT_FREQ_CAP_PROFILE_VERSION = 351;

  class UserFreqCapProfile
  {
  public:
    struct SeqOrder
    {
      SeqOrder() 
      {}

      SeqOrder(
        unsigned long ccg_id_val,
        unsigned long set_id_val,
        unsigned long imps_val)
        :
        ccg_id(ccg_id_val),
        set_id(set_id_val),
        imps(imps_val)
      {}

      unsigned long ccg_id;
      unsigned long set_id;
      unsigned long imps;
    };

    struct CampaignFreq
    {
      unsigned long campaign_id;
      unsigned long imps;
    };

    typedef std::list<SeqOrder> SeqOrderList;
    typedef std::list<unsigned long> FreqCapIdList;
    typedef std::list<CampaignFreq> CampaignFreqs;
    typedef std::list<unsigned long> CampaignIds;

  public:
    DECLARE_EXCEPTION(Invalid, eh::DescriptiveException);

    UserFreqCapProfile(ConstSmartMemBufPtr plain_profile)
      /*throw(Invalid)*/;

    bool
    full(FreqCapIdList& fcs,
      FreqCapIdList* virtual_fcs,
      SeqOrderList& seq_orders,
      CampaignFreqs& campaign_freqs,
      const Generics::Time& now,
      const FreqCapConfig& fc_config)
      /*throw(eh::Exception)*/;

    void
    consider(const Commons::RequestId& request_id,
      const Generics::Time& now,
      const FreqCapIdList& fcs, // sorted
      const FreqCapIdList& uc_fcs, // sorted
      const FreqCapIdList& virtual_fcs, // sorted
      const SeqOrderList& seq_orders,
      const CampaignIds& campaign_ids,
      const CampaignIds& uc_campaign_ids,
      const FreqCapConfig& fc_config) noexcept;

    bool
    confirm_request(
      const Commons::RequestId& request_id,
      const Generics::Time& now,
      const FreqCapConfig& fc_config) noexcept;

    void
    merge(
      ConstSmartMemBufPtr merge_profile,
      const Generics::Time& now,
      const FreqCapConfig& fc_config)
      /*throw(eh::Exception)*/;

    bool
    consider_publishers_optin(
      const std::set<unsigned long>& publisher_account_ids,
      const Generics::Time& timestamp) noexcept;
    
      void
    get_optin_publishers(
      std::list<unsigned long>& optin_publishers,
      const Generics::Time& time)
      /*throw(eh::Exception)*/;
    
    void
    print(
      std::ostream& out,
      const FreqCapConfig* fc_config) const
      /*throw(eh::Exception)*/;

    ConstSmartMemBuf_var
    transfer_membuf() noexcept
    {
      return Generics::transfer_membuf(plain_profile_);
    };

  private:
    SmartMemBuf_var plain_profile_;
  };
}
}

#endif /*USERINFOCOMMONS_FREQCAPPROFILE_HPP*/
