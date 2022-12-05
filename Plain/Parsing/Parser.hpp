#ifndef PLAIN_PARSER_HPP
#define PLAIN_PARSER_HPP

#include <iostream>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Code/Element.hpp>
#include <Declaration/Namespace.hpp>

namespace Parsing
{
  class Parser:
    public ReferenceCounting::DefaultImpl<>
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    bool parse(
      std::ostream& error_ostr,
      Code::ElementList* elements,
      std::istream& istr,
      Declaration::Namespace_var* root_namespace = 0)
      /*throw(Exception)*/;
  };

  typedef ReferenceCounting::SmartPtr<Parser> Parser_var;
}

#endif /*PLAIN_PARSER_HPP*/
