#ifndef USERINFOSVCS_USERINFOCOMMONS_ALLOCATOR_HPP
#define USERINFOSVCS_USERINFOCOMMONS_ALLOCATOR_HPP

#include <Generics/Allocator.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
  namespace UserInfoSvcs
  {
    struct MembufAllocator
    {
      static Generics::Allocator::FixedBase_var ALLOCATOR;
    };

    typedef Generics::MemBufTmpl<MembufAllocator> MemBuf;
    typedef Generics::SmartMemBufTmpl<Generics::MemBuf, MembufAllocator>
      SmartMemBuf;
    typedef Generics::SmartMemBufTmpl<const Generics::MemBuf,
      MembufAllocator> ConstSmartMemBuf;

    typedef Generics::SmartMemBuf* SmartMemBufPtr;
    typedef const Generics::ConstSmartMemBuf* ConstSmartMemBufPtr;

    using Generics::SmartMemBuf_var;
    using Generics::ConstSmartMemBuf_var;
    /*
    typedef ReferenceCounting::ConstPtr<ConstSmartMemBuf>
      ConstSmartMemBuf_var;
    */
  }
}

#endif
