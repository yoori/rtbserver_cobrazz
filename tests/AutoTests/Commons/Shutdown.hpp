
#pragma once

#include <Generics/Singleton.hpp>
#include <Sync/Condition.hpp>
#include "Request/BaseRequest.hpp"

namespace AutoTest
{
  DECLARE_EXCEPTION(ShutdownException, eh::DescriptiveException);

  class Shutdown_
  {
  public:

    Shutdown_();

    void set()
      /*throw(eh::Exception)*/;

    void wait(const Time& timeout)
      /*throw(ShutdownException, eh::Exception)*/;

    bool get();


  private:
    Sync::Condition cond_;
    volatile sig_atomic_t value_;
  };

  typedef Generics::Singleton<Shutdown_> Shutdown;
}
