#ifndef PLAIN_DECLARATION_BASETEMPLATE_HPP
#define PLAIN_DECLARATION_BASETEMPLATE_HPP

#include "BaseType.hpp"
#include "BaseDescriptor.hpp"

namespace Declaration
{
  class BaseTemplate: public BaseType
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidParam, Exception);

    BaseTemplate(const char* name, unsigned long args_count)
      noexcept;

    virtual BaseTemplate_var as_template() noexcept;

    unsigned long args() const noexcept;

    CompleteTemplateDescriptor_var
    complete_template_descriptor(
      const BaseDescriptorList& args) const
      /*throw(InvalidParam)*/;

  protected:
    virtual ~BaseTemplate() noexcept {}

    virtual CompleteTemplateDescriptor_var
    create_template_descriptor_(
      const char* name,
      const BaseDescriptorList& args) const
      /*throw(InvalidParam)*/ = 0;

  private:
    unsigned long args_count_;
  };

  typedef ReferenceCounting::SmartPtr<BaseTemplate>
    BaseTemplate_var;
}

#endif /*PLAIN_DECLARATION_BASETEMPLATE_HPP*/
