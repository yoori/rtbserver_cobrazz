#pragma once

#include <atomic>

#include <Generics/MetricsProvider.hpp>
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

  class FCGIAcceptorStatHolder final : public FCGIAcceptorStats
  {
  public:
    void
    add_fcgi_accept() noexcept override;

    void
    add_fcgi_connection() noexcept override;

    void
    complete_fcgi_connection() noexcept override;

    unsigned long
    fcgi_accept_total() const noexcept;

    unsigned long
    fcgi_connection_in_progress() const noexcept;

  private:
    std::atomic<unsigned long> fcgi_accept_total_{0};
    std::atomic<unsigned long> fcgi_connection_in_progress_{0};
  };

  typedef ReferenceCounting::SmartPtr<FCGIAcceptorStatHolder>
    FCGIAcceptorStatHolder_var;

  class FCGIAcceptorMetricsProvider final : public Generics::MetricsProvider
  {
  public:
    explicit
    FCGIAcceptorMetricsProvider(FCGIAcceptorStatHolder* stats);

    MetricArray
    get_values() override;

  private:
    FCGIAcceptorStatHolder_var stats_;
  };
}
