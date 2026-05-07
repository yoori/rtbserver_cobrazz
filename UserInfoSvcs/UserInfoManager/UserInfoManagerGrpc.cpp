#include "UserInfoManagerGrpc.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <string>
#include <utility>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include <UserInfoSvcs/UserInfoManager/UserInfoManagerGrpc.grpc.pb.h>

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    constexpr const char user_info_manager_grpc_aspect[] =
      "UserInfoManagerGrpc";

    template<typename CorbaSeq>
    void bytes_to_oct_seq_(const std::string& src, CorbaSeq& dst)
    {
      dst.length(src.size());
      std::copy(src.begin(), src.end(), dst.get_buffer());
    }

    template<typename CorbaSeq>
    std::string oct_seq_to_bytes_(const CorbaSeq& src)
    {
      return std::string(
        reinterpret_cast<const char*>(src.get_buffer()),
        reinterpret_cast<const char*>(src.get_buffer() + src.length()));
    }

    template<typename CorbaSeq, typename Repeated>
    void repeated_to_id_seq_(const Repeated& src, CorbaSeq& dst)
    {
      dst.length(src.size());
      for(int i = 0; i < src.size(); ++i)
      {
        dst[i] = src.Get(i);
      }
    }

    template<typename CorbaSeq, typename Repeated>
    void id_seq_to_repeated_(const CorbaSeq& src, Repeated* dst)
    {
      for(CORBA::ULong i = 0; i < src.length(); ++i)
      {
        dst->Add(src[i]);
      }
    }

    void convert_(
      const adserver::user_info_svcs::user_info_manager::GeoData& src,
      GeoData& dst)
    {
      bytes_to_oct_seq_(src.latitude(), dst.latitude);
      bytes_to_oct_seq_(src.longitude(), dst.longitude);
      bytes_to_oct_seq_(src.accuracy(), dst.accuracy);
    }

    void convert_(
      const GeoData& src,
      adserver::user_info_svcs::user_info_manager::GeoData& dst)
    {
      dst.set_latitude(oct_seq_to_bytes_(src.latitude));
      dst.set_longitude(oct_seq_to_bytes_(src.longitude));
      dst.set_accuracy(oct_seq_to_bytes_(src.accuracy));
    }

    void convert_(
      const adserver::user_info_svcs::user_info_manager::UserInfo& src,
      UserInfo& dst)
    {
      bytes_to_oct_seq_(src.user_id(), dst.user_id);
      bytes_to_oct_seq_(src.huser_id(), dst.huser_id);
      dst.time = src.time();
      dst.last_colo_id = src.last_colo_id();
      dst.current_colo_id = src.current_colo_id();
      dst.request_colo_id = src.request_colo_id();
      dst.temporary = src.temporary();
    }

    void convert_(
      const adserver::user_info_svcs::user_info_manager::ProfilesRequestInfo& src,
      ProfilesRequestInfo& dst)
    {
      dst.base_profile = src.base_profile();
      dst.add_profile = src.add_profile();
      dst.history_profile = src.history_profile();
      dst.freq_cap_profile = src.freq_cap_profile();
      dst.pref_profile = src.pref_profile();
    }

    void convert_(
      const adserver::user_info_svcs::user_info_manager::UserProfiles& src,
      UserProfiles& dst)
    {
      bytes_to_oct_seq_(src.base_user_profile(), dst.base_user_profile);
      bytes_to_oct_seq_(src.add_user_profile(), dst.add_user_profile);
      bytes_to_oct_seq_(src.history_user_profile(), dst.history_user_profile);
      bytes_to_oct_seq_(src.freq_cap(), dst.freq_cap);
      bytes_to_oct_seq_(src.pref_profile(), dst.pref_profile);
    }

    void convert_(
      const UserProfiles& src,
      adserver::user_info_svcs::user_info_manager::UserProfiles& dst)
    {
      dst.set_base_user_profile(oct_seq_to_bytes_(src.base_user_profile));
      dst.set_add_user_profile(oct_seq_to_bytes_(src.add_user_profile));
      dst.set_history_user_profile(oct_seq_to_bytes_(src.history_user_profile));
      dst.set_freq_cap(oct_seq_to_bytes_(src.freq_cap));
      dst.set_pref_profile(oct_seq_to_bytes_(src.pref_profile));
    }

    void convert_(
      const adserver::user_info_svcs::user_info_manager::ChannelTriggerMatch& src,
      UserInfoMatcher::ChannelTriggerMatch& dst)
    {
      dst.channel_id = src.channel_id();
      dst.channel_trigger_id = src.channel_trigger_id();
    }

    void convert_(
      const UserInfoMatcher::ChannelTriggerMatch& src,
      adserver::user_info_svcs::user_info_manager::ChannelTriggerMatch& dst)
    {
      dst.set_channel_id(src.channel_id);
      dst.set_channel_trigger_id(src.channel_trigger_id);
    }

    template<typename PbRepeated, typename CorbaSeq>
    void convert_channel_match_seq_(const PbRepeated& src, CorbaSeq& dst)
    {
      dst.length(src.size());
      for(int i = 0; i < src.size(); ++i)
      {
        convert_(src.Get(i), dst[i]);
      }
    }

    void convert_(
      const adserver::user_info_svcs::user_info_manager::MatchParams& src,
      UserInfoMatcher::MatchParams& dst)
    {
      repeated_to_id_seq_(src.persistent_channel_ids(), dst.persistent_channel_ids);
      convert_channel_match_seq_(src.search_channel_ids(), dst.search_channel_ids);
      convert_channel_match_seq_(src.page_channel_ids(), dst.page_channel_ids);
      convert_channel_match_seq_(src.url_channel_ids(), dst.url_channel_ids);
      convert_channel_match_seq_(
        src.url_keyword_channel_ids(),
        dst.url_keyword_channel_ids);
      bytes_to_oct_seq_(src.publishers_optin_timeout(), dst.publishers_optin_timeout);
      dst.use_empty_profile = src.use_empty_profile();
      dst.silent_match = src.silent_match();
      dst.no_match = src.no_match();
      dst.no_result = src.no_result();
      dst.ret_freq_caps = src.ret_freq_caps();
      dst.provide_channel_count = src.provide_channel_count();
      dst.provide_persistent_channels = src.provide_persistent_channels();
      dst.change_last_request = src.change_last_request();
      dst.cohort = src.cohort().c_str();
      dst.cohort2 = src.cohort2().c_str();
      dst.filter_contextual_triggers = src.filter_contextual_triggers();
      dst.geo_data_seq.length(src.geo_data_seq_size());
      for(int i = 0; i < src.geo_data_seq_size(); ++i)
      {
        convert_(src.geo_data_seq(i), dst.geo_data_seq[i]);
      }
    }

    void convert_(
      const UserInfoMatcher::ChannelWeight& src,
      adserver::user_info_svcs::user_info_manager::ChannelWeight& dst)
    {
      dst.set_channel_id(src.channel_id);
      dst.set_weight(src.weight);
    }

    void convert_(
      const UserInfoMatcher::SeqOrderInfo& src,
      adserver::user_info_svcs::user_info_manager::SeqOrderInfo& dst)
    {
      dst.set_ccg_id(src.ccg_id);
      dst.set_set_id(src.set_id);
      dst.set_imps(src.imps);
    }

    void convert_(
      const adserver::user_info_svcs::user_info_manager::SeqOrderInfo& src,
      UserInfoMatcher::SeqOrderInfo& dst)
    {
      dst.ccg_id = src.ccg_id();
      dst.set_id = src.set_id();
      dst.imps = src.imps();
    }

    void convert_(
      const Commons::CampaignFreq& src,
      adserver::user_info_svcs::user_info_manager::CampaignFreq& dst)
    {
      dst.set_campaign_id(src.campaign_id);
      dst.set_imps(src.imps);
    }

    void convert_(
      const UserInfoMatcher::MatchResult& src,
      adserver::user_info_svcs::user_info_manager::MatchResult& dst)
    {
      dst.set_times_inited(src.times_inited);
      dst.set_last_request_time(oct_seq_to_bytes_(src.last_request_time));
      dst.set_create_time(oct_seq_to_bytes_(src.create_time));
      dst.set_session_start(oct_seq_to_bytes_(src.session_start));
      dst.set_colo_id(src.colo_id);
      for(CORBA::ULong i = 0; i < src.channels.length(); ++i)
      {
        convert_(src.channels[i], *dst.add_channels());
      }
      for(CORBA::ULong i = 0; i < src.hid_channels.length(); ++i)
      {
        convert_(src.hid_channels[i], *dst.add_hid_channels());
      }
      id_seq_to_repeated_(src.full_freq_caps, dst.mutable_full_freq_caps());
      id_seq_to_repeated_(
        src.full_virtual_freq_caps,
        dst.mutable_full_virtual_freq_caps());
      for(CORBA::ULong i = 0; i < src.seq_orders.length(); ++i)
      {
        convert_(src.seq_orders[i], *dst.add_seq_orders());
      }
      for(CORBA::ULong i = 0; i < src.campaign_freqs.length(); ++i)
      {
        convert_(src.campaign_freqs[i], *dst.add_campaign_freqs());
      }
      dst.set_fraud_request(src.fraud_request);
      dst.set_process_time(oct_seq_to_bytes_(src.process_time));
      dst.set_adv_channel_count(src.adv_channel_count);
      dst.set_discover_channel_count(src.discover_channel_count);
      dst.set_cohort(src.cohort.in());
      dst.set_cohort2(src.cohort2.in());
      id_seq_to_repeated_(
        src.exclude_pubpixel_accounts,
        dst.mutable_exclude_pubpixel_accounts());
      for(CORBA::ULong i = 0; i < src.geo_data_seq.length(); ++i)
      {
        convert_(src.geo_data_seq[i], *dst.add_geo_data_seq());
      }
    }

    template<typename PbRepeated, typename CorbaSeq>
    void convert_seq_order_seq_(const PbRepeated& src, CorbaSeq& dst)
    {
      dst.length(src.size());
      for(int i = 0; i < src.size(); ++i)
      {
        convert_(src.Get(i), dst[i]);
      }
    }

    grpc::Status to_status_(const UserInfoManager::NotReady& ex)
    {
      return AdServer::Grpc::error_status(
        grpc::StatusCode::UNAVAILABLE,
        ex.description.in());
    }

    grpc::Status to_status_(const UserInfoManager::ChunkNotFound& ex)
    {
      return AdServer::Grpc::error_status(
        grpc::StatusCode::NOT_FOUND,
        ex.description.in());
    }

    grpc::Status to_status_(const UserInfoManager::ImplementationException& ex)
    {
      return AdServer::Grpc::error_status(
        grpc::StatusCode::INTERNAL,
        ex.description.in());
    }
  }

  class UserInfoManagerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      UserInfoManagerGrpc::ServiceImpl,
      adserver::user_info_svcs::user_info_manager::UserInfoManagerGrpc,
      adserver::user_info_svcs::user_info_manager::UserInfoManagerGrpc::AsyncService>
  {
    using AsyncService =
      adserver::user_info_svcs::user_info_manager::UserInfoManagerGrpc::AsyncService;

  public:
    explicit ServiceImpl(UserInfoManagerImpl* user_info_manager)
      : user_info_manager_(ReferenceCounting::add_ref(user_info_manager))
    {}

    static auto grpc_calls()
    {
      namespace pb = adserver::user_info_svcs::user_info_manager;
      return std::make_tuple(
        MAKE_GRPC_CALL(pb::GetSourceRequest, pb::GetSourceResponse, get_source),
        MAKE_GRPC_CALL(pb::GetMasterStampRequest, pb::GetMasterStampResponse, get_master_stamp),
        MAKE_GRPC_CALL(pb::GetUserProfileRequest, pb::GetUserProfileResponse, get_user_profile),
        MAKE_GRPC_CALL(pb::MatchRequest, pb::MatchResponse, match),
        MAKE_GRPC_CALL(pb::UpdateUserFreqCapsRequest, pb::UpdateUserFreqCapsResponse, update_user_freq_caps),
        MAKE_GRPC_CALL(pb::ConfirmUserFreqCapsRequest, pb::ConfirmUserFreqCapsResponse, confirm_user_freq_caps),
        MAKE_GRPC_CALL(pb::FraudUserRequest, pb::FraudUserResponse, fraud_user),
        MAKE_GRPC_CALL(pb::RemoveUserProfileRequest, pb::RemoveUserProfileResponse, remove_user_profile),
        MAKE_GRPC_CALL(pb::MergeRequest, pb::MergeResponse, merge),
        MAKE_GRPC_CALL(pb::ConsiderPublishersOptinRequest, pb::ConsiderPublishersOptinResponse, consider_publishers_optin),
        MAKE_GRPC_CALL(pb::UimReadyRequest, pb::UimReadyResponse, uim_ready),
        MAKE_GRPC_CALL(pb::GetProgressRequest, pb::GetProgressResponse, get_progress),
        MAKE_GRPC_CALL(pb::ClearExpiredRequest, pb::ClearExpiredResponse, clear_expired));
    }

    void get_source(
      const adserver::user_info_svcs::user_info_manager::GetSourceRequest&,
      adserver::user_info_svcs::user_info_manager::GetSourceResponse& response,
      grpc::Status& result_status) const;

    void get_master_stamp(
      const adserver::user_info_svcs::user_info_manager::GetMasterStampRequest&,
      adserver::user_info_svcs::user_info_manager::GetMasterStampResponse& response,
      grpc::Status& result_status) const;

    void get_user_profile(
      const adserver::user_info_svcs::user_info_manager::GetUserProfileRequest& request,
      adserver::user_info_svcs::user_info_manager::GetUserProfileResponse& response,
      grpc::Status& result_status) const;

    void match(
      const adserver::user_info_svcs::user_info_manager::MatchRequest& request,
      adserver::user_info_svcs::user_info_manager::MatchResponse& response,
      grpc::Status& result_status) const;

    void update_user_freq_caps(
      const adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsRequest& request,
      adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsResponse&,
      grpc::Status& result_status) const;

    void confirm_user_freq_caps(
      const adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsRequest& request,
      adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsResponse&,
      grpc::Status& result_status) const;

    void fraud_user(
      const adserver::user_info_svcs::user_info_manager::FraudUserRequest& request,
      adserver::user_info_svcs::user_info_manager::FraudUserResponse& response,
      grpc::Status& result_status) const;

    void remove_user_profile(
      const adserver::user_info_svcs::user_info_manager::RemoveUserProfileRequest& request,
      adserver::user_info_svcs::user_info_manager::RemoveUserProfileResponse& response,
      grpc::Status& result_status) const;

    void merge(
      const adserver::user_info_svcs::user_info_manager::MergeRequest& request,
      adserver::user_info_svcs::user_info_manager::MergeResponse& response,
      grpc::Status& result_status) const;

    void consider_publishers_optin(
      const adserver::user_info_svcs::user_info_manager::ConsiderPublishersOptinRequest& request,
      adserver::user_info_svcs::user_info_manager::ConsiderPublishersOptinResponse&,
      grpc::Status& result_status) const;

    void uim_ready(
      const adserver::user_info_svcs::user_info_manager::UimReadyRequest&,
      adserver::user_info_svcs::user_info_manager::UimReadyResponse& response,
      grpc::Status& result_status) const;

    void get_progress(
      const adserver::user_info_svcs::user_info_manager::GetProgressRequest&,
      adserver::user_info_svcs::user_info_manager::GetProgressResponse& response,
      grpc::Status& result_status) const;

    void clear_expired(
      const adserver::user_info_svcs::user_info_manager::ClearExpiredRequest& request,
      adserver::user_info_svcs::user_info_manager::ClearExpiredResponse&,
      grpc::Status& result_status) const;

  private:
    UserInfoManagerImpl_var user_info_manager_;
  };

  void UserInfoManagerGrpc::ServiceImpl::get_source(
    const adserver::user_info_svcs::user_info_manager::GetSourceRequest&,
    adserver::user_info_svcs::user_info_manager::GetSourceResponse& response,
    grpc::Status& result_status) const
  {
    try
    {
      UserInfoManagerImpl::ChunkIdList chunks;
      unsigned long chunks_number = 0;
      user_info_manager_->get_controllable_chunks(chunks, chunks_number);
      for(const auto chunk : chunks)
      {
        response.add_chunks(chunk);
      }
      response.set_chunks_number(chunks_number);
      result_status = grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void UserInfoManagerGrpc::ServiceImpl::get_master_stamp(
    const adserver::user_info_svcs::user_info_manager::GetMasterStampRequest&,
    adserver::user_info_svcs::user_info_manager::GetMasterStampResponse& response,
    grpc::Status& result_status) const
  {
    try
    {
      CORBACommons::TimestampInfo_var master_stamp;
      user_info_manager_->get_master_stamp(master_stamp.out());
      response.set_master_stamp(oct_seq_to_bytes_(master_stamp.in()));
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManager::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ImplementationException& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::get_user_profile(
    const adserver::user_info_svcs::user_info_manager::GetUserProfileRequest& request,
    adserver::user_info_svcs::user_info_manager::GetUserProfileResponse& response,
    grpc::Status& result_status) const
  {
    try
    {
      CORBACommons::UserIdInfo user_id;
      ProfilesRequestInfo profile_request;
      bytes_to_oct_seq_(request.user_id(), user_id);
      convert_(request.profile_request(), profile_request);

      UserProfiles_var user_profile;
      const bool found = user_info_manager_->get_user_profile(
        user_id,
        request.temporary(),
        profile_request,
        user_profile.out());
      response.set_found(found);
      convert_(user_profile.in(), *response.mutable_user_profile());
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManager::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ImplementationException& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::match(
    const adserver::user_info_svcs::user_info_manager::MatchRequest& request,
    adserver::user_info_svcs::user_info_manager::MatchResponse& response,
    grpc::Status& result_status) const
  {
    try
    {
      UserInfo user_info;
      UserInfoMatcher::MatchParams match_params;
      convert_(request.user_info(), user_info);
      convert_(request.match_params(), match_params);

      UserInfoMatcher::MatchResult_var match_result;
      const bool matched = user_info_manager_->match(
        user_info,
        match_params,
        match_result.out());
      response.set_matched(matched);
      convert_(match_result.in(), *response.mutable_match_result());
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManager::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ImplementationException& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::update_user_freq_caps(
    const adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsRequest& request,
    adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsResponse&,
    grpc::Status& result_status) const
  {
    try
    {
      CORBACommons::UserIdInfo user_id;
      CORBACommons::TimestampInfo time;
      CORBACommons::RequestIdInfo request_id;
      FreqCapIdSeq freq_caps;
      FreqCapIdSeq uc_freq_caps;
      FreqCapIdSeq virtual_freq_caps;
      UserInfoManager::SeqOrderSeq seq_orders;
      CampaignIdSeq campaign_ids;
      CampaignIdSeq uc_campaign_ids;
      bytes_to_oct_seq_(request.user_id(), user_id);
      bytes_to_oct_seq_(request.time(), time);
      bytes_to_oct_seq_(request.request_id(), request_id);
      repeated_to_id_seq_(request.freq_caps(), freq_caps);
      repeated_to_id_seq_(request.uc_freq_caps(), uc_freq_caps);
      repeated_to_id_seq_(request.virtual_freq_caps(), virtual_freq_caps);
      convert_seq_order_seq_(request.seq_orders(), seq_orders);
      repeated_to_id_seq_(request.campaign_ids(), campaign_ids);
      repeated_to_id_seq_(request.uc_campaign_ids(), uc_campaign_ids);
      user_info_manager_->update_user_freq_caps(
        user_id,
        time,
        request_id,
        freq_caps,
        uc_freq_caps,
        virtual_freq_caps,
        seq_orders,
        campaign_ids,
        uc_campaign_ids);
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManager::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ImplementationException& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::confirm_user_freq_caps(
    const adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsRequest& request,
    adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsResponse&,
    grpc::Status& result_status) const
  {
    try
    {
      CORBACommons::UserIdInfo user_id;
      CORBACommons::TimestampInfo time;
      CORBACommons::RequestIdInfo request_id;
      CORBACommons::IdSeq exclude_pubpixel_accounts;
      bytes_to_oct_seq_(request.user_id(), user_id);
      bytes_to_oct_seq_(request.time(), time);
      bytes_to_oct_seq_(request.request_id(), request_id);
      repeated_to_id_seq_(
        request.exclude_pubpixel_accounts(),
        exclude_pubpixel_accounts);
      user_info_manager_->confirm_user_freq_caps(
        user_id,
        time,
        request_id,
        exclude_pubpixel_accounts);
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManager::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ImplementationException& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::fraud_user(
    const adserver::user_info_svcs::user_info_manager::FraudUserRequest& request,
    adserver::user_info_svcs::user_info_manager::FraudUserResponse& response,
    grpc::Status& result_status) const
  {
    try
    {
      CORBACommons::UserIdInfo user_id;
      CORBACommons::TimestampInfo time;
      bytes_to_oct_seq_(request.user_id(), user_id);
      bytes_to_oct_seq_(request.time(), time);
      response.set_fraud(user_info_manager_->fraud_user(user_id, time));
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManager::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ImplementationException& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::remove_user_profile(
    const adserver::user_info_svcs::user_info_manager::RemoveUserProfileRequest& request,
    adserver::user_info_svcs::user_info_manager::RemoveUserProfileResponse& response,
    grpc::Status& result_status) const
  {
    try
    {
      CORBACommons::UserIdInfo user_id;
      bytes_to_oct_seq_(request.user_id(), user_id);
      response.set_removed(user_info_manager_->remove_user_profile(user_id));
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManager::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ImplementationException& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::merge(
    const adserver::user_info_svcs::user_info_manager::MergeRequest& request,
    adserver::user_info_svcs::user_info_manager::MergeResponse& response,
    grpc::Status& result_status) const
  {
    try
    {
      UserInfo user_info;
      UserInfoMatcher::MatchParams match_params;
      UserProfiles merge_user_profile;
      CORBA::Boolean merge_success = false;
      CORBACommons::TimestampInfo_var last_request;
      convert_(request.user_info(), user_info);
      convert_(request.match_params(), match_params);
      convert_(request.merge_user_profile(), merge_user_profile);
      response.set_result(user_info_manager_->merge(
        user_info,
        match_params,
        merge_user_profile,
        merge_success,
        last_request.out()));
      response.set_merge_success(merge_success);
      response.set_last_request(oct_seq_to_bytes_(last_request.in()));
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManager::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ImplementationException& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::consider_publishers_optin(
    const adserver::user_info_svcs::user_info_manager::ConsiderPublishersOptinRequest& request,
    adserver::user_info_svcs::user_info_manager::ConsiderPublishersOptinResponse&,
    grpc::Status& result_status) const
  {
    try
    {
      CORBACommons::UserIdInfo user_id;
      CORBACommons::TimestampInfo now;
      CORBACommons::IdSeq exclude_pubpixel_accounts;
      bytes_to_oct_seq_(request.user_id(), user_id);
      bytes_to_oct_seq_(request.now(), now);
      repeated_to_id_seq_(
        request.exclude_pubpixel_accounts(),
        exclude_pubpixel_accounts);
      user_info_manager_->consider_publishers_optin(
        user_id,
        exclude_pubpixel_accounts,
        now);
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManager::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManager::ImplementationException& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::uim_ready(
    const adserver::user_info_svcs::user_info_manager::UimReadyRequest&,
    adserver::user_info_svcs::user_info_manager::UimReadyResponse& response,
    grpc::Status& result_status) const
  {
    response.set_ready(user_info_manager_->uim_ready());
    result_status = grpc::Status::OK;
  }

  void UserInfoManagerGrpc::ServiceImpl::get_progress(
    const adserver::user_info_svcs::user_info_manager::GetProgressRequest&,
    adserver::user_info_svcs::user_info_manager::GetProgressResponse& response,
    grpc::Status& result_status) const
  {
    CORBA::String_var progress = user_info_manager_->get_progress();
    response.set_progress(progress.in());
    result_status = grpc::Status::OK;
  }

  void UserInfoManagerGrpc::ServiceImpl::clear_expired(
    const adserver::user_info_svcs::user_info_manager::ClearExpiredRequest& request,
    adserver::user_info_svcs::user_info_manager::ClearExpiredResponse&,
    grpc::Status& result_status) const
  {
    try
    {
      CORBACommons::TimestampInfo cleanup_time;
      bytes_to_oct_seq_(request.cleanup_time(), cleanup_time);
      user_info_manager_->clear_expired(
        request.sync(),
        cleanup_time,
        request.portion());
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManager::ImplementationException& ex)
    {
      result_status = to_status_(ex);
    }
  }

  UserInfoManagerGrpc::UserInfoManagerGrpc(
    UserInfoManagerImpl* user_info_manager,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        user_info_manager_grpc_aspect,
        bind_address_,
        std::make_unique<ServiceImpl>(user_info_manager)))
  {
    add_child_object(impl_);
  }

  UserInfoManagerGrpc::~UserInfoManagerGrpc() noexcept = default;
}
