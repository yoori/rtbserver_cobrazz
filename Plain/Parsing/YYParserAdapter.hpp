#ifndef PARSING_YYPARSERADAPTER_HPP
#define PARSING_YYPARSERADAPTER_HPP

#include "Processor.hpp"
#include "YYScanner.hpp"

struct YYParserAdapter
{
  Parsing::Processor_var processor;
  yy::PlainScanner_var scanner;
};

#endif /*PARSING_YYPARSERADAPTER_HPP*/
