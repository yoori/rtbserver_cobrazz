
#include <vector>
#include <map>
#include <eh/Exception.hpp>
#include <Stream/MemoryStream.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <String/Tokenizer.hpp>
#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/Constants.hpp>
#include <ChannelSvcs/ChannelCommons/Serialization.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include "ContainerMatchers.hpp"
#include "ChannelChunk.hpp"
#include "ChannelContainer.hpp"

namespace AdServer
{
namespace ChannelSvcs
{
  struct MatchingEntityLess
  {
    struct Holder
    {
      Holder(const std::string& trigger_val, unsigned short lang_val)
        : trigger(trigger_val),
          lang(lang_val)
      {}

      const std::string& trigger;
      unsigned short lang;
    };

    bool
    compare_(
      const std::string& left_trigger,
      unsigned short left_lang,
      const std::string& right_trigger,
      unsigned int right_lang) const
    {
      int trigger_compare = left_trigger.compare(right_trigger);
      return trigger_compare < 0 ||
        (trigger_compare == 0 && left_lang < right_lang);
    }

    template<typename LeftType, typename RightType>
    bool
    operator()(const LeftType& left, const RightType& right)
      const
    {
      return compare_(
        left.matcher->get_trigger(),
        left.matcher->get_lang(),
        right.matcher->get_trigger(),
        right.matcher->get_lang());
    }

    template<typename LeftType>
    bool
    operator()(const LeftType& left, const Holder& right)
      const
    {
      return compare_(
        left.matcher->get_trigger(),
        left.matcher->get_lang(),
        right.trigger,
        right.lang);
    }

    template<typename RightType>
    bool
    operator()(const Holder& left, const RightType& right)
      const
    {
      return compare_(
        left.trigger,
        left.lang,
        right.matcher->get_trigger(),
        right.matcher->get_lang());
    }
  };

  void dump_map(std::ostream& ostr, const TriggerMap& map) noexcept
  {
    for (TriggerMap::const_iterator i(map.begin()); i != map.end(); ++i)
    {
      ostr << "'" << static_cast<String::SubString>(i->first).str() << "' -> ";
      for (SoftVector::const_iterator soft_it(i->second->begin());
        soft_it != i->second->end(); ++soft_it)
      {
        ostr << ": '" << static_cast<String::SubString>(i->first).str() << "' "
          << soft_it->matcher << " ";
        for(auto it = soft_it->begin(); it != soft_it->end(); ++it)
        {
          if(it != soft_it->begin())
          {
            ostr << ", ";
          }
          ostr << '(' << it->channel_id << ", " << it->channel_trigger_id << ")";
        }
      }
      ostr << std::endl;
    }
  }

  /*
   * begin
   * ChannelChunk implementation
   *
   * */

  ChannelChunk::ChannelChunk(ChannelMatchInfo* info)
    /*throw(eh::Exception)*/
    : page_keyword_map_(new TriggerMap),
      url_keyword_map_(new TriggerMap),
      search_keyword_map_(new TriggerMap),
      uid_map_var_(new UidMap),
      url_map_var_(new TriggerMap),
      match_info_ptr_(ReferenceCounting::add_ref(info)),
      terminated_(false)
  {
    memset(params_, 0, sizeof(params_));
  }

  ChannelChunk::~ChannelChunk() noexcept
  {
  }

  unsigned int
  ChannelChunk::get_raiting(const String::SubString& word) const
    noexcept
  {
    unsigned int res = 1;

    {
      TriggerMap::const_iterator it = page_keyword_map_->find(word);
      if (it != page_keyword_map_->end())
      {
        res += it->second->size() * Commons::DEFAULT_MAX_HARD_WORD_SEQ;
      }

      AddItemMap::const_iterator it2 = add_item_page_keyword_map_.find(word);
      if (it2 != add_item_page_keyword_map_.end())
      {
        res += it2->second.size() * Commons::DEFAULT_MAX_HARD_WORD_SEQ;
      }
    }

    {
      TriggerMap::const_iterator it = url_keyword_map_->find(word);
      if (it != url_keyword_map_->end())
      {
        res += it->second->size() * Commons::DEFAULT_MAX_HARD_WORD_SEQ;
      }

      AddItemMap::const_iterator it2 = add_item_url_keyword_map_.find(word);
      if (it2 != add_item_url_keyword_map_.end())
      {
        res += it2->second.size() * Commons::DEFAULT_MAX_HARD_WORD_SEQ;
      }
    }

    {
      TriggerMap::const_iterator it = search_keyword_map_->find(word);
      if (it != search_keyword_map_->end())
      {
        res += it->second->size() * Commons::DEFAULT_MAX_HARD_WORD_SEQ;
      }

      AddItemMap::const_iterator it2 = add_item_search_keyword_map_.find(word);
      if (it2 != add_item_search_keyword_map_.end())
      {
        res += it2->second.size() * Commons::DEFAULT_MAX_HARD_WORD_SEQ;
      }
    }

    return res;
  }

  void ChannelChunk::match_url(
    const MatchUrl& url,
    unsigned int flags,
    TriggerMatchRes& res) const
    /*throw(eh::Exception)*/
  {
    //dump_map(std::cout, *url_map_var_);
    TriggerMap::const_iterator fnd = url_map_var_->find(url.prefix);
    if(fnd != url_map_var_->end())
    {
      const SoftVector& vector = *fnd->second;
      bool match_exact = (*url.postfix.rbegin() == '"');
      size_t length = url.postfix.length() - (match_exact ? 1 : 0);
      size_t comp_length;
      for(SoftVector::const_iterator it = vector.begin();
          it != vector.end(); ++it)
      {
        const String::SubString postfix = it->matcher->get_url_postfix();
        if(flags & MF_NONSTRICTURL)
        {
          comp_length = std::min(length, postfix.length());
        }
        else if((it->matcher->exact() && 
          (!match_exact || length != postfix.length())) ||
          length < postfix.length())
        {//doesn't match
          continue;
        }
        else
        {
          comp_length = postfix.length();
        }

        if(postfix.compare(
            0, comp_length, url.postfix, 0, comp_length) == 0)
        {
          res.count_channels[CT_URL] += match_cell_(
            *match_info_ptr_,
            *it,
            CT_URL,
            flags,
            res,
            (flags & MF_NEGATIVE ? false : it->matcher->negative()));
        }
      }
    }
  }

  void ChannelChunk::match_uid(
    const Generics::Uuid& uid, 
    TriggerMatchRes& res) const
    /*throw(eh::Exception)*/
  {
    UidMap::const_iterator fnd = uid_map_var_->find(uid);
    if(fnd != uid_map_var_->end())
    {
      for(auto it = fnd->second->begin(); it != fnd->second->end(); ++it)
      {
        res[*it].flags |= TriggerMatchItem::TMI_UID;
      }
    }
  }

  size_t ChannelChunk::match_cell_(
    const ChannelMatchInfo& cinfo,
    const MatchingEntity& atom,
    MatchType type,
    unsigned int flags,
    TriggerMatchRes& res,
    bool negative)
    noexcept
  {
    size_t count_new_ids = 0;
    unsigned int mask = ChannelChunk::get_active_options(flags);
    for(auto match_it = atom.begin(); match_it != atom.end(); ++match_it)
    {
      ChannelMatchInfo::const_iterator it = cinfo.find(match_it->channel_id);
      if(it != cinfo.end())
      {
        const Channel& channel = it->second;
        //check active mask, existing BH for channel and BL flag
        if((channel.match_mask(mask) && channel.match(type) &&
             (flags & MF_BLACK_LIST || !channel.match_mask(MF_BLACK_LIST))) ||
           (flags & MF_NONSTRICTKW))
        {
          TriggerMatchItem& item = res[match_it->channel_id];

          if(negative)
          {
            item.flags |= TriggerMatchItem::TMI_NEGATIVE; 
          }
          else if(!(item.flags & TriggerMatchItem::TMI_NEGATIVE))
          {
            item.trigger_ids[type].push_back(match_it->channel_trigger_id);
            ++count_new_ids;
            if(channel.match(Channel::CT_WEIGHT) && channel.weight_[type] > 0)
            {
              item.weight += channel.weight_[type];
            }
          }
        }
      }
    }
    return count_new_ids;
  }

  void ChannelChunk::match_words(
    const Generics::SubStringHashAdapter& phrase,
    const MatchWords& words,
    const StringVector* exact_words,
    MatchType type,
    unsigned int flags,
    TriggerMatchRes& res,
    MatcherVarsSet* must_match)
    const
    /*throw(eh::Exception)*/
  {
    const TriggerMap& check_trigger_map = get_trigger_map_(type);
    TriggerMap::const_iterator fnd = check_trigger_map.find(phrase);

    if(fnd != check_trigger_map.end()) // find one word
    {
      const TriggerAtom& atom = *fnd->second;
      const SoftVector& soft_vector = atom;
      char sym_type;
      switch(type)
      {
        case CT_PAGE:
          sym_type = 'P';
          break;
        case CT_URL_KEYWORDS:
          sym_type = 'R';
          break;
        default:
          sym_type = 'S';
          break;
      }

      for (SoftVector::const_iterator i(soft_vector.begin());
        i != soft_vector.end(); ++i)
      {
        const SoftMatcher* matcher = i->matcher.in();

        if(matcher->trigger_type() != sym_type)
        {
          continue;
        }
        bool match = matcher->match(words, (flags & MF_NONSTRICTKW ? true : false));
        if (exact_words && !match)
        {
          match |= matcher->match_exact(*exact_words);
        }
        if(must_match)
        {//additional soft matching
          if(must_match->erase(i->matcher) > 0)
          {
            match = true;
          }
        }
        if(match)
        {
          res.count_channels[type] += match_cell_(
            *match_info_ptr_,
            *i,
            type,
            flags,
            res,
            (flags & MF_NEGATIVE ? false : matcher->negative()));
        }
      }
    }
  }

  SoftMatcher*
  ChannelChunk::get_matcher(
    const String::SubString& phrase,
    const std::string& trigger,
    unsigned short lang,
    char trigger_type,
    String::SubString& key) const
    noexcept
  {
    const TriggerMap& check_trigger_map = get_trigger_map_(trigger_type);
    TriggerMap::const_iterator fnd = check_trigger_map.find(phrase);

    if(fnd != check_trigger_map.end())
    {
      const SoftVector& soft_vector = *fnd->second;
      SoftVector::const_iterator it = std::lower_bound(
        soft_vector.begin(),
        soft_vector.end(),
        MatchingEntityLess::Holder(trigger, lang),
        MatchingEntityLess());

      if(it != soft_vector.end() &&
         it->matcher->get_trigger() == trigger &&
         it->matcher->get_lang() == lang)
      {
        key = fnd->first;
        return it->matcher;
      }
    }

    return 0;
  }

  void ChannelChunk::update_uid(
    const Generics::Uuid& uuid,
    IdType channel_id) noexcept
  {
    IdSet& ids = add_item_uid_map_[uuid];
    if (channel_id)
    {//only create cell in hash for removing
      ids.insert(channel_id);
    }
  }

  bool ChannelChunk::remove_action(
    const String::SubString& key,
    unsigned int channel_trigger_id,
    const SoftMatcher* matcher)
    noexcept
  {
    //check only existing of key, it is cheap
    char trigger_type = matcher->trigger_type();
    if(trigger_type == 'U')
    {
      auto change_it = url_map_var_->find(key);
      if(change_it == url_map_var_->end())
      {
        return false;
      }
      removed_url_ids_.insert(channel_trigger_id);
      add_item_url_map_[change_it->first];
      //create key in update map for applying remove list
    }
    else if (trigger_type != 'D')
    {
      TriggerMap& trigger_map = get_trigger_map_(trigger_type);
      auto change_it = trigger_map.find(key);
      if(change_it == trigger_map.end())
      {
        return false;
      }

      RemovedType& removed_ids = get_removed_ids_(trigger_type);
      AddItemMap& add_map = get_add_map_(trigger_type);
      for(auto it_rm = change_it->second->begin();
          it_rm != change_it->second->end(); ++it_rm)
      {
        if(it_rm->matcher == matcher)
        {
          // create key in update map for applying remove list
          removed_ids.insert(channel_trigger_id);
          add_map.insert(std::make_pair(change_it->first, SoftVector()));
          return true;
        }
      }
      return false;
    }
    else
    {
      assert(0);
    }

    return true;
  }

  void ChannelChunk::add_entity(
    const String::SubString& key,
    const MatchingEntity& entity,
    char trigger_type)
    /*throw(Exception)*/
  {
    AddItemMap *tmap;
    switch(trigger_type)
    {
      case 'U':
        tmap = &add_item_url_map_;
        break;
      case 'P':
        tmap = &add_item_page_keyword_map_;
        break;
      case 'R':
        tmap = &add_item_url_keyword_map_;
        break;
      default:
        tmap = &add_item_search_keyword_map_;
        break;
    }
    assert (!entity.empty());

    SoftVector& vec = (*tmap)[key]; // invalid read here
    auto ins_it = std::lower_bound(vec.begin(), vec.end(), entity, MatchingEntityLess());
    vec.emplace(ins_it, entity);
  }

  /* merge 2 sequences to one exclude elements from second sequence 
  * if functor pred isn't true */
  template <
  class InputIterator1, class InputIterator2,
  class OutputIterator, class CompPred, class UnaryPredicate>
  OutputIterator merge_if_not_exclude (
    InputIterator1 first1,
    InputIterator1 last1,
    InputIterator2 first2,
    InputIterator2 last2,
    OutputIterator result,
    CompPred pred_comp,
    UnaryPredicate pred)
  {
    while (true)
    {
      if (first1 == last1)
      {
        return std::copy_if(first2, last2, result, pred);
      }

      if (first2 == last2)
      {
        return std::copy(first1, last1, result);
      }

      if(pred_comp(*first2, *first1))
      {
        if (pred(*first2))
        {
          *result++ = *first2;
        }
        first2++;
      }
      else
      {
        *result++ = *first1++;
      }
    }
  }

  struct RemovePred
  {
    RemovePred(const RemovedType& removed) noexcept
      : removed_(removed)
    {};

    bool
    operator() (const PositiveAtom& entity) const
    {
      return (removed_.find(entity.channel_trigger_id) == removed_.end());
    }

  private:
    const RemovedType& removed_;
  };

  void ChannelChunk::merge_items_(
    const SoftVector& old_vec,
    const SoftVector& new_vec,
    SoftVector& res_vec,
    const RemovedType& removed_ids,
    MatcherVarsSet* removed_matchers)
    noexcept
  {
    static MatchingEntityLess entity_less;
    RemovePred rem_pred(removed_ids);
    size_t guess_size = old_vec.size() + new_vec.size();
    res_vec.resize(guess_size);
    auto old_it = old_vec.begin();
    auto new_it = new_vec.begin();
    auto res_it = res_vec.begin();
    while(old_it != old_vec.end() && new_it != new_vec.end())
    {
      if(old_it->matcher == new_it->matcher)
      {
        // same matcher, just merge ids
        PositiveContainerType::iterator vec_it;
        res_it->resize(new_it->size() + old_it->size());
        if(removed_ids.empty())
        {
          vec_it = std::merge(
            new_it->begin(),
            new_it->end(),
            old_it->begin(),
            old_it->end(),
            res_it->begin());
        }
        else
        {
          vec_it = merge_if_not_exclude(
            new_it->begin(),
            new_it->end(),
            old_it->begin(),
            old_it->end(),
            res_it->begin(),
            std::less<PositiveAtom>(),
            rem_pred);
        }
        res_it->resize(vec_it - res_it->begin());
        if(!res_it->empty())
        {
          res_it->matcher = old_it->matcher;
          ++res_it;
        }
        else if(removed_matchers)
        {
          removed_matchers->insert(old_it->matcher);
        }

        ++old_it;
        ++new_it;
      }
      else
      {
        if(entity_less(*old_it, *new_it))
        {
          // old less copy it with removing ids
          if(removed_ids.empty())
          {
            *res_it++ = *old_it;
          }
          else
          {
            res_it->resize(old_it->size());
            auto vec_it = copy_if(old_it->begin(), old_it->end(), res_it->begin(), rem_pred); 
            res_it->resize(vec_it - res_it->begin());
            if(!res_it->empty())
            {
              res_it->matcher = old_it->matcher;
              ++res_it;
            }
            else if(removed_matchers)
            {
              removed_matchers->insert(old_it->matcher);
            }
          }
          ++old_it;
        }
        else
        {
          // new less, just copy it
          *res_it++ = (*new_it++);
        }
      }
    }
    if(old_it == old_vec.end())
    {
      while(new_it != new_vec.end())
      {
        *res_it++ = (*new_it++);
      }
    }
    else if(new_it == new_vec.end())
    {
      if(removed_ids.empty())
      {
        res_it = std::copy(old_it, old_vec.end(), res_it);
      }
      else
      {
        while(old_it != old_vec.end())
        {
          res_it->resize(old_it->size());
          auto vec_it = copy_if(
            old_it->begin(), old_it->end(), res_it->begin(), rem_pred); 
          res_it->resize(vec_it - res_it->begin());
          if(!res_it->empty())
          {
            res_it->matcher = old_it->matcher;
            ++res_it;
          }
          else if(removed_matchers)
          {
            removed_matchers->insert(old_it->matcher);
          }
          old_it++;
        }
      }
    }
    res_vec.resize(res_it - res_vec.begin());
  }

  void ChannelChunk::apply_map_update_(
    TriggerMap& res,
    AddItemMap& added,
    RemovedType& removed_ids,
    MatcherVarsSet* removed_matchers)
    noexcept
  {
    while(!added.empty())
    {
      auto it = added.begin();
      const String::SubString& key = it->first;
      SoftVector& new_vec = it->second;
      auto res_it = res.find(key);
      if (res_it == res.end())
      {
        assert (!new_vec.empty());
        TriggerAtom_var& value = res[key];
        value = new TriggerAtom;
        value->swap(new_vec);
      }
      else 
      {
        TriggerAtom_var temp = new TriggerAtom;
        const TriggerAtom& src_vec = *res_it->second;
        merge_items_(src_vec, new_vec, *temp, removed_ids, removed_matchers);
        if (removed_ids.empty())//don't need update key
        {
          res_it->second = temp;
        }
        else
        { //it was removed items, need remove key and setup it again, because
          //it could be removed with matchers
          res.erase(res_it);
          if (!temp->empty())
          {
            String::SubString new_key;
            assert(temp->begin()->matcher->find_token(key, new_key));
            res[new_key] = temp;
          }
        }
      }
      added.erase(it);
    }
    removed_ids.clear();
  }

  struct RemoveSortedPred
  {
    RemoveSortedPred(const RemovedType& removed) noexcept
      : first_(true),
        removed_(removed)
    {};

    bool operator() (unsigned long id)
    {
      if (first_)
      {
        first_ = false;
        last_it_ = removed_.lower_bound(id);
        if (last_it_ == removed_.end())
        {
          return true;
        }
      }
      else
      {
        while(*last_it_ < id)
        {
          last_it_++;
          if (last_it_ == removed_.end())
          {
            return true;
          }
        }
      }
      if (id < *last_it_)
      {
        return true;
      }
      return false;
    }
  private:
    bool first_;
    const RemovedType& removed_;
    RemovedType::const_iterator last_it_;
  };

  void ChannelChunk::apply_uid_map_update_(
    UidMap& res,
    AddUidMap& added,
    const IdSet& removed_uid_channels)
    noexcept
  {
    while(!added.empty())
    {
      auto it = added.begin();
      const AddUidMap::key_type& uid = it->first;
      const AddUidMap::mapped_type& channels = it->second;
      auto change_it = res.find(uid);
      if (change_it == res.end())
      {//new uid in container
        UidAtom_var& atom = res[uid];
        atom = new UidAtom();
        atom->insert(atom->end(), channels.begin(), channels.end());
      }
      else
      {
        const IdVector& old_channels = *change_it->second;
        UidAtom_var uid_atom = new UidAtom();
        uid_atom->resize(old_channels.size() + channels.size());
        if (removed_uid_channels.empty())
        {//merge 
          std::merge(
            old_channels.begin(),
            old_channels.end(),
            channels.begin(),
            channels.end(),
            uid_atom->begin());
        }
        else
        {
          auto end_it = merge_if_not_exclude(
            channels.begin(),
            channels.end(),
            old_channels.begin(),
            old_channels.end(),
            uid_atom->begin(),
            std::less<unsigned long>(),
            RemoveSortedPred(removed_uid_channels));
          uid_atom->resize(std::distance(uid_atom->begin(), end_it));
        }
        if (uid_atom->empty())
        {
          res.erase(change_it);
        }
        else
        {
          change_it->second = uid_atom;
        }
      }
      added.erase(it);
    }
  }

  void ChannelChunk::cancel_update() noexcept
  {
    add_item_url_keyword_map_.clear();
    removed_url_keyword_ids_.clear();

    add_item_page_keyword_map_.clear();
    removed_page_keyword_ids_.clear();

    add_item_search_keyword_map_.clear();
    removed_search_keyword_ids_.clear();

    add_item_url_map_.clear();
    removed_url_ids_.clear();

    add_item_uid_map_.clear();
    removed_uid_ids_.clear();
  }

  void ChannelChunk::apply_update(
    ChannelChunk& in,
    const IdSet& updated_uid_channels,
    MatcherVarsSet* removed_url_matchers,
    MatcherVarsSet* removed_page_keyword_matchers,
    MatcherVarsSet* removed_url_keyword_matchers,
    MatcherVarsSet* removed_search_keyword_matchers)
    /*throw(Exception)*/
  {
    try
    {
      if(add_item_url_keyword_map_.empty() &&
        add_item_page_keyword_map_.empty() &&
        add_item_search_keyword_map_.empty() &&
        add_item_url_map_.empty() &&
        add_item_uid_map_.empty())
      {
        in.page_keyword_map_ = page_keyword_map_;
        in.url_keyword_map_ = url_keyword_map_;
        in.search_keyword_map_ = search_keyword_map_;
        in.url_map_var_ = url_map_var_;
        in.uid_map_var_ = uid_map_var_;
        return;
      }
      in.page_keyword_map_ = new TriggerMap(*page_keyword_map_);
      in.url_keyword_map_ = new TriggerMap(*url_keyword_map_);
      in.search_keyword_map_ = new TriggerMap(*search_keyword_map_);
      in.url_map_var_ = new TriggerMap(*url_map_var_);
      in.uid_map_var_ = new UidMap(*uid_map_var_);

      apply_map_update_(
        *in.url_keyword_map_,
        add_item_url_keyword_map_,
        removed_url_keyword_ids_,
        removed_url_keyword_matchers);
      apply_map_update_(
        *in.page_keyword_map_,
        add_item_page_keyword_map_,
        removed_page_keyword_ids_,
        removed_page_keyword_matchers);
      apply_map_update_(
        *in.search_keyword_map_,
        add_item_search_keyword_map_,
        removed_search_keyword_ids_,
        removed_search_keyword_matchers);
      apply_map_update_(
        *in.url_map_var_,
        add_item_url_map_,
        removed_url_ids_,
        removed_url_matchers);
      apply_uid_map_update_(*in.uid_map_var_, add_item_uid_map_, updated_uid_channels);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exceptione: " << e.what();
      throw Exception(ostr);
    }
  }

  void ChannelChunk::accamulate_statistic(ChannelServerStats& stats)
    noexcept
  {
    if(!params_[ChannelServerStats::KW_COUNT] &&
       !params_[ChannelServerStats::URL_COUNT])
    {//assume statistic wasn't accumulate yet
      params_[ChannelServerStats::KW_COUNT] =
        page_keyword_map_->size() +
        url_keyword_map_->size() +
        search_keyword_map_->size();
      params_[ChannelServerStats::URL_COUNT] = url_map_var_->size();
      params_[ChannelServerStats::UID_COUNT] = uid_map_var_->size();

      for(TriggerMap::const_iterator i(page_keyword_map_->begin());
        i != page_keyword_map_->end(); ++i)
      {
        params_[ChannelServerStats::KW_ID_COUNT] += i->second->size();
      }

      for(TriggerMap::const_iterator i(url_keyword_map_->begin());
        i != url_keyword_map_->end(); ++i)
      {
        params_[ChannelServerStats::KW_ID_COUNT] += i->second->size();
      }

      for(TriggerMap::const_iterator i(search_keyword_map_->begin());
        i != search_keyword_map_->end(); ++i)
      {
        params_[ChannelServerStats::KW_ID_COUNT] += i->second->size();
      }

      for(TriggerMap::const_iterator i(url_map_var_->begin());
        i != url_map_var_->end(); ++i)
      {
        params_[ChannelServerStats::URL_ID_COUNT] += i->second->size();
      }

      for(UidMap::const_iterator i(uid_map_var_->begin());
        i != uid_map_var_->end(); ++i)
      {
        params_[ChannelServerStats::UID_ID_COUNT] += i->second->size();
      }
    }
    for(size_t i = 0; i <= ChannelServerStats::UID_ID_COUNT; ++i)
    {
      stats.params[i] += params_[i];
    }
  }

  /*
   * end
   * ChannelChunk implementation
   *
   * */
}
}
