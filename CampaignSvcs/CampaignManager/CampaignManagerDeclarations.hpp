#pragma once

#include <list>
#include <memory>
#include <set>
#include <vector>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Generics/MonoAllocator.hpp>
#include <Commons/UserInfoManip.hpp>

#include "CampaignConfig.hpp"
#include "CTRProvider.hpp"

namespace Aspect
{
  const char CAMPAIGN_MANAGER[] = "CampaignManager";
  const char TRAFFICKING_PROBLEM[] = "TraffickingProblem";
  const char ADVERTIZER_PROBLEM[] = "AdvertizerProblem";
}

namespace AdServer::CampaignSvcs
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

    bool count_impression; // FIXME: redundant, always equals !track_impr
    bool track_impr; // FIXME: should be moved to AdSelectionResult level
    bool selection_done;

    std::string click_url;
  };

  using CampaignSelectionDataList = Generics::MonoList<CampaignSelectionData>;

  using CampaignKeywordMap = Generics::MonoMultiMap<unsigned long, CampaignKeyword_var>;

  using FreqCapIdArray = Generics::MonoVector<unsigned long>;

  using ConstCampaignPtrArray = std::vector<const Campaign*>;

  typedef std::set<const Campaign*> ConstCampaignPtrSet;
  typedef std::set<const Creative*> ConstCreativePtrSet;

  struct AdSelectionResult
  {
    AdSelectionResult()
      : AdSelectionResult(std::make_shared<Generics::MonoAllocatorArena>())
    {}

    explicit AdSelectionResult(std::shared_ptr<Generics::MonoAllocatorArena> arena)
      : arena_(arena ? std::move(arena) : std::make_shared<Generics::MonoAllocatorArena>()),
        text_campaigns(false),
        min_no_adv_ecpm(RevenueDecimal::ZERO),
        min_text_ecpm(RevenueDecimal::ZERO),
        cpm_threshold(RevenueDecimal::ZERO),
        tag(0),
        tag_pricing(0),
        tag_size(0),
        selected_campaigns(Generics::MonoAllocator<CampaignSelectionData>{arena_.get()}),
        freq_caps(Generics::MonoAllocator<unsigned long>{arena_.get()}),
        uc_freq_caps(Generics::MonoAllocator<unsigned long>{arena_.get()}),
        walled_garden(false),
        household_based(false),
        auction_type(AT_RANDOM)
    {}

    AdSelectionResult(
      const AdSelectionResult& init,
      std::shared_ptr<Generics::MonoAllocatorArena> arena)
      : arena_(arena ? std::move(arena) : std::make_shared<Generics::MonoAllocatorArena>()),
        text_campaigns(init.text_campaigns),
        min_no_adv_ecpm(init.min_no_adv_ecpm),
        min_text_ecpm(init.min_text_ecpm),
        cpm_threshold(init.cpm_threshold),
        tag(init.tag),
        tag_pricing(init.tag_pricing),
        tag_size(init.tag_size),
        ctr_calculation(init.ctr_calculation),
        conv_rate_calculation(init.conv_rate_calculation),
        selected_campaigns(
          init.selected_campaigns,
          Generics::MonoAllocator<CampaignSelectionData>{arena_.get()}),
        freq_caps(
          init.freq_caps,
          Generics::MonoAllocator<unsigned long>{arena_.get()}),
        uc_freq_caps(
          init.uc_freq_caps,
          Generics::MonoAllocator<unsigned long>{arena_.get()}),
        walled_garden(init.walled_garden),
        household_based(init.household_based),
        auction_type(init.auction_type)
    {}

    const std::shared_ptr<Generics::MonoAllocatorArena>&
    arena() const noexcept
    {
      return arena_;
    }

  private:
    std::shared_ptr<Generics::MonoAllocatorArena> arena_;

  public:
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

    FreqCapIdArray freq_caps;
    FreqCapIdArray uc_freq_caps;

    bool walled_garden;
    bool household_based;

    AuctionType auction_type;
  };
} // namespace AdServer::CampaignSvcs
