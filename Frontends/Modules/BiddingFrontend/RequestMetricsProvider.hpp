#ifndef BIDDINGFRONTEND_REQUESTMETRICSPROVIDER_HPP
#define BIDDINGFRONTEND_REQUESTMETRICSPROVIDER_HPP

// STD
#include <array>
#include <atomic>

// THIS
#include <ReferenceCounting/SmartPtr.hpp>
#include <UServerUtils/MetricsHTTPProvider.hpp>

namespace AdServer
{
namespace Bidding
{

class RequestMetricsProvider final : public MetricsProvider
{
  enum class RequestType
  {
    Input = 0,
    Skip,
    UserInfo,
    ChannelServer,
    Max = ChannelServer
  };

  using Counter = std::atomic<long>;
  using Counters = std::array<Counter, static_cast<std::size_t>(RequestType::Max) + 1>;

  static constexpr const char* input_request = "input_request";
  static constexpr const char* skip_request = "skip_request";
  static constexpr const char* user_info_request = "user_info_request";
  static constexpr const char* channel_server_request = "channel_server_request";

public:
  explicit RequestMetricsProvider();

  ~RequestMetricsProvider() override = default;

  MetricArray get_values() override;

  void add_value(std::string_view n, double v) override;

  void add_value(std::string_view n, long v) override;

  void add_value(std::string_view n, const std::string& v) override;

  void add_input_request() noexcept;

  void add_skip_request() noexcept;

  void add_user_info_request() noexcept;

  void add_channel_server_request() noexcept;

private:
  Counters counters_;
};

using RequestMetricsProvider_var = ReferenceCounting::SmartPtr<RequestMetricsProvider>;

} // namespace Bidding
} // namespace AdServer

#endif //BIDDINGFRONTEND_REQUESTMETRICSPROVIDER_HPP
