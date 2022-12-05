#ifndef AD_SERVER_LOG_PROCESSING_RESEARCH_PROF_STAT_HPP
#define AD_SERVER_LOG_PROCESSING_RESEARCH_PROF_STAT_HPP

#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>
#include <LogCommons/CsvUtils.hpp>
#include <Commons/StringHolder.hpp>

namespace AdServer {
namespace LogProcessing {

struct ResearchProfData
{
  SecondsTimestamp time;
  std::string app;
  unsigned long colo_id;
  std::string uid_hash;
  std::string tuid_hash;
  std::string ip_addr_hash;
  std::string hid_hash;
  unsigned long device_channel_id;
  std::string referer;
  Commons::StringHolder_var page_keywords;
  Commons::StringHolder_var search_keywords;
  Commons::StringHolder_var url_keywords;
  NumberList channel_list;
};

typedef SeqCollector<ResearchProfData> ResearchProfCollector;

struct ResearchProfTraits:
  LogDefaultTraits<ResearchProfCollector, false, false>
{
  static const char* csv_base_name() { return "RProf"; }

  static const char* csv_header()
  {
    return "Timestamp,App,Colo ID,UID,TUID,IP Address,HID,Device,"
           "URL,Page Keywords,Search Keywords,URL Keywords,Channels";
  }

  static std::ostream&
  write_data_as_csv(
    std::ostream& os,
    const BaseTraits::CollectorType::DataT& data
  )
  {
    write_date_as_csv(os, data.time) << ',';
    write_string_as_csv(os, data.app) << ',';
    os << data.colo_id << ',';
    write_string_as_csv(os, data.uid_hash) << ',';
    write_string_as_csv(os, data.tuid_hash) << ',';
    write_string_as_csv(os, data.ip_addr_hash) << ',';
    write_string_as_csv(os, data.hid_hash) << ',';
    os << data.device_channel_id << ',';
    write_string_as_csv(os, data.referer) << ',';
    write_string_as_csv(os, data.page_keywords) << ',';
    write_string_as_csv(os, data.search_keywords) << ',';
    write_string_as_csv(os, data.url_keywords) << ',';
    write_list_as_csv(os, data.channel_list);
    return os;
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_RESEARCH_PROF_STAT_HPP */

