#include "ImprTrackMatchRequestState.hpp"

#include "ImprTrackFrontend.hpp"

namespace AdServer::ImprTrack
{
  ImprTrackMatchRequestState::ImprTrackMatchRequestState(
    Frontend* frontend) noexcept
    : frontend_(frontend)
  {}

  void
  ImprTrackMatchRequestState::start_match_channels_stage() noexcept
  {
    frontend_->start_match_channels_(shared_from_this());
  }

  void
  ImprTrackMatchRequestState::start_history_match_stage() noexcept
  {
    frontend_->start_history_match_(shared_from_this());
  }

  void
  ImprTrackMatchRequestState::process_match_request_stage() noexcept
  {
    frontend_->process_match_request_(shared_from_this());
  }
}
