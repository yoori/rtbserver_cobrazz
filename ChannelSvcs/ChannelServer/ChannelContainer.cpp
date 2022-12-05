
#include <ReferenceCounting/SmartPtr.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Commons/Constants.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include "ChannelContainer.hpp"
#include "UpdateContainer.hpp"

namespace AdServer
{
namespace ChannelSvcs
{

  const char* ChannelContainer::ASPECT = "ChannelContainer";

  namespace
  {
    const unsigned long PATH_DEPTH = 12;
    const std::string EMPTY_STRING;
  }

  /*
   * begin
   * ChannelContainerBase implementation
   *
   * */
  ChannelContainerBase::ChannelContainerBase() noexcept
  {
  }

  void ChannelContainer::check_update(
    const Generics::Time& master,
    const std::set<unsigned int>& ids,
    CheckInformation& info,
    bool use_only_list) const
    noexcept
  {
    ReadGuard_ lock(lock_update_data_);
    info.master = master_;
    info.first_master = first_master_;
    info.longest_update = max_update_;
    info.special_track =
      (channel_ids_.find(c_special_track) != channel_ids_.end());
    info.special_adv =
      (channel_ids_.find(c_special_adv) != channel_ids_.end());
    if(use_only_list)
    {
      for(auto id_it = ids.begin(); id_it != ids.end(); id_it++)
      {
        auto it = channel_ids_.find(*id_it);
        if(it != channel_ids_.end())
        {
          CheckInformation::CheckData& value = info.data[*id_it];
          value.stamp = it->second->stamp;
          value.size = it->second->channel_size;
        }
      }
    }
    else
    {
      for (ChannelMap::const_iterator it = channel_ids_.begin();
        it != channel_ids_.end(); ++it)
      {
        if(it->second->stamp > master || ids.find(it->first) != ids.end())
        {
          CheckInformation::CheckData& value = info.data[it->first];
          value.stamp = it->second->stamp;
          value.size = it->second->channel_size;
        }
      }
    }
  }

  /*
   * begin
   * ChannelContainer implementation
   *
   * */
  ChannelContainer::ChannelContainer(
    unsigned long count_chunks, bool nonstrict)
    /*throw(Exception)*/
    : ChannelContainerBase(),
      count_chunks_(count_chunks),
      chunks_(new ChannelChunkArray),
      ccgs_(new CCGMap),
      master_(0),
      first_master_(0),
      max_update_(0),
      non_strict_(nonstrict),
      queries_(0),
      exceptions_(0),
      terminated_(false)
  {
    try
    {
      memset(stats_.params, 0, sizeof(stats_.params));
      chunks_->resize(count_chunks_);
      ChannelMatchInfo_var info = new ChannelMatchInfo;
      for(unsigned int i = 0; i < count_chunks_; i++)
      {
        (*chunks_)[i] = new ChannelChunk(info);
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "ChannelContainer::ChannelContainer "
        "Caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  SoftMatcher*
  ChannelContainer::get_matcher(
    const std::string& trigger,
    unsigned short lang,
    String::SubString& part) const
    /*throw(Exception)*/
  {
    static const char* FN = "ChannelContainer::get_matcher";
    try
    {
      SoftMatcher* ret = 0;
      SubStringVector parts;
      Serialization::get_parts(trigger, parts);
      char trigger_type = Serialization::trigger_type(trigger.data());
      if(trigger_type == 'U')
      {
        assert(parts.size() == 2);
        parts.resize(1);
        //use prefix size of url
      }

      ChannelChunkArray_var array;

      {
        ReadGuard_ lock(lock_configuration_);
        array = ReferenceCounting::add_ref(chunks_);
      }

      for(SubStringVector::const_iterator it = parts.begin();
          it != parts.end(); ++it)
      {
        const ChannelChunk* chunk = (*array)[calc_chunk_num_i_(*it)];
        ret = chunk->get_matcher(*it, trigger, lang, trigger_type, part);
        // part => part of trigger or key inside chunk
        if (ret)
        {
          break;
        }
      }
      return ret;
    }
    catch(const eh::Exception& e)
    {
      Stream::Error err;
      err << FN << ": eh::Exception: " << e.what();
      throw Exception(err);
    }
  }

  void ChannelContainer::remove_non_strict_(
    NSTriggerAtom& urls,
    NSTriggerAtom& page_keywords,
    NSTriggerAtom& url_keywords,
    NSTriggerAtom& search_keywords)
    noexcept
  {
    for(auto it = urls.begin(); it != urls.end(); it++)
    {
      const String::SubString url_prefix = (*it)->get_url_postfix();
      size_t path_pos = url_prefix.find('/');
      if(path_pos != std::string::npos)
      {
        // this url should be in non strict map
        String::SubString key = url_prefix.substr(0, path_pos);
        remove_ns_urls_(key, *it);
      }
    }
    urls.clear();

    for(auto it = page_keywords.begin(); it != page_keywords.end(); ++it)
    {
      SubStringVector parts;
      (*it)->get_tokens(parts);
      remove_ns_words_(parts, *it);
    }
    page_keywords.clear();

    for(auto it = url_keywords.begin(); it != url_keywords.end(); ++it)
    {
      SubStringVector parts;
      (*it)->get_tokens(parts);
      remove_ns_words_(parts, *it);
    }
    url_keywords.clear();

    for(auto it = search_keywords.begin(); it != search_keywords.end(); it++)
    {
      SubStringVector parts;
      (*it)->get_tokens(parts);
      remove_ns_words_(parts, *it);
    }
    search_keywords.clear();
  }

  void ChannelContainer::remove_ns_words_(
    const SubStringVector& keys,
    const SoftMatcher_var& matcher)
    noexcept
  {
    WriteGuard_ lock(lock_ns_map_);
    for(SubStringVector::const_iterator key_it = keys.begin();
        key_it != keys.end(); ++key_it)
    {
      NSTriggerMapType::iterator it = ns_trigger_map_.find(*key_it);
      if (it != ns_trigger_map_.end())
      {
        if (it->second.erase(matcher) > 0)
        {//always remove key from map, because substring can be from this matcher
          String::SubString m_key;
          NSTriggerAtom temp;
          temp.swap(it->second);
          ns_trigger_map_.erase(it++);
          if (!temp.empty())
          {//set substring from first matcher
            if ((*temp.begin())->find_token(*key_it, m_key))
            {
              it = ns_trigger_map_.insert(
                it, std::make_pair(m_key, NSTriggerAtom()));
              it->second.swap(temp); 
            }
          }
        }
      }
    }
  }

  void ChannelContainer::add_ns_words_(
    const SubStringVector& keys,
    SoftMatcher* matcher,
    const String::SubString& key_word)
    noexcept
  {
    WriteGuard_ lock(lock_ns_map_);
    for(SubStringVector::const_iterator key_it = keys.begin();
        key_it != keys.end(); ++key_it)
    {
      if (*key_it != key_word)
      {
        NSTriggerAtom& atom = ns_trigger_map_[*key_it];
        atom.insert(ReferenceCounting::add_ref(matcher));
      }
    }
  }

  void ChannelContainer::remove_ns_urls_(
    const String::SubString& url,
    const SoftMatcher_var& matcher)
    noexcept
  {
    WriteGuard_ lock(lock_ns_map_);
    NSUrlMapType::iterator it = ns_url_map_.find(url);
    if(it == ns_url_map_.end())
    {
      return;
    }
    if (it->second.erase(matcher) > 0)
    {
      NSTriggerAtom temp;
      temp.swap(it->second);
      ns_url_map_.erase(it++);
      if (!temp.empty())
      {//update url in map
        String::SubString key(
          (*temp.begin())->get_url_prefix().text().data(),
          url.length());
        it = ns_url_map_.insert(it, std::make_pair(key, NSTriggerAtom()));
        it->second.swap(temp);
      }
    }
  }

  void ChannelContainer::add_ns_urls_(
    const String::SubString& url,
    SoftMatcher* matcher)
    noexcept
  {
    WriteGuard_ lock(lock_ns_map_);
    NSTriggerAtom& atom = ns_url_map_[url];
    atom.insert(ReferenceCounting::add_ref(matcher));
  }

  void ChannelContainer::add_ns_(
    SoftMatcher* matcher,
    const String::SubString& key_word)
    noexcept
  {
    if(!non_strict_)
    {
      return;
    }
    char trigger_type = matcher->trigger_type();
    if(trigger_type == 'D')
    {
      return;
    }
    if(trigger_type == 'U')
    {
      const String::SubString url_prefix = matcher->get_url_prefix();
      size_t path_pos = url_prefix.find('/');
      if(path_pos != std::string::npos)
      {//this url should be in  non strict map
        String::SubString key = url_prefix.substr(0, path_pos);
        add_ns_urls_(key, matcher);
      }
    }
    else
    {
      SubStringVector parts;
      add_ns_words_(matcher->get_tokens(parts), matcher, key_word);
    }
  }

  void
  ChannelContainer::fetch_add_item_url_map_(
    const std::string& step, const ChannelChunk& channel_chunk) const
  {
    const String::SubString PART_CHECK("xxxxxxxxxxxxxxxxxxx");

    std::cerr << "INVALID DEBUG: TO FETCH add_item_url_map_ ON STEP '" << step << "'" << std::endl;

    bool c = false;
    for(auto tr_it = channel_chunk.add_item_url_map_.begin();
      tr_it != channel_chunk.add_item_url_map_.end(); ++tr_it)
    {
      c = c || (tr_it->first < PART_CHECK);
    }
    (void)c;

    std::cerr << "INVALID DEBUG: FROM FETCH add_item_url_map_ ON STEP '" << step << "'" << std::endl;
  }

  void ChannelContainer::add_with_existing_matcher_(
    const ChannelChunkArray& array,
    UnmergedValue& items,
    SoftMatcher* matcher,
    const String::SubString& word) const
    noexcept
  {
    MatchingEntity entity(matcher);
    entity.reserve(items.size());
    for(auto it = items.begin(); it != items.end(); ++it)
    {
      entity.emplace_back(it->channel_id, it->word.channel_trigger_id);
    }
    std::sort(entity.begin(), entity.end());
    unsigned long num;
    const Lexeme_var& lexeme = matcher->matched_lexeme();
    if(lexeme.in())
    {
      for(LexemeData::Forms::const_iterator sub_it = lexeme->forms.begin();
        sub_it !=  lexeme->forms.end(); ++sub_it)
      {
        num = calc_chunk_num_i_(*sub_it);
        array[num]->add_entity(*sub_it, entity, matcher->trigger_type());
      }
    }
    else
    {
      num = calc_chunk_num_i_(word);

      //fetch_add_item_url_map_(std::string("before add_entity: ") + word.str(), *array[num]);

      array[num]->add_entity(word, entity, matcher->trigger_type()); // invalid read

      //fetch_add_item_url_map_(std::string("after add_entity: ") + word.str(), *array[num]);
    }
  }

  void ChannelContainer::add_uids_(
    const ChannelChunkArray& array,
    const SoftMatcher_var uid_matcher,
    IdType channel_id)
    /*throw(eh::Exception)*/
  {
    //there isn't word in matcher for uid channels, so extract them from trigger
    const std::string& trigger = uid_matcher->get_trigger();
    SubStringVector words;
    Serialization::get_parts(trigger, words);
    for(auto it = words.begin(); it != words.end(); ++it)
    {
      Generics::Uuid uuid(*it, false);
      array[calc_chunk_num_(uuid.hash(), array.size())]->update_uid(
        uuid, channel_id);
    }
  }

  template<class VECTOR>
  bool ChannelContainer::remove_action_(
    const ChannelChunkArray& array,
    unsigned int channel_trigger_id,
    SoftMatcher* matcher,
    const VECTOR& words) const
    noexcept
  {
    unsigned long num;
    bool found = false;
    for(auto sub_it = words.begin(); sub_it != words.end(); ++sub_it)
    {
      num = calc_chunk_num_i_(*sub_it);
      found |= array[num]->remove_action(
        *sub_it, channel_trigger_id, matcher);
    }
    return found;
  }

  unsigned int ChannelContainer::choose_word_(
    const ChannelChunkArray& array,
    const SubStringVector& parts,
    const LexemesPtrVector& lexemes) const
    noexcept
  {
    unsigned int word_num = 0;
    unsigned int raiting = UINT_MAX;
    unsigned int i = 0;

    for(auto word_it = parts.begin();
        word_it != parts.end(); ++word_it, i++)
    {
      long word_raiting;
      if(lexemes.size() > i && lexemes[i])
      {
        word_raiting = 0;
        for(LexemeData::Forms::const_iterator sub_it =
              lexemes[i]->forms.begin();
            sub_it != lexemes[i]->forms.end(); ++sub_it)
        {
          unsigned long num = calc_chunk_num_i_(*sub_it);
          word_raiting +=
            array[num]->get_raiting(*sub_it);
        }
      }
      else
      {
        unsigned long num =
          calc_chunk_num_i_(*word_it);
        word_raiting =
          array[num]->get_raiting(*word_it);
      }
      if(word_raiting < raiting)
      {
        word_num = i;
        if(word_raiting == 1)
        {
          break;
        }
        raiting = word_raiting;
      }
    }
    return word_num;
  }

  void ChannelContainer::check_actual_(
    const ChannelMap& info_old,
    ChannelIdToMatchInfo& info_new,
    ExcludeContainerType &new_ids,
    ExcludeContainerType &up_ids,
    ExcludeContainerType &rm_ids) 
    noexcept
  {
    ChannelMap::const_iterator it_i = info_old.begin();
    ChannelIdToMatchInfo::iterator it_j = info_new.begin();
    new_ids.clear();
    up_ids.clear();
    rm_ids.clear();
    while(it_i != info_old.end() || it_j != info_new.end())
    {
      if(it_j == info_new.end())
      {
        rm_ids.insert(it_i->first);//remove this id
        ++it_i;
      }
      else if(it_i == info_old.end())//add this id
      {
        new_ids.insert(it_j->first);
        ++it_j;
      }
      else if(it_i->first < it_j->first)//remove this id
      {
        rm_ids.insert(it_i->first);//remove this id
        ++it_i;
      }
      else if(it_i->first > it_j->first)//add this id
      {
        new_ids.insert(it_j->first);
        ++it_j;
      }
      else
      {
        it_j->second.stamp = it_i->second->stamp;
        it_j->second.db_stamp = it_i->second->db_stamp;
        it_j->second.channel_size = it_i->second->channel_size;
        if (it_j->second.lang != it_i->second->lang)
        {//update it, language changed
          up_ids.insert(it_j->first);
        }
        ++it_i;
        ++it_j;
      }
    }
  }

  void ChannelContainer::check_actual(
    ChannelIdToMatchInfo& info,
    ExcludeContainerType& new_channels,
    ExcludeContainerType& updated_channels,
    ExcludeContainerType& removed_channels) const
    noexcept
  {
    ReadGuard_ lock(lock_update_data_);
    check_actual_(
      channel_ids_,
      info,
      new_channels,
      updated_channels,
      removed_channels);
  }

  void ChannelContainer::trace_update_data_(
    const UpdateContainer& add,
    std::ostream& debug) 
    noexcept
  {
    const UpdateContainer::Matters& matters_cont = add.get_matters();
    for(auto matters_it = matters_cont.begin();
        !terminated_ && matters_it != matters_cont.end(); ++matters_it)
    {
      auto channel_id = matters_it->first;
      const auto& item = matters_it->second;
      const SoftTriggerList& matters = item.added;
      debug << ' ' << channel_id << " : " << item.lang << " : ";
      trace_sequence("removed", item.removed, debug);
      debug << " added:";
      for(SoftTriggerList::const_iterator it = matters.begin();
          it != matters.end(); ++it)
      {
        if(it != matters.begin())
        {
          debug << ",";
        }
        debug << " " << it->channel_trigger_id;
      }
      debug << ".";
    }
  }

  size_t ChannelContainer::add_stage1(
    UpdateContainer& add,
    UnmergedMap& unmerged,
    ChannelIdToTrigers& added)
    /*throw(Exception)*/
  {
    try
    {
      size_t count_channels = 0;
      IdType channel_id = 0;
      ChannelChunkArray_var match_chunks;
      {
        ReadGuard_ lock(lock_configuration_);
        match_chunks = ReferenceCounting::add_ref(chunks_);
      }
      UpdateContainer::Matters& matters_cont = add.get_matters();
      auto matters_it = matters_cont.begin();
      while(!terminated_ && matters_it != matters_cont.end())
      {
        // group all unmerged triggers by trigger + lang index
        channel_id = matters_it->first;
        UpdateContainer::MatterItem& item = matters_it->second;
        SoftTriggerList& matters = item.added;
        while(!matters.empty())
        {
          SoftTriggerList::iterator it = matters.begin();
          SoftTriggerWord& word = *it;
          //const std::string* old_trigger_ptr = &word.trigger;
          auto ch_it = unmerged.find(UnmergedKey(item.lang, word.trigger));
          if (ch_it == unmerged.end())
          {
            // new key need to create list first to allow use trigger ref inside it
            UnmergedValue unm_value;
            unm_value.emplace_back(UnmergedItem(channel_id));
            unm_value.back().word.swap(word);

            UnmergedValue& unm_value_ref = unmerged[UnmergedKey(item.lang, unm_value.back().word.trigger)];
            unm_value_ref.swap(unm_value);
          }
          else
          {
            ch_it->second.emplace_back(UnmergedItem(channel_id));
            ch_it->second.back().word.swap(word);
          }
          matters.erase(it);
        }

        if (!item.uids.empty())
        {
          // add uids here
          SoftMatcher_var uid_matcher = new SoftMatcher(
            item.lang,
            item.uids);
          added[channel_id][0] = uid_matcher;
          add_uids_(*match_chunks, uid_matcher, channel_id);
        }
        matters_it++;
        count_channels++;
      }

      String::SubString match_word;
      auto it = unmerged.begin();
      while(!terminated_ && it != unmerged.end())
      {
        // find existing matcher from container
        assert(it->first.trigger == it->second.begin()->word.trigger &&
          &(it->first.trigger) == &(it->second.begin()->word.trigger)); // TO CHECK !!!

        SoftMatcher* matcher = get_matcher(
          it->second.begin()->word.trigger, it->first.lang, match_word);
        // return not null matcher if it found inside chunk : key => key inside chunk

        if (matcher)
        {
          add_with_existing_matcher_(
            *match_chunks, it->second, matcher, match_word);

          for(auto it_un = it->second.begin(); it_un != it->second.end(); ++it_un)
          {
            added[it_un->channel_id][it_un->word.channel_trigger_id] =
              ReferenceCounting::add_ref(matcher);
          }

          // workaround : key hold ref to element (key keep reference to string inside value)
          UnmergedValue remove_unmerged_value;
          remove_unmerged_value.swap(it->second);

          unmerged.erase(it++);
        }
        else
        {
          ++it;
        }
      }
      return count_channels;
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh:Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void ChannelContainer::add_stage2(
    UpdateContainer& add,
    UnmergedMap& unmerged,
    ChannelIdToTrigers& added)
    /*throw(Exception)*/
  {
    ChannelChunkArray_var match_chunks;

    {
      ReadGuard_ lock(lock_configuration_);
      match_chunks = ReferenceCounting::add_ref(chunks_);
    }

    while(!unmerged.empty())
    {
      unsigned short lang;
      UnmergedValue remove_unmerged_value;

      {
        auto it = unmerged.begin();
        lang = it->first.lang;
        remove_unmerged_value.swap(it->second);
        unmerged.erase(it);
      }

      // it->first.trigger references to it->second.*->word.trigger
      // const std::string& trigger = remove_unmerged_value.begin()->word.trigger;

      // push 
      SoftMatcher_var new_matcher = new SoftMatcher(lang, remove_unmerged_value.begin()->word.trigger);
      const std::string& trigger = new_matcher->get_trigger();

      LexemesPtrVector lexemes;
      SubStringVector parts; // will be used as key in chunk & as key in lexemes cache

      Serialization::get_parts(trigger, parts);

      if (DictionaryMatcher::is_lexemized(trigger.c_str()))
      {
        // parts will be used as key in lexemes_ (it should be cleared at update start)
        add.fill_lexemes(lang, parts, lexemes);
        auto num = 0;
        if(!lexemes.empty())
        {
          for(auto part_it = parts.begin(); part_it != parts.end(); ++part_it, ++num)
          {
            if (Serialization::quoted(trigger.c_str(), num))
            {
              lexemes[num] = nullptr;
            }
          }
        }
      }

      /*
      // check
      {
        const char* key_start = &it->first.trigger[0];
        const char* key_end = &it->first.trigger[it->first.trigger.size()];
        bool assert_tr_found = false;
        for(auto sit = it->second.begin(); sit != it->second.end(); ++sit)
        {
          if(&sit->word.trigger[0] <= key_start && key_end <= &sit->word.trigger[sit->word.trigger.size()])
          {
            assert_tr_found = true;
          }
        }

        assert(assert_tr_found);
      }
      */

      // trigger has moved to SoftMatcher and can't be used after this point

      char trigger_type = new_matcher->trigger_type();
      unsigned int word_num = 0;
      unsigned int word_count = 1;
      if (trigger_type == 'U')
      {
        // for url always it first part
        word_num = 0;
      }
      else if (trigger_type == 'D')
      {
        // for D from first uid
        word_num = 0;
        word_count = parts.size();
      }
      else
      {
        // for keyword select one possible key
        word_num = choose_word_(*match_chunks, parts, lexemes);
      }

      new_matcher->configure_matching_info(parts, lexemes, word_num);

      assert(word_num + word_count <= parts.size());

      for (size_t i = 0; i < word_count; ++i)
      {
        // loop for uid, other cases word_count = 1
        add_with_existing_matcher_(
          *match_chunks,
          remove_unmerged_value, // use only channel_id, channel_trigger_id set
          new_matcher,
          parts[word_num + i]);
      }

      for(auto it_un = remove_unmerged_value.begin(); it_un != remove_unmerged_value.end(); ++it_un)
      {
        added[it_un->channel_id][it_un->word.channel_trigger_id] = new_matcher;
      }

      // update ns here, all matcher are new
      if (non_strict_ && trigger_type != 'D')
      {
        add_ns_(new_matcher, parts[word_num]);
      }
    }
  }

  /*merge 2 containers*/
  void ChannelContainer::merge(
    UpdateContainer& add,
    const ChannelIdToMatchInfo& info,
    bool update_finished,
    ProgressCounter* progress,
    const Generics::Time& master,
    const Generics::Time& first_load_stamp,
    std::ostream* debug)
    /*throw(Exception)*/
  {
    try
    {
      NSTriggerAtom removed_urls;
      NSTriggerAtom removed_page_keywords;
      NSTriggerAtom removed_url_keywords;
      NSTriggerAtom removed_search_keywords;
      UnmergedMap unmerged;
      ChannelIdToTrigers added;
      IdSet updated_uid_channels;

      if(debug)
      {
        trace_update_data_(add, *debug);
      }

      if (progress)
      {
        progress->change_stage(PROGRESS_MERGE1);
      }

      //try to find and merge triggers with matcher from container
      size_t count_channels = add_stage1(add, unmerged, added);

      if (progress)
      {
        progress->set_progess(count_channels);
        progress->change_stage(PROGRESS_PREMERGE);
      }

      // get additional information from update container
      add.prepare_merge(unmerged);

      if (progress)
      {
        progress->set_progess(count_channels);
        progress->change_stage(PROGRESS_MERGE2);
      }

      // create new matchers and add them to container
      add_stage2(add, unmerged, added);

      if (progress)
      {
        progress->set_progess(count_channels);
        progress->change_stage(PROGRESS_PREPARE_UPDATE);
      }

      ChannelChunkArray_var chunks_array;
      ChannelChunkArray_var match_chunks;
      {
        ReadGuard_ lock(lock_configuration_);
        match_chunks = ReferenceCounting::add_ref(chunks_);
      }
      const ExcludeContainerType& removed_channels = add.get_removed();
      IdType channel_id = 0;
      bool reset_stat = false;
      UpdateContainer::Matters& matters_cont = add.get_matters();
      auto matters_it = matters_cont.begin();
      while(!terminated_ && matters_it != matters_cont.end())
      {
        reset_stat = true;
        channel_id = matters_it->first;
        UpdateContainer::MatterItem& item = matters_it->second;
        ChannelUpdateData_var old_data, new_data;
        {//find information about triggers from exising container
          ReadGuard_ lock(lock_update_data_);
          ChannelMap::const_iterator old_it = channel_ids_.find(channel_id);
          if(old_it != channel_ids_.end())
          {
            old_data = old_it->second;
          }
        }
        
        ChannelUpdateData::TriggerItemVector::const_iterator old_item_it;
        for (IdVector::const_iterator it_rm = item.removed.begin();
             it_rm != item.removed.end(); ++it_rm)
        {
          if (*it_rm)
          {
            old_item_it = std::lower_bound(
              old_data->triggers.begin(),
              old_data->triggers.end(),
              *it_rm);
            for(; old_item_it != old_data->triggers.end() &&
                old_item_it->channel_trigger_id == *it_rm;
                ++old_item_it)
            {
              SoftMatcher_var matcher =
                ReferenceCounting::add_ref(old_item_it->matcher);
              char trigger_type = matcher->trigger_type();
              if (trigger_type == 'U')
              {
                remove_action_(
                  *match_chunks,
                  *it_rm,
                  matcher,
                  SoftMatcher::SubHashVector(1, matcher->get_url_prefix()));
              }
              else
              {
                const Lexeme_var& lexem = matcher->matched_lexeme();
                if(lexem.in())
                {
                  remove_action_(
                    *match_chunks,
                    *it_rm,
                    matcher,
                    lexem->forms);
                }
                else
                {
                  SubStringVector data;
                  remove_action_(
                    *match_chunks,
                    *it_rm,
                    matcher,
                    Serialization::get_parts(
                      matcher->get_trigger(), data));
                }
              }
            }
          }
          else 
          {//should be trigger_type == 'D'
            old_item_it = old_data->triggers.begin();
            assert(!old_item_it->channel_trigger_id);
            updated_uid_channels.insert(channel_id);
            //mark uids on removing
            add_uids_(*match_chunks, old_item_it->matcher, 0);
          }
        }

        bool remove_channel =
          removed_channels.find(channel_id) != removed_channels.end();
        if(!remove_channel)
        {
          IdMatchers& added_ids = added[channel_id];
          new_data = new ChannelUpdateData;
          ChannelIdToMatchInfo::const_iterator it_info = info.find(channel_id);
          if(it_info != info.end())
          {
            new_data->lang = it_info->second.lang;
            new_data->country = it_info->second.country;
            new_data->channel_size = it_info->second.channel_size;
            new_data->stamp = it_info->second.stamp;
            new_data->db_stamp = it_info->second.db_stamp;
          }
          auto add_it = added_ids.begin();
          if(old_data)
          {
            assert(old_data->triggers.size() + added_ids.size() >= item.removed.size());
            new_data->triggers.reserve(
              old_data->triggers.size() - item.removed.size() + added_ids.size());
            old_item_it = old_data->triggers.begin();
            while(old_item_it != old_data->triggers.end())
            {
              if(add_it != added_ids.end() &&
                 add_it->first <= old_item_it->channel_trigger_id)
              {
                new_data->triggers.push_back(ChannelUpdateData::TriggerItem(
                  add_it->first, add_it->second));
                ++add_it;
              }
              else
              {
                if(!std::binary_search(
                    item.removed.begin(),
                    item.removed.end(),
                    old_item_it->channel_trigger_id))
                {
                  new_data->triggers.push_back(*old_item_it);
                }
                ++old_item_it;
              }
            }
          }
          else
          {
            new_data->triggers.reserve(added_ids.size());
          }
          for(; add_it != added_ids.end(); ++add_it)
          {
            new_data->triggers.push_back(ChannelUpdateData::TriggerItem(
              add_it->first, add_it->second));
          }
          old_data = new_data;
        }
        {
          WriteGuard_ lock(lock_update_data_);
          if(remove_channel)
          {
            channel_ids_.erase(channel_id);
          }
          else
          {
            channel_ids_[channel_id].swap(old_data);
          }
        }
        matters_cont.erase(matters_it++); 
        if (progress)
        {
          progress->set_progess(1);
        }
        //new_data = 0;
      }
      if (progress)
      {
        progress->change_stage(PROGRESS_APPLY_UPDATE);
        progress->set_relative_progress(count_channels, count_chunks_);
      }

      ChannelMatchInfo_var info_res;
      if(update_finished)
      {
        info_res = new ChannelMatchInfo(info);
      }
      else
      {
        info_res = ReferenceCounting::add_ref(get_rules_());
      }

      for(unsigned int i = 0; i < count_chunks_; i++)
      {
        chunks_array = new ChannelChunkArray;
        chunks_array->resize(count_chunks_);
        for(unsigned int j = 0; j < count_chunks_; j++)
        {
          if(i == j)
          {
            (*chunks_array)[j] = new ChannelChunk(info_res);
            (*match_chunks)[i]->apply_update(
              *((*chunks_array)[i]),
              updated_uid_channels,
              non_strict_ ? &removed_urls : nullptr,
              non_strict_ ? &removed_page_keywords : nullptr,
              non_strict_ ? &removed_url_keywords : nullptr,
              non_strict_ ? &removed_search_keywords : nullptr);
          }
          else
          {
            (*chunks_array)[j] = ReferenceCounting::add_ref((*match_chunks)[j]);
          }
        }
        match_chunks = chunks_array;
        {
          WriteGuard_ lock(lock_configuration_);
          chunks_.swap(chunks_array); // destroy chunk outside lock
        }
        if (progress)
        {
          progress->set_progess(1);
        }
      }
      if (progress)
      {
        progress->change_stage(PROGRESS_REMOVE_NON_STRICT);
      }

      //Remove non strict here
      if (non_strict_)
      {
        remove_non_strict_(
          removed_urls,
          removed_page_keywords,
          removed_url_keywords,
          removed_search_keywords);
      }

      if(reset_stat)
      {
        Sync::PosixGuard lock(lock_statistic_);
        stats_.params[ChannelServerStats::KW_COUNT] = 0;
        stats_.params[ChannelServerStats::URL_COUNT] = 0;
        stats_.params[ChannelServerStats::UID_COUNT] = 0;
      }
      if(update_finished)
      {
        WriteGuard_ lock(lock_update_data_);
        if(master_ != master)
        {
          master_ = master;
          Generics::Time cur_date = Generics::Time::get_time_of_day();
          if(first_master_ != Generics::Time::ZERO)
          {
            max_update_ = std::max(
              max_update_,
              cur_date - start_update_);
          }
          if(first_master_ != first_load_stamp)
          {
            first_master_ = first_load_stamp;
          }
          start_update_ = cur_date;
        }
      }
      if (progress)
      {
        progress->set_progess(count_channels);
      }

    }
    catch(const Exception& e)
    {
      clean_failed_merge_();
      Stream::Error ostr;
      ostr << __func__ << ": Exceptionon: " << e.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& e)
    {
      clean_failed_merge_();
      Stream::Error ostr;
      ostr << __func__ << ": eh:Exceptionon: " << e.what();
      throw Exception(ostr);
    }
  }

  void ChannelContainer::clean_failed_merge_() noexcept
  {
    ChannelChunkArray_var match_chunks;
    {
      ReadGuard_ lock(lock_configuration_);
      match_chunks = ReferenceCounting::add_ref(chunks_);
    }
    for(unsigned int i = 0; i < count_chunks_; i++)
    {
      (*match_chunks)[i]->cancel_update();
    }
  }

  void ChannelContainer::set_ccg(CCGMap* map) noexcept
  {
    WriteGuard_ lock(lock_update_data_);
    ccgs_ =  ReferenceCounting::add_ref(map);
  }


  void ChannelContainer::match_uid_(
    const Generics::Uuid& uid, 
    const ChannelChunkArray& array,
    TriggerMatchRes& res)
    /*throw(Exception)*/
  {
    if(!uid.is_null())
    {
      ChannelChunk* chunk = array[calc_chunk_num_(uid.hash(), array.size())];
      chunk->match_uid(uid, res);
    }
  }

  /* match urls for triggers*/
  void ChannelContainer::match_urls_(
    const MatchUrls& urls,
    const ChannelChunkArray& array,
    unsigned int flags,
    TriggerMatchRes& res)
    /*throw(Exception)*/
  {
    try
    {
      for (MatchUrls::const_iterator i = urls.begin();
           i != urls.end(); ++i)
      {
        ChannelChunk* chunk =
          array[calc_chunk_num_i_(String::SubString(i->prefix))];
        chunk->match_url(*i, flags, res);
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << " ChannelContainer::match_urls_: Caught eh::Exception: " <<
        e.what();
      throw Exception(ostr);
    }
  }

  /* match ns urls*/
  void ChannelContainer::match_ns_urls_(
    const MatchUrls& urls,
    const ChannelChunkArray& array,
    unsigned int flags,
    TriggerMatchRes& res)
    /*throw(Exception)*/
  {
    try
    {
      MatchUrls additional_urls;
      {
        ReadGuard_ lock(lock_ns_map_);
        for (MatchUrls::const_reverse_iterator i(urls.rbegin());
             i < urls.rend(); ++i)
        {//
          NSUrlMapType::const_iterator it = ns_url_map_.find(i->prefix);
          if(it != ns_url_map_.end())
          {
            for(NSTriggerAtom::const_iterator it_ns = it->second.begin();
                it_ns != it->second.end(); ++it_ns)
            {
              const String::SubString url_prefix = (*it_ns)->get_url_prefix();
              if(i->prefix.length() + i->postfix.length() < url_prefix.length())
              {
                if(url_prefix.compare(
                    i->prefix.length(),
                    i->postfix.length(),
                    i->postfix, 0, i->postfix.length()) == 0)
                {
                  additional_urls.resize(additional_urls.size() + 1);
                  MatchUrl& url = additional_urls.back();
                  url.prefix = url_prefix.str();
                  url.postfix = "/";
                }
              }
              else
              {
                additional_urls.resize(additional_urls.size() + 1);
                MatchUrl& url = additional_urls.back();
                url.prefix.reserve(url_prefix.length());
                url.postfix.reserve(i->postfix.length());
                url.prefix.append(i->prefix);
                url.prefix.append(
                  i->postfix,
                  0, 
                  url_prefix.length() - i->prefix.size());
                url.postfix.append(
                  i->postfix,
                  url_prefix.length() - i->prefix.size(),
                  std::string::npos);
              }
            }
          }
        }
      }
      match_urls_(urls, array, flags, res);
      match_urls_(additional_urls, array, flags, res);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << " ChannelContainer::match_ns_urls_: Caught eh::Exception: " <<
        e.what();
      throw Exception(ostr);
    }
  }
  
  /* match words for triggers*/
  void ChannelContainer::match_words_(
    const MatchWords& key_words,
    const MatchWords& words,
    const ChannelChunkArray& array,
    const StringVector* exact_words,
    MatchType type,
    unsigned int flags,
    TriggerMatchRes& res,
    MatcherVarsSet* already_matched)
    /*throw(Exception)*/
  {
    static const char* FN = "ChannelContainer::match_words_";
    try
    {
      for (MatchWords::const_iterator i(key_words.begin());
           i != key_words.end(); ++i)
      {
        const MatchWords::key_type& current = *i;
        const ChannelChunk* chunk =
          array[calc_chunk_num_(i->hash(), array.size())];
        chunk->match_words(
          current,
          words, exact_words, type, flags, res, already_matched);
      }
    }
    catch(eh::Exception& e)
    {
      Stream::Error ostr;
      ostr <<  FN << ": Caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  /* match non strict words for triggers*/
  void ChannelContainer::match_ns_words_(
    const MatchWords& words,
    const ChannelChunkArray& array,
    MatchType type,
    unsigned int flags,
    TriggerMatchRes& res)
    /*throw(Exception)*/
  {
    static const char* FN = "ChannelContainer::match_ns_words_";
    try
    {
      MatchWords key_words = words;
      MatcherVarsSet matched;
      for (MatchWords::const_iterator it(words.begin());
        it != words.end(); ++it)
      {
        ReadGuard_ lock(lock_ns_map_);
        NSTriggerMapType::const_iterator ns_it =
          ns_trigger_map_.find(it->text());
        if(ns_it != ns_trigger_map_.end())
        {
          for(NSTriggerAtom::const_iterator m_it = ns_it->second.begin();
              m_it != ns_it->second.end(); ++m_it)
          {
            matched.insert(*m_it);
            const Lexeme_var& lexeme = (*m_it)->matched_lexeme();
            if(lexeme.in())
            {//any form from matched lexem is siutable
              key_words.insert(lexeme->forms[0]);
            }
            else
            {//try all parts like keys
              SubStringVector parts;
              Serialization::get_parts((*m_it)->get_trigger(), parts);
              key_words.insert(parts.begin(), parts.end());
            }
          }
        }
      }
      match_words_(key_words, words, array, nullptr, type, flags, res, &matched);
    }
    catch(eh::Exception& e)
    {
      Stream::Error ostr;
      ostr <<  FN << ": Caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  /*match channels for trigger */
  void ChannelContainer::match(
    const MatchUrls& url_words,
    const MatchUrls& additional_url_words,
    const MatchWords match_words[CT_MAX],
    const MatchWords& additional_url_keywords,
    const StringVector& exact_words,
    const Generics::Uuid& uid, 
    unsigned int flags,
    TriggerMatchRes& res)
    /*throw(Exception)*/
  {
    try
    {
      ChannelChunkArray_var chunk_array;
      {
        ReadGuard_ lock(lock_configuration_);
        chunk_array = ReferenceCounting::add_ref(chunks_);
      }

      if(flags & MF_NONSTRICTURL)
      {
        match_ns_urls_(
          url_words,
          *chunk_array,
          flags | MF_BLACK_LIST,
          res);

        match_ns_urls_(
          additional_url_words,
          *chunk_array,
          flags | MF_BLACK_LIST,
          res);
      }
      else
      {
        match_urls_(
          url_words,
          *chunk_array,
          flags | MF_BLACK_LIST,
          res);

        match_urls_(
          additional_url_words,
          *chunk_array,
          flags,
          res);

        match_uid_(uid, *chunk_array, res);
      }

      if(flags & MF_NONSTRICTKW)
      {
        match_ns_words_(
          match_words[CT_PAGE],
          *chunk_array,
          CT_PAGE,
          flags | MF_BLACK_LIST,
          res);

        match_ns_words_(
          match_words[CT_URL_KEYWORDS],
          *chunk_array,
          CT_URL_KEYWORDS,
          flags | MF_BLACK_LIST,
          res);

        match_ns_words_(
          additional_url_keywords,
          *chunk_array,
          CT_URL_KEYWORDS,
          flags | MF_BLACK_LIST,
          res);

        match_ns_words_(
          match_words[CT_SEARCH],
          *chunk_array,
          CT_SEARCH,
          flags | MF_BLACK_LIST,
          res);
      }
      else
      {
        match_words_(
          match_words[CT_PAGE],
          match_words[CT_PAGE],
          *chunk_array,
          nullptr,
          CT_PAGE,
          flags | MF_BLACK_LIST,
          res);

        match_words_(
          match_words[CT_URL_KEYWORDS],
          match_words[CT_URL_KEYWORDS],
          *chunk_array,
          nullptr,
          CT_URL_KEYWORDS,
          flags | MF_BLACK_LIST,
          res);

        match_words_(
          additional_url_keywords,
          additional_url_keywords,
          *chunk_array,
          nullptr,
          CT_URL_KEYWORDS,
          flags,
          res);

        match_words_(
          match_words[CT_SEARCH],
          match_words[CT_SEARCH],
          *chunk_array,
          &exact_words,
          CT_SEARCH,
          flags | MF_BLACK_LIST,
          res);
      }

      __gnu_cxx::__atomic_add(&queries_, 1);
    }
    catch(const Exception& e)
    {
      Stream::Error ostr;
      ostr << "ChannelContainer::match: Caught Exception: " <<
        e.what();
      __gnu_cxx::__atomic_add(&exceptions_, 1);
      throw Exception(ostr);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "ChannelContainer::match: Caught eh::Exception: " <<
        e.what();
      __gnu_cxx::__atomic_add(&exceptions_, 1);
      throw Exception(ostr);
    }
  }

  /*get trigger lists content by id*/
  void ChannelContainer::fill(ChannelMap& buffer) const
    /*throw(Exception)*/
  {
    static const char* FN = "ChannelContainer::fill";
    try
    {
      ReadGuard_ lock(lock_update_data_);
      for(ChannelMap::iterator it = buffer.begin();
          it != buffer.end(); ++it)
      {
        ChannelMap::const_iterator it2 = channel_ids_.find(it->first);
        if (it2 != channel_ids_.end())
        {
          it->second = it2->second;
        }
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FN << ": eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void ChannelContainer::get_ccg_update(
    const Generics::Time& old_master,
    unsigned long start,
    unsigned long limit,
    const std::set<unsigned long>& get_id,
    std::vector<CCGKeyword_var>& buffer,
    std::map<unsigned long, Generics::Time>& deleted,
    bool use_only_list) const
    /*throw(Exception)*/
  {
    static const char* FN = "ChannelContainer::get_pos_ccg_update";
    try
    {
      ChannelMatchInfo_var info_ptr = get_active();
      CCGMap_var ccg_info_ptr = get_ccg();
      if(use_only_list)
      {
        ChannelMatchInfo::const_iterator ch_iter = info_ptr->lower_bound(start);
        auto id_it = get_id.begin();
        while(id_it != get_id.end() && ch_iter != info_ptr->end()
              && buffer.size() < limit)
        {
          if(*id_it == ch_iter->first)
          {
            for(std::vector<CCGKeyword_var>::const_iterator it =
                ch_iter->second.ccg_keywords.begin();
                it != ch_iter->second.ccg_keywords.end(); ++it)
            {
              buffer.push_back(*it);
            }
            ++id_it;
            ++ch_iter;
          }
          else if(*id_it < ch_iter->first)
          {
            ++id_it;
          }
          else //(*id_it > ch_iter->first)
          {
            ++ch_iter;
          }
        }
      }
      else
      {
        for (ChannelMatchInfo::const_iterator ch_iter(info_ptr->lower_bound(start));
          ch_iter != info_ptr->end() && buffer.size() < limit; ++ch_iter)
        {
          bool push = (get_id.find(ch_iter->first) != get_id.end());
          for(std::vector<CCGKeyword_var>::const_iterator it =
              ch_iter->second.ccg_keywords.begin();
              it != ch_iter->second.ccg_keywords.end(); ++it)
          {
            if(push || (*it)->timestamp > old_master)
            {
              buffer.push_back(*it);
            }
          }
        }
      }
      if(buffer.empty())
      {
        fill_deleted(*ccg_info_ptr, old_master, deleted);
      }
    }
    catch(const ChannelChunk::Exception& e)
    {
      Stream::Error ostr;
      ostr << FN << ": caught ChannelChunk::Exception: " <<
        e.what();
      throw Exception(ostr);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << ": caught eh::Exception: " << e.what();
      throw Exception(ostr);
    }
  }

  void ChannelContainer::get_stats(ChannelServerStats& stats)
    noexcept
  {
    Sync::PosixGuard lock(lock_statistic_);
    if(!stats_.params[ChannelServerStats::KW_COUNT] &&
       !stats_.params[ChannelServerStats::URL_COUNT])
    {
      stats_.params[ChannelServerStats::KW_ID_COUNT] = 0;
      stats_.params[ChannelServerStats::URL_ID_COUNT] = 0;
      stats_.params[ChannelServerStats::UID_ID_COUNT] = 0;
      ChannelChunkArray_var chunk_array;
      {
        ReadGuard_ lock(lock_configuration_);
        chunk_array = ReferenceCounting::add_ref(chunks_);
      }
      for(unsigned int j = 0; j < count_chunks_; j++)
      {
        (*chunk_array)[j]->accamulate_statistic(stats_);
      }
      chunk_array.reset();
      {
        ReadGuard_ lock(lock_ns_map_);
        stats_.params[ChannelServerStats::NS_KW_COUNT] = ns_trigger_map_.size();
        stats_.params[ChannelServerStats::NS_URL_COUNT] = ns_url_map_.size();
      }
      stats_.params[ChannelServerStats::MATCHINGS_COUNT] = queries_;
      stats_.params[ChannelServerStats::EXCEPTIONS_COUNT] = exceptions_;
    }
    stats = stats_;
  }

  void ChannelContainer::match_parse_urls(
    const String::SubString& refer,
    const String::SubString& refer_words,
    const std::set<unsigned short>& allow_ports,
    bool soft_matching,
    MatchUrls& url_words,
    MatchWords& url_keywords,
    Logging::Logger* logger,
    const Language::Segmentor::SegmentorInterface* /*segmentor*/)
    /*throw(eh::Exception)*/
  {
    try
    {
      static const String::AsciiStringManip::SepNL sep_nl;
      String::SubString referer(refer);
      String::StringManip::trim(referer);

      if(!referer.empty())
      {
        String::StringManip::Splitter<decltype(sep_nl)> splitter(referer);
        String::SubString ref;
        while(splitter.get_token(ref))
        {
          ChannelContainer::match_parse_refer(
            ref,
            allow_ports,
            soft_matching,
            url_words,
            logger);
        }

        parse_keywords(
          refer_words, // ex: HTTP::keywords_from_http_address(referer)
          url_keywords,
          PM_NO_SIMPLIFY, //normalize_phrase called on Fronend side now (ADSC-9812)
          &sep_nl,//break by '\n'
          soft_matching ? 1 : Commons::DEFAULT_MAX_HARD_WORD_SEQ,
          0,//exact match
          0);//segmentor used in Frontends for extracting keywords (ADSC-9812)
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << e.what();
      logger->log(
          ostr.str(),
          Logging::Logger::TRACE,
          ASPECT);
      url_words.clear();
    }

    StringVector exact_phrases;
  }

  void ChannelContainer::match_parse_refer(
    const String::SubString& lower_refer,
    const std::set<unsigned short>& /*allow_ports*/,
    bool soft_matching,
    MatchUrls& match_words,
    Logging::Logger* /*logger*/)
    /*throw(eh::Exception)*/
  {
    try
    {
      std::string lower_path;
      std::string lower_query;
      const char* host;
      unsigned long host_len;
      HTTP::HTTPAddress url(lower_refer);//validation should be made by Frontend
      /*
      if(!url.is_default_port() &&
         allow_ports.find(url.port_number()) == allow_ports.end())
      {
        return;
      }
      */
      url.path().assign_to(lower_path);
      url.query().assign_to(lower_query);
      host = url.host().data();
      host_len = url.host().size();

      try
      {
        String::AsciiStringManip::to_lower(lower_path);
        String::AsciiStringManip::to_lower(lower_query);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << "Caught exception on executing to_lower for: " << lower_path
          << "?" << lower_query << ". : " << e.what();
        throw Exception(err);
      }
      const char* path = lower_path.c_str();
      const char* query = lower_query.c_str();
      unsigned long path_len = lower_path.size();
      unsigned long query_len = lower_query.size();
      while(host_len && *(host + host_len - 1) == '.') host_len--;

      if(host_len == 0)
      {
        return;
      }

      bool added_www = false;
      const char* pos;
      size_t count, reserve, path_depth = 1;
      size_t current_sub_pos = 0, count_rep, prev_sub_pos, start_index;
      MatchUrls::iterator current, from;
      MatchUrls::reverse_iterator rcurrent, rfrom;

      count = std::count(host, host + host_len, '.');
      reserve = count + 2;
      count = std::min(
        static_cast<unsigned long>(
          std::count(path, path + path_len, '/')),
        PATH_DEPTH);
      reserve += count * 2 - 1;
      match_words.reserve(reserve);
      current_sub_pos = host_len;
      do
      {
        pos = (const char*)memrchr(host, '.', current_sub_pos);
        if(!pos)
        {
          current_sub_pos = -1;
        }
        else
        {
          current_sub_pos = pos - host;
        }
        match_words.resize(match_words.size() + 1);
        rcurrent = match_words.rbegin();
        rcurrent->prefix.assign(
          host + current_sub_pos + 1, host_len - current_sub_pos - 1);
        rcurrent->postfix.reserve(path_len + query_len + 1);
        rcurrent->postfix.push_back('/');
      }
      while(pos);

      if(host_len < 5 || memcmp(host, "www.", 4) != 0)
      {
        added_www = true;
        //add www.
        match_words.resize(match_words.size() + 1);
        rcurrent = match_words.rbegin();
        rfrom = match_words.rbegin() + 1;
        rcurrent->prefix.reserve(rfrom->prefix.size() + 4);
        rcurrent->prefix.append("www.", 4);
        rcurrent->prefix.append(rfrom->prefix);
        rcurrent->postfix = rfrom->postfix;
      }

      current_sub_pos = 0;

      if(soft_matching)
      {
        count_rep = match_words.size();//add path to all domains
        for (current = match_words.begin();
          current != match_words.end(); ++current)
        {
          current->postfix.reserve(path_len + query_len + 1);
          current->postfix.assign(path, path_len);
        }
      }
      else
      {
        count_rep = 2;//add paths only to full domain name
        //path for full domain name will be set below
        rcurrent = match_words.rbegin();
        rcurrent->postfix.clear();
        ++rcurrent;
        rcurrent->postfix.clear();
        start_index = match_words.size() - count_rep;
        prev_sub_pos = 0;
        do
        {
          pos = (const char*)memchr(
            path + prev_sub_pos + 1, '/', path_len - prev_sub_pos - 1);
          if(!pos || path_depth >= PATH_DEPTH)
          {
            current = match_words.begin() + start_index;
            while(current != match_words.end())
            {
              current->postfix.append(
                path + prev_sub_pos,
                path_len - prev_sub_pos);
              ++current;
            }
          }
          else
          {
            match_words.resize(match_words.size() + count_rep);
            from = match_words.begin() + start_index;
            current = from + count_rep;
            current_sub_pos = pos - path;
            while(current != match_words.end())
            {
              from->postfix.append(
                path + prev_sub_pos,
                current_sub_pos - prev_sub_pos);
              current->prefix.reserve(
                from->prefix.size() + current_sub_pos - prev_sub_pos);
              current->prefix.append(from->prefix);
              current->prefix.append(
                path + prev_sub_pos,
                current_sub_pos - prev_sub_pos);
              ++from;
              ++current;
            }
            start_index += count_rep;
            prev_sub_pos = current_sub_pos;
          }
          path_depth++;
        }
        while(pos && path_depth <= PATH_DEPTH);
      }

      if(query_len)
      {
        rcurrent = match_words.rbegin();
        for(size_t i_cur = 0; i_cur < count_rep; i_cur++)
        {
          rcurrent->postfix.push_back('?');
          rcurrent->postfix.append(query, query_len);
          ++rcurrent;
        }
      }

      //hard matching of urls
      if(!soft_matching)
      {
        rcurrent = match_words.rbegin();
        if(added_www)
        {
          ++rcurrent;
        }

        rcurrent->postfix.push_back('"');
      }
      /*
      std::cout << "reserve = " << reserve << ", size = " << match_words.size() << std::endl;
      for (current = match_words.begin(); current!=match_words.end(); ++current)
      {
        std::cout << "\'" << current->prefix << '+'
          << current->postfix << "\' "  <<  std::endl;
      }*/
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": exception on parsing. : " << e.what();
      throw Exception(ostr);
    }
  }

  ProgressCounter::ProgressCounter(size_t stages, size_t total) noexcept
    : progress_(stages),
      relative_progress_(0),
      relative_div_(0),
      stage_(0),
      count_stages_(stages),
      total_(total)
  {
  }

  size_t ProgressCounter::get_progress() noexcept
  {
    ReadGuard_ guard(lock_progress_);
    if (relative_div_)
    {
      return relative_progress_;
    }
    return progress_[stage_];
  }

  size_t ProgressCounter::get_total() noexcept
  {
    return total_;
  }

  size_t ProgressCounter::get_stage() noexcept
  {
    ReadGuard_ guard(lock_progress_);
    return stage_;
  }

  void ProgressCounter::set_progess(size_t inc) noexcept
  {
    WriteGuard_ guard(lock_progress_);
    if (relative_div_)
    {
      relative_progress_ += inc;
    }
    else
    {
      progress_[stage_] += inc;
    }
  }

  void ProgressCounter::set_relative_progress(
    size_t absolute_value,
    size_t div)
    noexcept
  {
    WriteGuard_ guard(lock_progress_);
    change_stage_(stage_);
    absolute_value_ = absolute_value;
    relative_progress_ = 0;
    relative_div_ = div;
  }

  void ProgressCounter::change_stage_(size_t stage) noexcept
  {
    if (relative_div_)
    {
      progress_[stage_] += absolute_value_ * relative_progress_ / relative_div_;
      absolute_value_ = 0;
      relative_div_ = 0;
      relative_progress_ = 0;
    }
    stage_ = stage;
  }

  void ProgressCounter::change_stage(size_t stage) noexcept
  {
    WriteGuard_ guard(lock_progress_);
    change_stage_(stage);
  }

  size_t ProgressCounter::get_sum_progress() noexcept
  {
    ReadGuard_ guard(lock_progress_);
    size_t res = 0;
    for(size_t i = 0; i < count_stages_; ++i)
    {
      size_t cur = progress_[i];
      if (i == stage_ && relative_div_)
      {
        cur += absolute_value_ * relative_progress_ / relative_div_;
      }
      res += cur;
    }
    res /= count_stages_;
    return res;
  }

  void ProgressCounter::reset(size_t stages, size_t total) noexcept
  {
    WriteGuard_ guard(lock_progress_);
    progress_.resize(stages);
    std::fill(progress_.begin(), progress_.end(), 0);
    relative_progress_ = 0;
    relative_div_ = 0;
    stage_ = 0;
    count_stages_ = stages;
    total_ = total;
  }

}// namespace ChannelSvcs
}

