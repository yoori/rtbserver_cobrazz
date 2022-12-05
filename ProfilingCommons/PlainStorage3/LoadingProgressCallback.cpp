#include <cstdlib>
#include "LoadingProgressCallback.hpp"

namespace
{
  const unsigned int NUMBER_OF_CHUNKS_TO_REPORT = 10000;
}

namespace AdServer
{
namespace ProfilingCommons
{
  LoadingProgressCallback::LoadingProgressCallback(
    LoadingProgressCallbackBase* parent,
    double range) noexcept
    : parent_(ReferenceCounting::add_ref(parent)),
      range_(range),
      current_value_(0.0),
      last_reported_chunk_(0)
  {
    if (abs(range_) < 0.000001)
    {
      range_ = 1.0;
    }
  }

  void LoadingProgressCallback::post_progress(double value) noexcept
  {
    if (current_value_ > range_)
    {
      return;
    }

    current_value_ += value;
    unsigned int current_chunk = (current_value_ / range_) *
      NUMBER_OF_CHUNKS_TO_REPORT;
    if (current_chunk > last_reported_chunk_)
    {
      if (current_chunk > NUMBER_OF_CHUNKS_TO_REPORT)
      {
        current_chunk = NUMBER_OF_CHUNKS_TO_REPORT;
      }

      double progress = (double)(current_chunk - last_reported_chunk_) /
        NUMBER_OF_CHUNKS_TO_REPORT;

      parent_->post_progress(progress);
      last_reported_chunk_ = current_chunk;
    }
  }

  void LoadingProgressCallback::loading_is_finished() noexcept
  {
    if (last_reported_chunk_ >= NUMBER_OF_CHUNKS_TO_REPORT)
    {
      return;
    }

    double progress = (double)(NUMBER_OF_CHUNKS_TO_REPORT - last_reported_chunk_) /
      NUMBER_OF_CHUNKS_TO_REPORT;
    parent_->post_progress(progress);
    last_reported_chunk_ = NUMBER_OF_CHUNKS_TO_REPORT;
  }
}
}
