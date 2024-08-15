#include <functional>
#include <algorithm>

#include <Commons/Algs.hpp>

#include "ExpressionChannelIndex.hpp"

namespace
{
  //const bool TRACE_MATCH_ = true;
  const bool PRINT_CHECK_CHANNELS_ = true;
  const bool TRACE_MATCH_ = false;
}

namespace AdServer
{
namespace CampaignSvcs
{
  namespace
  {
    const String::AsciiStringManip::CharCategory FILTER_CHANNELS("BETA");
  }

  namespace
  {
    struct ExpressionIndexing: public ReferenceCounting::AtomicImpl
    {
      ChannelIdSet used_simple_channels;
        // helper for construct disappeared_simple_channels
      ChannelIdSet indexed_simple_channels;
      ExpressionChannelBase_var channel;

    private:
      virtual ~ExpressionIndexing() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<ExpressionIndexing>
      ExpressionIndexing_var;

    // channel_id => ExpressionIndexing
    struct ExpressionIndexingMap:
      public std::map<unsigned long, ExpressionIndexing_var>,
      public ReferenceCounting::AtomicImpl
    {
    private:
      virtual ~ExpressionIndexingMap() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<ExpressionIndexingMap>
      ExpressionIndexingMap_var;

    struct ChannelPriority
    {
      ChannelPriority(unsigned long use_count_val, unsigned long channel_id_val)
        noexcept
        : use_count(use_count_val),
          channel_id(channel_id_val)
      {}

      bool operator<(const ChannelPriority& right) const noexcept
      {
        return use_count < right.use_count ||
          (use_count == right.use_count && channel_id < right.channel_id);
      }

      const unsigned long use_count;
      const unsigned long channel_id; // for determined indexing output
    };

    // simple channel id => ( expression channel id => ExpressionIndexing )
    typedef std::map<unsigned long, ExpressionIndexingMap_var>
      SimpleChannelIndexingMap;

    struct ExpressionIndexingUsePriority
    {
      ExpressionIndexingUsePriority(
        unsigned long used_simple_channels_count_val,
        unsigned long channel_id_val)
        noexcept
        : used_simple_channels_count(used_simple_channels_count_val),
          channel_id(channel_id_val)
      {}

      bool operator<(const ExpressionIndexingUsePriority& right) const noexcept
      {
        return used_simple_channels_count < right.used_simple_channels_count ||
          (used_simple_channels_count == right.used_simple_channels_count &&
          channel_id < right.channel_id);
      }

      const unsigned long used_simple_channels_count;
      const unsigned long channel_id;
    };

    // used simple channels number => ExpressionIndexing
    struct ExpressionIndexingUseMap:
      public std::map<ExpressionIndexingUsePriority, ExpressionIndexing_var>,
      public ReferenceCounting::AtomicImpl
    {
    private:
      virtual ~ExpressionIndexingUseMap() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<ExpressionIndexingUseMap>
      ExpressionIndexingUseMap_var;

    // ChannelPriority -> ( expression channel id => ExpressionIndexing )
    typedef std::map<ChannelPriority, ExpressionIndexingUseMap_var>
      ChannelPriorityMap;

    void
    collect_expr_simple_channels(
      ChannelIdSet& simple_channels,
      const ExpressionChannel::Expression& expr);

    void
    collect_simple_channels(
      ChannelIdSet& simple_channels,
      const ExpressionChannelBase* channel);

    void
    collect_expr_required_channels(
      ChannelIdSet& simple_channels,
      const ExpressionChannel::Expression& expr)
      noexcept;

    void
    collect_required_channels(
      ChannelIdSet& simple_channels,
      ExpressionChannelBase* channel)
      noexcept;

    template<typename CollectOpType>
    void construct_simple_channel_indexing_map(
      ExpressionChannelIndex::ExpressionChannelBaseList& empty_channels,
      SimpleChannelIndexingMap& expression_indexing_map,
      const ExpressionChannelIndex::ExpressionChannelBaseMap& expression_channels,
      const CollectOpType& collect_op);

    // impl
    void collect_expr_simple_channels(
      ChannelIdSet& simple_channels,
      const ExpressionChannel::Expression& expr)
    {
      if(expr.op == ExpressionChannel::NOP)
      {
        if(expr.channel.in())
        {
          collect_simple_channels(simple_channels, expr.channel);
        }
      }
      else
      {
        ExpressionChannel::Expression::ExpressionList::const_iterator end_it =
          expr.sub_channels.end();

        if(expr.op == ExpressionChannel::AND_NOT)
        {
          --end_it;
        }

        for(ExpressionChannel::Expression::ExpressionList::const_iterator it =
              expr.sub_channels.begin();
            it != end_it; ++it)
        {
          collect_expr_simple_channels(simple_channels, *it);
        }
      }
    }

    void collect_simple_channels(
      ChannelIdSet& simple_channels,
      const ExpressionChannelBase* channel)
    {
      ConstSimpleChannel_var simple_channel =
        channel->simple_channel();
      if(simple_channel.in())
      {
        simple_channels.insert(simple_channel->params().channel_id);
      }
      else
      {
        ConstExpressionChannel_var expr_channel =
          channel->expression_channel();
        if(expr_channel.in())
        {
          collect_expr_simple_channels(
            simple_channels,
            expr_channel->expression());
        }
      }
    }

    void
    collect_expr_required_channels(
      ChannelIdSet& simple_channels,
      const ExpressionChannel::Expression& expr)
      noexcept
    {
      if(expr.op == ExpressionChannel::TRUE)
      {
        return;
      }
      else if(expr.op == ExpressionChannel::NOP)
      {
        collect_required_channels(simple_channels, expr.channel);
      }
      else if(expr.op == ExpressionChannel::OR)
      {
        return;
      }
      else if(expr.op == ExpressionChannel::AND ||
        expr.op == ExpressionChannel::AND_NOT)
      {
        ExpressionChannel::Expression::ExpressionList::const_iterator end_it =
          expr.op == ExpressionChannel::AND_NOT ?
          --expr.sub_channels.end() : expr.sub_channels.end();

        for(ExpressionChannel::Expression::ExpressionList::const_iterator eit =
              expr.sub_channels.begin();
            eit != end_it; ++eit)
        {
          collect_expr_required_channels(simple_channels, *eit);
        }
      }
    }

    void
    collect_required_channels(
      ChannelIdSet& simple_channels,
      ExpressionChannelBase* channel)
      noexcept
    {
      ConstSimpleChannel_var simple_channel = channel->simple_channel();
      if(simple_channel.in())
      {
        unsigned long simple_channel_id = simple_channel->params().channel_id;
        simple_channels.insert(simple_channel_id);
      }
      else
      {
        ConstExpressionChannel_var expression_channel = channel->expression_channel();
        if(expression_channel.in())
        {
          const ExpressionChannel::Expression& expr = expression_channel->expression();
          collect_expr_required_channels(simple_channels, expr);
        }
      }
    }

    template<typename CollectOpType>
    void
    construct_simple_channel_indexing_map(
      ExpressionChannelIndex::ExpressionChannelBaseList& empty_channels,
      SimpleChannelIndexingMap& simple_channel_indexing_map,
      const ExpressionChannelIndex::ExpressionChannelBaseMap& expression_channels,
      const CollectOpType& collect_op)
    {
      for(ExpressionChannelIndex::ExpressionChannelBaseMap::const_iterator ch_it =
            expression_channels.begin();
          ch_it != expression_channels.end(); ++ch_it)
      {
        unsigned long expr_channel_id = ch_it->first;

        ExpressionIndexing_var expr_indexing(new ExpressionIndexing());
        collect_op(expr_indexing->used_simple_channels, ch_it->second);
        if(!expr_indexing->used_simple_channels.empty())
        {
          expr_indexing->channel = ch_it->second;

          for(ChannelIdSet::const_iterator sch_it =
                expr_indexing->used_simple_channels.begin();
              sch_it != expr_indexing->used_simple_channels.end(); ++sch_it)
          {
            SimpleChannelIndexingMap::iterator sch_ind_it =
              simple_channel_indexing_map.find(*sch_it);
            if(sch_ind_it == simple_channel_indexing_map.end())
            {
              ExpressionIndexingMap_var ex_map(new ExpressionIndexingMap());
              ex_map->insert(std::make_pair(expr_channel_id, expr_indexing));
              simple_channel_indexing_map.insert(std::make_pair(*sch_it, ex_map));
            }
            else
            {
              sch_ind_it->second->insert(std::make_pair(expr_channel_id, expr_indexing));
            }
          }
        }
        else
        {
          // for example, simple channel was deleted, but expression still alive
          empty_channels.push_back(ch_it->second);
        }
      }
    }

    void construct_channel_priority_map(
      ChannelPriorityMap& channel_priority_map,
      const SimpleChannelIndexingMap& simple_channel_indexing_map)
    {
      for(SimpleChannelIndexingMap::const_iterator ch_it =
            simple_channel_indexing_map.begin();
          ch_it != simple_channel_indexing_map.end(); ++ch_it)
      {
        ExpressionIndexingUseMap_var expr_indexing_use_map(
          new ExpressionIndexingUseMap());
        for(ExpressionIndexingMap::const_iterator expr_it =
              ch_it->second->begin();
            expr_it != ch_it->second->end(); ++expr_it)
        {
          expr_indexing_use_map->insert(std::make_pair(
            ExpressionIndexingUsePriority(
              expr_it->second->used_simple_channels.size(), expr_it->first),
            expr_it->second));
        }
        channel_priority_map.insert(std::make_pair(
          ChannelPriority(ch_it->second->size(), ch_it->first),
          expr_indexing_use_map));
      }
    }

    void change_expression_channel_priority(
      ChannelPriorityMap& channel_priority_map,
      const SimpleChannelIndexingMap& simple_channel_indexing_map,
      unsigned long simple_channel_id,
      ExpressionIndexing* expr_indexing,
      unsigned long old_used_channels_size,
      unsigned long new_used_channels_size)
    {
      /* change priority of simple channel and expression channel in index maps */
      const unsigned long expr_channel_id = expr_indexing->channel->params().channel_id;

      SimpleChannelIndexingMap::const_iterator sch_ind_it =
        simple_channel_indexing_map.find(simple_channel_id);
      assert(sch_ind_it != simple_channel_indexing_map.end());

      const ExpressionIndexingMap& sub_expr_indexing_map = *(sch_ind_it->second);
      ChannelPriority ch_priority(sub_expr_indexing_map.size(), simple_channel_id);

      ChannelPriorityMap::iterator ch_pr_it = channel_priority_map.find(ch_priority);
      assert(ch_pr_it != channel_priority_map.end());

      ExpressionIndexingUseMap* ei_use_map_holder = ch_pr_it->second;

      int erased = ei_use_map_holder->erase(ExpressionIndexingUsePriority(
        old_used_channels_size, expr_channel_id));
      (void)erased;
      assert(erased);

      if(new_used_channels_size)
      {
        ei_use_map_holder->insert(std::make_pair(
          ExpressionIndexingUsePriority(new_used_channels_size, expr_channel_id),
          ReferenceCounting::add_ref(expr_indexing)));
      }
    }

    void change_channel_priority(
      ChannelPriorityMap& channel_priority_map,
      SimpleChannelIndexingMap& simple_channel_indexing_map,
      unsigned long simple_channel_id,
      ExpressionIndexing* expr_indexing,
      unsigned long old_used_channels_size)
    {
      /* change priority of simple channel and expression channel in index maps */
      const unsigned long expr_channel_id = expr_indexing->channel->params().channel_id;

      SimpleChannelIndexingMap::iterator sch_ind_it =
        simple_channel_indexing_map.find(simple_channel_id);
      assert(sch_ind_it != simple_channel_indexing_map.end());

      ExpressionIndexingMap& sub_expr_indexing_map = *(sch_ind_it->second);
      ChannelPriority old_ch_priority(
        sub_expr_indexing_map.size(), simple_channel_id);

      int expr_erased = sub_expr_indexing_map.erase(expr_channel_id);
      (void)expr_erased;
      assert(expr_erased);

      // change ChannelPriority cell
      ChannelPriorityMap::iterator old_ch_pr_it =
        channel_priority_map.find(old_ch_priority);
      assert(old_ch_pr_it != channel_priority_map.end());
      ExpressionIndexingUseMap_var ei_use_map_holder = old_ch_pr_it->second;
      channel_priority_map.erase(old_ch_pr_it);

      int erased = ei_use_map_holder->erase(ExpressionIndexingUsePriority(
        old_used_channels_size, expr_channel_id));
      (void)erased;
      assert(erased);

      if(!sub_expr_indexing_map.empty())
      {
        // new_ch_priority can't be present (simple channel id is unique)
        channel_priority_map.insert(std::make_pair(
          ChannelPriority(sub_expr_indexing_map.size(), simple_channel_id),
          ei_use_map_holder));
      }
    }

    // new_used_simple_channels_size must be equal to
    // size of used_simple_channels that will be assigned to
    // expr_indexing->used_simple_channels after this call
    void
    update_channel_priority_map(
      ChannelPriorityMap& channel_priority_map,
      SimpleChannelIndexingMap& simple_channel_indexing_map,
      ExpressionIndexing* expr_indexing,
      const ChannelIdSet& disappeared_simple_channels,
      const ChannelIdSet& remain_simple_channels)
    {
      // channel_priority_map:
      //   ChannelPriority(with simple_channel_id) =>
      //   ExpressionIndexingUsePriority(with expr channel_id) => ExpressionIndexing
      //
      // 
      for(ChannelIdSet::const_iterator sch_it =
            disappeared_simple_channels.begin();
          sch_it != disappeared_simple_channels.end();
          ++sch_it)
      {
        change_channel_priority(
          channel_priority_map,
          simple_channel_indexing_map,
          *sch_it,
          expr_indexing,
          expr_indexing->used_simple_channels.size());
      }

      // change channel_priority_map priorities
      for(ChannelIdSet::const_iterator sch_it =
            remain_simple_channels.begin();
          sch_it != remain_simple_channels.end(); ++sch_it)
      {
        change_expression_channel_priority(
          channel_priority_map,
          simple_channel_indexing_map,
          *sch_it,
          expr_indexing,
          expr_indexing->used_simple_channels.size(),
          remain_simple_channels.size());
      }
    }

    void
    collect_channel_ids(
      ChannelIdSet& channel_ids,
      const ExpressionChannelIndex::ExpressionChannelBaseList& expr_channels)
    {
      for(auto ch_it = expr_channels.begin();
          ch_it != expr_channels.end(); ++ch_it)
      {
        channel_ids.insert((*ch_it)->params().channel_id);
      }
    }

    void
    insert_if_not_exists(
      std::vector<ChannelId>& channels,
      ChannelId channel)
      /*throw(eh::Exception)*/
    {
      if (std::find(channels.cbegin(), channels.cend(), channel) == channels.cend())
      {
        channels.push_back(channel);
      }
    }

    bool
    find_required(
      const ChannelIdSet& indexed_channels,
      const ExpressionChannelBase* channel,
      ChannelIdSet& required);

    bool
    find_required(
      const ChannelIdSet& indexed_channels,
      const ExpressionChannel::Expression& expr,
      ChannelIdSet& required)
    {
      if (expr.op == ExpressionChannel::TRUE)
      {
        return true;
      }
      else if (expr.op == ExpressionChannel::NOP)
      {
        return find_required(indexed_channels, expr.channel, required);
      }
      else if(expr.op == ExpressionChannel::OR)
      {
        ChannelIdSet new_required;

        for (auto it = expr.sub_channels.begin(); it != expr.sub_channels.end(); ++it)
        {
          if (!find_required(indexed_channels, *it, new_required))
          {
            return false;
          }
        }

        required.insert(new_required.begin(), new_required.end());
        return true;
      }
      else if (expr.op == ExpressionChannel::AND)
      {
        for (auto it = expr.sub_channels.begin(); it != expr.sub_channels.end(); ++it)
        {
          ChannelIdSet new_required;

          if (find_required(indexed_channels, *it, new_required))
          {
            required.insert(new_required.begin(), new_required.end());
            return true;
          }
        }

        return false;
      }
      else if (expr.op == ExpressionChannel::AND_NOT)
      {
        return find_required(indexed_channels, expr.sub_channels.front(), required);
      }

      return false;
    }

    bool
    find_required(
      const ChannelIdSet& indexed_channels,
      const ExpressionChannelBase* channel,
      ChannelIdSet& required)
    {
      ConstSimpleChannel_var simple_channel = channel->simple_channel();

      if (simple_channel)
      {
        const ChannelId simple_channel_id = simple_channel->params().channel_id;

        if (indexed_channels.find(simple_channel_id) != indexed_channels.end())
        {
          required.insert(simple_channel_id);
          return true;
        }

        return false;
      }

      ConstExpressionChannel_var expression_channel = channel->expression_channel();

      if (expression_channel)
      {
        return find_required(indexed_channels, expression_channel->expression(), required);
      }

      return false;
    }
  }

  ExpressionChannelIndex::CheckChannel::CheckChannel(
    ExpressionChannelBase* ch)
    /*throw(eh::Exception)*/
    : channel(ReferenceCounting::add_ref(ch))
  {
    channel_id = channel->params().channel_id;
    cpm_flag = (channel->has_params() &&
      FILTER_CHANNELS.is_owned(channel->params().type));
  }

  ExpressionChannelIndex::ExpressionChannelIndex() noexcept
  {}

  bool
  ExpressionChannelIndex::top_level_index_(
    ExpressionChannelMatchMap& channels_by_simple_channel_id,
    const ExpressionChannelBaseMap& expr_channels)
    noexcept
  {
    SimpleChannelIndexingMap simple_channel_indexing_map;

    ExpressionChannelIndex::ExpressionChannelBaseList empty_channels;

    construct_simple_channel_indexing_map(
      empty_channels,
      simple_channel_indexing_map,
      expr_channels,
      std::ptr_fun(&collect_simple_channels));

    //assert(empty_channels.empty());

    ChannelPriorityMap channel_priority_map;

    construct_channel_priority_map(
      channel_priority_map,
      simple_channel_indexing_map);

    // priority indexing
    // channel_priority_map:
    //   ChannelPriority => ExpressionIndexingUseMap :
    //   used channel number => ExpressionIndexing
    // simple_channel_indexing_map:
    //   simple channel id => ExpressionIndexingMap :
    //   expr channel id => ExpressionIndexing
    //
    // indexing logic:
    // select "required combinations" with rare usage
    // and link expr channel to all channels from selected combination
    // required combinations: A&B|C&D => [[A,B],[C&D]]
    //
    while(!channel_priority_map.empty())
    {
      // index for channel that rarely used (correct for top level, for next levels - ?)
      ChannelPriorityMap::iterator ch_pr_it = channel_priority_map.begin();

      const unsigned long simple_channel_id = ch_pr_it->first.channel_id;
      assert(!ch_pr_it->second->empty());

      // index Expression with minimal used channels number
      ExpressionIndexingUseMap& expr_indexing_map = *(ch_pr_it->second);
      ExpressionIndexingUseMap::iterator expr_ind_it = expr_indexing_map.begin();
      ExpressionIndexing_var expr_indexing = expr_ind_it->second;

      ExpressionChannelMatch& expr_channel_match = init_match_cell_(
        channels_by_simple_channel_id, simple_channel_id);
      const unsigned long expr_channel_id = expr_indexing->channel->params().channel_id;

      ExpressionChannelBase_var reduced_channel;

      {
        // need to remove indexed channel from expression
        // otherwise indexing can loop
        reduced_channel = create_reduced_channel_(
          expr_indexing->channel,
          simple_channel_id);
      }

      if(reduced_channel)
      {
        expr_channel_match.check_channels.emplace_back(reduced_channel);
      }
      else
      {
        insert_if_not_exists(expr_channel_match.matched_channels, expr_channel_id);

        if (expr_indexing->channel->has_params() &&
            FILTER_CHANNELS.is_owned(expr_indexing->channel->params().type))
        {
          insert_if_not_exists(expr_channel_match.cpm_matched_channels, expr_channel_id);
        }
      }

      ChannelIdSet used_simple_channels;
      ChannelIdSet indexed_simple_channels(expr_indexing->indexed_simple_channels);
      indexed_simple_channels.insert(simple_channel_id);

      reduce_channel_(used_simple_channels,
        expr_indexing->channel,
        indexed_simple_channels);

      if (used_simple_channels.empty())
      {
        ChannelIdSet required{simple_channel_id};

        if (find_required(
              indexed_simple_channels,
              expr_indexing->channel,
              required))
        {
          std::vector<unsigned long> excess_channels;
          std::set_difference(
            indexed_simple_channels.begin(),
            indexed_simple_channels.end(),
            required.begin(),
            required.end(),
            std::back_inserter(excess_channels));

          for (auto i = excess_channels.begin(); i != excess_channels.end(); ++i)
          {
            ExpressionChannelMatch& match = init_match_cell_(channels_by_simple_channel_id, *i);

            for (auto it = match.check_channels.begin(); it != match.check_channels.end(); ++it)
            {
              if (it->channel_id == expr_channel_id)
              {
                match.check_channels.erase(it);
                break;
              }
            }
          }
        }
      }

      // decrease priority for disappeared channels including current
      std::vector<unsigned long> disappeared_simple_channels;
      std::set_difference(
        expr_indexing->used_simple_channels.begin(),
        expr_indexing->used_simple_channels.end(),
        used_simple_channels.begin(),
        used_simple_channels.end(),
        std::back_inserter(disappeared_simple_channels));

      for(auto sch_it = disappeared_simple_channels.begin();
          sch_it != disappeared_simple_channels.end(); ++sch_it)
      {
        change_channel_priority(
          channel_priority_map,
          simple_channel_indexing_map,
          *sch_it,
          expr_indexing,
          expr_indexing->used_simple_channels.size());
      }

      for(ChannelIdSet::const_iterator sch_it = used_simple_channels.begin();
          sch_it != used_simple_channels.end(); ++sch_it)
      {
        change_expression_channel_priority(
          channel_priority_map,
          simple_channel_indexing_map,
          *sch_it,
          expr_indexing,
          expr_indexing->used_simple_channels.size(),
          used_simple_channels.size());
      }

      expr_indexing->used_simple_channels.swap(used_simple_channels);
      expr_indexing->indexed_simple_channels.swap(indexed_simple_channels);
    }

    // in case when we have much then SUBINDEXING_THRESHOLD of equal channels
    // channels will be fully reduced to preicate branch
    //
    for(ExpressionChannelMatchMap::iterator eit =
          channels_by_simple_channel_id.begin();
        eit != channels_by_simple_channel_id.end(); ++eit)
    {
      sub_index_(*eit->second, 1);
    }

    return true;
  }

  void
  ExpressionChannelIndex::make_fast_expression_(
    ExpressionChannelMatch& channel_match)
    /*throw(eh::Exception)*/
  {
    for (auto it = channel_match.check_channels.begin();
         it != channel_match.check_channels.end(); ++it)
    {
      it->channel = new FastExpressionChannel(it->channel);
    }

    for (auto i = channel_match.channels_by_simple_channel_id.begin();
         i != channel_match.channels_by_simple_channel_id.end(); ++i)
    {
      make_fast_expression_(*i->second);
    }
  }

  void
  ExpressionChannelIndex::sub_index_(
    ExpressionChannelMatch& index_node,
    unsigned long depth)
    noexcept
  {
    // minimal number of uses for "required" channel that allow to
    // push it into required sub node
    const unsigned long SUBINDEXING_THRESHOLD = 2;

    ExpressionChannelIndex::ExpressionChannelBaseList new_check_channels;
    SimpleChannelIndexingMap simple_channel_indexing_map;

    // to redesign : we contruct check_channels_map only for call
    // construct_simple_channel_indexing_map
    ExpressionChannelBaseMap check_channels_map;

    for(auto check_ch_it = index_node.check_channels.cbegin();
        check_ch_it != index_node.check_channels.cend(); ++check_ch_it)
    {
      check_channels_map[check_ch_it->channel_id] = check_ch_it->channel;
    }

    construct_simple_channel_indexing_map(
      new_check_channels,
      simple_channel_indexing_map,
      check_channels_map,
      std::ptr_fun(&collect_required_channels));

    ChannelPriorityMap channel_priority_map;

    construct_channel_priority_map(
      channel_priority_map,
      simple_channel_indexing_map);

    ChannelIdSet reindexed_channel_ids;

    // select frequently used required simple channel
    // and link expression to it
    // this allow to exclude repeatance channel checking on match
    // but require reverse lookup for all simple channels in
    // input channel set
    while(!channel_priority_map.empty())
    {
      ChannelPriorityMap::iterator ch_pr_it = --channel_priority_map.end();

      if(ch_pr_it->first.use_count < SUBINDEXING_THRESHOLD)
      {
        break;
      }

      unsigned long simple_channel_id = ch_pr_it->first.channel_id;
      assert(!ch_pr_it->second->empty());

      // index Expression with minimal used channels number
      ExpressionIndexingUseMap& expr_indexing_map = *(ch_pr_it->second);
      ExpressionIndexingUseMap::iterator expr_ind_it = expr_indexing_map.begin();
      ExpressionIndexing_var expr_indexing = expr_ind_it->second;

      ExpressionChannelMatch& expr_channel_match = init_match_cell_(
        index_node.channels_by_simple_channel_id,
        simple_channel_id);
      const unsigned long expr_channel_id = expr_indexing->channel->params().channel_id;
      reindexed_channel_ids.insert(expr_channel_id);

      ExpressionChannelBase_var reduced_channel;

      {
        reduced_channel = create_reduced_channel_(
          expr_indexing->channel,
          simple_channel_id);
      }

      if(reduced_channel)
      {
        expr_channel_match.check_channels.emplace_back(reduced_channel);
      }
      else
      {
        insert_if_not_exists(expr_channel_match.matched_channels, expr_channel_id);

        if (expr_indexing->channel->has_params() &&
            FILTER_CHANNELS.is_owned(expr_indexing->channel->params().type))
        {
          insert_if_not_exists(expr_channel_match.cpm_matched_channels, expr_channel_id);
        }
      }

      update_channel_priority_map(
        channel_priority_map,
        simple_channel_indexing_map,
        expr_indexing,
        expr_indexing->used_simple_channels, // all channels disappeared
        ChannelIdSet() // no remaining channel
        );

      expr_indexing->used_simple_channels.clear();
    }

    // fill new check channels (without already reindexed channels)
    collect_channel_ids(reindexed_channel_ids, new_check_channels);

    std::vector<CheckChannel> new_check_channels_cells;

    for (auto it = new_check_channels.cbegin();
         it != new_check_channels.cend(); ++it)
    {
      new_check_channels_cells.emplace_back(*it);
    }

    for(auto ch_it = index_node.check_channels.begin();
        ch_it != index_node.check_channels.end(); ++ch_it)
    {
      if(reindexed_channel_ids.find(ch_it->channel_id) == reindexed_channel_ids.end())
      {
        new_check_channels_cells.push_back(*ch_it);
      }
    }

    index_node.check_channels.swap(new_check_channels_cells);

    // do sub indexing for result nodes
    for(ExpressionChannelMatchMap::iterator eit =
          index_node.channels_by_simple_channel_id.begin();
        eit != index_node.channels_by_simple_channel_id.end(); ++eit)
    {
      sub_index_(*eit->second, depth + 1);
    }
  }

  void
  ExpressionChannelIndex::index(
    const ExpressionChannelHolderMap& all_channels)
    noexcept
  {
    ExpressionChannelHolderMap optimized_all_channels = all_channels;

    for(auto ch_it = optimized_all_channels.begin();
        ch_it != optimized_all_channels.end(); )
    {
      if(ch_it->second->channel.in())
      {
        ch_it->second->channel = ch_it->second->channel->optimize();
      }

      if(!ch_it->second->channel.in())
      {
        optimized_all_channels.erase(ch_it++);
      }
      else
      {
        ++ch_it;
      }
    }

    ExpressionChannelBaseMap expr_channels;
    index_simple_channels_(expr_channels, optimized_all_channels);

    top_level_index_(channels_, expr_channels);

    // make fast expression channels
    for (auto it = channels_.begin(); it != channels_.end(); ++it)
    {
      make_fast_expression_(*it->second);
    }
  }

  void
  ExpressionChannelIndex::index_simple_channels_(
    ExpressionChannelBaseMap& expr_channels,
    const ExpressionChannelHolderMap& all_channels)
    noexcept
  {
    for(ExpressionChannelHolderMap::const_iterator ch_it = all_channels.begin();
        ch_it != all_channels.end(); ++ch_it)
    {
      ConstSimpleChannel_var simple_channel = ch_it->second->simple_channel();
      if(simple_channel.in())
      {
        const unsigned long channel_id = simple_channel->params().channel_id;
        ExpressionChannelMatch& expr_channel_match = init_match_cell_(
          channels_, channel_id);
        
        if(FILTER_CHANNELS.is_owned(simple_channel->params().type))
        {
          insert_if_not_exists(
            expr_channel_match.cpm_matched_channels, channel_id);
        }

        insert_if_not_exists(expr_channel_match.matched_channels, channel_id);
      }
      else
      {
        expr_channels.insert(*ch_it);
      }
    }
  }

  unsigned long
  ExpressionChannelIndex::size() const noexcept
  {
    return channels_.size();
  }

  unsigned long
  ExpressionChannelIndex::avg_seq_size_() const noexcept
  {
    unsigned long sum = 0;
    for(ExpressionChannelMatchMap::const_iterator mit =
          channels_.begin();
        mit != channels_.end(); ++mit)
    {
      sum += mit->second->matched_channels.size() + mit->second->check_channels.size();
    }

    return channels_.empty() ? 0 : (sum / channels_.size());
  }

  void
  ExpressionChannelIndex::print(
    std::ostream& out,
    const ChannelIdSet* custom_channels,
    bool expand)
    const noexcept
  {
    for(ExpressionChannelMatchMap::const_iterator ind_it =
          channels_.begin();
        ind_it != channels_.end(); ++ind_it)
    {
      if(!custom_channels || custom_channels->find(ind_it->first) != custom_channels->end())
      {
        out << ind_it->first << " => { matched_channels = [";

        for (auto ch_it = ind_it->second->matched_channels.begin();
            ch_it != ind_it->second->matched_channels.end(); ++ch_it)
        {
          out << (ch_it != ind_it->second->matched_channels.begin() ? "," : "") <<
            (*ch_it);
        }

        const auto& expr_channels = ind_it->second->check_channels;
        out << "], check_channels(" << expr_channels.size() << ") = [";

        if(expand)
        {
          out << std::endl;
        }

        for(auto ech_it = expr_channels.cbegin();
            ech_it != expr_channels.cend(); ++ech_it)
        {
          if(expand)
          {
            out << "  " << ech_it->channel_id << ": ";
            ech_it->channel->print(out);
            out << std::endl;
          }
          else
          {
            out << (ech_it != expr_channels.begin() ? "," : "") << ech_it->channel_id;
          }
        }

        out << "], channels_by_simple_channel_id = [";
        for(ExpressionChannelMatchMap::const_iterator sit =
              ind_it->second->channels_by_simple_channel_id.begin();
            sit != ind_it->second->channels_by_simple_channel_id.end(); ++sit)
        {
          out << (sit != ind_it->second->channels_by_simple_channel_id.begin() ? "," : "") <<
            sit->first << " => " << (
              sit->second->matched_channels.size() + sit->second->check_channels.size());
        }
        out << "]" << std::endl;
      }
    }
  }

  void
  ExpressionChannelIndex::match_(
    ChannelIdSet& result_channels,
    ChannelIdSet* result_cpm_channels,
    CheckChannelMap& check_channels,
    const ExpressionChannelMatch& expr_channel_match,
    const ChannelIdHashSet& channels)
    noexcept
  {
    if(TRACE_MATCH_)
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

    if(TRACE_MATCH_)
    {
      std::cout << "match_(): check_channels(" <<
        expr_channel_match.check_channels.size() << "): ";
      for(auto ech_it = expr_channel_match.check_channels.cbegin();
          ech_it != expr_channel_match.check_channels.cend(); ++ech_it)
      {
        if(ech_it != expr_channel_match.check_channels.begin())
        {
          std::cout << ",";
        }
        std::cout << ech_it->channel_id;
      }
      std::cout << std::endl;
    }

    for(auto ech_it = expr_channel_match.check_channels.cbegin();
        ech_it != expr_channel_match.check_channels.cend(); ++ech_it)
    {
      check_channels.insert(std::make_pair(
        ech_it->channel_id,
        &*ech_it));
    }

    for(ExpressionChannelMatchMap::const_iterator sit =
          expr_channel_match.channels_by_simple_channel_id.begin();
        sit != expr_channel_match.channels_by_simple_channel_id.end();
        ++sit)
    {
      if(TRACE_MATCH_)
      {
        std::cout << "match_(): reverse check for simple channel #" <<
          sit->first << std::endl;
      }

      if(channels.find(sit->first) != channels.end())
      {
        if(TRACE_MATCH_)
        {
          std::cout << "match_(): recursive call for channel #" <<
            sit->first << std::endl;
        }

        match_(
          result_channels,
          result_cpm_channels,
          check_channels,
          *sit->second,
          channels);
      }
    }

    if(TRACE_MATCH_)
    {
      std::cout << "< match_()" << std::endl;
    }
  }

  void
  ExpressionChannelIndex::match(
    ChannelIdSet& result_channels,
    const ChannelIdHashSet& channels,
    ChannelIdSet* result_cpm_channels) const
    /*throw(eh::Exception)*/
  {
    CheckChannelMap check_channels;

    for(ChannelIdHashSet::const_iterator ch_it = channels.begin();
      ch_it != channels.end(); ++ch_it)
    {
      ExpressionChannelMatchMap::const_iterator ind_it = channels_.find(*ch_it);
      if(ind_it != channels_.end())
      {
        if(TRACE_MATCH_)
        {
          std::cout << "match_(): first level call for channel #" <<
            *ch_it << std::endl;
        }

        match_(
          result_channels,
          result_cpm_channels,
          check_channels,
          *ind_it->second,
          channels);
      }
    }

    if(TRACE_MATCH_)
    {
      std::cout << "match(): result check_channels(" <<
        check_channels.size() << "): ";
      for(CheckChannelMap::const_iterator ech_it = check_channels.begin();
          ech_it != check_channels.end(); ++ech_it)
      {
        if(ech_it != check_channels.begin())
        {
          std::cout << ",";
        }
        std::cout << ech_it->first;
      }
      std::cout << std::endl;
    }

    //unsigned long result_channels_size = result_channels.size();

    for(CheckChannelMap::const_iterator ech_it = check_channels.begin();
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

  bool
  ExpressionChannelIndex::reduce_channel_(
    ChannelIdSet& used_simple_channels,
    const ExpressionChannelBase* holder,
    const ChannelIdSet& indexed_simple_channels)
    noexcept
  {
    ConstSimpleChannel_var simple_channel = holder->simple_channel();
    if(simple_channel.in())
    {
      unsigned long simple_channel_id = simple_channel->params().channel_id;
      ChannelIdSet::const_iterator ch_it =
        indexed_simple_channels.find(simple_channel_id);
      if(ch_it != indexed_simple_channels.end())
      {
        return true;
      }

      used_simple_channels.insert(simple_channel_id);
      return false;
    }

    ConstExpressionChannel_var expression_channel = holder->expression_channel();
    if(expression_channel.in())
    {
      const ExpressionChannel::Expression& expr = expression_channel->expression();
      return reduce_channel_(used_simple_channels, expr, indexed_simple_channels);
    }

    return true;
  }

  bool
  ExpressionChannelIndex::reduce_channel_(
    ChannelIdSet& used_simple_channels,
    const ExpressionChannel::Expression& expr,
    const ChannelIdSet& indexed_simple_channels)
    noexcept
  {
    /* collect simple channels that must be indexed (used_simple_channels)
     * excluding already indexed (indexed_simple_channels),
     * I is fully reduced expression(returned false), E is other expression:
     *   I | E => reduced E
     *   I & E => I (return true)
     *   I ^ E => I (return true)
     *
     * return true if reduced channel equal to 'false' (fully reduced)
     * return false otherwise, insert X to used_simple_channels
     *
     * Expression channel:
     *   OR, NOP: call reduce_channel for each child channel return true only,
     *     if all calls return true
     *   AND, AND_NOT: call reduce_channel for each sub channel (
     *     excluding last element if operation is AND NOT),
     *     if return code of one is true return true
     *     otherwise copy local_used_simple_channels into used_simple_channels
     *       and return false
     */

    if(expr.op == ExpressionChannel::NOP)
    {
      return reduce_channel_(
        used_simple_channels, expr.channel, indexed_simple_channels);
    }
    else if(expr.op == ExpressionChannel::OR)
    {
      // fully reduced if all sub channels fully reduced (look above)
      bool ret = true;
      for(ExpressionChannel::Expression::ExpressionList::const_iterator eit =
            expr.sub_channels.begin();
          eit != expr.sub_channels.end(); ++eit)
      {
        ret &= reduce_channel_(used_simple_channels,
          *eit, indexed_simple_channels);
      }

      return ret;
    }

    // AND, AND_NOT operations
    ExpressionChannel::Expression::ExpressionList::const_iterator end_it =
      expr.op == ExpressionChannel::AND_NOT ?
      --expr.sub_channels.end() : expr.sub_channels.end();

    ChannelIdSet local_used_simple_channels;
    for(ExpressionChannel::Expression::ExpressionList::const_iterator eit =
          expr.sub_channels.begin();
        eit != end_it; ++eit)
    {
      if(reduce_channel_(local_used_simple_channels, *eit, indexed_simple_channels))
      {
        return true;
      }
    }

    std::copy(local_used_simple_channels.begin(),
      local_used_simple_channels.end(),
      std::inserter(used_simple_channels, used_simple_channels.begin()));

    return false;
  }

  ExpressionChannelBase_var
  ExpressionChannelIndex::create_reduced_channel_(
    ExpressionChannelBase* channel,
    const ChannelId indexed_simple_channel)
    noexcept
  {
    ConstSimpleChannel_var simple_channel = channel->simple_channel();
    if(simple_channel.in())
    {
      if (simple_channel->params().channel_id == indexed_simple_channel)
      {
        return ExpressionChannelBase_var();
      }
    }
    else
    {
      ConstExpressionChannel_var expression_channel = channel->expression_channel();
      if(expression_channel.in())
      {
        const ExpressionChannel::Expression& expr = expression_channel->expression();
        ExpressionChannel::Expression reduced_expr(expr);
        if(create_reduced_expression_(reduced_expr, indexed_simple_channel))
        {
          return ExpressionChannelBase_var();
        }

        return new ExpressionChannel(expression_channel->params(), std::move(reduced_expr));
      }
    }

    return ReferenceCounting::add_ref(channel);
  }

  bool
  ExpressionChannelIndex::create_reduced_expression_(
    ExpressionChannel::Expression& expr,
    const ChannelId indexed_simple_channel)
    noexcept
  {
    if(expr.op == ExpressionChannel::TRUE)
    {
      return true;
    }
    else if(expr.op == ExpressionChannel::NOP)
    {
      expr.channel = create_reduced_channel_(
        expr.channel, indexed_simple_channel);
      return expr.channel.in() == 0;
    }
    else if(expr.op == ExpressionChannel::OR)
    {
      for(ExpressionChannel::Expression::ExpressionList::iterator eit =
            expr.sub_channels.begin();
          eit != expr.sub_channels.end(); ++eit)
      {
        if(create_reduced_expression_(*eit, indexed_simple_channel))
        {
          return true;
        }
      }
    }
    else if(expr.op == ExpressionChannel::AND)
    {
      for(ExpressionChannel::Expression::ExpressionList::iterator eit =
            expr.sub_channels.begin();
          eit != expr.sub_channels.end(); )
      {
        if(create_reduced_expression_(*eit, indexed_simple_channel))
        {
          eit = expr.sub_channels.erase(eit);
        }
        else
        {
          ++eit;
        }
      }

      return expr.sub_channels.empty();
    }
    else if(expr.op == ExpressionChannel::AND_NOT)
    {
      ExpressionChannel::Expression::ExpressionList::iterator eit =
        expr.sub_channels.begin();

      assert(!expr.sub_channels.empty());

      while(true)
      {
        if(eit == --expr.sub_channels.end())
        {
          break;
        }

        if(create_reduced_expression_(*eit, indexed_simple_channel))
        {
          eit = expr.sub_channels.erase(eit);
        }
        else
        {
          ++eit;
        }
      }

      if(expr.sub_channels.size() == 1)
      {
        // push true channel to begin
        ExpressionChannel::Expression true_expr;
        true_expr.op = ExpressionChannel::TRUE;
        expr.sub_channels.insert(expr.sub_channels.begin(), true_expr);
      }
    }

    return false;
  }

  ExpressionChannelIndex::ExpressionChannelMatch&
  ExpressionChannelIndex::init_match_cell_(
    ExpressionChannelMatchMap& channels_by_simple_channel_id,
    unsigned long channel_id)
    noexcept
  {
    auto it = channels_by_simple_channel_id.find(channel_id);

    if (it == channels_by_simple_channel_id.end())
    {
      it = channels_by_simple_channel_id.insert(std::make_pair(
        channel_id, new ExpressionChannelMatch())).first;
    }

    return *(it->second);
  }
}
}
