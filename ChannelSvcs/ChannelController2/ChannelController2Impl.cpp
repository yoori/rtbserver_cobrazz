#include "ChannelController2Impl.hpp"

#include <algorithm>
#include <sstream>

namespace
{
  namespace Aspect
  {
    const char CHANNEL_CONTROLLER[] = "ChannelController2";
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
}

namespace AdServer::ChannelSvcs
{
  ChannelController2Impl::ChannelController2Impl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const ChannelControllerConfig& config)
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      config_(config)
  {
    static const char* FUN =
      "ChannelController2Impl::ChannelController2Impl()";

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

  ChannelController2Impl::~ChannelController2Impl() noexcept = default;

  void
  ChannelController2Impl::fill_session_description(
    SessionDescription& response) const
  {
    if (channel_server_groups_.empty())
    {
      throw NotReady("ChannelController2 isn't ready");
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
  ChannelController2Impl::fill_refs_()
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
}
