#ifndef AD_SERVER_LOG_PROCESSING_RESEARCH_WEB_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_RESEARCH_WEB_STAT_HPP


#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>
#include <LogCommons/CsvUtils.hpp>

namespace AdServer {
namespace LogProcessing {

struct ResearchWebStatData
{
  SecondsTimestamp time;
  RequestId request_id;
  RequestId global_request_id;
  std::string app;
  std::string source;
  std::string operation;
  char result;
  std::string referer;
  std::string ip_address;
  std::string external_user_id;
  std::string user_agent;
};

typedef SeqCollector<ResearchWebStatData> ResearchWebStatCollector;

struct ResearchWebStatTraits:
  LogDefaultTraits<ResearchWebStatCollector, false, false>
{
  static const char* csv_base_name() { return "RWebStat"; }

  static const char* csv_header()
  {
    return "Timestamp,Request ID,Global Request ID,"
      "App,Source,Operation,Result,Referer,IP Address, External User ID,User-Agent";
  }

  static std::ostream&
  write_data_as_csv(
    std::ostream& os,
    const BaseTraits::CollectorType::DataT& data
  )
  {
    std::string und_user_agent;
    undisplayable_mime_encode(und_user_agent, data.user_agent);

    write_date_as_csv(os, data.time) << ',';
    os << static_cast<const UuidIoCsvWrapper&>(data.request_id ) << ',';
    os << static_cast<const UuidIoCsvWrapper&>(data.global_request_id ) << ',';
    write_string_as_csv(os, data.app) << ',';
    write_string_as_csv(os, data.source) << ',';
    write_string_as_csv(os, data.operation) << ',';
    os << data.result << ',';
    write_string_as_csv(os, data.referer) << ',';
    write_string_as_csv(os, data.ip_address) << ',';
    write_string_as_csv(os, data.external_user_id) << ',';
    write_string_as_csv(os, und_user_agent);
    return os;
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_RESEARCH_WEB_STAT_HPP */

