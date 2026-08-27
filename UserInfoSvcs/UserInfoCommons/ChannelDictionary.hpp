#pragma once

#include <set>
#include <map>
#include <boost/unordered/unordered_flat_map.hpp>
#include <vector>

#include <Generics/MonoAllocator.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Generics/Time.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>

#include "Allocator.hpp"

#include <Generics/TAlloc.hpp>

namespace AdServer::UserInfoSvcs
{
  struct ChannelWeight
  {
    unsigned long channel_id = 0;
    unsigned long weight = 0;
  };

  typedef Generics::MonoUnorderedMap<unsigned long, unsigned long>
    ChannelMatchMap; // channel_id => weight

  typedef std::list<unsigned long, Generics::TAlloc::ThreadPool<unsigned long, 256> >
    ChannelIdList;

  typedef std::list<unsigned long, Generics::TAlloc::ThreadPool<unsigned long, 256> >
    CategoryIdList;

  typedef std::map<
    unsigned long,
    CategoryIdList,
    std::less<unsigned long>,
    Generics::TAlloc::AllocOnly<
      std::map<unsigned long, CategoryIdList>::value_type, 256, true> >
    CategoryMap;

  const char AUDIENCE_CHANNEL = 'A';
  const char URL_CHANNEL = 'U';
  const char PAGE_CHANNEL = 'P';
  const char SEARCH_CHANNEL = 'S';
  const char URL_KEYWORD_CHANNEL = 'R';

  struct PrefChannelInterval
  {
    PrefChannelInterval()
      : max_time_from(0),
        max_time_to(0),
        max_visits(0),
        min_window_size(-1)
    {}

    Generics::Time max_time_from;
    Generics::Time max_time_to;
    unsigned long max_visits;
    unsigned long min_window_size;
  };


  struct ChannelInterval
  {
    ChannelInterval() {}

    ChannelInterval(
      const Generics::Time& time_from_in,
      const Generics::Time& time_to_in,
      const unsigned long min_visits_in,
      const unsigned long weight_in)
      : time_from(time_from_in),
        time_to(time_to_in),
        min_visits(min_visits_in),
        weight(weight_in)
    {}

    Generics::Time time_from;
    Generics::Time time_to;
    unsigned long min_visits;
    unsigned long weight;
  };

  struct ChannelHTCandidateTemplate
  {
    ChannelHTCandidateTemplate(
      unsigned long min_visits_val,
      unsigned long req_visits_val,
      unsigned long weight_val)
      : min_visits(min_visits_val),
        req_visits(req_visits_val),
        weight(weight_val)
    {}

    unsigned long min_visits;
    unsigned long req_visits;
    unsigned long weight;
  };

  typedef std::vector<ChannelHTCandidateTemplate>
    ChannelHTCandidateTemplateArray;

  class ChannelIntervalList:
    protected std::list<ChannelInterval, Generics::TAlloc::AllocOnly<ChannelInterval, 256, true> >
  {
  public:
    typedef std::list<ChannelInterval, Generics::TAlloc::AllocOnly<ChannelInterval, 256, true> >
      BaseChannelIntervalList;
    typedef BaseChannelIntervalList::const_iterator const_iterator;
    typedef BaseChannelIntervalList::const_reverse_iterator const_reverse_iterator;

    const_iterator begin() const
    {
      return BaseChannelIntervalList::begin();
    }

    const_iterator end() const
    {
      return BaseChannelIntervalList::end();
    }

    const_reverse_iterator rbegin() const
    {
      return BaseChannelIntervalList::rbegin();
    }

    const_reverse_iterator rend() const
    {
      return BaseChannelIntervalList::rend();
    }

    bool empty() const
    {
      return BaseChannelIntervalList::empty();
    }

    unsigned long size() const
    {
      return BaseChannelIntervalList::size();
    }

    void insert(const ChannelInterval& val)
    {
      iterator it = BaseChannelIntervalList::begin();

      while (it != BaseChannelIntervalList::end() && it->time_from < val.time_from)
      {
        ++it;
      }

      if (it != BaseChannelIntervalList::end())
      {
        BaseChannelIntervalList::insert(it, val);
      }
      else
      {
        BaseChannelIntervalList::push_back(val);
      }

      if (static_cast<unsigned long>(val.time_to.tv_sec - val.time_from.tv_sec) <
        pref_channel_interval_.min_window_size)
      {
        pref_channel_interval_.min_window_size = val.time_to.tv_sec - val.time_from.tv_sec;
      }

      if (val.min_visits > pref_channel_interval_.max_visits)
      {
        pref_channel_interval_.max_visits = val.min_visits;
      }

      if (val.time_to > pref_channel_interval_.max_time_to)
      {
        pref_channel_interval_.max_time_to = val.time_to;
      }
    }

    unsigned long max_visits() const
    {
      return pref_channel_interval_.max_visits;
    }

    unsigned long min_window_size() const
    {
      return pref_channel_interval_.min_window_size;
    }

    unsigned long max_time_to() const
    {
      return pref_channel_interval_.max_time_to.tv_sec;
    }

  private:
    PrefChannelInterval pref_channel_interval_;
  };

  struct ChannelIntervalsPack: public ReferenceCounting::AtomicImpl
  {
    ChannelIntervalsPack(): contextual(false), zero_channel(false), weight(0)
    {}

    bool contextual;
    bool zero_channel;
    unsigned long weight;

    ChannelIntervalList short_intervals;
    ChannelIntervalList today_long_intervals;
    ChannelIntervalList long_intervals;
    ChannelHTCandidateTemplateArray ht_candidate_templates;

  protected:
    virtual ~ChannelIntervalsPack() noexcept {}
  };

  struct ChannelFeatures
  {
    ChannelFeatures(bool discover_val, unsigned long threshold_val)
      : discover(discover_val),
        threshold(threshold_val)
    {}

    bool discover;
    unsigned long threshold;
  };

  using ChannelIntervalsPack_var = ReferenceCounting::SmartPtr<ChannelIntervalsPack>;

  typedef Generics::NumericHashAdapter<unsigned long> ChannelIdHash;

  typedef Generics::GnuHashTable<
    ChannelIdHash,
    ChannelIntervalsPack_var,
    Generics::TAlloc::AllocOnly<
      Generics::GnuHashTable<ChannelIdHash, ChannelIntervalsPack_var>::value_type, 256, true> >
    ChannelsHashMap;

  using ChannelFeaturesMap = boost::unordered_flat_map<unsigned long, ChannelFeatures>;

  struct ChannelDictionary: public ReferenceCounting::AtomicImpl
  {
    ChannelsHashMap audience_channels;
    ChannelsHashMap page_channels;
    ChannelsHashMap search_channels;
    ChannelsHashMap url_channels;
    ChannelsHashMap url_keyword_channels;

    CategoryMap channel_categories;
    ChannelFeaturesMap channel_features;

  protected:
    virtual
    ~ChannelDictionary() noexcept
    {}
  };

  using ChannelDictionary_var = ReferenceCounting::SmartPtr<ChannelDictionary>;
}
