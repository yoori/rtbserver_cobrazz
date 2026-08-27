#include "UserBindControllerImpl.hpp"

#include <chrono>
#include <sstream>

#include <grpcpp/grpcpp.h>

#include <Commons/Grpc/GrpcClient.hpp>
#include <String/StringManip.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindServerGrpc.grpc.pb.h>

namespace
{
  const Generics::Time REINIT_SOURCES_INTERVAL = Generics::Time(10);
  const std::chrono::seconds GET_SOURCE_TIMEOUT(5);

  namespace Aspect
  {
    const char USER_BIND_CONTROLLER[] = "UserBindController";
  }

  std::string
  endpoint_from_grpc_ref_(const xsd::AdServer::Configuration::GrpcEndpointConfigType& endpoint)
  {
    const std::string host =
      endpoint.host().present() && *endpoint.host() != "*" ? *endpoint.host() : "127.0.0.1";

    return host + ":" + std::to_string(endpoint.port());
  }

}

namespace AdServer::UserInfoSvcs
{
  UserBindControllerImpl::UserBindControllerImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const UserBindControllerConfig& config)
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_, 1)),
      config_(config)
  {
    static const char* FUN = "UserBindControllerImpl::UserBindControllerImpl()";

    try
    {
      add_child_object(task_runner_);
      add_child_object(scheduler_);
      fill_refs_();
      task_runner_->enqueue_task(Task_var(new InitUserBindSourceTask(this, 0)));
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't instantiate object: " << ex.what();
      throw Exception(ostr);
    }
  }

  UserBindControllerImpl::~UserBindControllerImpl() noexcept = default;

  void
  UserBindControllerImpl::fill_session_description(SessionDescription& response) const
  {
    UserBindConfig_var user_bind_config;
    {
      SyncPolicy::ReadGuard lock(lock_);
      user_bind_config = user_bind_config_;
    }

    if (!user_bind_config || !user_bind_config->first_all_ready)
    {
      throw NotReady("UserBindController isn't ready");
    }

    for (const auto& server : user_bind_config->user_bind_servers)
    {
      auto* description = response.add_user_bind_servers();
      description->set_user_bind_server_endpoint(server.endpoint);
      for (const auto chunk_id : server.chunks)
      {
        description->add_chunk_ids(static_cast<std::uint32_t>(chunk_id));
      }
    }
  }

  void
  UserBindControllerImpl::fill_refs_()
  {
    UserBindConfig_var user_bind_config = new UserBindConfig();

    for (const auto& host : config_.UserBindServerHost())
    {
      UserBindServerRef server;
      server.endpoint = endpoint_from_grpc_ref_(host.UserBindServerGrpcRef());

      user_bind_config->user_bind_servers.emplace_back(std::move(server));
    }

    SyncPolicy::WriteGuard lock(lock_);
    user_bind_config_.swap(user_bind_config);
  }

  bool
  UserBindControllerImpl::get_user_bind_server_sources_(UserBindConfig* user_bind_config)
  {
    std::ostringstream tracing;
    std::ostringstream errors;
    bool has_errors = false;
    bool all_ready = true;

    for (auto& server : user_bind_config->user_bind_servers)
    {
      try
      {
        auto channel = AdServer::Grpc::create_channel(
          server.endpoint,
          grpc::InsecureChannelCredentials());
        auto stub = adserver::user_info_svcs::user_bind::UserBindServerGrpc::NewStub(channel);

        grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + GET_SOURCE_TIMEOUT);
        adserver::user_info_svcs::user_bind::GetSourceRequest request;
        adserver::user_info_svcs::user_bind::GetSourceResponse response;
        const auto status = stub->get_source(&context, request, &response);

        if (!status.ok())
        {
          Stream::Error ostr;
          ostr << "get_source failed: " << status.error_message();
          throw Exception(ostr);
        }

        if (!response.chunks_number())
        {
          throw Exception("get_source returned zero chunks_number");
        }

        if (!user_bind_config->common_chunks_number)
        {
          user_bind_config->common_chunks_number = response.chunks_number();
        }
        else if (user_bind_config->common_chunks_number != response.chunks_number())
        {
          Stream::Error ostr;
          ostr << "different chunks number on servers";
          throw Exception(ostr);
        }

        server.chunks.clear();
        for (const auto chunk : response.chunks())
        {
          server.chunks.emplace(chunk);
        }
        server.ready = true;

        if (logger_->log_level() >= Logging::Logger::TRACE)
        {
          tracing << "Ready UserBindServer '" << server.endpoint << "'";
        }
      }
      catch (const eh::Exception& ex)
      {
        all_ready = false;
        has_errors = true;
        server.ready = false;
        server.chunks.clear();
        errors << " UserBindServer '" << server.endpoint << "': " << ex.what();
      }
    }

    if (has_errors)
    {
      logger_->sstream(Logging::Logger::ERROR, Aspect::USER_BIND_CONTROLLER, "ADS-IMPL-72") <<
        "Errors of getting UserBindServer gRPC sources:" << errors.str();
    }

    if (all_ready)
    {
      user_bind_config->first_all_ready = true;
    }
    else if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE, Aspect::USER_BIND_CONTROLLER) <<
        "Not ready UserBindServers: " << tracing.str();
    }

    return all_ready;
  }

  void
  UserBindControllerImpl::init_user_bind_state_() noexcept
  {
    try
    {
      UserBindConfig_var user_bind_config;
      {
        SyncPolicy::ReadGuard lock(lock_);
        user_bind_config = new UserBindConfig(*user_bind_config_);
      }

      const bool all_ready = get_user_bind_server_sources_(user_bind_config.in());
      if (all_ready)
      {
        check_source_consistency_(user_bind_config.in());
        user_bind_config->all_ready = true;
        user_bind_config->first_all_ready = true;

        {
          SyncPolicy::WriteGuard lock(lock_);
          user_bind_config_.swap(user_bind_config);
        }

        task_runner_->enqueue_task(Task_var(new CheckUserBindServerStateTask(this, 0)));
      }
      else
      {
        scheduler_->schedule(
          Task_var(new InitUserBindSourceTask(this, task_runner_)),
          Generics::Time::get_time_of_day() + REINIT_SOURCES_INTERVAL);
      }
    }
    catch (const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::ERROR, Aspect::USER_BIND_CONTROLLER, "ADS-IMPL-72") <<
        "Can't get UserBindServer gRPC sources: " << ex.what();
    }
  }

  void
  UserBindControllerImpl::check_user_bind_state_() noexcept
  {
    try
    {
      UserBindConfig_var user_bind_config;
      {
        SyncPolicy::ReadGuard lock(lock_);
        user_bind_config = new UserBindConfig(*user_bind_config_);
      }

      const bool all_ready = get_user_bind_server_sources_(user_bind_config.in());
      if (all_ready)
      {
        check_source_consistency_(user_bind_config.in());
        user_bind_config->all_ready = true;

        {
          SyncPolicy::WriteGuard lock(lock_);
          user_bind_config_.swap(user_bind_config);
        }

        scheduler_->schedule(
          Task_var(new CheckUserBindServerStateTask(this, task_runner_)),
          Generics::Time::get_time_of_day() + config_.status_check_period());
      }
      else
      {
        {
          SyncPolicy::WriteGuard lock(lock_);
          user_bind_config_->all_ready = false;
        }

        scheduler_->schedule(
          Task_var(new InitUserBindSourceTask(this, task_runner_)),
          Generics::Time::get_time_of_day() + REINIT_SOURCES_INTERVAL);
      }
    }
    catch (const eh::Exception& ex)
    {
      logger_->sstream(Logging::Logger::ERROR, Aspect::USER_BIND_CONTROLLER, "ADS-IMPL-52") <<
        "check_user_bind_state_ failed: " << ex.what();
    }
  }

  void
  UserBindControllerImpl::check_source_consistency_(UserBindConfig* user_bind_config) const
  {
    std::vector<long> chunk_refs(user_bind_config->common_chunks_number, -1);
    std::ostringstream errors;
    bool has_errors = false;
    unsigned long server_index = 0;

    for (const auto& server : user_bind_config->user_bind_servers)
    {
      for (const auto chunk_id : server.chunks)
      {
        if (chunk_id >= user_bind_config->common_chunks_number)
        {
          has_errors = true;
          errors << "Server '" << server.endpoint << "' has chunk " << chunk_id <<
            " >= common_chunks_number(" << user_bind_config->common_chunks_number << "). ";
        }
        else if (chunk_refs[chunk_id] != -1)
        {
          has_errors = true;
          errors << "Chunk #" << chunk_id << " defined twice. ";
        }
        else
        {
          chunk_refs[chunk_id] = server_index;
        }
      }
      ++server_index;
    }

    for (std::size_t i = 0; i < chunk_refs.size(); ++i)
    {
      if (chunk_refs[i] == -1)
      {
        has_errors = true;
        errors << "No UserBindServer contains chunk #" << i << ". ";
      }
    }

    if (has_errors)
    {
      Stream::Error ostr;
      ostr << "Data sources aren't consistent: " << errors.str();
      throw Exception(ostr);
    }
  }
}
