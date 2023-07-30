#ifdef KALL
#include "RequestMetricsProvider.hpp"

namespace AdServer
{
namespace Bidding
{

RequestMetricsProvider::RequestMetricsProvider()
 : counters_()
{
}

RequestMetricsProvider::MetricArray RequestMetricsProvider::get_values()
{
  MetricArray metric_array;
  const auto size = counters_.size();
  for (std::size_t i = 0; i < size; ++i)
  {
    switch (i)
    {
    case static_cast<std::size_t>(RequestType::Input):
      metric_array.emplace_back(
        input_request,
        counters_[i].exchange(0, std::memory_order_relaxed));
      break;
    case static_cast<std::size_t>(RequestType::Skip):
      metric_array.emplace_back(
        skip_request,
        counters_[i].exchange(0, std::memory_order_relaxed));
      break;
    case static_cast<std::size_t>(RequestType::UserInfo):
      metric_array.emplace_back(
        user_info_request,
        counters_[i].exchange(0, std::memory_order_relaxed));
      break;
    case static_cast<std::size_t>(RequestType::ChannelServer):
      metric_array.emplace_back(
        channel_server_request,
        counters_[i].exchange(0, std::memory_order_relaxed));
      break;
    default:
      break;
    }
  }

  return metric_array;
}

void RequestMetricsProvider::add_value(std::string_view /*n*/, double /*v*/)
{
}

void RequestMetricsProvider::add_value(std::string_view /*name*/, long /*value*/)
{
}

void RequestMetricsProvider::add_value(std::string_view /*n*/, const std::string& /*v*/)
{
}

void RequestMetricsProvider::add_input_request() noexcept
{
  counters_[static_cast<std::size_t>(RequestType::Input)].fetch_add(1l, std::memory_order_relaxed);
}

void RequestMetricsProvider::add_skip_request() noexcept
{
  counters_[static_cast<std::size_t>(RequestType::Skip)].fetch_add(1l, std::memory_order_relaxed);
}

void RequestMetricsProvider::add_user_info_request() noexcept
{
  counters_[static_cast<std::size_t>(RequestType::UserInfo)].fetch_add(1l, std::memory_order_relaxed);
}

void RequestMetricsProvider::add_channel_server_request() noexcept
{
  counters_[static_cast<std::size_t>(RequestType::ChannelServer)].fetch_add(1l, std::memory_order_relaxed);
}

} // namespace Bidding
} // namespace AdServer
#endif