#ifndef PLAIN_CPP_DESCRIPTORGENERATOR_HPP
#define PLAIN_CPP_DESCRIPTORGENERATOR_HPP

#include <string>
#include <iostream>

#include <Declaration/SimpleDescriptor.hpp>
#include <Declaration/SimpleReader.hpp>
#include <Declaration/SimpleWriter.hpp>
#include <Declaration/StructDescriptor.hpp>
#include <Declaration/CompleteTemplateDescriptor.hpp>

namespace Cpp
{
  /* DescriptorGenerator:
   *   generate descriptor struct with field offsets
   *   generate _WriterBase struct: container for load and resave value
   *     inside writer that don't declare accessor for field with this type
   */
  class DescriptorGenerator
  {
  public:
    DescriptorGenerator(
      std::ostream& out_hpp,
      std::ostream& out_cpp,
      const char* offset)
      noexcept;

    void generate_decl(
      Declaration::StructDescriptor* struct_descriptor)
      noexcept;

    void generate_impl(
      Declaration::StructDescriptor* struct_descriptor)
      noexcept;

  private:
    void generate_descriptor_base_decl_(
      const Declaration::StructDescriptor* descriptor) noexcept;

  private:
    std::ostream& out_;
    std::ostream& out_cpp_;
    std::string offset_;
  };
}

#endif /*PLAIN_CPP_DESCRIPTORGENERATOR_HPP*/
