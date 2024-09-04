#ifndef FRONTENDCOMMONS_STATISTICS_HPP
#define FRONTENDCOMMONS_STATISTICS_HPP

// STD
#include <map>
#include <string>
#include <unordered_map>

// UNIXCOMMONS
#include <UServerUtils/Statistics/CommonStatisticsProvider.hpp>
#include <UServerUtils/Statistics/CounterStatisticsProvider.hpp>
#include <UServerUtils/Statistics/TimeStatisticsProvider.hpp>

namespace FrontendCommons
{

extern const std::string time_provider_name;
extern const std::string common_counter_provider_name;

extern const std::string pubpixel_frontend_counter_provider_name;
extern const std::string pubpixel_frontend_with_labels_counter_provider_name;

enum class TimeStatisticId
{
  BiddingFrontend_InputRequest,
  BiddingFrontend_SkipRequest,
  BiddingFrontend_ServerRequest,
  BiddingFrontend_UserInfoRequest,
  Max  // should be the last
};

enum class CounterStatisticId
{
  BiddingFrontend_Bids,
  UserBind_RequestCount,
  Max  // should be the last
};

enum class PubPixelFrontendStatisticId
{
  PubPixelFrontend_TotalRequests,
  Max  // should be the last
};

enum class PubPixelFrontendWithLabelsStatisticId
{
  PubPixelFrontendWithLabels_PublisherRequests,
  Max  // should be the last
};

class TimeStatisticIdToStringConverter final
{
public:
  TimeStatisticIdToStringConverter() = default;

  ~TimeStatisticIdToStringConverter() = default;

  auto operator()()
  {
    const std::map<TimeStatisticId, std::string> id_to_name = {
      {TimeStatisticId::BiddingFrontend_InputRequest, "BiddingFrontend:input_request"},
      {TimeStatisticId::BiddingFrontend_SkipRequest, "BiddingFrontend:skip_request"},
      {TimeStatisticId::BiddingFrontend_ServerRequest, "BiddingFrontend:server_request"},
      {TimeStatisticId::BiddingFrontend_UserInfoRequest, "BiddingFrontend:user_info_request"},
    };

    return id_to_name;
  }
};

inline auto& get_time_statistics_provider()
{
  return UServerUtils::Statistics::get_time_statistics_provider<
    TimeStatisticId,
    TimeStatisticIdToStringConverter,
    4,
    50>(time_provider_name);
}

class EnumCounterConverter final
{
public:
  EnumCounterConverter() = default;

  ~EnumCounterConverter() = default;

  auto operator()()
  {
    const std::map<CounterStatisticId, std::pair<UServerUtils::Statistics::CommonType, std::string>> id_to_name = {
      {CounterStatisticId::BiddingFrontend_Bids, {UServerUtils::Statistics::CommonType::UInt, "BiddingFrontend:bids:ccg_id"}},
      {CounterStatisticId::UserBind_RequestCount, {UServerUtils::Statistics::CommonType::UInt, "UserBind:request_count:ssp_name"}}
    };

    return id_to_name;
  }
};

inline auto& get_common_counter_statistics_provider()
{
  return UServerUtils::Statistics::get_common_statistics_provider<
    CounterStatisticId,
    EnumCounterConverter,
    std::shared_mutex,
    std::unordered_map>(common_counter_provider_name, 500);
}

class PubPixelFrontendStatisticIdConverter final
{
private:
  using Data = std::pair<UServerUtils::Statistics::CounterType, std::string>;

public:
  PubPixelFrontendStatisticIdConverter() = default;

  ~PubPixelFrontendStatisticIdConverter() = default;

  auto operator()()
  {
    const std::map<PubPixelFrontendStatisticId, Data> id_to_name =
      {   
        {PubPixelFrontendStatisticId::PubPixelFrontend_TotalRequests, {UServerUtils::Statistics::CounterType::UInt, "pubpixel_requests_total"}}
      };  

    return id_to_name;
  }
};

inline auto& get_pubpixel_frontend_counter_statistics_provider()
{
  return UServerUtils::Statistics::get_counter_statistics_provider<
    PubPixelFrontendStatisticId,
    PubPixelFrontendStatisticIdConverter>(pubpixel_frontend_counter_provider_name);
}

class PubPixelFrontendWithLabelsStatisticIdConverter final
{
public:
  PubPixelFrontendWithLabelsStatisticIdConverter() = default;

  ~PubPixelFrontendWithLabelsStatisticIdConverter() = default;

  auto operator()()
  {
    const std::map<PubPixelFrontendWithLabelsStatisticId, std::pair<UServerUtils::Statistics::CommonType, std::string>> id_to_name = {
      {PubPixelFrontendWithLabelsStatisticId::PubPixelFrontendWithLabels_PublisherRequests, {UServerUtils::Statistics::CommonType::UInt, "pubpixel_requests"}}
    };

    return id_to_name;
  }
};

inline auto& get_pubpixel_frontend_with_labels_counter_statistics_provider()
{
  return UServerUtils::Statistics::get_common_statistics_provider<
    PubPixelFrontendWithLabelsStatisticId,
    PubPixelFrontendWithLabelsStatisticIdConverter,
    std::shared_mutex,
    std::unordered_map>(pubpixel_frontend_with_labels_counter_provider_name, 50);
}

} // namespace FrontendCommons

#define DO_TIME_STATISTIC_FRONTEND(id) \
  auto measure = FrontendCommons::get_time_statistics_provider()->make_measure(id);

#define ADD_COMMON_COUNTER_STATISTIC(id, label, value) \
  FrontendCommons::get_common_counter_statistics_provider()->add(id, label, value);

#define ADD_PUBPIXEL_FRONTEND_COUNTER_STATISTIC(id, value) \
  FrontendCommons::get_pubpixel_frontend_counter_statistics_provider()->add(id, value);

#define ADD_PUBPIXEL_FRONTEND_WITH_LABELS_COUNTER_STATISTIC(id, label, value) \
  FrontendCommons::get_pubpixel_frontend_with_labels_counter_statistics_provider()->add(id, label, value);

#endif //FRONTENDCOMMONS_STATISTICS_HPP
