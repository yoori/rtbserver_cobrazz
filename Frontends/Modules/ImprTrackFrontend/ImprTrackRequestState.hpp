#pragma once

#include <memory>

#include <Commons/UserInfoManip.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer::ImprTrack
{
  class Frontend;

  class ImprTrackRequestState:
    public std::enable_shared_from_this<ImprTrackRequestState>
  {
  public:
    ImprTrackRequestState(
      Frontend* frontend,
      const RequestInfo& request_info,
      const AdServer::Commons::UserId& input_user_id);

    void
    start();

  private:
    Frontend* frontend_;
    RequestInfo request_info_;
    AdServer::Commons::UserId result_user_id_;
  };
}
