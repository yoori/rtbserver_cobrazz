#pragma once

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include "OraException.hpp"

namespace AdServer
{
namespace Commons
{
  namespace Oracle
  {
    class TimeoutControl
    {
    public:
      TimeoutControl(const Generics::Time* timeout = 0) noexcept;

      bool sleep_step() /*throw(Exception, TimedOut)*/;

      Generics::Time passed_time() const noexcept;

      Generics::Time timeout() const noexcept;

    private:
      static const Generics::Time MIN_SLEEP_TIME;
      static const Generics::Time MAX_SLEEP_TIME;

    private:
      Generics::Time timeout_;
      Generics::Time start_time_;
    };
  }
}
}
