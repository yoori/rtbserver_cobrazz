#ifndef PLAIN_PARSING_YYSCANNER_HPP
#define PLAIN_PARSING_YYSCANNER_HPP

#ifndef YY_DECL
# define YY_DECL \
    yy::PlainParser::token_type \
    yy::PlainScanner::lex( \
      yy::PlainParser::semantic_type* yylval, \
      yy::PlainParser::location_type* yylloc)

#endif

#ifndef __FLEX_LEXER_H
# define yyFlexLexer PlainFlexLexer
# include "FlexLexer.h"
# undef yyFlexLexer
#endif

#include <iostream>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Parsing/YYParser.yy.hpp>

namespace yy
{
  class PlainScanner:
    public PlainFlexLexer,
    public ReferenceCounting::DefaultImpl<>
  {
  public:
    PlainScanner(std::istream* in, std::ostream* out);

    /* lex adapter */
    virtual yy::PlainParser::token_type lex(
      yy::PlainParser::semantic_type* yyval,
      yy::location* loc);

    void set_debug(bool debug);

  protected:
    virtual ~PlainScanner() noexcept;
  };

  typedef ReferenceCounting::SmartPtr<PlainScanner>
    PlainScanner_var;
}

#endif /*PLAIN_PARSING_YYSCANNER_HPP*/
