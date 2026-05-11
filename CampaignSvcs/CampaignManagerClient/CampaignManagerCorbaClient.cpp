#include "CampaignManagerCorbaClient.hpp"

#include <algorithm>
#include <map>

#include <grpcpp/support/status.h>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBACommons/ObjectPool.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>

namespace AdServer::CampaignSvcs
{
  namespace
  {
    namespace pb = adserver::campaign_svcs::campaign_manager;
    namespace CorbaCampaign = AdServer::CampaignSvcs_v360;
    using ChannelIdSeq = CorbaCampaign::ChannelIdSeq;
    using ChannelIdSeq_var = CorbaCampaign::ChannelIdSeq_var;
    using ChannelWeightSeq = CorbaCampaign::ChannelWeightSeq;
    using ColocationFlagsSeq_var = CorbaCampaign::ColocationFlagsSeq_var;
    using PublisherAccountIdSeq = CorbaCampaign::PublisherAccountIdSeq;
    using StringSeq_var = CorbaCampaign::StringSeq_var;

    template<typename CorbaOctSeq>
    std::string pack_oct_seq(const CorbaOctSeq& seq)
    {
      return std::string(
        reinterpret_cast<const char*>(seq.get_buffer()),
        reinterpret_cast<const char*>(seq.get_buffer()) + seq.length());
    }

    template<typename CorbaOctSeq>
    void unpack_oct_seq(
      const std::string& value,
      CorbaOctSeq& target)
    {
      target.length(value.size());
      std::copy(value.begin(), value.end(), target.get_buffer());
    }

    grpc::Status exception_status(
      grpc::StatusCode code,
      const char* description)
    {
      return grpc::Status(code, description ? description : "");
    }

    grpc::Status exception_status(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << ex;
      return grpc::Status(grpc::StatusCode::UNAVAILABLE, ostr.str().str());
    }

    grpc::Status exception_status(const eh::Exception& ex)
    {
      return grpc::Status(grpc::StatusCode::INTERNAL, ex.what());
    }

    void unpack_geo_seq(
      const google::protobuf::RepeatedPtrField<pb::GeoInfo>& source,
      CorbaCampaign::CampaignManager::GeoInfoSeq& target)
    {
      target.length(source.size());
      for(int i = 0; i < source.size(); ++i)
      {
        target[i].country = source[i].country().c_str();
        target[i].region = source[i].region().c_str();
        target[i].city = source[i].city().c_str();
      }
    }

    void unpack_geo_coord_seq(
      const google::protobuf::RepeatedPtrField<pb::GeoCoordInfo>& source,
      CorbaCampaign::CampaignManager::GeoCoordInfoSeq& target)
    {
      target.length(source.size());
      for(int i = 0; i < source.size(); ++i)
      {
        unpack_oct_seq(source[i].longitude(), target[i].longitude);
        unpack_oct_seq(source[i].latitude(), target[i].latitude);
        unpack_oct_seq(source[i].accuracy(), target[i].accuracy);
      }
    }

    template<typename CorbaIdSeq>
    void pack_ids(
      const CorbaIdSeq& source,
      google::protobuf::RepeatedField<google::protobuf::uint64>* target)
    {
      for(CORBA::ULong i = 0; i < source.length(); ++i)
      {
        target->Add(source[i]);
      }
    }

    template<typename CorbaIdSeq>
    void unpack_ids(
      const google::protobuf::RepeatedField<google::protobuf::uint64>& source,
      CorbaIdSeq& target)
    {
      target.length(source.size());
      for(int i = 0; i < source.size(); ++i)
      {
        target[i] = source[i];
      }
    }

    void pack_category_channel_node(
      const CorbaCampaign::CampaignManager::CategoryChannelNodeInfo& source,
      pb::CategoryChannelNode& target)
    {
      target.set_channel_id(source.channel_id);
      target.set_name(source.name.in());
      target.set_flags(source.flags);
      for(CORBA::ULong i = 0;
        i < source.child_category_channels.length();
        ++i)
      {
        pack_category_channel_node(
          source.child_category_channels[i],
          *target.add_child_category_channels());
      }
    }
  }

  class CampaignManagerCorbaClient::Impl
  {
  public:
    explicit Impl(const CampaignManagerRefs& campaign_manager_refs)
      /*throw(Exception)*/;

    CorbaCampaign::CampaignManager_var get_manager(
      const std::string& service_index = std::string())
      /*throw(Exception)*/;

  private:
    struct PoolConfig:
      public CORBACommons::ObjectPoolRefConfiguration
    {
      explicit PoolConfig(
        const CORBACommons::CorbaClientAdapter* corba_client_adapter) noexcept
        : ObjectPoolRefConfiguration(corba_client_adapter),
          resolver(corba_client_adapter)
      {}

      struct Resolver
      {
        explicit Resolver(
          const CORBACommons::CorbaClientAdapter* corba_client_adapter)
          noexcept;

        template<typename PoolType>
        PoolType* resolve(const ObjectRef& ref)
          /*throw(Exception)*/;

      private:
        CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
      };

      Resolver resolver;
    };

    using PoolImpl = CORBACommons::ObjectPool<
      CorbaCampaign::CampaignManager,
      PoolConfig>;

    using PoolImplPtr = std::unique_ptr<PoolImpl>;

  public:
    PoolImpl::ObjectHandlerType get_manager_handler(
      const std::string& service_index = std::string())
      /*throw(Exception)*/;

  private:
    CORBACommons::CorbaClientAdapter_var corba_client_adapter_;
    PoolImplPtr pool_;
    std::map<std::string, unsigned> service_index_pool_key_map_;
  };

  CampaignManagerCorbaClient::Impl::PoolConfig::Resolver::Resolver(
    const CORBACommons::CorbaClientAdapter* corba_client_adapter)
    noexcept
    : corba_client_adapter_(ReferenceCounting::add_ref(corba_client_adapter))
  {}

  template<typename PoolType>
  PoolType*
  CampaignManagerCorbaClient::Impl::PoolConfig::Resolver::resolve(
    const ObjectRef& ref)
    /*throw(Exception)*/
  {
    static const char* FUN =
      "CampaignManagerCorbaClient::PoolConfig::Resolver::resolve()";

    try
    {
      CORBA::Object_var obj = corba_client_adapter_->resolve_object(ref);

      CorbaCampaign::CampaignManager_var campaign_manager =
        CorbaCampaign::CampaignManager::_narrow(obj);
      if(CORBA::is_nil(campaign_manager))
      {
        return nullptr;
      }

      return campaign_manager._retn();
    }
    catch(const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": failed to resolve CampaignManager reference '" <<
        ref.object_ref << "'. CORBA::Exception caught: " << ex;
      throw Exception(ostr);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": failed to resolve CampaignManager reference '" <<
        ref.object_ref << "'. eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  CampaignManagerCorbaClient::Impl::Impl(
    const CampaignManagerRefs& campaign_manager_refs)
    /*throw(Exception)*/
    : corba_client_adapter_(new CORBACommons::CorbaClientAdapter())
  {
    static const char* FUN = "CampaignManagerCorbaClient::Impl::Impl()";

    try
    {
      if(campaign_manager_refs.empty())
      {
        throw Exception("CampaignManager refs list is empty");
      }

      PoolConfig pool_config(corba_client_adapter_);
      unsigned pool_key = 0;
      for(const auto& ref : campaign_manager_refs)
      {
        pool_config.iors_list.emplace_back(ref.object_ref.c_str());
        if(!ref.service_index.empty())
        {
          service_index_pool_key_map_.emplace(ref.service_index, pool_key);
        }
        ++pool_key;
      }
      pool_config.timeout = Generics::Time(10);

      pool_.reset(new PoolImpl(
        pool_config,
        CORBACommons::ChoosePolicyType::PT_PRECISE));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  CorbaCampaign::CampaignManager_var
  CampaignManagerCorbaClient::Impl::get_manager(
    const std::string& service_index)
    /*throw(Exception)*/
  {
    const auto it = service_index_pool_key_map_.find(service_index);
    const unsigned pool_key = it != service_index_pool_key_map_.end() ?
      it->second : PoolImpl::SPECIAL_KEY;

    auto handler = pool_->get_object<Exception>(pool_key);
    return CorbaCampaign::CampaignManager::_duplicate(handler.operator->());
  }

  CampaignManagerCorbaClient::Impl::PoolImpl::ObjectHandlerType
  CampaignManagerCorbaClient::Impl::get_manager_handler(
    const std::string& service_index)
    /*throw(Exception)*/
  {
    const auto it = service_index_pool_key_map_.find(service_index);
    const unsigned pool_key = it != service_index_pool_key_map_.end() ?
      it->second : PoolImpl::SPECIAL_KEY;

    return pool_->get_object<Exception>(pool_key);
  }

  CampaignManagerCorbaClient::CampaignManagerCorbaClient(
    const CampaignManagerRefs& campaign_manager_refs)
    /*throw(Exception)*/
    : impl_(std::make_unique<Impl>(campaign_manager_refs))
  {}

  CampaignManagerCorbaClient::CampaignManagerCorbaClient(
    const CampaignManagerObjectRefs& campaign_manager_refs)
    /*throw(Exception)*/
    : impl_(std::make_unique<Impl>([&campaign_manager_refs]()
      {
        CampaignManagerRefs result;
        result.reserve(campaign_manager_refs.size());
        for(const auto& object_ref : campaign_manager_refs)
        {
          result.push_back({object_ref, std::string()});
        }
        return result;
      }()))
  {}

  CampaignManagerCorbaClient::~CampaignManagerCorbaClient() noexcept = default;

  void
  CampaignManagerCorbaClient::ready(
    const pb::ReadyRequest&,
    ReadyCallback callback)
  {
    pb::ReadyResponse response;
    try
    {
      CorbaCampaign::CampaignManager_var manager =
        CorbaCampaign::CampaignManager::_duplicate(impl_->get_manager());
      response.set_ready(!manager->_non_existent());
      callback(grpc::Status::OK, response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::progress_comment(
    const pb::ProgressCommentRequest&,
    ProgressCommentCallback callback)
  {
    pb::ProgressCommentResponse response;
    callback(grpc::Status::OK, response);
  }

  void
  CampaignManagerCorbaClient::match_geo_channels(
    const pb::MatchGeoChannelsRequest& request,
    MatchGeoChannelsCallback callback)
  {
    pb::MatchGeoChannelsResponse response;
    try
    {
      CorbaCampaign::CampaignManager::GeoInfoSeq location;
      CorbaCampaign::CampaignManager::GeoCoordInfoSeq coord_location;
      unpack_geo_seq(request.location(), location);
      unpack_geo_coord_seq(request.coord_location(), coord_location);

      ChannelIdSeq_var geo_channels;
      ChannelIdSeq_var coord_channels;
      impl_->get_manager()->match_geo_channels(
        location,
        coord_location,
        geo_channels.out(),
        coord_channels.out());

      pack_ids(geo_channels.in(), response.mutable_geo_channels());
      pack_ids(coord_channels.in(), response.mutable_coord_channels());
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::NotReady& ex)
    {
      callback(
        exception_status(grpc::StatusCode::UNAVAILABLE, ex.description.in()),
        response);
    }
    catch(const CorbaCampaign::CampaignManager::ImplementationException& ex)
    {
      callback(
        exception_status(grpc::StatusCode::INTERNAL, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::get_file(
    const pb::GetFileRequest& request,
    GetFileCallback callback)
  {
    pb::GetFileResponse response;
    std::string service_index = request.service_index();
    for(;;)
    {
      auto manager = impl_->get_manager_handler(service_index);
      try
      {
        CORBACommons::OctSeq_var file;
        manager->get_file(request.file_name().c_str(), file.out());
        response.set_file(
          reinterpret_cast<const char*>(file->get_buffer()),
          file->length());
        callback(grpc::Status::OK, response);
        return;
      }
      catch(const CorbaCampaign::CampaignManager::ImplementationException& ex)
      {
        callback(
          exception_status(grpc::StatusCode::INTERNAL, ex.description.in()),
          response);
        return;
      }
      catch(const CORBA::SystemException& ex)
      {
        Stream::Error ostr;
        ostr << ex;
        manager.release_bad(ostr.str());
        if(service_index.empty())
        {
          callback(exception_status(ex), response);
          return;
        }
        service_index.clear();
      }
      catch(const eh::Exception& ex)
      {
        callback(exception_status(ex), response);
        return;
      }
    }
  }

  void
  CampaignManagerCorbaClient::trace_campaign_selection_index(
    const pb::TraceCampaignSelectionIndexRequest&,
    TraceCampaignSelectionIndexCallback callback)
  {
    pb::TraceCampaignSelectionIndexResponse response;
    try
    {
      CORBA::String_var trace_xml;
      impl_->get_manager()->trace_campaign_selection_index(trace_xml.out());
      response.set_trace_xml(trace_xml.in());
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::ImplementationException& ex)
    {
      callback(
        exception_status(grpc::StatusCode::INTERNAL, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::get_campaign_creative_by_ccid(
    const pb::GetCampaignCreativeByCcidRequest& request,
    GetCampaignCreativeByCcidCallback callback)
  {
    pb::GetCampaignCreativeByCcidResponse response;
    try
    {
      CorbaCampaign::CampaignManager::CreativeParams params;
      params.ccid = request.ccid();
      params.tag_id = request.tag_id();
      params.format = request.format().c_str();
      params.original_url = request.original_url().c_str();
      params.peer_ip = request.peer_ip().c_str();

      CORBA::String_var creative_body;
      response.set_found(impl_->get_manager()->get_campaign_creative_by_ccid(
        params,
        creative_body.out()));
      response.set_creative_body(creative_body.in());
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::ImplementationException& ex)
    {
      callback(
        exception_status(grpc::StatusCode::INTERNAL, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::get_channel_links(
    const pb::GetChannelLinksRequest& request,
    GetChannelLinksCallback callback)
  {
    pb::GetChannelLinksResponse response;
    try
    {
      ChannelIdSeq ids;
      unpack_ids(request.channels(), ids);

      CorbaCampaign::CampaignManager::ChannelSearchResultSeq_var result =
        impl_->get_manager()->get_channel_links(ids, request.match());

      for(CORBA::ULong i = 0; i < result->length(); ++i)
      {
        const auto& source = (*result)[i];
        auto* target = response.add_channels();
        target->set_channel_id(source.channel_id);
        target->set_use_count(source.use_count);
        pack_ids(
          source.matched_simple_channels,
          target->mutable_matched_simple_channels());
        pack_ids(source.ccg_ids, target->mutable_ccg_ids());
        target->set_discover_query(source.discover_query.in());
        target->set_language(source.language.in());
      }
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::ImplementationException& ex)
    {
      callback(
        exception_status(grpc::StatusCode::INTERNAL, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::get_discover_channels(
    const pb::GetDiscoverChannelsRequest& request,
    GetDiscoverChannelsCallback callback)
  {
    pb::GetDiscoverChannelsResponse response;
    try
    {
      ChannelWeightSeq channels;
      channels.length(request.channels_size());
      for(int i = 0; i < request.channels_size(); ++i)
      {
        channels[i].channel_id = request.channels(i).channel_id();
        channels[i].weight = request.channels(i).weight();
      }

      CorbaCampaign::CampaignManager::DiscoverChannelResultSeq_var result =
        impl_->get_manager()->get_discover_channels(
          channels,
          request.country().c_str(),
          request.language().c_str(),
          request.all());

      for(CORBA::ULong i = 0; i < result->length(); ++i)
      {
        const auto& source = (*result)[i];
        auto* target = response.add_channels();
        target->set_channel_id(source.channel_id);
        target->set_name(source.name.in());
        target->set_query(source.query.in());
        target->set_annotation(source.annotation.in());
        target->set_weight(source.weight);
        pack_ids(source.categories, target->mutable_categories());
        target->set_country_code(source.country_code.in());
        target->set_language(source.language.in());
      }
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::NotReady& ex)
    {
      callback(
        exception_status(grpc::StatusCode::UNAVAILABLE, ex.description.in()),
        response);
    }
    catch(const CorbaCampaign::CampaignManager::ImplementationException& ex)
    {
      callback(
        exception_status(grpc::StatusCode::INTERNAL, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::get_category_channels(
    const pb::GetCategoryChannelsRequest& request,
    GetCategoryChannelsCallback callback)
  {
    pb::GetCategoryChannelsResponse response;
    try
    {
      CorbaCampaign::CampaignManager::CategoryChannelNodeSeq_var result =
        impl_->get_manager()->get_category_channels(request.language().c_str());

      for(CORBA::ULong i = 0; i < result->length(); ++i)
      {
        pack_category_channel_node((*result)[i], *response.add_channels());
      }
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::NotReady& ex)
    {
      callback(
        exception_status(grpc::StatusCode::UNAVAILABLE, ex.description.in()),
        response);
    }
    catch(const CorbaCampaign::CampaignManager::ImplementationException& ex)
    {
      callback(
        exception_status(grpc::StatusCode::INTERNAL, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::get_colocation_flags(
    const pb::GetColocationFlagsRequest&,
    GetColocationFlagsCallback callback)
  {
    pb::GetColocationFlagsResponse response;
    try
    {
      ColocationFlagsSeq_var result =
        impl_->get_manager()->get_colocation_flags();

      for(CORBA::ULong i = 0; i < result->length(); ++i)
      {
        auto* target = response.add_colocations();
        target->set_colo_id(result.in()[i].colo_id);
        target->set_flags(result.in()[i].flags);
        target->set_hid_profile(result.in()[i].hid_profile);
      }
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::NotReady& ex)
    {
      callback(
        exception_status(grpc::StatusCode::UNAVAILABLE, ex.description.in()),
        response);
    }
    catch(const CorbaCampaign::CampaignManager::ImplementationException& ex)
    {
      callback(
        exception_status(grpc::StatusCode::INTERNAL, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::get_pub_pixels(
    const pb::GetPubPixelsRequest& request,
    GetPubPixelsCallback callback)
  {
    pb::GetPubPixelsResponse response;
    try
    {
      PublisherAccountIdSeq publisher_account_ids;
      unpack_ids(request.publisher_account_ids(), publisher_account_ids);

      StringSeq_var result = impl_->get_manager()->get_pub_pixels(
        request.country().c_str(),
        request.user_status(),
        publisher_account_ids);

      for(CORBA::ULong i = 0; i < result->length(); ++i)
      {
        response.add_pixels((*result)[i].in());
      }
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::NotReady& ex)
    {
      callback(
        exception_status(grpc::StatusCode::UNAVAILABLE, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::consider_passback(
    const pb::ConsiderPassbackRequest& request,
    ConsiderPassbackCallback callback)
  {
    pb::ConsiderPassbackResponse response;
    try
    {
      CorbaCampaign::CampaignManager::PassbackInfo info;
      unpack_oct_seq(request.request_id(), info.request_id);
      unpack_oct_seq(request.time(), info.time);
      info.user_id_hash_mod.defined =
        request.has_user_id_hash_mod() &&
        request.user_id_hash_mod().defined();
      info.user_id_hash_mod.value =
        info.user_id_hash_mod.defined ?
        request.user_id_hash_mod().value() :
        0;
      info.passback = "";

      impl_->get_manager()->consider_passback(info);
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::ImplementationException& ex)
    {
      callback(
        exception_status(grpc::StatusCode::INTERNAL, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::consider_passback_track(
    const pb::ConsiderPassbackTrackRequest& request,
    ConsiderPassbackTrackCallback callback)
  {
    pb::ConsiderPassbackTrackResponse response;
    try
    {
      CorbaCampaign::CampaignManager::PassbackTrackInfo info;
      unpack_oct_seq(request.time(), info.time);
      info.country = request.country().c_str();
      info.colo_id = request.colo_id();
      info.tag_id = request.tag_id();
      info.user_status = request.user_status();

      impl_->get_manager()->consider_passback_track(info);
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::NotReady& ex)
    {
      callback(
        exception_status(grpc::StatusCode::UNAVAILABLE, ex.description.in()),
        response);
    }
    catch(const CorbaCampaign::CampaignManager::ImplementationException& ex)
    {
      callback(
        exception_status(grpc::StatusCode::INTERNAL, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::verify_opt_operation(
    const pb::VerifyOptOperationRequest& request,
    VerifyOptOperationCallback callback)
  {
    pb::VerifyOptOperationResponse response;
    try
    {
      CorbaCampaign::CampaignManager::OptOperation operation =
        CorbaCampaign::CampaignManager::OO_STATUS;
      switch(request.operation())
      {
      case pb::OPT_OPERATION_IN:
        operation = CorbaCampaign::CampaignManager::OO_IN;
        break;
      case pb::OPT_OPERATION_OUT:
        operation = CorbaCampaign::CampaignManager::OO_OUT;
        break;
      case pb::OPT_OPERATION_FORCED_IN:
        operation = CorbaCampaign::CampaignManager::OO_FORCED_IN;
        break;
      case pb::OPT_OPERATION_STATUS:
      default:
        operation = CorbaCampaign::CampaignManager::OO_STATUS;
        break;
      }

      CORBACommons::UserIdInfo user_id;
      unpack_oct_seq(request.user_id(), user_id);
      impl_->get_manager()->verify_opt_operation(
        request.time(),
        request.colo_id(),
        request.referer().c_str(),
        operation,
        request.status(),
        request.user_status(),
        request.log_as_test(),
        request.browser().c_str(),
        request.os().c_str(),
        request.ct().c_str(),
        request.curct().c_str(),
        user_id);
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::NotReady& ex)
    {
      callback(
        exception_status(grpc::StatusCode::UNAVAILABLE, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }

  void
  CampaignManagerCorbaClient::consider_web_operation(
    const pb::ConsiderWebOperationRequest& request,
    ConsiderWebOperationCallback callback)
  {
    pb::ConsiderWebOperationResponse response;
    try
    {
      CorbaCampaign::CampaignManager::WebOperationInfo info;
      unpack_oct_seq(request.time(), info.time);
      info.colo_id = request.colo_id();
      info.tag_id = request.tag_id();
      info.cc_id = request.cc_id();
      info.ct = request.ct().c_str();
      info.curct = request.curct().c_str();
      info.browser = request.browser().c_str();
      info.os = request.os().c_str();
      info.app = request.app().c_str();
      info.source = request.source().c_str();
      info.operation = request.operation().c_str();
      info.user_bind_src = request.user_bind_src().c_str();
      info.result = static_cast<CORBA::Char>(request.result());
      info.user_status = request.user_status();
      info.test_request = request.test_request();
      info.request_ids.length(request.request_ids_size());
      for(int i = 0; i < request.request_ids_size(); ++i)
      {
        unpack_oct_seq(request.request_ids(i), info.request_ids[i]);
      }
      unpack_oct_seq(request.global_request_id(), info.global_request_id);
      info.referer = request.referer().c_str();
      info.ip_address = request.ip_address().c_str();
      info.external_user_id = request.external_user_id().c_str();
      info.user_agent = request.user_agent().c_str();

      impl_->get_manager()->consider_web_operation(info);
      callback(grpc::Status::OK, response);
    }
    catch(const CorbaCampaign::CampaignManager::IncorrectArgument&)
    {
      callback(
        exception_status(grpc::StatusCode::INVALID_ARGUMENT, "incorrect argument"),
        response);
    }
    catch(const CorbaCampaign::CampaignManager::NotReady& ex)
    {
      callback(
        exception_status(grpc::StatusCode::UNAVAILABLE, ex.description.in()),
        response);
    }
    catch(const CorbaCampaign::CampaignManager::ImplementationException& ex)
    {
      callback(
        exception_status(grpc::StatusCode::INTERNAL, ex.description.in()),
        response);
    }
    catch(const CORBA::SystemException& ex)
    {
      callback(exception_status(ex), response);
    }
    catch(const eh::Exception& ex)
    {
      callback(exception_status(ex), response);
    }
  }
}
