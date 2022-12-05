#ifndef _CODE_TYPEDEFELEMENT_HPP_
#define _CODE_TYPEDEFELEMENT_HPP_

#include <Declaration/BaseType.hpp>
#include "Element.hpp"

namespace Code
{
  class TypeDefElement: public Element
  {
  public:
    TypeDefElement(
      const char* type_name_val,
      Declaration::BaseType* base_type_val)
      noexcept;

    const char* type_name() const noexcept;

    Declaration::BaseType_var base_type() const noexcept;

    virtual void visited(ElementVisitor* visitor) const noexcept;

  protected:
    virtual ~TypeDefElement() noexcept {}
    
  private:
    std::string type_name_;
    Declaration::BaseType_var base_type_;
  };
}

namespace Code
{
  inline
  TypeDefElement::TypeDefElement(
    const char* type_name_val,
    Declaration::BaseType* base_type_val)
    noexcept
    : type_name_(type_name_val),
      base_type_(ReferenceCounting::add_ref(base_type_val))
  {}

  inline
  const char*
  TypeDefElement::type_name() const noexcept
  {
    return type_name_.c_str();
  }

  inline
  Declaration::BaseType_var
  TypeDefElement::base_type() const noexcept
  {
    return base_type_;
  }
  
  inline
  void
  TypeDefElement::visited(ElementVisitor* visitor) const noexcept
  {
    visitor->visit_i(this);
  }
}

#endif /*_CODE_TYPEDEFELEMENT_HPP_*/
