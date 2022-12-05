#ifndef _BASEREADER_HPP_
#define _BASEREADER_HPP_

#include <list>
#include "BaseType.hpp"
#include "BaseDescriptor.hpp"

namespace Declaration
{
  class SimpleReader;
  typedef ReferenceCounting::SmartPtr<SimpleReader>
    SimpleReader_var;

  class StructReader;
  typedef ReferenceCounting::SmartPtr<StructReader>
    StructReader_var;

  class BaseReader: public virtual BaseType
  {
  public:
    BaseReader(const char* name_val) noexcept;

    virtual BaseDescriptor_var descriptor() noexcept = 0;

    virtual SimpleReader_var as_simple_reader() noexcept;

    virtual StructReader_var as_struct_reader() noexcept;

    virtual BaseReader_var as_reader() noexcept;
  };

  typedef ReferenceCounting::SmartPtr<BaseReader>
    BaseReader_var;

  typedef std::list<BaseReader_var> BaseReaderList;
}

#endif /*_BSEREADER_HPP_*/
