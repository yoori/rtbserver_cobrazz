#include "Processor.hpp"
#include "YYParserAdapter.hpp"
#include <Parsing/YYParser.yy.hpp>
#include "Parser.hpp"

namespace Parsing
{
  bool Parser::parse(
    std::ostream& error_ostr,
    Code::ElementList* elements,
    std::istream& istr,
    Declaration::Namespace_var* root_namespace)
    /*throw(Exception)*/
  {
    Parsing::Processor_var processor = new Parsing::Processor(elements);

    YYParserAdapter parse_adapter;
    parse_adapter.processor = processor;
    parse_adapter.scanner = new yy::PlainScanner(&istr, &error_ostr);
    yy::PlainParser parser(parse_adapter);
    if(!parser.parse())
    {
      if(root_namespace)
      {
        *root_namespace = processor->root_namespace();
      }

      return true;
    }

    return false;
  }
}
