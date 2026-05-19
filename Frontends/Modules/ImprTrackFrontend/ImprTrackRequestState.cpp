#include "ImprTrackRequestState.hpp"

namespace AdServer::ImprTrack
{
  ImprTrackRequestState::ImprTrackRequestState(
    Frontend* frontend,
    const RequestInfo& request_info,
    const AdServer::Commons::UserId& input_user_id)
    : frontend_(frontend),
      request_info_(request_info),
      result_user_id_(input_user_id)
  {}

  void
  ImprTrackRequestState::start()
  {}
}
