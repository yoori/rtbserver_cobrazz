
#pragma once

#include <Sync/Condition.hpp>
#include <Generics/Time.hpp>

class SecondStartWaiter
{
  static const unsigned short WAIT_DURATION = 1; // 1 second

protected:
  typedef Sync::ConditionalGuard Guard_;
  typedef Sync::Condition Condition_;

  mutable Condition_ condition_;


public:
  SecondStartWaiter();

  ~SecondStartWaiter();

private:
  Generics::Time second_start_time_;
};
