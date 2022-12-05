#ifndef PLAIN_CPP_UTILS_HPP
#define PLAIN_CPP_UTILS_HPP

#include <string>
#include <iostream>
#include <Declaration/StructDescriptor.hpp>
#include <Declaration/StructWriter.hpp>
#include <Declaration/SimpleType.hpp>

namespace Cpp
{
namespace Utils
{
  template<typename FieldListType, typename FieldOpsType>
  void
  fetch_fields_with_fixed_sum(
    const FieldListType& fields,
    const FieldOpsType& field_ops)
  {
    unsigned long fixed_buf_size = 0;
    unsigned long fixed_buf_i = 0;
    std::string first_fixed_field_name;

    for(typename FieldListType::const_iterator fit = fields.begin();
        fit != fields.end(); ++fit)
    {
      Declaration::StructDescriptor_var struct_descriptor =
        (*fit)->descriptor()->as_struct();

      if(!(*fit)->descriptor()->is_fixed() || struct_descriptor.in())
      {
        if(fixed_buf_size)
        {
          field_ops.process_fixed_sum(
            fixed_buf_i, fixed_buf_size, first_fixed_field_name.c_str());

          fixed_buf_size = 0;
          ++fixed_buf_i;
        }

        first_fixed_field_name.clear();

        field_ops.process_non_fixed_or_complex(*fit);
      }
      else
      {
        if(first_fixed_field_name.empty())
        {
          first_fixed_field_name = (*fit)->name();
        }

        fixed_buf_size += (*fit)->descriptor()->fixed_size();
      }
    }

    if(fixed_buf_size)
    {
      field_ops.process_fixed_sum(
        fixed_buf_i, fixed_buf_size, first_fixed_field_name.c_str());
    }
  }

  struct HolderTypeOfField
  {
    // can be used for get default holder types (without specifiers)
    // fixed types sizes fetch
    std::string
    operator()(const Declaration::StructDescriptor::PosedField* field) const
    {
      Declaration::BaseDescriptor_var field_descriptor =
        field->descriptor();

      Declaration::BaseWriter_var field_writer =
        field_descriptor->as_writer();

      if(field_writer.in())
      {
        Declaration::SimpleWriter_var simple_field_writer =
          field_writer->as_simple_writer();
        assert(simple_field_writer.in());

        Declaration::SimpleWriter::CppWriteTraitsGenerator_var
          cpp_write_traits_generator =
            simple_field_writer->cpp_write_traits_generator();

        return cpp_write_traits_generator->generate(
          Declaration::MappingSpecifierSet())->holder_type_name;
      }
      else
      {
        Declaration::StructDescriptor_var struct_field_descriptor =
          field_descriptor->as_struct();
        if(struct_field_descriptor.in())
        {
          return std::string(struct_field_descriptor->name()) +
            PROTECTED_WRITER_SUFFIX;
        }
      }

      assert(0);
      return "";
    }

    std::string
    operator()(const Declaration::StructWriter::FieldWriter* field) const
    {
      Declaration::BaseWriter_var field_writer = field->writer();
      Declaration::SimpleWriter_var simple_field_writer =
        field_writer->as_simple_writer();

      if(simple_field_writer.in())
      {
        Declaration::SimpleWriter::CppWriteTraitsGenerator_var
          cpp_write_traits_generator =
            simple_field_writer->cpp_write_traits_generator();

        return cpp_write_traits_generator->generate(
          field->mapping_specifiers())->holder_type_name;

        /*
        Declaration::SimpleDescriptor_var simple_descriptor =
          simple_field_writer->descriptor()->as_simple();
        assert(simple_descriptor.in());
        return simple_descriptor->cpp_traits().holder_type_name.c_str();
        */
      }
      else
      {
        Declaration::StructWriter_var struct_field_writer =
          field_writer->as_struct_writer();
        if(struct_field_writer.in())
        {
          return struct_field_writer->name();
        }
      }

      assert(0);
      return std::string();
    }
  };
}
}

#endif /*PLAIN_CPP_UTILS_HPP*/
