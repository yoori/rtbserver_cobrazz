%{
# include <string>
# include <Parsing/YYParser.yy.hpp>
# include <Parsing/YYScanner.hpp>

# undef yywrap
# define yywrap() 1

# define yyterminate() return token::PTOKEN_END

# define YY_NO_UNISTD_H

  typedef yy::PlainParser::token token;
  typedef yy::PlainParser::token_type token_type;
%}

%option c++
%option prefix="Plain"
%option noyywrap nounput batch debug

%{
# define YY_USER_ACTION yylloc->columns(yyleng);
%}

D [0-9]
L [a-zA-Z_]

%x ML_COMMENT

%%

%{
  yylloc->step();
%}

"/*" BEGIN(ML_COMMENT);
<ML_COMMENT>[^*\n]*
<ML_COMMENT>"*"+[^*/\n]*
<ML_COMMENT>\n { yylloc->lines(yyleng); yylloc->step(); }
<ML_COMMENT>"*"+"/" BEGIN(INITIAL);

"//"[^\n]*\n { yylloc->lines(1); yylloc->step(); }

"typedef" { return token::PTOKEN_TYPEDEF; }
"struct" { return token::PTOKEN_STRUCT; }
"reader" { return token::PTOKEN_READER; }
"writer" { return token::PTOKEN_WRITER; }
"autoreader" { return token::PTOKEN_AUTOREADER; }
"autowriter" { return token::PTOKEN_AUTOWRITER; }
"namespace" { return token::PTOKEN_NAMESPACE; }

{L}({L}|{D})* {
  yylval->identifier = new std::string(YYText());
  return token::PTOKEN_IDENTIFIER;
}

("{"|"<%") { return token_type('{'); }
("}"|"%>") { return token_type('}'); }

[ \t\v\f]+ { yylloc->step(); }
\n { yylloc->lines(yyleng); yylloc->step(); }
. { return token_type(yytext[0]); }

%%

namespace yy
{
  PlainScanner::PlainScanner(
    std::istream* in,
    std::ostream* out)
    : PlainFlexLexer(in, out)
  {}

  PlainScanner::~PlainScanner() throw()
  {}

  void PlainScanner::set_debug(bool debug)
  {
    yy_flex_debug = debug;
  }
}

#ifdef yylex
#undef yylex
#endif

int PlainFlexLexer::yylex()
{
  assert(0);
  return 0;
}
