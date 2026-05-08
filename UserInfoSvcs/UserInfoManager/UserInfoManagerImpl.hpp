#pragma once

#include <memory>

#include <CORBACommons/ServantImpl.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManager_s.hpp>

#include "UserInfoManagerCore.hpp"

namespace AdServer::UserInfoSvcs
{
  class UserInfoManagerImpl:
    public virtual CORBACommons::ReferenceCounting::ServantImpl<
      POA_AdServer::UserInfoSvcs::UserInfoManager>
  {
  public:
    explicit UserInfoManagerImpl(UserInfoManagerCorePtr core) noexcept;

    void get_master_stamp(
      CORBACommons::TimestampInfo_out master_stamp) override;

    CORBA::Boolean uim_ready() noexcept override;

    char* get_progress() noexcept override;

    CORBA::Boolean merge(
      const AdServer::UserInfoSvcs::UserInfo& user_info,
      const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
      const AdServer::UserInfoSvcs::UserProfiles& merge_user_profile,
      CORBA::Boolean_out merge_success,
      CORBACommons::TimestampInfo_out last_request) override;

    CORBA::Boolean fraud_user(
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time) override;

    CORBA::Boolean match(
      const AdServer::UserInfoSvcs::UserInfo& user_info,
      const AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams& match_params,
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_out match_result)
      override;

    CORBA::Boolean get_user_profile(
      const CORBACommons::UserIdInfo& user_id,
      CORBA::Boolean temporary,
      const AdServer::UserInfoSvcs::ProfilesRequestInfo& profile_request,
      AdServer::UserInfoSvcs::UserProfiles_out user_profile) override;

    CORBA::Boolean remove_user_profile(
      const CORBACommons::UserIdInfo& user_info) override;

    void update_user_freq_caps(
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const FreqCapIdSeq& freq_caps,
      const FreqCapIdSeq& uc_freq_caps,
      const FreqCapIdSeq& virtual_freq_caps,
      const AdServer::UserInfoSvcs::UserInfoManager::SeqOrderSeq& seq_orders,
      const CampaignIdSeq& campaign_ids_seq,
      const CampaignIdSeq& uc_campaign_ids_seq) override;

    void confirm_user_freq_caps(
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const CORBACommons::IdSeq& exclude_pubpixel_accounts) override;

    void consider_publishers_optin(
      const CORBACommons::UserIdInfo& user_id_info,
      const CORBACommons::IdSeq& exclude_pubpixel_accounts,
      const CORBACommons::TimestampInfo& now) override;

    void clear_expired(
      CORBA::Boolean synch,
      const CORBACommons::TimestampInfo& cleanup_time,
      CORBA::Long portion) override;

  protected:
    ~UserInfoManagerImpl() noexcept override;

  private:
    UserInfoManagerCorePtr core_;
  };

  typedef ReferenceCounting::SmartPtr<UserInfoManagerImpl>
    UserInfoManagerImpl_var;
}
