#pragma once

#include <iostream>
#include <list>
#include <set>
#include <map>
#include <eh/Exception.hpp>
#include <Commons/Algs.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include "ExpressionChannel.hpp"

namespace AdServer::CampaignSvcs
{
  class ExpressionChannelIndex:
    public ReferenceCounting::AtomicImpl
  {
  public:
    typedef std::map<unsigned long, ExpressionChannelBase_var>
      ExpressionChannelBaseMap;

    typedef std::list<ExpressionChannelBase_var>
      ExpressionChannelBaseList;

    struct CheckChannel
    {
      ChannelId channel_id;
      bool cpm_flag;
      ExpressionChannelBase_var channel;

      CheckChannel(ExpressionChannelBase* ch) /*throw(eh::Exception)*/;
    };

  public:
    ExpressionChannelIndex() noexcept;

    void index(const ExpressionChannelHolderMap& channels)
      noexcept;

    unsigned long size() const noexcept;

    unsigned long avg_seq_size_() const noexcept;

    void
    match(
      ChannelIdSet& result_channels,
      const ChannelIdHashSet& channels,
      ChannelIdSet* result_cpm_channels = 0) const /*throw(eh::Exception)*/;

    void match(Generics::MonoSet<ChannelId>& result_channels, const ChannelIdHashSet& channels,
      ChannelIdSet* result_cpm_channels = nullptr) const;

    void
    print(std::ostream& out, const ChannelIdSet* custom_channels = 0, bool expand = false) const
      noexcept;

  protected:
    typedef std::map<ChannelId, const CheckChannel*> CheckChannelMap;

    struct ExpressionChannelMatch;

    typedef ReferenceCounting::NonNullPtr<ExpressionChannelMatch>::Ptr
      ExpressionChannelMatch_var;

    typedef std::unordered_map<
      unsigned long,
      ExpressionChannelMatch_var>
      ExpressionChannelMatchMap;

    struct ExpressionChannelMatch: public ReferenceCounting::DefaultImpl<>
    {
      std::vector<ChannelId> matched_channels;
      std::vector<ChannelId> cpm_matched_channels;
      std::vector<CheckChannel> check_channels;
      ExpressionChannelMatchMap channels_by_simple_channel_id;

    protected:
      virtual
      ~ExpressionChannelMatch() noexcept
      {}
    };

  protected:
    virtual
    ~ExpressionChannelIndex() noexcept
    {}

    static ExpressionChannelMatch&
    init_match_cell_(ExpressionChannelMatchMap& channels, unsigned long channel_id)
      noexcept;

    static void
    make_fast_expression_(ExpressionChannelMatch& channel_match)
      /*throw(eh::Exception)*/;

    template<typename ResultChannels>
    void match_impl_(ResultChannels& result_channels, const ChannelIdHashSet& channels,
      ChannelIdSet* result_cpm_channels) const;

    template<typename ResultChannels>
    static void match_(
      ResultChannels& result_channels,
      ChannelIdSet* result_cpm_channels,
      CheckChannelMap& check_channels,
      const ExpressionChannelMatch& expr_channel_match,
      const ChannelIdHashSet& channels)
      noexcept;

    static bool
    index_(
      ExpressionChannelBaseList& check_channels,
      ExpressionChannelMatchMap& channels_by_simple_channel_id,
      const ExpressionChannelBaseMap& expr_channels,
      unsigned long depth)
      noexcept;

    bool
    top_level_index_(
      ExpressionChannelMatchMap& channels_by_simple_channel_id,
      const ExpressionChannelBaseMap& expr_channels)
      noexcept;

    void
    sub_index_(ExpressionChannelMatch& index_node, unsigned long depth)
      noexcept;

    // cpu usage optimization (isn't required, but simplify indexing)
    void index_simple_channels_(
      ExpressionChannelBaseMap& expr_channels,
      const ExpressionChannelHolderMap& channels)
      noexcept;

    static bool
    reduce_channel_(
      ChannelIdSet& used_simple_channels,
      const ExpressionChannelBase* holder,
      const ChannelIdSet& indexed_simple_channels)
      noexcept;

    static bool
    reduce_channel_(
      ChannelIdSet& used_simple_channels,
      const ExpressionChannel::Expression& expr,
      const ChannelIdSet& indexed_simple_channels)
      noexcept;

    static ExpressionChannelBase_var
    create_reduced_channel_(
      ExpressionChannelBase* channel, // const
      const ChannelId indexed_simple_channel)
      noexcept;

    static bool
    create_reduced_expression_(
      ExpressionChannel::Expression& expr,
      const ChannelId indexed_simple_channel)
      noexcept;

    static void
    get_required_channels_(ChannelIdSet& simple_channels, const ExpressionChannel::Expression& expr)
      noexcept;

  protected:
    static constexpr bool TRACE_MATCH_ = false;

    ExpressionChannelMatchMap channels_;
  };

  typedef ReferenceCounting::SmartPtr<ExpressionChannelIndex>
    ExpressionChannelIndex_var;

  template<typename ResultChannels>
  void ExpressionChannelIndex::match_(
    ResultChannels& result_channels,
    ChannelIdSet* result_cpm_channels,
    CheckChannelMap& check_channels,
    const ExpressionChannelMatch& expr_channel_match,
    const ChannelIdHashSet& channels)
    noexcept
  {
    if (TRACE_MATCH_)
    {
      std::cout << "> match_()" << std::endl;
      std::cout << "match_(): matched_channels(" <<
        expr_channel_match.matched_channels.size() << "): ";
      Algs::print(std::cout,
        expr_channel_match.matched_channels.begin(),
        expr_channel_match.matched_channels.end());
      std::cout << std::endl;
    }

    result_channels.insert(
      expr_channel_match.matched_channels.begin(),
      expr_channel_match.matched_channels.end());

    if (result_cpm_channels)
    {
      result_cpm_channels->insert(
        expr_channel_match.cpm_matched_channels.begin(),
        expr_channel_match.cpm_matched_channels.end());
    }

    if (TRACE_MATCH_)
    {
      std::cout << "match_(): check_channels(" << expr_channel_match.check_channels.size() << "): ";
      for (auto ech_it = expr_channel_match.check_channels.cbegin();
          ech_it != expr_channel_match.check_channels.cend(); ++ech_it)
      {
        if (ech_it != expr_channel_match.check_channels.begin())
        {
          std::cout << ",";
        }
        std::cout << ech_it->channel_id;
      }
      std::cout << std::endl;
    }

    for (auto ech_it = expr_channel_match.check_channels.cbegin();
        ech_it != expr_channel_match.check_channels.cend(); ++ech_it)
    {
      check_channels.insert(std::make_pair(ech_it->channel_id, &*ech_it));
    }

    for (ExpressionChannelMatchMap::const_iterator sit =
          expr_channel_match.channels_by_simple_channel_id.begin();
        sit != expr_channel_match.channels_by_simple_channel_id.end();
        ++sit)
    {
      if (TRACE_MATCH_)
      {
        std::cout << "match_(): reverse check for simple channel #" << sit->first << std::endl;
      }

      if (channels.find(sit->first) != channels.end())
      {
        if (TRACE_MATCH_)
        {
          std::cout << "match_(): recursive call for channel #" << sit->first << std::endl;
        }

        match_(result_channels, result_cpm_channels, check_channels, *sit->second, channels);
      }
    }

    if (TRACE_MATCH_)
    {
      std::cout << "< match_()" << std::endl;
    }
  }

  template<typename ResultChannels>
  void ExpressionChannelIndex::match_impl_(
    ResultChannels& result_channels,
    const ChannelIdHashSet& channels,
    ChannelIdSet* result_cpm_channels) const
    /*throw(eh::Exception)*/
  {
    CheckChannelMap check_channels;

    for (ChannelIdHashSet::const_iterator ch_it = channels.begin();
      ch_it != channels.end(); ++ch_it)
    {
      ExpressionChannelMatchMap::const_iterator ind_it = channels_.find(*ch_it);
      if (ind_it != channels_.end())
      {
        if (TRACE_MATCH_)
        {
          std::cout << "match_(): first level call for channel #" << *ch_it << std::endl;
        }

        match_(result_channels, result_cpm_channels, check_channels, *ind_it->second, channels);
      }
    }

    if (TRACE_MATCH_)
    {
      std::cout << "match(): result check_channels(" << check_channels.size() << "): ";
      for (CheckChannelMap::const_iterator ech_it = check_channels.begin();
          ech_it != check_channels.end(); ++ech_it)
      {
        if (ech_it != check_channels.begin())
        {
          std::cout << ",";
        }
        std::cout << ech_it->first;
      }
      std::cout << std::endl;
    }

    //unsigned long result_channels_size = result_channels.size();

    for (CheckChannelMap::const_iterator ech_it = check_channels.begin();
        ech_it != check_channels.end(); ++ech_it)
    {
      if (ech_it->second->channel->triggered(&channels, 0, 0))
      {
        result_channels.insert(ech_it->first);

        if (result_cpm_channels && ech_it->second->cpm_flag)
        {
          result_cpm_channels->insert(ech_it->first);
        }
      }
    }

    /*
    std::cout << "check_channels.size() = " << check_channels.size() << ", matched = " <<
      (result_channels.size() - result_channels_size) << std::endl;
    */
  }
}
