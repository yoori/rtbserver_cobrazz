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
