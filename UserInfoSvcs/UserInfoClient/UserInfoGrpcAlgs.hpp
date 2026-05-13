#pragma once

#include <UserInfoManagerGrpc.grpc-client.hpp>
#include <UserInfoSvcs/UserInfoManager/UserInfoManager.hpp>

namespace AdServer::UserInfoSvcs::GrpcAlgs
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

  UserInfoMatcher::MatchResult* make_history_match_result(
    const adserver::user_info_svcs::user_info_manager::MatchResponse& response);

  void make_update_user_freq_caps_request(
    adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsRequest& request,
    const CORBACommons::UserIdInfo& user_id,
    const CORBACommons::TimestampInfo& time,
    const CORBACommons::RequestIdInfo& request_id,
    const FreqCapIdSeq& freq_caps,
    const FreqCapIdSeq& uc_freq_caps,
    const FreqCapIdSeq& virtual_freq_caps,
    const UserInfoManager::SeqOrderSeq& seq_orders,
    const CampaignIdSeq& campaign_ids,
    const CampaignIdSeq& uc_campaign_ids);

  void make_confirm_user_freq_caps_request(
    adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsRequest& request,
    const CORBACommons::UserIdInfo& user_id,
    const CORBACommons::TimestampInfo& time,
    const CORBACommons::RequestIdInfo& request_id,
    const CORBACommons::IdSeq& exclude_pubpixel_accounts);

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
