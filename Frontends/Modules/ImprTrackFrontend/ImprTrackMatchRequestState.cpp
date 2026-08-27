#include "ImprTrackMatchRequestState.hpp"

#include "ImprTrackFrontend.hpp"

namespace AdServer::ImprTrack
{
  ImprTrackMatchRequestState::ImprTrackMatchRequestState(Frontend* frontend) noexcept
    : frontend_(frontend)
  {}
}
