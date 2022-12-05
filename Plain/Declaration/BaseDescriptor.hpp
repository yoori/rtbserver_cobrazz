#ifndef PLAIN_DECLARATION_BASEDESCRIPTOR_HPP
#define PLAIN_DECLARATION_BASEDESCRIPTOR_HPP

#include <list>
#include "BaseType.hpp"

namespace Declaration
{
  class SimpleDescriptor;
  typedef ReferenceCounting::SmartPtr<SimpleDescriptor>
    SimpleDescriptor_var;

  class StructDescriptor;
  typedef ReferenceCounting::SmartPtr<StructDescriptor>
    StructDescriptor_var;

  class TemplateDescriptor;
  typedef ReferenceCounting::SmartPtr<TemplateDescriptor>
    TemplateDescriptor_var;

  class CompleteTemplateDescriptor;
  typedef ReferenceCounting::SmartPtr<CompleteTemplateDescriptor>
    CompleteTemplateDescriptor_var;

  typedef unsigned long SizeType;

  class BaseDescriptor: public virtual BaseType
  {
  public:
    BaseDescriptor(const char* name_val);
    
    virtual SimpleDescriptor_var as_simple() noexcept;

    virtual StructDescriptor_var as_struct() noexcept;

    virtual CompleteTemplateDescriptor_var as_complete_template() noexcept;

    virtual bool is_fixed() const noexcept = 0;

    virtual SizeType fixed_size() const noexcept = 0;

    virtual BaseDescriptor_var as_descriptor() noexcept;

  protected:
    virtual ~BaseDescriptor() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<BaseDescriptor>
    BaseDescriptor_var;

  typedef std::list<BaseDescriptor_var> BaseDescriptorList;
}

#endif /*PLAIN_DECLARATION_BASEDESCRIPTOR_HPP*/
