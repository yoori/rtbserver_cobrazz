#include "ChannelUpdateGrpcClient.hpp"

#include <grpcpp/grpcpp.h>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/GrpcAlgs.hpp>

namespace AdServer::ChannelSvcs
{
  namespace
  {
    namespace pb = adserver::channel_svcs::channel_server;

    Generics::Time unpack_stamp(const std::string& source)
    {
      if (source.size() != Generics::Time::TIME_PACK_LEN)
      {
        throw ImplementationException("invalid timestamp in ChannelUpdate gRPC response");
      }

      return ::GrpcAlgs::unpack_time(source);
    }

    template<class Request, class Response, class Method>
    Response call(
      pb::ChannelServerGrpc::Stub& stub, Method method, const Request& request,
      std::chrono::milliseconds timeout)
    {
      grpc::ClientContext context;
      context.set_deadline(std::chrono::system_clock::now() + timeout);
      Response response;
      const auto status = (stub.*method)(&context, request, &response);
      if (!status.ok())
      {
        const std::string error = "ChannelUpdate gRPC " + std::to_string(status.error_code()) +
          ": " + status.error_message();
        if (status.error_code() == grpc::StatusCode::FAILED_PRECONDITION)
        {
          throw NotConfigured(error.c_str());
        }

        throw ImplementationException(error.c_str());
      }

      return response;
    }
  }

  ChannelUpdateSource::~ChannelUpdateSource() = default;

  ChannelUpdateGrpcClient::ChannelUpdateGrpcClient(
    const std::string& endpoint, std::chrono::milliseconds timeout)
    : timeout_(timeout)
  {
    if (endpoint.empty() || endpoint.find("corbaloc:") == 0 || endpoint.find("IOR:") == 0)
    {
      throw ImplementationException("ChannelUpdate requires a gRPC endpoint");
    }

    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
    args.SetMaxReceiveMessageSize(-1);
    args.SetMaxSendMessageSize(-1);
    stub_ = pb::ChannelServerGrpc::NewStub(
      grpc::CreateCustomChannel(endpoint, grpc::InsecureChannelCredentials(), args));
  }

  void ChannelUpdateGrpcClient::check(
    const ChannelUpdate::CheckQuery& query, ChannelUpdate::CheckDataPtr& data)
  {
    pb::CheckRequest request;
    request.set_colo_id(query.colo_id);
    request.set_version(query.version);
    request.set_master_stamp(::GrpcAlgs::pack_time(query.master_stamp));
    request.set_use_only_list(query.use_only_list);
    for (std::uint32_t i = 0; i < query.new_ids.size(); ++i)
    {
      request.add_new_ids(query.new_ids[i]);
    }

    const auto response = call<pb::CheckRequest, pb::CheckResponse>(
      *stub_, &pb::ChannelServerGrpc::Stub::check, request, timeout_);
    auto result = std::make_unique<ChannelUpdate::CheckData>();
    result->first_stamp = unpack_stamp(response.first_stamp());
    result->master_stamp = unpack_stamp(response.master_stamp());
    result->max_time = unpack_stamp(response.max_time());
    result->source_id = response.source_id();
    result->special_adv = response.special_adv();
    result->special_track = response.special_track();
    result->versions.resize(response.versions_size());
    for (int i = 0; i < response.versions_size(); ++i)
    {
      const auto& source = response.versions(i);
      auto& target = result->versions[i];
      target.id = source.id();
      target.size = source.size();
      target.stamp = unpack_stamp(source.stamp());
    }

    data = std::move(result);
  }

  void ChannelUpdateGrpcClient::update_triggers(
    const ChannelIdList& ids, ChannelUpdate::UpdateDataPtr& data)
  {
    pb::UpdateTriggersRequest request;
    for (std::uint32_t i = 0; i < ids.size(); ++i)
    {
      request.add_ids(ids[i]);
    }

    const auto response = call<pb::UpdateTriggersRequest, pb::UpdateTriggersResponse>(
      *stub_, &pb::ChannelServerGrpc::Stub::update_triggers, request, timeout_);
    auto result = std::make_unique<ChannelUpdate::UpdateData>();
    result->source_id = response.source_id();
    result->channels.resize(response.channels_size());
    for (int i = 0; i < response.channels_size(); ++i)
    {
      const auto& source = response.channels(i);
      auto& target = result->channels[i];
      target.channel_id = source.channel_id();
      target.stamp = unpack_stamp(source.stamp());
      target.words.resize(source.words_size());
      for (int j = 0; j < source.words_size(); ++j)
      {
        target.words[j].channel_trigger_id = source.words(j).channel_trigger_id();
        target.words[j].trigger = source.words(j).trigger();
      }
    }

    data = std::move(result);
  }

  void ChannelUpdateGrpcClient::update_all_ccg(
    const ChannelUpdate::CCGQuery& query, ChannelUpdate::PosCCGResultPtr& data)
  {
    pb::UpdateAllCcgRequest request;
    request.set_master_stamp(::GrpcAlgs::pack_time(query.master_stamp));
    request.set_start(query.start);
    request.set_limit(query.limit);
    request.set_use_only_list(query.use_only_list);
    for (std::uint32_t i = 0; i < query.channel_ids.size(); ++i)
    {
      request.add_channel_ids(query.channel_ids[i]);
    }

    const auto response = call<pb::UpdateAllCcgRequest, pb::UpdateAllCcgResponse>(
      *stub_, &pb::ChannelServerGrpc::Stub::update_all_ccg, request, timeout_);
    auto result = std::make_unique<ChannelUpdate::PosCCGResult>();
    result->source_id = response.source_id();
    result->start_id = response.start_id();
    result->keywords.resize(response.keywords_size());
    for (int i = 0; i < response.keywords_size(); ++i)
    {
      const auto& source = response.keywords(i);
      auto& target = result->keywords[i];
      if (source.max_cpc().size() != CampaignSvcs::RevenueDecimal::PACK_SIZE ||
          source.ctr().size() != CampaignSvcs::CTRDecimal::PACK_SIZE)
      {
        throw ImplementationException("invalid decimal in ChannelUpdate gRPC response");
      }

      target.ccg_keyword_id = source.ccg_keyword_id();
      target.ccg_id = source.ccg_id();
      target.channel_id = source.channel_id();
      target.max_cpc = ::GrpcAlgs::unpack_decimal<CampaignSvcs::RevenueDecimal>(source.max_cpc());
      target.ctr = ::GrpcAlgs::unpack_decimal<CampaignSvcs::CTRDecimal>(source.ctr());
      target.click_url = source.click_url();
      target.original_keyword = source.original_keyword();
    }

    result->deleted.resize(response.deleted_size());
    for (int i = 0; i < response.deleted_size(); ++i)
    {
      result->deleted[i].id = response.deleted(i).id();
      result->deleted[i].stamp = unpack_stamp(response.deleted(i).stamp());
    }

    data = std::move(result);
  }

  std::uint32_t ChannelUpdateGrpcClient::get_count_chunks()
  {
    const pb::GetCountChunksRequest request;
    const auto response = call<pb::GetCountChunksRequest, pb::GetCountChunksResponse>(
      *stub_, &pb::ChannelServerGrpc::Stub::get_count_chunks, request, timeout_);
    return response.count_chunks();
  }
}
