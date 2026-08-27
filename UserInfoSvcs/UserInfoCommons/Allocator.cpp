#include "Allocator.hpp"

namespace AdServer::UserInfoSvcs
{
  Generics::Allocator::FixedBase_var MembufAllocator::ALLOCATOR(new Generics::Allocator::Default);

  /*
  Generics::Allocator::FixedBase_var MembufAllocator::ALLOCATOR(
    new Generics::Allocator::Universal);
  */
}
