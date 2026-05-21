#include "UserInfoGrpcAlgs.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Grpc/GrpcServer.hpp>
#include <Commons/Grpc/GrpcSync.hpp>
#include <Commons/UserInfoManip.hpp>

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

namespace AdServer::UserInfoSvcs
{
  namespace
  {
    namespace pb = adserver::user_info_svcs::user_info_manager;

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

    grpc::Status exception_status_(
      grpc::StatusCode code,
      const char* description)
    {
      return AdServer::Grpc::error_status(code, description);
    }

    grpc::Status exception_status_(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << ex;
      return grpc::Status(grpc::StatusCode::UNAVAILABLE, ostr.str().str());
    }

    grpc::Status exception_status_(const eh::Exception& ex)
    {
      return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
    }

    grpc::Status unsupported_status_(const char* method)
    {
      Stream::Error ostr;
      ostr << "UserInfoCorbaClient::" << method <<
        "(): unsupported by CORBA distributor";
      return grpc::Status(grpc::StatusCode::UNIMPLEMENTED, ostr.str().str());
    }

    void convert_(const pb::GeoData& src, GeoData& dst)
    {
      bytes_to_oct_seq_(src.latitude(), dst.latitude);
      bytes_to_oct_seq_(src.longitude(), dst.longitude);
      bytes_to_oct_seq_(src.accuracy(), dst.accuracy);
    }

    void convert_(const GeoData& src, pb::GeoData& dst)
    {
      dst.set_latitude(oct_seq_to_bytes_(src.latitude));
      dst.set_longitude(oct_seq_to_bytes_(src.longitude));
      dst.set_accuracy(oct_seq_to_bytes_(src.accuracy));
    }

    void convert_(const pb::UserInfo& src, UserInfo& dst)
    {
      bytes_to_oct_seq_(src.user_id(), dst.user_id);
      bytes_to_oct_seq_(src.huser_id(), dst.huser_id);
      dst.time = src.time();
      dst.last_colo_id = src.last_colo_id();
      dst.current_colo_id = src.current_colo_id();
      dst.request_colo_id = src.request_colo_id();
      dst.temporary = src.temporary();
    }

    void convert_(const UserInfo& src, pb::UserInfo& dst)
    {
      dst.set_user_id(oct_seq_to_bytes_(src.user_id));
      dst.set_huser_id(oct_seq_to_bytes_(src.huser_id));
      dst.set_time(src.time);
      dst.set_last_colo_id(src.last_colo_id);
      dst.set_current_colo_id(src.current_colo_id);
      dst.set_request_colo_id(src.request_colo_id);
      dst.set_temporary(src.temporary);
    }

    void convert_(const pb::ProfilesRequestInfo& src, ProfilesRequestInfo& dst)
    {
      dst.base_profile = src.base_profile();
      dst.add_profile = src.add_profile();
      dst.history_profile = src.history_profile();
      dst.freq_cap_profile = src.freq_cap_profile();
      dst.pref_profile = src.pref_profile();
    }

    void convert_(const ProfilesRequestInfo& src, pb::ProfilesRequestInfo& dst)
    {
      dst.set_base_profile(src.base_profile);
      dst.set_add_profile(src.add_profile);
      dst.set_history_profile(src.history_profile);
      dst.set_freq_cap_profile(src.freq_cap_profile);
      dst.set_pref_profile(src.pref_profile);
    }

    void convert_(const pb::UserProfiles& src, UserProfiles& dst)
    {
      bytes_to_oct_seq_(src.base_user_profile(), dst.base_user_profile);
      bytes_to_oct_seq_(src.add_user_profile(), dst.add_user_profile);
      bytes_to_oct_seq_(src.history_user_profile(), dst.history_user_profile);
      bytes_to_oct_seq_(src.freq_cap(), dst.freq_cap);
      bytes_to_oct_seq_(src.pref_profile(), dst.pref_profile);
    }

    void convert_(const UserProfiles& src, pb::UserProfiles& dst)
    {
      dst.set_base_user_profile(oct_seq_to_bytes_(src.base_user_profile));
      dst.set_add_user_profile(oct_seq_to_bytes_(src.add_user_profile));
      dst.set_history_user_profile(oct_seq_to_bytes_(src.history_user_profile));
      dst.set_freq_cap(oct_seq_to_bytes_(src.freq_cap));
      dst.set_pref_profile(oct_seq_to_bytes_(src.pref_profile));
    }

    void convert_(
      const pb::ChannelTriggerMatch& src,
      UserInfoMatcher::ChannelTriggerMatch& dst)
    {
      dst.channel_id = src.channel_id();
      dst.channel_trigger_id = src.channel_trigger_id();
    }

    void convert_(
      const UserInfoMatcher::ChannelTriggerMatch& src,
      pb::ChannelTriggerMatch& dst)
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

    void convert_(const pb::MatchParams& src, UserInfoMatcher::MatchParams& dst)
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

    template<typename CorbaSeq, typename PbRepeated>
    void convert_channel_match_seq_(const CorbaSeq& src, PbRepeated* dst)
    {
      for(CORBA::ULong i = 0; i < src.length(); ++i)
      {
        convert_(src[i], *dst->Add());
      }
    }

    void convert_(const UserInfoMatcher::MatchParams& src, pb::MatchParams& dst)
    {
      id_seq_to_repeated_(
        src.persistent_channel_ids,
        dst.mutable_persistent_channel_ids());
      convert_channel_match_seq_(
        src.search_channel_ids,
        dst.mutable_search_channel_ids());
      convert_channel_match_seq_(
        src.page_channel_ids,
        dst.mutable_page_channel_ids());
      convert_channel_match_seq_(
        src.url_channel_ids,
        dst.mutable_url_channel_ids());
      convert_channel_match_seq_(
        src.url_keyword_channel_ids,
        dst.mutable_url_keyword_channel_ids());
      dst.set_publishers_optin_timeout(oct_seq_to_bytes_(
        src.publishers_optin_timeout));
      dst.set_use_empty_profile(src.use_empty_profile);
      dst.set_silent_match(src.silent_match);
      dst.set_no_match(src.no_match);
      dst.set_no_result(src.no_result);
      dst.set_ret_freq_caps(src.ret_freq_caps);
      dst.set_provide_channel_count(src.provide_channel_count);
      dst.set_provide_persistent_channels(src.provide_persistent_channels);
      dst.set_change_last_request(src.change_last_request);
      dst.set_cohort(src.cohort.in());
      dst.set_cohort2(src.cohort2.in());
      dst.set_filter_contextual_triggers(src.filter_contextual_triggers);
      for(CORBA::ULong i = 0; i < src.geo_data_seq.length(); ++i)
      {
        convert_(src.geo_data_seq[i], *dst.add_geo_data_seq());
      }
    }

    void convert_(const UserInfoMatcher::ChannelWeight& src, pb::ChannelWeight& dst)
    {
      dst.set_channel_id(src.channel_id);
      dst.set_weight(src.weight);
    }

    void convert_(const UserInfoMatcher::SeqOrderInfo& src, pb::SeqOrderInfo& dst)
    {
      dst.set_ccg_id(src.ccg_id);
      dst.set_set_id(src.set_id);
      dst.set_imps(src.imps);
    }

    void convert_(const pb::SeqOrderInfo& src, UserInfoMatcher::SeqOrderInfo& dst)
    {
      dst.ccg_id = src.ccg_id();
      dst.set_id = src.set_id();
      dst.imps = src.imps();
    }

    void convert_(const Commons::CampaignFreq& src, pb::CampaignFreq& dst)
    {
      dst.set_campaign_id(src.campaign_id);
      dst.set_imps(src.imps);
    }

    void convert_(
      const pb::ChannelWeight& src,
      UserInfoMatcher::ChannelWeight& dst)
    {
      dst.channel_id = src.channel_id();
      dst.weight = src.weight();
    }

    void convert_(
      const pb::CampaignFreq& src,
      Commons::CampaignFreq& dst)
    {
      dst.campaign_id = src.campaign_id();
      dst.imps = src.imps();
    }

    void convert_(const UserInfoMatcher::MatchResult& src, pb::MatchResult& dst)
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

    void convert_(const pb::MatchResult& src, UserInfoMatcher::MatchResult& dst)
    {
      dst.times_inited = src.times_inited();
      bytes_to_oct_seq_(src.last_request_time(), dst.last_request_time);
      bytes_to_oct_seq_(src.create_time(), dst.create_time);
      bytes_to_oct_seq_(src.session_start(), dst.session_start);
      dst.colo_id = src.colo_id();
      dst.channels.length(src.channels_size());
      for(int i = 0; i < src.channels_size(); ++i)
      {
        convert_(src.channels(i), dst.channels[i]);
      }
      dst.hid_channels.length(src.hid_channels_size());
      for(int i = 0; i < src.hid_channels_size(); ++i)
      {
        convert_(src.hid_channels(i), dst.hid_channels[i]);
      }
      repeated_to_id_seq_(src.full_freq_caps(), dst.full_freq_caps);
      repeated_to_id_seq_(
        src.full_virtual_freq_caps(),
        dst.full_virtual_freq_caps);
      dst.seq_orders.length(src.seq_orders_size());
      for(int i = 0; i < src.seq_orders_size(); ++i)
      {
        convert_(src.seq_orders(i), dst.seq_orders[i]);
      }
      dst.campaign_freqs.length(src.campaign_freqs_size());
      for(int i = 0; i < src.campaign_freqs_size(); ++i)
      {
        convert_(src.campaign_freqs(i), dst.campaign_freqs[i]);
      }
      dst.fraud_request = src.fraud_request();
      bytes_to_oct_seq_(src.process_time(), dst.process_time);
      dst.adv_channel_count = src.adv_channel_count();
      dst.discover_channel_count = src.discover_channel_count();
      dst.cohort = src.cohort().c_str();
      dst.cohort2 = src.cohort2().c_str();
      repeated_to_id_seq_(
        src.exclude_pubpixel_accounts(),
        dst.exclude_pubpixel_accounts);
      dst.geo_data_seq.length(src.geo_data_seq_size());
      for(int i = 0; i < src.geo_data_seq_size(); ++i)
      {
        convert_(src.geo_data_seq(i), dst.geo_data_seq[i]);
      }
    }

    UserInfoMatcher::MatchResult*
    convert_history_match_result_(const pb::MatchResult& response)
    {
      UserInfoMatcher::MatchResult_var result =
        new UserInfoMatcher::MatchResult;
      convert_(response, *result);
      return result._retn();
    }

    void throw_user_info_exception_(const grpc::Status& status)
    {
      const std::string message = status.error_message();
      switch(status.error_code())
      {
      case grpc::StatusCode::UNAVAILABLE:
        throw UserInfoMatcher::NotReady(message.c_str());
      case grpc::StatusCode::NOT_FOUND:
        throw UserInfoMatcher::ChunkNotFound(message.c_str());
      default:
        throw UserInfoMatcher::ImplementationException(message.c_str());
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
  }

  namespace GrpcAlgs
  {
    bool get_user_profile(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id,
      CORBA::Boolean temporary,
      const ProfilesRequestInfo& profile_request,
      UserProfiles_out user_profile)
    {
      pb::GetUserProfileRequest request;
      request.set_user_id(oct_seq_to_bytes_(user_id));
      request.set_temporary(temporary);
      convert_(profile_request, *request.mutable_profile_request());
      const auto response = AdServer::Grpc::sync_call<
        pb::GetUserProfileResponse>(
          [&](auto callback)
          {
            client.get_user_profile(request, std::move(callback));
          },
          throw_user_info_exception_);
      UserProfiles_var result = new UserProfiles;
      convert_(response.user_profile(), *result);
      user_profile = result._retn();
      return response.found();
    }

    bool history_match(
      UserInfoManagerGrpcAsyncClient& client,
      const pb::MatchRequest& request,
      pb::MatchResponse& response)
    {
      response = AdServer::Grpc::sync_call<pb::MatchResponse>(
        [&](auto callback)
        {
          client.match(request, std::move(callback));
        },
        throw_user_info_exception_);
      return response.matched();
    }

    UserInfoMatcher::MatchResult* history_match(
      UserInfoManagerGrpcAsyncClient& client,
      const pb::MatchRequest& request)
    {
      pb::MatchResponse response;
      history_match(client, request, response);
      return convert_history_match_result_(response.match_result());
    }

    UserInfoMatcher::MatchResult* make_history_match_result(
      const pb::MatchResponse& response)
    {
      return convert_history_match_result_(response.match_result());
    }

    void make_update_user_freq_caps_request(
      pb::UpdateUserFreqCapsRequest& request,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const FreqCapIdSeq& freq_caps,
      const FreqCapIdSeq& uc_freq_caps,
      const FreqCapIdSeq& virtual_freq_caps,
      const UserInfoManager::SeqOrderSeq& seq_orders,
      const CampaignIdSeq& campaign_ids,
      const CampaignIdSeq& uc_campaign_ids)
    {
      request.set_user_id(oct_seq_to_bytes_(user_id));
      request.set_time(oct_seq_to_bytes_(time));
      request.set_request_id(oct_seq_to_bytes_(request_id));
      id_seq_to_repeated_(freq_caps, request.mutable_freq_caps());
      id_seq_to_repeated_(uc_freq_caps, request.mutable_uc_freq_caps());
      id_seq_to_repeated_(
        virtual_freq_caps,
        request.mutable_virtual_freq_caps());
      for(CORBA::ULong i = 0; i < seq_orders.length(); ++i)
      {
        convert_(seq_orders[i], *request.add_seq_orders());
      }
      id_seq_to_repeated_(campaign_ids, request.mutable_campaign_ids());
      id_seq_to_repeated_(uc_campaign_ids, request.mutable_uc_campaign_ids());
    }

    bool merge(
      UserInfoManagerGrpcAsyncClient& client,
      const UserInfo& user_info,
      const UserInfoMatcher::MatchParams& match_params,
      const UserProfiles& merge_user_profile,
      CORBA::Boolean_out merge_success,
      CORBACommons::TimestampInfo_out last_request)
    {
      pb::MergeRequest request;
      convert_(user_info, *request.mutable_user_info());
      convert_(match_params, *request.mutable_match_params());
      convert_(merge_user_profile, *request.mutable_merge_user_profile());
      const auto response = AdServer::Grpc::sync_call<pb::MergeResponse>(
        [&](auto callback)
        {
          client.merge(request, std::move(callback));
        },
        throw_user_info_exception_);
      merge_success = response.merge_success();
      CORBACommons::TimestampInfo_var last_request_value =
        new CORBACommons::TimestampInfo;
      bytes_to_oct_seq_(response.last_request(), last_request_value.inout());
      last_request = last_request_value._retn();
      return response.result();
    }

    bool remove_user_profile(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id_info)
    {
      pb::RemoveUserProfileRequest request;
      request.set_user_id(oct_seq_to_bytes_(user_id_info));
      const auto response = AdServer::Grpc::sync_call<
        pb::RemoveUserProfileResponse>(
          [&](auto callback)
          {
            client.remove_user_profile(request, std::move(callback));
          },
          throw_user_info_exception_);
      return response.removed();
    }

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
      const CampaignIdSeq& uc_campaign_ids)
    {
      pb::UpdateUserFreqCapsRequest request;
      make_update_user_freq_caps_request(
        request,
        user_id,
        time,
        request_id,
        freq_caps,
        uc_freq_caps,
        virtual_freq_caps,
        seq_orders,
        campaign_ids,
        uc_campaign_ids);
      AdServer::Grpc::sync_call<pb::UpdateUserFreqCapsResponse>(
        [&](auto callback)
        {
          client.update_user_freq_caps(request, std::move(callback));
        },
        throw_user_info_exception_);
    }

    void make_confirm_user_freq_caps_request(
      pb::ConfirmUserFreqCapsRequest& request,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const CORBACommons::IdSeq& exclude_pubpixel_accounts)
    {
      request.set_user_id(oct_seq_to_bytes_(user_id));
      request.set_time(oct_seq_to_bytes_(time));
      request.set_request_id(oct_seq_to_bytes_(request_id));
      id_seq_to_repeated_(
        exclude_pubpixel_accounts,
        request.mutable_exclude_pubpixel_accounts());
    }

    void confirm_user_freq_caps(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time,
      const CORBACommons::RequestIdInfo& request_id,
      const CORBACommons::IdSeq& exclude_pubpixel_accounts)
    {
      pb::ConfirmUserFreqCapsRequest request;
      make_confirm_user_freq_caps_request(
        request,
        user_id,
        time,
        request_id,
        exclude_pubpixel_accounts);
      AdServer::Grpc::sync_call<pb::ConfirmUserFreqCapsResponse>(
        [&](auto callback)
        {
          client.confirm_user_freq_caps(request, std::move(callback));
        },
        throw_user_info_exception_);
    }

    bool fraud_user(
      UserInfoManagerGrpcAsyncClient& client,
      const CORBACommons::UserIdInfo& user_id,
      const CORBACommons::TimestampInfo& time)
    {
      pb::FraudUserRequest request;
      request.set_user_id(oct_seq_to_bytes_(user_id));
      request.set_time(oct_seq_to_bytes_(time));
      const auto response =
        AdServer::Grpc::sync_call<pb::FraudUserResponse>(
          [&](auto callback)
          {
            client.fraud_user(request, std::move(callback));
          },
          throw_user_info_exception_);
      return response.fraud();
    }
  }
}
