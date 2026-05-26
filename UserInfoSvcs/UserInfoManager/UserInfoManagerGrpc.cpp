#include "UserInfoManagerGrpc.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <atomic>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <unistd.h>
#include <unistd.h>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Grpc/GrpcServer.hpp>
#include <Commons/Grpc/ProcessControl.grpc.pb.h>

#include <UserInfoSvcs/UserInfoManager/UserInfoManagerGrpc.grpc.pb.h>

namespace AdServer::UserInfoSvcs
{
  struct UserInfoManagerGrpc::StatsCounters
  {
    std::atomic<std::uint64_t> call_in_progress{0};
    std::atomic<std::uint64_t> match_in_progress{0};
    std::atomic<std::uint64_t> update_user_freq_caps_in_progress{0};
    std::atomic<std::uint64_t> confirm_user_freq_caps_in_progress{0};
    std::atomic<std::uint64_t> fraud_user_in_progress{0};
    std::atomic<std::uint64_t> remove_user_profile_in_progress{0};
    std::atomic<std::uint64_t> merge_in_progress{0};
    std::atomic<std::uint64_t> consider_publishers_optin_in_progress{0};
  };

  namespace
  {
    constexpr const char user_info_manager_grpc_aspect[] =
      "UserInfoManagerGrpc";
    namespace pc = adserver::grpc::process_control;

    const std::string&
    service_hostname_()
    {
      static const std::string hostname = []()
      {
        char buffer[256];
        if (::gethostname(buffer, sizeof(buffer)) != 0)
        {
          return std::string();
        }
        buffer[sizeof(buffer) - 1] = 0;
        return std::string(buffer);
      }();
      return hostname;
    }

    class InProgressGuard final
    {
    public:
      explicit InProgressGuard(std::atomic<std::uint64_t>& counter) noexcept
        : counter_(counter)
      {
        counter_.fetch_add(1, std::memory_order_relaxed);
      }

      ~InProgressGuard() noexcept
      {
        counter_.fetch_sub(1, std::memory_order_relaxed);
      }

    private:
      std::atomic<std::uint64_t>& counter_;
    };

    template<typename Seq>
    void bytes_to_seq_(const std::string& src, Seq& dst)
    {
      dst.length(src.size());
      std::copy(src.begin(), src.end(), dst.get_buffer());
    }

    template<typename Seq>
    std::string seq_to_bytes_(const Seq& src)
    {
      return std::string(
        reinterpret_cast<const char*>(src.get_buffer()),
        reinterpret_cast<const char*>(src.get_buffer() + src.length()));
    }

    AdServer::Commons::UserId unpack_user_id_(const std::string& src)
    {
      CORBACommons::UserIdInfo id;
      bytes_to_seq_(src, id);
      return CorbaAlgs::unpack_user_id(id);
    }

    Generics::Time unpack_time_(const std::string& src)
    {
      CORBACommons::TimestampInfo time;
      bytes_to_seq_(src, time);
      return CorbaAlgs::unpack_time(time);
    }

    Generics::Uuid unpack_request_id_(const std::string& src)
    {
      CORBACommons::RequestIdInfo request_id;
      bytes_to_seq_(src, request_id);
      return CorbaAlgs::unpack_request_id(request_id);
    }

    std::string pack_time_(const Generics::Time& src)
    {
      return seq_to_bytes_(CorbaAlgs::pack_time(src));
    }

    template<typename Decimal>
    Decimal unpack_decimal_(const std::string& src)
    {
      CORBACommons::OctSeq value;
      bytes_to_seq_(src, value);
      return CorbaAlgs::unpack_decimal<Decimal>(value);
    }

    template<typename Decimal>
    std::string pack_decimal_(const Decimal& src)
    {
      return seq_to_bytes_(CorbaAlgs::pack_decimal<Decimal>(src));
    }

    UserInfoManagerCore::ByteVector bytes_to_vector_(const std::string& src)
    {
      return UserInfoManagerCore::ByteVector(src.begin(), src.end());
    }

    std::string vector_to_bytes_(
      const UserInfoManagerCore::ByteVector& src)
    {
      return std::string(src.begin(), src.end());
    }

    UserInfoManagerCore::UserInfo unpack_user_info_(
      const adserver::user_info_svcs::user_info_manager::UserInfo& src)
    {
      UserInfoManagerCore::UserInfo dst;
      dst.user_id = unpack_user_id_(src.user_id());
      dst.huser_id = unpack_user_id_(src.huser_id());
      dst.time = Generics::Time(src.time());
      dst.request_colo_id = src.request_colo_id();
      dst.current_colo_id = src.current_colo_id();
      dst.temporary = src.temporary();
      return dst;
    }

    UserInfoManagerCore::ProfilesRequest
    unpack_profiles_request_(
      const adserver::user_info_svcs::user_info_manager::ProfilesRequestInfo& src)
    {
      return {
        src.base_profile(),
        src.add_profile(),
        src.history_profile(),
        src.freq_cap_profile()};
    }

    UserInfoManagerCore::UserProfiles
    unpack_user_profiles_(
      const adserver::user_info_svcs::user_info_manager::UserProfiles& src)
    {
      return {
        bytes_to_vector_(src.base_user_profile()),
        bytes_to_vector_(src.add_user_profile()),
        bytes_to_vector_(src.history_user_profile()),
        bytes_to_vector_(src.freq_cap()),
        bytes_to_vector_(src.pref_profile())};
    }

    void pack_user_profiles_(
      const UserInfoManagerCore::UserProfiles& src,
      adserver::user_info_svcs::user_info_manager::UserProfiles& dst)
    {
      dst.set_base_user_profile(vector_to_bytes_(src.base_user_profile));
      dst.set_add_user_profile(vector_to_bytes_(src.add_user_profile));
      dst.set_history_user_profile(vector_to_bytes_(src.history_user_profile));
      dst.set_freq_cap(vector_to_bytes_(src.freq_cap));
      dst.set_pref_profile(vector_to_bytes_(src.pref_profile));
    }

    template<typename Repeated>
    std::vector<UserInfoManagerCore::ChannelTriggerMatch>
    unpack_channel_matches_(const Repeated& src)
    {
      std::vector<UserInfoManagerCore::ChannelTriggerMatch> result;
      result.reserve(src.size());
      for(int i = 0; i < src.size(); ++i)
      {
        const auto& item = src.Get(i);
        result.push_back({item.channel_id(), item.channel_trigger_id()});
      }
      return result;
    }

    template<typename Repeated>
    std::vector<unsigned long> unpack_ids_(const Repeated& src)
    {
      std::vector<unsigned long> result;
      result.reserve(src.size());
      for(int i = 0; i < src.size(); ++i)
      {
        result.push_back(src.Get(i));
      }
      return result;
    }

    template<typename Repeated>
    std::set<unsigned long> unpack_id_set_(const Repeated& src)
    {
      std::set<unsigned long> result;
      for(int i = 0; i < src.size(); ++i)
      {
        result.insert(src.Get(i));
      }
      return result;
    }

    template<typename Repeated>
    std::vector<UserInfoManagerCore::SeqOrder>
    unpack_seq_orders_(const Repeated& src)
    {
      std::vector<UserInfoManagerCore::SeqOrder> result;
      result.reserve(src.size());
      for(int i = 0; i < src.size(); ++i)
      {
        const auto& item = src.Get(i);
        result.push_back({item.ccg_id(), item.set_id(), item.imps()});
      }
      return result;
    }

    UserInfoManagerCore::MatchParams
    unpack_match_params_(
      const adserver::user_info_svcs::user_info_manager::MatchParams& src)
    {
      UserInfoManagerCore::MatchParams dst;
      dst.page_channel_ids = unpack_channel_matches_(src.page_channel_ids());
      dst.search_channel_ids = unpack_channel_matches_(src.search_channel_ids());
      dst.url_channel_ids = unpack_channel_matches_(src.url_channel_ids());
      dst.url_keyword_channel_ids =
        unpack_channel_matches_(src.url_keyword_channel_ids());
      dst.persistent_channel_ids = unpack_ids_(src.persistent_channel_ids());
      dst.cohort = src.cohort();
      dst.cohort2 = src.cohort2();
      dst.publishers_optin_timeout =
        unpack_time_(src.publishers_optin_timeout());
      dst.use_empty_profile = src.use_empty_profile();
      dst.filter_contextual_triggers = src.filter_contextual_triggers();
      dst.silent_match = src.silent_match();
      dst.no_match = src.no_match();
      dst.no_result = src.no_result();
      dst.provide_channel_count = src.provide_channel_count();
      dst.provide_persistent_channels = src.provide_persistent_channels();
      dst.change_last_request = src.change_last_request();
      dst.ret_freq_caps = src.ret_freq_caps();
      dst.geo_data_seq.reserve(src.geo_data_seq_size());
      for(int i = 0; i < src.geo_data_seq_size(); ++i)
      {
        const auto& geo_data = src.geo_data_seq(i);
        dst.geo_data_seq.push_back({
          unpack_decimal_<CampaignSvcs::CoordDecimal>(geo_data.latitude()),
          unpack_decimal_<CampaignSvcs::CoordDecimal>(geo_data.longitude()),
          unpack_decimal_<CampaignSvcs::AccuracyDecimal>(geo_data.accuracy())});
      }
      return dst;
    }

    void convert_(
      const UserInfoManagerCore::ChannelWeight& src,
      adserver::user_info_svcs::user_info_manager::ChannelWeight& dst)
    {
      dst.set_channel_id(src.channel_id);
      dst.set_weight(src.weight);
    }

    void convert_(
      const UserInfoManagerCore::SeqOrder& src,
      adserver::user_info_svcs::user_info_manager::SeqOrderInfo& dst)
    {
      dst.set_ccg_id(src.ccg_id);
      dst.set_set_id(src.set_id);
      dst.set_imps(src.imps);
    }

    void convert_(
      const UserInfoManagerCore::CampaignFreq& src,
      adserver::user_info_svcs::user_info_manager::CampaignFreq& dst)
    {
      dst.set_campaign_id(src.campaign_id);
      dst.set_imps(src.imps);
    }

    void convert_(
      const UserInfoManagerCore::GeoData& src,
      adserver::user_info_svcs::user_info_manager::GeoData& dst)
    {
      dst.set_latitude(pack_decimal_(src.latitude));
      dst.set_longitude(pack_decimal_(src.longitude));
      dst.set_accuracy(pack_decimal_(src.accuracy));
    }

    void convert_(
      const UserInfoManagerCore::MatchResult& src,
      adserver::user_info_svcs::user_info_manager::MatchResult& dst)
    {
      dst.set_times_inited(src.times_inited);
      dst.set_last_request_time(pack_time_(src.last_request_time));
      dst.set_create_time(pack_time_(src.create_time));
      dst.set_session_start(pack_time_(src.session_start));
      dst.set_colo_id(src.colo_id);
      for(const auto& channel : src.channels)
      {
        convert_(channel, *dst.add_channels());
      }
      for(const auto& channel : src.hid_channels)
      {
        convert_(channel, *dst.add_hid_channels());
      }
      for(const auto id : src.full_freq_caps)
      {
        dst.add_full_freq_caps(id);
      }
      for(const auto id : src.full_virtual_freq_caps)
      {
        dst.add_full_virtual_freq_caps(id);
      }
      for(const auto& seq_order : src.seq_orders)
      {
        convert_(seq_order, *dst.add_seq_orders());
      }
      for(const auto& campaign_freq : src.campaign_freqs)
      {
        convert_(campaign_freq, *dst.add_campaign_freqs());
      }
      dst.set_fraud_request(src.fraud_request);
      dst.set_process_time(pack_time_(src.process_time));
      dst.set_adv_channel_count(src.adv_channel_count);
      dst.set_discover_channel_count(src.discover_channel_count);
      dst.set_cohort(src.cohort);
      dst.set_cohort2(src.cohort2);
      for(const auto id : src.exclude_pubpixel_accounts)
      {
        dst.add_exclude_pubpixel_accounts(id);
      }
      for(const auto& geo_data : src.geo_data_seq)
      {
        convert_(geo_data, *dst.add_geo_data_seq());
      }
    }

    grpc::Status to_status_(const UserInfoManagerCore::NotReady& ex)
    {
      return AdServer::Grpc::error_status(
        grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }

    grpc::Status to_status_(const UserInfoManagerCore::ChunkNotFound& ex)
    {
      return AdServer::Grpc::error_status(
        grpc::StatusCode::NOT_FOUND,
        ex.what());
    }

    grpc::Status to_status_(const UserInfoManagerCore::Exception& ex)
    {
      return AdServer::Grpc::error_status(
        grpc::StatusCode::INTERNAL,
        ex.what());
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
    ServiceImpl(
      UserInfoManagerCorePtr user_info_manager,
      std::shared_ptr<StatsCounters> stats_counters)
      : process_control_service_(*this),
        stats_counters_(std::move(stats_counters)),
        user_info_manager_(std::move(user_info_manager))
    {
      add_grpc_service(&process_control_service_);
    }

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
      adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsResponse& response,
      grpc::Status& result_status) const;

    void confirm_user_freq_caps(
      const adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsRequest& request,
      adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsResponse& response,
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
    class ProcessControlService final:
      public pc::ProcessControl::Service
    {
    public:
      explicit ProcessControlService(const ServiceImpl& owner);

      grpc::Status get_status(
        grpc::ServerContext* context,
        const pc::GetStatusRequest* request,
        pc::GetStatusResponse* response) override;

    private:
      const ServiceImpl& owner_;
    };

    void get_status_(pc::GetStatusResponse& response) const;

  private:
    ProcessControlService process_control_service_;
    std::shared_ptr<StatsCounters> stats_counters_;
    UserInfoManagerCorePtr user_info_manager_;
  };

  UserInfoManagerGrpc::ServiceImpl::ProcessControlService::
  ProcessControlService(const ServiceImpl& owner)
    : owner_(owner)
  {}

  grpc::Status
  UserInfoManagerGrpc::ServiceImpl::ProcessControlService::get_status(
    grpc::ServerContext*,
    const pc::GetStatusRequest*,
    pc::GetStatusResponse* response)
  {
    owner_.get_status_(*response);
    return grpc::Status::OK;
  }

  void
  UserInfoManagerGrpc::ServiceImpl::get_status_(
    pc::GetStatusResponse& response) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    try
    {
      const bool ready = user_info_manager_->uim_ready();
      response.set_ready(ready);
      if (!ready)
      {
        response.set_description(user_info_manager_->get_progress());
      }
    }
    catch(const eh::Exception& ex)
    {
      response.set_ready(false);
      response.set_description(ex.what());
    }
  }

  void UserInfoManagerGrpc::ServiceImpl::get_source(
    const adserver::user_info_svcs::user_info_manager::GetSourceRequest&,
    adserver::user_info_svcs::user_info_manager::GetSourceResponse& response,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    try
    {
      UserInfoManagerCore::ChunkIdList chunks;
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
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    try
    {
      response.set_master_stamp(pack_time_(user_info_manager_->get_master_stamp()));
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManagerCore::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::Exception& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::get_user_profile(
    const adserver::user_info_svcs::user_info_manager::GetUserProfileRequest& request,
    adserver::user_info_svcs::user_info_manager::GetUserProfileResponse& response,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    response.set_hostname(service_hostname_());
    try
    {
      UserInfoManagerCore::UserProfiles user_profile;
      const bool found = user_info_manager_->get_user_profile(
        unpack_user_id_(request.user_id()),
        request.temporary(),
        unpack_profiles_request_(request.profile_request()),
        user_profile);
      response.set_found(found);
      pack_user_profiles_(user_profile, *response.mutable_user_profile());
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManagerCore::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::Exception& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::match(
    const adserver::user_info_svcs::user_info_manager::MatchRequest& request,
    adserver::user_info_svcs::user_info_manager::MatchResponse& response,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    InProgressGuard in_progress(stats_counters_->match_in_progress);
    response.set_hostname(service_hostname_());
    try
    {
      UserInfoManagerCore::MatchResult match_result;
      const bool matched = user_info_manager_->match(
        unpack_user_info_(request.user_info()),
        unpack_match_params_(request.match_params()),
        match_result);
      response.set_matched(matched);
      convert_(match_result, *response.mutable_match_result());
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManagerCore::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::Exception& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::update_user_freq_caps(
    const adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsRequest& request,
    adserver::user_info_svcs::user_info_manager::UpdateUserFreqCapsResponse& response,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    InProgressGuard in_progress(
      stats_counters_->update_user_freq_caps_in_progress);
    response.set_hostname(service_hostname_());
    try
    {
      user_info_manager_->update_user_freq_caps(
        unpack_user_id_(request.user_id()),
        unpack_time_(request.time()),
        unpack_request_id_(request.request_id()),
        unpack_ids_(request.freq_caps()),
        unpack_ids_(request.uc_freq_caps()),
        unpack_ids_(request.virtual_freq_caps()),
        unpack_seq_orders_(request.seq_orders()),
        unpack_ids_(request.campaign_ids()),
        unpack_ids_(request.uc_campaign_ids()));
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManagerCore::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::Exception& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::confirm_user_freq_caps(
    const adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsRequest& request,
    adserver::user_info_svcs::user_info_manager::ConfirmUserFreqCapsResponse& response,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    InProgressGuard in_progress(
      stats_counters_->confirm_user_freq_caps_in_progress);
    response.set_hostname(service_hostname_());
    try
    {
      user_info_manager_->confirm_user_freq_caps(
        unpack_user_id_(request.user_id()),
        unpack_time_(request.time()),
        unpack_request_id_(request.request_id()),
        unpack_id_set_(request.exclude_pubpixel_accounts()));
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManagerCore::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::Exception& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::fraud_user(
    const adserver::user_info_svcs::user_info_manager::FraudUserRequest& request,
    adserver::user_info_svcs::user_info_manager::FraudUserResponse& response,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    InProgressGuard in_progress(
      stats_counters_->fraud_user_in_progress);
    try
    {
      response.set_fraud(user_info_manager_->fraud_user(
        unpack_user_id_(request.user_id()),
        unpack_time_(request.time())));
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManagerCore::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::Exception& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::remove_user_profile(
    const adserver::user_info_svcs::user_info_manager::RemoveUserProfileRequest& request,
    adserver::user_info_svcs::user_info_manager::RemoveUserProfileResponse& response,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    InProgressGuard in_progress(
      stats_counters_->remove_user_profile_in_progress);
    try
    {
      response.set_removed(user_info_manager_->remove_user_profile(
        unpack_user_id_(request.user_id())));
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManagerCore::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::Exception& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::merge(
    const adserver::user_info_svcs::user_info_manager::MergeRequest& request,
    adserver::user_info_svcs::user_info_manager::MergeResponse& response,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    InProgressGuard in_progress(stats_counters_->merge_in_progress);
    try
    {
      bool merge_success = false;
      Generics::Time last_request;
      response.set_result(user_info_manager_->merge(
        unpack_user_info_(request.user_info()),
        unpack_match_params_(request.match_params()),
        unpack_user_profiles_(request.merge_user_profile()),
        merge_success,
        last_request));
      response.set_merge_success(merge_success);
      response.set_last_request(pack_time_(last_request));
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManagerCore::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::Exception& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::consider_publishers_optin(
    const adserver::user_info_svcs::user_info_manager::ConsiderPublishersOptinRequest& request,
    adserver::user_info_svcs::user_info_manager::ConsiderPublishersOptinResponse&,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    InProgressGuard in_progress(
      stats_counters_->consider_publishers_optin_in_progress);
    try
    {
      user_info_manager_->consider_publishers_optin(
        unpack_user_id_(request.user_id()),
        unpack_id_set_(request.exclude_pubpixel_accounts()),
        unpack_time_(request.now()));
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManagerCore::NotReady& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::ChunkNotFound& ex) { result_status = to_status_(ex); }
    catch(const UserInfoManagerCore::Exception& ex) { result_status = to_status_(ex); }
  }

  void UserInfoManagerGrpc::ServiceImpl::uim_ready(
    const adserver::user_info_svcs::user_info_manager::UimReadyRequest&,
    adserver::user_info_svcs::user_info_manager::UimReadyResponse& response,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    response.set_ready(user_info_manager_->uim_ready());
    result_status = grpc::Status::OK;
  }

  void UserInfoManagerGrpc::ServiceImpl::get_progress(
    const adserver::user_info_svcs::user_info_manager::GetProgressRequest&,
    adserver::user_info_svcs::user_info_manager::GetProgressResponse& response,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    response.set_progress(user_info_manager_->get_progress());
    result_status = grpc::Status::OK;
  }

  void UserInfoManagerGrpc::ServiceImpl::clear_expired(
    const adserver::user_info_svcs::user_info_manager::ClearExpiredRequest& request,
    adserver::user_info_svcs::user_info_manager::ClearExpiredResponse&,
    grpc::Status& result_status) const
  {
    InProgressGuard call_in_progress(stats_counters_->call_in_progress);
    try
    {
      user_info_manager_->clear_expired(
        request.sync(),
        unpack_time_(request.cleanup_time()),
        request.portion());
      result_status = grpc::Status::OK;
    }
    catch(const UserInfoManagerCore::Exception& ex)
    {
      result_status = to_status_(ex);
    }
  }

  UserInfoManagerGrpc::UserInfoManagerGrpc(
    UserInfoManagerCorePtr user_info_manager,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      stats_counters_(std::make_shared<StatsCounters>()),
      impl_(std::make_shared<Impl>(
        logger,
        user_info_manager_grpc_aspect,
        bind_address_,
        std::make_unique<ServiceImpl>(
          std::move(user_info_manager),
          stats_counters_)))
  {
    add_child_object(impl_);
  }

  UserInfoManagerGrpc::Stats
  UserInfoManagerGrpc::stats() const noexcept
  {
    return Stats{
      stats_counters_->call_in_progress.load(std::memory_order_relaxed),
      stats_counters_->match_in_progress.load(std::memory_order_relaxed),
      stats_counters_->update_user_freq_caps_in_progress.load(std::memory_order_relaxed),
      stats_counters_->confirm_user_freq_caps_in_progress.load(std::memory_order_relaxed),
      stats_counters_->fraud_user_in_progress.load(std::memory_order_relaxed),
      stats_counters_->remove_user_profile_in_progress.load(std::memory_order_relaxed),
      stats_counters_->merge_in_progress.load(std::memory_order_relaxed),
      stats_counters_->consider_publishers_optin_in_progress.load(std::memory_order_relaxed)
    };
  }

  UserInfoManagerGrpc::~UserInfoManagerGrpc() noexcept = default;
}
