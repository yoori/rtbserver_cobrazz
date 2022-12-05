#ifndef _BASETYPE_HPP_
#define _BASETYPE_HPP_

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

/*
 * BaseType ->
 *   as_descriptor: BaseDescriptor ->
 *     as_simple: SimpleDescriptor
 *     as_struct: StructDescriptor
 *     as_complete_template: CompleteTemplateDescriptor
 *   as_reader: BaseReader ->
 *     as_simple_reader: SimpleReader
 *     as_struct_reader: StructReader
 *   as_writer: BaseWriter ->
 *     as_simple_writer: SimpleWriter
 *     as_struct_writer: StructWriter
 *   as_template: BaseTemplate
 */
namespace Declaration
{
  class BaseDescriptor;
  typedef ReferenceCounting::SmartPtr<BaseDescriptor>
    BaseDescriptor_var;

  class BaseReader;
  typedef ReferenceCounting::SmartPtr<BaseReader>
    BaseReader_var;

  class BaseWriter;
  typedef ReferenceCounting::SmartPtr<BaseWriter>
    BaseWriter_var;

  class BaseTemplate;
  typedef ReferenceCounting::SmartPtr<BaseTemplate>
    BaseTemplate_var;

  struct BaseType: public virtual ReferenceCounting::Interface
  {
    BaseType(const char* name_val);
    
    const char* name() const; // local name

    virtual BaseDescriptor_var as_descriptor() noexcept;

    virtual BaseReader_var as_reader() noexcept;

    virtual BaseWriter_var as_writer() noexcept;

    virtual BaseTemplate_var as_template() noexcept;

  protected:
    virtual ~BaseType() noexcept {}
    
  private:
    std::string name_;
  };

  typedef ReferenceCounting::SmartPtr<BaseType> BaseType_var;
}

#endif
