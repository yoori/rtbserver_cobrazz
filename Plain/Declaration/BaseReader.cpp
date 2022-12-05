#include "BaseReader.hpp"
#include "SimpleReader.hpp"
#include "StructReader.hpp"

namespace Declaration
{
  BaseReader::BaseReader(const char* name_val) noexcept
    : BaseType(name_val)
  {}

  BaseReader_var
  BaseReader::as_reader() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  SimpleReader_var
  BaseReader::as_simple_reader() noexcept
  {
    return SimpleReader_var();
  }

  StructReader_var
  BaseReader::as_struct_reader() noexcept
  {
    return StructReader_var();
  }
}
