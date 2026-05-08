#pragma once

#include "LoadingProgressCallbackBase.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  class LoadingProgressCallback
    : public LoadingProgressCallbackBase
  {
  public:
    LoadingProgressCallback(
      LoadingProgressCallbackBase* parent,
      double range) noexcept;

    void post_progress(double value) noexcept;

    void loading_is_finished() noexcept;

  private:
    LoadingProgressCallbackBase_var parent_;
    double range_;
    double current_value_;
    unsigned int last_reported_chunk_;
  };
}
}
