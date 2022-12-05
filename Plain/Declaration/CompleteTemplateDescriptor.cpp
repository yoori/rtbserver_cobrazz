#include <sstream>
#include <cstring>
#include "CompleteTemplateDescriptor.hpp"

namespace Declaration
{
  CompleteTemplateDescriptor::CompleteTemplateDescriptor(
    const char* name,
    const BaseDescriptorList& args)
    noexcept
    : BaseType(name),
      BaseDescriptor(name),
      args_(args)
  {}

  CompleteTemplateDescriptor_var
  CompleteTemplateDescriptor::as_complete_template()
    noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  const BaseDescriptorList&
  CompleteTemplateDescriptor::args() const noexcept
  {
    return args_;
  }

  BaseReader_var
  CompleteTemplateDescriptor::complete_template_reader(
    const BaseReaderList& args)
    /*throw(InvalidParam)*/
  {
    if(args.size() != args_.size())
    {
      std::ostringstream ostr;
      ostr << "can't init template reader - incorrect number of arguments: " <<
        args.size() << " instead " << args_.size();
      throw InvalidParam(ostr.str());
    }

    int arg_i = 0;
    BaseDescriptorList::const_iterator dit = args_.begin();
    for(BaseReaderList::const_iterator rit = args.begin();
        rit != args.end(); ++rit, ++dit, ++arg_i)
    {
      if(::strcmp((*rit)->descriptor()->name(), (*dit)->name()) != 0)
      {
        std::ostringstream ostr;
        ostr << "can't init template reader - argument #" << arg_i <<
          ": type '" << (*rit)->name() << "' isn't reader of '" <<
          (*dit)->name() << "', it is reader of '" <<
          (*rit)->descriptor()->name() << "'";
        throw InvalidParam(ostr.str());
      }    
    }

    return create_template_reader_(args);
  }

  BaseWriter_var
  CompleteTemplateDescriptor::complete_template_writer(
    const BaseWriterList& args)
    /*throw(InvalidParam)*/
  {
    if(args.size() != args_.size())
    {
      std::ostringstream ostr;
      ostr << "can't init template writer - incorrect number of arguments: " <<
        args.size() << " instead " << args_.size();
      throw InvalidParam(ostr.str());
    }

    int arg_i = 0;
    BaseDescriptorList::const_iterator dit = args_.begin();
    for(BaseWriterList::const_iterator rit = args.begin();
        rit != args.end(); ++rit, ++dit, ++arg_i)
    {
      if(::strcmp((*rit)->descriptor()->name(), (*dit)->name()) != 0)
      {
        std::ostringstream ostr;
        ostr << "can't init template writer - argument #" << arg_i <<
          ": type '" << (*rit)->name() << "' isn't writer of '" <<
          (*dit)->name() << "', it is reader of '" <<
          (*rit)->descriptor()->name() << "'";
        throw InvalidParam(ostr.str());
      }    
    }

    return create_template_writer_(args);
  }
}
