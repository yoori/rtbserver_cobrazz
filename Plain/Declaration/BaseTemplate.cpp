#include <sstream>
#include "CompleteTemplateDescriptor.hpp"
#include "BaseTemplate.hpp"

namespace Declaration
{
  BaseTemplate::BaseTemplate(
    const char* name,
    unsigned long args_count_val)
    noexcept
    : BaseType(name),
      args_count_(args_count_val)
  {}

  BaseTemplate_var
  BaseTemplate::as_template() noexcept
  {
    return ReferenceCounting::add_ref(this);
  }

  unsigned long
  BaseTemplate::args() const noexcept
  {
    return args_count_;
  }

  CompleteTemplateDescriptor_var
  BaseTemplate::complete_template_descriptor(
    const BaseDescriptorList& args) const
    /*throw(InvalidParam)*/
  {
    if(args.size() != args_count_)
    {
      std::ostringstream ostr;
      ostr << "can't init template descriptor - incorrect number of arguments: " <<
        args.size() << " instead " << args_count_;
      throw InvalidParam(ostr.str());
    }

    std::ostringstream name_ostr;
    name_ostr << name() << "<";
    for(BaseDescriptorList::const_iterator dit =
          args.begin();
        dit != args.end(); ++dit)
    {
      if(dit != args.begin())
      {
        name_ostr << ",";
      }
      name_ostr << (*dit)->name();
    }
    name_ostr << ">";

    return create_template_descriptor_(name_ostr.str().c_str(), args);
  }
}
