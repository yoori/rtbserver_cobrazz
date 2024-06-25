#ifndef USERINFOMANAGERCONTROLLER_GRPCUSERINFOOPERATIONDISTRIBUTOR_HPP
#define USERINFOMANAGERCONTROLLER_GRPCUSERINFOOPERATIONDISTRIBUTOR_HPP

// STD
#include <shared_mutex>
#include <unordered_map>
#include <vector>

// PROTO
#include "UserInfoSvcs/UserInfoManager/proto/UserInfoManager_client.cobrazz.pb.hpp"

// UNIX_COMMONS
#include <CORBACommons/CorbaAdapters.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <UServerUtils/Grpc/Client/ConfigPoolCoro.hpp>
#include <UServerUtils/Grpc/Client/PoolClientFactory.hpp>
#include <UServerUtils/Grpc/Common/Scheduler.hpp>
#include <UServerUtils/Manager.hpp>

// THIS
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/ExternalUserIdUtils.hpp>
#include <Commons/UserInfoManip.hpp>
#include <GrpcAlgs.hpp>

namespace AdServer::UserInfoSvcs
{

extern const char* ASPECT_GRPC_USER_INFO_DISTRIBUTOR;

namespace Types
{

using FreqCaps = std::vector<std::uint32_t>;
using UcFreqCaps = std::vector<std::uint32_t>;
using VirtualFreqCaps = std::vector<std::uint32_t>;
using CampaignIds = std::vector<std::uint32_t>;
using UcCampaignIds = std::vector<std::uint32_t>;
using ExcludePubpixelAccounts = std::vector<std::uint32_t>;
using PersistentChannelIds = std::vector<std::uint32_t>;

struct ProfilesRequestInfo final
{
  ProfilesRequestInfo() = default;
  ~ProfilesRequestInfo() = default;

  bool base_profile = true;
  bool add_profile = true;
  bool history_profile = true;
  bool freq_cap_profile = true;
  bool pref_profile = true;
};

struct SeqOrderInfo final
{
  SeqOrderInfo() = default;

  SeqOrderInfo(
    const std::uint32_t ccg_id,
    const std::uint32_t set_id,
    const std::uint32_t imps)
    : ccg_id(ccg_id),
      set_id(set_id),
      imps(imps)
  {
  }

  ~SeqOrderInfo() = default;

  std::uint32_t ccg_id = 0;
  std::uint32_t set_id = 0;
  std::uint32_t imps = 0;
};
using SeqOrders = std::vector<SeqOrderInfo>;

struct UserInfo final
{
  UserInfo() = default;
  ~UserInfo() = default;

  std::string user_id;
  std::string huser_id;
  std::uint32_t time = 0;
  std::int32_t last_colo_id = 0;
  std::int32_t current_colo_id = 0;
  std::int32_t request_colo_id = 0;
  bool temporary = true;
};

struct ChannelTriggerMatch final
{
  ChannelTriggerMatch() = default;

  ChannelTriggerMatch(
    const std::uint32_t channel_id,
    const std::uint32_t channel_trigger_id)
    : channel_id(channel_id),
      channel_trigger_id(channel_trigger_id)
  {
  }

  ~ChannelTriggerMatch() = default;

  std::uint32_t channel_id = 0;
  std::uint32_t channel_trigger_id = 0;
};
using ChannelTriggerMatchArray = std::vector<ChannelTriggerMatch>;

struct GeoData final
{
  using CoordDecimal = AdServer::CampaignSvcs::CoordDecimal;
  using AccuracyDecimal = AdServer::CampaignSvcs::AccuracyDecimal;

  GeoData() = default;

  GeoData(
    const CoordDecimal& latitude,
    const CoordDecimal& longitude,
    const AccuracyDecimal& accuracy)
    : latitude(latitude),
      longitude(longitude),
      accuracy(accuracy)
  {
  }

  ~GeoData() = default;

  CoordDecimal latitude;
  CoordDecimal longitude;
  AccuracyDecimal accuracy;
};
using GeoDataArray = std::vector<GeoData>;

struct MatchParams final
{
  MatchParams() = default;
  ~MatchParams() = default;

  PersistentChannelIds persistent_channel_ids;
  ChannelTriggerMatchArray search_channel_ids;
  ChannelTriggerMatchArray page_channel_ids;
  ChannelTriggerMatchArray url_channel_ids;
  ChannelTriggerMatchArray url_keyword_channel_ids;
  Generics::Time publishers_optin_timeout = Generics::Time::ZERO;
  GeoDataArray geo_data_seq;
  std::string cohort;
  std::string cohort2;
  bool use_empty_profile = true;
  bool silent_match = true;
  bool no_match = true;
  bool no_result = true;
  bool ret_freq_caps = true;
  bool provide_channel_count = true;
  bool provide_persistent_channels = true;
  bool change_last_request = true;
  bool filter_contextual_triggers = true;
};

struct UserProfiles final
{
  UserProfiles() = default;
  ~UserProfiles() = default;

  std::string base_user_profile;
  std::string add_user_profile;
  std::string history_user_profile;
  std::string freq_cap;
  std::string pref_profile;
};

} // namespace Types

class GrpcUserInfoOperationDistributor final :
  public Generics::CompositeActiveObject,
  public ReferenceCounting::AtomicImpl
{
private:
  using PartitionNumber = unsigned long;
  using TryCount = PartitionNumber;

  class ClientContainer;
  class FactoryClientContainer;
  class Partition;
  struct PartitionHolder;
  class PartitionResolver;

  using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;
  using ClientContainerPtr = std::shared_ptr<ClientContainer>;
  using FactoryClientContainerPtr = std::unique_ptr<FactoryClientContainer>;
  using PartitionPtr = std::shared_ptr<Partition>;
  using PartitionHolderPtr = std::shared_ptr<PartitionHolder>;
  using PartitionHolderArray = std::vector<PartitionHolderPtr>;
  using CorbaClientAdapter = CORBACommons::CorbaClientAdapter;
  using CorbaClientAdapter_var = CORBACommons::CorbaClientAdapter_var;
  using TaskRunner_var = Generics::TaskRunner_var;

public:
  using ChunkId = unsigned long;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using ConfigPoolClient = UServerUtils::Grpc::Client::ConfigPoolCoro;
  using ControllerRef = CORBACommons::CorbaObjectRefList;
  using ControllerRefList = std::list<ControllerRef>;
  using TaskProcessor = userver::engine::TaskProcessor;
  using GetMasterStampRequestPtr = std::unique_ptr<Proto::GetMasterStampRequest>;
  using GetMasterStampResponsePtr = std::unique_ptr<Proto::GetMasterStampResponse>;
  using GetUserProfileRequestPtr = std::unique_ptr<Proto::GetUserProfileRequest>;
  using GetUserProfileResponsePtr = std::unique_ptr<Proto::GetUserProfileResponse>;
  using MatchRequestPtr = std::unique_ptr<Proto::MatchRequest>;
  using MatchResponsePtr = std::unique_ptr<Proto::MatchResponse>;
  using UpdateUserFreqCapsRequestPtr = std::unique_ptr<Proto::UpdateUserFreqCapsRequest>;
  using UpdateUserFreqCapsResponsePtr = std::unique_ptr<Proto::UpdateUserFreqCapsResponse>;
  using ConfirmUserFreqCapsRequestPtr = std::unique_ptr<Proto::ConfirmUserFreqCapsRequest>;
  using ConfirmUserFreqCapsResponsePtr = std::unique_ptr<Proto::ConfirmUserFreqCapsResponse>;
  using FraudUserRequestPtr = std::unique_ptr<Proto::FraudUserRequest>;
  using FraudUserResponsePtr = std::unique_ptr<Proto::FraudUserResponse>;
  using RemoveUserProfileRequestPtr = std::unique_ptr<Proto::RemoveUserProfileRequest>;
  using RemoveUserProfileResponsePtr = std::unique_ptr<Proto::RemoveUserProfileResponse>;
  using MergeRequestPtr = std::unique_ptr<Proto::MergeRequest>;
  using MergeResponsePtr = std::unique_ptr<Proto::MergeResponse>;
  using ConsiderPublishersOptinRequestPtr = std::unique_ptr<Proto::ConsiderPublishersOptinRequest>;
  using ConsiderPublishersOptinResponsePtr = std::unique_ptr<Proto::ConsiderPublishersOptinResponse>;
  using UimReadyRequestPtr = std::unique_ptr<Proto::UimReadyRequest>;
  using UimReadyResponsePtr = std::unique_ptr<Proto::UimReadyResponse>;
  using GetProgressRequestPtr = std::unique_ptr<Proto::GetProgressRequest>;
  using GetProgressResponsePtr = std::unique_ptr<Proto::GetProgressResponse>;

public:
  GrpcUserInfoOperationDistributor(
    Logger* logger,
    TaskProcessor& task_processor,
    const ControllerRefList& controller_refs,
    const CorbaClientAdapter* corba_client_adapter,
    const ConfigPoolClient& config_pool_client,
    const std::size_t grpc_client_timeout = 1500,
    const Generics::Time& pool_timeout = Generics::Time::ONE_SECOND);

  GetUserProfileResponsePtr get_user_profile(
    const String::SubString& user_id,
    const bool temporary,
    const Types::ProfilesRequestInfo& profiles_request_info) noexcept;

  UpdateUserFreqCapsResponsePtr update_user_freq_caps(
    const String::SubString& user_id,
    const Generics::Time& time,
    const String::SubString& request_id,
    const Types::FreqCaps& freq_caps,
    const Types::UcFreqCaps& uc_freq_caps,
    const Types::VirtualFreqCaps& virtual_freq_caps,
    const Types::SeqOrders& seq_orders,
    const Types::CampaignIds& campaign_ids,
    const Types::UcCampaignIds& uc_campaign_ids) noexcept;

  ConfirmUserFreqCapsResponsePtr confirm_user_freq_caps(
    const String::SubString& user_id,
    const Generics::Time& time,
    const String::SubString& request_id,
    const Types::ExcludePubpixelAccounts& exclude_pubpixel_accounts) noexcept;

  FraudUserResponsePtr fraud_user(
    const String::SubString& user_id,
    const Generics::Time& time) noexcept;

  RemoveUserProfileResponsePtr remove_user_profile(
    const String::SubString& user_id) noexcept;

  ConsiderPublishersOptinResponsePtr consider_publishers_optin(
    const String::SubString& user_id,
    const Types::ExcludePubpixelAccounts& exclude_pubpixel_accounts,
    const Generics::Time& now) noexcept;

  MatchResponsePtr match(
    const Types::UserInfo& user_info,
    const Types::MatchParams& match_params) noexcept;

  MergeResponsePtr merge(
    const Types::UserInfo& user_info,
    const Types::MatchParams& match_params,
    const Types::UserProfiles& user_profiles) noexcept;

protected:
  ~GrpcUserInfoOperationDistributor() override = default;

private:
  void resolve_partition(
    const PartitionNumber partition_number) noexcept;

  void try_to_reresolve_partition(
    const PartitionNumber partition_number) noexcept;

  PartitionNumber get_partition_number(
    const String::SubString& id) const noexcept;

  PartitionPtr get_partition(
    const PartitionNumber partition_number) noexcept;

  bool the_same_chunk(
    const String::SubString uid1,
    const String::SubString uid2);

  GetUserProfileRequestPtr create_get_user_profile_request(
    const String::SubString& user_id,
    const bool temporary,
    const Types::ProfilesRequestInfo& profiles_request_info);

  UpdateUserFreqCapsRequestPtr create_update_user_freq_caps_request(
    const String::SubString& user_id,
    const Generics::Time& time,
    const String::SubString& request_id,
    const Types::FreqCaps& freq_caps,
    const Types::UcFreqCaps& uc_freq_caps,
    const Types::VirtualFreqCaps& virtual_freq_caps,
    const Types::SeqOrders& seq_orders,
    const Types::CampaignIds& campaign_ids,
    const Types::UcCampaignIds& uc_campaign_ids);

  ConfirmUserFreqCapsRequestPtr create_confirm_user_freq_caps_request(
    const String::SubString& user_id,
    const Generics::Time& time,
    const String::SubString& request_id,
    const Types::ExcludePubpixelAccounts& exclude_pubpixel_accounts);

  FraudUserRequestPtr create_fraud_user_request(
    const String::SubString& user_id,
    const Generics::Time& time);

  RemoveUserProfileRequestPtr create_remove_user_profile_request(
    const String::SubString& user_id);

  ConsiderPublishersOptinRequestPtr create_consider_publishers_optin_request(
    const String::SubString& user_id,
    const Types::ExcludePubpixelAccounts& exclude_pubpixel_accounts,
    const Generics::Time& now);

  MatchRequestPtr create_match_request(
    const Types::UserInfo& user_info,
    const Types::MatchParams& match_params);

  MergeRequestPtr create_merge_request(
    const Types::UserInfo& user_info,
    const Types::MatchParams& match_params,
    const Types::UserProfiles& user_profiles);

  template<class Client, class Request, class Response, class ...Args>
  std::unique_ptr<Response> do_request(
    const String::SubString& id,
    Args&& ...args) noexcept
  {
    ChunkId chunk_id = 0;
    for (std::size_t i = 0; i < try_count_; ++i)
    {
      const PartitionNumber partition_number =
        (get_partition_number(id) + i) % partition_holders_.size();

      try
      {
        PartitionPtr partition = get_partition(partition_number);
        if (!partition)
        {
          try_to_reresolve_partition(partition_number);
          continue;
        }

        chunk_id = partition->chunk_id(id);
        auto client_container =
          partition->get_client_container(chunk_id);
        if (!client_container)
        {
          try_to_reresolve_partition(partition_number);
          continue;
        }

        if (client_container->is_bad(pool_timeout_))
        {
          continue;
        }

        std::unique_ptr<Request> request;
        if constexpr (std::is_same_v<Request, Proto::GetUserProfileRequest>)
        {
          request = create_get_user_profile_request(std::forward<Args>(args)...);
        }
        else if constexpr (std::is_same_v<Request, Proto::UpdateUserFreqCapsRequest>)
        {
          request = create_update_user_freq_caps_request(std::forward<Args>(args)...);
        }
        else if constexpr (std::is_same_v<Request, Proto::ConfirmUserFreqCapsRequest>)
        {
          request = create_confirm_user_freq_caps_request(std::forward<Args>(args)...);
        }
        else if constexpr (std::is_same_v<Request, Proto::FraudUserRequest>)
        {
          request = create_fraud_user_request(std::forward<Args>(args)...);
        }
        else if constexpr (std::is_same_v<Request, Proto::RemoveUserProfileRequest>)
        {
          request = create_remove_user_profile_request(std::forward<Args>(args)...);
        }
        else if constexpr (std::is_same_v<Request, Proto::ConsiderPublishersOptinRequest>)
        {
          request = create_consider_publishers_optin_request(std::forward<Args>(args)...);
        }
        else if constexpr (std::is_same_v<Request, Proto::MatchRequest>)
        {
          request = create_match_request(std::forward<Args>(args)...);
        }
        else if constexpr (std::is_same_v<Request, Proto::MergeRequest>)
        {
          request = create_merge_request(std::forward<Args>(args)...);
        }
        else
        {
          static_assert(GrpcAlgs::AlwaysFalseV<Request>);
        }

        auto response = client_container->template do_request<Client, Request, Response>(
          std::move(request),
          grpc_client_timeout_);
        if (!response)
        {
          client_container->set_bad();
          try_to_reresolve_partition(partition_number);
          continue;
        }

        const auto data_case = response->data_case();
        if (data_case == Response::DataCase::kInfo)
        {
          return response;
        }
        else if (data_case == Response::DataCase::kError)
        {
          const auto& error = response->error();
          const auto error_type = error.type();
          switch (error_type)
          {
            case Proto::Error_Type::Error_Type_NotReady:
            {
              client_container->set_bad();
              break;
            }
            case Proto::Error_Type::Error_Type_ChunkNotFound:
            {
              client_container->set_bad();
              try_to_reresolve_partition(partition_number);
              break;
            }
            case Proto::Error_Type::Error_Type_Implementation:
            {
              client_container->set_bad();
              break;
            }
            default:
            {
              Stream::Error stream;
              stream << FNS
                     << ": "
                     << "Unknown error type";
              logger_->error(
                stream.str(),
                ASPECT_GRPC_USER_INFO_DISTRIBUTOR);
              client_container->set_bad();
              try_to_reresolve_partition(partition_number);
              break;
            }
          }
        }
      }
      catch (const eh::Exception& exc)
      {
        try
        {
          Stream::Error stream;
          stream << FNS
                 << ": "
                 << exc.what();
          logger_->error(
            stream.str(),
            ASPECT_GRPC_USER_INFO_DISTRIBUTOR);
        }
        catch (...)
        {
        }
      }
    }

    try
    {
      Stream::Error stream;
      stream << FNS
             << ": max tries reached for id="
             << id
             << " and chunk="
             << chunk_id;
      logger_->error(
        stream.str(),
        ASPECT_GRPC_USER_INFO_DISTRIBUTOR);
    }
    catch (...)
    {
    }

    return {};
  }

private:
  const Logger_var logger_;

  Generics::ActiveObjectCallback_var callback_;

  const TryCount try_count_;

  const ConfigPoolClient config_pool_client_;

  const std::size_t grpc_client_timeout_;

  const Generics::Time pool_timeout_;

  const ControllerRefList controller_refs_;

  const SchedulerPtr scheduler_;

  FactoryClientContainerPtr factory_client_container_;

  CorbaClientAdapter_var corba_client_adapter_;

  TaskRunner_var task_runner_;

  PartitionHolderArray partition_holders_;
};

class GrpcUserInfoOperationDistributor::PartitionResolver final :
  public Generics::Task,
  public ReferenceCounting::AtomicImpl
{
public:
  explicit PartitionResolver(
    GrpcUserInfoOperationDistributor& distributor,
    const PartitionNumber partition_number) noexcept
    : distributor_(distributor),
      partition_number_(partition_number)
  {
  }

  void execute() noexcept override
  {
    distributor_.resolve_partition(partition_number_);
  }

protected:
  ~PartitionResolver() noexcept override = default;

private:
  GrpcUserInfoOperationDistributor& distributor_;

  const PartitionNumber partition_number_;
};

class GrpcUserInfoOperationDistributor::ClientContainer final
  : private Generics::Uncopyable
{
public:
  using GetMasterStampClient =
    Proto::UserInfoManagerService_get_master_stamp_ClientPool;
  using GetMasterStampClientPtr =
    Proto::UserInfoManagerService_get_master_stamp_ClientPoolPtr;
  using GetUserProfileClient =
    Proto::UserInfoManagerService_get_user_profile_ClientPool;
  using GetUserProfileClientPtr =
    Proto::UserInfoManagerService_get_user_profile_ClientPoolPtr;
  using MatchClient =
    Proto::UserInfoManagerService_match_ClientPool;
  using MatchClientPtr =
    Proto::UserInfoManagerService_match_ClientPoolPtr;
  using UpdateUserFreqCapsClient =
    Proto::UserInfoManagerService_update_user_freq_caps_ClientPool;
  using UpdateUserFreqCapsClientPtr =
    Proto::UserInfoManagerService_update_user_freq_caps_ClientPoolPtr;
  using ConfirmUserFreqCapsClient =
    Proto::UserInfoManagerService_confirm_user_freq_caps_ClientPool;
  using ConfirmUserFreqCapsClientPtr =
    Proto::UserInfoManagerService_confirm_user_freq_caps_ClientPoolPtr;
  using FraudUserClient =
    Proto::UserInfoManagerService_fraud_user_ClientPool;
  using FraudUserClientPtr =
    Proto::UserInfoManagerService_fraud_user_ClientPoolPtr;
  using RemoveUserProfileClient =
    Proto::UserInfoManagerService_remove_user_profile_ClientPool;
  using RemoveUserProfileClientPtr =
    Proto::UserInfoManagerService_remove_user_profile_ClientPoolPtr;
  using MergeClient =
    Proto::UserInfoManagerService_merge_ClientPool;
  using MergeClientPtr =
    Proto::UserInfoManagerService_merge_ClientPoolPtr;
  using ConsiderPublishersOptinClient =
    Proto::UserInfoManagerService_consider_publishers_optin_ClientPool;
  using ConsiderPublishersOptinClientPtr =
    Proto::UserInfoManagerService_consider_publishers_optin_ClientPoolPtr;
  using UimReadyClient =
    Proto::UserInfoManagerService_uim_ready_ClientPool;
  using UimReadyClientPtr =
    Proto::UserInfoManagerService_uim_ready_ClientPoolPtr;
  using GetProgressClient =
    Proto::UserInfoManagerService_get_progress_ClientPool;
  using GetProgressClientPtr =
    Proto::UserInfoManagerService_get_progress_ClientPoolPtr;
  using ClearExpiredClient =
    Proto::UserInfoManagerService_clear_expired_ClientPool;
  using ClearExpiredClientPtr =
    Proto::UserInfoManagerService_clear_expired_ClientPoolPtr;

  struct Clients final
  {
    Clients() = default;
    ~Clients() = default;

    GetMasterStampClientPtr get_master_stamp_client;
    GetUserProfileClientPtr get_user_profile_client;
    MatchClientPtr match_client;
    UpdateUserFreqCapsClientPtr update_user_freq_caps_client;
    ConfirmUserFreqCapsClientPtr confirm_user_freq_caps_client;
    FraudUserClientPtr fraud_user_client;
    RemoveUserProfileClientPtr remove_user_profile_client;
    MergeClientPtr merge_client;
    ConsiderPublishersOptinClientPtr consider_publishers_optin_client;
    UimReadyClientPtr uim_ready_client;
    GetProgressClientPtr get_progress_client;
    ClearExpiredClientPtr clear_expired_client;
  };
  using ClientsPtr = std::shared_ptr<Clients>;

private:
  using Status = UServerUtils::Grpc::Client::Status;
  using Mutex = std::shared_mutex;

public:
  explicit ClientContainer(
    const ClientsPtr& clients)
    : clients_(clients)
  {
  }

  bool is_bad(const Generics::Time& timeout) const
  {
    {
      std::shared_lock lock(mutex_);
      if (!marked_as_bad_)
        return false;
    }

    Generics::Time now = Generics::Time::get_time_of_day();

    std::unique_lock lock(mutex_);
    if(marked_as_bad_ && now >= marked_as_bad_time_ + timeout)
    {
      marked_as_bad_ = false;
    }

    return marked_as_bad_;
  }

  void set_bad()
  {
    Generics::Time now = Generics::Time::get_time_of_day();

    std::unique_lock lock(mutex_);
    marked_as_bad_ = true;
    marked_as_bad_time_ = now;
  }

  template<class Client, class Request, class Response>
  std::unique_ptr<Response>
  do_request(
    std::unique_ptr<Request>&& request,
    const std::size_t timeout) const noexcept
  {
    using ClientPtr = std::shared_ptr<Client>;

    ClientPtr client;
    if constexpr (std::is_same_v<Client, GetMasterStampClient>)
    {
      client = clients_->get_master_stamp_client;
    }
    else if constexpr (std::is_same_v<Client, GetUserProfileClient>)
    {
      client = clients_->get_user_profile_client;
    }
    else if constexpr (std::is_same_v<Client, MatchClient>)
    {
      client = clients_->match_client;
    }
    else if constexpr (std::is_same_v<Client, UpdateUserFreqCapsClient>)
    {
      client = clients_->update_user_freq_caps_client;
    }
    else if constexpr (std::is_same_v<Client, ConfirmUserFreqCapsClient>)
    {
      client = clients_->confirm_user_freq_caps_client;
    }
    else if constexpr (std::is_same_v<Client, FraudUserClient>)
    {
      client = clients_->fraud_user_client;
    }
    else if constexpr (std::is_same_v<Client, RemoveUserProfileClient>)
    {
      client = clients_->remove_user_profile_client;
    }
    else if constexpr (std::is_same_v<Client, MergeClient>)
    {
      client = clients_->merge_client;
    }
    else if constexpr (std::is_same_v<Client, ConsiderPublishersOptinClient>)
    {
      client = clients_->consider_publishers_optin_client;
    }
    else if constexpr (std::is_same_v<Client, UimReadyClient>)
    {
      client = clients_->uim_ready_client;
    }
    else if constexpr (std::is_same_v<Client, GetProgressClient>)
    {
      client = clients_->get_progress_client;
    }
    else if constexpr (std::is_same_v<Client, ClearExpiredClient>)
    {
      client = clients_->clear_expired_client;
    }
    else
    {
      static_assert(GrpcAlgs::AlwaysFalseV<Client>);
    }

    auto result = client->write(std::move(request), timeout);
    if (result.status == Status::Ok)
    {
      return std::move(result.response);
    }
    else
    {
      return {};
    }
  }

private:
  const ClientsPtr clients_;

  mutable Mutex mutex_;

  mutable bool marked_as_bad_ = false;

  Generics::Time marked_as_bad_time_;
};

class GrpcUserInfoOperationDistributor::FactoryClientContainer final
  : private Generics::Uncopyable
{
public:
  using GetMasterStampClientPtr = typename ClientContainer::GetMasterStampClientPtr;
  using GetUserProfileClientPtr = typename ClientContainer::GetUserProfileClientPtr;
  using MatchClientPtr = typename ClientContainer::GetUserProfileClientPtr;
  using UpdateUserFreqCapsClientPtr = typename ClientContainer::GetUserProfileClientPtr;
  using ConfirmUserFreqCapsClientPtr = typename ClientContainer::GetUserProfileClientPtr;
  using FraudUserClientPtr = typename ClientContainer::GetUserProfileClientPtr;
  using RemoveUserProfileClientPtr = typename ClientContainer::GetUserProfileClientPtr;
  using MergeClientPtr = typename ClientContainer::GetUserProfileClientPtr;
  using ConsiderPublishersOptinClientPtr = typename ClientContainer::GetUserProfileClientPtr;
  using UimReadyClientPtr = typename ClientContainer::GetUserProfileClientPtr;
  using GetProgressClientPtr = typename ClientContainer::GetUserProfileClientPtr;
  using ClearExpiredClientPtr = typename ClientContainer::GetUserProfileClientPtr;
  using GrpcPoolClientFactory = UServerUtils::Grpc::Client::PoolClientFactory;
  using TaskProcessor = userver::engine::TaskProcessor;

private:
  using Mutex = std::shared_mutex;
  using Host = std::string;
  using Port = std::size_t;
  using Key = std::pair<Host, Port>;
  using Value = ClientContainer::ClientsPtr;
  using Cache = std::unordered_map<Key, Value, boost::hash<Key>>;

public:
  FactoryClientContainer(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const ConfigPoolClient& config,
    TaskProcessor& task_processor)
    : logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(scheduler),
      config_(config),
      task_processor_(task_processor)
  {
  }

  ClientContainerPtr create(
    const Host& host,
    const Port port)
  {
    Key key(host, port);
    {
      std::shared_lock lock(mutex_);
      auto it = cache_.find(key);
      if (it != std::end(cache_))
      {
        ClientContainer::ClientsPtr clients = it->second;
        lock.unlock();
        return std::make_shared<ClientContainer>(clients);
      }
    }

    auto clients = std::make_shared<ClientContainer::Clients>();
    auto config = config_;

    std::unique_lock lock(mutex_);
    auto it = cache_.find(key);
    if (it == std::end(cache_))
    {
      std::stringstream endpoint_stream;
      endpoint_stream << host
                      << ":"
                      << port;
      std::string endpoint = endpoint_stream.str();
      config.endpoint = endpoint;

      GrpcPoolClientFactory factory(
        logger_.in(),
        scheduler_,
        config);

      clients->clear_expired_client =
        factory.create<Proto::UserInfoManagerService_clear_expired_ClientPool>(
          task_processor_);
      clients->get_master_stamp_client =
        factory.create<Proto::UserInfoManagerService_get_master_stamp_ClientPool>(
          task_processor_);
      clients->get_user_profile_client =
        factory.create<Proto::UserInfoManagerService_get_user_profile_ClientPool>(
          task_processor_);
      clients->match_client =
        factory.create<Proto::UserInfoManagerService_match_ClientPool>(
          task_processor_);
      clients->update_user_freq_caps_client =
        factory.create<Proto::UserInfoManagerService_update_user_freq_caps_ClientPool>(
          task_processor_);
      clients->confirm_user_freq_caps_client =
        factory.create<Proto::UserInfoManagerService_confirm_user_freq_caps_ClientPool>(
          task_processor_);
      clients->fraud_user_client =
        factory.create<Proto::UserInfoManagerService_fraud_user_ClientPool>(
          task_processor_);
      clients->remove_user_profile_client =
        factory.create<Proto::UserInfoManagerService_remove_user_profile_ClientPool>(
          task_processor_);
      clients->merge_client =
        factory.create<Proto::UserInfoManagerService_merge_ClientPool>(
          task_processor_);
      clients->consider_publishers_optin_client =
        factory.create<Proto::UserInfoManagerService_consider_publishers_optin_ClientPool>(
          task_processor_);
      clients->uim_ready_client =
        factory.create<Proto::UserInfoManagerService_uim_ready_ClientPool>(
          task_processor_);
      clients->get_progress_client =
        factory.create<Proto::UserInfoManagerService_get_progress_ClientPool>(
          task_processor_);

      it = cache_.try_emplace(
        key,
        std::move(clients)).first;
    }

    clients = it->second;
    lock.unlock();

    return std::make_shared<ClientContainer>(clients);
  }

private:
  const Logger_var logger_;

  const SchedulerPtr scheduler_;

  const ConfigPoolClient config_;

  TaskProcessor& task_processor_;

  Mutex mutex_;

  Cache cache_;
};

class GrpcUserInfoOperationDistributor::Partition final
  : private Generics::Uncopyable
{
public:
  using ChunkToClientContainer = std::map<ChunkId, ClientContainerPtr>;

public:
  explicit Partition(
    ChunkToClientContainer&& chunk_to_client_container,
    const ChunkId max_chunk_number)
    : chunk_to_client_container_(std::move(chunk_to_client_container)),
      max_chunk_number_(max_chunk_number)
  {
  }

  ~Partition() = default;

  ChunkId chunk_id(
    const String::SubString& id) const noexcept
  {
    return AdServer::Commons::external_id_distribution_hash(id) % max_chunk_number_;
  }

  ClientContainerPtr get_client_container(
    const ChunkId chunk_id) noexcept
  {
    using Result = std::pair<bool, ClientContainerPtr>;

    const auto it = chunk_to_client_container_.find(chunk_id);
    if (it == std::end(chunk_to_client_container_))
    {
      return {};
    }
    else
    {
      return it->second;
    }
  }

  ChunkId max_chunk_number() const noexcept
  {
    return max_chunk_number_;
  }

  bool empty() const noexcept
  {
    return chunk_to_client_container_.empty();
  }

private:
  const ChunkToClientContainer chunk_to_client_container_;

  const ChunkId max_chunk_number_ = 0;
};

struct GrpcUserInfoOperationDistributor::PartitionHolder final
  : private Generics::Uncopyable
{
public:
  using Mutex = std::shared_mutex;

public:
  explicit PartitionHolder() noexcept = default;
  ~PartitionHolder() noexcept = default;

  PartitionPtr partition;
  Generics::Time last_try_to_resolve;
  bool resolve_in_progress = false;
  mutable Mutex mutex;
};

using GrpcUserInfoOperationDistributor_var =
  ReferenceCounting::SmartPtr<GrpcUserInfoOperationDistributor>;

} // namespace AdServer::UserInfoSvcs

#endif // USERINFOMANAGERCONTROLLER_GRPCUSERINFOOPERATIONDISTRIBUTOR_HPP