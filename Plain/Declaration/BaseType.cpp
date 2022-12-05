#include "BaseType.hpp"
#include "BaseDescriptor.hpp"
#include "BaseReader.hpp"
#include "BaseWriter.hpp"
#include "BaseTemplate.hpp"

namespace Declaration
{
  BaseType::BaseType(const char* name_val)
    : name_(name_val)
  {}

  const char*
  BaseType::name() const
  {
    return name_.c_str();
  }

  BaseDescriptor_var
  BaseType::as_descriptor() noexcept
  {
    return BaseDescriptor_var();
  }

  BaseReader_var
  BaseType::as_reader() noexcept
  {
    return BaseReader_var();
  }

  BaseWriter_var
  BaseType::as_writer() noexcept
  {
    return BaseWriter_var();
  }

  BaseTemplate_var
  BaseType::as_template() noexcept
  {
    return BaseTemplate_var();
  }
}

