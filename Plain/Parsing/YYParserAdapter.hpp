#pragma once

#include "Processor.hpp"
#include "YYScanner.hpp"

struct YYParserAdapter
{
  Parsing::Processor_var processor;
  yy::PlainScanner_var scanner;
};
