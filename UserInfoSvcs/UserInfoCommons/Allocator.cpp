#include "Allocator.hpp"


namespace AdServer
{
  namespace UserInfoSvcs
  {
    Generics::Allocator::FixedBase_var MembufAllocator::ALLOCATOR(
      new Generics::Allocator::Default);

    /*
    Generics::Allocator::FixedBase_var MembufAllocator::ALLOCATOR(
      new Generics::Allocator::Universal);
    */
  }
}
