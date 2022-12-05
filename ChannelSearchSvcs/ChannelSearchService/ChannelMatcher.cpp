#include <Commons/Algs.hpp>
#include "ChannelMatcher.hpp"

namespace Aspect
{
  const char CHANNEL_MATCHER[] = "ChannelMatcher";
}

namespace AdServer
{
namespace ChannelSearchSvcs
{
  ChannelMatcher::ChannelMatcher() noexcept
  {}
  
  ChannelMatcher::Config_var
  ChannelMatcher::config() const /*throw(Exception)*/
  {
    try
    {
      SyncPolicy::ReadGuard lock(lock_);
      return ReferenceCounting::add_ref(config_);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "ChannelMatcher::config(): Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  ChannelMatcher::ExpressionChannelIndex_var
  ChannelMatcher::get_channel_index_() const /*throw(Exception)*/
  {
    try
    {
      SyncPolicy::ReadGuard lock(lock_);
      return ReferenceCounting::add_ref(channel_index_);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "ChannelMatcher::get_channel_index_(): Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  ChannelMatcher::ExpressionChannelSearchIndex_var
  ChannelMatcher::get_channel_search_index_() const /*throw(Exception)*/
  {
    try
    {
      SyncPolicy::ReadGuard lock(lock_);
      return ReferenceCounting::add_ref(channel_search_index_);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "ChannelMatcher::get_channel_search_index_(): Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  void
  ChannelMatcher::config(Config* new_config) /*throw(Exception)*/
  {
    try
    {
      ExpressionChannelIndex_var ch_index =
        new AdServer::CampaignSvcs::ExpressionChannelIndex();
      ch_index->index(new_config->expression_channels);

      ExpressionChannelSearchIndex_var ch_search_index =
        new ExpressionChannelSearchIndex();

      for(ChannelMap::const_iterator ch_it =
            new_config->expression_channels.begin();
          ch_it != new_config->expression_channels.end(); ++ch_it)
      {
        ChannelIdSet used_channels;
        ch_it->second->get_all_channels(used_channels);

        for(ChannelIdSet::const_iterator uch_it =
              used_channels.begin();
            uch_it != used_channels.end(); ++uch_it)
        {
          ch_search_index->channel_search_map[*uch_it].insert(
            ch_it->first);
        }
      }

      Config_var config = ReferenceCounting::add_ref(new_config);

      SyncPolicy::WriteGuard lock(lock_);
      config_.swap(config);
      channel_index_.swap(ch_index);
      channel_search_index_.swap(ch_search_index);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "ChannelMatcher::config(...): Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  void
  ChannelMatcher::match(
    ChannelMatchResultMap& result,
    const ChannelIdSet& history_channels)
    /*throw(Exception)*/
  {
    Config_var channels_config = config();
    if (!channels_config.in())
    {
      throw Exception("there isn't config of service");
    }

    AdServer::CampaignSvcs::ExpressionChannelIndex_var channel_index =
      get_channel_index_();

    for(ChannelIdSet::const_iterator ch_it = history_channels.begin();
        ch_it != history_channels.end(); ++ch_it)
    {
      AdServer::CampaignSvcs::ChannelIdHashSet local_history_channels;
      local_history_channels.insert(*ch_it);
      ChannelIdSet local_result_channels;
      channel_index->match(local_result_channels, local_history_channels);

      for(ChannelIdSet::const_iterator ech_it = local_result_channels.begin();
          ech_it != local_result_channels.end(); ++ech_it)
      {
        ChannelMatchResultMap::iterator res_it = result.find(*ech_it);
        if(res_it != result.end())
        {
          res_it->second.matched_simple_channels.insert(*ch_it);
        }
        else
        {
          ChannelMatchResult& res = result[*ech_it];
          res.matched_simple_channels.insert(*ch_it);

          Config::ChannelTraitsMap::const_iterator chm_it =
            channels_config->expression_channel_traits_map.find(*ech_it);
          if(chm_it != channels_config->expression_channel_traits_map.end())
          {
            res.ccg_ids = chm_it->second.ccg_ids;
          }
        }
      }
    }
  }

  void ChannelMatcher::search(
    ChannelIdSet& result_channels,
    const ChannelIdSet& history_channels)
    /*throw(Exception)*/
  {
    ExpressionChannelSearchIndex_var channel_search_index = get_channel_search_index_();

    if(channel_search_index.in())
    {
      for(ChannelIdSet::const_iterator ch_it = history_channels.begin();
          ch_it != history_channels.end(); ++ch_it)
      {
        ExpressionChannelSearchIndex::ChannelSearchMap::const_iterator chs_it =
          channel_search_index->channel_search_map.find(*ch_it);
        if(chs_it != channel_search_index->channel_search_map.end())
        {
          std::copy(chs_it->second.begin(),
            chs_it->second.end(), std::inserter(result_channels, result_channels.begin()));
        }
      }
    }
  }
}
}
