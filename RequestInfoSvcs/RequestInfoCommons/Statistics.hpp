#ifndef REQUESTINFOCOMMONS_STATISTICS_HPP
#define REQUESTINFOCOMMONS_STATISTICS_HPP

// STD
#include <map>
#include <string>
#include <unordered_map>

// UNIXCOMMONS
#include <UServerUtils/Statistics/CommonStatisticsProvider.hpp>

namespace AdServer::RequestInfoSvcs
{

extern const std::string common_counter_provider_name;

enum class CounterStatisticId
{
  LogRecordFetcherBase_ProcessedFiles,
  LogRecordFetcherBase_ProcessedRecordCount,
  Max  // should be the last
};

class EnumCounterConverter final
{
public:
  EnumCounterConverter() = default;

  ~EnumCounterConverter() = default;

  auto operator()()
  {
    const std::map<CounterStatisticId, std::pair<UServerUtils::Statistics::CommonType, std::string>> id_to_name = {
      {CounterStatisticId::LogRecordFetcherBase_ProcessedFiles, {UServerUtils::Statistics::CommonType::UInt, "LogRecordFetcherBase:processed_files:type"}},
      {CounterStatisticId::LogRecordFetcherBase_ProcessedRecordCount, {UServerUtils::Statistics::CommonType::UInt, "LogRecordFetcherBase:processed_record_count:type"}}
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

} // namespace AdServer::RequestInfoSvcs

#define ADD_COMMON_COUNTER_STATISTIC(id, label, value) \
  AdServer::RequestInfoSvcs::get_common_counter_statistics_provider()->add(id, label, value);

#endif // REQUESTINFOCOMMONS_STATISTICS_HPP