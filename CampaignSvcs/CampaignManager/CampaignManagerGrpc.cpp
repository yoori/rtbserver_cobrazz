#include "CampaignManagerGrpc.hpp"

#include <grpcpp/grpcpp.h>

#include <string>
#include <utility>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include <CampaignSvcs/CampaignManager/CampaignManagerGrpc.grpc.pb.h>

namespace AdServer::CampaignSvcs
{
  namespace
  {
    constexpr const char campaign_manager_grpc_aspect[] =
      "CampaignManagerGrpc";

    namespace pb = adserver::campaign_svcs::campaign_manager;

    CORBACommons::OctSeq
    unpack_oct_seq(const std::string& source)
    {
      CORBACommons::OctSeq result;
      result.length(source.size());
      for(CORBA::ULong i = 0; i < source.size(); ++i)
      {
        result[i] = static_cast<CORBA::Octet>(source[i]);
      }
      return result;
    }

    Generics::Time
    unpack_time(const std::string& source)
    {
      Generics::Time result;
      if(source.size() != Generics::Time::TIME_PACK_LEN)
      {
        Stream::Error ostr;
        ostr << "Invalid packed time size: " << source.size();
        throw CampaignManagerCore::Exception(ostr);
      }

      result.unpack(reinterpret_cast<const unsigned char*>(source.data()));
      return result;
    }

    void
    pack_ids(
      const CampaignManagerCore::IdVector& source,
      google::protobuf::RepeatedField<google::protobuf::uint64>* target)
    {
      for(const auto id : source)
      {
        target->Add(id);
      }
    }

    CampaignManagerCore::IdVector
    unpack_ids(
      const google::protobuf::RepeatedField<google::protobuf::uint64>& source)
    {
      CampaignManagerCore::IdVector result;
      result.reserve(source.size());
      for(const auto id : source)
      {
        result.push_back(id);
      }
      return result;
    }

    CampaignManagerCore::GeoInfo
    unpack_geo_info(const pb::GeoInfo& source)
    {
      return {source.country(), source.region(), source.city()};
    }

    CampaignManagerCore::GeoCoordInfo
    unpack_geo_coord_info(const pb::GeoCoordInfo& source)
    {
      return {
        CorbaAlgs::unpack_decimal<CoordDecimal>(
          unpack_oct_seq(source.longitude())),
        CorbaAlgs::unpack_decimal<CoordDecimal>(
          unpack_oct_seq(source.latitude())),
        CorbaAlgs::unpack_decimal<CoordDecimal>(
          unpack_oct_seq(source.accuracy()))};
    }

    void
    unpack_geo_info_seq(
      const google::protobuf::RepeatedPtrField<pb::GeoInfo>& source,
      std::vector<CampaignManagerCore::GeoInfo>& target)
    {
      target.clear();
      target.reserve(source.size());
      for(const auto& item : source)
      {
        target.push_back(unpack_geo_info(item));
      }
    }

    void
    unpack_geo_coord_info_seq(
      const google::protobuf::RepeatedPtrField<pb::GeoCoordInfo>& source,
      std::vector<CampaignManagerCore::GeoCoordInfo>& target)
    {
      target.clear();
      target.reserve(source.size());
      for(const auto& item : source)
      {
        target.push_back(unpack_geo_coord_info(item));
      }
    }

    void
    pack_channel_search_result(
      const CampaignManagerCore::ChannelSearchResult& source,
      pb::ChannelSearchResult& target)
    {
      target.set_channel_id(source.channel_id);
      target.set_use_count(source.use_count);
      pack_ids(
        source.matched_simple_channels,
        target.mutable_matched_simple_channels());
      pack_ids(source.ccg_ids, target.mutable_ccg_ids());
      target.set_discover_query(source.discover_query);
      target.set_language(source.language);
    }

    void
    pack_discover_channel_result(
      const CampaignManagerCore::DiscoverChannelResult& source,
      pb::DiscoverChannelResult& target)
    {
      target.set_channel_id(source.channel_id);
      target.set_name(source.name);
      target.set_query(source.query);
      target.set_annotation(source.annotation);
      target.set_weight(source.weight);
      pack_ids(source.categories, target.mutable_categories());
      target.set_country_code(source.country_code);
      target.set_language(source.language);
    }

    void
    pack_category_channel_node(
      const CampaignManagerCore::CategoryChannelNodeInfo& source,
      pb::CategoryChannelNode& target)
    {
      target.set_channel_id(source.channel_id);
      target.set_name(source.name);
      target.set_flags(source.flags);

      for(const auto& child : source.child_category_channels)
      {
        pack_category_channel_node(child, *target.add_child_category_channels());
      }
    }
  }

  class CampaignManagerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      CampaignManagerGrpc::ServiceImpl,
      pb::CampaignManagerGrpc,
      pb::CampaignManagerGrpc::AsyncService>
  {
    using AsyncService = pb::CampaignManagerGrpc::AsyncService;

  public:
    explicit ServiceImpl(CampaignManagerCore* core);

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_GRPC_CALL(pb::ReadyRequest, pb::ReadyResponse, ready),
        MAKE_GRPC_CALL(
          pb::ProgressCommentRequest,
          pb::ProgressCommentResponse,
          progress_comment),
        MAKE_GRPC_CALL(
          pb::MatchGeoChannelsRequest,
          pb::MatchGeoChannelsResponse,
          match_geo_channels),
        MAKE_GRPC_CALL(pb::GetFileRequest, pb::GetFileResponse, get_file),
        MAKE_GRPC_CALL(
          pb::TraceCampaignSelectionIndexRequest,
          pb::TraceCampaignSelectionIndexResponse,
          trace_campaign_selection_index),
        MAKE_GRPC_CALL(
          pb::GetCampaignCreativeByCcidRequest,
          pb::GetCampaignCreativeByCcidResponse,
          get_campaign_creative_by_ccid),
        MAKE_GRPC_CALL(
          pb::GetChannelLinksRequest,
          pb::GetChannelLinksResponse,
          get_channel_links),
        MAKE_GRPC_CALL(
          pb::GetDiscoverChannelsRequest,
          pb::GetDiscoverChannelsResponse,
          get_discover_channels),
        MAKE_GRPC_CALL(
          pb::GetCategoryChannelsRequest,
          pb::GetCategoryChannelsResponse,
          get_category_channels),
        MAKE_GRPC_CALL(
          pb::GetColocationFlagsRequest,
          pb::GetColocationFlagsResponse,
          get_colocation_flags),
        MAKE_GRPC_CALL(
          pb::GetPubPixelsRequest,
          pb::GetPubPixelsResponse,
          get_pub_pixels),
        MAKE_GRPC_CALL(
          pb::ConsiderPassbackRequest,
          pb::ConsiderPassbackResponse,
          consider_passback),
        MAKE_GRPC_CALL(
          pb::ConsiderPassbackTrackRequest,
          pb::ConsiderPassbackTrackResponse,
          consider_passback_track),
        MAKE_GRPC_CALL(
          pb::VerifyOptOperationRequest,
          pb::VerifyOptOperationResponse,
          verify_opt_operation),
        MAKE_GRPC_CALL(
          pb::ConsiderWebOperationRequest,
          pb::ConsiderWebOperationResponse,
          consider_web_operation));
    }

    void ready(
      const pb::ReadyRequest& request,
      pb::ReadyResponse& response,
      ::grpc::Status& result_status) const;

    void progress_comment(
      const pb::ProgressCommentRequest& request,
      pb::ProgressCommentResponse& response,
      ::grpc::Status& result_status) const;

    void match_geo_channels(
      const pb::MatchGeoChannelsRequest& request,
      pb::MatchGeoChannelsResponse& response,
      ::grpc::Status& result_status) const;

    void get_file(
      const pb::GetFileRequest& request,
      pb::GetFileResponse& response,
      ::grpc::Status& result_status) const;

    void trace_campaign_selection_index(
      const pb::TraceCampaignSelectionIndexRequest& request,
      pb::TraceCampaignSelectionIndexResponse& response,
      ::grpc::Status& result_status) const;

    void get_campaign_creative_by_ccid(
      const pb::GetCampaignCreativeByCcidRequest& request,
      pb::GetCampaignCreativeByCcidResponse& response,
      ::grpc::Status& result_status) const;

    void get_channel_links(
      const pb::GetChannelLinksRequest& request,
      pb::GetChannelLinksResponse& response,
      ::grpc::Status& result_status) const;

    void get_discover_channels(
      const pb::GetDiscoverChannelsRequest& request,
      pb::GetDiscoverChannelsResponse& response,
      ::grpc::Status& result_status) const;

    void get_category_channels(
      const pb::GetCategoryChannelsRequest& request,
      pb::GetCategoryChannelsResponse& response,
      ::grpc::Status& result_status) const;

    void get_colocation_flags(
      const pb::GetColocationFlagsRequest& request,
      pb::GetColocationFlagsResponse& response,
      ::grpc::Status& result_status) const;

    void get_pub_pixels(
      const pb::GetPubPixelsRequest& request,
      pb::GetPubPixelsResponse& response,
      ::grpc::Status& result_status) const;

    void consider_passback(
      const pb::ConsiderPassbackRequest& request,
      pb::ConsiderPassbackResponse& response,
      ::grpc::Status& result_status) const;

    void consider_passback_track(
      const pb::ConsiderPassbackTrackRequest& request,
      pb::ConsiderPassbackTrackResponse& response,
      ::grpc::Status& result_status) const;

    void verify_opt_operation(
      const pb::VerifyOptOperationRequest& request,
      pb::VerifyOptOperationResponse& response,
      ::grpc::Status& result_status) const;

    void consider_web_operation(
      const pb::ConsiderWebOperationRequest& request,
      pb::ConsiderWebOperationResponse& response,
      ::grpc::Status& result_status) const;

  private:
    CampaignManagerCore_var core_;
  };

  CampaignManagerGrpc::ServiceImpl::ServiceImpl(CampaignManagerCore* core)
    : core_(ReferenceCounting::add_ref(core))
  {}

  void
  CampaignManagerGrpc::ServiceImpl::ready(
    const pb::ReadyRequest&,
    pb::ReadyResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      response.set_ready(core_->ready());
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::progress_comment(
    const pb::ProgressCommentRequest&,
    pb::ProgressCommentResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      std::string comment;
      core_->progress_comment(comment);
      response.set_comment(std::move(comment));
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::match_geo_channels(
    const pb::MatchGeoChannelsRequest& request,
    pb::MatchGeoChannelsResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      std::vector<CampaignManagerCore::GeoInfo> location;
      std::vector<CampaignManagerCore::GeoCoordInfo> coord_location;
      unpack_geo_info_seq(request.location(), location);
      unpack_geo_coord_info_seq(request.coord_location(), coord_location);

      CampaignManagerCore::IdVector geo_channels;
      CampaignManagerCore::IdVector coord_channels;
      core_->match_geo_channels(
        location,
        coord_location,
        geo_channels,
        coord_channels);

      pack_ids(geo_channels, response.mutable_geo_channels());
      pack_ids(coord_channels, response.mutable_coord_channels());
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_file(
    const pb::GetFileRequest& request,
    pb::GetFileResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      const auto file = core_->get_file(request.file_name());
      response.set_file(file.data(), file.size());
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::trace_campaign_selection_index(
    const pb::TraceCampaignSelectionIndexRequest&,
    pb::TraceCampaignSelectionIndexResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      response.set_trace_xml(core_->trace_campaign_selection_index());
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_campaign_creative_by_ccid(
    const pb::GetCampaignCreativeByCcidRequest& request,
    pb::GetCampaignCreativeByCcidResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      CampaignManagerCore::PreviewCreativeParams params;
      params.ccid = request.ccid();
      params.tag_id = request.tag_id();
      params.format = request.format();
      params.original_url = request.original_url();
      params.peer_ip = request.peer_ip();

      std::string creative_body;
      response.set_found(
        core_->get_campaign_creative_by_ccid(params, creative_body));
      response.set_creative_body(std::move(creative_body));
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_channel_links(
    const pb::GetChannelLinksRequest& request,
    pb::GetChannelLinksResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      const auto result = core_->get_channel_links(
        unpack_ids(request.channels()),
        request.match());

      for(const auto& channel : result)
      {
        pack_channel_search_result(channel, *response.add_channels());
      }

      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_discover_channels(
    const pb::GetDiscoverChannelsRequest& request,
    pb::GetDiscoverChannelsResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      std::vector<CampaignManagerCore::ChannelWeight> channels;
      channels.reserve(request.channels_size());
      for(const auto& channel : request.channels())
      {
        channels.push_back({channel.channel_id(), channel.weight()});
      }

      const auto result = core_->get_discover_channels(
        channels,
        request.country(),
        request.language(),
        request.all());

      for(const auto& channel : result)
      {
        pack_discover_channel_result(channel, *response.add_channels());
      }

      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_category_channels(
    const pb::GetCategoryChannelsRequest& request,
    pb::GetCategoryChannelsResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      const auto result = core_->get_category_channels(request.language());
      for(const auto& channel : result)
      {
        pack_category_channel_node(channel, *response.add_channels());
      }

      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_colocation_flags(
    const pb::GetColocationFlagsRequest&,
    pb::GetColocationFlagsResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      const auto result = core_->get_colocation_flags();
      for(const auto& colocation : result)
      {
        auto* target = response.add_colocations();
        target->set_colo_id(colocation.colo_id);
        target->set_flags(colocation.flags);
        target->set_hid_profile(colocation.hid_profile);
      }

      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::get_pub_pixels(
    const pb::GetPubPixelsRequest& request,
    pb::GetPubPixelsResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      const auto result = core_->get_pub_pixels(
        request.country(),
        request.user_status(),
        unpack_ids(request.publisher_account_ids()));

      for(const auto& pixel : result)
      {
        response.add_pixels(pixel);
      }

      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::consider_passback(
    const pb::ConsiderPassbackRequest& request,
    pb::ConsiderPassbackResponse&,
    ::grpc::Status& result_status) const
  {
    try
    {
      CampaignManagerCore::PassbackInfo info;
      info.request_id = CorbaAlgs::unpack_request_id(
        unpack_oct_seq(request.request_id()));
      info.time = unpack_time(request.time());
      if(request.has_user_id_hash_mod() &&
        request.user_id_hash_mod().defined())
      {
        info.user_id_hash_mod = AdServer::Commons::Optional<unsigned long>(
          request.user_id_hash_mod().value());
      }

      core_->consider_passback(info);
      result_status = ::grpc::Status::OK;
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::consider_passback_track(
    const pb::ConsiderPassbackTrackRequest& request,
    pb::ConsiderPassbackTrackResponse&,
    ::grpc::Status& result_status) const
  {
    try
    {
      CampaignManagerCore::PassbackTrackInfo info;
      info.time = unpack_time(request.time());
      info.country = request.country();
      info.colo_id = request.colo_id();
      info.tag_id = request.tag_id();
      info.user_status = request.user_status();

      core_->consider_passback_track(info);
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::verify_opt_operation(
    const pb::VerifyOptOperationRequest& request,
    pb::VerifyOptOperationResponse&,
    ::grpc::Status& result_status) const
  {
    try
    {
      CampaignManagerCore::OptOperation operation =
        CampaignManagerCore::OptOperation::STATUS;
      switch(request.operation())
      {
      case pb::OPT_OPERATION_IN:
        operation = CampaignManagerCore::OptOperation::IN;
        break;
      case pb::OPT_OPERATION_OUT:
        operation = CampaignManagerCore::OptOperation::OUT;
        break;
      case pb::OPT_OPERATION_FORCED_IN:
        operation = CampaignManagerCore::OptOperation::FORCED_IN;
        break;
      case pb::OPT_OPERATION_STATUS:
      default:
        operation = CampaignManagerCore::OptOperation::STATUS;
        break;
      }

      core_->verify_opt_operation(
        request.time(),
        request.colo_id(),
        request.referer(),
        operation,
        request.status(),
        request.user_status(),
        request.log_as_test(),
        request.browser(),
        request.os(),
        request.ct(),
        request.curct(),
        CorbaAlgs::unpack_user_id(unpack_oct_seq(request.user_id())));
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  CampaignManagerGrpc::ServiceImpl::consider_web_operation(
    const pb::ConsiderWebOperationRequest& request,
    pb::ConsiderWebOperationResponse&,
    ::grpc::Status& result_status) const
  {
    try
    {
      CampaignManagerCore::WebOperationInfo info;
      info.time = unpack_time(request.time());
      info.colo_id = request.colo_id();
      info.tag_id = request.tag_id();
      info.cc_id = request.cc_id();
      info.ct = request.ct();
      info.curct = request.curct();
      info.browser = request.browser();
      info.os = request.os();
      info.app = request.app();
      info.source = request.source();
      info.operation = request.operation();
      info.user_bind_src = request.user_bind_src();
      info.result = static_cast<char>(request.result());
      info.user_status = request.user_status();
      info.test_request = request.test_request();
      info.request_ids.reserve(request.request_ids_size());
      for(const auto& request_id : request.request_ids())
      {
        info.request_ids.emplace_back(
          CorbaAlgs::unpack_request_id(unpack_oct_seq(request_id)));
      }
      info.global_request_id = CorbaAlgs::unpack_request_id(
        unpack_oct_seq(request.global_request_id()));
      info.referer = request.referer();
      info.ip_address = request.ip_address();
      info.external_user_id = request.external_user_id();
      info.user_agent = request.user_agent();

      core_->consider_web_operation(info);
      result_status = ::grpc::Status::OK;
    }
    catch(const CampaignManagerCore::InvalidArgument&)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INVALID_ARGUMENT,
        "incorrect argument");
    }
    catch(const CampaignManagerCore::NotReady& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch(const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  CampaignManagerGrpc::CampaignManagerGrpc(
    CampaignManagerCore* core,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(std::make_shared<Impl>(
        logger,
        campaign_manager_grpc_aspect,
        bind_address_,
        std::make_unique<ServiceImpl>(core)))
  {
    add_child_object(impl_);
  }

  CampaignManagerGrpc::~CampaignManagerGrpc() noexcept = default;
}
