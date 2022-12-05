#ifndef AD_SERVER_CHANNEL_CHUNK
#define AD_SERVER_CHANNEL_CHUNK

#include <vector>
#include <map>
#include <iostream>
#include <eh/Exception.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Stream/MemoryStream.hpp>
#include <Commons/GranularContainer.hpp>
#include <ChannelSvcs/ChannelCommons/CommonTypes.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelCommons.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#include "ChannelServerTypes.hpp"
#include "ContainerMatchers.hpp"
#include "SoftMatcher.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  struct MatchingEntity: public PositiveContainerType
  {
    MatchingEntity() noexcept
      : matcher(SoftMatcher_var())
    {}

    MatchingEntity(SoftMatcher* data) noexcept
      : matcher(ReferenceCounting::add_ref(data))
    {}

    MatchingEntity(const MatchingEntity& init) noexcept
      : PositiveContainerType(init),
        matcher(init.matcher)
    {}
    
    MatchingEntity(MatchingEntity&& init) noexcept
    {
      this->swap(init);
    }

    void
    swap(MatchingEntity& entity) noexcept
    {
      matcher.swap(entity.matcher);
      PositiveContainerType::swap(entity);
    }

    MatchingEntity&
    operator=(const MatchingEntity& init)
    {
      PositiveContainerType::operator=(init);
      matcher = init.matcher;
      return *this;
    }

    SoftMatcher_var matcher;

  /*
  private:
    MatchingEntity(const MatchingEntity&);
  */
  };

  typedef CORBA::ULong MatchType;
  typedef std::vector<MatchingEntity> SoftVector;

  enum MatchingFlags
  {
    MF_NONE = 0,
    MF_NONSTRICTKW = 1,//non strict keyword matching
    MF_NONSTRICTURL = 2,//non strict url matching
    MF_ACTIVE = 4,//match active channels
    MF_INACTIVE = 8,//match inactive channels
    MF_NEGATIVE = 16,//return negative channel with trigger
    MF_BLACK_LIST = 32 //match black list channels and special
  };

  struct TriggerAtom:
    public SoftVector,
    public ReferenceCounting::AtomicImpl //first map item
  {
  private:
    // SoftVector ordered by (trigger, lang_index)
    virtual
    ~TriggerAtom() noexcept
    {
    }
  };

  typedef ReferenceCounting::SmartPtr<TriggerAtom> TriggerAtom_var;

  struct UidAtom:
    public IdVector,
    public ReferenceCounting::AtomicImpl //first map item
  {
  private:
    // IdVector ordered by id
    virtual
    ~UidAtom() noexcept
    {
    }
  };

  typedef ReferenceCounting::SmartPtr<UidAtom> UidAtom_var;

  const size_t STRING_ADAPTER_SIZE = sizeof(Generics::SubStringHashAdapter);
  const size_t POSITIVE_ATOM_SIZE = sizeof(MatchingEntity);
  const size_t TRIGGER_ATOM_SIZE = sizeof(TriggerAtom);
  const size_t SOFT_MATCHER_SIZE = sizeof(SoftMatcherType);

  struct MatchUrl
  {
    std::string prefix;
    std::string postfix;
  };

  typedef Generics::GnuHashTable<Generics::SubStringHashAdapter, TriggerAtom_var>
    TriggerMapType;

  typedef Generics::GnuHashTable<Generics::Uuid, UidAtom_var>
    UidMapType;

  class TriggerMap:
    public TriggerMapType,
    public ReferenceCounting::AtomicCopyImpl
  {
  protected:
    virtual
    ~TriggerMap() noexcept
    {
    }
  };

  class UidMap:
    public UidMapType,
    public ReferenceCounting::AtomicCopyImpl
  {
  protected:
    virtual
    ~UidMap() noexcept
    {
    }
  };


  typedef ReferenceCounting::SmartPtr<UidMap> UidMap_var;
  typedef ReferenceCounting::SmartPtr<TriggerMap> TriggerMap_var;
  typedef ReferenceCounting::SmartPtr<TriggerMap> UrlMap_var;
  typedef std::set<unsigned int> RemovedType;

  class CCGKeyword: public ReferenceCounting::AtomicImpl
  {
  protected:
    virtual
    ~CCGKeyword() noexcept
    {
    }

  public:

    bool operator==(const CCGKeyword& cp) const noexcept
    {
      return ccg_keyword_id == cp.ccg_keyword_id &&
        ccg_id == cp.ccg_id &&
        channel_id == cp.channel_id &&
        max_cpc == cp.max_cpc &&
        ctr == cp.ctr &&
        click_url == cp.click_url &&
        original_keyword == cp.original_keyword;
    }

    unsigned int ccg_keyword_id;
    unsigned int ccg_id;
    unsigned int channel_id;
    CampaignSvcs::RevenueDecimal max_cpc;
    CampaignSvcs::CTRDecimal ctr;
    std::string click_url;
    std::string original_keyword;
    Generics::Time timestamp;
  };

  typedef ReferenceCounting::SmartPtr<CCGKeyword> CCGKeyword_var;

  typedef AdServer::Commons::NoCopyGranularContainer<
    unsigned int, CCGKeyword_var> CCGType;

  class CCGMap:
    public CCGType,
    public ReferenceCounting::AtomicImpl
  {
  protected:
    virtual
    ~CCGMap() noexcept
    {
    }
  };

  typedef ReferenceCounting::SmartPtr<CCGMap> CCGMap_var;

  struct Channel
  {
    typedef unsigned int ChannelIdType;

    enum
    {
      CT_UIDS = CT_MAX,
      CT_BLACK_LIST,
      CT_WEIGHT,
      CT_ACTIVE,
      CT_INACTIVE,
      CT_WAIT
    };

    enum
    {
      CH_URL = 1<<CT_URL,//1
      CH_PAGE = 1<<CT_PAGE,//2
      CH_SEARCH = 1<<CT_SEARCH,//4
      CH_URL_KEYWORDS = 1<<CT_URL_KEYWORDS,//8
      CH_UIDS = 1 <<CT_UIDS, // 16
      CH_BLACK_LIST = 1 <<CT_BLACK_LIST, //32
      CH_WEIGHT = 1<<CT_WEIGHT,//64
      CH_ACTIVE = 1<<CT_ACTIVE,//128
      CH_INACTIVE = 1<<CT_INACTIVE,//256
      CH_WAIT = 1<<CT_WAIT//512
    };

    Channel(ChannelIdType id_ = 0) noexcept;

    bool operator!=(ChannelIdType) const;

    ChannelIdType get_id() const noexcept;
    void mark_type(unsigned int type) noexcept;
    void set_status(char status) noexcept;
    void set_weight(MatchType type, unsigned int weight) noexcept;
    bool match(unsigned int type) const noexcept;
    unsigned int match_mask(unsigned int mask) const noexcept;

    ChannelIdType id;
    unsigned short flags_;
    unsigned int weight_[CT_MAX];
    std::vector<CCGKeyword_var> ccg_keywords;
  };

  struct MatchInfo //second map item
  {
    Channel channel;
    std::string lang;
    std::string country;
    unsigned int channel_size;
    Generics::Time stamp;//time stamp
    Generics::Time db_stamp;//time stamp
  };

  typedef std::map<unsigned int, MatchInfo>
    MatchInfoContainerType;

  class ChannelIdToMatchInfo:
    public ReferenceCounting::AtomicImpl,
    public MatchInfoContainerType
  {
  public:
    ChannelIdToMatchInfo& operator=(const ChannelIdToMatchInfo& in)
      /*throw(eh::Exception)*/;
  private:
    virtual
    ~ChannelIdToMatchInfo() noexcept
    {
    }
  };

  typedef ReferenceCounting::SmartPtr<ChannelIdToMatchInfo>
    ChannelIdToMatchInfo_var;

  typedef std::map<unsigned int, Channel>
    ChannelMatchInfoContainerType;

  class ChannelMatchInfo:
    public ChannelMatchInfoContainerType,
    public ReferenceCounting::AtomicImpl
  {
  public:
    ChannelMatchInfo() noexcept {};
    ChannelMatchInfo(const ChannelIdToMatchInfo& info)
      noexcept;
  private:
    virtual
    ~ChannelMatchInfo() noexcept
    {
    }
  };

  typedef ReferenceCounting::SmartPtr<ChannelMatchInfo>
    ChannelMatchInfo_var;


  typedef std::map<unsigned long, unsigned int> WeightMap;

  class ChannelContainer;

  struct TriggerMatchItem
  {
    enum
    {
      TMI_NEGATIVE = 0x01,
      TMI_UID = 0x02
    };

    TriggerMatchItem() noexcept : flags(0), weight(0){};

    typedef IdVector value_type;

    unsigned int flags;
    unsigned int weight;
    value_type trigger_ids[CT_MAX];
  };

  typedef std::map<unsigned int, TriggerMatchItem> TriggerMatchData;

  class TriggerMatchRes: public TriggerMatchData
  {
  public:
    TriggerMatchRes()
    {
      memset(count_channels, 0, sizeof(count_channels));
    }
  public:
    size_t count_channels[CT_MAX + 1];
  };

  class ChannelChunk:
    public ReferenceCounting::AtomicImpl
  {
  public:

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    /* info - channel information, which usualy shared
     * beetween chunks. Chunk will own it */
    explicit
    ChannelChunk(ChannelMatchInfo* info)
      /*throw(eh::Exception)*/;

    typedef std::map<String::SubString, SoftVector> AddItemMap; 
    typedef std::map<Generics::Uuid, IdSet> AddUidMap;

  public:

    /*match urls */
    void match_url(
      const MatchUrl& url,
      unsigned int flags,
      TriggerMatchRes& res) const
      /*throw(eh::Exception)*/;

    void match_words(
      const Generics::SubStringHashAdapter& phrase,
      const MatchWords& words,
      const StringVector* exact_words,
      MatchType type,
      unsigned int flags,
      TriggerMatchRes& res,
      MatcherVarsSet* must_match)
      const
      /*throw(eh::Exception)*/;

    void match_uid(
      const Generics::Uuid& uid, 
      TriggerMatchRes& res)
      const
      /*throw(eh::Exception)*/;

    /* apply update saved in chunk 
     * @in - result chunk
     * updated_uid_channels - id of uid channels witch should be removed
     */
    void apply_update(
      ChannelChunk& in,
      const IdSet& updated_uid_channels,
      MatcherVarsSet* removed_url_matchers,
      MatcherVarsSet* removed_page_keyword_matchers,
      MatcherVarsSet* removed_url_keyword_matchers,
      MatcherVarsSet* removed_search_keyword_matchers)
      /*throw(Exception)*/;

    /* merge 2 vectors to a new one
     * @old_vec - old vector, assume it is in container
     * @new_vec - new vector with new and changed items, can be modifed on merging
     * @res_vec - result of merging
     * @removed_ids - channel trigger ids which should be erased from old vector
     */
    static
    void merge_items_(
      const SoftVector& old_vec,
      const SoftVector& new_vec,
      SoftVector& res_vec,
      const RemovedType& removed_ids,
      MatcherVarsSet* removed_matchers)
      noexcept;

    /* return rating of using key in chunk, more value is more used*/
    unsigned int get_raiting(const String::SubString& word) const noexcept;

    /* search matcher in chunk by parameters
     * @phrase - key of matcher in chunk
     * @trigger - trigger of matcher
     * @lang - lang of matcher
     * @trigger_type - type of trigger of matcher
     * returns pointer on matcher or 0
     * */
    SoftMatcher* get_matcher(
      const String::SubString& phrase,
      const std::string& trigger,
      unsigned short lang,
      char trigger_type,
      String::SubString& key) const
      noexcept;

    void set_info(ChannelMatchInfo* a_info) /*throw(eh::Exception)*/;

    ChannelMatchInfo* get_info_ptr() noexcept;

    void accamulate_statistic(ChannelServerStats& stats)
      noexcept;

    void terminate() noexcept;

    bool remove_action(
      const String::SubString& key,
      unsigned int channel_trigger_id,
      const SoftMatcher* matcher)
      noexcept;

    /* add entity  in chunk for merging*/
    void add_entity(
      const String::SubString& key,
      const MatchingEntity& entity,
      char trigger_type)
      /*throw(Exception)*/;

    /* add/remove channel to uid 
     * @uuid - uid
     * channel_id - new channel id, it id == 0, uid marks on removing
     */
    void update_uid(
      const Generics::Uuid& uuid,
      IdType channel_id)
      noexcept;

    static unsigned int get_active_options(unsigned int flags) noexcept;

    /* clear data preparing for merging */
    void cancel_update() noexcept;

    /* apply update to UidMap
     * @param res - result map, data in map isn't modified,
     * but map is modified
     * @param added - added new uid
     * @removed_uid_channels - list of removed ids of uid channels
     * witch should be erased from data in res map
     * */
    static void apply_uid_map_update_(
      UidMap& res,
      AddUidMap& added,
      const IdSet& removed_uid_channels)
      noexcept;

  private:

    /* add new and remove old triggers from map */
    static void apply_map_update_(
      TriggerMap& res,
      AddItemMap& added,
      RemovedType& removed_ids,
      MatcherVarsSet* removed_matchers)
      noexcept;

    static size_t match_cell_(
      const ChannelMatchInfo& cinfo,
      const MatchingEntity& atom,
      MatchType type,
      unsigned int flags,
      TriggerMatchRes& res,
      bool negative)
      noexcept;

    const TriggerMap&
    get_trigger_map_(MatchType type) const
    {
      if(type == CT_PAGE)
      {
        return *page_keyword_map_;
      }
      else if(type == CT_URL_KEYWORDS)
      {
        return *url_keyword_map_;
      }

      return *search_keyword_map_;
    }

    const TriggerMap&
    get_trigger_map_(char type) const
    {
      if(type == 'U')
      {
        return *url_map_var_;
      }
      else if(type == 'P')
      {
        return *page_keyword_map_;
      }
      else if(type == 'R')
      {
        return *url_keyword_map_;
      }

      return *search_keyword_map_;
    }

    TriggerMap&
    get_trigger_map_(char type)
    {
      if(type == 'U')
      {
        return *url_map_var_;
      }
      else if(type == 'P')
      {
        return *page_keyword_map_;
      }
      else if(type == 'R')
      {
        return *url_keyword_map_;
      }

      return *search_keyword_map_;
    }

    RemovedType&
    get_removed_ids_(char type)
    {
      if(type == 'U')
      {
        return removed_url_ids_;
      }
      else if(type == 'P')
      {
        return removed_page_keyword_ids_;
      }
      else if(type == 'R')
      {
        return removed_url_keyword_ids_;
      }

      return removed_search_keyword_ids_;
    }

    AddItemMap&
    get_add_map_(char type)
    {
      if(type == 'U')
      {
        return add_item_url_map_;
      }
      else if(type == 'P')
      {
        return add_item_page_keyword_map_;
      }
      else if(type == 'R')
      {
        return add_item_url_keyword_map_;
      }

      return add_item_search_keyword_map_;
    }

  protected:
    virtual
    ~ChannelChunk() noexcept;

  private:
    TriggerMap_var page_keyword_map_;
    TriggerMap_var url_keyword_map_;
    TriggerMap_var search_keyword_map_;

    //TriggerMap_var trigger_map_var_;//map triggers to TriggerAtom
    UidMap_var uid_map_var_;//map uids to UidAtom
    UrlMap_var url_map_var_;//map name to UrlMatcher

    ChannelMatchInfo_var match_info_ptr_;//
    volatile sig_atomic_t terminated_;

    AddItemMap add_item_url_keyword_map_;
    AddItemMap add_item_page_keyword_map_;
    AddItemMap add_item_search_keyword_map_;

  public:
    AddItemMap add_item_url_map_;

  private:
    AddUidMap add_item_uid_map_;

    RemovedType removed_page_keyword_ids_;
    RemovedType removed_url_keyword_ids_;
    RemovedType removed_search_keyword_ids_;
    RemovedType removed_url_ids_;
    RemovedType removed_uid_ids_;

    /// Statistics
    size_t params_[ChannelServerStats::NS_KW_COUNT];
  };

  typedef ReferenceCounting::SmartPtr<ChannelChunk> ChannelChunk_var;

  class ChannelChunkArray:
    public ReferenceCounting::AtomicImpl,
    public std::vector<ChannelChunk_var>
  {
  protected:
    virtual
    ~ChannelChunkArray() noexcept
    {
    }
  };

  typedef ReferenceCounting::SmartPtr<ChannelChunkArray> ChannelChunkArray_var;


  /*
  class TriggerMatchRes
  {
  public:
    typedef std::set<unsigned long> value_type;
    typedef std::vector<PositiveAtom> value_type2;

  public:
    value_type2 channel_ids[CT_MAX];
    value_type negative_channels;
    IdVector uid_channels;
    WeightMap weights;
  };*/

}
}

namespace AdServer
{
namespace ChannelSvcs
{

  inline
  void ChannelChunk::set_info(ChannelMatchInfo* a_info)
    /*throw(eh::Exception)*/
  {
    match_info_ptr_ = ReferenceCounting::add_ref(a_info);
  }

  inline
  ChannelMatchInfo* ChannelChunk::get_info_ptr() noexcept
  {
    return match_info_ptr_;
  }

  inline
  unsigned int
  ChannelChunk::get_active_options(unsigned int flags) noexcept
  {
    unsigned int active = 0;
    if(flags & MF_ACTIVE)
    {
      active |= (Channel::CH_ACTIVE | Channel::CH_WAIT);
    }
    if(flags & MF_INACTIVE)
    {
      active |= Channel::CH_INACTIVE;
    }
    return active;
  }

  inline
  void ChannelChunk::terminate() noexcept
  {
    terminated_ = true;
  }

  inline
  ChannelIdToMatchInfo& ChannelIdToMatchInfo::operator=(
    const ChannelIdToMatchInfo& in)
    /*throw(eh::Exception)*/
  {
    if(&in != this)
    {
      MatchInfoContainerType::operator=(in);
    }
    return *this;
  }

  inline
  Channel::Channel(ChannelIdType id_) noexcept
    : id(id_),
      flags_(0)
  {
    memset(weight_, 0, sizeof(weight_));
  }

  inline
  Channel::ChannelIdType Channel::get_id() const noexcept
  {
    return id;
  }

  inline
  bool Channel::operator!=(Channel::ChannelIdType cid) const
  {
    return (id != cid);
  }

  inline
  bool Channel::match(unsigned int type) const noexcept
  {
    return ((flags_ & (1<<type)) != 0);
  }

  inline
  unsigned int Channel::match_mask(unsigned int mask) const noexcept
  {
    return (flags_ & mask);
  }

  inline
  void Channel::mark_type(unsigned int type) noexcept
  {
    flags_ |= (1<<type);
  }

  inline
  void Channel::set_status(char status) noexcept
  {
    size_t index_type;
    switch(status)
    {
      case 'A':
        index_type = CT_ACTIVE;
        break;
      case 'W':
        index_type = CT_WAIT;
        break;
      case 'I':
        index_type = CT_INACTIVE;
        break;
     default:
        return;
        break;
    }
    mark_type(index_type);
  }

  inline
  void Channel::set_weight(MatchType type, unsigned int weight) noexcept
  {
    weight_[type] = weight;
    flags_ |= CH_WEIGHT;
  }

  inline
  ChannelMatchInfo::ChannelMatchInfo(const ChannelIdToMatchInfo& info) noexcept
  {
    for(ChannelIdToMatchInfo::const_iterator i = info.begin();
        i != info.end(); ++i)
    {
      (*this)[i->first] = i->second.channel;
    }
  }

}
}

#endif //AD_SERVER_CHANNEL_CHUNK
