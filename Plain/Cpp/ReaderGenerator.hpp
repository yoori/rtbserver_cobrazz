#ifndef PLAIN_CPP_READERGENERATOR_HPP
#define PLAIN_CPP_READERGENERATOR_HPP

#include <string>
#include <iostream>

#include <Declaration/SimpleDescriptor.hpp>
#include <Declaration/SimpleReader.hpp>
#include <Declaration/StructDescriptor.hpp>
#include <Declaration/StructReader.hpp>
#include <Declaration/CompleteTemplateDescriptor.hpp>

namespace Cpp
{
  class ReaderGenerator
  {
  public:
    ReaderGenerator(std::ostream& out, const char* offset)
      noexcept;

    void generate_decl(
      Declaration::StructReader* struct_reader)
      noexcept;

    void generate_impl(
      Declaration::StructReader* struct_reader)
      noexcept;

  private:
    void generate_ctor_impl_(
      const Declaration::StructReader* struct_reader)
      noexcept;

    void generate_field_funs_impl_(
      const Declaration::StructReader* struct_reader)
      noexcept;

  private:
    std::ostream& out_;
    std::string offset_;
  };
}

#endif /*PLAIN_CPP_READERGENERATOR_HPP*/
