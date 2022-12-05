#include <iostream>
#include <Stream/MemoryStream.hpp>

#include "CountActiveObject.hpp"

namespace AdServer
{
namespace Commons
{
  /** CountActiveObject */
  CountActiveObject::CountActiveObject() noexcept
    : active_count_(0)
  {
  }

  int
  CountActiveObject::active_count() const noexcept
  {
    return active_count_;
  }

  bool
  CountActiveObject::wait_more_() noexcept
  {
    return active_count_ > 0;
  }

  bool
  CountActiveObject::add_active_count(int inc, bool ignore_state) noexcept
  {
    if (ignore_state || active() || inc < 0)
    {
      int new_count = active_count_.exchange_and_add(inc) + inc;
      assert(new_count >= 0); // based on current use cases
      if (!active() && new_count <= 0)
      {
        Sync::PosixGuard guard(cond_);
        cond_.broadcast();
      }

      return true;
    }

    return false;
  }
}
}
