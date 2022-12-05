#ifndef PLAIN_CPP_WRITERGENERATOR_HPP
#define PLAIN_CPP_WRITERGENERATOR_HPP

#include <string>
#include <iostream>

#include <Declaration/SimpleDescriptor.hpp>
#include <Declaration/SimpleWriter.hpp>
#include <Declaration/StructDescriptor.hpp>
#include <Declaration/StructWriter.hpp>
#include <Declaration/CompleteTemplateDescriptor.hpp>

namespace Cpp
{
  class WriterGenerator
  {
  public:
    WriterGenerator(
      std::ostream& out_hpp,
      std::ostream& out_cpp,
      const char* offset)
      noexcept;

    /* X_DefaultBuffers */
    void generate_default_buffers_decl(
      Declaration::StructDescriptor* struct_descriptor)
      noexcept;

    void generate_default_buffers_impl(
      Declaration::StructDescriptor* struct_descriptor)
      noexcept;

    /* X_ProtectedWriter */
    void generate_protected_decl(
      Declaration::StructDescriptor* struct_descriptor)
      noexcept;

    void generate_protected_impl(
      Declaration::StructDescriptor* struct_descriptor)
      noexcept;

    /* X */
    void generate_decl(
      Declaration::StructWriter* struct_writer)
      noexcept;

    void generate_impl(
      Declaration::StructWriter* struct_writer)
      noexcept;

  private:
    /* X_ProtectedWriter & X */
    void generate_common_funs_decl_(const char* name) noexcept;

    void generate_common_funs_impl_(
      const char* class_name,
      const Declaration::StructDescriptor* struct_descriptor)
      noexcept;

    void generate_swap_impl_(
      const char* class_name,
      Declaration::StructDescriptor* struct_descriptor) noexcept;

    /* X */
    void generate_field_types_decl_(
      Declaration::StructWriter* writer) noexcept;

    void generate_accessors_decl_(
      Declaration::StructWriter* writer) noexcept;

    void generate_accessors_impl_(
      Declaration::StructWriter* writer) noexcept;

  private:
    std::ostream& out_;
    std::ostream& out_cpp_;
    std::string offset_;
  };
}

#endif /*PLAIN_CPP_WRITERGENERATOR_HPP*/
