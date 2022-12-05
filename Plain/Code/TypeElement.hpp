#ifndef _CODE_TYPEELEMENT_HPP_
#define _CODE_TYPEELEMENT_HPP_

#include <Declaration/BaseType.hpp>
#include "Element.hpp"

namespace Code
{
  class TypeElement: public Element
  {
  public:
    TypeElement(Declaration::BaseType* type_val) noexcept;

    Declaration::BaseType_var type() const noexcept;

    virtual void visited(ElementVisitor* visitor) const noexcept;

  private:
    Declaration::BaseType_var type_;
  };
}

namespace Code
{
  inline
  TypeElement::TypeElement(Declaration::BaseType* type_val)
    noexcept
    : type_(ReferenceCounting::add_ref(type_val))
  {}

  inline
  Declaration::BaseType_var
  TypeElement::type() const noexcept
  {
    return type_;
  }

  inline
  void
  TypeElement::visited(ElementVisitor* visitor) const noexcept
  {
    visitor->visit_i(this);
  }
}

#endif /*_CODE_TYPEELEMENT_HPP_*/
