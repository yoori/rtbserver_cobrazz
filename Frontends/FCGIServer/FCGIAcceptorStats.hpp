#pragma once

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

namespace AdServer::Frontends
{
  class FCGIAcceptorStats: public ReferenceCounting::AtomicImpl
  {
  public:
    virtual void
    add_fcgi_accept() noexcept = 0;

    virtual void
    add_fcgi_connection() noexcept = 0;

    virtual void
    complete_fcgi_connection() noexcept = 0;

  protected:
    virtual
    ~FCGIAcceptorStats() noexcept = default;
  };

  typedef ReferenceCounting::SmartPtr<FCGIAcceptorStats>
    FCGIAcceptorStats_var;
}
