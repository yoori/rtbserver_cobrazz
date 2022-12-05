
#include <ChannelSvcs/ChannelCommons/Serialization.hpp>
#include "UpdateContainer.hpp"

#include <string>
#include <algorithm>

namespace AdServer
{
namespace ChannelSvcs
{
  namespace
  {
    std::string get_lang_str(unsigned short lang) noexcept
    {
      std::string res;
      res.reserve(2);
      res.push_back(static_cast<char>(lang >> 8));
      res.push_back(static_cast<char>(lang & 0xFF));
      return res;
    }

    unsigned short get_lang_from_str(const std::string& lang) noexcept
    {
      if(lang.size() <  2)
      {
        return 0;
      }
      return (static_cast<unsigned short>(lang[0]) << 8)
        + static_cast<unsigned short>(lang[1]);
    }

  }

  /*
   * begin
   * UpdateContainer implementation
   *
   * */

  UpdateContainer::UpdateContainer(
    ChannelContainer* cont,
    DictionaryMatcher* dict)
    noexcept
    : ChannelContainerBase(),
      helper_(cont),
      dict_matcher_(dict)
  {
  }

  void UpdateContainer::cleanup_() noexcept
  {
    new_channels_.clear();
    updated_channels_.clear();
    removed_channels_.clear();
    matters_.clear();
    lexemes_.clear();
  }

  size_t UpdateContainer::check_actual(ChannelIdToMatchInfo& info)
    noexcept
  {
    size_t mem_size = 0;
    cleanup_();
    helper_->check_actual(
      info, new_channels_, updated_channels_, removed_channels_);
    for(ExcludeContainerType::const_iterator it = removed_channels_.begin();
        it != removed_channels_.end(); ++it)
    {
      ChannelUpdateData_var data_ptr = helper_->get_channel(*it);
      if(data_ptr)
      {
        const ChannelUpdateData::TriggerItemVector& old_triggers =
          data_ptr->triggers;
        MatterItem& item = matters_[*it];
        item.removed.insert(
          item.removed.end(), old_triggers.begin(), old_triggers.end());
        mem_size +=
          (sizeof(IdType) + STRING_ADAPTER_SIZE + TRIGGER_ATOM_SIZE) *
          item.removed.size();
      }
    }
    return mem_size;
  }

  ChannelIdToMatchInfo_var UpdateContainer::get_old_info() noexcept
  {
    ChannelMatchInfo_var old_info = helper_->get_active();
    ChannelIdToMatchInfo_var res = new ChannelIdToMatchInfo;
    for(auto it = old_info->begin(); it != old_info->end(); ++it)
    {
      ChannelUpdateData_var data_ptr = helper_->get_channel(it->first);
      if(data_ptr)
      {//should be always true
        MatchInfo& info = (*res)[it->first];
        info.channel = it->second;
        info.lang = data_ptr->lang;
        info.country = data_ptr->country;
        info.channel_size = data_ptr->channel_size;
        info.stamp = data_ptr->stamp;
        info.db_stamp = data_ptr->db_stamp;
      }
    }
    return res;
  }

  void UpdateContainer::prepare_merge(const UnmergedMap& unmerged) /*throw(Exception)*/
  {
    if (!dict_matcher_)
    {
      return;
    }

    try
    {
      // TODO: think about time of life this cache
      lexemes_.clear();

      // fill lexemes words
      unsigned short lang = 0;
      auto it = unmerged.begin();
      bool ask_lexemes = false, have_unprocesed = false;
      while(true)
      {
        if (ask_lexemes)
        {
          auto lex_lang_it = lexemes_.find(lang);
          if (lex_lang_it != lexemes_.end())
          {
            std::string lang_str;
            lang_str = get_lang_str(lang); 
            dict_matcher_->get_lexemes(lang_str.c_str(), lex_lang_it->second);
          }
          ask_lexemes = false;
          have_unprocesed = false;
        }

        if (it == unmerged.end())
        {
          if (have_unprocesed)
          {
            ask_lexemes = true;
          }
          else
          {
            break;
          }
        }
        else
        {
          if (it->first.lang != lang)
          {
            if (have_unprocesed)
            {
              ask_lexemes = true;
            }
            else
            {
              lang = it->first.lang;
            }
          }

          if (it->first.lang == lang)
          {
            if (DictionaryMatcher::is_lexemized(it->first.trigger.c_str()))
            {
              // need lexemes only for unprocessed words
              SubStringVector parts;
              Serialization::get_parts(it->first.trigger, parts);
              for(auto part_it = parts.begin();
                part_it != parts.end(); ++part_it)
              {
                lexemes_[lang].insert(std::make_pair(*part_it, Lexeme_var()));
              }
              have_unprocesed = true;
            }
            ++it;
          }
        }
      }
    }
    catch(const DictionaryMatcher::Exception& e)
    {
      Stream::Error err;
      err << __func__ << ": DictionaryMatcher::Exception: " << e.what();
      throw Exception(err);
    }
  }
  
  void UpdateContainer::fill_lexemes(
    unsigned short lang,
    const SubStringVector& parts,
    LexemesPtrVector& lexemes) const
    noexcept
  {
    auto lex_lang_it = lexemes_.find(lang);
    if (lex_lang_it != lexemes_.end())
    {
      for(auto index = 0U; index < parts.size(); ++index)
      {
        auto lex_it = lex_lang_it->second.find(parts[index]);

        if (lex_it != lex_lang_it->second.end())
        {
          if (lexemes.size() < parts.size())
          {
            lexemes.resize(parts.size());
          }
          lexemes[index] = lex_it->second;
        }
      }
    }
  }

  size_t UpdateContainer::add_trigger(MergeAtom& merge_atom)
    /*throw(TriggerParser::Exception)*/
  {
    try
    {
      MatterItem& list = matters_[merge_atom.id];
      list.lang = get_lang_from_str(merge_atom.lang);
      list.added.swap(merge_atom.soft_words);
      list.uids.swap(merge_atom.uids_trigger);
      return 0;
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << __func__ << ": eh::Exception: " << e.what();
      throw TriggerParser::Exception(ostr);
    }
  }

  size_t UpdateContainer::select_parsed_triggers(
    unsigned long channel_id,
    TriggerList& triggers,
    bool load_once)
    noexcept
  {
    size_t mem_size = 0;
    if(new_channels_.find(channel_id) != new_channels_.end())
    {//need to load all triggers from this channel
      return mem_size;
    }
    ChannelUpdateData_var data_ptr = helper_->get_channel(channel_id);
    if(data_ptr)
    {
      MatterItem& item = matters_[channel_id];
      bool updated =
        (updated_channels_.find(channel_id) != updated_channels_.end());
      const ChannelUpdateData::TriggerItemVector& old_triggers =
        data_ptr->triggers;
      IdSet old_ids;
      old_ids.insert(old_triggers.begin(), old_triggers.end());
      if(!updated && load_once)
      {
        data_ptr.reset();
        for(TriggerList::iterator new_it = triggers.begin();
            new_it != triggers.end();)
        {
          IdSet::iterator id_it = old_ids.find(new_it->channel_trigger_id);
          if(id_it != old_ids.end())
          {
            do
            {
              new_it = triggers.erase(new_it);//don't need parse and load it
            }while(new_it != triggers.end() &&
                   *id_it == new_it->channel_trigger_id);
            old_ids.erase(id_it);
          }
          else
          {
            ++new_it;
          }
        }
      }
      item.removed.insert(
        item.removed.end(), old_ids.begin(), old_ids.end());
      mem_size +=
        (sizeof(IdType) + STRING_ADAPTER_SIZE + TRIGGER_ATOM_SIZE) *
        item.removed.size();
    }
    return mem_size;
  }

}// namespace ChannelSvcs
}

