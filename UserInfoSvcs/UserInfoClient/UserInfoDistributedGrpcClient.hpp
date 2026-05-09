#pragma once

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

#include <Commons/Grpc/GrpcClient.hpp>
#include <Commons/Grpc/GrpcExecutor.hpp>
#include <Commons/Grpc/RefPool.hpp>
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
      Logging::Logger* logger);

    ~UserInfoDistributedGrpcClient() noexcept override;

    AdServer::Grpc::Stats stats() const noexcept override;

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
    using ClientPtr = std::shared_ptr<Client>;

    class ResolveRefsTask;

    struct ClientHolder;
    using ClientHolderPtr = std::shared_ptr<ClientHolder>;

    struct RefHolder;
    using Pool = AdServer::Grpc::RefPool<RefHolder>;
    using PoolPtr = std::shared_ptr<Pool>;
    using ControllerRefsState =
      std::vector<std::pair<std::string, std::vector<std::string>>>;

    void activate_object_() override;
    void deactivate_object_() override;

    std::optional<Pool::Ref> get_ref_(const std::string& user_id) const noexcept;
    std::optional<Pool::Ref> get_any_ref_() const noexcept;
    void resolve_refs_() noexcept;
    bool fill_refs_state_(ControllerRefsState& refs_state) noexcept;
    bool update_pools_if_changed_(ControllerRefsState refs_state) noexcept;
    ClientHolderPtr get_or_create_client_holder_(const std::string& endpoint);
    void wait_next_resolve_() noexcept;

    static unsigned long chunk_index_(
      const std::string& user_id,
      unsigned long chunks_number);

    static void merge_stats_(
      AdServer::Grpc::Stats& result,
      const AdServer::Grpc::Stats& source) noexcept;

  private:
    const UserInfoControllerRefs user_info_controller_refs_;
    const AdServer::Grpc::BatchingOptions batching_options_;
    std::shared_ptr<AdServer::Grpc::GrpcExecutor> grpc_executor_;
    const Generics::Time pool_timeout_;
    const Generics::Time resolve_period_;

    Generics::ActiveObjectCallback_var callback_;
    Generics::FixedTaskRunner_var task_runner_;
    Logging::Logger_var logger_;

    mutable std::shared_mutex pool_lock_;
    std::map<unsigned long, PoolPtr> chunk_pools_;
    PoolPtr any_pool_;
    unsigned long chunks_number_ = 0;
    ControllerRefsState refs_state_;
    std::vector<ClientHolderPtr> current_client_holders_;
    std::map<std::string, std::weak_ptr<ClientHolder>> client_holders_;

    std::mutex resolve_lock_;
    std::condition_variable resolve_cond_;
    std::atomic_bool deactivated_{false};
  };
}
