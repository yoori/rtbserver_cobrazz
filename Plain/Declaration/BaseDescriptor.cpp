#include "BaseDescriptor.hpp"
#include "SimpleDescriptor.hpp"
#include "StructDescriptor.hpp"
#include "CompleteTemplateDescriptor.hpp"

namespace Declaration
{
  BaseDescriptor::BaseDescriptor(const char* name_val)
    : BaseType(name_val)
  {}

  BaseDescriptor_var
  BaseDescriptor::as_descriptor() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  SimpleDescriptor_var
  BaseDescriptor::as_simple() noexcept
  {
    return SimpleDescriptor_var();
  }

  StructDescriptor_var
  BaseDescriptor::as_struct() noexcept
  {
    return StructDescriptor_var();
  }

  CompleteTemplateDescriptor_var
  BaseDescriptor::as_complete_template() noexcept
  {
    return CompleteTemplateDescriptor_var();
  }
}
