#include "CampaignServerClient.hpp"
#include <CampaignSvcs/CampaignServer/CampaignServerGrpcAlgs.hpp>

#include <stdexcept>
#include <grpcpp/grpcpp.h>

namespace AdServer::CampaignSvcs
{
  namespace cs_rpc = adserver::campaign::server;

  namespace
  {
    template<class Request, class Response, class Method>
    void call(cs_rpc::CampaignServer::Stub& stub, Method method, const Request& request,
      Response& response, std::chrono::milliseconds timeout)
    {
      grpc::ClientContext context;
      context.set_deadline(std::chrono::system_clock::now() + timeout);
      const auto status = (stub.*method)(&context, request, &response);
      if (status.ok())
      {
        return;
      }

      const std::string message = "CampaignServer gRPC " + std::to_string(status.error_code()) +
        ": " + status.error_message();
      if (status.error_code() == grpc::StatusCode::FAILED_PRECONDITION)
      {
        throw CampaignServer::NotReady(message.c_str());
      }

      if (status.error_code() == grpc::StatusCode::UNIMPLEMENTED)
      {
        throw CampaignServer::NotSupport(message.c_str());
      }

      throw CampaignServer::ImplementationException(message.c_str());
    }
  }

  CampaignServerClient::CampaignServerClient(
    const std::string& endpoint, std::chrono::milliseconds timeout)
    : timeout_(timeout)
  {
    if (endpoint.empty() || endpoint.find("corbaloc:") == 0 ||
        endpoint.find("IOR:") == 0 || timeout.count() <= 0)
    {
      throw std::invalid_argument("CampaignServer requires a gRPC endpoint and positive timeout");
    }

    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
    args.SetInt(GRPC_ARG_ENABLE_RETRIES, 0);
    args.SetMaxReceiveMessageSize(-1);
    args.SetMaxSendMessageSize(-1);
    stub_ = cs_rpc::CampaignServer::NewStub(
      grpc::CreateCustomChannel(endpoint, grpc::InsecureChannelCredentials(), args));
  }

  CampaignConfigUpdateInfo* CampaignServerClient::get_config(
    const CampaignGetConfigSettings& get_config_settings)
  {
    cs_rpc::get_configRequest request;
    CampaignGrpc::pack(get_config_settings, *request.mutable_get_config_settings());
    cs_rpc::get_configResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::get_config, request, response, timeout_);
    auto result = std::make_unique<CampaignConfigUpdateInfo>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  bool CampaignServerClient::need_config(const TimestampInfo& req_timestamp)
  {
    cs_rpc::need_configRequest request;
    request.set_req_timestamp(
      reinterpret_cast<const char*>(req_timestamp.data()), req_timestamp.size());
    cs_rpc::need_configResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::need_config, request, response, timeout_);
    return response.value();
  }

  EcpmSeq* CampaignServerClient::get_ecpms(const TimestampInfo& request_timestamp)
  {
    cs_rpc::get_ecpmsRequest request;
    request.set_request_timestamp(
      reinterpret_cast<const char*>(request_timestamp.data()), request_timestamp.size());
    cs_rpc::get_ecpmsResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::get_ecpms, request, response, timeout_);
    auto result = std::make_unique<EcpmSeq>();
    try
    {
      result->resize(response.value_size());
      for (std::uint32_t i = 0; i < result->size(); ++i)
      {
        CampaignGrpc::unpack(response.value(i), (*result)[i]);
      }
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  ExpressionChannelsInfo* CampaignServerClient::get_expression_channels(
    const CampaignServer::GetExpressionChannelsInfo& params)
  {
    cs_rpc::get_expression_channelsRequest request;
    CampaignGrpc::pack(params, *request.mutable_params());
    cs_rpc::get_expression_channelsResponse response;
    call(
      *stub_, &cs_rpc::CampaignServer::Stub::get_expression_channels, request, response, timeout_);
    auto result = std::make_unique<ExpressionChannelsInfo>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  CampaignServer::PassbackInfo*
  CampaignServerClient::get_tag_passback(std::uint32_t tag_id, const char* app_format)
  {
    cs_rpc::get_tag_passbackRequest request;
    request.set_tag_id(tag_id);
    request.set_app_format(app_format);
    cs_rpc::get_tag_passbackResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::get_tag_passback, request, response, timeout_);
    auto result = std::make_unique<CampaignServer::PassbackInfo>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  BriefSimpleChannelAnswer* CampaignServerClient::brief_simple_channels(
    const CampaignServer::GetSimpleChannelsInfo& params)
  {
    cs_rpc::brief_simple_channelsRequest request;
    CampaignGrpc::pack(params, *request.mutable_params());
    cs_rpc::brief_simple_channelsResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::brief_simple_channels, request, response, timeout_);
    auto result = std::make_unique<BriefSimpleChannelAnswer>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  SimpleChannelAnswer* CampaignServerClient::simple_channels(
    const CampaignServer::GetSimpleChannelsInfo& params)
  {
    cs_rpc::simple_channelsRequest request;
    CampaignGrpc::pack(params, *request.mutable_params());
    cs_rpc::simple_channelsResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::simple_channels, request, response, timeout_);
    auto result = std::make_unique<SimpleChannelAnswer>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  ChannelServerChannelAnswer* CampaignServerClient::chsv_simple_channels(
    const CampaignServer::GetSimpleChannelsInfo& params)
  {
    cs_rpc::chsv_simple_channelsRequest request;
    CampaignGrpc::pack(params, *request.mutable_params());
    cs_rpc::chsv_simple_channelsResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::chsv_simple_channels, request, response, timeout_);
    auto result = std::make_unique<ChannelServerChannelAnswer>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  FraudConditionConfig* CampaignServerClient::fraud_conditions()
  {
    cs_rpc::fraud_conditionsRequest request;
    cs_rpc::fraud_conditionsResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::fraud_conditions, request, response, timeout_);
    auto result = std::make_unique<FraudConditionConfig>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  FreqCapConfigInfo* CampaignServerClient::freq_caps()
  {
    cs_rpc::freq_capsRequest request;
    cs_rpc::freq_capsResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::freq_caps, request, response, timeout_);
    auto result = std::make_unique<FreqCapConfigInfo>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  ColocationFlagsSeq* CampaignServerClient::get_colocation_flags()
  {
    cs_rpc::get_colocation_flagsRequest request;
    cs_rpc::get_colocation_flagsResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::get_colocation_flags, request, response, timeout_);
    auto result = std::make_unique<ColocationFlagsSeq>();
    try
    {
      result->resize(response.value_size());
      for (std::uint32_t i = 0; i < result->size(); ++i)
      {
        CampaignGrpc::unpack(response.value(i), (*result)[i]);
      }
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  DetectorsConfig* CampaignServerClient::detectors(const TimestampInfo& request_timestamp)
  {
    cs_rpc::detectorsRequest request;
    request.set_request_timestamp(
      reinterpret_cast<const char*>(request_timestamp.data()), request_timestamp.size());
    cs_rpc::detectorsResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::detectors, request, response, timeout_);
    auto result = std::make_unique<DetectorsConfig>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  void CampaignServerClient::update_stat()
  {
    cs_rpc::update_statRequest request;
    cs_rpc::update_statResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::update_stat, request, response, timeout_);
  }

  StatInfo* CampaignServerClient::get_stat()
  {
    cs_rpc::get_statRequest request;
    cs_rpc::get_statResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::get_stat, request, response, timeout_);
    auto result = std::make_unique<StatInfo>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  BillStatInfo* CampaignServerClient::get_bill_stat()
  {
    cs_rpc::get_bill_statRequest request;
    cs_rpc::get_bill_statResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::get_bill_stat, request, response, timeout_);
    auto result = std::make_unique<BillStatInfo>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  CampaignServer::ColocationPropInfo* CampaignServerClient::get_colocation_prop(
    std::uint32_t colo_id)
  {
    cs_rpc::get_colocation_propRequest request;
    request.set_colo_id(colo_id);
    cs_rpc::get_colocation_propResponse response;
    call(*stub_, &cs_rpc::CampaignServer::Stub::get_colocation_prop, request, response, timeout_);
    auto result = std::make_unique<CampaignServer::ColocationPropInfo>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }

  CampaignServer::DeliveryLimitConfigInfo* CampaignServerClient::get_delivery_limit_config()
  {
    cs_rpc::get_delivery_limit_configRequest request;
    cs_rpc::get_delivery_limit_configResponse response;
    call(
      *stub_, &cs_rpc::CampaignServer::Stub::get_delivery_limit_config,
      request, response, timeout_);
    auto result = std::make_unique<CampaignServer::DeliveryLimitConfigInfo>();
    try
    {
      CampaignGrpc::unpack(response.value(), *result);
    }
    catch (const std::exception& e)
    {
      throw CampaignServer::ImplementationException(e.what());
    }

    return result.release();
  }
}
