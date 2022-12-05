#ifndef PLAIN_DECLARATION_LISTTEMPLATE_HPP
#define PLAIN_DECLARATION_LISTTEMPLATE_HPP

#include <ReferenceCounting/DefaultImpl.hpp>
#include "BaseTemplate.hpp"
#include "CompleteTemplateDescriptor.hpp"

namespace Declaration
{
  /* BaseArrayTemplate */
  class BaseArrayTemplate:
    public virtual ReferenceCounting::DefaultImpl<>,
    public BaseTemplate
  {
  public:
    BaseArrayTemplate(
      const char* name,
      unsigned long header_size) noexcept;

  protected:
    virtual ~BaseArrayTemplate() noexcept {}
    
    virtual CompleteTemplateDescriptor_var
    create_template_descriptor_(
      const char* name,
      const BaseDescriptorList& args) const
      /*throw(InvalidParam)*/;

  private:
    CompleteTemplateDescriptor_var create_array_simple_type_(
      BaseDescriptor* descriptor) const
      noexcept;

    CompleteTemplateDescriptor_var create_array_struct_type_(
      BaseDescriptor* descriptor) const
      noexcept;

  private:
    unsigned long header_size_;
  };

  /* ArrayTemplate */
  class ArrayTemplate: public BaseArrayTemplate
  {
  public:
    ArrayTemplate() noexcept;

  protected:
    virtual ~ArrayTemplate() noexcept {}
  };

  /* CompatibilityListTemplate */
  class CompatibilityListTemplate: public BaseArrayTemplate
  {
  public:
    CompatibilityListTemplate() noexcept;

  protected:
    virtual ~CompatibilityListTemplate() noexcept {}
  };
}

#endif /*PLAIN_DECLARATION_LISTTEMPLATE_HPP*/
