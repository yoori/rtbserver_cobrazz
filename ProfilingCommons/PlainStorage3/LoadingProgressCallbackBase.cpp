#include "LoadingProgressCallbackBase.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  LoadingProgressCallbackBase::LoadingProgressCallbackBase() noexcept
  {}

  LoadingProgressCallbackBase::~LoadingProgressCallbackBase() noexcept
  {}

  void LoadingProgressCallbackBase::post_progress(double) noexcept
  {}

  void LoadingProgressCallbackBase::loading_is_finished() noexcept
  {}
}
}
