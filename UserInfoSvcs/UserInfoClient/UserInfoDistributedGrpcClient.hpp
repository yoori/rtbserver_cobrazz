#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/DistributedPartitionPool.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/BoostAsioContextRunActiveObject.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>

namespace AdServer::UserInfoSvcs
{
  class UserInfoDistributedGrpcClient:
    public Generics::CompositeActiveObject,
    public UserInfoManagerGrpcAsyncClient
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    using UserInfoControllerRefs = std::vector<std::string>;

    UserInfoDistributedGrpcClient(
      const UserInfoControllerRefs& user_info_controller_refs,
      AdServer::Grpc::BatchingOptions batching_options,
      std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor,
      Logging::Logger* logger,
      std::shared_ptr<AdServer::Commons::BoostAsioContextRunActiveObject>
        coalesce_runner = {});

    ~UserInfoDistributedGrpcClient() noexcept override;

    AdServer::Grpc::Stats stats() const noexcept override;

    std::string endpoint_for_user(const std::string& user_id) noexcept;

    void get_source(
      const adserver::user_info_svcs::user_info_manager::GetSourceRequest& request,
      GetSourceCallback callback) override;

    void get_master_stamp(
      const adserver::user_info_svcs::user_info_manager::GetMasterStampRequest& request,
      GetMasterStampCallback callback) override;

    void get_user_profile(
      const adserver::user_info_svcs::user_info_manager::GetUserProfileRequest& request,
      GetUserProfileCallback callback) override;

    void match(
      const adserver::user_info_svcs::user_info_manager::MatchRequest& request,
      MatchCallback callback) override;

    void update_user_freq_caps(
      const adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsRequest& request,
      UpdateUserFreqCapsCallback callback) override;

    void confirm_user_freq_caps(
      const adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsRequest& request,
      ConfirmUserFreqCapsCallback callback) override;

    void fraud_user(
      const adserver::user_info_svcs::user_info_manager::FraudUserRequest& request,
      FraudUserCallback callback) override;

    void remove_user_profile(
      const adserver::user_info_svcs::user_info_manager::RemoveUserProfileRequest& request,
      RemoveUserProfileCallback callback) override;

    void merge(
      const adserver::user_info_svcs::user_info_manager::MergeRequest& request,
      MergeCallback callback) override;

    void consider_publishers_optin(
      const adserver::user_info_svcs::user_info_manager::ConsiderPublishersOptinRequest& request,
      ConsiderPublishersOptinCallback callback) override;

    void uim_ready(
      const adserver::user_info_svcs::user_info_manager::UimReadyRequest& request,
      UimReadyCallback callback) override;

    void get_progress(
      const adserver::user_info_svcs::user_info_manager::GetProgressRequest& request,
      GetProgressCallback callback) override;

    void clear_expired(
      const adserver::user_info_svcs::user_info_manager::ClearExpiredRequest& request,
      ClearExpiredCallback callback) override;

  private:
    using Client = UserInfoManagerGrpcAsyncBatchingClient;
    using Pool = AdServer::Grpc::DistributedPartitionPool<Client>;
    using PoolPtr = std::shared_ptr<Pool>;

    std::optional<Pool::Ref> get_ref_(const std::string& user_id) noexcept;
    std::optional<Pool::Ref> get_any_ref_() noexcept;
    static std::optional<Pool::EndpointChunksList> resolve_partition_(
      const std::string& endpoint);

    static unsigned long chunk_index_(
      const std::string& user_id,
      unsigned long chunks_number);
    static unsigned long partition_index_(
      const std::string& user_id,
      unsigned long partitions_number);

  private:
    PoolPtr pool_;
  };
}
