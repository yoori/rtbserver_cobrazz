#include "FCGIAcceptorStats.hpp"

namespace AdServer::Frontends
{
  void
  FCGIAcceptorStatHolder::add_fcgi_accept() noexcept
  {
    fcgi_accept_total_.fetch_add(1, std::memory_order_relaxed);
  }

  void
  FCGIAcceptorStatHolder::add_fcgi_connection() noexcept
  {
    fcgi_connection_in_progress_.fetch_add(1, std::memory_order_relaxed);
  }

  void
  FCGIAcceptorStatHolder::complete_fcgi_connection() noexcept
  {
    fcgi_connection_in_progress_.fetch_sub(1, std::memory_order_relaxed);
  }

  unsigned long
  FCGIAcceptorStatHolder::fcgi_accept_total() const noexcept
  {
    return fcgi_accept_total_.load(std::memory_order_relaxed);
  }

  unsigned long
  FCGIAcceptorStatHolder::fcgi_connection_in_progress() const noexcept
  {
    return fcgi_connection_in_progress_.load(std::memory_order_relaxed);
  }

  FCGIAcceptorMetricsProvider::FCGIAcceptorMetricsProvider(FCGIAcceptorStatHolder* stats)
    : stats_(ReferenceCounting::add_ref(stats))
  {}

  FCGIAcceptorMetricsProvider::MetricArray
  FCGIAcceptorMetricsProvider::get_values()
  {
    MetricArray result;
    if (stats_.in())
    {
      result.emplace_back("fcgi_accept_total", static_cast<long>(stats_->fcgi_accept_total()));
      result.emplace_back(
        "fcgi_connection_in_progress",
        static_cast<long>(stats_->fcgi_connection_in_progress()));
    }
    return result;
  }
}
