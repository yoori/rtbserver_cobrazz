#ifndef _AD_SERVER_CAMPAIGN_SVCS_CAMPAIGN_MANAGER_DECLARATIONS_HPP_
#define _AD_SERVER_CAMPAIGN_SVCS_CAMPAIGN_MANAGER_DECLARATIONS_HPP_

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Commons/UserInfoManip.hpp>

#include "CampaignConfig.hpp"
#include "CTRProvider.hpp"

namespace Aspect
{
  const char CAMPAIGN_MANAGER[] = "CampaignManager";
  const char TRAFFICKING_PROBLEM[] = "TraffickingProblem";
  const char ADVERTIZER_PROBLEM[] = "AdvertizerProblem";
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    using namespace AdInstances;

    typedef std::list<unsigned long> CCGIdList;

    struct CampaignKeyword:
      public virtual ReferenceCounting::DefaultImpl<>,
      public CampaignKeywordBase
    {
      unsigned long channel_id;
      RevenueDecimal max_cpc; // in account currency
      RevenueDecimal ctr;
      const Campaign* campaign;
      std::string click_url_domain;
      RevenueDecimal ecpm; // system currency

    private:
      virtual ~CampaignKeyword() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<CampaignKeyword> CampaignKeyword_var;    
    
    struct CampaignSelectionData
    {
      CampaignSelectionData()
        : campaign(0),
          creative(0),
          ecpm(0),
          ecpm_bid(0),
          actual_cpc(0),
          ctr(RevenueDecimal::ZERO),
          conv_rate(RevenueDecimal::ZERO),
          campaign_imps(0),
          count_impression(false),
          track_impr(true),
          selection_done(true)
      {}

      AdServer::Commons::RequestId request_id;

      const Campaign* campaign;
      const Creative* creative;
      CampaignKeyword_var campaign_keyword;

      RevenueDecimal ecpm;
      RevenueDecimal ecpm_bid; // can be non equal ecpm for keyword/text campaigns

      RevenueDecimal actual_cpc; // in account currency
      RevenueDecimal ctr;
      RevenueDecimal conv_rate;
      unsigned long campaign_imps;

      std::string responded_expression;
      ChannelIdList responded_channels;
      bool count_impression; // FIXME: redundant, always equals !track_impr
      bool track_impr; // FIXME: should be moved to AdSelectionResult level
      bool selection_done;

      std::string click_url;
    };

    typedef std::list<CampaignSelectionData> CampaignSelectionDataList;

    typedef std::multimap<unsigned long, CampaignKeyword_var>
      CampaignKeywordMap;

    typedef std::list<const Campaign*> ConstCampaignPtrList;

    typedef std::set<const Campaign*> ConstCampaignPtrSet;
    typedef std::set<const Creative*> ConstCreativePtrSet;

    struct LostAuction
    {
      void swap(LostAuction& right)
      {
        ccgs.swap(right.ccgs);
        creatives.swap(right.creatives);
      }

      ConstCampaignPtrSet ccgs;
      ConstCreativePtrSet creatives;
    };

    struct AdSelectionResult
    {
      AdSelectionResult()
        : text_campaigns(false),
          min_no_adv_ecpm(RevenueDecimal::ZERO),
          min_text_ecpm(RevenueDecimal::ZERO),
          cpm_threshold(RevenueDecimal::ZERO),
          tag(0),
          tag_pricing(0),
          tag_size(0),
          walled_garden(false),
          household_based(false),
          auction_type(AT_RANDOM)
      {}

      bool text_campaigns;

      RevenueDecimal min_no_adv_ecpm;
      RevenueDecimal min_text_ecpm;

      RevenueDecimal cpm_threshold;  // maximum tag cpm when this ad's will be served
      const Tag* tag;
      const Tag::TagPricing* tag_pricing;
      const Tag::Size* tag_size;
      CTR::CTRProvider::Calculation_var ctr_calculation;
      CTR::CTRProvider::Calculation_var conv_rate_calculation;
      CampaignSelectionDataList selected_campaigns;

      FreqCapIdSet freq_caps;
      FreqCapIdSet uc_freq_caps;

      bool walled_garden;
      bool household_based;

      AuctionType auction_type;
      LostAuction lost_auction;
    };

    typedef std::map<std::string, CreativeInstantiateRule>
      CreativeInstantiateRuleMap;
  } // namespace CampaignSvcs
} // namespace AdServer

#endif // _AD_SERVER_CAMPAIGN_SVCS_CAMPAIGN_MANAGER_DECLARATIONS_HPP_
