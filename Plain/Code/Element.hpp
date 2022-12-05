#ifndef _CODE_ELEMENT_HPP_
#define _CODE_ELEMENT_HPP_

#include <list>
#include <iostream>
#include <eh/Exception.hpp>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace Code
{
  class Element;
  class IncludeElement;
  class NamespaceElement;
  class TypeDefElement;
  class TypeElement;

  class ElementVisitor
  {
  public:
    virtual ~ElementVisitor() noexcept {}

    virtual void visit(const Element*) noexcept;

    virtual void visit_i(const Element*) noexcept = 0;

    virtual void visit_i(const IncludeElement*) noexcept;

    virtual void visit_i(const NamespaceElement*) noexcept;

    virtual void visit_i(const TypeDefElement*) noexcept;

    virtual void visit_i(const TypeElement*) noexcept;
  };
  
  class Element: public ReferenceCounting::DefaultImpl<>
  {
  public:
    virtual void visited(ElementVisitor* visitor) const noexcept;
    
  protected:
    virtual ~Element() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<Element> Element_var;

  struct ElementList:
    public std::list<Element_var>,
    public ReferenceCounting::DefaultImpl<>
  {
  protected:
    virtual ~ElementList() noexcept {}
  };
  
  typedef ReferenceCounting::SmartPtr<ElementList>
    ElementList_var;
}

namespace Code
{
  inline
  void ElementVisitor::visit(const Element* elem) noexcept
  {
    elem->visited(this);
  }
}

#endif /*_CODE_ELEMENT_HPP_*/
