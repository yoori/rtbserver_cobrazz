#ifndef _BASEWRITER_HPP_
#define _BASEWRITER_HPP_

#include <list>
#include <set>
#include <eh/Exception.hpp>
#include "BaseType.hpp"

namespace Declaration
{
  typedef std::set<std::string> MappingSpecifierSet;

  class SimpleWriter;
  typedef ReferenceCounting::SmartPtr<SimpleWriter>
    SimpleWriter_var;

  class StructWriter;
  typedef ReferenceCounting::SmartPtr<StructWriter>
    StructWriter_var;

  class BaseWriter: public virtual BaseType
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidMappingSpecifier, Exception);

    BaseWriter(const char* name_val) noexcept;

    virtual BaseDescriptor_var descriptor() noexcept = 0;

    virtual void
    check_mapping_specifiers(
      const MappingSpecifierSet& mapping_specifiers)
      /*throw(InvalidMappingSpecifier)*/ = 0;

    /* non fixed field */
    virtual SimpleWriter_var as_simple_writer() noexcept;

    virtual StructWriter_var as_struct_writer() noexcept;

    virtual BaseWriter_var as_writer() noexcept;

  protected:
    virtual ~BaseWriter() noexcept {}
  };

  typedef ReferenceCounting::SmartPtr<BaseWriter>
    BaseWriter_var;

  typedef std::list<BaseWriter_var> BaseWriterList;
}

#endif /*_BASEWRITER_HPP_*/
