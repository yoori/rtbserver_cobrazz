#ifndef ADSERVER_LOGCOMMONS_RESEARCHLOGS_HPP_
#define ADSERVER_LOGCOMMONS_RESEARCHLOGS_HPP_

#include <iosfwd>
#include <string>
#include <fstream>
#include <sstream>

#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <Commons/Containers.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <LogCommons/LogCommons.hpp>
#include <LogCommons/StatCollector.hpp>

#include <LogCommons/GenericLogIoImpl.hpp>
#include <LogCommons/CsvUtils.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <LogCommons/Request.hpp>

namespace AdServer
{
namespace LogProcessing
{
  // ResearchBid
  struct ResearchBidData
  {
    SecondsTimestamp time;
    RequestId request_id;
    RequestId global_request_id;
    UserId user_id;
    UserId household_id;
    unsigned long tag_id;

    std::string ext_tag_id;

    std::string ip_address;
    unsigned long cc_id;

    NumberList channel_list;
    NumberList history_channel_list;

    NumberList geo_channels;
    RequestData::DeviceChannelIdOptional device_channel_id;

    std::string referer;
    unsigned long referer_hash;

    RequestData::FixedNum bid_cost;
    RequestData::FixedNum floor_cost;

    std::string algorithm_id;
    unsigned long size_id;
    unsigned long colo_id;
    unsigned long campaign_freq;

    RequestData::FixedNum predicted_ctr;
    std::string conv_rate_algorithm_id;
    RequestData::FixedNum predicted_conv_rate;

    long tag_visibility;
    long tag_predicted_viewability;

    FixedNumberList model_ctrs;
  };

  typedef SeqCollector<ResearchBidData> ResearchBidCollector;

  struct ResearchBidTraits:
    LogDefaultTraits<ResearchBidCollector, false, false>
  {
    static const char* csv_base_name() { return "RBid"; }

    static const char* csv_header()
    {
      return "Timestamp,Request ID,Global Request ID,Device,IP Address,HID,UID,"
        "URL,Tag ID,External Tag ID,Campaign Creative ID,Geo Channels,"
        "User Channels,Impression Channels,Bid Price,Bid Floor,"
        "Algorithm ID,Size ID,Colo ID,Predicted CTR,"
        "Campaign Frequency,CR Algorithm ID,Predicted CR";
    }

    static std::ostream&
    write_data_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT& data)
    {
      write_date_as_csv(os, data.time) << ',';
      os << static_cast<const UuidIoCsvWrapper&>(data.request_id) << ',';
      os << hex_request_id(data.global_request_id) << ',';
      write_optional_value_as_csv(os, data.device_channel_id) << ',';
      write_string_as_csv(os, data.ip_address) << ',';
      os << static_cast<const UuidIoCsvWrapper&>(data.household_id) << ',';
      os << static_cast<const UuidIoCsvWrapper&>(data.user_id) << ',';
      write_string_as_csv(os, data.referer) << ',';
      os << data.tag_id << ',';
      write_string_as_csv(os, data.ext_tag_id) << ',';
      os << data.cc_id << ',';
      write_list_as_csv(os, data.geo_channels) << ',';
      write_list_as_csv(os, data.history_channel_list) << ',';
      write_list_as_csv(os, data.channel_list) << ',';
      os << data.bid_cost << ',';
      os << data.floor_cost << ',';
      write_string_as_csv(os, data.algorithm_id) << ',';
      os << data.size_id << ',';
      os << data.colo_id << ',';
      os << data.predicted_ctr << ',';
      os << data.campaign_freq << ',';
      os << data.conv_rate_algorithm_id << ',';
      os << data.predicted_conv_rate << ',';
      os << data.tag_visibility << ",";
      os << data.tag_predicted_viewability << ",";
      write_list_as_csv(os, data.model_ctrs, "|");
      return os;
    }
  };

  // ResearchImpression
  struct ResearchImpressionData
  {
    SecondsTimestamp time;
    RequestId request_id;
    RequestId global_request_id;
    UserId user_id;
    UserId household_id;
    unsigned long publisher_account_id;
    unsigned long tag_id;

    std::string ext_tag_id;

    std::string ip_address;
    unsigned long campaign_id;
    unsigned long ccg_id;
    unsigned long cc_id;

    NumberList channel_list;
    NumberList history_channel_list;

    NumberList geo_channels;
    RequestData::DeviceChannelIdOptional device_channel_id;

    std::string referer;
    unsigned long referer_hash;

    RequestData::FixedNum bid_cost;
    RequestData::FixedNum floor_cost;

    std::string algorithm_id;
    unsigned long size_id;
    unsigned long colo_id;
    unsigned long campaign_freq;

    RequestData::FixedNum predicted_ctr;
    std::string conv_rate_algorithm_id;
    RequestData::FixedNum predicted_conv_rate;

    long tag_visibility;
    long tag_predicted_viewability;

    FixedNumberList model_ctrs;

    FixedNumber win_price;
  };

  typedef SeqCollector<ResearchImpressionData> ResearchImpressionCollector;

  struct ResearchImpressionTraits:
    LogDefaultTraits<ResearchImpressionCollector, false, false>
  {
    static const char* csv_base_name() { return "RImpression"; }

    static const char* csv_header()
    {
      return "Timestamp,#RequestID,#GlobalRequestID,Device,#IPAddress,#HID,#UID,"
        "URL,Publisher,Tag,ETag,Campaign,Group,CCID,GeoCh,"
        "UserCh,#ImpCh,#BidPrice,#BidFloor,"
        "#AlgorithmID,SizeID,Colo,#PredictedCTR,"
        "Campaign_Freq,#CRAlgorithmID,#PredictedCR,#WinPrice,#Viewability";
    }

    static std::ostream&
    write_data_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT& data)
    {
      write_date_as_csv(os, data.time) << ',';
      os << static_cast<const UuidIoCsvWrapper&>(data.request_id) << ',';
      os << hex_request_id(data.global_request_id) << ',';
      write_optional_value_as_csv(os, data.device_channel_id) << ',';
      write_string_as_csv(os, data.ip_address) << ',';
      os << static_cast<const UuidIoCsvWrapper&>(data.household_id) << ',';
      os << static_cast<const UuidIoCsvWrapper&>(data.user_id) << ',';
      write_string_as_csv(os, data.referer) << ',';
      os << data.publisher_account_id << ',';
      os << data.tag_id << ',';
      write_string_as_csv(os, data.ext_tag_id) << ',';
      os << data.campaign_id << ',';
      os << data.ccg_id << ',';
      os << data.cc_id << ',';
      write_list_as_csv(os, data.geo_channels) << ',';
      write_list_as_csv(os, data.history_channel_list) << ',';
      write_list_as_csv(os, data.channel_list) << ',';
      os << data.bid_cost << ',';
      os << data.floor_cost << ',';
      write_string_as_csv(os, data.algorithm_id) << ',';
      os << data.size_id << ',';
      os << data.colo_id << ',';
      os << data.predicted_ctr << ',';
      os << data.campaign_freq << ',';
      os << data.conv_rate_algorithm_id << ',';
      os << data.predicted_conv_rate << ',';
      os << data.tag_visibility << ',';
      os << data.win_price << ',';
      os << data.tag_predicted_viewability;
      return os;
    }
  };

  // ResearchClick
  struct ResearchClickData
  {
    SecondsTimestamp time;
    RequestId request_id;
    StringIoWrapperOptional referrer;
  };

  typedef SeqCollector<ResearchClickData> ResearchClickCollector;

  struct ResearchClickTraits:
    LogDefaultTraits<ResearchClickCollector, false, false>
  {
    static const char* csv_base_name() { return "RClick"; }

    static const char* csv_header()
    {
      return "Timestamp,Request ID,URL";
    }

    static std::ostream&
    write_data_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT& data)
    {
      write_date_as_csv(os, data.time) << ',';
      os << static_cast<const UuidIoCsvWrapper&>(data.request_id) << ',';
      write_optional_string_as_csv(os, data.referrer);
      return os;
    }
  };

  // ResearchAction
  typedef SeqCollector<AdvertiserActionData> ResearchActionCollector;

  struct ResearchActionTraits:
    LogDefaultTraits<ResearchActionCollector, false, false>
  {
    static const char* csv_base_name() { return "RAction"; }

    static const char* csv_header()
    {
      return "Timestamp,Device,IP Address,UID,URL,Action ID,"
        "Order ID,Order Value";
    }

    static std::ostream&
    write_data_as_csv(
      std::ostream& os,
      const BaseTraits::CollectorType::DataT& data)
    {
      write_date_as_csv(os, data.time()) << ',';
      write_optional_value_as_csv(os, data.device_channel_id()) << ',';
      write_optional_string_as_csv(os, data.ip_address()) << ',';
      os << static_cast<const UuidIoCsvWrapper&>(data.user_id()) << ',';
      write_optional_string_as_csv(os, data.referrer()) << ',';
      write_optional_value_as_csv(os, data.action_id()) << ',';
      write_optional_string_as_csv(os, data.order_id()) << ',';
      os << data.cur_value();
      return os;
    }
  };
} // namespace LogProcessing
} // namespace AdServer

#endif /* ADSERVER_LOGCOMMONS_RESEARCHLOGS_HPP_ */

