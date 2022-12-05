/**
 * RequestInfoContainerMTTest.hpp
 */

#ifndef REQUESTINFOCONTAINERMTTEST_HPP
#define REQUESTINFOCONTAINERMTTEST_HPP

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

#endif /*REQUESTINFOCONTAINERMTTEST_HPP*/
