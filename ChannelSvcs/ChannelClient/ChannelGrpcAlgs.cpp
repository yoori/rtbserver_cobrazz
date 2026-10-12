#include "ChannelGrpcAlgs.hpp"

#include <Commons/Grpc/GrpcSync.hpp>
#include <Stream/MemoryStream.hpp>

namespace AdServer::ChannelSvcs::GrpcAlgs
{
  namespace
  {
    void throw_channel_exception_(const grpc::Status& status)
    {
      Stream::Error ostr;
      ostr << "ChannelServer grpc match failed: code=" <<
        static_cast<int>(status.error_code()) << ", message=" << status.error_message();
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
}
