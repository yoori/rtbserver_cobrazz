#include "ChannelGrpcAlgs.hpp"

#include <algorithm>
#include <cstdint>
#include <map>

#include <Commons/Grpc/GrpcSync.hpp>
#include <Stream/MemoryStream.hpp>

namespace AdServer::ChannelSvcs::GrpcAlgs
{
  namespace
  {
    namespace PB = adserver::channel_svcs::channel_server;

    void merge_channel_atoms_(
      google::protobuf::RepeatedPtrField<PB::ChannelAtom>* target,
      const google::protobuf::RepeatedPtrField<PB::ChannelAtom>& source)
    {
      if (source.empty())
      {
        return;
      }

      google::protobuf::RepeatedPtrField<PB::ChannelAtom> merged(target->GetArena());
      merged.Reserve(target->size() + source.size());
      std::set_union(
        target->begin(), target->end(), source.begin(), source.end(),
        google::protobuf::RepeatedPtrFieldBackInserter(&merged),
        [](const auto& left, const auto& right)
        {
          return left.id() < right.id() ||
            (left.id() == right.id() && left.trigger_channel_id() < right.trigger_channel_id());
        });
      target->Swap(&merged);
    }

    void throw_channel_exception_(const grpc::Status& status)
    {
      Stream::Error ostr;
      ostr << "ChannelServer grpc match failed: code=" <<
        static_cast<int>(status.error_code()) << ", message=" << status.error_message();
      throw Exception(ostr);
    }
  }

  void merge_match_response(
    adserver::channel_svcs::channel_server::MatchResponse& target,
    const adserver::channel_svcs::channel_server::MatchResponse& source)
  {
    auto* target_channels = target.mutable_matched_channels();
    const auto& source_channels = source.matched_channels();

    merge_channel_atoms_(target_channels->mutable_page_channels(), source_channels.page_channels());
    merge_channel_atoms_(
      target_channels->mutable_search_channels(), source_channels.search_channels());
    merge_channel_atoms_(target_channels->mutable_url_channels(), source_channels.url_channels());
    merge_channel_atoms_(
      target_channels->mutable_url_keyword_channels(), source_channels.url_keyword_channels());

    auto* target_uids = target_channels->mutable_uid_channels();
    const auto& source_uids = source_channels.uid_channels();
    if (!source_uids.empty())
    {
      google::protobuf::RepeatedField<std::uint64_t> merged(target_uids->GetArena());
      merged.Reserve(target_uids->size() + source_uids.size());
      std::set_union(
        target_uids->begin(), target_uids->end(), source_uids.begin(), source_uids.end(),
        google::protobuf::RepeatedFieldBackInserter(&merged));
      target_uids->Swap(&merged);
    }

    std::map<std::uint32_t, std::uint32_t> content_weights;
    for (const auto& channel : target.content_channels())
    {
      content_weights[channel.id()] += channel.weight();
    }

    for (const auto& channel : source.content_channels())
    {
      content_weights[channel.id()] += channel.weight();
    }
    target.clear_content_channels();
    for (const auto& [channel_id, weight] : content_weights)
    {
      auto* channel = target.add_content_channels();
      channel->set_id(channel_id);
      channel->set_weight(weight);
    }

    target.set_no_adv(target.no_adv() || source.no_adv());
    target.set_no_track(target.no_track() || source.no_track());
    target.set_full_loaded(target.full_loaded() && source.full_loaded());

    if (!source.hostname().empty())
    {
      if (target.hostname().empty())
      {
        target.set_hostname(source.hostname());
      }
      else
      {
        target.set_hostname(target.hostname() + "," + source.hostname());
      }
    }

    if (!source.match_time().empty())
    {
      if (target.match_time().empty())
      {
        target.set_match_time(source.match_time());
      }
    }
  }

  adserver::channel_svcs::channel_server::MatchResponse channel_match(
    ChannelServerGrpcAsyncClient& client,
    const adserver::channel_svcs::channel_server::MatchRequest& request)
  {
    return AdServer::Grpc::sync_call<
      adserver::channel_svcs::channel_server::MatchResponse>(
        [&](auto callback)
        {
          client.match(request, std::move(callback));
        },
        throw_channel_exception_);
  }
}
