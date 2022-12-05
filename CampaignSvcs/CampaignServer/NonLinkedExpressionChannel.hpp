#ifndef _NONLINKEDEXPRESSIONCHANNEL_HPP_
#define _NONLINKEDEXPRESSIONCHANNEL_HPP_

#include <vector>
#include <memory>

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <CampaignSvcs/CampaignCommons/ExpressionChannel.hpp>

namespace AdServer
{
  namespace CampaignSvcs
  {
    /* Hold expression channel (or simple channel) presentation
     * non linked to other channels */
    class NonLinkedExpressionChannel:
      public ReferenceCounting::AtomicImpl
    {
    public:
      friend void print(std::ostream& out,
        const NonLinkedExpressionChannel* channel)
        noexcept;

      friend bool channel_equal(
        NonLinkedExpressionChannel* left,
        NonLinkedExpressionChannel* right,
        bool status_check)
        noexcept;

      enum Operation
      {
        NOP = '-',
        AND = '&',
        OR = '|',
        AND_NOT = '^'
      };

      struct Expression
      {
        typedef std::vector<Expression> ExpressionList;
        Operation op;
        unsigned long channel_id;
        ExpressionList sub_channels;

        Expression(): op(NOP), channel_id(0) {}
        Expression(unsigned long channel_id_val)
          : op(NOP), channel_id(channel_id_val)
        {}

        bool operator==(const Expression& right) const noexcept;

        void swap(Expression& right) noexcept;

        void all_channels(ChannelIdSet& channels) const noexcept;
      };

    public:
      NonLinkedExpressionChannel(const ChannelParams& channel_params)
        : channel_params_(channel_params)
      {}

      NonLinkedExpressionChannel(
        const ChannelParams& channel_params,
        const Expression& expression_val)
        noexcept
        : channel_params_(channel_params),
          expr_(new Expression(expression_val))
      {};

      NonLinkedExpressionChannel(const Expression& expression_val)
        noexcept
        : expr_(new Expression(expression_val))
      {};

      NonLinkedExpressionChannel(const NonLinkedExpressionChannel& right)
        noexcept
        : ReferenceCounting::Interface(right),
          ReferenceCounting::AtomicImpl(),
          channel_params_(right.channel_params_),
          expr_(right.expr_.get() ? new Expression(*right.expr_) : 0)
      {}

      bool equal(const NonLinkedExpressionChannel* right,
        bool status_check) const noexcept;

      const Expression* expression() const noexcept
      {
        return expr_.get();
      }

      const ChannelParams& params() const noexcept
      {
        return channel_params_;
      }

      ChannelParams& params() noexcept
      {
        return channel_params_;
      }

      void params(const ChannelParams& channel_params)
      {
        channel_params_ = channel_params;
      }

    protected:
      virtual
      ~NonLinkedExpressionChannel() noexcept
      {
      }

    private:
      ChannelParams channel_params_;
      std::unique_ptr<Expression> expr_;
    };

    typedef ReferenceCounting::SmartPtr<NonLinkedExpressionChannel>
      NonLinkedExpressionChannel_var;
  }
}

namespace AdServer
{
  namespace CampaignSvcs
  {
    inline
    void print_expression(std::ostream& out,
      const NonLinkedExpressionChannel::Expression& expr)
      noexcept
    {
      if(expr.op == NonLinkedExpressionChannel::NOP)
      {
        out << expr.channel_id;
      }
      else
      {
        out << "(";
        for(NonLinkedExpressionChannel::Expression::ExpressionList::const_iterator eit =
              expr.sub_channels.begin();
            eit != expr.sub_channels.end(); ++eit)
        {
          if(eit != expr.sub_channels.begin())
          {
            out << " " << static_cast<char>(expr.op) << " ";
          }

          print_expression(out, *eit);
        }
        out << ")";
      }
    }

    inline
    void print(std::ostream& out,
      const NonLinkedExpressionChannel* channel)
      noexcept
    {
      if(!channel->expr_.get())
      {
        out << "[" << channel->channel_params_.channel_id << "]";
      }
      else
      {
        print_expression(out, *(channel->expr_));
      }
    }

    inline
    bool
    channel_equal(NonLinkedExpressionChannel* left,
      NonLinkedExpressionChannel* right,
      bool status_check = true)
      noexcept
    {
      return ((left->expr_.get() && right->expr_.get() &&
        *(left->expr_) == *(right->expr_)) ||
        (!left->expr_.get() && !right->expr_.get())) &&
        left->channel_params_.equal(right->channel_params_, status_check);
    }

    inline
    bool NonLinkedExpressionChannel::Expression::operator==(
      const Expression& right) const
      noexcept
    {
      return op == right.op &&
        (op != NOP || channel_id == right.channel_id) &&
        sub_channels.size() == right.sub_channels.size() &&
        std::equal(sub_channels.begin(),
          sub_channels.end(), right.sub_channels.begin());
    }

    inline
    void NonLinkedExpressionChannel::Expression::swap(
      Expression& right)
      noexcept
    {
      std::swap(op, right.op);
      std::swap(channel_id, right.channel_id);
      sub_channels.swap(right.sub_channels);
    }

    inline
    void
    NonLinkedExpressionChannel::Expression::all_channels(
      ChannelIdSet& channels) const noexcept
    {
      if(op == NOP)
      {
        if(channel_id)
        {
          channels.insert(channel_id);
        }
      }
      else
      {
        for(ExpressionList::const_iterator eit = sub_channels.begin();
            eit != sub_channels.end(); ++eit)
        {
          eit->all_channels(channels);
        }
      }
    }
  }
}

#endif /*_NONLINKEDEXPRESSIONCHANNEL_HPP_*/
