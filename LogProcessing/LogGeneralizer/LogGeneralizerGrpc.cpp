#include "LogGeneralizerGrpc.hpp"

#include <utility>
#include <grpcpp/grpcpp.h>
#include <Commons/GrpcAlgs.hpp>

namespace AdServer::LogProcessing
{
  namespace
  {
    Generics::Time unpack_time(const std::string& value)
    {
      if (value.size() != Generics::Time::TIME_PACK_LEN)
      {
        throw LogGeneralizerError("Invalid timestamp in LogGeneralizer response");
      }

      return GrpcAlgs::unpack_time(value);
    }

    void pack(const CreativeStatInfo& source, lg_rpc::CreativeStatInfo& target)
    {
      target.set_cc_id(source.cc_id);
      target.set_requests(source.requests);
      target.set_impressions(source.impressions);
      target.set_clicks(source.clicks);
      target.set_actions(source.actions);
    }

    CreativeStatInfo unpack(const lg_rpc::CreativeStatInfo& source)
    {
      CreativeStatInfo target;
      target.cc_id = source.cc_id();
      target.requests = source.requests();
      target.impressions = source.impressions();
      target.clicks = source.clicks();
      target.actions = source.actions();
      return target;
    }

    void pack(const PublisherAmountInfo& source, lg_rpc::PublisherAmountInfo& target)
    {
      target.set_publisher_account_id(source.publisher_account_id);
      target.set_adv_amount(source.adv_amount.str());
    }

    PublisherAmountInfo unpack(const lg_rpc::PublisherAmountInfo& source)
    {
      PublisherAmountInfo target;
      target.publisher_account_id = source.publisher_account_id();
      target.adv_amount = StatDecimal(source.adv_amount());
      return target;
    }

    void pack(const TagAmountInfo& source, lg_rpc::TagAmountInfo& target)
    {
      target.set_tag_id(source.tag_id);
      target.set_pub_isp_amount(source.pub_isp_amount.str());
      target.set_adv_amount(source.adv_amount.str());
      target.set_adv_comm_amount(source.adv_comm_amount.str());
    }

    TagAmountInfo unpack(const lg_rpc::TagAmountInfo& source)
    {
      TagAmountInfo target;
      target.tag_id = source.tag_id();
      target.pub_isp_amount = StatDecimal(source.pub_isp_amount());
      target.adv_amount = StatDecimal(source.adv_amount());
      target.adv_comm_amount = StatDecimal(source.adv_comm_amount());
      return target;
    }

    void pack(const CtrResetStatInfo& source, lg_rpc::CtrResetStatInfo& target)
    {
      target.set_ctr_reset_id(source.ctr_reset_id);
      target.set_impressions(source.impressions);
    }

    CtrResetStatInfo unpack(const lg_rpc::CtrResetStatInfo& source)
    {
      CtrResetStatInfo target;
      target.ctr_reset_id = source.ctr_reset_id();
      target.impressions = source.impressions();
      return target;
    }

    void pack(const CampaignStatInfo& source, lg_rpc::CampaignStatInfo& target)
    {
      target.set_sdate(GrpcAlgs::pack_time(source.sdate));
      target.set_adv_sdate(GrpcAlgs::pack_time(source.adv_sdate));
      target.set_adv_account_id(source.adv_account_id);
      target.set_campaign_id(source.campaign_id);
      target.set_ccg_id(source.ccg_id);
      target.set_adv_account_amount(source.adv_account_amount.str());
      target.set_adv_amount(source.adv_amount.str());
      target.set_adv_comm_amount(source.adv_comm_amount.str());
      target.set_adv_payable_comm_amount(source.adv_payable_comm_amount.str());
      for (const auto& value : source.creative_stats)
      {
        pack(value, *target.add_creative_stats());
      }

      for (const auto& value : source.publisher_amounts)
      {
        pack(value, *target.add_publisher_amounts());
      }

      for (const auto& value : source.tag_amounts)
      {
        pack(value, *target.add_tag_amounts());
      }

      for (const auto& value : source.ctr_reset_stats)
      {
        pack(value, *target.add_ctr_reset_stats());
      }

    }

    CampaignStatInfo unpack(const lg_rpc::CampaignStatInfo& source)
    {
      CampaignStatInfo target;
      target.sdate = unpack_time(source.sdate());
      target.adv_sdate = unpack_time(source.adv_sdate());
      target.adv_account_id = source.adv_account_id();
      target.campaign_id = source.campaign_id();
      target.ccg_id = source.ccg_id();
      target.adv_account_amount = StatDecimal(source.adv_account_amount());
      target.adv_amount = StatDecimal(source.adv_amount());
      target.adv_comm_amount = StatDecimal(source.adv_comm_amount());
      target.adv_payable_comm_amount = StatDecimal(source.adv_payable_comm_amount());
      target.creative_stats.reserve(source.creative_stats_size());
      for (const auto& value : source.creative_stats())
      {
        target.creative_stats.push_back(unpack(value));
      }

      target.publisher_amounts.reserve(source.publisher_amounts_size());
      for (const auto& value : source.publisher_amounts())
      {
        target.publisher_amounts.push_back(unpack(value));
      }

      target.tag_amounts.reserve(source.tag_amounts_size());
      for (const auto& value : source.tag_amounts())
      {
        target.tag_amounts.push_back(unpack(value));
      }

      target.ctr_reset_stats.reserve(source.ctr_reset_stats_size());
      for (const auto& value : source.ctr_reset_stats())
      {
        target.ctr_reset_stats.push_back(unpack(value));
      }

      return target;
    }

    void pack(const StatInfo& source, lg_rpc::StatInfo& target)
    {
      for (const auto& value : source.campaign_stats)
      {
        pack(value, *target.add_campaign_stats());
      }

    }

    StatInfo unpack(const lg_rpc::StatInfo& source)
    {
      StatInfo target;
      target.campaign_stats.reserve(source.campaign_stats_size());
      for (const auto& value : source.campaign_stats())
      {
        target.campaign_stats.push_back(unpack(value));
      }

      return target;
    }

    template<class Action>
    grpc::Status invoke(grpc::ServerContext* context, Action&& action)
    {
      if (context->IsCancelled())
      {
        return {grpc::StatusCode::CANCELLED, "LogGeneralizer request cancelled"};
      }

      try
      {
        action();
        return grpc::Status::OK;
      }
      catch (const NotSupported& error)
      {
        return {grpc::StatusCode::UNIMPLEMENTED, error.what()};
      }
      catch (const CollectionNotStarted& error)
      {
        return {grpc::StatusCode::FAILED_PRECONDITION, error.what()};
      }
      catch (const std::exception& error)
      {
        return {grpc::StatusCode::INTERNAL, error.what()};
      }
      catch (...)
      {
        return {grpc::StatusCode::INTERNAL, "Unknown LogGeneralizer failure"};
      }
    }

    template<class Response, class Method>
    Response call(
      lg_rpc::LogGeneralizer::Stub& stub, Method method, std::uint32_t client_id, bool clear,
      std::chrono::milliseconds timeout)
    {
      lg_rpc::StatRequest request;
      request.set_client_id(client_id);
      request.set_clear(clear);
      grpc::ClientContext context;
      context.set_deadline(std::chrono::system_clock::now() + timeout);
      Response response;
      const auto status = (stub.*method)(&context, request, &response);
      if (!status.ok())
      {
        const std::string message = "LogGeneralizer gRPC " + std::to_string(status.error_code()) +
          ": " + status.error_message();
        if (status.error_code() == grpc::StatusCode::UNIMPLEMENTED)
        {
          throw NotSupported(message);
        }

        if (status.error_code() == grpc::StatusCode::FAILED_PRECONDITION)
        {
          throw CollectionNotStarted(message);
        }

        throw LogGeneralizerError(message);
      }

      return response;
    }
  }

  LogGeneralizerGrpcService::LogGeneralizerGrpcService(Stop stop, Start start, Get get)
    : stop_(std::move(stop)), start_(std::move(start)), get_(std::move(get))
  {}

  grpc::Status LogGeneralizerGrpcService::stop_stat_upload(
    grpc::ServerContext* context, const lg_rpc::StatRequest* request, lg_rpc::Empty*)
  {
    return invoke(context, [&] { stop_(request->client_id()); });
  }

  grpc::Status LogGeneralizerGrpcService::start_stat_upload(
    grpc::ServerContext* context, const lg_rpc::StatRequest* request, lg_rpc::Empty*)
  {
    return invoke(context, [&] { start_(request->client_id(), request->clear()); });
  }

  grpc::Status LogGeneralizerGrpcService::get_stat_info(
    grpc::ServerContext* context, const lg_rpc::StatRequest* request, lg_rpc::StatInfo* response)
  {
    return invoke(context, [&] { pack(get_(request->client_id(), request->clear()), *response); });
  }

  LogGeneralizerClient::LogGeneralizerClient(
    const std::string& endpoint, std::chrono::milliseconds timeout)
    : timeout_(timeout)
  {
    if (timeout.count() <= 0 || endpoint.empty() ||
        endpoint.find("corbaloc:") == 0 || endpoint.find("IOR:") == 0)
    {
      throw LogGeneralizerError("LogGeneralizer requires a gRPC endpoint and positive timeout");
    }

    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);
    // get_stat_info(clear=true) consumes data and must not be retried.
    args.SetInt(GRPC_ARG_ENABLE_RETRIES, 0);
    args.SetMaxReceiveMessageSize(-1);
    args.SetMaxSendMessageSize(-1);
    stub_ = lg_rpc::LogGeneralizer::NewStub(
      grpc::CreateCustomChannel(endpoint, grpc::InsecureChannelCredentials(), args));
  }

  void LogGeneralizerClient::stop_stat_upload(std::uint32_t client_id) const
  {
    call<lg_rpc::Empty>(
      *stub_, &lg_rpc::LogGeneralizer::Stub::stop_stat_upload, client_id, false, timeout_);
  }

  void LogGeneralizerClient::start_stat_upload(std::uint32_t client_id, bool clear) const
  {
    call<lg_rpc::Empty>(
      *stub_, &lg_rpc::LogGeneralizer::Stub::start_stat_upload, client_id, clear, timeout_);
  }

  StatInfo LogGeneralizerClient::get_stat_info(std::uint32_t client_id, bool clear) const
  {
    const auto response = call<lg_rpc::StatInfo>(
      *stub_, &lg_rpc::LogGeneralizer::Stub::get_stat_info, client_id, clear, timeout_);
    try
    {
      return unpack(response);
    }
    catch (const eh::Exception& error)
    {
      throw LogGeneralizerError(std::string("Invalid LogGeneralizer response: ") + error.what());
    }
  }
}
