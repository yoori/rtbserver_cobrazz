#ifndef EXPRESSIONMATCHER_STATISTICS_HPP
#define EXPRESSIONMATCHER_STATISTICS_HPP

// STD
#include <map>
#include <string>
#include <unordered_map>

// UNIXCOMMONS
#include <UServerUtils/Statistics/CounterStatisticsProvider.hpp>

namespace AdServer::RequestInfoSvcs
{

extern const std::string counter_provider_name;

enum class CounterStatisticId
{
  ProcessedRbcRecords,
  ProcessRbcCacheHits,
  Max  // should be the last
};

class EnumCounterConverter final
{
private:
  using Data = std::pair<UServerUtils::Statistics::CounterType, std::string>;

public:
  EnumCounterConverter() = default;

  ~EnumCounterConverter() = default;

  auto operator()()
  {
    const std::map<CounterStatisticId, Data> id_to_name =
      {
        {CounterStatisticId::ProcessedRbcRecords, {UServerUtils::Statistics::CounterType::UInt, "processed_rbc_records"}},
        {CounterStatisticId::ProcessRbcCacheHits, {UServerUtils::Statistics::CounterType::UInt, "process_rbc_cache_hits"}}
      };

    return id_to_name;
  }
};

inline auto& get_counter_statistics_provider()
{
  return UServerUtils::Statistics::get_counter_statistics_provider<
    CounterStatisticId,
    EnumCounterConverter>(counter_provider_name);
}

} // namespace AdServer::RequestInfoSvcs

#define ADD_COUNTER_STATISTIC(id, value) \
  AdServer::RequestInfoSvcs::get_counter_statistics_provider()->add(id, value);

#endif //EXPRESSIONMATCHER_STATISTICS_HPP