#ifndef USERINFOSVCS_USERINFOMANAGER_PROTO_CONVERTOR_HPP_
#define USERINFOSVCS_USERINFOMANAGER_PROTO_CONVERTOR_HPP_

#include <Commons/CorbaAlgs.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoManager.hpp>
#include <UserInfoSvcs/UserInfoManager/proto/UserInfoManager.grpc.pb.h>

namespace AdServer::UserInfoSvcs
{

inline
AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult*
convertor_proto_match_result(
  const AdServer::UserInfoSvcs::Proto::MatchResult& match_result_proto)
{
  AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var result =
    new AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult{};

  result->times_inited = match_result_proto.times_inited();

  const auto& last_request_time_proto = match_result_proto.last_request_time();
  if (!last_request_time_proto.empty())
  {
    result->last_request_time.length(last_request_time_proto.size());
    std::memcpy(
      result->last_request_time.get_buffer(),
      last_request_time_proto.data(),
      last_request_time_proto.size());
  }

  const auto& create_time_proto = match_result_proto.create_time();
  if (!create_time_proto.empty())
  {
    result->create_time.length(create_time_proto.size());
    std::memcpy(
      result->create_time.get_buffer(),
      create_time_proto.data(),
      create_time_proto.size());
  }

  const auto& session_start_proto = match_result_proto.session_start();
  if (!session_start_proto.empty())
  {
    result->session_start.length(session_start_proto.size());
    std::memcpy(
      result->session_start.get_buffer(),
      session_start_proto.data(),
      session_start_proto.size());
  }

  result->colo_id = match_result_proto.colo_id();

  const auto& channels_proto = match_result_proto.channels();
  std::size_t index = 0;
  if (!channels_proto.empty())
  {
    result->channels.length(channels_proto.size());
    for (const auto& channel : channels_proto)
    {
      result->channels[index].channel_id = channel.channel_id();
      result->channels[index].weight = channel.weight();
      index += 1;
    }
  }

  const auto& hid_channels_proto = match_result_proto.hid_channels();
  index = 0;
  if (!hid_channels_proto.empty())
  {
    result->hid_channels.length(hid_channels_proto.size());
    for (const auto& hid_channel : hid_channels_proto)
    {
      result->hid_channels[index].channel_id = hid_channel.channel_id();
      result->hid_channels[index].weight = hid_channel.weight();
      index += 1;
    }
  }

  const auto& full_freq_caps_proto = match_result_proto.full_freq_caps();
  CorbaAlgs::fill_sequence(
    std::begin(full_freq_caps_proto),
    std::end(full_freq_caps_proto),
    result->full_freq_caps);

  const auto& full_virtual_freq_caps_proto = match_result_proto.full_virtual_freq_caps();
  CorbaAlgs::fill_sequence(
    std::begin(full_virtual_freq_caps_proto),
    std::end(full_virtual_freq_caps_proto),
    result->full_virtual_freq_caps);

  const auto& seq_orders_proto = match_result_proto.seq_orders();
  if (!seq_orders_proto.empty())
  {
    result->seq_orders.length(seq_orders_proto.size());
    index = 0;
    for (const auto& seq_order: seq_orders_proto)
    {
      result->seq_orders[index].ccg_id = seq_order.ccg_id();
      result->seq_orders[index].imps = seq_order.imps();
      result->seq_orders[index].set_id = seq_order.set_id();
      index += 1;
    }
  }

  const auto& campaign_freqs_proto = match_result_proto.campaign_freqs();
  if (!campaign_freqs_proto.empty())
  {
    result->campaign_freqs.length(campaign_freqs_proto.size());
    index = 0;
    for (const auto& campaign_freq: campaign_freqs_proto)
    {
      result->campaign_freqs[index].imps = campaign_freq.imps();
      result->campaign_freqs[index].campaign_id = campaign_freq.campaign_id();
      index += 1;
    }
  }

  result->fraud_request = match_result_proto.fraud_request();

  const auto& process_time_proto = match_result_proto.process_time();
  if (!process_time_proto.empty())
  {
    result->process_time.length(process_time_proto.size());
    std::memcpy(
      result->process_time.get_buffer(),
      process_time_proto.data(),
      process_time_proto.size());
  }

  result->adv_channel_count = match_result_proto.adv_channel_count();
  result->discover_channel_count = match_result_proto.discover_channel_count();
  result->cohort << match_result_proto.cohort();
  result->cohort2 << match_result_proto.cohort2();

  const auto& exclude_pubpixel_accounts_proto = match_result_proto.exclude_pubpixel_accounts();
  CorbaAlgs::fill_sequence(
    std::begin(exclude_pubpixel_accounts_proto),
    std::end(exclude_pubpixel_accounts_proto),
    result->exclude_pubpixel_accounts);

  const auto& geo_data_seq_proto = match_result_proto.geo_data_seq();
  if (!geo_data_seq_proto.empty())
  {
    result->geo_data_seq.length(geo_data_seq_proto.size());
    index = 0;
    for (const auto& geo_data : geo_data_seq_proto)
    {
      auto& geo_data_seq = result->geo_data_seq[index];

      const auto& accuracy_proto = geo_data.accuracy();
      geo_data_seq.accuracy.length(accuracy_proto.size());
      std::memcpy(
        geo_data_seq.accuracy.get_buffer(),
        accuracy_proto.data(),
        accuracy_proto.size());

      const auto& latitude_proto = geo_data.latitude();
      geo_data_seq.latitude.length(latitude_proto.size());
      std::memcpy(
        geo_data_seq.latitude.get_buffer(),
        latitude_proto.data(),
        latitude_proto.size());

      const auto& longitude_proto = geo_data.longitude();
      geo_data_seq.longitude.length(longitude_proto.size());
      std::memcpy(
        geo_data_seq.longitude.get_buffer(),
        longitude_proto.data(),
        longitude_proto.size());

      index += 1;
    }
  }

  return result._retn();
}

} // namespace AdServer::UserInfoSvcs

#endif //USERINFOSVCS_USERINFOMANAGER_PROTO_CONVERTOR_HPP_
