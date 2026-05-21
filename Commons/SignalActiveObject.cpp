#include "SignalActiveObject.hpp"

#include <pthread.h>

namespace AdServer::Commons
{
  SignalActiveObject::SignalActiveObject() noexcept
  {
    fill_signals_();
    pthread_sigmask(SIG_BLOCK, &signals_, nullptr);
  }

  void
  SignalActiveObject::wait_object() noexcept
  {
    while (true)
    {
      int signal = 0;
      if (sigwait(&signals_, &signal) == 0 &&
        (signal == SIGINT || signal == SIGTERM || signal == SIGQUIT))
      {
        return;
      }
    }
  }

  void
  SignalActiveObject::fill_signals_() noexcept
  {
    sigemptyset(&signals_);
    sigaddset(&signals_, SIGINT);
    sigaddset(&signals_, SIGTERM);
    sigaddset(&signals_, SIGQUIT);
  }
}
