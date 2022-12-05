#ifndef _CAMPAIGN_INDEX_HPP_
#define _CAMPAIGN_INDEX_HPP_

#include <eh/Exception.hpp>
#include <Generics/CRC.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>

#include <Commons/Containers.hpp>

#include "CampaignManagerDeclarations.hpp"
#include "CampaignConfig.hpp"
#include "SequencePacker.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    struct IndexingProgress
    {
      typedef Sync::Policy::PosixThread SyncPolicy;

      IndexingProgress()
        : loaded_campaign_count(0),
          common_campaign_count(0)
      {}

      SyncPolicy::Mutex lock;

      unsigned long loaded_campaign_count;
      unsigned long common_campaign_count;
    };

    typedef std::set<unsigned long> CCGIdSet;

    struct CampaignSelectionCell:
      public virtual ReferenceCounting::DefaultImpl<>
    {
      CampaignSelectionCell();

      CampaignSelectionCell(
        const Campaign* campaign_val,
        const RevenueDecimal& adjusted_campaign_ecpm,
        const Tag::TagPricing* tag_pricing_val);

      bool operator==(const CampaignSelectionCell& right) const;

      unsigned long hash() const;

      ConstCampaign_var campaign;
      const Tag::TagPricing* tag_pricing;
      // ecpm profit (adjusted): can be negative at negative margins,
      // used for determine cell position (selection priority)
      RevenueDecimal ecpm;

    protected:
      virtual
      ~CampaignSelectionCell() noexcept;
    };

    typedef ReferenceCounting::SmartPtr<
      CampaignSelectionCell, ReferenceCounting::PolicyAssert>
      CampaignSelectionCell_var;

    typedef ReferenceCounting::SmartPtr<
      const CampaignSelectionCell, ReferenceCounting::PolicyAssert>
      ConstCampaignSelectionCell_var;

    bool
    campaign_selection_cell_less_pred(
      const CampaignSelectionCell* left,
      const CampaignSelectionCell* right) noexcept;

    // CampaignCell
    //   campaign holder
    //   TODO: try to remove it, it required only as hash provider for SequencePacker (
    //     can be replaced by external function)
    //   operator== can operate with campaign pointers (no few campaigns with equal ccg_id)
    class CampaignCell: public ReferenceCounting::DefaultImpl<>
    {
    public:
      CampaignCell();

      CampaignCell(const Campaign* campaign_val);

      bool operator==(const CampaignCell& right) const;

      unsigned long hash() const;

      ConstCampaign_var campaign;

    protected:
      virtual
      ~CampaignCell() noexcept;
    };

    typedef ReferenceCounting::SmartPtr<
      CampaignCell, ReferenceCounting::PolicyAssert>
      CampaignCell_var;

    typedef ReferenceCounting::SmartPtr<
      const CampaignCell, ReferenceCounting::PolicyAssert>
      ConstCampaignCell_var;

    bool
    campaign_cell_less_pred(
      const CampaignCell* left,
      const CampaignCell* right) noexcept;

    // CampaignSelectionCellList
    //   list of CampaignSelectionCell's ordered by ecpm profit decrease
    class CampaignSelectionCellList:
      public std::list<ConstCampaignSelectionCell_var>,
      public ElementSeqBase,
      public ReferenceCounting::DefaultImpl<>
    {
    public:
      CampaignSelectionCellList() noexcept;
      CampaignSelectionCellList(const CampaignSelectionCellList& other)
        /*throw(eh::Exception)*/;

      void insert(const CampaignSelectionCell* ins);

    protected:
      virtual
      ~CampaignSelectionCellList() noexcept;
    };

    typedef ReferenceCounting::SmartPtr<
      const CampaignSelectionCellList, ReferenceCounting::PolicyAssert>
      CampaignSelectionCellList_var;

    // CampaignCellList
    //   list of campaigns ordered by ccg_id (campaign_cell_less_pred)
    class CampaignCellList:
      public std::list<ConstCampaignCell_var>,
      public ElementSeqBase,
      public ReferenceCounting::DefaultImpl<>
    {
    public:
      CampaignCellList() noexcept;
      CampaignCellList(const CampaignCellList& other) /*throw(eh::Exception)*/;

      void insert(const CampaignCell* ins);

    protected:
      virtual
      ~CampaignCellList() noexcept;
    };

    typedef ReferenceCounting::SmartPtr<
      const CampaignCellList, ReferenceCounting::PolicyAssert>
      CampaignCellList_var;

    // sequences pack holders
    typedef SequencePacker<CampaignSelectionCell, CampaignSelectionCellList>
      CampaignSelectionCellListHolder;

    typedef ReferenceCounting::SmartPtr<
      CampaignSelectionCellListHolder, ReferenceCounting::PolicyAssert>
      CampaignSelectionCellListHolder_var;

    typedef SequencePacker<CampaignCell, CampaignCellList>
      CampaignCellListHolder;

    typedef ReferenceCounting::SmartPtr<
      CampaignCellListHolder, ReferenceCounting::PolicyAssert>
      CampaignCellListHolder_var;

    /**
     * CampaignSelectionIndex
     * implement next campaign filters:
     *   get_campaigns:
     *     1. opt-out
     *     2. real, test-real branches
     *     3. include specific sites
     *     4. available for tag creatives
     *     5. countries
     *     6. available for format creatives
     *     7. exclusion for site by creative categories
     *
     *   check_campaign:
     *     1. exclude colocations
     *     2. campaign & ccg freq caps
     *     3. weekly run intervals
     *
     *   filter_creatives:
     *     1. creative freq caps
     *     2. click url match
     *     + it repeat creative filters from get_campaigns
     *
     * It return list of weighted campaigns ordered by cost decreasing
     *
     */
    class CampaignIndex:
      public Generics::Last<ReferenceCounting::AtomicImpl>
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      enum StatusType
      {
        ST_REAL = 0,
        ST_TEST_REAL = 4,
      };

      typedef std::deque<const CampaignSelectionCell*>
        CampaignSelectionCellPtrList;
      typedef std::deque<const CampaignCell*> CampaignCellPtrList;
      typedef std::list<const Creative*> ConstCreativePtrList;

    public:
      CampaignIndex(
        const CampaignConfig* campaign_config,
        Logging::Logger* logger)
        noexcept;

      CampaignIndex(
        const CampaignIndex& init,
        Logging::Logger* logger)
        noexcept;

      bool
      index_campaigns(
        IndexingProgress* indexing_progress = 0,
        Generics::ActiveObject* interrupter = 0)
        /*throw(eh::Exception)*/;

      ConstCampaignConfig_var
      configuration() const /*throw(eh::Exception)*/;

      struct Key
      {
        Key(const Tag* tag_val): tag(tag_val) {}

        const Tag* tag;
        std::string country_code;
        std::string format;
        //unsigned long colo_id;
        UserStatus user_status;
        bool none_user_status;
        bool test_request;
        unsigned long tag_delivery_factor;
        unsigned long ccg_delivery_factor;
      };

      struct TraceParams
      {
        TraceParams(
          const Key& key_val,
          const Campaign* cmp,
          const Tag* tag_val,
          AuctionType auction_type_val,
          std::ostream& trace_stream_val)
          : key(key_val),
            campaign(cmp),
            tag(tag_val),
            auction_type(auction_type_val),
            trace_stream(trace_stream_val)
        {}

        Key key;
        const Campaign* campaign;
        const Tag* tag;
        AuctionType auction_type;
        std::ostream& trace_stream;
      };

      unsigned long
      size() const noexcept;

      static bool
      match_domain(
        const std::string& domain,
        const String::SubString& referer)
        noexcept;

      static bool
      check_tag_domain_exclusion(
        const String::SubString& url,
        const Tag* tag)
        noexcept;

      static bool
      check_tag_visibility(
        long tag_visibility,
        const Tag* tag,
        TraceParams* trace_params = 0);

      static bool
      check_site_freq_cap(
        bool profiling_available,
        const FreqCapIdSet& full_freq_caps,
        const Tag* tag,
        TraceParams* trace_params = 0);

      void
      index_campaign(
        const Campaign* campaign,
        TraceParams* trace_params = 0)
        /*throw(Exception, eh::Exception)*/;

      /* get_campaigns
       * choose campaigns that can be shown
       * wg_campaign_cell_list, campaign_cell_list :
       *   lists of candidates ordered by ecpm profit decrease
       * text_campaign_cell_list, keyword_campaign_cell_list:
       *   lists of text candidates ordered by ccg_id
       * lost_wg_campaign_cell_list, lost_campaign_cell_list:
       *   lists of lost candidates ordered by ccg_id
       */
      void
      get_campaigns(
        const Key& index,
        CampaignSelectionCellPtrList& wg_campaign_cell_list,
        CampaignSelectionCellPtrList& campaign_cell_list,
        CampaignCellPtrList& text_campaign_cell_list,
        CampaignCellPtrList& keyword_campaign_cell_list,
        CampaignCellPtrList* lost_wg_campaign_cell_list,
        CampaignCellPtrList* lost_campaign_cell_list)
        const;

      void
      get_random_campaigns(
        const Key& index,
        CampaignSelectionCellPtrList& wg_display_campaign_cell_list,
        CampaignSelectionCellPtrList& display_campaign_cell_list,
        CampaignCellPtrList& text_campaign_cell_list,
        CampaignCellPtrList& keyword_campaign_cell_list)
        const;

      bool
      check_campaign(
        const Key& key,
        const Campaign* campaign,
        const Generics::Time& current_time,
        bool profiling_available,
        const FreqCapIdSet& full_freq_caps,
        unsigned long colo_id,
        const Generics::Time& user_create_time,
        const AdServer::Commons::UserId& user_id_hash,
        const TraceParams* trace_params)
        const;

      bool
      check_campaign_channel(
        const Campaign* campaign,
        const ChannelIdHashSet& matched_channels)
        const;

      void
      filter_creatives(
        const Key& key,
        const Tag* tag,
        const Tag::SizeMap* tag_sizes,
        const Campaign* campaign,
        bool profiling_available,
        const FreqCapIdSet& full_freq_caps,
        const SeqOrderMap& seq_orders,
        ConstCreativePtrList& creatives,
        bool check_click_categories,
        unsigned long up_expand_space,
        unsigned long right_expand_space,
        unsigned long down_expand_space,
        unsigned long left_expand_space,
        unsigned long video_min_duration,
        const AdServer::Commons::Optional<unsigned long>& video_max_duration,
        const AdServer::Commons::Optional<unsigned long>& video_skippable_max_duration,
        bool video_allow_skippable,
        bool video_allow_unskippable,
        const AllowedDurationSet& allowed_durations,
        const CreativeCategoryIdSet& exclude_categories,
        const CreativeCategoryIdSet& required_categories,
        bool secure,
        bool filter_empty_destination,
        TraceParams* trace_params)
        const;

      static bool
      creative_available_by_size(
        const Tag* tag,
        const Tag::Size* tag_size,
        const Creative* creative,
        unsigned long up_expand_space,
        unsigned long right_expand_space,
        unsigned long down_expand_space,
        unsigned long left_expand_space)
        noexcept;

      void
      trace_tree(std::ostream& ostr) noexcept;

      void
      trace_indexing(
        const Key& key,
        const Generics::Time& current_time,
        bool profiling_available,
        const FreqCapIdSet& full_freq_caps,
        unsigned long colo_id,
        const Generics::Time& user_create_time,
        const ChannelIdHashSet& matched_channels,
        const AdServer::Commons::UserId& user_id_hash,
        const Campaign* campaign,
        const RevenueDecimal& min_ecpm,
        unsigned long up_expand_space,
        unsigned long right_expand_space,
        unsigned long down_expand_space,
        unsigned long left_expand_space,
        long tag_visibility,
        unsigned long video_min_duration,
        const AdServer::Commons::Optional<unsigned long>& video_max_duration,
        const AdServer::Commons::Optional<unsigned long>& video_skippable_max_duration,
        bool video_allow_skippable,
        bool video_allow_unskippable,
        const AllowedDurationSet& allowed_durations,
        const CreativeCategoryIdSet& exclude_categories,
        const CreativeCategoryIdSet& required_categories,
        AuctionType auction_type,
        bool secure,
        bool filter_empty_destination,
        std::ostream& ostr)
        /*throw(Exception, eh::Exception)*/;

    protected:
      virtual
      ~CampaignIndex() noexcept = default;

    private:
      typedef ReferenceCounting::SmartPtr<const Campaign> ConstCampaign_var;
      typedef ReferenceCounting::SmartPtr<const Creative> ConstCreative_var;

      /** tag filter contains:
       *     site filter,
       *     size filter - available creative sizes,
       *     account filter,
       *     tag pricing filter,
       *     site creative exclusion & creative category exclusion
       */
      struct IndexNode
      {
        CampaignSelectionCellList_var wg_display_campaigns;
        CampaignSelectionCellList_var display_campaigns;
        CampaignCellList_var text_campaigns;
        CampaignCellList_var keyword_campaigns;

        CampaignSelectionCellList_var wg_display_random_campaigns;
        CampaignSelectionCellList_var display_random_campaigns;
        CampaignCellList_var text_random_campaigns;
        CampaignCellList_var keyword_random_campaigns;

        CampaignCellList_var lost_wg_campaigns;
        CampaignCellList_var lost_campaigns;
      };

      typedef std::list<const IndexNode*> IndexNodeList;

      struct KeyHashAdapter
      {
        KeyHashAdapter(
          unsigned char match_status_type_val,
          unsigned long tag_id_val,
          const char* country_code_val,
          const char* app_format_val)
          noexcept;

        bool operator==(const KeyHashAdapter& right) const noexcept;

        unsigned long hash() const noexcept;

        const unsigned char match_status_type;
        const unsigned long tag_id;
        char country_code[2];
        const std::string app_format;

      private:
        unsigned long hash_;
      };

      typedef Generics::GnuHashTable<
        KeyHashAdapter, IndexNode> OrderedCampaignMap;

      struct TagCampaignApprove
      {
        // campaign_id is first order : used in indexing
        TagCampaignApprove(
          unsigned long campaign_id_val,
          const Tag* tag_val)
          noexcept;

        bool
        operator<(const TagCampaignApprove& right) const;

        const unsigned long campaign_id;
        const Tag* tag;
      };

      typedef std::map<TagCampaignApprove, ConstCreativePtrList>
        TagCampaignApproveMap;

      typedef std::map<unsigned long, Colocation_var>
        ColocationMap;

      typedef Sync::Policy::PosixThread SyncPolicy;

    private:
      /* campaign indexing help methods */
      void
      preindex_for_tag_(
        const Tag* tag,
        const Campaign* campaign,
        TraceParams* trace_params)
        /*throw(Exception, eh::Exception)*/;

      void
      index_for_status_(
        const Campaign* campaign,
        UserStatus user_status,
        TraceParams* trace_params)
        /*throw(Exception, eh::Exception)*/;

      static bool
      colo_precheck_(
        UserStatus user_status,
        const Colocation* colocation)
        noexcept;

      void
      index_for_colo_(
        UserStatus user_status,
        StatusType status_type,
        const Campaign* campaign,
        TraceParams* trace_params)
        /*throw(Exception, eh::Exception)*/;

      void
      index_for_tags_(
        UserStatus user_status,
        StatusType status_type,
        const Campaign* campaign,
        TraceParams* trace_params)
        /*throw(Exception, eh::Exception)*/;

      void
      index_for_tag_i_(
        UserStatus user_status,
        StatusType status_type,
        const Campaign* campaign,
        const Tag* tag,
        const ConstCreativePtrList& creatives,
        TraceParams* trace_params)
        /*throw(Exception, eh::Exception)*/;

      void
      index_for_countries_(
        UserStatus user_status,
        StatusType status_type,
        const Campaign* campaign,
        const Tag* tag,
        const ConstCreativePtrList& creatives,
        TraceParams* trace_params)
        /*throw(Exception, eh::Exception)*/;

      bool
      margin_check_(
        const RevenueDecimal& adjusted_campaign_ecpm,
        const Tag::TagPricing* tag_pricing,
        const Campaign* campaign,
        bool walled_garden,
        TraceParams* trace_params)
        /*throw(Exception, eh::Exception)*/;

      void
      index_for_appformat_(
        UserStatus user_status,
        StatusType status_type,
        const Campaign* campaign,
        const Tag* tag,
        const Tag::TagPricing* tag_pricing,
        const char* country_code,
        const ConstCreativePtrList& creatives,
        TraceParams* trace_params)
        /*throw(Exception, eh::Exception)*/;

      /* campaign search help methods */
      static bool
      creative_available_by_exclude_categories_(
        const Creative* creative,
        const CreativeCategoryIdSet& exclude_categories)
        noexcept;

      static bool
      creative_available_by_required_categories_(
        const Creative* creative,
        const CreativeCategoryIdSet& required_categories)
        noexcept;

      static bool
      check_categories_(
        const Tag* tag,
        bool site_approve_creative,
        bool tag_exclusion,
        bool site_exclusion,
        const Creative::CategorySet& creative_categories)
        noexcept;

      static bool
      creative_preavailable_by_sizes_(
        const Tag* tag,
        const Creative* creative)
        noexcept;

      static bool
      creative_available_by_sizes_(
        const Tag* tag,
        const Tag::SizeMap* tag_sizes,
        const Creative* creative,
        unsigned long up_expand_space,
        unsigned long right_expand_space,
        unsigned long down_expand_space,
        unsigned long left_expand_space)
        noexcept;

      static bool
      creative_available_by_expand_spaces_(
        const Tag* tag,
        const Creative* creative,
        unsigned long up_expand_space,
        unsigned long right_expand_space,
        unsigned long down_expand_space,
        unsigned long left_expand_space)
        noexcept;

      bool
      creative_available_by_categories_(
        const Campaign* campaign,
        const Creative* creative,
        const Tag* tag,
        bool check_click_url_categories = true)
        const
        noexcept;

      bool
      creative_available_by_templates_(
        const Campaign* campaign,
        const Creative* creative,
        const Tag* tag,
        const char* app_format)
        const
        noexcept;

      bool
      check_campaign_time_(
        const Campaign* campaign,
        const Generics::Time& current_time)
        const;

      void
      get_index_nodes_(
        IndexNodeList& result_nodes,
        const Key& request_params)
        const
        noexcept;

      template<
        typename ResultContainerType,
        typename ListFieldType,
        typename LessPredType>
      static void
      merge_lists_(
        ResultContainerType& result,
        const IndexNodeList& nodes,
        ListFieldType IndexNode::* list_field,
        const LessPredType& less_pred);

      static
      std::string
      decode_match_status_type_(
        unsigned char match_status_type) noexcept;

    private:
      mutable SyncPolicy::Mutex lock_;

      Logging::Logger_var logger_;

      ConstCampaignConfig_var campaign_config_;
      OrderedCampaignMap ordered_campaigns_;

      // indexing temporary helpers
      TagCampaignApproveMap tag_campaign_approve_;

      CampaignSelectionCellListHolder_var cell_holder_;
      CampaignCellListHolder_var campaign_cell_holder_;
    };

    typedef ReferenceCounting::SmartPtr<CampaignIndex>
      CampaignIndex_var;
  }
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    // CampaignSelectionCell
    inline
    CampaignSelectionCell::CampaignSelectionCell()
    {}

    inline
    CampaignSelectionCell::~CampaignSelectionCell() noexcept
    {}

    inline
    CampaignSelectionCell::CampaignSelectionCell(
      const Campaign* campaign_val,
      const RevenueDecimal& adjusted_campaign_ecpm,
      const Tag::TagPricing* tag_pricing_val)
      : campaign(ReferenceCounting::add_ref(campaign_val)),
        tag_pricing(tag_pricing_val)
    {
      // profit ecpm - specify selection order
      if(tag_pricing)
      {
        ecpm = adjusted_campaign_ecpm -
          tag_pricing->cpm -
          RevenueDecimal::mul(adjusted_campaign_ecpm,
            tag_pricing->revenue_share, Generics::DMR_FLOOR);
      }
      else
      {
        ecpm = adjusted_campaign_ecpm;
      }
    }

    inline
    bool
    CampaignSelectionCell::operator==(const CampaignSelectionCell& right) const
    {
      return campaign->campaign_id == right.campaign->campaign_id &&
        tag_pricing->site_rate_id == right.tag_pricing->site_rate_id;
    }

    inline
    unsigned long
    CampaignSelectionCell::hash() const
    {
      uint32_t res = 0;
      res = Generics::CRC::quick(
        res, &(campaign->campaign_id), sizeof(campaign->campaign_id));
      res = Generics::CRC::quick(
        res, &(tag_pricing->site_rate_id), sizeof(tag_pricing->site_rate_id));
      return res;
    }

    // campaign_selection_cell_less_pred()
    inline
    bool
    campaign_selection_cell_less_pred(
      const CampaignSelectionCell* left,
      const CampaignSelectionCell* right)
      noexcept
    {
      return left->ecpm > right->ecpm;
    }

    // CampaignSelectionCellList
    inline
    CampaignSelectionCellList::CampaignSelectionCellList() noexcept
    {}

    inline
    CampaignSelectionCellList::CampaignSelectionCellList(
      const CampaignSelectionCellList& other) /*throw(eh::Exception)*/
      : ReferenceCounting::Interface(),
        std::list<ConstCampaignSelectionCell_var>(other),
        ElementSeqBase(other), ReferenceCounting::DefaultImpl<>()
    {
    }

    inline
    CampaignSelectionCellList::~CampaignSelectionCellList() noexcept
    {
      unkeep_(this);
    }

    inline
    void
    CampaignSelectionCellList::insert(const CampaignSelectionCell* ins)
    {
      iterator ins_it = begin();

      while(ins_it != end())
      {
        if(campaign_selection_cell_less_pred(ins, *ins_it))
        {
          break;
        }

        ++ins_it;
      }

      std::list<ConstCampaignSelectionCell_var>::insert(
        ins_it, ReferenceCounting::add_ref(ins));
    }

    // CampaignCell
    inline
    CampaignCell::CampaignCell()
    {}

    inline
    CampaignCell::CampaignCell(
      const Campaign* campaign_val)
      : campaign(ReferenceCounting::add_ref(campaign_val))
    {}

    inline
    CampaignCell::~CampaignCell() noexcept
    {}

    inline
    bool
    CampaignCell::operator==(
      const CampaignCell& right) const
    {
      return campaign->campaign_id == right.campaign->campaign_id;
    }

    inline
    unsigned long
    CampaignCell::hash() const
    {
      return campaign->campaign_id;
    }

    // campaign_cell_less_pred()
    inline
    bool
    campaign_cell_less_pred(
      const CampaignCell* left,
      const CampaignCell* right) noexcept
    {
      return left->campaign->campaign_id < right->campaign->campaign_id;
    }

    // CampaignCellList
    inline
    CampaignCellList::CampaignCellList() noexcept
    {}

    inline
    CampaignCellList::CampaignCellList(const CampaignCellList& other)
      /*throw(eh::Exception)*/
      : ReferenceCounting::Interface(),
        std::list<ConstCampaignCell_var>(other), ElementSeqBase(other),
        ReferenceCounting::DefaultImpl<>()
    {
    }

    inline
    CampaignCellList::~CampaignCellList() noexcept
    {
      unkeep_(this);
    }

    inline
    void
    CampaignCellList::insert(const CampaignCell* ins)
    {
      iterator ins_it = begin();

      while(ins_it != end())
      {
        if(campaign_cell_less_pred(ins, *ins_it))
        {
          break;
        }

        ++ins_it;
      }

      std::list<ConstCampaignCell_var>::insert(
        ins_it, ReferenceCounting::add_ref(ins));
    }

    // CampaignIndex
    inline
    ConstCampaignConfig_var
    CampaignIndex::configuration() const /*throw(eh::Exception)*/
    {
      return campaign_config_;
    }

    inline
    unsigned long
    CampaignIndex::size() const noexcept
    {
      return ordered_campaigns_.size();
    }

    // CampaignIndex::KeyHashAdapter
    inline
    CampaignIndex::KeyHashAdapter::KeyHashAdapter(
      unsigned char match_status_type_val,
      unsigned long tag_id_val,
      const char* country_code_val,
      const char* app_format_val)
      noexcept
      : match_status_type(match_status_type_val),
        tag_id(tag_id_val),
        app_format(app_format_val)
    {
      country_code[0] = country_code_val[0];
      country_code[1] = country_code_val[0] ? country_code_val[1] : 0;
      hash_ = Generics::CRC::quick(0, &match_status_type, sizeof(match_status_type));
      hash_ = Generics::CRC::quick(hash_, &tag_id, sizeof(tag_id));
      hash_ = Generics::CRC::quick(hash_, country_code, 2);
      hash_ = Generics::CRC::quick(hash_, app_format.data(), app_format.size());
    }

    inline
    bool
    CampaignIndex::KeyHashAdapter::operator==(
      const KeyHashAdapter& right) const noexcept
    {
      return match_status_type == right.match_status_type &&
        tag_id == right.tag_id &&
        country_code[0] == right.country_code[0] &&
        country_code[1] == right.country_code[1] &&
        app_format == right.app_format;
    }

    inline
    unsigned long
    CampaignIndex::KeyHashAdapter::hash() const noexcept
    {
      return hash_;
    }

    // CampaignIndex::TagCampaignApprove
    inline
    CampaignIndex::TagCampaignApprove::TagCampaignApprove(
      unsigned long campaign_id_val,
      const Tag* tag_val)
      noexcept
      : campaign_id(campaign_id_val),
        tag(tag_val)
    {}

    inline
    bool
    CampaignIndex::TagCampaignApprove::operator<(
      const TagCampaignApprove& right) const
    {
      return campaign_id < right.campaign_id ||
        (campaign_id == right.campaign_id && tag < right.tag);
    }
  }
}

#endif /*_CAMPAIGN_INDEX_HPP_*/
