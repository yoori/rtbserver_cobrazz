#include "CampaignServerGrpc.hpp"
#include "CampaignServerGrpcAlgs.hpp"

#include <memory>
#include <stdexcept>
#include <grpcpp/grpcpp.h>

namespace AdServer::CampaignSvcs
{
  namespace
  {
    template<class Action>
    grpc::Status invoke(grpc::ServerContext* context, Action&& action)
    {
      if (context->IsCancelled())
      {
        return {grpc::StatusCode::CANCELLED, "CampaignServer request cancelled"};
      }

      try
      {
        action();
        return grpc::Status::OK;
      }
      catch (const CampaignServer::NotReady& e)
      {
        return {grpc::StatusCode::FAILED_PRECONDITION, e.what()};
      }
      catch (const CampaignServer::NotSupport& e)
      {
        return {grpc::StatusCode::UNIMPLEMENTED, e.what()};
      }
      catch (const CampaignServer::ImplementationException& e)
      {
        return {grpc::StatusCode::INTERNAL, e.what()};
      }
      catch (const std::invalid_argument& e)
      {
        return {grpc::StatusCode::INVALID_ARGUMENT, e.what()};
      }
      catch (const std::exception& e)
      {
        return {grpc::StatusCode::INTERNAL, e.what()};
      }
      catch (...)
      {
        return {grpc::StatusCode::INTERNAL, "Unknown CampaignServer failure"};
      }
    }

    template<class Settings>
    void validate_portion(const Settings& settings)
    {
      if (!settings.portions_number || settings.portion >= settings.portions_number)
      {
        throw std::invalid_argument("Invalid campaign portion");
      }
    }
  }

  CampaignServerGrpcService::CampaignServerGrpcService(CampaignServerApi& source)
    : source_(source)
  {}

  grpc::Status CampaignServerGrpcService::get_config(
    grpc::ServerContext* context, const cs_rpc::get_configRequest* request,
    cs_rpc::get_configResponse* response)
  {
    return invoke(context, [&]
    {
      CampaignGetConfigSettings get_config_settings;
      CampaignGrpc::unpack(request->get_config_settings(), get_config_settings);
      validate_portion(get_config_settings);
      std::unique_ptr<CampaignConfigUpdateInfo> value(source_.get_config(get_config_settings));
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::need_config(
    grpc::ServerContext* context, const cs_rpc::need_configRequest* request,
    cs_rpc::need_configResponse* response)
  {
    return invoke(context, [&]
    {
      TimestampInfo req_timestamp;
      if (request->req_timestamp().size() != 8)
      {
        throw std::invalid_argument("Invalid request timestamp");
      }

      req_timestamp.resize(8);
      std::copy(
        request->req_timestamp().begin(),
        request->req_timestamp().end(),
        req_timestamp.data());
      response->set_value(source_.need_config(req_timestamp));
    });
  }

  grpc::Status CampaignServerGrpcService::get_ecpms(
    grpc::ServerContext* context, const cs_rpc::get_ecpmsRequest* request,
    cs_rpc::get_ecpmsResponse* response)
  {
    return invoke(context, [&]
    {
      TimestampInfo request_timestamp;
      if (request->request_timestamp().size() != 8)
      {
        throw std::invalid_argument("Invalid request timestamp");
      }

      request_timestamp.resize(8);
      std::copy(
        request->request_timestamp().begin(),
        request->request_timestamp().end(),
        request_timestamp.data());
      std::unique_ptr<EcpmSeq> value(source_.get_ecpms(request_timestamp));
      for (std::uint32_t i = 0; i < value->size(); ++i)
      {
        CampaignGrpc::pack((*value)[i], *response->add_value());
      }
    });
  }

  grpc::Status CampaignServerGrpcService::get_expression_channels(
    grpc::ServerContext* context, const cs_rpc::get_expression_channelsRequest* request,
    cs_rpc::get_expression_channelsResponse* response)
  {
    return invoke(context, [&]
    {
      CampaignServer::GetExpressionChannelsInfo params;
      CampaignGrpc::unpack(request->params(), params);
      validate_portion(params);
      std::unique_ptr<ExpressionChannelsInfo> value(source_.get_expression_channels(params));
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::get_tag_passback(
    grpc::ServerContext* context, const cs_rpc::get_tag_passbackRequest* request,
    cs_rpc::get_tag_passbackResponse* response)
  {
    return invoke(context, [&]
    {
      std::unique_ptr<CampaignServer::PassbackInfo> value(
        source_.get_tag_passback(request->tag_id(), request->app_format().c_str()));
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::brief_simple_channels(
    grpc::ServerContext* context, const cs_rpc::brief_simple_channelsRequest* request,
    cs_rpc::brief_simple_channelsResponse* response)
  {
    return invoke(context, [&]
    {
      CampaignServer::GetSimpleChannelsInfo params;
      CampaignGrpc::unpack(request->params(), params);
      validate_portion(params);
      std::unique_ptr<BriefSimpleChannelAnswer> value(source_.brief_simple_channels(params));
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::simple_channels(
    grpc::ServerContext* context, const cs_rpc::simple_channelsRequest* request,
    cs_rpc::simple_channelsResponse* response)
  {
    return invoke(context, [&]
    {
      CampaignServer::GetSimpleChannelsInfo params;
      CampaignGrpc::unpack(request->params(), params);
      validate_portion(params);
      std::unique_ptr<SimpleChannelAnswer> value(source_.simple_channels(params));
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::chsv_simple_channels(
    grpc::ServerContext* context, const cs_rpc::chsv_simple_channelsRequest* request,
    cs_rpc::chsv_simple_channelsResponse* response)
  {
    return invoke(context, [&]
    {
      CampaignServer::GetSimpleChannelsInfo params;
      CampaignGrpc::unpack(request->params(), params);
      validate_portion(params);
      std::unique_ptr<ChannelServerChannelAnswer> value(source_.chsv_simple_channels(params));
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::fraud_conditions(
    grpc::ServerContext* context, const cs_rpc::fraud_conditionsRequest*,
    cs_rpc::fraud_conditionsResponse* response)
  {
    return invoke(context, [&]
    {
      std::unique_ptr<FraudConditionConfig> value(source_.fraud_conditions());
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::freq_caps(
    grpc::ServerContext* context, const cs_rpc::freq_capsRequest*,
    cs_rpc::freq_capsResponse* response)
  {
    return invoke(context, [&]
    {
      std::unique_ptr<FreqCapConfigInfo> value(source_.freq_caps());
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::get_colocation_flags(
    grpc::ServerContext* context, const cs_rpc::get_colocation_flagsRequest*,
    cs_rpc::get_colocation_flagsResponse* response)
  {
    return invoke(context, [&]
    {
      std::unique_ptr<ColocationFlagsSeq> value(source_.get_colocation_flags());
      for (std::uint32_t i = 0; i < value->size(); ++i)
      {
        CampaignGrpc::pack((*value)[i], *response->add_value());
      }
    });
  }

  grpc::Status CampaignServerGrpcService::detectors(
    grpc::ServerContext* context, const cs_rpc::detectorsRequest* request,
    cs_rpc::detectorsResponse* response)
  {
    return invoke(context, [&]
    {
      TimestampInfo request_timestamp;
      if (request->request_timestamp().size() != 8)
      {
        throw std::invalid_argument("Invalid request timestamp");
      }

      request_timestamp.resize(8);
      std::copy(
        request->request_timestamp().begin(),
        request->request_timestamp().end(),
        request_timestamp.data());
      std::unique_ptr<DetectorsConfig> value(source_.detectors(request_timestamp));
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::update_stat(
    grpc::ServerContext* context, const cs_rpc::update_statRequest*,
    cs_rpc::update_statResponse*)
  {
    return invoke(context, [&]
    {
      source_.update_stat();
    });
  }

  grpc::Status CampaignServerGrpcService::get_stat(
    grpc::ServerContext* context, const cs_rpc::get_statRequest*,
    cs_rpc::get_statResponse* response)
  {
    return invoke(context, [&]
    {
      std::unique_ptr<StatInfo> value(source_.get_stat());
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::get_bill_stat(
    grpc::ServerContext* context, const cs_rpc::get_bill_statRequest*,
    cs_rpc::get_bill_statResponse* response)
  {
    return invoke(context, [&]
    {
      std::unique_ptr<BillStatInfo> value(source_.get_bill_stat());
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::get_colocation_prop(
    grpc::ServerContext* context, const cs_rpc::get_colocation_propRequest* request,
    cs_rpc::get_colocation_propResponse* response)
  {
    return invoke(context, [&]
    {
      std::unique_ptr<CampaignServer::ColocationPropInfo> value(
        source_.get_colocation_prop(request->colo_id()));
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

  grpc::Status CampaignServerGrpcService::get_delivery_limit_config(
    grpc::ServerContext* context, const cs_rpc::get_delivery_limit_configRequest*,
    cs_rpc::get_delivery_limit_configResponse* response)
  {
    return invoke(context, [&]
    {
      std::unique_ptr<CampaignServer::DeliveryLimitConfigInfo> value(
        source_.get_delivery_limit_config());
      CampaignGrpc::pack(*value, *response->mutable_value());
    });
  }

}
