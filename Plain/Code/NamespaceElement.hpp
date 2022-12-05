#ifndef _CODE_NAMESPACEELEMENT_HPP_
#define _CODE_NAMESPACEELEMENT_HPP_

#include <Declaration/Namespace.hpp>
#include "Element.hpp"

namespace Code
{
  class NamespaceElement: public Element
  {
  public:
    NamespaceElement(
      NamespaceElement* owner,
      Declaration::Namespace* namespace_decl) noexcept;

    Declaration::Namespace_var namespace_decl() const noexcept;

    ElementList_var elements() const noexcept;

    void add_element(Element* elem) noexcept;

    ReferenceCounting::SmartPtr<NamespaceElement>
    owner() noexcept;

    virtual void visited(ElementVisitor* visitor) const noexcept;

  protected:
    virtual ~NamespaceElement() noexcept {}
    
  private:
    NamespaceElement* owner_;
    Declaration::Namespace_var namespace_;
    ElementList_var elements_;
  };  

  typedef ReferenceCounting::SmartPtr<NamespaceElement>
    NamespaceElement_var;
}

namespace Code
{
  inline
  NamespaceElement::NamespaceElement(
    NamespaceElement* owner,
    Declaration::Namespace* namespace_decl) noexcept
    : owner_(owner),
      namespace_(ReferenceCounting::add_ref(namespace_decl)),
      elements_(new ElementList)
  {}

  inline
  Declaration::Namespace_var
  NamespaceElement::namespace_decl() const noexcept
  {
    return namespace_;
  }

  inline
  ElementList_var
  NamespaceElement::elements() const noexcept
  {
    return elements_;
  }

  inline
  void
  NamespaceElement::add_element(Element* elem) noexcept
  {
    elements_->push_back(ReferenceCounting::add_ref(elem));
  }

  inline
  ReferenceCounting::SmartPtr<NamespaceElement>
  NamespaceElement::owner() noexcept
  {
    return ReferenceCounting::add_ref(owner_);
  }

  inline
  void
  NamespaceElement::visited(ElementVisitor* visitor) const noexcept
  {
    visitor->visit_i(this);
  }
}

#endif /*_CODE_NAMESPACEELEMENT_HPP_*/
