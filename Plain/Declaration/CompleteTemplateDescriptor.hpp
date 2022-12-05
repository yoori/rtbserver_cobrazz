#ifndef PLAIN_DECLARATION_COMPLETETEMPLATEDESCRIPTOR_HPP
#define PLAIN_DECLARATION_COMPLETETEMPLATEDESCRIPTOR_HPP

#include "BaseDescriptor.hpp"
#include "BaseReader.hpp"
#include "BaseWriter.hpp"

namespace Declaration
{
  class CompleteTemplateDescriptor: public virtual BaseDescriptor
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidParam, Exception);

    CompleteTemplateDescriptor(
      const char* name,
      const BaseDescriptorList& args)
      noexcept;

    virtual bool is_fixed() const noexcept = 0;

    virtual SizeType fixed_size() const noexcept = 0;

    CompleteTemplateDescriptor_var
    as_complete_template() noexcept;

    BaseReader_var
    complete_template_reader(const BaseReaderList& args)
      /*throw(InvalidParam)*/;

    BaseWriter_var
    complete_template_writer(const BaseWriterList& args)
      /*throw(InvalidParam)*/;

    const BaseDescriptorList& args() const noexcept;

  protected:
    virtual ~CompleteTemplateDescriptor() noexcept {}

    virtual BaseReader_var
    create_template_reader_(const BaseReaderList& args)
      /*throw(InvalidParam)*/ = 0;

    virtual BaseWriter_var
    create_template_writer_(const BaseWriterList& args)
      /*throw(InvalidParam)*/ = 0;

  private:
    BaseDescriptorList args_;
  };

  typedef ReferenceCounting::SmartPtr<CompleteTemplateDescriptor>
    CompleteTemplateDescriptor_var;
}

#endif /*PLAIN_DECLARATION_COMPLETETEMPLATEDESCRIPTOR_HPP*/
