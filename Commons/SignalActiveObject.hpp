#pragma once

#include <csignal>

namespace AdServer::Commons
{
  class SignalActiveObject final
  {
  public:
    SignalActiveObject() noexcept;

    void wait_object() noexcept;

  private:
    void fill_signals_() noexcept;

    sigset_t signals_;
  };
}
