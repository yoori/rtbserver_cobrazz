#include <sstream>

#include "ExpressionChannel.hpp"

namespace AdServer
{
namespace CampaignSvcs
{
  const ExpressionChannel::Expression ExpressionChannel::Expression::EMPTY;

  unsigned long
  ExpressionChannelHolder::triggered(
    const ChannelIdHashSet* triggered_channels,
    const ChannelWeightMap* weighted_triggered_channels,
    const char* status_set,
    ChannelUseCountMap* uc_tbl,
    ChannelIdSet* matched_channels) const
    /*throw(Exception)*/
  {
    return channel.in() ? channel->triggered(
      triggered_channels,
      weighted_triggered_channels,
      status_set,
      uc_tbl,
      matched_channels) : 0;
  }

  bool ExpressionChannelHolder::use(
    ChannelUseCountMap& uc_tbl,
    const ChannelIdHashSet& triggered_channels,
    const char* status_set,
    ChannelIdSet* matched_channels) const
    /*throw(Exception)*/
  {
    return channel.in() && channel->use(
      uc_tbl,
      triggered_channels,
      status_set,
      matched_channels);
  }

  void ExpressionChannelHolder::triggered_named_channels(
    ChannelIdSet& responded_channels,
    const ChannelIdHashSet& triggered_channels) const
    /*throw(Exception)*/
  {
    if(channel.in())
    {
      channel->triggered_named_channels(
        responded_channels,
        triggered_channels);
    }
  }

  bool
  ExpressionChannelHolder::triggered_expression(
    std::ostream& responded_expr,
    const ChannelIdHashSet& triggered_channels)
    const
    /*throw(Exception, eh::Exception)*/
  {
    return channel.in() && channel->triggered_expression(
      responded_expr,
      triggered_channels);
  }

  void ExpressionChannelHolder::get_cmp_channels(
    ExpressionChannelList& cmp_channels,
    const ChannelIdHashSet& simple_channels)
    /*throw(Exception)*/
  {
    if(channel.in())
    {
      channel->get_cmp_channels(cmp_channels, simple_channels);
    }
  }
  
  void ExpressionChannelHolder::get_all_cmp_channels(
    ExpressionChannelList& cmp_channels)
    /*throw(Exception)*/
  {
    if(channel.in())
    {
      channel->get_all_cmp_channels(cmp_channels);
    }
  }

  void ExpressionChannelHolder::get_all_channels(
    ChannelIdSet& channels)
    noexcept
  {
    if(channel.in())
    {
      channel->get_all_channels(channels);
    }
  }

  /* SimpleChannel */
  unsigned long
  SimpleChannel::triggered(
    const ChannelIdHashSet* triggered_channels,
    const ChannelWeightMap* weighted_triggered_channels,
    const char* status_set,
    ChannelUseCountMap* uc_tbl,
    ChannelIdSet* matched_channels) const
    /*throw(Exception)*/
  {
    unsigned long result = 0;

    if(!status_set || strchr(status_set, channel_params_.status) != 0)
    {
      if(triggered_channels)
      {
        result = triggered_channels->find(channel_params_.channel_id) !=
          triggered_channels->end() ? 1 : 0;
      }
      else if(weighted_triggered_channels)
      {
        ChannelWeightMap::const_iterator wit =
          weighted_triggered_channels->find(channel_params_.channel_id);
        if(wit != weighted_triggered_channels->end())
        {
          result = wit->second;
        }
      }

      if(result)
      {
        if(uc_tbl)
        {
          (*uc_tbl)[channel_params_.channel_id].channel_ids.insert(
            channel_params_.channel_id);
        }

        if(matched_channels)
        {
          matched_channels->insert(channel_params_.channel_id);
        }
      }
    }

    return result;
  }

  bool
  SimpleChannel::use(
    ChannelId channel_id,
    ChannelUseCountMap& uc_tbl,
    const ChannelIdHashSet& triggered_channels,
    ChannelIdSet* matched_channels)
    /*throw(Exception)*/
  {
    if(triggered_channels.find(channel_id) ==
       triggered_channels.end())
    {
      return false;
    }

    ChannelUseCountMap::iterator it = uc_tbl.find(channel_id);
    if (it == uc_tbl.end())
    {
      uc_tbl[channel_id].count = 1;
    }

    if(matched_channels)
    {
      matched_channels->insert(channel_id);
    }

    return true;
  }

  bool
  SimpleChannel::use(
    ChannelUseCountMap& uc_tbl,
    const ChannelIdHashSet& triggered_channels,
    const char* /*status_set*/,
    ChannelIdSet* matched_channels) const
    /*throw(Exception)*/
  {
    return use(
      channel_params_.channel_id,
      uc_tbl,
      triggered_channels,
      matched_channels);
  }

  void SimpleChannel::triggered_named_channels(
    ChannelIdSet& triggered_named_channels,
    const ChannelIdHashSet& triggered_channels) const
    /*throw(Exception)*/
  {
    if(triggered(&triggered_channels, 0, "AW"))
    {
      triggered_named_channels.insert(channel_params_.channel_id);
    }
  }

  bool
  SimpleChannel::triggered_expression(
    std::ostream& responded_expr,
    const ChannelIdHashSet& triggered_channels)
    const
    /*throw(Exception, eh::Exception)*/
  {
    if(triggered(&triggered_channels, 0))
    {
      responded_expr << channel_params_.channel_id;
      return true;
    }
    
    return false;
  }

  void SimpleChannel::get_cmp_channels(
    ExpressionChannelList& cmp_channels,
    const ChannelIdHashSet& simple_channels)
    /*throw(Exception)*/
  {
    if(channel_params_.cmp_params.in() && triggered(&simple_channels, 0))
    {
      cmp_channels.push_back(ReferenceCounting::add_ref(this));
    }
  }

  void SimpleChannel::get_all_cmp_channels(
    ExpressionChannelList& cmp_channels)
    /*throw(Exception)*/
  {
    if(channel_params_.cmp_params.in())
    {
      cmp_channels.push_back(ReferenceCounting::add_ref(this));
    }
  }

  void SimpleChannel::get_all_channels(
    ChannelIdSet& channels)
    noexcept
  {
    channels.insert(channel_params_.channel_id);
  }

  // ExpressionChannel
  void ExpressionChannel::store_use_count_(
    ChannelUseCountMap* uc_tbl,
    const ExpressionChannelBase* channel,
    const ChannelIdSet& local_matched_channels)
    /*throw(Exception)*/
  {
    if(channel)
    {
      ChannelUseCountMap::iterator it = uc_tbl->find(
        channel->params().channel_id);
      if(it != uc_tbl->end())
      {
        ++it->second.count;
      }
      else
      {
        ChannelUseCount& res_uc_rec = (*uc_tbl)[channel->params().channel_id];
        res_uc_rec.count = 1;
        std::copy(
          local_matched_channels.begin(),
          local_matched_channels.end(),
          std::inserter(
            res_uc_rec.channel_ids,
            res_uc_rec.channel_ids.begin()));
      }
    }
  }

  inline unsigned long
  ExpressionChannel::triggered_expr_(
    const Expression& expr,
    const ChannelIdHashSet* triggered_channels,
    const ChannelWeightMap* weighted_triggered_channels,
    const char* status_set,
    ChannelUseCountMap* uc_tbl,
    ChannelIdSet* matched_channels_ptr)
    /*throw(Exception)*/
  {
    bool result = false;

    switch (expr.op)
    {
    case TRUE:
      result = true;
      break;
    case NOP:
      {
        if(expr.channel.in())
        {
          result = expr.channel->triggered(
            triggered_channels, weighted_triggered_channels,
            status_set, uc_tbl, matched_channels_ptr);
        }
      }
      break;
    case AND:
      {
        result = true;
        for(Expression::ExpressionList::const_iterator ch_it =
              expr.sub_channels.begin();
            ch_it != expr.sub_channels.end(); ++ch_it)
        {
          if (!triggered_(*ch_it,
            triggered_channels, weighted_triggered_channels,
            status_set, uc_tbl, matched_channels_ptr))
          {
            result = false;
            break;
          }
        }
      }
      break;
    case OR:
      {
        for(Expression::ExpressionList::const_iterator ch_it =
              expr.sub_channels.begin();
            ch_it != expr.sub_channels.end(); ++ch_it)
        {
          if (triggered_(*ch_it,
            triggered_channels, weighted_triggered_channels,
            status_set, uc_tbl, matched_channels_ptr))
          {
            result = true;
            break;
          }
        }
      }
      break;
    case AND_NOT:
      {
        Expression::ExpressionList::const_iterator ch_it = expr.sub_channels.begin();
        result = triggered_(*ch_it,
          triggered_channels, weighted_triggered_channels,
          status_set, uc_tbl, matched_channels_ptr);

        if(result)
        {
          ++ch_it;
          for( ; ch_it != expr.sub_channels.end(); ++ch_it)
          {
            if(triggered_(*ch_it,
              triggered_channels, weighted_triggered_channels,
              status_set, uc_tbl, matched_channels_ptr))
            {
              result = false;
              break;
            }
          }
        }
      }
      break;

    default:
      assert(0);
    }

    return result ? 1 : 0;
  }

  inline unsigned long
  ExpressionChannel::triggered_(
    const Expression& expr,
    const ChannelIdHashSet* triggered_channels,
    const ChannelWeightMap* weighted_triggered_channels,
    const char* status_set,
    ChannelUseCountMap* uc_tbl,
    ChannelIdSet* matched_channels)
    /*throw(Exception)*/
  {
    if(matched_channels || uc_tbl)
    {
      ChannelIdSet local_matched_channels;

      unsigned long result = triggered_expr_(
        expr,
        triggered_channels,
        weighted_triggered_channels,
        status_set,
        uc_tbl,
        &local_matched_channels);

      if(result)
      {
        if(matched_channels)
        {
          std::copy(
            local_matched_channels.begin(),
            local_matched_channels.end(),
            std::inserter(*matched_channels, matched_channels->begin()));
        }

        if(uc_tbl)
        {
          store_use_count_(uc_tbl, expr.channel, local_matched_channels);
        }
      }

      return result;
    }
    else
    {
      return triggered_expr_(
        expr,
        triggered_channels,
        weighted_triggered_channels,
        status_set,
        0,
        0);
    }
  }

  unsigned long
  ExpressionChannel::triggered(
    const ChannelIdHashSet* triggered_channels,
    const ChannelWeightMap* weighted_triggered_channels,
    const char* status_set,
    ChannelUseCountMap* uc_tbl,
    ChannelIdSet* matched_channels) const
    /*throw(Exception)*/
  {
    if(status_set && strchr(status_set, channel_params_.status) == 0)
    {
      return 0;
    }

    return triggered_(
      expr_,
      triggered_channels,
      weighted_triggered_channels,
      status_set,
      uc_tbl,
      matched_channels);
  }

  bool ExpressionChannel::use_(
    ChannelUseCountMap& uc_tbl,
    const Expression& expr,
    const ChannelIdHashSet& triggered_channels,
    const char* status_set,
    ChannelIdSet* matched_channels)
    /*throw(Exception)*/
  {
    bool result = false;

    if(!expr.channel.in())
    {
      for(Expression::ExpressionList::const_iterator ch_it =
            expr.sub_channels.begin();
        ch_it != expr.sub_channels.end(); ++ch_it)
      {
        result |= use_(
          uc_tbl,
          *ch_it,
          triggered_channels,
          status_set,
          matched_channels);
      }
    }
    else
    {
      result |= expr.channel->use(
        uc_tbl,
        triggered_channels,
        status_set,
        matched_channels);
    }
    
    return result;
  }
  
  bool ExpressionChannel::use(
    ChannelUseCountMap& uc_tbl,
    const ChannelIdHashSet& triggered_channels,
    const char* status_set,
    ChannelIdSet* matched_channels) const
    /*throw(Exception)*/
  {
    bool result = false;
    
    ChannelUseCountMap::iterator it = uc_tbl.find(
      params().channel_id);

    if (it != uc_tbl.end())
    {
      if(matched_channels)
      {
        std::copy(
          it->second.channel_ids.begin(),
          it->second.channel_ids.end(),
          std::inserter(*matched_channels, matched_channels->begin()));
      }
      
      ++it->second.count;
      result = true;
    }
    else
    {
      ChannelIdSet local_matched_channels;

      result = use_(
        uc_tbl, expr_, triggered_channels, status_set, &local_matched_channels);

      if(result)
      {
        ChannelUseCount& use_count = uc_tbl[params().channel_id];
        use_count.count = 1;

        std::copy(
          local_matched_channels.begin(),
          local_matched_channels.end(),
          std::inserter(use_count.channel_ids, use_count.channel_ids.begin()));

        if(matched_channels)
        {
          std::copy(
            local_matched_channels.begin(),
            local_matched_channels.end(),
            std::inserter(*matched_channels, matched_channels->begin()));
        }
      }
    }

    return result;
  }

  void
  ExpressionChannel::triggered_named_channels_(
    ChannelIdSet& triggered_named_channels,
    const Expression& expr,
    const ChannelIdHashSet& triggered_channels)
    /*throw(Exception)*/
  {
    if(expr.channel.in() &&
       expr.channel->has_params())
    {
      expr.channel->triggered_named_channels(
        triggered_named_channels, triggered_channels);
    }

    /* trace childs */
    for (Expression::ExpressionList::const_iterator ch_it =
           expr.sub_channels.begin();
         ch_it != expr.sub_channels.end(); ++ch_it)
    {
      triggered_named_channels_(
        triggered_named_channels,
        *ch_it,
        triggered_channels);
    }
  }

  void
  ExpressionChannel::triggered_named_channels(
    ChannelIdSet& triggered_named_channels,
    const ChannelIdHashSet& triggered_channels) const
    /*throw(Exception)*/
  {
    if(triggered(&triggered_channels, 0, "AW") &&
       params().channel_id)
    {
      triggered_named_channels.insert(params().channel_id);
    }

    triggered_named_channels_(
      triggered_named_channels,
      expr_,
      triggered_channels);
  }

  bool
  ExpressionChannel::triggered_expression(
    std::ostream& responded_expr,
    const ChannelIdHashSet& triggered_channels)
    const
    /*throw(Exception, eh::Exception)*/
  {
    return triggered_expression_(responded_expr, expr_, triggered_channels);
  }
  
  bool
  ExpressionChannel::triggered_expression_(
    std::ostream& responded_expr,
    const Expression& expr,
    const ChannelIdHashSet& triggered_channels)
    const
    /*throw(Exception, eh::Exception)*/
  {
    if(!triggered_(expr, &triggered_channels, 0, "A", 0, 0))
    {
      return false;
    }

    Expression::ExpressionList::const_iterator end_it = expr.sub_channels.end();

    switch (expr.op)
    {
    case NOP:
      if (expr.channel.in())
      {
        if(params().channel_id || expr.channel->params().type == 'V')
        {
          responded_expr << expr.channel->params().channel_id;
          return true;
        }
        else
        {
          return expr.channel->triggered_expression(responded_expr, triggered_channels);
        }
      }
      else
      {
        return false;
      }
      
    case AND_NOT:
      assert(expr.sub_channels.begin() != expr.sub_channels.end());
      end_it = ++expr.sub_channels.begin();
      [[fallthrough]];
  
    case AND:
      {
        std::ostringstream sub_expr;
        for (Expression::ExpressionList::const_iterator ch_it = expr.sub_channels.begin();
             ch_it != end_it;
             ++ch_it)
        {
          if(ch_it != expr.sub_channels.begin())
          {
            sub_expr << " & ";
          }
          
          if(!triggered_expression_(sub_expr, *ch_it, triggered_channels))
          {
            return false;
          }
        }

        responded_expr << '(' << sub_expr.str() << ')';
        return true;
      }
      
    case OR:
      {
        std::ostringstream sub_expr;
        unsigned int count = 0;
        for (Expression::ExpressionList::const_iterator ch_it = expr.sub_channels.begin();
             ch_it != expr.sub_channels.end();
             ++ch_it)
        {
          std::ostringstream ss_expr;
          if (triggered_expression_(ss_expr, *ch_it, triggered_channels))
          {
            sub_expr << (count ? " | " : "") << ss_expr.str();
            ++count;
          }
        }

        if (count == 0)
        {
          return false;
        }

        responded_expr << (count > 1 ? "(" : "") << sub_expr.str() <<
          (count > 1 ? ")" : "");
            
        return true;
      }
      
    default:
      assert(0);
    }
    
    return true;
  }

  bool
  ExpressionChannel::optimize_expr_(
    Expression& expr)
  {
    if(expr.op == NOP)
    {
      if(expr.channel)
      {
        expr.channel = expr.channel->optimize();
      }

      return expr.channel.in();
    }
    else if(expr.op == AND_NOT)
    {
      assert(!expr.sub_channels.empty());

      if(!optimize_expr_(*expr.sub_channels.begin()))
      {
        expr.op = NOP;
        expr.sub_channels.clear();
        return false;
      }
      
      for(Expression::ExpressionList::iterator sit =
            ++expr.sub_channels.begin();
          sit != expr.sub_channels.end(); )
      {
        if(!optimize_expr_(*sit))
        {
          sit = expr.sub_channels.erase(sit);
        }
        else
        {
          ++sit;
        }
      }

      if(expr.sub_channels.size() == 1)
      {
        // all AND NOT parts skipped
        expr.op = AND;
      }
    }
    else
    {
      for(Expression::ExpressionList::iterator sit =
            expr.sub_channels.begin();
          sit != expr.sub_channels.end(); )
      {
        if(!optimize_expr_(*sit))
        {
          if(expr.op == AND)
          {
            expr.op = NOP;
            expr.sub_channels.clear();
            return false;
          }
          else if(expr.op == OR)
          {
            sit = expr.sub_channels.erase(sit);
          }
          else
          {
            ++sit;
          }
        }
        else
        {
          ++sit;
        }
      }

      if(expr.sub_channels.empty())
      {
        expr.op = NOP;
        return false;
      }
    }

    if(expr.op == AND || expr.op == OR)
    {
      Expression::ExpressionList new_sub_channels;

      for(Expression::ExpressionList::const_iterator sit =
            expr.sub_channels.begin();
          sit != expr.sub_channels.end(); ++sit)
      {
        if(sit->op == expr.op || (
             (sit->op == AND || sit->op == OR) &&
             (sit->sub_channels.size() == 1 || expr.sub_channels.size() == 1)))
        {
          if(expr.sub_channels.size() == 1)
          {
            expr.op = sit->op;
          }

          std::copy(
            sit->sub_channels.begin(),
            sit->sub_channels.end(),
            std::back_inserter(new_sub_channels));
        }
        else
        {
          new_sub_channels.push_back(*sit);
        }
      }

      expr.sub_channels.swap(new_sub_channels);
    }

    return true;
  }

  ReferenceCounting::SmartPtr<ExpressionChannelBase, ReferenceCounting::PolicyAssert>
  ExpressionChannel::optimize()
  {
    if(optimize_expr_(expr_))
    {
      return ReferenceCounting::add_ref(this);
    }

    return ReferenceCounting::SmartPtr<ExpressionChannelBase, ReferenceCounting::PolicyAssert>();
  }

  bool
  ExpressionChannel::equal(const ExpressionChannel* right) const noexcept
  {
    if(!(params().equal(right->params())))
    {
      return false;
    }

    return expr_ == right->expr_;
  }
  
  void ExpressionChannel::get_all_cmp_channels_(
    ExpressionChannelList& cmp_channels,
    const Expression& expr)
    /*throw(Exception)*/
  {
    if(expr.channel.in())
    {
      expr.channel->get_all_cmp_channels(cmp_channels);
    }
    else
    {
      for (Expression::ExpressionList::const_iterator ch_it =
             expr.sub_channels.begin();
           ch_it != expr.sub_channels.end(); ++ch_it)
      {
        get_all_cmp_channels_(cmp_channels, *ch_it);
      }
    }
  }

  void ExpressionChannel::get_cmp_channels_(
    ExpressionChannelList& cmp_channels,
    const ChannelIdHashSet& simple_channels,
    const Expression& expr)
    /*throw(Exception)*/
  {
    if(expr.channel.in())
    {
      expr.channel->get_cmp_channels(cmp_channels, simple_channels);
    }
    else
    {
      Expression::ExpressionList::const_iterator last_it =
        expr.sub_channels.end();
      
      if(expr.op == AND_NOT)
      {
        get_all_cmp_channels_(cmp_channels, *expr.sub_channels.rbegin());
        --last_it;
      }

      for (Expression::ExpressionList::const_iterator ch_it =
             expr.sub_channels.begin();
           ch_it != last_it; ++ch_it)
      {
        get_cmp_channels_(cmp_channels, simple_channels, *ch_it);
      }
    }
  }
  
  void ExpressionChannel::get_all_channels_(
    ChannelIdSet& channels,
    const Expression& expr)
    noexcept
  {
    if(expr.channel.in())
    {
      expr.channel->get_all_channels(channels);
    }
    else
    {
      for (Expression::ExpressionList::const_iterator ch_it =
             expr.sub_channels.begin();
           ch_it != expr.sub_channels.end(); ++ch_it)
      {
        get_all_channels_(channels, *ch_it);
      }
    }
  }

  void ExpressionChannel::get_cmp_channels(
    ExpressionChannelList& cmp_channels,
    const ChannelIdHashSet& simple_channels)
    /*throw(Exception)*/
  {
    if((channel_params_.channel_id == 0 ||
        (channel_params_.common_params.in() &&
          !channel_params_.common_params->is_public)) &&
       triggered(&simple_channels, 0))
    {
      if(channel_params_.cmp_params.in())
      {
        cmp_channels.push_back(ReferenceCounting::add_ref(this));
      }
      else
      {
        get_cmp_channels_(cmp_channels, simple_channels, expr_);
      }
    }
  }

  void ExpressionChannel::get_all_cmp_channels(
    ExpressionChannelList& cmp_channels)
    /*throw(Exception)*/
  {
    if(channel_params_.channel_id == 0 ||
       (channel_params_.common_params.in() &&
         !channel_params_.common_params->is_public))
    {
      if(channel_params_.cmp_params.in())
      {
        cmp_channels.push_back(ReferenceCounting::add_ref(this));
      }
      else
      {
        get_all_cmp_channels_(cmp_channels, expr_);
      }
    }
  }

  void ExpressionChannel::get_all_channels(
    ChannelIdSet& channels)
    noexcept
  {
    get_all_channels_(channels, expr_);
  }

  /*
   * FastExpressionChannel
   */
  namespace
  {
    ChannelParams EMPTY_CHANNEL_PARAMS;
  }

  FastExpressionChannel::FastExpressionChannel(
    const ExpressionChannelBase* channel)
    /*throw(eh::Exception)*/
  {
    if (channel->expression_channel())
    {
      expr_ = make_cell_(channel->expression_channel()->expression());
    }
    else if (channel->simple_channel())
    {
      expr_.op = ExpressionChannel::NOP;
      expr_.channel_id = channel->simple_channel()->has_params() ?
        channel->simple_channel()->params().channel_id : 0;
    }

    ChannelIdSet unused;
    reordering_(expr_, unused);
  }

  bool
  FastExpressionChannel::has_params() const noexcept
  {
    return false;
  }

  const ChannelParams&
  FastExpressionChannel::params() const noexcept
  {
    return EMPTY_CHANNEL_PARAMS;
  }

  ChannelParams&
  FastExpressionChannel::params() noexcept
  {
    return EMPTY_CHANNEL_PARAMS;
  }

  unsigned long
  FastExpressionChannel::triggered(
    const ChannelIdHashSet* triggered_channels,
    const ChannelWeightMap*,
    const char*,
    ChannelUseCountMap*,
    ChannelIdSet*) const
    /*throw(Exception)*/
  {
    return (match_cell_(expr_, *triggered_channels) ? 1 : 0);
  }

  bool
  FastExpressionChannel::use(
    ChannelUseCountMap& uc_tbl,
    const ChannelIdHashSet& triggered_channels,
    const char* status_set,
    ChannelIdSet* matched_channels) const
    /*throw(Exception)*/
  {
    bool result = false;

    ChannelUseCountMap::iterator it = uc_tbl.find(
      params().channel_id);

    if (it != uc_tbl.end())
    {
      if(matched_channels)
      {
        std::copy(
          it->second.channel_ids.begin(),
          it->second.channel_ids.end(),
          std::inserter(*matched_channels, matched_channels->begin()));
      }

      ++it->second.count;
      result = true;
    }
    else
    {
      ChannelIdSet local_matched_channels;

      result = use_(
        uc_tbl, expr_, triggered_channels, status_set, &local_matched_channels);

      if(result)
      {
        ChannelUseCount& use_count = uc_tbl[params().channel_id];
        use_count.count = 1;

        use_count.channel_ids.insert(
          local_matched_channels.begin(),
          local_matched_channels.end());

        if(matched_channels)
        {
          matched_channels->insert(
            local_matched_channels.begin(),
            local_matched_channels.end());
        }
      }
    }

    return result;
  }

  void
  FastExpressionChannel::get_all_channels(
    ChannelIdSet& channels)
    noexcept
  {
    get_all_channels_(channels, expr_);
  }

  std::ostream&
  FastExpressionChannel::print_(
    std::ostream& os,
    const Expression& expr)
    /*throw(eh::Exception)*/
  {
    if (expr.op == ExpressionChannel::NOP)
    {
      os << expr.channel_id;
    }
    else if (expr.op == ExpressionChannel::TRUE)
    {
      os << 'T';
    }
    else
    {
      os << '(';
      bool first_argument_flag = true;

      for (auto it = expr.simple.begin(); it != expr.simple.end(); ++it)
      {
        if (!first_argument_flag)
        {
          os << ' ' << static_cast<char>(expr.op) << ' ';
        }

        os << *it;
        first_argument_flag = false;
      }

      for (auto it = expr.sub_channels.begin(); it != expr.sub_channels.end(); ++it)
      {
        if (!first_argument_flag)
        {
          os << ' ' << (char)expr.op << ' ';
        }

        print_(os, *it);
        first_argument_flag = false;
      }

      os << ')';
    }

    return os;
  }

  std::ostream&
  FastExpressionChannel::print(std::ostream& os) const /*throw(eh::Exception)*/
  {
    return print_(os, expr_);
  }

  bool
  FastExpressionChannel::match_cell_(
    const Expression& expr,
    const ChannelIdHashSet& channels)
  {
    bool result = false;

    switch (expr.op)
    {
    case ExpressionChannel::NOP:
      result = expr.channel_id && (channels.find(expr.channel_id) != channels.end());
      break;
    case ExpressionChannel::AND:
      {
        for (auto it = expr.simple.cbegin(); it != expr.simple.cend(); ++it)
        {
          if (channels.find(*it) == channels.end())
          {
            return false;
          }
        }

        for(auto it = expr.sub_channels.begin(); it != expr.sub_channels.end(); ++it)
        {
          if (!match_cell_(*it, channels))
          {
            return false;
          }
        }

        result = true;
      }
      break;

    case ExpressionChannel::OR:
      {
        for(auto it = expr.simple.cbegin(); it != expr.simple.cend(); ++it)
        {
          if(channels.find(*it) != channels.end())
          {
            return true;
          }
        }

        for(auto it = expr.sub_channels.begin(); it != expr.sub_channels.end(); ++it)
        {
          if (match_cell_(*it, channels))
          {
            result = true;
            break;
          }
        }
      }
      break;

    case ExpressionChannel::AND_NOT:
      {
        auto it = expr.sub_channels.begin();
        result = match_cell_(*it, channels);

        if (result)
        {
          ++it;

          for (; it != expr.sub_channels.end(); ++it)
          {
            if (match_cell_(*it, channels))
            {
              result = false;
              break;
            }
          }
        }
      }
      break;

    case ExpressionChannel::TRUE:
      result = true;
      break;

    default:
      assert(0);
    }

    return result;
  }

  void
  FastExpressionChannel::get_all_channels_(
    ChannelIdSet& channels,
    const Expression& expr)
    /*throw(eh::Exception)*/
  {
    if (expr.op == ExpressionChannel::NOP)
    {
      channels.insert(expr.channel_id);
    }
    else
    {
      channels.insert(expr.simple.begin(), expr.simple.end());

      for (auto it = expr.sub_channels.begin();
           it != expr.sub_channels.end(); ++it)
      {
        get_all_channels_(channels, *it);
      }
    }
  }

  bool
  FastExpressionChannel::use_(
    ChannelUseCountMap& uc_tbl,
    const Expression& expr,
    const ChannelIdHashSet& triggered_channels,
    const char* status_set,
    ChannelIdSet* matched_channels)
    /*throw(Exception)*/
  {
    bool result = false;

    if(expr.op == ExpressionChannel::NOP)
    {
      result |= SimpleChannel::use(
        expr.channel_id,
        uc_tbl,
        triggered_channels,
        matched_channels);
    }
    else
    {
      for (auto it = expr.sub_channels.begin();
           it != expr.sub_channels.end(); ++it)
      {
        result |= use_(
          uc_tbl,
          *it,
          triggered_channels,
          status_set,
          matched_channels);
      }
    }

    return result;
  }

  FastExpressionChannel::Expression
  FastExpressionChannel::make_cell_(const ExpressionChannel::Expression& expr)
    /*throw(eh::Exception)*/
  {
    Expression cell;
    cell.op = expr.op;

    switch (expr.op)
    {
    case ExpressionChannel::NOP:
      {
        assert (expr.channel);
        const ConstSimpleChannel_var simple_channel = expr.channel->simple_channel();

        if (simple_channel)
        {
          cell.channel_id = simple_channel->params().channel_id;
        }
        else
        {
          const ConstExpressionChannel_var expression_channel =
            expr.channel->expression_channel();

          if(expression_channel.in())
          {
            cell = make_cell_(expression_channel->expression());
          }
          else
          {
            cell.channel_id = 0;
          }
        }
      }
      break;

    case ExpressionChannel::OR:
    case ExpressionChannel::AND:
      {
        if (expr.sub_channels.size() == 1)
        {
          cell = make_cell_(expr.sub_channels.front());
        }
        else
        {
          ChannelIdSet simple;

          for(auto it = expr.sub_channels.begin(); it != expr.sub_channels.end(); ++it)
          {
            const Expression sub_cell = make_cell_(*it);

            if (sub_cell.op == ExpressionChannel::NOP)
            {
              //assert(sub_cell.sub_channels.empty() && sub_cell.simple.empty() && sub_cell.channel_id);
              if(sub_cell.channel_id)
              {
                simple.insert(sub_cell.channel_id);
              }
            }
            else if (sub_cell.op == expr.op ||
              ((sub_cell.op == ExpressionChannel::OR || sub_cell.op == ExpressionChannel::AND) &&
              (sub_cell.simple.size() + sub_cell.sub_channels.size() == 1)))
            {
              simple.insert(sub_cell.simple.begin(), sub_cell.simple.end());
              cell.sub_channels.insert(
                cell.sub_channels.end(),
                sub_cell.sub_channels.begin(),
                sub_cell.sub_channels.end());
            }
            else
            {
              cell.sub_channels.emplace_back(std::move(sub_cell));
            }
          }

          cell.simple.assign(simple.cbegin(), simple.cend());
        }
      }
      break;

    case ExpressionChannel::AND_NOT:
      {
        for (auto it = expr.sub_channels.begin();
             it != expr.sub_channels.end(); ++it)
        {
          cell.sub_channels.emplace_back(make_cell_(*it));
        }
      }
      break;

    case ExpressionChannel::TRUE:
      break;
    default:
      assert(0);
      break;
    }

    return cell;
  }

  void
  FastExpressionChannel::reordering_(
    Expression& expr,
    ChannelIdSet& basis)
    /*throw(eh::Exception)*/
  {
    switch (expr.op)
    {
    case ExpressionChannel::NOP:
      basis.insert(expr.channel_id);
      break;

    case ExpressionChannel::AND_NOT:
      {
        std::multimap<std::size_t, std::size_t> complexity;

        int i = 0;
        for (auto sit = expr.sub_channels.begin(); sit != expr.sub_channels.end(); ++sit, ++i)
        {
          Expression& sub_expr = *sit;
          ChannelIdSet sub_basis(sub_expr.simple.begin(), sub_expr.simple.end());
          reordering_(sub_expr, sub_basis);
          complexity.insert(std::make_pair(sub_basis.size(), i));

          if (i == 0 || sub_basis.size() < basis.size())
          {
            basis.swap(sub_basis);
          }
        }

        std::vector<Expression> sub_channels;

        if (!expr.sub_channels.empty())
        {
          sub_channels.emplace_back(std::move(expr.sub_channels[0]));
        }

        for (auto it = complexity.begin(); it != complexity.end(); ++it)
        {
          if (it->second != 0)
          {
            sub_channels.emplace_back(std::move(expr.sub_channels[it->second]));
          }
        }

        sub_channels.swap(expr.sub_channels);
      }
      break;

    case ExpressionChannel::AND:
      {
        std::multimap<std::size_t, std::size_t> complexity;

        for (std::size_t i = 0; i <  expr.sub_channels.size(); ++i)
        {
          Expression& sub_expr = expr.sub_channels[i];
          ChannelIdSet sub_basis(sub_expr.simple.begin(), sub_expr.simple.end());
          reordering_(sub_expr, sub_basis);
          complexity.insert(std::make_pair(sub_basis.size(), i));

          if (!i || sub_basis.size() < basis.size())
          {
            basis.swap(sub_basis);
          }
        }

        std::vector<Expression> sub_channels;

        for (auto it = complexity.begin(); it != complexity.end(); ++it)
        {
          sub_channels.emplace_back(std::move(expr.sub_channels[it->second]));
        }

        sub_channels.swap(expr.sub_channels);
      }
      break;

    case ExpressionChannel::OR:
      {
        std::multimap<std::size_t, std::size_t> complexity;

        for (std::size_t i = 0; i < expr.sub_channels.size(); ++i)
        {
          ChannelIdSet sub_basis;
          reordering_(expr.sub_channels[i], sub_basis);
          complexity.insert(std::make_pair(sub_basis.size(), i));
          basis.insert(sub_basis.begin(), sub_basis.end());
        }

        std::vector<Expression> sub_channels;

        for (auto it = complexity.begin(); it != complexity.end(); ++it)
        {
          sub_channels.emplace_back(std::move(expr.sub_channels[it->second]));
        }

        sub_channels.swap(expr.sub_channels);
      }
      break;

    case ExpressionChannel::TRUE:
      break;

    default:
      assert(0);
    }
  }
}
}
