#include "UserInfoControllerImpl.hpp"

#include <chrono>
#include <sstream>

#include <grpcpp/grpcpp.h>

#include <String/StringManip.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoManagerGrpc.grpc.pb.h>

namespace
{
  const Generics::Time REINIT_SOURCES_INTERVAL = Generics::Time(10);
  const std::chrono::seconds GET_SOURCE_TIMEOUT(60);

  namespace Aspect
  {
    const char USER_INFO_CONTROLLER[] = "UserInfoController";
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

namespace AdServer::UserInfoSvcs
{
  UserInfoControllerImpl::UserInfoControllerImpl(
    Generics::ActiveObjectCallback* callback,
    Logging::Logger* logger,
    const UserInfoControllerConfig& config)
    : callback_(ReferenceCounting::add_ref(callback)),
      logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_, 1)),
      config_(config)
  {
    static const char* FUN =
      "UserInfoControllerImpl::UserInfoControllerImpl()";

    try
    {
      add_child_object(task_runner_);
      add_child_object(scheduler_);
      fill_refs_();
      task_runner_->enqueue_task(
        Task_var(new InitUserInfoManagerSourceTask(this, 0)));
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't instantiate object: " << ex.what();
      throw Exception(ostr);
    }
  }

  UserInfoControllerImpl::~UserInfoControllerImpl() noexcept = default;

  void
  UserInfoControllerImpl::fill_session_description(
    SessionDescription& response) const
  {
    UserInfoConfig_var user_info_config;
    {
      SyncPolicy::ReadGuard lock(lock_);
      user_info_config = user_info_config_;
    }

    if (!user_info_config || !user_info_config->first_all_ready)
    {
      throw NotReady("UserInfoController isn't ready");
    }

    for (const auto& server : user_info_config->user_info_managers)
    {
      auto* description = response.add_user_info_managers();
      description->set_user_info_manager_endpoint(server.endpoint);
      for (const auto chunk_id : server.chunks)
      {
        description->add_chunk_ids(static_cast<std::uint32_t>(chunk_id));
      }
    }
  }

  void
  UserInfoControllerImpl::fill_refs_()
  {
    UserInfoConfig_var user_info_config = new UserInfoConfig();

    for (const auto& host : config_.UserInfoManagerHost())
    {
      UserInfoManagerRef server;
      server.endpoint = endpoint_from_grpc_ref_(host.UserInfoManagerGrpcRef());

      user_info_config->user_info_managers.emplace_back(std::move(server));
    }

    SyncPolicy::WriteGuard lock(lock_);
    user_info_config_.swap(user_info_config);
  }

  bool
  UserInfoControllerImpl::get_user_info_manager_sources_(
    UserInfoConfig* user_info_config)
  {
    std::ostringstream tracing;
    std::ostringstream errors;
    bool has_errors = false;
    bool all_ready = true;

    for (auto& server : user_info_config->user_info_managers)
    {
      try
      {
        auto channel = grpc::CreateChannel(
          server.endpoint,
          grpc::InsecureChannelCredentials());
        auto stub = adserver::user_info_svcs::user_info_manager::
          UserInfoManagerGrpc::NewStub(channel);

        grpc::ClientContext context;
        context.set_deadline(
          std::chrono::system_clock::now() + GET_SOURCE_TIMEOUT);
        adserver::user_info_svcs::user_info_manager::GetSourceRequest request;
        adserver::user_info_svcs::user_info_manager::GetSourceResponse response;
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

        if (!user_info_config->common_chunks_number)
        {
          user_info_config->common_chunks_number = response.chunks_number();
        }
        else if (user_info_config->common_chunks_number != response.chunks_number())
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
          tracing << "Ready UserInfoManager '" << server.endpoint << "'";
        }
      }
      catch (const eh::Exception& ex)
      {
        all_ready = false;
        has_errors = true;
        server.ready = false;
        server.chunks.clear();
        errors << " UserInfoManager '" << server.endpoint << "': " << ex.what();
      }
    }

    if (has_errors)
    {
      logger_->sstream(
        Logging::Logger::ERROR,
        Aspect::USER_INFO_CONTROLLER,
        "ADS-IMPL-72") <<
        "Errors of getting UserInfoManager gRPC sources:" << errors.str();
    }

    if (all_ready)
    {
      user_info_config->first_all_ready = true;
    }
    else if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(
        Logging::Logger::TRACE,
        Aspect::USER_INFO_CONTROLLER) <<
        "Not ready UserInfoManagers: " << tracing.str();
    }

    return all_ready;
  }

  void
  UserInfoControllerImpl::init_user_info_manager_state_() noexcept
  {
    try
    {
      UserInfoConfig_var user_info_config;
      {
        SyncPolicy::ReadGuard lock(lock_);
        user_info_config = new UserInfoConfig(*user_info_config_);
      }

      const bool all_ready = get_user_info_manager_sources_(user_info_config.in());
      if (all_ready)
      {
        check_source_consistency_(user_info_config.in());
        user_info_config->all_ready = true;
        user_info_config->first_all_ready = true;

        {
          SyncPolicy::WriteGuard lock(lock_);
          user_info_config_.swap(user_info_config);
        }

        task_runner_->enqueue_task(
          Task_var(new CheckUserInfoManagerStateTask(this, 0)));
      }
      else
      {
        scheduler_->schedule(
          Task_var(new InitUserInfoManagerSourceTask(this, task_runner_)),
          Generics::Time::get_time_of_day() + REINIT_SOURCES_INTERVAL);
      }
    }
    catch (const eh::Exception& ex)
    {
      logger_->sstream(
        Logging::Logger::ERROR,
        Aspect::USER_INFO_CONTROLLER,
        "ADS-IMPL-72") <<
        "Can't get UserInfoManager gRPC sources: " << ex.what();
    }
  }

  void
  UserInfoControllerImpl::check_user_info_manager_state_() noexcept
  {
    try
    {
      UserInfoConfig_var user_info_config;
      {
        SyncPolicy::ReadGuard lock(lock_);
        user_info_config = new UserInfoConfig(*user_info_config_);
      }

      const bool all_ready = get_user_info_manager_sources_(user_info_config.in());
      if (all_ready)
      {
        check_source_consistency_(user_info_config.in());
        user_info_config->all_ready = true;

        {
          SyncPolicy::WriteGuard lock(lock_);
          user_info_config_.swap(user_info_config);
        }

        scheduler_->schedule(
          Task_var(new CheckUserInfoManagerStateTask(this, task_runner_)),
          Generics::Time::get_time_of_day() + config_.status_check_period());
      }
      else
      {
        {
          SyncPolicy::WriteGuard lock(lock_);
          user_info_config_->all_ready = false;
        }

        scheduler_->schedule(
          Task_var(new InitUserInfoManagerSourceTask(this, task_runner_)),
          Generics::Time::get_time_of_day() + REINIT_SOURCES_INTERVAL);
      }
    }
    catch (const eh::Exception& ex)
    {
      logger_->sstream(
        Logging::Logger::ERROR,
        Aspect::USER_INFO_CONTROLLER,
        "ADS-IMPL-52") <<
        "check_user_info_manager_state_ failed: " << ex.what();
    }
  }

  void
  UserInfoControllerImpl::check_source_consistency_(
    UserInfoConfig* user_info_config) const
  {
    std::vector<long> chunk_refs(user_info_config->common_chunks_number, -1);
    std::ostringstream errors;
    bool has_errors = false;
    unsigned long server_index = 0;

    for (const auto& server : user_info_config->user_info_managers)
    {
      for (const auto chunk_id : server.chunks)
      {
        if (chunk_id >= user_info_config->common_chunks_number)
        {
          has_errors = true;
          errors << "Server '" << server.endpoint << "' has chunk " << chunk_id <<
            " >= common_chunks_number(" <<
            user_info_config->common_chunks_number << "). ";
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
        errors << "No UserInfoManager contains chunk #" << i << ". ";
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
