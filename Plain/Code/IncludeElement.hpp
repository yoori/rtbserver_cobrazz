#ifndef _CODE_INCLUDEELEMENT_HPP_
#define _CODE_INCLUDEELEMENT_HPP_

#include "Element.hpp"

namespace Code
{
  class IncludeElement: public Element
  {
  public:
    IncludeElement(const char* file_val) noexcept;

    const char* file() const noexcept;

    virtual void visited(ElementVisitor* visitor) const noexcept;

  protected:
    virtual ~IncludeElement() noexcept {}
    
  private:
    std::string file_;
  };
}

namespace Code
{
  inline
  IncludeElement::IncludeElement(const char* file_val) noexcept
    : file_(file_val)
  {}

  inline
  const char*
  IncludeElement::file() const noexcept
  {
    return file_.c_str();
  }
  
  inline
  void
  IncludeElement::visited(ElementVisitor* visitor) const noexcept
  {
    visitor->visit_i(this);
  }
}

#endif /*_CODE_INCLUDEELEMENT_HPP_*/
