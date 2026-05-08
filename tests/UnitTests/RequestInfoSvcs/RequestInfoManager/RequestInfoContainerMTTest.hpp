/**
 * RequestInfoContainerMTTest.hpp
 */

#pragma once

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    class RequestInfoContainer;
  }
}

bool
multi_thread_test(
  AdServer::RequestInfoSvcs::RequestInfoContainer* request_info_container)
  noexcept;
