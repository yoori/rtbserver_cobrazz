#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <ChannelSvcs/ChannelClient/ChannelGrpcAlgs.hpp>
#include <ChannelSvcs/ChannelCommons/ChannelUtils.hpp>
#include <ChannelSvcs/ChannelServer/ChannelContainer.hpp>
#include <ChannelSvcs/ChannelServer/UpdateContainer.hpp>

namespace
{
  namespace CS = AdServer::ChannelSvcs;
  namespace PB = adserver::channel_svcs::channel_server;
  using Pair = std::pair<std::uint64_t, std::uint64_t>;
  using Atoms = google::protobuf::RepeatedPtrField<PB::ChannelAtom>;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  std::array<Atoms*, 4> sequences(PB::MatchResponse& response)
  {
    auto* channels = response.mutable_matched_channels();
    return {channels->mutable_page_channels(), channels->mutable_search_channels(),
      channels->mutable_url_channels(), channels->mutable_url_keyword_channels()};
  }

  void fill(PB::MatchResponse& response, const std::vector<Pair>& pairs)
  {
    const auto fields = sequences(response);
    for (std::size_t type = 0; type < fields.size(); ++type)
    {
      for (const auto& [channel, trigger] : pairs)
      {
        auto* atom = fields[type]->Add();
        atom->set_id(channel);
        atom->set_trigger_channel_id(trigger + type * 100);
      }
    }

    response.set_full_loaded(true);
  }

  void test_merge(bool use_arena)
  {
    std::array<PB::MatchResponse, 4> sources;
    fill(sources[0], {{7, 2}, {42, 10}, {42, 30}});
    fill(sources[1], {{3, 40}, {7, 2}, {42, 20}});
    fill(sources[2], {{7, 3}, {42, 10}, {90, 1}});
    fill(sources[3], {});
    const std::array<std::vector<std::uint64_t>, 4> uids{{{3, 7}, {7, 9}, {1, 9}, {}}};
    for (std::size_t i = 0; i < sources.size(); ++i)
    {
      auto& source = sources[i];
      for (const auto uid : uids[i])
      {
        source.mutable_matched_channels()->add_uid_channels(uid);
      }

      source.set_hostname("server" + std::to_string(i));
      source.set_match_time("time" + std::to_string(i));
      auto* content = source.add_content_channels();
      content->set_id(42);
      content->set_weight(i + 1);
    }

    sources[0].set_no_adv(true);
    sources[1].set_no_track(true);
    sources[2].set_full_loaded(false);

    std::array<std::set<Pair>, 4> expected;
    std::array<std::string, 4> original;
    for (std::size_t i = 0; i < sources.size(); ++i)
    {
      original[i] = sources[i].SerializeAsString();
      const auto fields = sequences(sources[i]);
      for (std::size_t type = 0; type < fields.size(); ++type)
      {
        for (const auto& atom : *fields[type])
        {
          expected[type].emplace(atom.id(), atom.trigger_channel_id());
        }
      }
    }

    std::array<unsigned int, 4> order{0, 1, 2, 3};
    do
    {
      google::protobuf::Arena arena;
      PB::MatchResponse local;
      auto& result =
        use_arena ? *google::protobuf::Arena::Create<PB::MatchResponse>(&arena) : local;
      result.set_full_loaded(true);
      std::string hostname;
      for (const auto index : order)
      {
        CS::GrpcAlgs::merge_match_response(result, sources[index]);
        hostname += (hostname.empty() ? "" : ",") + sources[index].hostname();
      }

      const auto fields = sequences(result);
      for (std::size_t type = 0; type < fields.size(); ++type)
      {
        std::vector<Pair> actual;
        for (const auto& atom : *fields[type])
        {
          require(atom.GetArena() == result.GetArena(),
            "merged channel atoms must use the destination Arena");
          actual.emplace_back(atom.id(), atom.trigger_channel_id());
        }

        require(actual == std::vector<Pair>(expected[type].begin(), expected[type].end()),
          "merge lost pairs, retained duplicates or changed their order");
      }

      auto& actual_uids = *result.mutable_matched_channels()->mutable_uid_channels();
      require(actual_uids.GetArena() == result.GetArena(),
        "merged UID channels must use the destination Arena");
      require(std::vector<std::uint64_t>(actual_uids.begin(), actual_uids.end()) ==
        std::vector<std::uint64_t>{1, 3, 7, 9}, "UID channels must be sorted and unique");
      require(result.content_channels_size() == 1 && result.content_channels(0).id() == 42 &&
        result.content_channels(0).weight() == 10, "content weights must still be summed");
      require(result.no_adv() && result.no_track() && !result.full_loaded(), "merge flags changed");
      require(
        result.hostname() == hostname && result.match_time() == sources[order[0]].match_time(),
        "merge metadata changed");
    }
    while (std::next_permutation(order.begin(), order.end()));

    for (std::size_t i = 0; i < sources.size(); ++i)
    {
      require(sources[i].SerializeAsString() == original[i], "merge modified a source response");
    }

    PB::MatchResponse empty;
    PB::MatchResponse result;
    CS::GrpcAlgs::merge_match_response(result, empty);
    for (const auto* field : sequences(result))
    {
      require(field->empty(), "merging empty results added matches");
    }
  }

  void test_repeated_url()
  {
    CS::ChannelContainer container;
    CS::ChannelIdToMatchInfo_var info = new CS::ChannelIdToMatchInfo;
    for (const unsigned int id : {42, 7})
    {
      auto& item = (*info)[id];
      item.channel = CS::Channel(id);
      item.channel.mark_type(CS::Channel::CT_ACTIVE);
      item.channel.mark_type(CS::CT_URL);
      item.channel.set_weight(CS::CT_URL, 7);
      item.stamp = Generics::Time::get_time_of_day();
    }

    CS::UpdateContainer changes(&container, nullptr);
    changes.check_actual(*info);
    for (const unsigned int id : {42, 7})
    {
      CS::TriggerList empty;
      changes.select_parsed_triggers(id, empty, false);
      CS::MergeAtom atom;
      atom.id = id;
      for (const unsigned int trigger : {30, 10, 20})
      {
        CS::TriggerParser::TriggerParser::parse_url(
          id, trigger, "example.com/path", false, {80, 443}, atom.soft_words, nullptr);
      }

      changes.add_trigger(atom);
    }

    container.merge(changes, *info, true);

    CS::MatchUrls urls;
    CS::MatchWords words[CS::CT_MAX];
    CS::ChannelContainer::match_parse_urls(
      "http://example.com/path", {80, 443}, false, urls, nullptr);
    CS::TriggerMatchRes single;
    container.match(urls, {}, words, {}, {}, Generics::Uuid(), CS::MF_ACTIVE, single);
    CS::TriggerMatchRes repeated;
    container.match(urls, urls, words, {}, {}, Generics::Uuid(), CS::MF_ACTIVE, repeated);
    require(single.size() == 2 && repeated.size() == 2, "URL must match both channels");
    for (const auto& [id, item] : repeated)
    {
      const auto& ids = item.trigger_ids[CS::CT_URL];
      require(std::vector<CS::IdType>(ids.begin(), ids.end()) ==
        std::vector<CS::IdType>{10, 20, 30}, "repeated URL triggers must be sorted and unique");
      require(single[id].weight == 21 && item.weight == 42,
        "deduplication must preserve the weight of every hit");
    }

    require(single.count_channels[CS::CT_URL] == 6 && repeated.count_channels[CS::CT_URL] == 12,
      "deduplication must preserve hit counters");
  }
}

int main()
{
  try
  {
    test_repeated_url();
    test_merge(false);
    test_merge(true);
    std::cout << "ChannelMatchMergeTest passed\n";
    return 0;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "ChannelMatchMergeTest: " << ex.what() << '\n';
    return 1;
  }
}
