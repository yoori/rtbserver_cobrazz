#include "BaseWriter.hpp"
#include "SimpleWriter.hpp"
#include "StructWriter.hpp"

namespace Declaration
{
  BaseWriter::BaseWriter(const char* name_val) noexcept
    : BaseType(name_val)
  {}

  BaseWriter_var
  BaseWriter::as_writer() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  SimpleWriter_var
  BaseWriter::as_simple_writer() noexcept
  {
    return SimpleWriter_var();
  }

  StructWriter_var
  BaseWriter::as_struct_writer() noexcept
  {
    return StructWriter_var();
  }
}

