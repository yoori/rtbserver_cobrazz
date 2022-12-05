#ifndef _CAMPAIGNCOMMONS_EXPRESSIONCHANNELPARSER_HPP_
#define _CAMPAIGNCOMMONS_EXPRESSIONCHANNELPARSER_HPP_

#include <String/StringManip.hpp>

#include "NonLinkedExpressionChannel.hpp"

namespace AdServer
{
  namespace CampaignSvcs
  {
    class ExpressionChannelParser
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      /* parse expression:
       *   attach child channels to channels from
       *   channels container (channel_id -> ExpressionChannelHolder_var) and
       *   insert ExpressionChannelHolder(0) for not found channels
       */
      static
      NonLinkedExpressionChannel_var
      parse(const String::SubString& expression)
        /*throw(Exception)*/;

    private:
      static
      void
      remove_border_parenthesis_(String::SubString& expression)
        /*throw(Exception)*/;

      static
      bool
      parse_op_(
        NonLinkedExpressionChannel::Expression& result_expression,
        const String::SubString& expression,
        NonLinkedExpressionChannel::Operation op)
        /*throw(Exception)*/;

      static
      NonLinkedExpressionChannel::Expression
      parse_expr_(String::SubString expression)
        /*throw(Exception)*/;
    };
  }
}

namespace AdServer
{
namespace CampaignSvcs
{
  inline
  void
  ExpressionChannelParser::remove_border_parenthesis_(
    String::SubString& expression) /*throw(Exception)*/
  {
    if (expression.empty())
    {
      return;
    }

    int remove_bracket_count = 0;

    String::SubString::Pointer border_it = expression.begin();
    String::SubString::Pointer rev_border_it = expression.end() - 1;

    /* search count of brackets that opened at begin and closed at end */
    for (; border_it != rev_border_it; ++border_it)
    {
      if (std::isspace(*border_it))
      {
        continue;
      }

      if (*border_it == '(')
      {
        bool found = false;

        /* search close bracket */
        for (; rev_border_it != border_it; --rev_border_it)
        {
          if (std::isspace(*rev_border_it))
          {
            continue;
          }
          if (*rev_border_it == ')')
          {
            --rev_border_it;
            ++remove_bracket_count;
            found = true;
          }
          break;
        }

        if(rev_border_it == border_it)
        {
          if(!found)
          {
            Stream::Error ostr;
            ostr << "ExpressionChannelParser::parse_op_(): "
              "incorrect brackets number in sub expression: " <<
              expression;
            throw Exception(ostr);
          }
          
          break;
        }

        if (found)
        {
          continue;
        }
      }

      break;
    }

    /* search minimum of bracket count */
    int bracket_count_min = remove_bracket_count;

    for (String::SubString::Pointer it = border_it;
      it != rev_border_it; ++it)
    {
      if (*it == '(')
      {
        ++remove_bracket_count;
      }
      else if (*it == ')')
      {
        if (--remove_bracket_count < bracket_count_min)
        {
          bracket_count_min = remove_bracket_count;
        }
      }
    }

    remove_bracket_count = bracket_count_min;

    if (remove_bracket_count)
    {
      /* clear excess brackets at end */
      {
        int found_brackets = remove_bracket_count;
        String::SubString::Pointer rev_it = expression.end();

        while (*--rev_it != ')' || --found_brackets)
        {
        }

        expression = expression.substr(0, rev_it - expression.begin());
      }

      /* clear excess brackets at begin */
      {
        int found_brackets = remove_bracket_count;
        String::SubString::Pointer it = expression.begin();

        while (*it++ != '(' || --found_brackets)
        {
        }

        expression = expression.substr(it - expression.begin());
      }
    }
  }

  inline
  bool
  ExpressionChannelParser::parse_op_(
    NonLinkedExpressionChannel::Expression& result_expression,
    const String::SubString& expression,
    NonLinkedExpressionChannel::Operation op)
    /*throw(Exception)*/
  {
    static const char* FUN = "ExpressionChannelParser::parse_op_()";

    if (expression.empty())
    {
      throw Exception(
        "ExpressionChannelParser::parse_op_(): expression is empty.");
    }

    if (expression[0] == op)
    {
      Stream::Error ostr;
      ostr << FUN << ": Illegal format of expression = '" << expression << "'";
      throw Exception(ostr);
    }

    unsigned int parenthesis_count = 0;
    const char* start_pos = expression.begin();

    for (const char* pos = start_pos; pos != expression.end(); ++pos)
    {
      if (*pos == op)
      {
        if (!parenthesis_count)
        {
          result_expression.op = op;
          result_expression.sub_channels.push_back(
            parse_expr_(String::SubString(start_pos, pos)));
          start_pos = pos + 1;
        }
      }
      else if (*pos == '(')
      {
        parenthesis_count++;
      }
      else if (*pos == ')')
      {
        parenthesis_count--;
      }
    }

    if (start_pos != expression.begin())
    {
      result_expression.sub_channels.push_back(parse_expr_(
        String::SubString(start_pos, expression.end())));
      return true;
    }

    return false;
  }

  inline
  NonLinkedExpressionChannel::Expression
  ExpressionChannelParser::parse_expr_(String::SubString expression)
    /*throw(Exception)*/
  {
    static const char* FUN = "ExpressionChannelParser::parse_expr_()";

    remove_border_parenthesis_(expression);
    String::StringManip::trim(expression);

    NonLinkedExpressionChannel::Expression result_expression;

    if(!parse_op_(result_expression,
         expression, NonLinkedExpressionChannel::OR) &&
       !parse_op_(result_expression,
         expression, NonLinkedExpressionChannel::AND) &&
       !parse_op_(result_expression,
         expression, NonLinkedExpressionChannel::AND_NOT))
    {
      Stream::Parser istr(expression.data(), expression.size());
      ChannelId channel_id;
      istr >> channel_id;

      if (istr.fail() || !istr.eof())
      {
        Stream::Error ostr;
        ostr << FUN << ": Failed to read channel_id from expression = '" <<
          expression << "'";
        throw Exception(ostr);
      }

      result_expression.op = NonLinkedExpressionChannel::NOP;
      result_expression.channel_id = channel_id;
    }

    return result_expression;
  }

  inline
  NonLinkedExpressionChannel_var
  ExpressionChannelParser::parse(
    const String::SubString& expression_val)
    /*throw(Exception)*/
  {
    NonLinkedExpressionChannel::Expression expression =
      parse_expr_(expression_val);

    return new NonLinkedExpressionChannel(expression);
  }
}
}

#endif /*_CAMPAIGNCOMMONS_EXPRESSIONCHANNELPARSER_HPP_*/
