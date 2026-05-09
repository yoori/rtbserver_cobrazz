#pragma once

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>

#include <list>
#include <memory>
#include <string>
#include <vector>

#include <UserInfoSvcs/UserInfoManager/UserInfoManager.hpp>
#include <UserInfoSvcs/UserInfoManagerController/UserInfoManagerController.hpp>
#include <UserInfoManagerGrpc.grpc-client.hpp>

namespace AdServer::UserInfoSvcs
{
  class UserInfoCorbaClient:
    public Generics::CompositeActiveObject,
    public UserInfoManagerGrpcAsyncClient
  {
  public:
    using ControllerRef = std::vector<std::string>;
    using ControllerRefList = std::list<ControllerRef>;

    UserInfoCorbaClient(
      Logging::Logger* logger,
      const ControllerRefList& controller_refs,
      const CORBACommons::CorbaClientAdapter* corba_client_adapter,
      const Generics::Time& pool_timeout = Generics::Time::ONE_SECOND);

    ~UserInfoCorbaClient() noexcept override;

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
    class Distributor;
    using Distributor_var = ReferenceCounting::SmartPtr<Distributor>;

    Distributor_var distributor_;
  };

  using UserInfoCorbaClientPtr = std::shared_ptr<UserInfoCorbaClient>;

  namespace GrpcAlgs
  {
    bool get_user_profile(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id,
      CORBA::Boolean temporary,
      const ProfilesRequestInfo& profile_request,
      UserProfiles_out user_profile);

    bool history_match(
      UserInfoManagerGrpcAsyncClient& client,
      const adserver::user_info_svcs::user_info_manager::MatchRequest& request,
      adserver::user_info_svcs::user_info_manager::MatchResponse& response);

    UserInfoMatcher::MatchResult* history_match(
      UserInfoManagerGrpcAsyncClient& client,
      const adserver::user_info_svcs::user_info_manager::MatchRequest& request);

    bool merge(
      UserInfoManagerGrpcAsyncClient& client,
      const UserInfo& user_info,
      const UserInfoMatcher::MatchParams& match_params,
      const UserProfiles& merge_user_profile,
      CORBA::Boolean_out merge_success,
      CORBACommons::TimestampInfo_out last_request);

    bool remove_user_profile(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id_info);

    void update_user_freq_caps(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const FreqCapIdSeq& freq_caps,
      const FreqCapIdSeq& uc_freq_caps,
      const FreqCapIdSeq& virtual_freq_caps,
      const UserInfoManager::SeqOrderSeq& seq_orders,
      const CampaignIdSeq& campaign_ids,
      const CampaignIdSeq& uc_campaign_ids);

    void confirm_user_freq_caps(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const CORBACommons::IdSeq& exclude_pubpixel_accounts);

    bool fraud_user(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time);
  }
}
