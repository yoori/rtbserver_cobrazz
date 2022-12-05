#include <math.h>
#include <assert.h>
#include <iostream>

#include <Generics/GnuHashTable.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Generics/RandomSelect.hpp>
#include <Generics/Rand.hpp>

#include <Commons/Algs.hpp>

#include "CampaignSelector.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    struct CampaignKeywordCreative
    {
      CampaignKeywordCreative(
        const CampaignSelector::WeightedCampaignKeyword& weighted_campaign_keyword_val,
        CampaignIndex::ConstCreativePtrList& available_creatives_val,
        const Creative* creative_val)
        : weighted_campaign_keyword(weighted_campaign_keyword_val),
          creative(creative_val)
      {
        available_creatives.swap(available_creatives_val);
      }

      CampaignSelector::WeightedCampaignKeyword weighted_campaign_keyword;
      CampaignIndex::ConstCreativePtrList available_creatives;
      const Creative* creative;
    };

    typedef std::list<CampaignKeywordCreative>
      CampaignKeywordCreativeList;

    typedef std::map<RevenueDecimal, CampaignKeywordCreativeList>
      CPCKeywordCreativeMap;

    struct TextSelectionBySize
    {
      Tag::SizeMap tag_sizes;
      RevenueDecimal expected_ecpm;
      CampaignSelector::WeightedCampaignKeywordList campaign_candidates;
    };

    typedef std::list<TextSelectionBySize> TextSelectionBySizeList;

    struct RandomTextSelectionBySize
    {
      Tag::SizeMap tag_sizes;
      CampaignSelector::WeightedCampaignKeywordList campaign_candidates;
      CampaignSelector::WeightedCampaignKeywordGroupList grouped_campaign_candidates;
    };

    typedef std::list<RandomTextSelectionBySize> RandomTextSelectionBySizeList;

    struct SizedCreativeHolder
    {
      SizedCreativeHolder(
        const Tag::Size* tag_size_val,
        const Creative* creative_val,
        const RevenueDecimal& conv_rate_val)
        : tag_size(tag_size_val),
          creative(creative_val),
          conv_rate(conv_rate_val)
      {}

      const Tag::Size* tag_size;
      const Creative* creative;
      RevenueDecimal conv_rate;
    };

    typedef std::list<SizedCreativeHolder> SizedCreativeHolderList;

    struct CTRWeightedCampaignHolder
    {
      CTRWeightedCampaignHolder(
        CampaignSelector::WeightedCampaignPtr&& weighted_campaign_val)
          : weighted_campaign(std::move(weighted_campaign_val))
      {}

      CTRWeightedCampaignHolder(CTRWeightedCampaignHolder&& init)
        : weighted_campaign(std::move(init.weighted_campaign))
      {
        cur_creatives.swap(init.cur_creatives);
      }

      CampaignSelector::WeightedCampaignPtr weighted_campaign;
      SizedCreativeHolderList cur_creatives;
    };

    typedef std::list<CTRWeightedCampaignHolder> CTRWeightedCampaignHolderList;

    /**
     * Weighted function that calculate weights in list for random_select method.
     */
    namespace
    {
      inline unsigned long
      balance_function(const Creative* creative) noexcept
      {
        return creative->weight;
      }

      inline unsigned long
      ctr_creative_holder_balance_function(
        const SizedCreativeHolder& creative_holder) noexcept
      {
        return creative_holder.creative->weight;
      }

      inline unsigned long
      equal_weight_fun(const CampaignSelector::WeightedCampaignPtr&) noexcept
      {
        return 1;
      }
    }

    /** CampaignSelector::ExpectedEcpm */
    CampaignSelector::ExpectedEcpm::ExpectedEcpm() noexcept
      : ex_ecpm_quad_sum_(ExtRevenueDecimal::ZERO),
        ex_ecpm_sum_(ExtRevenueDecimal::ZERO)
    {}

    RevenueDecimal
    CampaignSelector::ExpectedEcpm::value() const noexcept
    {
      if (ex_ecpm_sum_ == ExtRevenueDecimal::ZERO)
      {
        return RevenueDecimal::ZERO;
      }

      const ExtRevenueDecimal ex_exp_ecpm =
        ExtRevenueDecimal::div(ex_ecpm_quad_sum_, ex_ecpm_sum_);

      RevenueDecimal exp_ecpm;
      narrow_decimal(exp_ecpm, ex_exp_ecpm);
      assert(exp_ecpm > RevenueDecimal::ZERO);

      return exp_ecpm;
    }

    CampaignSelector::ExpectedEcpm&
    CampaignSelector::ExpectedEcpm::operator+= (const RevenueDecimal& arg) noexcept
    {
      ExtRevenueDecimal ex_ecpm;
      narrow_decimal(ex_ecpm, arg);
      ex_ecpm_quad_sum_ += ExtRevenueDecimal::mul(
        ex_ecpm, ex_ecpm, Generics::DMR_FLOOR);
      ex_ecpm_sum_ += ex_ecpm;

      return *this;
    }

    // CampaignSelector
    CampaignSelector::CampaignSelector(
      const CampaignIndex* campaign_selection_index,
      const CTRProvider* ctr_provider,
      const CTRProvider* conv_rate_provider)
      : campaign_selection_index_(campaign_selection_index),
        campaign_config_(campaign_selection_index->configuration()),
        ctr_provider_(ReferenceCounting::add_ref(ctr_provider)),
        conv_rate_provider_(ReferenceCounting::add_ref(conv_rate_provider))
    {}

    RevenueDecimal
    CampaignSelector::campaign_ctr_(
      const Creative*& result_creative,
      const CTRProvider::CalculationContext* ctr_calculation_context,
      const CampaignIndex::ConstCreativePtrList& available_creatives)
      noexcept
    {
      RevenueDecimal max_ctr = RevenueDecimal::ZERO;
      const Creative* max_ctr_creative = 0;
      CampaignIndex::ConstCreativePtrList equal_creatives;

      for(CampaignIndex::ConstCreativePtrList::const_iterator creative_it =
            available_creatives.begin();
          creative_it != available_creatives.end(); ++creative_it)
      {
        RevenueDecimal cur_ctr = ctr_calculation_context->get_ctr(*creative_it);

        if(cur_ctr > max_ctr)
        {
          max_ctr = cur_ctr;
          max_ctr_creative = *creative_it;
          equal_creatives.clear();
        }
        else if(cur_ctr == max_ctr)
        {
          if(max_ctr_creative)
          {
            equal_creatives.push_back(max_ctr_creative);
            equal_creatives.push_back(*creative_it);
            max_ctr_creative = 0;
          }
          else
          {
            max_ctr_creative = *creative_it;
          }
        }
      }

      if(!equal_creatives.empty())
      {
        // select creative randomly
        CampaignIndex::ConstCreativePtrList::iterator wc_it =
          Generics::random_select<unsigned long>(
            equal_creatives.begin(),
            equal_creatives.end(),
            balance_function);

        assert(wc_it != equal_creatives.end());

        result_creative = *wc_it;
      }
      else if(max_ctr_creative == 0 && !available_creatives.empty())
      {
        // all ctr's == 0
        result_creative = *available_creatives.begin();
      }
      else
      {
        result_creative = max_ctr_creative;
      }

      return max_ctr;
    }

    RevenueDecimal
    CampaignSelector::default_campaign_ecpm_(
      const Tag* tag,
      const Campaign* campaign)
      noexcept
    {
      if(campaign->use_ctr())
      {
        return RevenueDecimal::mul(
          std::min(
            RevenueDecimal::mul(
              RevenueDecimal::mul(campaign->ctr, ECPM_FACTOR,
                Generics::DMR_FLOOR),
              tag->adjustment,
              Generics::DMR_FLOOR),
            ECPM_FACTOR),
          campaign->click_sys_revenue,
          Generics::DMR_FLOOR);
      }

      return campaign->ecpm_;
    }

    RevenueDecimal
    CampaignSelector::campaign_ecpm_(
      RevenueDecimal& ctr,
      const Creative*& result_creative,
      const CTRProvider::CalculationContext* ctr_calculation_context,
      const Campaign* campaign,
      const CampaignIndex::ConstCreativePtrList& available_creatives)
      noexcept
    {
      if(campaign->use_ctr())
      {
        ctr = campaign_ctr_(
          result_creative,
          ctr_calculation_context,
          available_creatives);

        return RevenueDecimal::mul(
          campaign->click_sys_revenue,
          RevenueDecimal::mul(ctr, ECPM_FACTOR, Generics::DMR_FLOOR),
          Generics::DMR_FLOOR);
      }
      else
      {
        ctr = RevenueDecimal::ZERO;
      }

      result_creative = 0;
      return campaign->ecpm_;
    }

    RevenueDecimal
    CampaignSelector::default_campaign_keyword_ecpm_(
      const Tag* tag,
      const Campaign* campaign,
      const CampaignKeyword* campaign_keyword)
      noexcept
    {
      if(campaign_keyword)
      {
        return campaign_keyword->campaign->
          account->currency->to_system_currency(
            RevenueDecimal::mul(
              RevenueDecimal::mul(
                ECPM_FACTOR, campaign_keyword->ctr, Generics::DMR_FLOOR),
              campaign_keyword->max_cpc,
              Generics::DMR_FLOOR));
      }

      return default_campaign_ecpm_(
        tag,
        campaign);
    }

    RevenueDecimal
    CampaignSelector::campaign_keyword_ecpm_(
      RevenueDecimal& ctr,
      const Creative*& result_creative,
      const CTRProvider::CalculationContext* ctr_calculation_context,
      const Campaign* campaign,
      const CampaignKeyword* campaign_keyword,
      const CampaignIndex::ConstCreativePtrList& available_creatives)
      noexcept
    {
      if(campaign_keyword)
      {
        ctr = campaign_ctr_(
          result_creative,
          ctr_calculation_context,
          available_creatives);

        return campaign_keyword->campaign->
          account->currency->to_system_currency(
            RevenueDecimal::mul(
              RevenueDecimal::mul(ECPM_FACTOR, ctr, Generics::DMR_FLOOR),
              campaign_keyword->max_cpc,
              Generics::DMR_FLOOR));
      }

      return campaign_ecpm_(
        ctr,
        result_creative,
        ctr_calculation_context,
        campaign,
        available_creatives);
    }

    bool
    CampaignSelector::check_min_ecpm_(
      const Tag::TagPricing* tag_pricing,
      const RevenueDecimal& min_ecpm,
      const RevenueDecimal& campaign_ecpm)
    {
      return campaign_ecpm >= min_ecpm &&
        campaign_ecpm >= tag_pricing->cpm;
    }

    void
    CampaignSelector::filter_creatives_(
      CampaignIndex::ConstCreativePtrList& available_creatives,
      const CampaignIndex::Key& key,
      const CampaignSelectParams& request_params,
      const Campaign* campaign,
      const Tag* tag)
      const
      /*throw(eh::Exception)*/
    {
      // search available creatives
      campaign_selection_index_->filter_creatives(
        key,
        tag,
        &request_params.tag_sizes,
        campaign,
        request_params.profiling_available,
        request_params.full_freq_caps,
        request_params.seq_orders,
        available_creatives,
        true, // check click url categories
        request_params.up_expand_space,
        request_params.right_expand_space,
        request_params.down_expand_space,
        request_params.left_expand_space,
        request_params.video_min_duration,
        request_params.video_max_duration,
        request_params.video_skippable_max_duration,
        request_params.video_allow_skippable,
        request_params.video_allow_unskippable,
        request_params.allowed_durations,
        request_params.exclude_categories,
        request_params.required_categories,
        request_params.secure,
        request_params.filter_empty_destination,
        0);
    }

    /**
     * CampaignSelector::select_display_creative_
     * method must repeat all filtering steps marked as 'tag filtering'
     * in Campaign::weight
     */
    const Creative*
    CampaignSelector::select_display_creative_(
      CampaignIndex::ConstCreativePtrList& available_creatives) const
      /*throw(eh::Exception)*/
    {
      if(available_creatives.empty())
      {
        return 0;
      }

      CampaignIndex::ConstCreativePtrList::iterator wc_it =
        Generics::random_select<unsigned long>(
          available_creatives.begin(),
          available_creatives.end(),
          balance_function);

      assert(wc_it != available_creatives.end());

      return *wc_it;
    }

    const Tag::Size*
    CampaignSelector::select_tag_size_(
      const Tag* tag, const Creative* creative)
      noexcept
    {
      typedef std::list<const Tag::Size*> TagSizePtrList;

      TagSizePtrList available_sizes;

      for(Tag::SizeMap::const_iterator ts_it = tag->sizes.begin();
          ts_it != tag->sizes.end(); ++ts_it)
      {
        Creative::SizeMap::const_iterator cs_it =
          creative->sizes.find(ts_it->first);
        if(cs_it != creative->sizes.end())
        {
          available_sizes.push_back(ts_it->second);
        }
      }

      assert(!available_sizes.empty());

      TagSizePtrList::const_iterator as_it = available_sizes.begin();
      std::advance(as_it, Generics::safe_rand(available_sizes.size()));
      return *as_it;
    }

    void
    CampaignSelector::check_text_campaign_channels_(
      const CampaignIndex::Key& key,
      const CampaignSelectParams& request_params,
      const ChannelIdHashSet& matched_channels,
      const CampaignIndex::CampaignCellPtrList& campaign_list,
      ConstCampaignPtrList& filtered_campaigns)
      const
      noexcept
    {
      for(CampaignIndex::CampaignCellPtrList::const_iterator
            cit = campaign_list.begin();
          cit != campaign_list.end(); ++cit)
      {
        // don't check ecpm for text campaigns, will be checked sum
        if(campaign_selection_index_->check_campaign(
             key,
             (*cit)->campaign,
             request_params.time,
             request_params.profiling_available,
             request_params.full_freq_caps,
             request_params.colocation->colo_id,
             request_params.user_create_time,
             request_params.user_id,
             0) &&
           campaign_selection_index_->check_campaign_channel(
             (*cit)->campaign,
             matched_channels))
        {
          filtered_campaigns.push_back((*cit)->campaign);
        }
      }
    }

    template<typename CampaignHolderIteratorType>
    void
    CampaignSelector::collect_lost_(
      LostAuction& lost_auction,
      const CampaignIndex::Key& key,
      const CampaignSelectParams& request_params,
      const Tag* tag,
      const ChannelIdHashSet& matched_channels,
      CampaignHolderIteratorType it,
      CampaignHolderIteratorType end)
      const
      noexcept
    {
      for(; it != end; ++it)
      {
        const Campaign* campaign = (*it)->campaign;

        if(campaign_selection_index_->check_campaign(
             key,
             campaign,
             request_params.time,
             request_params.profiling_available,
             request_params.full_freq_caps,
             request_params.colocation->colo_id,
             request_params.user_create_time,
             request_params.user_id,
             0) &&
           campaign_selection_index_->check_campaign_channel(
             campaign,
             matched_channels))
        {
          // collect ads lost auction obviously (ecpm < result ecpm)
          CampaignIndex::ConstCreativePtrList available_creatives;

          campaign_selection_index_->filter_creatives(
            key,
            tag,
            &request_params.tag_sizes,
            campaign,
            request_params.profiling_available,
            request_params.full_freq_caps,
            SeqOrderMap(),
            available_creatives,
            true, // check click categories
            request_params.up_expand_space,
            request_params.right_expand_space,
            request_params.down_expand_space,
            request_params.left_expand_space,
            request_params.video_min_duration,
            request_params.video_max_duration,
            request_params.video_skippable_max_duration,
            request_params.video_allow_skippable,
            request_params.video_allow_unskippable,
            request_params.allowed_durations,
            request_params.exclude_categories,
            request_params.required_categories,
            request_params.secure,
            request_params.filter_empty_destination,
            0);

          std::copy(
            available_creatives.begin(),
            available_creatives.end(),
            std::inserter(lost_auction.creatives,
              lost_auction.creatives.begin()));

          lost_auction.ccgs.insert(campaign);
        }
      }
    }

    // get_all_display_campaign_candidates_ with ctr calculation
    void
    CampaignSelector::get_all_display_campaign_candidates_(
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
      noexcept
    {
      if(ctr_calculation == 0)
      {
        get_all_display_campaign_candidates_(
          result_campaign_candidates,
          lost_campaigns,
          key,
          request_params,
          tag,
          matched_channels,
          campaign_list,
          check_min_ecpm);

        return;
      }

      assert(ctr_calculation);

      CTRWeightedCampaignHolderList unknown_ctr_campaign_candidates;
      CTRWeightedCampaignHolderList known_ctr_campaign_candidates;

      // step 1: filter all campaigns without ecpm checking
      for(CampaignIndex::CampaignSelectionCellPtrList::const_iterator cmp_it =
            campaign_list.begin();
          cmp_it != campaign_list.end(); ++cmp_it)
      {
        if(campaign_selection_index_->check_campaign(
             key,
             (*cmp_it)->campaign,
             request_params.time,
             request_params.profiling_available,
             request_params.full_freq_caps,
             request_params.colocation->colo_id,
             request_params.user_create_time,
             request_params.user_id,
             0) &&
           campaign_selection_index_->check_campaign_channel(
             (*cmp_it)->campaign,
             matched_channels))
        {
          // get available creatives for all request_params.tag_sizes (filtered tag sizes)
          CampaignIndex::ConstCreativePtrList available_creatives;

          filter_creatives_(
            available_creatives,
            key,
            request_params,
            (*cmp_it)->campaign,
            tag);

          if(!available_creatives.empty()) // any creative can be selected
          {
            if((*cmp_it)->campaign->use_ctr() ||
              (*cmp_it)->campaign->bid_strategy == BS_MIN_CTR_GOAL)
            {
              unknown_ctr_campaign_candidates.push_back(
                std::move(CTRWeightedCampaignHolder(
                  std::move(WeightedCampaignPtr(new WeightedCampaign(
                    tag,
                    (*cmp_it)->tag_pricing,
                    0, // tag_size
                    (*cmp_it)->campaign,
                    nullptr, // result creative
                    available_creatives, // clear available_creatives
                    RevenueDecimal::ZERO,
                    RevenueDecimal::ZERO, // ctr will be inited after
                    RevenueDecimal::ZERO // conv rate will be inited after
                    ))))));
            }
            else
            {
              RevenueDecimal current_ecpm =
                default_campaign_ecpm_(tag, (*cmp_it)->campaign);

              if(check_min_ecpm && !check_min_ecpm_(
                   (*cmp_it)->tag_pricing,
                   request_params.min_ecpm,
                   current_ecpm))
              {
                if (lost_campaigns)
                {
                  lost_campaigns->push_back(*cmp_it);
                }
                continue;
              }

              try
              {
                known_ctr_campaign_candidates.push_back(
                  WeightedCampaignPtr(new WeightedCampaign(
                    tag,
                    (*cmp_it)->tag_pricing,
                    nullptr, // tag size
                    (*cmp_it)->campaign,
                    nullptr, // result creative
                    available_creatives, // clear available_creatives
                    current_ecpm,
                    (*cmp_it)->campaign->ctr,
                    RevenueDecimal::ZERO // conv rate will be inited after
                    )));
              }
              catch(const RevenueDecimal::Overflow&)
              {}
            }
          }
        }
      }

      // fetch sizes
      for(Tag::SizeMap::const_iterator tag_size_it = request_params.tag_sizes.begin();
        tag_size_it != request_params.tag_sizes.end();
        ++tag_size_it)
      {
        CTRProvider::CalculationContext_var ctr_calculation_context =
          ctr_calculation->create_context(
            tag_size_it->second);

        CTRProvider::CalculationContext_var conv_rate_calculation_context;
        if(conv_rate_calculation)
        {
          conv_rate_calculation_context = conv_rate_calculation->create_context(
            tag_size_it->second);
        }

        // fetch unknown_ctr_campaign_candidates
        // filter creatives by ctr with known size 
        for(CTRWeightedCampaignHolderList::iterator wit =
              unknown_ctr_campaign_candidates.begin();
            wit != unknown_ctr_campaign_candidates.end();
            ++wit)
        {
          bool rate_checked = false;

          for(CampaignIndex::ConstCreativePtrList::const_iterator creative_it =
                wit->weighted_campaign->available_creatives.begin();
              creative_it != wit->weighted_campaign->available_creatives.end();
              ++creative_it)
          {
            RevenueDecimal conv_rate = RevenueDecimal::ZERO;

            if(CampaignIndex::creative_available_by_size(
                tag,
                tag_size_it->second,
                *creative_it,
                request_params.up_expand_space,
                request_params.right_expand_space,
                request_params.down_expand_space,
                request_params.left_expand_space))
            {
              bool rate_creative_dependent = false;

              // check_rate is hard operation - make it last
              if(rate_checked ||
                 !conv_rate_calculation_context ||
                 conv_rate_calculation_context->check_rate(
                   *creative_it, &conv_rate, &rate_creative_dependent))
              {
                RevenueDecimal ctr = ctr_calculation_context->get_ctr(*creative_it);

                RevenueDecimal ecpm = wit->weighted_campaign->campaign->use_ctr() ?
                  RevenueDecimal::mul(
                    wit->weighted_campaign->campaign->click_sys_revenue,
                    RevenueDecimal::mul(ctr, ECPM_FACTOR, Generics::DMR_FLOOR),
                    Generics::DMR_FLOOR) :
                  // CPM campaign with BS_MIN_CTR_GOAL
                  default_campaign_ecpm_(tag, wit->weighted_campaign->campaign);

                // select creative with max ecpm for all auction types
                // fill ecpm and (tag_size, creative) candidates
                if((!check_min_ecpm || check_min_ecpm_(
                     wit->weighted_campaign->tag_pricing,
                     request_params.min_ecpm,
                     ecpm)) &&
                   (wit->weighted_campaign->campaign->bid_strategy != BS_MIN_CTR_GOAL ||
                    ctr >= wit->weighted_campaign->campaign->min_ctr_goal()))
                {
                  if(ecpm > wit->weighted_campaign->ecpm)
                  {
                    wit->weighted_campaign->ecpm = ecpm;
                    wit->weighted_campaign->ctr = ctr;
                    wit->weighted_campaign->conv_rate = conv_rate;
                    wit->cur_creatives.clear();
                    wit->cur_creatives.push_back(
                      SizedCreativeHolder(tag_size_it->second, *creative_it, conv_rate));
                  }
                  else if(ecpm == wit->weighted_campaign->ecpm)
                  {
                    wit->cur_creatives.push_back(
                      SizedCreativeHolder(tag_size_it->second, *creative_it, conv_rate));
                  }
                }
              }

              rate_checked = !rate_creative_dependent;
            } // check size
          }
        }

        // fetch known_ctr_campaign_candidates
        // not eval
        for(CTRWeightedCampaignHolderList::iterator wit =
              known_ctr_campaign_candidates.begin();
            wit != known_ctr_campaign_candidates.end();
            ++wit)
        {
          bool rate_checked = false;

          for(CampaignIndex::ConstCreativePtrList::const_iterator creative_it =
                wit->weighted_campaign->available_creatives.begin();
              creative_it != wit->weighted_campaign->available_creatives.end();
              ++creative_it)
          {
            RevenueDecimal conv_rate = RevenueDecimal::ZERO;

            if(CampaignIndex::creative_available_by_size(
                tag,
                tag_size_it->second,
                *creative_it,
                request_params.up_expand_space,
                request_params.right_expand_space,
                request_params.down_expand_space,
                request_params.left_expand_space))
            {
              bool rate_creative_dependent = false;

              // check_rate is hard operation - make it last
              if(rate_checked ||
                 !conv_rate_calculation_context ||
                  conv_rate_calculation_context->check_rate(
                    *creative_it, &conv_rate, &rate_creative_dependent))
              {
                wit->cur_creatives.push_back(
                  SizedCreativeHolder(tag_size_it->second, *creative_it, conv_rate));
              }

              rate_checked = !rate_creative_dependent;
            }
          }
        }
      }

      // select tag_size, creative
      for(CTRWeightedCampaignHolderList::iterator wit =
            unknown_ctr_campaign_candidates.begin();
          wit != unknown_ctr_campaign_candidates.end();
          ++wit)
      {
        if(!wit->cur_creatives.empty()) // all creatives filtered by check_min_ecpm
        {
          SizedCreativeHolderList::iterator cr_it =
            Generics::random_select<unsigned long>(
              wit->cur_creatives.begin(),
              wit->cur_creatives.end(),
              ctr_creative_holder_balance_function);

          wit->weighted_campaign->tag_size = cr_it->tag_size;
          wit->weighted_campaign->creative = cr_it->creative;
          wit->weighted_campaign->conv_rate = cr_it->conv_rate;

          result_campaign_candidates.push_back(std::move(wit->weighted_campaign));
        }
      }

      for(CTRWeightedCampaignHolderList::iterator wit =
            known_ctr_campaign_candidates.begin();
          wit != known_ctr_campaign_candidates.end();
          ++wit)
      {
        if(!wit->cur_creatives.empty()) // all creatives filtered by conv rate check
        {
          SizedCreativeHolderList::iterator cr_it =
            Generics::random_select<unsigned long>(
              wit->cur_creatives.begin(),
              wit->cur_creatives.end(),
              ctr_creative_holder_balance_function);

          wit->weighted_campaign->tag_size = cr_it->tag_size;
          wit->weighted_campaign->creative = cr_it->creative;
          wit->weighted_campaign->conv_rate = cr_it->conv_rate;

          result_campaign_candidates.push_back(std::move(wit->weighted_campaign));
        }
      }
    }

    // get_all_display_campaign_candidates_ with default ctr algorithm
    void
    CampaignSelector::get_all_display_campaign_candidates_(
      WeightedCampaignList& result_campaign_candidates,
      CampaignIndex::CampaignSelectionCellPtrList* lost_campaigns,
      const CampaignIndex::Key& key,
      const CampaignSelectParams& request_params,
      const Tag* tag,
      const ChannelIdHashSet& matched_channels,
      const CampaignIndex::CampaignSelectionCellPtrList& campaign_list,
      bool check_min_ecpm)
      const
      noexcept
    {
      for(CampaignIndex::CampaignSelectionCellPtrList::const_iterator cmp_it =
            campaign_list.begin();
          cmp_it != campaign_list.end(); ++cmp_it)
      {
        if(campaign_selection_index_->check_campaign(
             key,
             (*cmp_it)->campaign,
             request_params.time,
             request_params.profiling_available,
             request_params.full_freq_caps,
             request_params.colocation->colo_id,
             request_params.user_create_time,
             request_params.user_id,
             0) &&
           campaign_selection_index_->check_campaign_channel(
             (*cmp_it)->campaign,
             matched_channels))
        {
          RevenueDecimal current_ecpm = default_campaign_ecpm_(tag, (*cmp_it)->campaign);

          if(check_min_ecpm && !check_min_ecpm_(
               (*cmp_it)->tag_pricing,
               request_params.min_ecpm,
               current_ecpm))
          {
            if (lost_campaigns)
            {
              lost_campaigns->push_back(*cmp_it);
            }
            continue;
          }

          if((*cmp_it)->campaign->bid_strategy == BS_MIN_CTR_GOAL &&
             (*cmp_it)->campaign->ctr < (*cmp_it)->campaign->min_ctr_goal())
          {
            continue;
          }

          // get available creatives for eval ecpm
          CampaignIndex::ConstCreativePtrList available_creatives;

          filter_creatives_(
            available_creatives,
            key,
            request_params,
            (*cmp_it)->campaign,
            tag);

          if(!available_creatives.empty()) // any creative can be selected
          {
            const Creative* result_creative = select_display_creative_(
              available_creatives);

            try
            {
              result_campaign_candidates.push_back(
                WeightedCampaignPtr(new WeightedCampaign(
                  tag,
                  (*cmp_it)->tag_pricing,
                  select_tag_size_(tag, result_creative),
                  (*cmp_it)->campaign,
                  result_creative,
                  available_creatives, // clear available_creatives
                  current_ecpm,
                  (*cmp_it)->campaign->ctr,
                  RevenueDecimal::ZERO // conv rate
                  )));
            }
            catch(const RevenueDecimal::Overflow&)
            {}
          }
        }
      }
    }

    // get_max_display_campaign_candidates_
    // can be used only for default CTR algorithm
    void
    CampaignSelector::get_max_display_campaign_candidates_(
      WeightedCampaignList& result_campaign_candidates,
      LostAuction* lost_auction,
      const CampaignIndex::Key& key,
      const CampaignSelectParams& request_params,
      const Tag* tag,
      const ChannelIdHashSet& matched_channels,
      const CampaignIndex::CampaignSelectionCellPtrList& campaign_list,
      const CampaignIndex::CampaignCellPtrList& lost_campaigns)
      const
      noexcept
    {
      // method use static ecpm - can't be used if ctr algorithm isn't default
      RevenueDecimal selected_non_adjusted_ecpm = RevenueDecimal::ZERO;

      CampaignIndex::CampaignSelectionCellPtrList::const_iterator
        cmp_it = campaign_list.begin();

      for(; cmp_it != campaign_list.end(); ++cmp_it)
      {
        if(selected_non_adjusted_ecpm != RevenueDecimal::ZERO &&
           (*cmp_it)->campaign->ecpm_ != selected_non_adjusted_ecpm)
        {
          break;
        }

        // min_ecpm (check separate way below)
        if(campaign_selection_index_->check_campaign(
             key,
             (*cmp_it)->campaign,
             request_params.time,
             request_params.profiling_available,
             request_params.full_freq_caps,
             request_params.colocation->colo_id,
             request_params.user_create_time,
             request_params.user_id,
             0) &&
           campaign_selection_index_->check_campaign_channel(
             (*cmp_it)->campaign,
             matched_channels))
        {
          // default CTR algorithm
          RevenueDecimal current_ecpm = default_campaign_ecpm_(
            tag,
            (*cmp_it)->campaign);

          // method use static ecpm's and
          // campaigns are ordered by ecpm descending,
          // the next will not satisfy the min_ecpm filter
          // and will be checked below only for lost auction
          // break if check failed
          if(!check_min_ecpm_(
              (*cmp_it)->tag_pricing,
              request_params.min_ecpm,
              current_ecpm))
          {
            break;
          }

          if((*cmp_it)->campaign->bid_strategy == BS_MIN_CTR_GOAL &&
             (*cmp_it)->campaign->ctr < (*cmp_it)->campaign->min_ctr_goal())
          {
            continue;
          }

          CampaignIndex::ConstCreativePtrList available_creatives;

          filter_creatives_(
            available_creatives,
            key,
            request_params,
            (*cmp_it)->campaign,
            tag);

          const Creative* result_creative = select_display_creative_(
            available_creatives);

          if(result_creative)
          {
            selected_non_adjusted_ecpm = (*cmp_it)->campaign->ecpm_;

            try
            {
              result_campaign_candidates.push_back(
                WeightedCampaignPtr(new WeightedCampaign(
                  tag,
                  (*cmp_it)->tag_pricing,
                  select_tag_size_(tag, result_creative),
                  (*cmp_it)->campaign,
                  result_creative,
                  available_creatives, // clear available_creatives
                  current_ecpm,
                  (*cmp_it)->campaign->ctr,
                  RevenueDecimal::ZERO // conv rate
                  )));
            }
            catch(const RevenueDecimal::Overflow&)
            {}
          }
        }
      }

      if(lost_auction)
      {
        // collect lost for margin filtered campaigns
        collect_lost_(
          *lost_auction,
          key,
          request_params,
          tag,
          matched_channels,
          lost_campaigns.begin(),
          lost_campaigns.end());

        // collect lost for non selected campaigns with ecpm < selected ecpm
        collect_lost_(
          *lost_auction,
          key,
          request_params,
          tag,
          matched_channels,
          cmp_it,
          campaign_list.end());
      }
    }

    void
    CampaignSelector::get_text_campaign_candidates_(
      WeightedCampaignKeywordList& result_campaign_candidates,
      const CampaignIndex::Key& key,
      const CampaignSelectParams& request_params,
      const Tag* tag,
      const CTRProvider::CalculationContext* ctr_calculation_context,
      const CampaignKeywordMap& matched_keywords,
      const ChannelIdHashSet& matched_channels,
      const CampaignIndex::CampaignCellPtrList& keyword_campaigns,
      const CampaignIndex::CampaignCellPtrList& nonkeyword_campaigns)
      const
      noexcept
    {
      CampaignKeywordMap filtered_campaign_keywords;
      ConstCampaignPtrList filtered_text_campaigns;

      cross_campaigns_with_keywords_(
        key,
        request_params,
        matched_channels,
        keyword_campaigns,
        matched_keywords,
        filtered_campaign_keywords);

      check_text_campaign_channels_(
        key,
        request_params,
        matched_channels,
        nonkeyword_campaigns,
        filtered_text_campaigns);

      // filtered_campaign_keywords and filtered_text_campaigns contains
      // only campaigns, that can be shown
      // combine filtered_campaign_keywords, filtered_text_campaigns
      for(CampaignKeywordMap::const_iterator kw_it =
            filtered_campaign_keywords.begin();
          kw_it != filtered_campaign_keywords.end(); ++kw_it)
      {
        if(!ctr_calculation_context)
        {
          // for default CTR algorithm we can skip available creatives
          // checking for all candidates and get these candidates only after
          // final selection
          RevenueDecimal current_ecpm = default_campaign_keyword_ecpm_(
            tag,
            kw_it->second->campaign,
            kw_it->second);

          result_campaign_candidates.push_back(
            WeightedCampaignKeyword(
              tag,
              kw_it->second,
              kw_it->second->campaign,
              0, // creative will be selected after
              current_ecpm, // ecpm_bid (can be changed at selection)
              kw_it->second->max_cpc, // actual_cpc
              current_ecpm,
              kw_it->second->ctr,
              RevenueDecimal::ZERO // conv rate (don't calc in default ctr mode)
              ));
        }
        else
        {
          // for non default CTR algorithm we need to check available creatives
          CampaignIndex::ConstCreativePtrList available_creatives;

          filter_creatives_(
            available_creatives,
            key,
            request_params,
            kw_it->second->campaign,
            tag);

          const Creative* creative;

          RevenueDecimal current_ctr;
          RevenueDecimal current_ecpm = campaign_keyword_ecpm_(
            current_ctr,
            creative,
            ctr_calculation_context,
            kw_it->second->campaign,
            kw_it->second,
            available_creatives);

          result_campaign_candidates.push_back(
            WeightedCampaignKeyword(
              tag,
              kw_it->second,
              kw_it->second->campaign,
              creative,
              current_ecpm, // ecpm_bid (can be changed at selection)
              kw_it->second->max_cpc, // actual_cpc
              current_ecpm,
              current_ctr,
              RevenueDecimal::ZERO // conv rate (
                // will be filled on text campaign candidates filtering)
              ));
        }
      }

      for(ConstCampaignPtrList::const_iterator ch_text_campaign_it =
            filtered_text_campaigns.begin();
          ch_text_campaign_it != filtered_text_campaigns.end();
          ++ch_text_campaign_it)
      {
        try
        {
          RevenueDecimal current_ecpm;
          RevenueDecimal current_ctr;
          const Creative* creative = 0;

          if(ctr_calculation_context)
          {
            CampaignIndex::ConstCreativePtrList available_creatives;

            filter_creatives_(
              available_creatives,
              key,
              request_params,
              *ch_text_campaign_it,
              tag);

            if(available_creatives.empty())
            {
              continue;
            }

            current_ecpm = campaign_ecpm_(
              current_ctr,
              creative,
              ctr_calculation_context,
              *ch_text_campaign_it,
              available_creatives);
          }
          else
          {
            current_ecpm = default_campaign_ecpm_(tag, *ch_text_campaign_it);
            current_ctr = (*ch_text_campaign_it)->ctr;
          }
          
          result_campaign_candidates.push_back(
            WeightedCampaignKeyword(
              tag,
              0,
              *ch_text_campaign_it,
              creative,
              current_ecpm, // ecpm_bid (can be changed at selection)
              RevenueDecimal::ZERO, // actual_cpc
              current_ecpm,
              current_ctr,
              RevenueDecimal::ZERO // conv rate (
                // will be filled on text campaign candidates filtering)
              ));
        }
        catch(const RevenueDecimal::Overflow&)
        {}
      }
    }

    CampaignSelector::WeightedCampaignPtr
    CampaignSelector::select_display_campaign_(
      LostAuction* lost_auction,
      AuctionType auction_type,
      const CampaignIndex::Key& key,
      const CampaignSelectParams& request_params,
      const Tag* tag,
      const CTRProvider::Calculation* ctr_calculation,
      const CTRProvider::Calculation* conv_rate_calculation,
      const ChannelIdHashSet& matched_channels,
      const CampaignIndex::CampaignSelectionCellPtrList& campaign_list,
      const CampaignIndex::CampaignCellPtrList& lost_campaigns)
      const
      noexcept
    {
      if(ctr_calculation == 0 && auction_type == AT_MAX_ECPM)
      {
        WeightedCampaignList random_select_campaigns;

        get_max_display_campaign_candidates_(
          random_select_campaigns,
          lost_auction,
          key,
          request_params,
          tag,
          matched_channels,
          campaign_list,
          lost_campaigns);

        /*
        std::cerr << "random_select_campaigns :" << std::endl;
        for(WeightedCampaignList::const_iterator rit = random_select_campaigns.begin();
            rit != random_select_campaigns.end(); ++rit)
        {
          std::cerr << "[ campaign_id = " << (*rit)->campaign->campaign_id <<
            ", ecpm = " << (*rit)->ecpm <<
            ", ccid = " << (*rit)->creative->ccid << "]" << std::endl;
        }
        */

        if(!random_select_campaigns.empty())
        {
          WeightedCampaignList::iterator res_it =
            Generics::random_select<unsigned int>(
              random_select_campaigns.begin(),
              random_select_campaigns.end(),
              equal_weight_fun);

          if(lost_auction)
          {
            // collect lost for campaigns with equal ecpm (all filters already passed)
            for(WeightedCampaignList::iterator eq_it =
                  random_select_campaigns.begin();
                eq_it != random_select_campaigns.end(); ++eq_it)
            {
              if(eq_it != res_it)
              {
                lost_auction->ccgs.insert((*eq_it)->campaign);

                std::copy(
                  (*eq_it)->available_creatives.begin(),
                  (*eq_it)->available_creatives.end(),
                  std::inserter(lost_auction->creatives,
                    lost_auction->creatives.begin()));
              }
            }
          }

          return std::move(*res_it);
        }
      }
      else
      {
        WeightedCampaignList random_select_campaigns;
        CampaignIndex::CampaignSelectionCellPtrList get_all_display_lost_campaigns;

        get_all_display_campaign_candidates_(
          random_select_campaigns,
          &get_all_display_lost_campaigns,
          key,
          request_params,
          tag,
          ctr_calculation,
          conv_rate_calculation,
          matched_channels,
          campaign_list,
          ctr_calculation != 0 || auction_type != AT_RANDOM // check min ecpm
          );

        /*
        std::cerr << "random_select_campaigns :" << std::endl;
        for(WeightedCampaignList::const_iterator rit = random_select_campaigns.begin();
            rit != random_select_campaigns.end(); ++rit)
        {
          std::cerr << "[ campaign_id = " << (*rit)->campaign->campaign_id <<
            ", ecpm = " << (*rit)->ecpm <<
            ", ccid = " << (*rit)->creative->ccid << "]" << std::endl;
        }
        */

        if(!random_select_campaigns.empty())
        {
          if(auction_type == AT_RANDOM)
          {
            // select campaign randomly (with equal weight)
            unsigned long pos =
              request_params.random2 % random_select_campaigns.size();

            WeightedCampaignList::iterator cmp_it =
              random_select_campaigns.begin();

            std::advance(cmp_it, pos);

            return std::move(*cmp_it);
          }
          else // auction_type == AT_PROPORTIONAL_PROBABILITY or
            // defined ctr_calculation_context
          {
            WeightedCampaignList::iterator cmp_it = random_select_campaigns.end();

            if(auction_type == AT_PROPORTIONAL_PROBABILITY)
            {
              // select campaign randomly with weight = ecpm
              RevenueDecimal sum_ecpm = RevenueDecimal::ZERO;

              for(WeightedCampaignList::const_iterator it =
                    random_select_campaigns.begin();
                  it != random_select_campaigns.end(); ++it)
              {
                sum_ecpm += (*it)->ecpm;
              }

              RevenueDecimal ecpm_offset = RevenueDecimal::mul(
                sum_ecpm,
                RevenueDecimal::div(
                  RevenueDecimal(request_params.random2, 0),
                  RevenueDecimal(RANDOM_PARAM_MAX, 0)),
                Generics::DMR_FLOOR);

              sum_ecpm = RevenueDecimal::ZERO;

              cmp_it = random_select_campaigns.begin();

              for(; cmp_it != random_select_campaigns.end(); ++cmp_it)
              {
                if(ecpm_offset < sum_ecpm)
                {
                  break;
                }
                sum_ecpm += (*cmp_it)->ecpm;
              }

              assert(cmp_it != random_select_campaigns.begin());

              --cmp_it;
            }
            else // ctr_calculation_context defined
            {
              RevenueDecimal max_ecpm_margin = RevenueDecimal::ZERO;
              std::list<WeightedCampaignList::iterator> max_ecpm_campaigns;

              for(WeightedCampaignList::iterator it =
                    random_select_campaigns.begin();
                  it != random_select_campaigns.end(); ++it)
              {
                const RevenueDecimal cur_ecpm_margin = (*it)->ecpm -
                  (*it)->tag_pricing->cpm -
                  RevenueDecimal::mul(
                    (*it)->ecpm,
                    (*it)->tag_pricing->revenue_share,
                    Generics::DMR_FLOOR);

                if(cur_ecpm_margin > max_ecpm_margin)
                {
                  max_ecpm_campaigns.clear();
                  max_ecpm_campaigns.push_back(it);
                  max_ecpm_margin = cur_ecpm_margin;
                }
                else if(cur_ecpm_margin == max_ecpm_margin)
                {
                  max_ecpm_campaigns.push_back(it);
                }
              }

              assert(!max_ecpm_campaigns.empty());

              // select result campaign randomly (from set with equal ecpm = max ecpm)
              unsigned long pos =
                request_params.random2 % max_ecpm_campaigns.size();

              std::list<WeightedCampaignList::iterator>::iterator cmp_it_it =
                max_ecpm_campaigns.begin();

              std::advance(cmp_it_it, pos);

              cmp_it = *cmp_it_it;
            }

            if(lost_auction)
            {
              // fill lost
              for(WeightedCampaignList::iterator lost_cmp_it =
                    random_select_campaigns.begin();
                  lost_cmp_it != random_select_campaigns.end();
                  ++lost_cmp_it)
              {
                if(lost_cmp_it != cmp_it)
                {
                  lost_auction->ccgs.insert((*lost_cmp_it)->campaign);

                  std::copy(
                    (*lost_cmp_it)->available_creatives.begin(),
                    (*lost_cmp_it)->available_creatives.end(),
                    std::inserter(lost_auction->creatives,
                      lost_auction->creatives.begin()));
                }
              }
            }

            return std::move(*cmp_it);
          }
        } // !random_select_campaigns.empty()

        if(lost_auction)
        {
          // collect lost for margin filtered campaigns
          collect_lost_(
            *lost_auction,
            key,
            request_params,
            tag,
            matched_channels,
            lost_campaigns.begin(),
            lost_campaigns.end());

          // collect lost for non selected campaigns with ecpm < selected ecpm
          collect_lost_(
            *lost_auction,
            key,
            request_params,
            tag,
            matched_channels,
            get_all_display_lost_campaigns.begin(),
            get_all_display_lost_campaigns.end());
        }
      }

      return CampaignSelector::WeightedCampaignPtr();
    }

    void
    CampaignSelector::convert_text_candidates_to_cpc_map_(
      CPCKeywordMap& cpc_keyword_map,
      const WeightedCampaignKeywordList& text_campaign_candidates)
      noexcept
    {
      for(WeightedCampaignKeywordList::const_iterator kit =
            text_campaign_candidates.begin();
          kit != text_campaign_candidates.end();
          ++kit)
      {
        try
        {
          /* insert into random position for mixing
           * campaign positions with one cost */
          WeightedCampaignKeywordList& lst = cpc_keyword_map[kit->actual_ecpm];

          if(!lst.empty())
          {
            WeightedCampaignKeywordList::iterator it = lst.begin();
            std::advance(it, Generics::safe_rand(lst.size() + 1));
            lst.insert(it, *kit);
          }
          else
          {
            lst.push_back(*kit);
          }
        }
        catch(const RevenueDecimal::Overflow&)
        {}
      }
    }

    void
    CampaignSelector::revert_cost_(
      WeightedCampaignKeywordList& weighted_campaign_keywords)
      noexcept
    {
      for(WeightedCampaignKeywordList::iterator it =
            weighted_campaign_keywords.begin();
          it != weighted_campaign_keywords.end(); ++it)
      {
        if(it->campaign_keyword)
        {
          it->actual_cpc = it->campaign_keyword->campaign->advertiser->revert_cost(
            it->actual_cpc,
            it->campaign_keyword->campaign->commision);

          if(it->actual_cpc == RevenueDecimal::ZERO &&
             it->campaign_keyword->max_cpc != RevenueDecimal::ZERO)
          {
            // 1 / 10 ^ fraction ~ ceil of 0.00...1
            it->actual_cpc = RevenueDecimal(
              1, it->campaign_keyword->campaign->account->currency->fraction);
          }
          else
          {
            it->actual_cpc.ceil(
              it->campaign_keyword->campaign->account->currency->fraction);
          }
        }
      }
    }

    bool
    CampaignSelector::select_text_campaigns_randomly_(
      WeightedCampaignKeywordList& result_text_campaigns,
      WeightedCampaignKeywordGroupList& grouped_text_campaign_candidates,
      unsigned long max_text_creatives)
      const
      noexcept
    {
      unsigned long grouped_text_campaign_candidates_size =
        grouped_text_campaign_candidates.size();

      for(unsigned long select_i = 0;
          select_i < max_text_creatives &&
            !grouped_text_campaign_candidates.empty();
          ++select_i)
      {
        unsigned long up_pos = Generics::safe_rand(
          grouped_text_campaign_candidates_size);

        WeightedCampaignKeywordGroupList::iterator cmp_cell_it =
          grouped_text_campaign_candidates.begin();

        std::advance(cmp_cell_it, up_pos);

        unsigned long sub_pos = Generics::safe_rand(cmp_cell_it->size());

        WeightedCampaignKeywordPtrArray::iterator wcmp_it =
          cmp_cell_it->begin();

        std::advance(wcmp_it, sub_pos);

        RevenueDecimal actual_cpc(RevenueDecimal::ZERO); // account currency

        if((*wcmp_it)->campaign_keyword.in())
        {
          actual_cpc = (*wcmp_it)->campaign_keyword->max_cpc;
        }
        else
        {
          actual_cpc = (*wcmp_it)->campaign->click_revenue;
        }

        (*wcmp_it)->actual_cpc = actual_cpc;

        result_text_campaigns.push_back(**wcmp_it);

        grouped_text_campaign_candidates.erase(cmp_cell_it);

        --grouped_text_campaign_candidates_size;
      }

      revert_cost_(result_text_campaigns);

      return true;
    }

    bool
    CampaignSelector::select_text_campaigns_prop_probability_(
      WeightedCampaignKeywordList& result_text_campaigns,
      const WeightedCampaignKeywordList& text_campaign_candidates_val,
      const RevenueDecimal& tag_min_ecpm,
      unsigned long max_text_creatives)
      noexcept
    {
      RevenueDecimal selected_ecpm_sum = RevenueDecimal::ZERO;

      WeightedCampaignKeywordList text_campaign_candidates(
        text_campaign_candidates_val);

      IdWeightedCampaignKeywordMaps campaigns_maps;
      fill_id_weighted_campaigns_keyword_maps(campaigns_maps, text_campaign_candidates);

      for(unsigned long select_i = 0;
          select_i < max_text_creatives;
          ++select_i)
      {
        RevenueDecimal min_ecpm = RevenueDecimal::div(
          tag_min_ecpm - selected_ecpm_sum,
          RevenueDecimal(false, max_text_creatives - select_i, 0));

        WeightedCampaignKeywordPtrArray filtered_text_campaign_candidates;

        for(WeightedCampaignKeywordList::iterator it =
              text_campaign_candidates.begin();
            it != text_campaign_candidates.end(); ++it)
        {
          if(it->campaign &&
             it->actual_ecpm >= min_ecpm)
          {
            filtered_text_campaign_candidates.push_back(&*it);
          }
        }

        if(filtered_text_campaign_candidates.empty())
        {
          if(selected_ecpm_sum < tag_min_ecpm)
          {
            result_text_campaigns.clear();
            return false;
          }
          else
          {
            return true;
          }
        }

        RevenueDecimal cur_ecpm_sum;
        ExpRevWeightedCampaignKeywordMap grouped_text_campaign_candidates;

        group_text_campaigns_(
          grouped_text_campaign_candidates,
          cur_ecpm_sum,
          filtered_text_campaign_candidates);

        ExpRevWeightedCampaignKeywordMap::iterator group_it =
          grouped_text_campaign_candidates.begin();

        if (cur_ecpm_sum != RevenueDecimal::ZERO)
        {
          RevenueDecimal ecpm_offset = RevenueDecimal::mul(
            RevenueDecimal::div(
              RevenueDecimal(false, Generics::safe_rand(), 0),
              RevenueDecimal(false, RAND_MAX, 0)),
            cur_ecpm_sum,
            Generics::DMR_FLOOR);

          RevenueDecimal cur_offset = RevenueDecimal::ZERO;

          for(; group_it != grouped_text_campaign_candidates.end();
              ++group_it)
          {
            cur_offset += group_it->first;

            if(ecpm_offset < cur_offset)
            {
              break;
            }
          }

          if(group_it == grouped_text_campaign_candidates.end())
          {
            // arithmetical mistake on RevenueDecimal can give this.
            --group_it;
          }
        }
        else
        {
          // All groups with ecpm == 0. Select randomly.
          unsigned long offset = Generics::safe_rand(grouped_text_campaign_candidates.size());
          std::advance(group_it, offset);
        }

        RevenueDecimal group_ecpm_sum = RevenueDecimal::ZERO;
        WeightedCampaignKeywordPtrArray& cmp_list = group_it->second;

        for(WeightedCampaignKeywordPtrArray::iterator cmp_it =
              cmp_list.begin();
            cmp_it != cmp_list.end(); ++cmp_it)
        {
          assert((*cmp_it)->campaign);
          group_ecpm_sum += (*cmp_it)->actual_ecpm;
        }


        WeightedCampaignKeywordPtrArray::iterator cmp_it = cmp_list.begin();
        if (group_ecpm_sum != RevenueDecimal::ZERO)
        {
          RevenueDecimal campaign_ecpm_offset = RevenueDecimal::mul(
            RevenueDecimal::div(
              RevenueDecimal(false, Generics::safe_rand(), 0),
              RevenueDecimal(false, RAND_MAX, 0)),
            group_ecpm_sum,
            Generics::DMR_FLOOR);

          RevenueDecimal cur_campaign_ecpm_offset = RevenueDecimal::ZERO;
          WeightedCampaignKeywordPtrArray::iterator cmp_last_it = --cmp_list.end();

          for(; cmp_it != cmp_last_it; ++cmp_it)
          {
            assert((*cmp_it)->campaign);

            cur_campaign_ecpm_offset += (*cmp_it)->actual_ecpm;
            if(cur_campaign_ecpm_offset > campaign_ecpm_offset)
            {
              break;
            }
          }
          // Here cmp_it is valid (selected by campaign_ecpm_offset or last in group)
        }
        else
        {
          // Select randomly. All have ecpm == 0
          unsigned long offset = Generics::safe_rand(cmp_list.size());
          std::advance(cmp_it, offset);
        }

        RevenueDecimal actual_cpc(RevenueDecimal::ZERO); // account currency

        if((*cmp_it)->campaign_keyword.in())
        {
          actual_cpc = (*cmp_it)->campaign_keyword->max_cpc;
        }
        else
        {
          actual_cpc = (*cmp_it)->campaign->click_revenue;
        }

        (*cmp_it)->actual_cpc = actual_cpc;

        assert((*cmp_it)->campaign);

        result_text_campaigns.push_back(**cmp_it);

        selected_ecpm_sum += (*cmp_it)->actual_ecpm;

        // Null campaigns in group with selected cmp_it
        WeightedCampaignKeywordPtrArray* campaigns_group = 0;
        get_campaigns_group(campaigns_group, campaigns_maps, *cmp_it);
        assert(campaigns_group);

        for (WeightedCampaignKeywordPtrArray::iterator iter = campaigns_group->begin();
              iter != campaigns_group->end(); ++iter)
        {
          (*iter)->campaign = 0;
        }
      }

      revert_cost_(result_text_campaigns);

      return true;
    }

    bool
    CampaignSelector::select_campaign_keywords_n_(
      LostAuction* lost_auction,
      const CampaignIndex::Key& key,
      const CampaignSelectParams& request_params,
      const Tag* tag,
      const Tag::Size* tag_size,
      const CTRProvider::CalculationContext* ctr_calculation_context,
      const CPCKeywordMap& cpc_keyword_map,
      const RevenueDecimal& min_sum_ecpm,
      unsigned long max_keywords,
      WeightedCampaignKeywordList& weighted_campaign_keywords)
      const
      noexcept
    {
      Tag::SizeMap tag_sizes;
      tag_sizes.insert(std::make_pair(
        tag_size->size->size_id, ReferenceCounting::add_ref(tag_size)));

      CPCKeywordCreativeMap filtered_cpc_keyword_map;      
      unsigned long selected_keywords = 0;

      // removing multiple ads from same accounts filters
      AccountIdSet account_filter_ids;
      AccountIdSet advertiser_filter_ids;
      CCGIdSet ccg_filter_ids;

      /* filter keywords by
       *   creative availability
       *   text ads multi showing
       * fetch all if lost ccgs collect required
       */
      for(CPCKeywordMap::const_reverse_iterator cit = cpc_keyword_map.rbegin();
          cit != cpc_keyword_map.rend() && (
            selected_keywords < max_keywords || lost_auction);
          ++cit)
      {
        const WeightedCampaignKeywordList& kws = cit->second;

        /* check keywords with equal actual cpc */
        for(WeightedCampaignKeywordList::const_iterator sub_cit =
              kws.begin();
            sub_cit != kws.end() && (
              selected_keywords < max_keywords || lost_auction);
            ++sub_cit)
        {
          bool filtered = false;

          AccountDef::TextAdserving ta_type =
            sub_cit->campaign->account->get_text_adserving();

          // check multi showing filters
          if(ta_type == AccountDef::TA_ALL) // one per CCG
          {
            filtered = ccg_filter_ids.find(sub_cit->campaign->campaign_id) !=
              ccg_filter_ids.end();
          }
          else if(ta_type == AccountDef::TA_ADVERTISER_ONE) // one per advertiser
          {
            filtered = advertiser_filter_ids.find(
              sub_cit->campaign->advertiser->account_id) !=
              advertiser_filter_ids.end();
          }
          else if(ta_type == AccountDef::TA_ONE) // one per account
          {
            filtered = account_filter_ids.find(
              sub_cit->campaign->account->account_id) !=
              account_filter_ids.end();
          }

          if(!filtered)
          {
            CampaignIndex::ConstCreativePtrList available_creatives;
            // creative can be already selected by max ecpm - use it
            const Creative* creative_candidate = sub_cit->creative;

            if(!creative_candidate)
            {
              /* need to select creatives available only for selected tag */
              campaign_selection_index_->filter_creatives(
                key,
                tag,
                &tag_sizes,
                sub_cit->campaign,
                request_params.profiling_available,
                request_params.full_freq_caps,
                request_params.seq_orders,
                available_creatives,
                !sub_cit->campaign_keyword.in() ||
                  sub_cit->campaign_keyword->click_url.empty(),
                  // check click url categories
                request_params.up_expand_space,
                request_params.right_expand_space,
                request_params.down_expand_space,
                request_params.left_expand_space,
                request_params.video_min_duration,
                request_params.video_max_duration,
                request_params.video_skippable_max_duration,
                request_params.video_allow_skippable,
                request_params.video_allow_unskippable,
                request_params.allowed_durations,
                request_params.exclude_categories,
                request_params.required_categories,
                request_params.secure,
                request_params.filter_empty_destination,
                0);

              {
                CampaignIndex::ConstCreativePtrList::const_iterator wc_it =
                  Generics::random_select<unsigned long>(
                    available_creatives.begin(),
                    available_creatives.end(),
                    balance_function);

                creative_candidate = (wc_it != available_creatives.end() ? *wc_it : 0);
              }
            }
            else
            {
              available_creatives.push_back(creative_candidate);
            }

            if(creative_candidate)
            {
              filtered_cpc_keyword_map[cit->first].push_back(
                CampaignKeywordCreative(
                  *sub_cit,
                  available_creatives, // clear available_creatives
                  creative_candidate));
              ++selected_keywords;

              // fill multi showing filters
              if(ta_type == AccountDef::TA_ALL) // one per CCG
              {
                ccg_filter_ids.insert(sub_cit->campaign->campaign_id);
              }
              else if(ta_type == AccountDef::TA_ADVERTISER_ONE) // one per advertiser
              {
                advertiser_filter_ids.insert(
                  sub_cit->campaign->advertiser->account_id);
              }
              else if(ta_type == AccountDef::TA_ONE) // one per account
              {
                account_filter_ids.insert(
                  sub_cit->campaign->account->account_id);
              }
            }
          }
        }
      }

      if(!filtered_cpc_keyword_map.empty())
      {
        /* actual ecpm and actual cpc calculate loop:
         *   for 2..N bidders:
         *     actual eCPM = actual cpc * ctr * 1000
         *     actual cpc = maximum cpc
         *   for first bidder:
         *     if number of bidders > 1 (ADSC-6665) :
         *       actual eCPM = min(
         *         max ecpm bid, max(actual eCPM[2], min_ecpm - sum(actual eCPM[2..N])))
         *       actual cpc = actual eCPM / (ctr * 1000)
         *     otherwise:
         *       actual eCPM = actual cpc * ctr * 1000
         *       actual cpc = maximum cpc
         */
        selected_keywords = 0;

        RevenueDecimal non_first_bidders_actual_ecpm_sum(RevenueDecimal::ZERO);
        RevenueDecimal second_bidder_actual_ecpm(RevenueDecimal::ZERO);
        unsigned long bidder_number = 0;
        
        CPCKeywordCreativeMap::reverse_iterator cit =
          filtered_cpc_keyword_map.rbegin();

        ConstCampaignPtrSet sub_lost_ccgs;
        ConstCreativePtrSet sub_lost_creatives;

        for(; cit != filtered_cpc_keyword_map.rend() &&
              selected_keywords < max_keywords - 1;
            ++cit)
        {
          // cit->first is actual cpc in system currency
          CampaignKeywordCreativeList& kws = cit->second;

          CampaignKeywordCreativeList::iterator sub_cit = kws.begin();

          for(; sub_cit != kws.end() && selected_keywords < max_keywords - 1;
              ++sub_cit)
          {
            if(bidder_number > 0)
            {
              ++selected_keywords;
            
              // actual_cpc : one click cost in account currency
              RevenueDecimal actual_cpc(RevenueDecimal::ZERO);
              RevenueDecimal actual_ecpm(RevenueDecimal::ZERO); // system currency
              RevenueDecimal current_ctr;

              CampaignKeyword* campaign_keyword =
                sub_cit->weighted_campaign_keyword.campaign_keyword.in();

              if(ctr_calculation_context)
              {
                // TO CHANGE: we calculate ctr few times !!!
                const Creative* creative;

                actual_ecpm = campaign_keyword_ecpm_(
                  current_ctr,
                  creative,
                  ctr_calculation_context,
                  sub_cit->weighted_campaign_keyword.campaign,
                  campaign_keyword,
                  sub_cit->available_creatives);
              }
              else
              {
                actual_ecpm = default_campaign_keyword_ecpm_(
                  tag,
                  sub_cit->weighted_campaign_keyword.campaign,
                  campaign_keyword);

                current_ctr = campaign_keyword ? campaign_keyword->ctr :
                  sub_cit->weighted_campaign_keyword.campaign->ctr;
              }

              if(campaign_keyword)
              {
                actual_cpc = campaign_keyword->max_cpc;
              }
              else
              {
                actual_cpc = sub_cit->weighted_campaign_keyword.campaign->
                  click_revenue;
              }

              if(bidder_number == 1)
              {
                second_bidder_actual_ecpm = actual_ecpm;
              }

              non_first_bidders_actual_ecpm_sum += actual_ecpm;

              assert(sub_cit->creative);

              weighted_campaign_keywords.push_back(
                WeightedCampaignKeyword(
                  tag,
                  campaign_keyword,
                  sub_cit->weighted_campaign_keyword.campaign,
                  sub_cit->creative,
                  actual_ecpm, // ecpm_bid for non first slots
                  actual_cpc,
                  actual_ecpm,
                  current_ctr,
                  sub_cit->weighted_campaign_keyword.conv_rate
                  ));
            }

            ++bidder_number;
          } // keywords with equal max_ecpm loop

          if(lost_auction)
          {
            // fetch remaining campaigns for fill lost auction
            // campaigns is unique, because minimal filtering for text ad :
            // one ccg for showing
            for(; sub_cit != kws.end(); ++sub_cit)
            {
              sub_lost_ccgs.insert(
                sub_cit->weighted_campaign_keyword.campaign);

              std::copy(
                sub_cit->available_creatives.begin(),
                sub_cit->available_creatives.end(),
                std::inserter(sub_lost_creatives,
                  sub_lost_creatives.begin()));
            }
          }
        } // filtered_cpc_keyword_map loop for 2..N bidders

        // first bidder calculations
        const CampaignKeywordCreativeList& kws =
          filtered_cpc_keyword_map.rbegin()->second;

        const WeightedCampaignKeyword& weighted_campaign =
          kws.front().weighted_campaign_keyword;

        // calculate first bidder actual cpc and actual ecpm
        RevenueDecimal actual_cpc(RevenueDecimal::ZERO);
        // actual_ecpm: ecpm_bid in system currency will be lowered for keyword campaigns
        RevenueDecimal actual_ecpm(RevenueDecimal::ZERO);
        RevenueDecimal max_ecpm_bid(RevenueDecimal::ZERO);
        RevenueDecimal ctr;

        if(weighted_campaign.campaign_keyword && bidder_number > 1)
        {
          // REVIEW: this code eval ctr twice
          const Creative* creative = 0;

          if(ctr_calculation_context && weighted_campaign.campaign->use_ctr())
          {
            max_ecpm_bid = campaign_keyword_ecpm_(
              ctr,
              creative,
              ctr_calculation_context,
              weighted_campaign.campaign,
              weighted_campaign.campaign_keyword,
              kws.front().available_creatives);
          }
          else
          {
            max_ecpm_bid = default_campaign_keyword_ecpm_(
              tag,
              weighted_campaign.campaign,
              weighted_campaign.campaign_keyword);
            ctr = weighted_campaign.campaign_keyword->ctr;
          }

          // correct ecpm only for keyword campaigns
          // min_required_actual_ecpm > 0
          RevenueDecimal min_required_actual_ecpm = std::max(
            second_bidder_actual_ecpm + REVENUE_ONE,
            non_first_bidders_actual_ecpm_sum < min_sum_ecpm ?
              min_sum_ecpm - non_first_bidders_actual_ecpm_sum :
              RevenueDecimal::ZERO);

          actual_ecpm = std::min(max_ecpm_bid, min_required_actual_ecpm);

          const Currency* currency = weighted_campaign.campaign->account->currency;

          if(ctr != RevenueDecimal::ZERO)
          {
            RevenueDecimal ctr_mul = RevenueDecimal::mul(
              ctr,
              ECPM_FACTOR,
              Generics::DMR_FLOOR);

            // actual_cpc can be == 0 if max_ecpm_bid == 0 (
            //   very low ctr * max_cpc),
            // this will be solved after commision revert
            actual_cpc = currency->from_system_currency(
              RevenueDecimal::div(actual_ecpm, ctr_mul));
          }
        }
        else // top campaign isn't keyword or one bidder (bidder_number == 1)
          // use cost defined by advertiser
        {
          if(weighted_campaign.campaign_keyword) // keyword targeted
          {
            actual_cpc = weighted_campaign.campaign_keyword->max_cpc;
          }
          else
          {
            actual_cpc = weighted_campaign.campaign->click_revenue;
          }

          if(ctr_calculation_context)
          {
            const Creative* unused_creative;

            actual_ecpm = campaign_keyword_ecpm_(
              ctr,
              unused_creative,
              ctr_calculation_context,
              weighted_campaign.campaign,
              weighted_campaign.campaign_keyword,
              kws.front().available_creatives);
          }
          else
          {
            actual_ecpm = default_campaign_keyword_ecpm_(
              tag,
              weighted_campaign.campaign,
              weighted_campaign.campaign_keyword);
            ctr = weighted_campaign.campaign_keyword ?
              weighted_campaign.campaign_keyword->ctr :
              weighted_campaign.campaign->ctr;
          }

          max_ecpm_bid = actual_ecpm;
        }

        assert(kws.front().creative);

        weighted_campaign_keywords.push_front(
          WeightedCampaignKeyword(
            tag,
            weighted_campaign.campaign_keyword,
            weighted_campaign.campaign,
            kws.front().creative,
            actual_ecpm, // ecpm_bid (truncated ecpm)
            actual_cpc,
            max_ecpm_bid,
            ctr,
            weighted_campaign.conv_rate
            ));

        revert_cost_(weighted_campaign_keywords);

        // use truncated ecpm values for garantee revenue
        bool text_ads_selected = (
          non_first_bidders_actual_ecpm_sum + actual_ecpm) >=
          min_sum_ecpm;

        if(lost_auction)
        {
          if(!text_ads_selected)
          {
            // all campaigns lost
            cit = filtered_cpc_keyword_map.rbegin();
          }
          else
          {
            std::copy(
              sub_lost_ccgs.begin(),
              sub_lost_ccgs.end(),
              std::inserter(lost_auction->ccgs,
                lost_auction->ccgs.begin()));
            std::copy(
              sub_lost_creatives.begin(),
              sub_lost_creatives.end(),
              std::inserter(lost_auction->creatives,
                lost_auction->creatives.begin()));

            // first bidder win if text ads selected
            if(cit == filtered_cpc_keyword_map.rbegin())
            {
              ++cit;
            }
          }

          for(; cit != filtered_cpc_keyword_map.rend(); ++cit)
          {
            CampaignKeywordCreativeList& kws = cit->second;

            for(CampaignKeywordCreativeList::iterator sub_cit = kws.begin();
                sub_cit != kws.end(); ++sub_cit)
            {
              lost_auction->ccgs.insert(
                sub_cit->weighted_campaign_keyword.campaign);
              std::copy(
                sub_cit->available_creatives.begin(),
                sub_cit->available_creatives.end(),
                std::inserter(lost_auction->creatives,
                  lost_auction->creatives.begin()));
            }
          }
        }

        return text_ads_selected;
      } // !filtered_cpc_keyword_map.empty()

      return false;
    }
    
    void
    CampaignSelector::cross_campaigns_with_keywords_(
      const CampaignIndex::Key& key,
      const CampaignSelectParams& request_params,
      const ChannelIdHashSet& matched_channels,
      const CampaignIndex::CampaignCellPtrList& campaigns,
      const CampaignKeywordMap& campaign_keywords,
      CampaignKeywordMap& filtered_campaign_keywords)
      const
      noexcept
    {
      /* intersect campaigns & campaign_keywords sets */
      CampaignIndex::CampaignCellPtrList::const_iterator
        cmp_it = campaigns.begin();
      CampaignKeywordMap::const_iterator kw_it = campaign_keywords.begin();

      while(cmp_it != campaigns.end() && kw_it != campaign_keywords.end())
      {
        if((*cmp_it)->campaign->campaign_id < kw_it->first)
        {
          ++cmp_it;
        }
        else if((*cmp_it)->campaign->campaign_id == kw_it->first)
        {
          // don't check ecpm for text campaigns, will be checked sum
          if(campaign_selection_index_->check_campaign(
               key,
               kw_it->second->campaign,
               request_params.time,
               request_params.profiling_available,
               request_params.full_freq_caps,
               request_params.colocation->colo_id,
               request_params.user_create_time,
               request_params.user_id,
               0) &&
             // check non keyword channel conditions
             campaign_selection_index_->check_campaign_channel(
               kw_it->second->campaign,
               matched_channels))
          {
            filtered_campaign_keywords.insert(*kw_it);
            ++kw_it;
          }
          else
          {
            ++cmp_it;
          }
        }
        else
        {
          ++kw_it;
        }
      }
    }

    RevenueDecimal
    CampaignSelector::sum_campaign_ecpm_(
      const WeightedCampaignList& campaign_candidates)
      noexcept
    {
      RevenueDecimal result = RevenueDecimal::ZERO;

      for(WeightedCampaignList::const_iterator it =
            campaign_candidates.begin();
          it != campaign_candidates.end(); ++it)
      {
        result += (*it)->ecpm;
      }

      return result;
    }

    RevenueDecimal
    CampaignSelector::campaign_ecpm_expected_value_(
      const WeightedCampaignList& campaign_candidates)
      noexcept
    {
      ExpectedEcpm expected_ecpm;

      for(WeightedCampaignList::const_iterator cmp_it =
            campaign_candidates.begin();
          cmp_it != campaign_candidates.end(); ++cmp_it)
      {
        expected_ecpm += (*cmp_it)->ecpm;
      }

      return expected_ecpm.value();
    }

    template<typename EcpmOutType>
    void
    CampaignSelector::group_id_map_(
      ExpRevWeightedCampaignKeywordMap& text_campaign_map,
      EcpmOutType& expected_ecpm,
      const IdWeightedCampaignKeywordMap& campaigns)
      noexcept
    {
      for(IdWeightedCampaignKeywordMap::const_iterator group_it =
            campaigns.begin();
          group_it != campaigns.end(); ++group_it)
      {
        RevenueDecimal exp_ecpm_val;

        if(group_it->second.size() == 1)
        {
          exp_ecpm_val = (*group_it->second.begin())->actual_ecpm;
        }
        else
        {
          ExpectedEcpm exp_ecpm;

          for(WeightedCampaignKeywordPtrArray::const_iterator
                cmp_it = group_it->second.begin();
              cmp_it != group_it->second.end(); ++cmp_it)
          {
            exp_ecpm += (*cmp_it)->actual_ecpm;
          }

          exp_ecpm_val = exp_ecpm.value();
        }

        expected_ecpm += exp_ecpm_val;
        text_campaign_map.insert(std::make_pair(
          exp_ecpm_val, group_it->second));
      }
    }

    void
    CampaignSelector::group_id_map_(
      WeightedCampaignKeywordGroupList& text_campaign_map,
      IdWeightedCampaignKeywordMap& campaigns)
      noexcept
    {
      for(IdWeightedCampaignKeywordMap::iterator group_it =
            campaigns.begin();
          group_it != campaigns.end(); ++group_it)
      {
        text_campaign_map.push_back(WeightedCampaignKeywordPtrArray());
        text_campaign_map.back().swap(group_it->second);
      }
    }

    RevenueDecimal
    CampaignSelector::eval_text_campaigns_max_sum_(
      const ExpRevWeightedCampaignKeywordMap& grouped_text_campaign_candidates,
      unsigned long max_text_creatives)
      noexcept
    {
      std::set<RevenueDecimal> cur_elements;

      for(ExpRevWeightedCampaignKeywordMap::const_iterator group_it =
            grouped_text_campaign_candidates.begin();
          group_it != grouped_text_campaign_candidates.end();
          ++group_it)
      {
        RevenueDecimal group_max = RevenueDecimal::ZERO;

        for(WeightedCampaignKeywordPtrArray::const_iterator
              cmp_it = group_it->second.begin();
            cmp_it != group_it->second.end(); ++cmp_it)
        {
          if((*cmp_it)->actual_ecpm > group_max)
          {
            group_max = (*cmp_it)->actual_ecpm;
          }
        }

        if(cur_elements.empty() ||
           group_max > *cur_elements.begin())
        {
          cur_elements.insert(group_max);

          if(cur_elements.size() > max_text_creatives)
          {
            cur_elements.erase(cur_elements.begin());
          }
        }
      }

      RevenueDecimal res = RevenueDecimal::ZERO;

      for(std::set<RevenueDecimal>::const_iterator it =
            cur_elements.begin();
          it != cur_elements.end(); ++it)
      {
        res += *it;
      }

      return res;
    }

    void
    CampaignSelector::get_campaigns_group(
      WeightedCampaignKeywordPtrArray*& campaigns_group,
      IdWeightedCampaignKeywordMaps& campaigns_maps,
      WeightedCampaignKeyword* wc)
    {
      AccountDef::TextAdserving ta_type =
        wc->campaign->account->get_text_adserving();

      IdWeightedCampaignKeywordMap* selected_map = 0;
      unsigned long selected_id = 0;

      switch (ta_type)
      {
      case AccountDef::TA_ALL:
        selected_id = wc->campaign->campaign_id;
        selected_map = &campaigns_maps.ccg_campaigns;
        break;
      case AccountDef::TA_ADVERTISER_ONE:
        selected_id = wc->campaign->advertiser->account_id;
        selected_map = &campaigns_maps.advertiser_campaigns;
        break;
      case AccountDef::TA_ONE:
        selected_id = wc->campaign->account->account_id;
        selected_map = &campaigns_maps.account_campaigns;
        break;
      };

      IdWeightedCampaignKeywordMap::iterator iter = selected_map->find(selected_id);
      if (iter != selected_map->end())
      {
        campaigns_group = &(iter->second);
      }
    }

    void
    CampaignSelector::fill_id_weighted_campaigns_keyword_maps(
      IdWeightedCampaignKeywordMaps& campaigns_maps,
      WeightedCampaignKeywordList& text_campaign_candidates)
    {
      for(WeightedCampaignKeywordList::iterator cmp_it =
            text_campaign_candidates.begin();
          cmp_it != text_campaign_candidates.end(); ++cmp_it)
      {
        AccountDef::TextAdserving ta_type =
          cmp_it->campaign->account->get_text_adserving();

        if(ta_type == AccountDef::TA_ALL) // one per CCG
        {
          campaigns_maps.ccg_campaigns[cmp_it->campaign->campaign_id].push_back(&*cmp_it);
        }
        else if(ta_type == AccountDef::TA_ADVERTISER_ONE) // one per advertiser
        {
          campaigns_maps.advertiser_campaigns[
            cmp_it->campaign->advertiser->account_id].push_back(&*cmp_it);
        }
        else if(ta_type == AccountDef::TA_ONE) // one per account
        {
          campaigns_maps.account_campaigns[
            cmp_it->campaign->account->account_id].push_back(&*cmp_it);
        }
      }
    }

    void
    CampaignSelector::group_text_campaigns_(
      ExpRevWeightedCampaignKeywordMap& text_campaign_map,
      ExpectedEcpm& expected_ecpm,
      WeightedCampaignKeywordList& text_campaign_candidates)
      noexcept
    {
      IdWeightedCampaignKeywordMaps campsigns_maps;
      fill_id_weighted_campaigns_keyword_maps(campsigns_maps, text_campaign_candidates);

      group_id_map_(text_campaign_map, expected_ecpm, campsigns_maps.ccg_campaigns);
      group_id_map_(text_campaign_map, expected_ecpm, campsigns_maps.advertiser_campaigns);
      group_id_map_(text_campaign_map, expected_ecpm, campsigns_maps.account_campaigns);
    }

    void
    CampaignSelector::group_text_campaigns_(
      ExpRevWeightedCampaignKeywordMap& text_campaign_map,
      RevenueDecimal& ecpm_sum,
      WeightedCampaignKeywordPtrArray& text_campaign_candidates)
      noexcept
    {
      ecpm_sum = RevenueDecimal::ZERO;

      IdWeightedCampaignKeywordMap account_campaigns;
      IdWeightedCampaignKeywordMap advertiser_campaigns;
      IdWeightedCampaignKeywordMap ccg_campaigns;

      for(WeightedCampaignKeywordPtrArray::iterator cmp_it =
            text_campaign_candidates.begin();
          cmp_it != text_campaign_candidates.end(); ++cmp_it)
      {
        AccountDef::TextAdserving ta_type =
          (*cmp_it)->campaign->account->get_text_adserving();

        if(ta_type == AccountDef::TA_ALL) // one per CCG
        {
          ccg_campaigns[(*cmp_it)->campaign->campaign_id].push_back(*cmp_it);
        }
        else if(ta_type == AccountDef::TA_ADVERTISER_ONE) // one per advertiser
        {
          advertiser_campaigns[
            (*cmp_it)->campaign->advertiser->account_id].push_back(*cmp_it);
        }
        else if(ta_type == AccountDef::TA_ONE) // one per account
        {
          account_campaigns[
            (*cmp_it)->campaign->account->account_id].push_back(*cmp_it);
        }
      }

      group_id_map_(text_campaign_map, ecpm_sum, ccg_campaigns);
      group_id_map_(text_campaign_map, ecpm_sum, advertiser_campaigns);
      group_id_map_(text_campaign_map, ecpm_sum, account_campaigns);
    }

    void
    CampaignSelector::group_text_campaigns_(
      WeightedCampaignKeywordGroupList& text_campaign_map,
      WeightedCampaignKeywordList& text_campaign_candidates)
      noexcept
    {
      IdWeightedCampaignKeywordMaps campsigns_maps;
      fill_id_weighted_campaigns_keyword_maps(campsigns_maps, text_campaign_candidates);

      group_id_map_(text_campaign_map, campsigns_maps.ccg_campaigns);
      group_id_map_(text_campaign_map, campsigns_maps.advertiser_campaigns);
      group_id_map_(text_campaign_map, campsigns_maps.account_campaigns);
    }

    CampaignSelector::WeightedCampaignList::iterator
    CampaignSelector::select_campaign_prop_probability_(
      WeightedCampaignList& campaign_candidates,
      const RevenueDecimal& ecpm_offset)
      noexcept
    {
      if(campaign_candidates.empty())
      {
        return campaign_candidates.end();
      }

      RevenueDecimal cur_ecpm_offset = RevenueDecimal::ZERO;

      WeightedCampaignList::iterator it =
        campaign_candidates.begin();

      for(; it != campaign_candidates.end(); ++it)
      {
        cur_ecpm_offset += (*it)->ecpm;
        if(cur_ecpm_offset > ecpm_offset)
        {
          break;
        }
      }

      assert(it != campaign_candidates.end());
      return it;
    }

    CampaignSelector::WeightedCampaignList::iterator
    CampaignSelector::select_campaign_randomly_(
      WeightedCampaignList& campaign_candidates)
      noexcept
    {
      if(campaign_candidates.empty())
      {
        return campaign_candidates.end();
      }

      unsigned long offset = Generics::safe_rand(campaign_candidates.size());

      WeightedCampaignList::iterator it =
        campaign_candidates.begin();

      std::advance(it, offset);

      return it;
    }

    void
    CampaignSelector::filter_text_campaign_candidates_(
      WeightedCampaignKeywordList& filtered_text_campaigns,
      const WeightedCampaignKeywordList& text_campaigns,
      const CampaignIndex::Key& key,
      const CampaignSelectParams& request_params,
      const Tag* tag,
      const Tag::Size* tag_size,
      const CTRProvider::CalculationContext* ctr_calculation_context,
      const CTRProvider::CalculationContext* conv_rate_calculation_context)
      const
      noexcept
    {
      Tag::SizeMap tag_sizes;
      tag_sizes.insert(std::make_pair(
        tag_size->size->size_id, ReferenceCounting::add_ref(tag_size)));

      for(WeightedCampaignKeywordList::const_iterator cmp_it =
            text_campaigns.begin();
          cmp_it != text_campaigns.end(); ++cmp_it)
      {
        // filter campaign with BS_MIN_CTR_GOAL if algo isn't default
        if(cmp_it->campaign->bid_strategy != BS_MIN_CTR_GOAL ||
           ctr_calculation_context ||
           cmp_it->campaign->ctr > cmp_it->campaign->min_ctr_goal())
        {
          if(cmp_it->creative == 0)
          {
            CampaignIndex::ConstCreativePtrList available_creatives;
            const Creative* creative_candidate = 0;

            // need to select creatives available only for selected tag
            campaign_selection_index_->filter_creatives(
              key,
              tag,
              &tag_sizes,
              cmp_it->campaign,
              request_params.profiling_available,
              request_params.full_freq_caps,
              request_params.seq_orders,
              available_creatives,
              !cmp_it->campaign_keyword.in() ||
                cmp_it->campaign_keyword->click_url.empty(),
              // check click url categories
              request_params.up_expand_space,
              request_params.right_expand_space,
              request_params.down_expand_space,
              request_params.left_expand_space,
              request_params.video_min_duration,
              request_params.video_max_duration,
              request_params.video_skippable_max_duration,
              request_params.video_allow_skippable,
              request_params.video_allow_unskippable,
              request_params.allowed_durations,
              request_params.exclude_categories,
              request_params.required_categories,
              request_params.secure,
              request_params.filter_empty_destination,
              0);

            if(!available_creatives.empty())
            {
              bool change_ecpm = false;
              RevenueDecimal max_ctr = RevenueDecimal::ZERO;
              RevenueDecimal max_conv_rate = RevenueDecimal::ZERO;
              RevenueDecimal new_ecpm;

              if((ctr_calculation_context && (
                   cmp_it->campaign->use_ctr() ||
                   cmp_it->campaign->bid_strategy == BS_MIN_CTR_GOAL)) ||
                 conv_rate_calculation_context)
              {
                CampaignIndex::ConstCreativePtrList max_ecpm_available_creatives;

                change_ecpm = ctr_calculation_context && cmp_it->campaign->use_ctr();

                for(CampaignIndex::ConstCreativePtrList::const_iterator cr_it =
                      available_creatives.begin();
                    cr_it != available_creatives.end(); ++cr_it)
                {
                  RevenueDecimal conv_rate = RevenueDecimal::ZERO;

                  if(!conv_rate_calculation_context ||
                     conv_rate_calculation_context->check_rate(*cr_it, &conv_rate))
                  {
                    if(!ctr_calculation_context)
                    {
                      max_ecpm_available_creatives.push_back(*cr_it);
                    }
                    else
                    {
                      RevenueDecimal ctr = ctr_calculation_context->get_ctr(*cr_it);

                      if(ctr >= max_ctr && (
                           cmp_it->campaign->bid_strategy != BS_MIN_CTR_GOAL ||
                           ctr >= cmp_it->campaign->min_ctr_goal()))
                      {
                        // for candidates with equal ctr select candidate with highest conv_rate
                        if(ctr == max_ctr && conv_rate == max_conv_rate)
                        {
                          max_ecpm_available_creatives.push_back(*cr_it);
                        }
                        else
                        {
                          max_ecpm_available_creatives.clear();
                          max_ecpm_available_creatives.push_back(*cr_it);
                          max_ctr = ctr;
                          max_conv_rate = conv_rate;
                        }
                      }

                      if(cmp_it->campaign->use_ctr())
                      {
                        if(cmp_it->campaign_keyword)
                        {
                          new_ecpm = cmp_it->campaign_keyword->campaign->
                            account->currency->to_system_currency(
                              RevenueDecimal::mul(
                                RevenueDecimal::mul(
                                  ECPM_FACTOR, max_ctr, Generics::DMR_FLOOR),
                                cmp_it->campaign_keyword->max_cpc,
                                Generics::DMR_FLOOR));
                        }
                        else
                        {
                          new_ecpm = RevenueDecimal::mul(
                            cmp_it->campaign->click_sys_revenue,
                            RevenueDecimal::mul(max_ctr, ECPM_FACTOR, Generics::DMR_FLOOR),
                            Generics::DMR_FLOOR);
                        }
                      }
                    }
                  }
                }

                max_ecpm_available_creatives.swap(available_creatives);
              }

              CampaignIndex::ConstCreativePtrList::const_iterator wc_it =
                Generics::random_select<unsigned long>(
                  available_creatives.begin(),
                  available_creatives.end(),
                  balance_function);

              creative_candidate = (wc_it != available_creatives.end() ? *wc_it : 0);

              if(creative_candidate)
              {
                filtered_text_campaigns.push_back(*cmp_it);
                filtered_text_campaigns.back().creative = creative_candidate;
                if(change_ecpm)
                {
                  filtered_text_campaigns.back().actual_ecpm = new_ecpm;
                  filtered_text_campaigns.back().ecpm = new_ecpm;
                }
                filtered_text_campaigns.back().conv_rate = max_conv_rate;
              }
            }
          }
          else
          {
            filtered_text_campaigns.push_back(*cmp_it);
          }
        }
      }
    }

    void
    CampaignSelector::select_campaigns_randomly_(
      WeightedCampaignKeywordListPtr& result_text_campaigns,
      WeightedCampaignPtr& result_display_campaign,
      AdSelectionResult& select_result,
      const CampaignIndex::Key& key,
      const CampaignSelectParams& request_params,
      const Tag* tag,
      const CTRProvider::Calculation* ctr_calculation,
      const CTRProvider::Calculation* conv_rate_calculation,
      const Tag::SizeMap& tag_sizes,
      const ChannelIdHashSet& matched_channels,
      const CampaignKeywordMap& matched_keywords)
      const
      noexcept
    {
      if(request_params.min_pub_ecpm > tag->pub_max_random_cpm)
      {
        return;
      }

      CampaignIndex::CampaignSelectionCellPtrList wg_display_check_campaigns;
      CampaignIndex::CampaignSelectionCellPtrList display_check_campaigns;
      CampaignIndex::CampaignCellPtrList keyword_check_campaigns;
      CampaignIndex::CampaignCellPtrList text_check_campaigns;

      campaign_selection_index_->get_random_campaigns(
        key,
        wg_display_check_campaigns,
        display_check_campaigns,
        text_check_campaigns,
        keyword_check_campaigns);

      // get WG display candidates (have priority over all other campaigns)
      WeightedCampaignList wg_display_campaign_candidates;

      get_all_display_campaign_candidates_(
        wg_display_campaign_candidates,
        0, // lost_campaigns in RANDOM not collected?
        key,
        request_params,
        tag,
        ctr_calculation,
        conv_rate_calculation,
        matched_channels,
        wg_display_check_campaigns,
        false // check min ecpm
        );

      if(!wg_display_campaign_candidates.empty())
      {
        // select WG display campaign
        unsigned long campaign_offset = Generics::safe_rand(
          wg_display_campaign_candidates.size());

        WeightedCampaignList::iterator cmp_it =
          wg_display_campaign_candidates.begin();

        std::advance(cmp_it, campaign_offset);

        result_display_campaign = std::move(*cmp_it);

        select_result.walled_garden = true;
      }

      if(!result_display_campaign.get() &&
         tag->marketplace != 'W' // tag allow only WG auction
         )
      {
        // get OIX display candidates
        WeightedCampaignList display_campaign_candidates;

        get_all_display_campaign_candidates_(
          display_campaign_candidates,
          0, // lost_campaigns in randon not collected? FIXME
          key,
          request_params,
          tag,
          ctr_calculation,
          conv_rate_calculation,
          matched_channels,
          display_check_campaigns,
          false // check min ecpm
          );

        // get text candidates
        RandomTextSelectionBySizeList text_selections;
        unsigned long text_count = 0;

        if(!request_params.only_display_ad)
        {
          const Tag::TagPricing* text_tag_pricing = tag->select_tag_pricing(
            request_params.country_code.c_str(),
            CT_TEXT,
            CR_ALL
            // tag pricing for text campaigns must have
            // 'all' ccg rates targeting
            );

          select_result.min_text_ecpm = std::max(
            text_tag_pricing->cpm,
            request_params.min_ecpm);

          WeightedCampaignKeywordList text_campaign_candidates;

          get_text_campaign_candidates_(
            text_campaign_candidates,
            key,
            request_params,
            tag,
            0, // ctr_calculation_context
            matched_keywords,
            matched_channels,
            keyword_check_campaigns,
            text_check_campaigns);

          if(select_result.min_text_ecpm <= tag->max_random_cpm)
          {
            for(Tag::SizeMap::const_iterator tag_size_it = tag_sizes.begin();
                tag_size_it != tag_sizes.end(); ++tag_size_it)
            {
              if (tag_size_it->second->max_text_creatives == 0)
              {
                continue;
              }

              CTRProvider::CalculationContext_var ctr_calculation_context;

              if(ctr_calculation)
              {
                ctr_calculation_context = ctr_calculation->create_context(
                  tag_size_it->second);
              }

              CTRProvider::CalculationContext_var conv_rate_calculation_context;
              if(conv_rate_calculation)
              {
                conv_rate_calculation_context = conv_rate_calculation->create_context(
                  tag_size_it->second);
              }

              WeightedCampaignKeywordList filtered_text_campaign_candidates;

              filter_text_campaign_candidates_(
                filtered_text_campaign_candidates,
                text_campaign_candidates,
                key,
                request_params,
                tag,
                tag_size_it->second,
                ctr_calculation_context,
                conv_rate_calculation_context);

              if(!filtered_text_campaign_candidates.empty())
              {
                text_selections.push_back(RandomTextSelectionBySize());
                RandomTextSelectionBySize& text_selection = text_selections.back();
                text_selection.campaign_candidates.swap(filtered_text_campaign_candidates);
                text_selection.tag_sizes.insert(
                  std::make_pair(tag_size_it->second->size->size_id, tag_size_it->second));
                group_text_campaigns_(
                  text_selection.grouped_campaign_candidates,
                  text_selection.campaign_candidates);
              }
            }
          }

          std::set<ConstCampaignPtrSet> uniq_text_campaign_groups;

          for(auto ts_it = text_selections.begin();
              ts_it != text_selections.end(); ++ts_it)
          {
            for(auto cmpg_it = ts_it->grouped_campaign_candidates.begin();
                cmpg_it != ts_it->grouped_campaign_candidates.end(); ++cmpg_it)
            {
              ConstCampaignPtrSet uniq_text_campaign_candidates;

              for (auto cmp_it = cmpg_it->begin();
                   cmp_it != cmpg_it->end(); ++cmp_it)
              {
                uniq_text_campaign_candidates.insert((*cmp_it)->campaign);
              }

              uniq_text_campaign_groups.insert(uniq_text_campaign_candidates);
            }
          }

          text_count = uniq_text_campaign_groups.size();
        }

        // choose display or text
        unsigned long display_count = display_campaign_candidates.size();

        // implement selection by factors
        // T = max ( <text ads available> / <maximum text ads in tag> , 1 )
        // D = <display ads available>
        // in integer values
        unsigned long sum_max_text_creatives = 0;
        unsigned long ts_size = 0;
        for(RandomTextSelectionBySizeList::const_iterator ts_it =
              text_selections.begin();
            ts_it != text_selections.end(); ++ts_it)
        {
          assert(ts_it->tag_sizes.size() == 1);
          sum_max_text_creatives += ts_it->tag_sizes.begin()->second->max_text_creatives;
          ++ts_size;
        }

        unsigned long campaign_offset = Generics::safe_rand(
          (text_count > 0 ? sum_max_text_creatives * display_count +
          std::max(text_count * ts_size, sum_max_text_creatives) : display_count));

        if(display_count > 0 && (
             campaign_offset < sum_max_text_creatives * display_count ||
             sum_max_text_creatives == 0))
        {
          if (sum_max_text_creatives)
          {
            campaign_offset /= sum_max_text_creatives;
          }

          // select display campaign (OIX)
          WeightedCampaignList::iterator cmp_it =
            display_campaign_candidates.begin();

          std::advance(cmp_it, campaign_offset);

          result_display_campaign = std::move(*cmp_it);
        }
        else if(text_count > 0)
        {
          // select text cell
          unsigned long ts_offset = Generics::safe_rand(sum_max_text_creatives);

          unsigned long cur_sum_max_text_creatives = 0;
          RandomTextSelectionBySizeList::iterator ts_it =
            text_selections.begin();

          for(; ts_it != text_selections.end(); ++ts_it)
          {
            assert(ts_it->tag_sizes.size() == 1);
            cur_sum_max_text_creatives += ts_it->tag_sizes.begin()->second->max_text_creatives;

            if(ts_offset < cur_sum_max_text_creatives)
            {
              break;
            }
          }

          assert(ts_it != text_selections.end());

          const Tag::Size* tag_size = ts_it->tag_sizes.begin()->second;
          select_result.tag_size = tag_size;
          result_text_campaigns.reset(new WeightedCampaignKeywordList());

          select_text_campaigns_randomly_(
            *result_text_campaigns,
            ts_it->grouped_campaign_candidates,
            tag_size->max_text_creatives);
        }
      }
      else
      {
        select_result.walled_garden = true;
      }
    }

    void
    CampaignSelector::select_campaigns_prop_probability_(
      WeightedCampaignKeywordListPtr& result_text_campaigns,
      WeightedCampaignPtr& result_display_campaign,
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
      noexcept
    {
      LostAuction* lost_auction = collect_lost ? &select_result.lost_auction : 0;

      CampaignIndex::CampaignSelectionCellPtrList wg_display_check_campaigns;
      CampaignIndex::CampaignSelectionCellPtrList display_check_campaigns;
      CampaignIndex::CampaignCellPtrList keyword_check_campaigns;
      CampaignIndex::CampaignCellPtrList text_check_campaigns;
      CampaignIndex::CampaignCellPtrList lost_wg_campaigns;
      CampaignIndex::CampaignCellPtrList lost_campaigns;
      CampaignIndex::CampaignSelectionCellPtrList get_all_display_lost_campaigns;

      campaign_selection_index_->get_campaigns(
        key,
        wg_display_check_campaigns,
        display_check_campaigns,
        text_check_campaigns,
        keyword_check_campaigns,
        collect_lost ? &lost_wg_campaigns : 0,
        collect_lost ? &lost_campaigns : 0);

      // get WG display candidates (have priority over all other campaigns)
      WeightedCampaignList wg_display_campaign_candidates;

      get_all_display_campaign_candidates_(
        wg_display_campaign_candidates,
        0, // don't collect lost campagns for WG
        key,
        request_params,
        tag,
        ctr_calculation,
        conv_rate_calculation,
        matched_channels,
        wg_display_check_campaigns,
        true // check min ecpm
        );

      // select WG display campaign
      if(!wg_display_campaign_candidates.empty())
      {
        RevenueDecimal wg_display_ecpm_sum = sum_campaign_ecpm_(
          wg_display_campaign_candidates);

        RevenueDecimal ecpm_offset = RevenueDecimal::mul(
          wg_display_ecpm_sum,
          RevenueDecimal::div(
            RevenueDecimal(request_params.random2, 0),
            RevenueDecimal(RANDOM_PARAM_MAX, 0)),
            Generics::DMR_FLOOR);

        result_display_campaign = std::move(
          *select_campaign_prop_probability_(wg_display_campaign_candidates,
            ecpm_offset));
      }

      if(!result_display_campaign.get() &&
         tag->marketplace != 'W' // tag allow only WG auction
         )
      {
        // get OIX display candidates
        WeightedCampaignList display_campaign_candidates;

        get_all_display_campaign_candidates_(
          display_campaign_candidates,
          collect_lost ? &get_all_display_lost_campaigns : 0,
          key,
          request_params,
          tag,
          ctr_calculation,
          conv_rate_calculation,
          matched_channels,
          display_check_campaigns,
          true // check min ecpm
          );

        const Tag::TagPricing* text_tag_pricing = tag->select_tag_pricing(
          request_params.country_code.c_str(),
          CT_TEXT,
          CR_ALL
          // tag pricing for text campaigns must have
          // 'all' ccg rates targeting
          );

        TextSelectionBySizeList text_selections;
        TextSelectionBySizeList text_selections_zero_ecpm;
        RevenueDecimal text_selections_ecpm_sum = RevenueDecimal::ZERO;

        // get text candidates
        if(!request_params.only_display_ad)
        {
          WeightedCampaignKeywordList text_campaign_candidates;

          get_text_campaign_candidates_(
            text_campaign_candidates,
            key,
            request_params,
            tag,
            0, // ctr_calculation_context
            matched_keywords,
            matched_channels,
            keyword_check_campaigns,
            text_check_campaigns);

          for(Tag::SizeMap::const_iterator tag_size_it = tag_sizes.begin();
              tag_size_it != tag_sizes.end(); ++tag_size_it)
          {
            if (tag_size_it->second->max_text_creatives == 0)
            {
              continue;
            }

            CTRProvider::CalculationContext_var ctr_calculation_context;
            if(ctr_calculation)
            {
              ctr_calculation_context = ctr_calculation->create_context(
                tag_size_it->second);
            }

            CTRProvider::CalculationContext_var conv_rate_calculation_context;
            if(conv_rate_calculation)
            {
              conv_rate_calculation_context = conv_rate_calculation->create_context(
                tag_size_it->second);
            }

            RevenueDecimal max_text_ecpm_sum = RevenueDecimal::ZERO;
            WeightedCampaignKeywordList filtered_text_campaign_candidates;

            filter_text_campaign_candidates_(
              filtered_text_campaign_candidates,
              text_campaign_candidates,
              key,
              request_params,
              tag,
              tag_size_it->second,
              ctr_calculation_context,
              conv_rate_calculation_context);

            ExpRevWeightedCampaignKeywordMap grouped_text_campaign_candidates;
            ExpectedEcpm expected_ecpm;

            group_text_campaigns_(
              grouped_text_campaign_candidates,
              expected_ecpm,
              filtered_text_campaign_candidates);

            const std::size_t slots_count = std::min(
              tag_size_it->second->max_text_creatives,
              grouped_text_campaign_candidates.size());

            const RevenueDecimal text_expected_ecpm =
              RevenueDecimal::mul(expected_ecpm.value(),
              RevenueDecimal(false, slots_count, 0),
              Generics::DMR_FLOOR);
            assert(text_expected_ecpm >= RevenueDecimal::ZERO);

            max_text_ecpm_sum = eval_text_campaigns_max_sum_(
              grouped_text_campaign_candidates,
              tag_size_it->second->max_text_creatives);

            if(max_text_ecpm_sum >= text_tag_pricing->cpm)
            {
              TextSelectionBySize text_selection;
              text_selection.tag_sizes.insert(
                std::make_pair(tag_size_it->second->size->size_id, tag_size_it->second));
              text_selection.expected_ecpm = text_expected_ecpm;
              text_selection.campaign_candidates.swap(filtered_text_campaign_candidates);

              if (text_expected_ecpm != RevenueDecimal::ZERO)
              {
                text_selections_ecpm_sum += text_expected_ecpm;
                text_selections.push_back(text_selection);
              }
              else
              {
                text_selections_zero_ecpm.push_back(text_selection);
              }
            }
          }
        }

        select_result.min_text_ecpm = std::max(
          text_tag_pricing->cpm,
          request_params.min_ecpm);

        // select text selection by size
        TextSelectionBySize* result_text_selection = 0;

        if (!text_selections.empty())
        {
          const RevenueDecimal text_selection_ecpm_offset = RevenueDecimal::mul(
            text_selections_ecpm_sum,
            RevenueDecimal::div(
              RevenueDecimal(request_params.random2, 0),
              RevenueDecimal(RANDOM_PARAM_MAX, 0)),
              Generics::DMR_FLOOR);

          RevenueDecimal cur_ecpm_offset = RevenueDecimal::ZERO;

          for(TextSelectionBySizeList::iterator text_selection_it =
                text_selections.begin();
              text_selection_it != text_selections.end(); ++text_selection_it)
          {
            cur_ecpm_offset += text_selection_it->expected_ecpm;
            if(cur_ecpm_offset > text_selection_ecpm_offset)
            {
              result_text_selection = &*text_selection_it;
              break;
            }
          }
        }
        else if (!text_selections_zero_ecpm.empty())
        {
          //equal weights for zero ecpm candidates
          const RevenueDecimal text_selection_ecpm_offset = RevenueDecimal::mul(
            RevenueDecimal(text_selections_zero_ecpm.size(), 0),
            RevenueDecimal::div(
              RevenueDecimal(request_params.random2, 0),
              RevenueDecimal(RANDOM_PARAM_MAX, 0)),
              Generics::DMR_FLOOR);

          RevenueDecimal cur_ecpm_offset = RevenueDecimal::ZERO;

          for(auto text_selection_zero_ecpm_it = text_selections_zero_ecpm.begin();
              text_selection_zero_ecpm_it != text_selections.end();
              ++text_selection_zero_ecpm_it)
          {
            cur_ecpm_offset += REVENUE_ONE;

            if (cur_ecpm_offset > text_selection_ecpm_offset)
            {
              result_text_selection = &*text_selection_zero_ecpm_it;
              break;
            }
          }
        }

        // choose display or text (sum ecpm proportionaly)
        RevenueDecimal display_ecpm_expected_value =
          campaign_ecpm_expected_value_(
            display_campaign_candidates);

        RevenueDecimal text_ecpm_expected_value = (
          result_text_selection ? result_text_selection->expected_ecpm :
            RevenueDecimal::ZERO);

        if (display_ecpm_expected_value == RevenueDecimal::ZERO &&
            text_ecpm_expected_value == RevenueDecimal::ZERO)
        {
          if(!display_campaign_candidates.empty())
          {
            display_ecpm_expected_value = REVENUE_ONE;
          }

          if(!result_text_selection ||
             !result_text_selection->campaign_candidates.empty())
          {
            text_ecpm_expected_value = REVENUE_ONE;
          }
        }

        const RevenueDecimal ecpm_offset =
          RevenueDecimal::mul(
            display_ecpm_expected_value + text_ecpm_expected_value,
            RevenueDecimal::div(
              RevenueDecimal(request_params.random2, 0),
              RevenueDecimal(RANDOM_PARAM_MAX, 0)),
            Generics::DMR_FLOOR);

        if(ecpm_offset < display_ecpm_expected_value)
        {
          const RevenueDecimal display_ecpm_sum = sum_campaign_ecpm_(
            display_campaign_candidates);

          WeightedCampaignList::iterator result_cmp_it;

          if(display_ecpm_sum != RevenueDecimal::ZERO)
          {
            RevenueDecimal display_ecpm_offset = RevenueDecimal::mul(
              RevenueDecimal::div(
                RevenueDecimal(false, Generics::safe_rand(), 0),
                RevenueDecimal(false, RAND_MAX, 0)),
              display_ecpm_sum,
              Generics::DMR_FLOOR);

            // select display campaign (OIX)
            result_cmp_it = select_campaign_prop_probability_(
              display_campaign_candidates,
              display_ecpm_offset);
          }
          else
          {
            result_cmp_it = select_campaign_randomly_(
              display_campaign_candidates);
          }

          result_display_campaign = std::move(*result_cmp_it);
        }
        else if (result_text_selection &&
                 !result_text_selection->campaign_candidates.empty())
        {
          result_text_campaigns.reset(new WeightedCampaignKeywordList());
          const Tag::Size* tag_size = result_text_selection->tag_sizes.begin()->second;
          select_result.tag_size = tag_size;

          const RevenueDecimal min_text_ecpm = std::max(
            text_tag_pricing->cpm,
            request_params.min_ecpm);

          select_text_campaigns_prop_probability_(
            *result_text_campaigns,
            result_text_selection->campaign_candidates,
            min_text_ecpm,
            tag_size->max_text_creatives);
        }

        if(lost_auction)
        {
          // fill lost
          for(WeightedCampaignList::iterator lost_cmp_it =
                display_campaign_candidates.begin();
              lost_cmp_it != display_campaign_candidates.end();
              ++lost_cmp_it)
          {
            if(lost_cmp_it->get() && (
                 !result_display_campaign.get() ||
                 (*lost_cmp_it)->campaign !=
                   result_display_campaign->campaign))
            {
              lost_auction->ccgs.insert((*lost_cmp_it)->campaign);
              std::copy(
                (*lost_cmp_it)->available_creatives.begin(),
                (*lost_cmp_it)->available_creatives.end(),
                std::inserter(
                  lost_auction->creatives,
                  lost_auction->creatives.begin()));
            }
          }

          // push all candidates into lost, clear selected after
          text_selections.insert(text_selections.end(),
            text_selections_zero_ecpm.begin(),
            text_selections_zero_ecpm.end());

          for(TextSelectionBySizeList::iterator text_selection_it =
                text_selections.begin();
              text_selection_it != text_selections.end();
              ++text_selection_it)
          {
            for(WeightedCampaignKeywordList::iterator lost_cmp_it =
                  text_selection_it->campaign_candidates.begin();
                lost_cmp_it != text_selection_it->campaign_candidates.end();
                ++lost_cmp_it)
            {
              lost_auction->ccgs.insert(lost_cmp_it->campaign);
            }
          }

          if(result_text_campaigns.get())
          {
            for(WeightedCampaignKeywordList::const_iterator res_cmp_it =
                  result_text_campaigns->begin();
                res_cmp_it != result_text_campaigns->end();
                ++res_cmp_it)
            {
              lost_auction->ccgs.erase(res_cmp_it->campaign);
            }
          }

          for(ConstCampaignPtrSet::const_iterator lost_ccg_it =
                lost_auction->ccgs.begin();
              lost_ccg_it != lost_auction->ccgs.end(); ++lost_ccg_it)
          {
            CampaignIndex::ConstCreativePtrList available_creatives;

            for(TextSelectionBySizeList::iterator text_selection_it =
                  text_selections.begin();
                text_selection_it != text_selections.end();
                ++text_selection_it)
            {
              campaign_selection_index_->filter_creatives(
                key,
                tag,
                &text_selection_it->tag_sizes,
                *lost_ccg_it,
                request_params.profiling_available,
                request_params.full_freq_caps,
                SeqOrderMap(),
                available_creatives,
                true, // check click categories
                request_params.up_expand_space,
                request_params.right_expand_space,
                request_params.down_expand_space,
                request_params.left_expand_space,
                request_params.video_min_duration,
                request_params.video_max_duration,
                request_params.video_skippable_max_duration,
                request_params.video_allow_skippable,
                request_params.video_allow_unskippable,
                request_params.allowed_durations,
                request_params.exclude_categories,
                request_params.required_categories,
                request_params.secure,
                request_params.filter_empty_destination,
                0);
            }

            for(CampaignIndex::ConstCreativePtrList::const_iterator cr_it =
                  available_creatives.begin();
                cr_it != available_creatives.end(); ++cr_it)
            {
              lost_auction->creatives.insert(*cr_it);
            }
          }

          // collect lost for margin filtered campaigns
          collect_lost_(
            *lost_auction,
            key,
            request_params,
            tag,
            matched_channels,
            lost_campaigns.begin(),
            lost_campaigns.end());

          // collect lost for non selected campaigns with ecpm < selected ecpm
          collect_lost_(
            *lost_auction,
            key,
            request_params,
            tag,
            matched_channels,
            get_all_display_lost_campaigns.begin(),
            get_all_display_lost_campaigns.end());
        } // if(lost_auction)
      }
      else
      {
        select_result.walled_garden = true;
      }
    }

    void
    CampaignSelector::select_campaigns_max_ecpm_(
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
      noexcept
    {
      // select display campaign (WG)
      CampaignIndex::CampaignSelectionCellPtrList wg_display_check_campaigns;
      CampaignIndex::CampaignSelectionCellPtrList display_check_campaigns;
      CampaignIndex::CampaignCellPtrList keyword_check_campaigns;
      CampaignIndex::CampaignCellPtrList text_check_campaigns;
      CampaignIndex::CampaignCellPtrList lost_wg_campaigns;
      CampaignIndex::CampaignCellPtrList lost_campaigns;
      LostAuction* lost_auction = (collect_lost ? &select_result.lost_auction : 0);

      campaign_selection_index_->get_campaigns(
        key,
        wg_display_check_campaigns,
        display_check_campaigns,
        text_check_campaigns,
        keyword_check_campaigns,
        collect_lost ? &lost_wg_campaigns : 0,
        collect_lost ? &lost_campaigns : 0);

      weighted_campaign = select_display_campaign_(
        lost_auction,
        AT_MAX_ECPM,
        key,
        request_params,
        tag,
        ctr_calculation,
        conv_rate_calculation,
        channels,
        wg_display_check_campaigns,
        lost_wg_campaigns);

      // select display campaign (OIX)
      if(!weighted_campaign.get() &&
         tag->marketplace != 'W' // tag allow only WG auction
         )
      {

        weighted_campaign = select_display_campaign_(
          lost_auction,
          AT_MAX_ECPM,
          key,
          request_params,
          tag,
          ctr_calculation,
          conv_rate_calculation,
          channels,
          display_check_campaigns,
          lost_campaigns);

        if(collect_lost && weighted_campaign.get())
        {
          // remove selected campaign from lost campaigns,
          //   it can be present if lost WG auction
          select_result.lost_auction.ccgs.erase(weighted_campaign->campaign);
        }

        const Tag::TagPricing* tag_pricing = tag->select_tag_pricing(
          request_params.country_code.c_str(),
          CT_TEXT,
          CR_ALL
          // tag pricing for text campaigns must have
          // 'all' ccg rates targeting
          );

        if(!request_params.only_display_ad)
        {
          select_result.min_text_ecpm = std::max(
            tag_pricing->cpm, request_params.min_ecpm);

          if (weighted_campaign.get())
          {
            select_result.min_text_ecpm = std::max(
              select_result.min_text_ecpm, weighted_campaign->ecpm);
          }

          TextSelectionBySizeList text_selections;
          RevenueDecimal max_sum_ecpm = RevenueDecimal::ZERO;

          WeightedCampaignKeywordList text_campaign_candidates;

          get_text_campaign_candidates_(
            text_campaign_candidates,
            key,
            request_params,
            tag,
            0, // ctr_calculation
            hit_keywords,
            channels,
            keyword_check_campaigns,
            text_check_campaigns);

          for(Tag::SizeMap::const_iterator tag_size_it = tag_sizes.begin();
              tag_size_it != tag_sizes.end(); ++tag_size_it)
          {
            if(tag_size_it->second->max_text_creatives != 0)
            {
              CTRProvider::CalculationContext_var ctr_calculation_context;

              if(ctr_calculation)
              {
                ctr_calculation_context = ctr_calculation->create_context(
                  tag_size_it->second);
              }

              CTRProvider::CalculationContext_var conv_rate_calculation_context;
              if(conv_rate_calculation)
              {
                conv_rate_calculation_context = conv_rate_calculation->create_context(
                  tag_size_it->second);
              }

              WeightedCampaignKeywordList filtered_text_campaign_candidates;

              filter_text_campaign_candidates_(
                filtered_text_campaign_candidates,
                text_campaign_candidates,
                key,
                request_params,
                tag,
                tag_size_it->second,
                ctr_calculation_context,
                conv_rate_calculation_context);

              /*
              std::cerr << "filtered_text_campaign_candidates(" << tag_size_it->first <<
                ") :" << std::endl;
              for(WeightedCampaignKeywordList::const_iterator rit =
                    filtered_text_campaign_candidates.begin();
                  rit != filtered_text_campaign_candidates.end(); ++rit)
              {
                std::cerr << "[ campaign_id = " << rit->campaign->campaign_id <<
                  ", ecpm = " << rit->ecpm <<
                  ", ccid = " << rit->creative->ccid << "]" << std::endl;
              }
              */

              // order text campaigns by cpc with using text_campaign_candidates
              CPCKeywordMap cpc_keyword_map;

              convert_text_candidates_to_cpc_map_(
                cpc_keyword_map,
                filtered_text_campaign_candidates);

              /*
              std::cerr << "cpc_keyword_map(" << tag_size_it->first <<
                ") :" << std::endl;
              for(CPCKeywordMap::const_iterator rit =
                    cpc_keyword_map.begin();
                  rit != cpc_keyword_map.end(); ++rit)
              {
                std::cerr << "[ " << rit->first << ": (";

                for(WeightedCampaignKeywordList::const_iterator cit =
                      rit->second.begin();
                    cit != rit->second.end(); ++cit)
                {
                  std::cerr << "[ campaign_id = " << cit->campaign->campaign_id <<
                    ", ecpm = " << cit->ecpm <<
                    ", ccid = " << cit->creative->ccid << "]" << std::endl;
                }
                std::cerr << ")"<< std::endl;
              }
              */

              WeightedCampaignKeywordList step_weighted_campaign_keywords;

              if(select_campaign_keywords_n_(
                   collect_lost ? &select_result.lost_auction : 0,
                   key,
                   request_params,
                   tag,
                   tag_size_it->second,
                   ctr_calculation_context,
                   cpc_keyword_map,
                   select_result.min_text_ecpm,
                   tag_size_it->second->max_text_creatives,
                   step_weighted_campaign_keywords))
              {
                RevenueDecimal sum_step_ecpm = RevenueDecimal::ZERO;
                for(WeightedCampaignKeywordList::const_iterator wit =
                      step_weighted_campaign_keywords.begin();
                    wit != step_weighted_campaign_keywords.end(); ++wit)
                {
                  sum_step_ecpm += wit->actual_ecpm;
                }

                if(collect_lost)
                {
                  for(WeightedCampaignKeywordList::const_iterator wit =
                        step_weighted_campaign_keywords.begin();
                      wit != step_weighted_campaign_keywords.end(); ++wit)
                  {
                    select_result.lost_auction.ccgs.insert(wit->campaign);
                    select_result.lost_auction.creatives.insert(wit->creative);
                  }
                }

                /*
                std::cerr << "text_selection step ecpm(" <<
                  tag_size_it->first << ") = " << sum_step_ecpm << std::endl;
                */
                if(sum_step_ecpm >= max_sum_ecpm)
                {
                  if(sum_step_ecpm > max_sum_ecpm)
                  {
                    text_selections.clear();
                  }

                  text_selections.push_back(TextSelectionBySize());
                  TextSelectionBySize& text_selection = text_selections.back();
                  text_selection.tag_sizes.insert(
                    std::make_pair(tag_size_it->second->size->size_id, tag_size_it->second));
                  text_selection.campaign_candidates.swap(step_weighted_campaign_keywords);
                  max_sum_ecpm = sum_step_ecpm;
                }
              }
            }
          }

          if(!text_selections.empty())
          {
            // select randomly text selection cell
            const unsigned long text_selections_size = text_selections.size();
            TextSelectionBySizeList::iterator ts_it = text_selections.begin();

            if(text_selections_size > 1)
            {
              const unsigned long pos =
                Generics::safe_rand() % text_selections_size;
              std::advance(ts_it, pos);
            }

            assert(ts_it->tag_sizes.size() == 1);
            select_result.tag_size = ts_it->tag_sizes.begin()->second;
            result_weighted_campaign_keywords.reset(
              new WeightedCampaignKeywordList());
            result_weighted_campaign_keywords->swap(
              ts_it->campaign_candidates);

            assert(select_result.tag_size);

            if(collect_lost)
            {
              // remove from lost ccgs, creatives selected cells
              for(WeightedCampaignKeywordList::const_iterator wit =
                    result_weighted_campaign_keywords->begin();
                  wit != result_weighted_campaign_keywords->end(); ++wit)
              {
                select_result.lost_auction.ccgs.erase(wit->campaign);
                select_result.lost_auction.creatives.erase(wit->creative);
              }

              if(weighted_campaign.get())
              {
                // text ads win: push selected display campaign to lost auctions
                select_result.lost_auction.ccgs.insert(weighted_campaign->campaign);
                std::copy(weighted_campaign->available_creatives.begin(),
                  weighted_campaign->available_creatives.end(),
                  std::inserter(select_result.lost_auction.creatives,
                    select_result.lost_auction.creatives.begin()));
              }
            }
          }
        } // !request_params.only_display_ad
      }
      else
      {
        select_result.walled_garden = true;
      }

      if(lost_auction)
      {
        collect_lost_(
          select_result.lost_auction,
          key,
          request_params,
          tag,
          channels,
          lost_campaigns.begin(),
          lost_campaigns.end());
      }
    }

    void
    CampaignSelector::select_campaigns(
      AuctionType auction_type,
      AuctionType second_auction_type,
      const CampaignSelectParams* request_params_ptr,
      const ChannelIdHashSet& channels,
      const CampaignKeywordMap& hit_keywords,
      bool collect_lost,
      WeightedCampaignKeywordListPtr& result_weighted_campaign_keywords,
      WeightedCampaignPtr& weighted_campaign,
      AdSelectionResult& select_result)
    {
      const CampaignSelectParams& request_params = *request_params_ptr;

      const Tag* tag = request_params.tag;

      if(tag->tag_pricings.empty() ||
         tag->sizes.empty() ||
         request_params.colocation == 0 || // non found colo equal to none
         request_params.colocation->ad_serving == CampaignSvcs::CS_NONE)
      {
        return;
      }

      // create ctr calculation context
      CTRProvider::Calculation_var ctr_calculation;
      CTRProvider::Calculation_var conv_rate_calculation;

      if(ctr_provider_.in())
      {
        ctr_calculation = ctr_provider_->create_calculation(request_params_ptr);
        select_result.ctr_calculation = ctr_calculation;
      }

      if(conv_rate_provider_.in())
      {
        conv_rate_calculation = conv_rate_provider_->create_calculation(
          request_params_ptr);
        select_result.conv_rate_calculation = conv_rate_calculation;
      }

      select_result.tag = tag;
      select_result.walled_garden = false;

      CampaignIndex::Key key(tag);

      key.country_code = request_params.country_code;
      key.format = request_params.format;

      // concrete user status checking colocation traits independent,
      // because ccg us targeting if defined override colocation us targeting
      // reduce user statuses for targeting
      // for US_FOREIGN, US_BLACKLISTED can't be shown ad - don't check it
      key.user_status =
        (request_params.user_status == CampaignSvcs::US_OPTIN ||
          request_params.user_status == CampaignSvcs::US_OPTOUT ?
          request_params.user_status :
          (request_params.user_status == US_EXTERNALPROBE ||
           request_params.user_status == US_NOEXTERNALID ?
            CampaignSvcs::US_OPTOUT : CampaignSvcs::US_UNDEFINED));
      // check cells for all user statuses only colocation allow this
      // status targeting
      key.none_user_status =
        request_params.colocation->ad_serving == CS_ALL ||
        // CS_NONE blocked at frontend side (optimization), but check it
        (key.user_status == CampaignSvcs::US_OPTIN &&
          request_params.colocation->ad_serving != CS_NONE) ||
        (key.user_status != CampaignSvcs::US_OPTOUT &&
          request_params.colocation->ad_serving == CampaignSvcs::CS_NON_OPTOUT);
      key.test_request = request_params.test_request;
      key.tag_delivery_factor = request_params.tag_delivery_factor;
      key.ccg_delivery_factor = request_params.ccg_delivery_factor;

      if(auction_type == AT_RANDOM)
      {
        select_campaigns_randomly_(
          result_weighted_campaign_keywords,
          weighted_campaign,
          select_result,
          key,
          request_params,
          tag,
          ctr_calculation,
          conv_rate_calculation,
          request_params.tag_sizes,
          channels,
          hit_keywords);

        if((result_weighted_campaign_keywords.get() == 0 ||
            result_weighted_campaign_keywords->empty()) &&
           weighted_campaign.get() == 0)
        {
          // try regular action, if random is failed
          result_weighted_campaign_keywords.reset(0);
          select_result.walled_garden = false;
          auction_type = second_auction_type;
        }
        else
        {
          select_result.ctr_calculation = CTRProvider::Calculation_var();
        }
      }

      if(auction_type == AT_MAX_ECPM)
      {
        select_campaigns_max_ecpm_(
          result_weighted_campaign_keywords,
          weighted_campaign,
          select_result,
          key,
          request_params,
          tag,
          request_params.tag_sizes,
          ctr_calculation,
          conv_rate_calculation,
          channels,
          hit_keywords,
          collect_lost);
      }
      else if(auction_type == AT_PROPORTIONAL_PROBABILITY)
      {
        select_campaigns_prop_probability_(
          result_weighted_campaign_keywords,
          weighted_campaign,
          select_result,
          key,
          request_params,
          tag,
          ctr_calculation,
          conv_rate_calculation,
          request_params.tag_sizes,
          channels,
          hit_keywords,
          collect_lost);
      }

      /* calculate cpm_threshold: first check display selection,
       * its cost can be only less then text campaigns sum
       */
      if(result_weighted_campaign_keywords.get())
      {
        RevenueDecimal sum_text_ecpm = RevenueDecimal::ZERO;

        for(WeightedCampaignKeywordList::iterator kw_it =
              result_weighted_campaign_keywords->begin();
            kw_it != result_weighted_campaign_keywords->end(); ++kw_it)
        {
          sum_text_ecpm += kw_it->ecpm; // non truncated ecpm
        }

        select_result.cpm_threshold = sum_text_ecpm;
      }
      else if(weighted_campaign.get())
      {
        select_result.cpm_threshold = weighted_campaign->ecpm;
      }

      const Tag::TagPricing* tag_pricing = tag->select_country_tag_pricing(
        request_params.country_code.c_str());

      select_result.auction_type = auction_type;
      // calculate only for DebugInfo
      select_result.min_no_adv_ecpm = std::max(
        tag_pricing->cpm, request_params.min_ecpm);

      if(result_weighted_campaign_keywords.get())
      {
        weighted_campaign.reset(0);
        select_result.walled_garden = false;
      }

      if(weighted_campaign.get())
      {
        select_result.tag_size = weighted_campaign->tag_size;
      }

      if(ctr_calculation && select_result.tag_size)
      {
        CTRProvider::CalculationContext_var ctr_calculation_context =
          ctr_calculation->create_context(select_result.tag_size);

        if(weighted_campaign.get())
        {
          if(!weighted_campaign->campaign->use_ctr() &&
            weighted_campaign->campaign->bid_strategy != BS_MIN_CTR_GOAL)
          {
            weighted_campaign->ctr = ctr_calculation_context->get_ctr(
              weighted_campaign->creative);
          }
        }
        else if(result_weighted_campaign_keywords.get())
        {
          for(auto it = result_weighted_campaign_keywords->begin();
            it != result_weighted_campaign_keywords->end(); ++it)
          {
            if(!it->campaign->use_ctr() &&
              it->campaign->bid_strategy != BS_MIN_CTR_GOAL)
            {
              it->ctr = ctr_calculation_context->get_ctr(it->creative);
            }
          }
        }
      } // if(ctr_calculation)
    }
  }
}

