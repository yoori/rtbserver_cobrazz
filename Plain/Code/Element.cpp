#include "Element.hpp"
#include "IncludeElement.hpp"
#include "NamespaceElement.hpp"
#include "TypeDefElement.hpp"
#include "TypeElement.hpp"

namespace Code
{  
  void ElementVisitor::visit_i(const IncludeElement* elem) noexcept
  {
    visit_i(static_cast<const Element*>(elem));
  }

  void ElementVisitor::visit_i(const NamespaceElement* elem) noexcept
  {
    visit_i(static_cast<const Element*>(elem));
  }

  void ElementVisitor::visit_i(const TypeDefElement* elem) noexcept
  {
    visit_i(static_cast<const Element*>(elem));
  }  

  void ElementVisitor::visit_i(const TypeElement* elem) noexcept
  {
    visit_i(static_cast<const Element*>(elem));
  }

  void Element::visited(ElementVisitor* visitor) const noexcept
  {
    visitor->visit_i(this);
  }
}
