#include "ChannelGrpcAlgs.hpp"

#include <algorithm>
#include <string>

#include <Commons/Grpc/GrpcSync.hpp>
#include <Stream/MemoryStream.hpp>

namespace AdServer::ChannelSvcs::GrpcAlgs
{
  namespace
  {
    template<typename CorbaOctSeq>
    void unpack_oct_seq(const std::string& value, CorbaOctSeq& target)
    {
      target.length(value.size());
      std::copy(value.begin(), value.end(), target.get_buffer());
    }

    void unpack_channel_atoms(
      const google::protobuf::RepeatedPtrField<
        adserver::channel_svcs::channel_server::ChannelAtom>& source,
      ChannelServerBase::ChannelAtomSeq& target)
    {
      target.length(source.size());
      for(int i = 0; i < source.size(); ++i)
      {
        target[i].id = source[i].id();
        target[i].trigger_channel_id = source[i].trigger_channel_id();
      }
    }

    void throw_channel_exception_(const grpc::Status& status)
    {
      Stream::Error ostr;
      ostr << "ChannelServer grpc match failed: code=" <<
        static_cast<int>(status.error_code()) <<
        ", message=" << status.error_message();
      throw Exception(ostr);
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

  void make_match_request(
    const ChannelServerBase::MatchQuery& source,
    adserver::channel_svcs::channel_server::MatchRequest& target)
  {
    target.set_request_id(source.request_id.in());
    target.set_first_url(source.first_url.in());
    target.set_first_url_words(source.first_url_words.in());
    target.set_urls(source.urls.in());
    target.set_urls_words(source.urls_words.in());
    target.set_pwords(source.pwords.in());
    target.set_swords(source.swords.in());
    target.set_uid(
      reinterpret_cast<const char*>(source.uid.get_buffer()),
      source.uid.length());
    target.set_statuses(source.statuses, sizeof(source.statuses));
    target.set_non_strict_word_match(source.non_strict_word_match);
    target.set_non_strict_url_match(source.non_strict_url_match);
    target.set_return_negative(source.return_negative);
    target.set_simplify_page(source.simplify_page);
    target.set_fill_content(source.fill_content);
  }

  ChannelServerBase::MatchResult* make_match_result(
    const adserver::channel_svcs::channel_server::MatchResponse& source)
  {
    ChannelServerBase::MatchResult_var result =
      new ChannelServerBase::MatchResult;
    unpack_channel_atoms(
      source.matched_channels().page_channels(),
      result->matched_channels.page_channels);
    unpack_channel_atoms(
      source.matched_channels().search_channels(),
      result->matched_channels.search_channels);
    unpack_channel_atoms(
      source.matched_channels().url_channels(),
      result->matched_channels.url_channels);
    unpack_channel_atoms(
      source.matched_channels().url_keyword_channels(),
      result->matched_channels.url_keyword_channels);

    result->matched_channels.uid_channels.length(
      source.matched_channels().uid_channels_size());
    for(int i = 0; i < source.matched_channels().uid_channels_size(); ++i)
    {
      result->matched_channels.uid_channels[i] =
        source.matched_channels().uid_channels(i);
    }

    result->content_channels.length(source.content_channels_size());
    for(int i = 0; i < source.content_channels_size(); ++i)
    {
      result->content_channels[i].id = source.content_channels(i).id();
      result->content_channels[i].weight =
        source.content_channels(i).weight();
    }

    result->no_adv = source.no_adv();
    result->no_track = source.no_track();
    result->match_time.length(source.match_time().empty() ? 0 : 1);
    if(!source.match_time().empty())
    {
      unpack_oct_seq(source.match_time(), result->match_time[0]);
    }

    return result._retn();
  }

  void make_get_ccg_traits_request(
    const ChannelIdSeq& source,
    adserver::channel_svcs::channel_server::GetCcgTraitsRequest& target)
  {
    for (CORBA::ULong i = 0; i < source.length(); ++i)
    {
      target.add_ids(source[i]);
    }
  }

  ChannelServerBase::CCGKeywordSeq* make_ccg_traits_result(
    const adserver::channel_svcs::channel_server::GetCcgTraitsResponse& source)
  {
    ChannelServerBase::CCGKeywordSeq_var result =
      new ChannelServerBase::CCGKeywordSeq;
    result->length(source.ccg_keywords_size());
    for (int i = 0; i < source.ccg_keywords_size(); ++i)
    {
      const auto& source_keyword = source.ccg_keywords(i);
      auto& target_keyword = (*result)[i];
      target_keyword.ccg_keyword_id = source_keyword.ccg_keyword_id();
      target_keyword.ccg_id = source_keyword.ccg_id();
      target_keyword.channel_id = source_keyword.channel_id();
      unpack_oct_seq(source_keyword.max_cpc(), target_keyword.max_cpc);
      unpack_oct_seq(source_keyword.ctr(), target_keyword.ctr);
      target_keyword.click_url = source_keyword.click_url().c_str();
      target_keyword.original_keyword = source_keyword.original_keyword().c_str();
    }

    return result._retn();
  }
}
