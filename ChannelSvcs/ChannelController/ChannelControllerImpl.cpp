#include "ChannelControllerImpl.hpp"

#include <chrono>
#include <algorithm>
#include <sstream>

#include <grpcpp/grpcpp.h>

#include <Generics/Hash.hpp>
#include <Commons/CorbaConfig.hpp>

#include <ChannelSvcs/ChannelServer/ChannelServerGrpc.grpc.pb.h>

namespace
{
  namespace Aspect
  {
    const char CHANNEL_CONTROLLER[] = "ChannelController";
  }

  std::string
  endpoint_from_grpc_ref_(
    const xsd::AdServer::Configuration::GrpcEndpointConfigType& endpoint)
  {
    const std::string host =
      endpoint.host().present() && *endpoint.host() != "*" ?
      *endpoint.host() :
      "127.0.0.1";

    return host + ":" + std::to_string(endpoint.port());
  }

  unsigned long
  add_to_check_sum_(unsigned long check_sum, const std::string& value)
  {
    Generics::CRC32Hash hash(check_sum, check_sum);
    hash.add(value.c_str(), value.size());
    return check_sum;
  }
}

namespace AdServer::ChannelSvcs
{
  ChannelControllerImpl::ChannelControllerImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const ChannelControllerConfig& config)
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      config_(config)
  {
    static const char* FUN =
      "ChannelControllerImpl::ChannelControllerImpl()";

    try
    {
      fill_refs_();
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't instantiate object: " << ex.what();
      throw Exception(ostr);
    }
  }

  ChannelControllerImpl::~ChannelControllerImpl() noexcept = default;

  void
  ChannelControllerImpl::activate_object_()
  {
    Generics::CompositeActiveObject::activate_object_();
    set_sources_();
  }

  void
  ChannelControllerImpl::fill_session_description(
    SessionDescription& response) const
  {
    if (channel_server_groups_.empty())
    {
      throw NotReady("ChannelController isn't ready");
    }

    for (const auto& group : channel_server_groups_)
    {
      auto* response_group = response.add_channel_server_groups();
      for (const auto& server : group)
      {
        auto* description = response_group->add_channel_servers();
        description->set_channel_server_endpoint(server.endpoint);
        for (const auto chunk_id : server.chunks)
        {
          description->add_chunk_ids(static_cast<std::uint32_t>(chunk_id));
        }
      }
    }

    response.set_chunks_number(config_.count_chunks());
  }

  void
  ChannelControllerImpl::fill_refs_()
  {
    if (!config_.count_chunks())
    {
      throw Exception("count chunks is zero");
    }

    channel_server_groups_.clear();
    channel_server_groups_.reserve(config_.ChannelServerGroup().size());

    for (const auto& config_group : config_.ChannelServerGroup())
    {
      ChannelServerGroup group;
      group.reserve(config_group.ChannelServerHost().size());
      for (const auto& config_host : config_group.ChannelServerHost())
      {
        ChannelServerRef server;
        server.endpoint = endpoint_from_grpc_ref_(
          config_host.ChannelServerGrpcRef());
        if (config_host.ChannelUpdateRef().present())
        {
          CORBACommons::CorbaObjectRef update_ref;
          Config::CorbaConfigReader::read_corba_ref(
            config_host.ChannelUpdateRef().get(),
            update_ref);
          server.update_ref = update_ref.object_ref;
        }
        group.emplace_back(std::move(server));
      }

      if (group.empty())
      {
        throw Exception("empty ChannelServerGroup");
      }

      auto server_it = group.begin();
      bool all_assign = false;
      for (unsigned long chunk_id = 0;
           chunk_id < config_.count_chunks();
           ++chunk_id)
      {
        server_it->chunks.push_back(chunk_id);
        if (++server_it == group.end())
        {
          all_assign = true;
          server_it = group.begin();
        }
      }

      if (!all_assign)
      {
        logger_->sstream(Logging::Logger::WARNING, Aspect::CHANNEL_CONTROLLER) <<
          "count of chunks less than servers, count servers = " <<
          group.size() << ", chunks = " << config_.count_chunks();
      }

      channel_server_groups_.emplace_back(std::move(group));
    }
  }

  void
  ChannelControllerImpl::set_sources_() const
  {
    static const char* FUN = "ChannelControllerImpl::set_sources_()";
    namespace pb = adserver::channel_svcs::channel_server;

    if (!config_.ChannelSource().RegularSource().present())
    {
      throw Exception("only regular channel source is supported");
    }
    const auto& regular_source = config_.ChannelSource().RegularSource().get();
    CORBACommons::CorbaObjectRefList campaign_refs;
    Config::CorbaConfigReader::read_multi_corba_ref(
      regular_source.CampaignServerCorbaRef(),
      campaign_refs);

    unsigned long check_sum = 0;
    const std::string pg_connection =
      regular_source.PGConnection().connection_string();
    check_sum = add_to_check_sum_(check_sum, pg_connection);
    for (const auto& campaign_ref : campaign_refs)
    {
      check_sum = add_to_check_sum_(check_sum, campaign_ref.object_ref);
    }

    grpc::ChannelArguments channel_args;
    channel_args.SetInt(GRPC_ARG_ENABLE_HTTP_PROXY, 0);

    const bool use_local_group = channel_server_groups_.size() > 1;
    const auto& local_group = channel_server_groups_.front();

    for (std::size_t group_index = 0;
         group_index < channel_server_groups_.size();
         ++group_index)
    {
      const auto& group = channel_server_groups_[group_index];
      for (const auto& server : group)
      {
        grpc::ClientContext context;
        context.set_deadline(
          std::chrono::system_clock::now() + std::chrono::seconds(10));

        auto channel = grpc::CreateCustomChannel(
          server.endpoint,
          grpc::InsecureChannelCredentials(),
          channel_args);
        auto stub = pb::ChannelServerGrpc::NewStub(channel);

        if (use_local_group && group_index > 0)
        {
          pb::SetProxySourcesRequest request;
          request.set_colo(config_.ColoSettings().colo());
          request.set_version(config_.ColoSettings().version());
          request.set_count_chunks(config_.count_chunks());
          request.set_check_sum(check_sum);

          for (const auto& campaign_ref : campaign_refs)
          {
            request.add_campaign_refs(campaign_ref.object_ref);
          }

          auto* local_pb_group = request.add_local_groups();
          for (const auto& local_server : local_group)
          {
            if (local_server.update_ref.empty())
            {
              Stream::Error ostr;
              ostr << FUN << ": ChannelServer " << local_server.endpoint <<
                " doesn't have ChannelUpdateRef for local source";
              throw Exception(ostr);
            }

            auto* local_pb_server = local_pb_group->add_servers();
            local_pb_server->set_channel_update_ref(local_server.update_ref);
            local_pb_server->set_count_chunks(config_.count_chunks());
            for (const auto chunk_id : local_server.chunks)
            {
              local_pb_server->add_sources(chunk_id);
            }
          }

          for (const auto chunk_id : server.chunks)
          {
            request.add_sources(chunk_id);
          }

          pb::SetProxySourcesResponse response;
          const grpc::Status status =
            stub->set_proxy_sources(&context, request, &response);
          if (!status.ok())
          {
            Stream::Error ostr;
            ostr << FUN << ": ChannelServer " << server.endpoint <<
              " rejected proxy sources: " << status.error_message();
            logger_->log(
              ostr.str(),
              Logging::Logger::ERROR,
              Aspect::CHANNEL_CONTROLLER,
              "ADS-IMPL-42");
            throw Exception(ostr);
          }
        }
        else
        {
          pb::SetSourcesRequest request;
          request.set_pg_connection(pg_connection);
          request.set_colo(config_.ColoSettings().colo());
          request.set_version(config_.ColoSettings().version());
          request.set_count_chunks(config_.count_chunks());
          request.set_check_sum(check_sum);

          for (const auto& campaign_ref : campaign_refs)
          {
            request.add_campaign_refs(campaign_ref.object_ref);
          }

          for (const auto chunk_id : server.chunks)
          {
            request.add_sources(chunk_id);
          }

          pb::SetSourcesResponse response;
          const grpc::Status status =
            stub->set_sources(&context, request, &response);
          if (!status.ok())
          {
            Stream::Error ostr;
            ostr << FUN << ": ChannelServer " << server.endpoint <<
              " rejected sources: " << status.error_message();
            logger_->log(
              ostr.str(),
              Logging::Logger::ERROR,
              Aspect::CHANNEL_CONTROLLER,
              "ADS-IMPL-42");
            throw Exception(ostr);
          }
        }

        logger_->sstream(
          Logging::Logger::TRACE,
          Aspect::CHANNEL_CONTROLLER) <<
          "set sources for ChannelServer " << server.endpoint <<
          ", chunks = " << server.chunks.size();
      }
    }
  }
}
