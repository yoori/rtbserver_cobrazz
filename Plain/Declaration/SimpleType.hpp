#ifndef PLAIN_DECLARATION_SIMPLETYPE_HPP
#define PLAIN_DECLARATION_SIMPLETYPE_HPP

#include <ReferenceCounting/DefaultImpl.hpp>
#include "SimpleDescriptor.hpp"
#include "SimpleReader.hpp"
#include "SimpleWriter.hpp"

namespace Declaration
{
  /* SimpleType is descriptor, reader and writer with one name
   *   requirement : descriptor must return writer at as_writer
   */
  class SimpleType:
    public virtual ReferenceCounting::DefaultImpl<>,
    public virtual BaseType,
    public SimpleDescriptor,
    public SimpleReader,
    public SimpleWriter
  {
  public:
    SimpleType(
      const char* name,
      bool is_fixed_val,
      SizeType fixed_size_val,
      const SimpleReader::CppReadTraits& cpp_read_traits,
      SimpleWriter::CppWriteTraitsGenerator* cpp_write_traits_generator);

    /* BaseType */
    virtual BaseDescriptor_var as_descriptor() noexcept;

    virtual BaseReader_var as_reader() noexcept;

    virtual BaseWriter_var as_writer() noexcept;

    /* BaseDescriptor */
    virtual SimpleDescriptor_var as_simple() noexcept;

    /* BaseReader/BaseWriter */
    virtual BaseDescriptor_var descriptor() noexcept;

  protected:
    virtual ~SimpleType() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<SimpleType>
    SimpleType_var;
}

namespace Declaration
{
  inline
  SimpleType::SimpleType(
    const char* name_val,
    bool is_fixed_val,
    SizeType fixed_size_val,
    const SimpleReader::CppReadTraits& cpp_read_traits,
    SimpleWriter::CppWriteTraitsGenerator* cpp_write_traits_generator)
    : BaseType(name_val),
      BaseDescriptor(name_val),
      BaseReader(name_val),
      SimpleDescriptor(name_val, is_fixed_val, fixed_size_val),
      SimpleReader(name_val, cpp_read_traits),
      SimpleWriter(name_val, cpp_write_traits_generator)
  {}

  inline
  BaseDescriptor_var
  SimpleType::as_descriptor() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  inline
  BaseReader_var
  SimpleType::as_reader() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  inline
  BaseWriter_var
  SimpleType::as_writer() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  inline
  SimpleDescriptor_var
  SimpleType::as_simple() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  inline
  BaseDescriptor_var
  SimpleType::descriptor() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }
}

#endif /*PLAIN_DECLARATION_SIMPLETYPE_HPP*/
