#ifndef _CAMPAIGN_SELECTOR_HPP_
#define _CAMPAIGN_SELECTOR_HPP_

#include <map>
#include <cassert>

#include <Commons/Containers.hpp>

#include "CampaignManagerDeclarations.hpp"
#include "CampaignConfig.hpp"
#include "CampaignIndex.hpp"
#include "CampaignSelectParams.hpp"
#include "CTRProvider.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    using namespace AdInstances;

    /**
     * CampaignSelector
     *   use CampaignSelectionIndex
     *   perform non index campaign filters:
     *     1. exclude colocations
     *     2. freq caps
     *     3. daily run intervals
     *     4. creative click url filter
     */
    class CampaignSelector
    {
    public:
      struct RequestKeywords // TO CHECK
      {
        typedef Generics::GnuHashSet<Generics::StringHashAdapter> KeywordSet;
        KeywordSet context_words;
      };

      struct WeightedCampaign
      {
        WeightedCampaign(
          const Tag* tag_val,
          const Tag::TagPricing* tag_pricing_val,
          const Tag::Size* tag_size_val,
          const Campaign* campaign_val,
          const Creative* creative_val,
          CampaignIndex::ConstCreativePtrList& available_creatives_val,
            // c-tor clear this collection,
          const RevenueDecimal& ecpm_val,
          const RevenueDecimal& ctr_val,
          const RevenueDecimal& conv_rate_val)
          : tag(tag_val),
            tag_pricing(tag_pricing_val),
            tag_size(tag_size_val),
            campaign(campaign_val),
            creative(creative_val),
            ecpm(ecpm_val),
            ctr(ctr_val),
            conv_rate(conv_rate_val)
        {
          available_creatives.swap(available_creatives_val);
          assert(ctr.is_nonnegative());
        }

        const Tag* tag;
        const Tag::TagPricing* tag_pricing;
        const Tag::Size* tag_size;
        const Campaign* campaign;
        const Creative* creative;
        CampaignIndex::ConstCreativePtrList available_creatives;
        RevenueDecimal ecpm;
        RevenueDecimal ctr;
        RevenueDecimal conv_rate;
      };

      typedef std::unique_ptr<WeightedCampaign> WeightedCampaignPtr;

      struct WeightedCampaignKeyword
      {
        WeightedCampaignKeyword(
          const Tag* tag_val,
          CampaignKeyword* campaign_keyword_val,
          const Campaign* campaign_val,
          const Creative* creative_val,
          const RevenueDecimal& actual_ecpm_val,
          const RevenueDecimal& actual_cpc_val,
          const RevenueDecimal& ecpm_val,
          const RevenueDecimal& ctr_val,
          const RevenueDecimal& conv_rate_val
          )
          : tag(tag_val),
            campaign_keyword(ReferenceCounting::add_ref(campaign_keyword_val)),
            campaign(campaign_val),
            creative(creative_val),
            actual_ecpm(actual_ecpm_val),
            actual_cpc(actual_cpc_val),
            ecpm(ecpm_val),
            ctr(ctr_val),
            conv_rate(conv_rate_val)
        {
          assert(ctr.is_nonnegative());
          assert(conv_rate.is_nonnegative());
        }

        const Tag* tag;
        CampaignKeyword_var campaign_keyword;
        const Campaign* campaign;
        const Creative* creative;

        RevenueDecimal actual_ecpm; // ecpm_bid
        RevenueDecimal actual_cpc;
        RevenueDecimal ecpm;
        RevenueDecimal ctr;
        RevenueDecimal conv_rate;
      };

      typedef std::list<WeightedCampaignKeyword>
        WeightedCampaignKeywordList;

      typedef std::unique_ptr<WeightedCampaignKeywordList>
        WeightedCampaignKeywordListPtr;

      typedef std::vector<WeightedCampaignKeyword*>
        WeightedCampaignKeywordPtrArray;

      typedef std::list<WeightedCampaignKeywordPtrArray>
        WeightedCampaignKeywordGroupList;

      class ExpectedEcpm
      {
      public:
        ExpectedEcpm() noexcept;

        RevenueDecimal
        value() const noexcept;

        ExpectedEcpm&
        operator+= (const RevenueDecimal& arg) noexcept;

      private:
        ExtRevenueDecimal ex_ecpm_quad_sum_;
        ExtRevenueDecimal ex_ecpm_sum_;
      };

    public:
      CampaignSelector(
        const CampaignIndex* campaign_index,
        const CTRProvider* ctr_provider,
        const CTRProvider* conv_rate_provider);

      /* facade that do all ops */
      void
      select_campaigns(
        AuctionType auction_type,
        AuctionType second_auction_type,
        const CampaignSelectParams* request_params,
        const ChannelIdHashSet& channels,
        const CampaignKeywordMap& hit_keywords,
        bool collect_lost,
        WeightedCampaignKeywordListPtr& weighted_campaign_keywords,
        WeightedCampaignPtr& weighted_campaign,
        AdSelectionResult& select_result);

    protected:
      typedef std::map<RevenueDecimal, WeightedCampaignKeywordList>
        CPCKeywordMap;

      typedef std::list<WeightedCampaignPtr> WeightedCampaignList;

      typedef std::multimap<RevenueDecimal, WeightedCampaignKeywordPtrArray>
        ExpRevWeightedCampaignKeywordMap;

      typedef std::map<unsigned long, WeightedCampaignKeywordPtrArray>
        IdWeightedCampaignKeywordMap;

      struct IdWeightedCampaignKeywordMaps
      {
        IdWeightedCampaignKeywordMap account_campaigns;
        IdWeightedCampaignKeywordMap advertiser_campaigns;
        IdWeightedCampaignKeywordMap ccg_campaigns;
      };

      static
      void
      fill_id_weighted_campaigns_keyword_maps(
        IdWeightedCampaignKeywordMaps& campaigns,
        WeightedCampaignKeywordList& text_campaign_candidates);

      static
      void
      get_campaigns_group(
        WeightedCampaignKeywordPtrArray*& campaigns_group,
        IdWeightedCampaignKeywordMaps& select_from,
        WeightedCampaignKeyword* wc);

    protected:
      void
      select_campaigns_randomly_(
        WeightedCampaignKeywordListPtr& result_weighted_campaign_keywords,
        WeightedCampaignPtr& weighted_campaign,
        AdSelectionResult& select_result,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Tag* tag,
        const CTRProvider::Calculation* ctr_calculation,
        const CTRProvider::Calculation* conv_rate_calculation,
        const Tag::SizeMap& tag_sizes,
        const ChannelIdHashSet& channels,
        const CampaignKeywordMap& hit_keywords)
        const
        noexcept;

      void
      select_campaigns_prop_probability_(
        WeightedCampaignKeywordListPtr& result_weighted_campaign_keywords,
        WeightedCampaignPtr& weighted_campaign,
        AdSelectionResult& select_result,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Tag* tag,
        const CTRProvider::Calculation* ctr_calculation,
        const CTRProvider::Calculation* conv_rate_calculation,
        const Tag::SizeMap& tag_sizes,
        const ChannelIdHashSet& matched_channels,
        const CampaignKeywordMap& matched_keywords,
        bool collect_lost)
        const
        noexcept;

      void
      select_campaigns_max_ecpm_(
        WeightedCampaignKeywordListPtr& result_weighted_campaign_keywords,
        WeightedCampaignPtr& weighted_campaign,
        AdSelectionResult& select_result,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Tag* tag,
        const Tag::SizeMap& tag_sizes,
        const CTRProvider::Calculation* ctr_calculation,
        const CTRProvider::Calculation* conv_rate_calculation,
        const ChannelIdHashSet& channels,
        const CampaignKeywordMap& hit_keywords,
        bool collect_lost)
        const
        noexcept;

      // default CTR algorithm variant
      void
      get_all_display_campaign_candidates_(
        WeightedCampaignList& result_campaign_candidates,
        CampaignIndex::CampaignSelectionCellPtrList* lost_campaigns,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Tag* tag,
        const ChannelIdHashSet& matched_channels,
        const CampaignIndex::CampaignSelectionCellPtrList& campaign_list,
        bool check_min_ecpm)
        const
        noexcept;

      // non default CTR algorithm variant
      void
      get_all_display_campaign_candidates_(
        WeightedCampaignList& result_campaign_candidates,
        CampaignIndex::CampaignSelectionCellPtrList* lost_campaigns,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Tag* tag,
        const CTRProvider::Calculation* ctr_calculation,
        const CTRProvider::Calculation* conv_rate_calculation,
        const ChannelIdHashSet& matched_channels,
        const CampaignIndex::CampaignSelectionCellPtrList& campaign_list,
        bool check_min_ecpm)
        const
        noexcept;

      void
      get_max_display_campaign_candidates_(
        WeightedCampaignList& result_campaign_candidates,
        LostAuction* lost_auction,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Tag* tag,
        const ChannelIdHashSet& matched_channels,
        const CampaignIndex::CampaignSelectionCellPtrList& campaign_list,
        const CampaignIndex::CampaignCellPtrList& lost_campaigns)
        const
        noexcept;

      void
      get_text_campaign_candidates_(
        WeightedCampaignKeywordList& result_campaign_candidates,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Tag* tag,
        const CTRProvider::CalculationContext* ctr_calculation,
        const CampaignKeywordMap& matched_keywords,
        const ChannelIdHashSet& matched_channels,
        const CampaignIndex::CampaignCellPtrList& keyword_campaigns,
        const CampaignIndex::CampaignCellPtrList& nonkeyword_campaigns)
        const
        noexcept;

      WeightedCampaignPtr
      select_display_campaign_(
        LostAuction* lost_auction,
        AuctionType auction_type,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Tag* tag,
        const CTRProvider::Calculation* ctr_calculation,
        const CTRProvider::Calculation* conv_rate_calculation,
        const ChannelIdHashSet& matched_channels,
        const CampaignIndex::CampaignSelectionCellPtrList& candidates,
        const CampaignIndex::CampaignCellPtrList& lost_campaigns)
        const
        noexcept;

      static
      void
      revert_cost_(WeightedCampaignKeywordList& weighted_campaign_keywords)
        noexcept;

      void
      check_text_campaign_channels_(
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const ChannelIdHashSet& matched_channels,
        const CampaignIndex::CampaignCellPtrList& campaign_list,
        ConstCampaignPtrList& filtered_campaigns)
        const
        noexcept;

      /* cross */
      void
      cross_campaigns_with_keywords_(
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const ChannelIdHashSet& matched_channels,
        const CampaignIndex::CampaignCellPtrList& campaigns,
        const CampaignKeywordMap& campaign_keywords,
        CampaignKeywordMap& filtered_campaign_keywords)
        const
        noexcept;

      static
      RevenueDecimal
      eval_text_campaigns_max_sum_(
        const ExpRevWeightedCampaignKeywordMap& grouped_text_campaign_candidates,
        unsigned long max_text_creatives)
        noexcept;

      template<typename EcpmOutType>
      static
      void
      group_id_map_(
        ExpRevWeightedCampaignKeywordMap& text_campaign_map,
        EcpmOutType& expected_ecpm,
        const IdWeightedCampaignKeywordMap& campaigns)
        noexcept;

      static
      void
      group_id_map_(
        WeightedCampaignKeywordGroupList& text_campaign_map,
        IdWeightedCampaignKeywordMap& campaigns)
        noexcept;

      static
      void
      group_text_campaigns_(
        ExpRevWeightedCampaignKeywordMap& text_campaign_map,
        ExpectedEcpm& expected_ecpm,
        WeightedCampaignKeywordList& text_campaign_candidates)
        noexcept;

      static
      void
      group_text_campaigns_(
        ExpRevWeightedCampaignKeywordMap& text_campaign_map,
        RevenueDecimal& ecpm_sum,
        WeightedCampaignKeywordPtrArray& text_campaign_candidates)
        noexcept;

      static
      void
      group_text_campaigns_(
        WeightedCampaignKeywordGroupList& text_campaign_map,
        WeightedCampaignKeywordList& text_campaign_candidates)
        noexcept;

      static
      void
      convert_text_candidates_to_cpc_map_(
        CPCKeywordMap& cpc_keyword_map,
        const WeightedCampaignKeywordList& text_campaign_candidates)
        noexcept;

      void
      filter_text_campaign_candidates_(
        WeightedCampaignKeywordList& filtered_text_campaigns,
        const WeightedCampaignKeywordList& text_campaigns,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Tag* tag,
        const Tag::Size* tag_size,
        const CTRProvider::CalculationContext* ctr_calculation_context,
        const CTRProvider::CalculationContext* conv_rate_calculation_context)
        const
        noexcept;

      bool
      select_text_campaigns_randomly_(
        WeightedCampaignKeywordList& result_text_campaigns,
        WeightedCampaignKeywordGroupList& text_campaign_candidates,
        unsigned long max_text_creatives)
        const
        noexcept;

      static
      bool
      select_text_campaigns_prop_probability_(
        WeightedCampaignKeywordList& result_text_campaigns,
        const WeightedCampaignKeywordList& text_campaign_candidates,
        const RevenueDecimal& min_ecpm,
        unsigned long max_text_creatives)
        noexcept;

      bool
      select_campaign_keywords_n_(
        LostAuction* lost_auction,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Tag* tag,
        const Tag::Size* tag_size,
        const CTRProvider::CalculationContext* ctr_calculation,
        const CPCKeywordMap& cpc_keyword_map,
        const RevenueDecimal& min_sum_ecpm,
        unsigned long max_keywords,
        WeightedCampaignKeywordList& weighted_campaign_keywords)
        const
        noexcept;

      void
      filter_creatives_(
        CampaignIndex::ConstCreativePtrList& available_creatives,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Campaign* campaign,
        const Tag* tag) const
        /*throw(eh::Exception)*/;

      static
      RevenueDecimal
      sum_campaign_ecpm_(const WeightedCampaignList& campaign_candidates)
        noexcept;

      static
      RevenueDecimal
      campaign_ecpm_expected_value_(
        const WeightedCampaignList& campaign_candidates)
        noexcept;

      static
      WeightedCampaignList::iterator
      select_campaign_prop_probability_(
        WeightedCampaignList& campaign_candidates,
        const RevenueDecimal& ecpm_offset)
        noexcept;

      static
      WeightedCampaignList::iterator
      select_campaign_randomly_(//not const due to bug in CustomAutoPtr on Centos6
        WeightedCampaignList& campaign_candidates)
        noexcept;

      const Creative*
      select_display_creative_(
        CampaignIndex::ConstCreativePtrList& available_creatives) const
        /*throw(eh::Exception)*/;

      static
      const Tag::Size*
      select_tag_size_(const Tag* tag, const Creative* creative)
        noexcept;

      // Filter lost creatives and campaigns
      // ex collect_max_ecpm_lost_
      template<typename CampaignHolderIteratorType>
      void collect_lost_(
        LostAuction& lost_auction,
        const CampaignIndex::Key& key,
        const CampaignSelectParams& request_params,
        const Tag* tag,
        const ChannelIdHashSet& matched_channels,
        CampaignHolderIteratorType begin,
        CampaignHolderIteratorType end)
        const
        noexcept;

      static RevenueDecimal
      campaign_ctr_(
        const Creative*& result_creative,
        const CTRProvider::CalculationContext* ctr_calculation_context,
        const CampaignIndex::ConstCreativePtrList& available_creatives)
        noexcept;

      static RevenueDecimal
      default_campaign_ecpm_(
        const Tag* tag,
        const Campaign* campaign)
        noexcept;

      static RevenueDecimal
      campaign_ecpm_(
        RevenueDecimal& ctr,
        const Creative*& result_creative,
        const CTRProvider::CalculationContext* ctr_calculation_context,
        const Campaign* campaign,
        const CampaignIndex::ConstCreativePtrList& available_creatives)
        noexcept;

      static RevenueDecimal
      default_campaign_keyword_ecpm_(
        const Tag* tag,
        const Campaign* campaign,
        const CampaignKeyword* campaign_keyword)
        noexcept;

      static RevenueDecimal
      campaign_keyword_ecpm_(
        RevenueDecimal& ctr,
        const Creative*& result_creative,
        const CTRProvider::CalculationContext* ctr_calculation,
        const Campaign* campaign,
        const CampaignKeyword* campaign_keyword,
        const CampaignIndex::ConstCreativePtrList& available_creatives)
        noexcept;

      static bool
      check_min_ecpm_(
        const Tag::TagPricing* tag_pricing,
        const RevenueDecimal& min_ecpm,
        const RevenueDecimal& campaign_ecpm);

    private:
      const CampaignIndex* campaign_selection_index_;
      ConstCampaignConfig_var campaign_config_;
      ConstCTRProvider_var ctr_provider_;
      ConstCTRProvider_var conv_rate_provider_;
    };
  }
}

#endif /*_CAMPAIGN_SELECTOR_HPP_*/
