#include "ChannelServerGrpc.hpp"
#include "ChannelServerCore.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <string>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/Grpc/GrpcServer.hpp>
#include <Commons/Grpc/ProcessControl.grpc.pb.h>

#include <ChannelSvcs/ChannelServer/ChannelServerGrpc.grpc.pb.h>

namespace AdServer::ChannelSvcs
{
  namespace
  {
    constexpr const char channel_server_grpc_aspect[] = "ChannelServerGrpc";
    namespace pc = adserver::grpc::process_control;

    class InProgressGuard final
    {
    public:
      InProgressGuard(
        std::atomic<std::uint64_t>& call_counter,
        std::atomic<std::uint64_t>& method_counter) noexcept
        : call_counter_(call_counter),
          method_counter_(method_counter)
      {
        call_counter_.fetch_add(1, std::memory_order_relaxed);
        method_counter_.fetch_add(1, std::memory_order_relaxed);
      }

      ~InProgressGuard()
      {
        method_counter_.fetch_sub(1, std::memory_order_relaxed);
        call_counter_.fetch_sub(1, std::memory_order_relaxed);
      }

      InProgressGuard(const InProgressGuard&) = delete;
      InProgressGuard& operator=(const InProgressGuard&) = delete;

    private:
      std::atomic<std::uint64_t>& call_counter_;
      std::atomic<std::uint64_t>& method_counter_;
    };

    template<typename CorbaOctSeq>
    std::string
    pack_oct_seq(const CorbaOctSeq& seq)
    {
      return std::string(
        reinterpret_cast<const char*>(seq.get_buffer()),
        reinterpret_cast<const char*>(seq.get_buffer()) + seq.length());
    }

    void pack_channel_atoms(
      const std::vector<ChannelServerCore::ChannelAtom>& source,
      google::protobuf::RepeatedPtrField<
        adserver::channel_svcs::channel_server::ChannelAtom>* target)
    {
      for (const auto& source_atom : source)
      {
        auto* atom = target->Add();
        atom->set_id(source_atom.id);
        atom->set_trigger_channel_id(source_atom.trigger_channel_id);
      }
    }

    void pack_match_result(
      const ChannelServerCore::MatchResult& source,
      adserver::channel_svcs::channel_server::MatchResponse& target)
    {
      auto* matched_channels = target.mutable_matched_channels();
      pack_channel_atoms(
        source.matched_channels.page_channels,
        matched_channels->mutable_page_channels());
      pack_channel_atoms(
        source.matched_channels.search_channels,
        matched_channels->mutable_search_channels());
      pack_channel_atoms(
        source.matched_channels.url_channels,
        matched_channels->mutable_url_channels());
      pack_channel_atoms(
        source.matched_channels.url_keyword_channels,
        matched_channels->mutable_url_keyword_channels());

      for (const auto channel_id : source.matched_channels.uid_channels)
      {
        matched_channels->add_uid_channels(channel_id);
      }

      for (const auto& source_channel : source.content_channels)
      {
        auto* content_channel = target.add_content_channels();
        content_channel->set_id(source_channel.id);
        content_channel->set_weight(source_channel.weight);
      }

      target.set_no_adv(source.no_adv);
      target.set_no_track(source.no_track);
      target.set_match_time(pack_oct_seq(CorbaAlgs::pack_time(source.match_time)));
    }

    void pack_traits_result(
      const ChannelServerCore::TraitsResult& source,
      adserver::channel_svcs::channel_server::GetCcgTraitsResponse& target)
    {
      for (const auto& source_keyword : source.ccg_keywords)
      {
        auto* keyword = target.add_ccg_keywords();
        keyword->set_ccg_keyword_id(source_keyword.ccg_keyword_id);
        keyword->set_ccg_id(source_keyword.ccg_id);
        keyword->set_channel_id(source_keyword.channel_id);
        keyword->set_max_cpc(pack_oct_seq(
          CorbaAlgs::pack_decimal<CampaignSvcs::RevenueDecimal>(
            source_keyword.max_cpc)));
        keyword->set_ctr(pack_oct_seq(
          CorbaAlgs::pack_decimal<CampaignSvcs::CTRDecimal>(
            source_keyword.ctr)));
        keyword->set_click_url(source_keyword.click_url);
        keyword->set_original_keyword(source_keyword.original_keyword);
      }

      for (const auto ccg_id : source.neg_ccg)
      {
        target.add_neg_ccg(ccg_id);
      }
    }

    std::vector<unsigned long>
    unpack_sources(
      const google::protobuf::RepeatedField<::google::protobuf::uint64>& source)
    {
      std::vector<unsigned long> result;
      result.reserve(source.size());
      for (const auto item : source)
      {
        result.emplace_back(item);
      }
      return result;
    }

    void
    unpack_local_groups(
      const google::protobuf::RepeatedPtrField<
        adserver::channel_svcs::channel_server::LocalLoadGroup>& groups,
      ChannelSvcs::GroupLoadDescriptionSeq& result)
    {
      CORBACommons::CorbaClientAdapter_var adapter =
        new CORBACommons::CorbaClientAdapter();
      result.length(groups.size());

      CORBA::ULong group_index = 0;
      for (const auto& group : groups)
      {
        result[group_index].length(group.servers_size());
        CORBA::ULong server_index = 0;
        for (const auto& server : group.servers())
        {
          CORBACommons::CorbaObjectRef ref(
            server.channel_update_ref().c_str());
          CORBA::Object_var obj = adapter->resolve_object(ref);
          ChannelSvcs::ChannelUpdate_var update =
            ChannelSvcs::ChannelUpdate::_narrow(obj.in());
          result[group_index][server_index].load_server =
            ChannelSvcs::ChannelUpdateBase::_duplicate(update.in());
          if (CORBA::is_nil(result[group_index][server_index].load_server.in()))
          {
            Stream::Error ostr;
            ostr << "can't narrow ChannelUpdateBase reference: " <<
              server.channel_update_ref();
            throw ChannelServerCore::Exception(ostr);
          }

          result[group_index][server_index].chunks.length(
            server.sources_size());
          CORBA::ULong source_index = 0;
          for (const auto source : server.sources())
          {
            result[group_index][server_index].chunks[source_index++] = source;
          }
          result[group_index][server_index].count_chunks =
            server.count_chunks();

          ++server_index;
        }
        ++group_index;
      }
    }
  }

  struct ChannelServerGrpc::AtomicStats
  {
    std::atomic<std::uint64_t> call_in_progress{0};
    std::atomic<std::uint64_t> match_in_progress{0};
    std::atomic<std::uint64_t> get_ccg_traits_in_progress{0};
  };

  class ChannelServerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      ChannelServerGrpc::ServiceImpl,
      adserver::channel_svcs::channel_server::ChannelServerGrpc,
      adserver::channel_svcs::channel_server::ChannelServerGrpc::AsyncService>
  {
    using AsyncService =
      adserver::channel_svcs::channel_server::ChannelServerGrpc::AsyncService;

  public:
    ServiceImpl(
      ChannelServerCorePtr core,
      std::shared_ptr<AtomicStats> stats);

    static auto grpc_calls()
    {
      return std::make_tuple(
        MAKE_GRPC_CALL(
          adserver::channel_svcs::channel_server::MatchRequest,
          adserver::channel_svcs::channel_server::MatchResponse,
          match),
        MAKE_GRPC_CALL(
          adserver::channel_svcs::channel_server::GetCcgTraitsRequest,
          adserver::channel_svcs::channel_server::GetCcgTraitsResponse,
          get_ccg_traits),
        MAKE_GRPC_CALL(
          adserver::channel_svcs::channel_server::SetSourcesRequest,
          adserver::channel_svcs::channel_server::SetSourcesResponse,
          set_sources),
        MAKE_GRPC_CALL(
          adserver::channel_svcs::channel_server::SetProxySourcesRequest,
          adserver::channel_svcs::channel_server::SetProxySourcesResponse,
          set_proxy_sources));
    }

    void match(
      const adserver::channel_svcs::channel_server::MatchRequest& request,
      adserver::channel_svcs::channel_server::MatchResponse& response,
      ::grpc::Status& result_status) const;

    void get_ccg_traits(
      const adserver::channel_svcs::channel_server::GetCcgTraitsRequest& request,
      adserver::channel_svcs::channel_server::GetCcgTraitsResponse& response,
      ::grpc::Status& result_status) const;

    void set_sources(
      const adserver::channel_svcs::channel_server::SetSourcesRequest& request,
      adserver::channel_svcs::channel_server::SetSourcesResponse& response,
      ::grpc::Status& result_status) const;

    void set_proxy_sources(
      const adserver::channel_svcs::channel_server::SetProxySourcesRequest& request,
      adserver::channel_svcs::channel_server::SetProxySourcesResponse& response,
      ::grpc::Status& result_status) const;

  private:
    class ProcessControlService final:
      public pc::ProcessControl::Service
    {
    public:
      explicit ProcessControlService(const ServiceImpl& owner);

      ::grpc::Status get_status(
        ::grpc::ServerContext* context,
        const pc::GetStatusRequest* request,
        pc::GetStatusResponse* response) override;

      ::grpc::Status get_db_state(
        ::grpc::ServerContext* context,
        const pc::GetDbStateRequest* request,
        pc::GetDbStateResponse* response) override;

      ::grpc::Status set_db_state(
        ::grpc::ServerContext* context,
        const pc::SetDbStateRequest* request,
        pc::SetDbStateResponse* response) override;

    private:
      const ServiceImpl& owner_;
    };

    void get_status_(pc::GetStatusResponse& response) const;
    ::grpc::Status get_db_state_(pc::GetDbStateResponse& response) const;
    ::grpc::Status set_db_state_(const pc::SetDbStateRequest& request) const;

  private:
    ProcessControlService process_control_service_;
    ChannelServerCorePtr core_;
    const std::shared_ptr<AtomicStats> stats_;
  };

  ChannelServerGrpc::ServiceImpl::ProcessControlService::
  ProcessControlService(const ServiceImpl& owner)
    : owner_(owner)
  {}

  ::grpc::Status
  ChannelServerGrpc::ServiceImpl::ProcessControlService::get_status(
    ::grpc::ServerContext*,
    const pc::GetStatusRequest*,
    pc::GetStatusResponse* response)
  {
    owner_.get_status_(*response);
    return ::grpc::Status::OK;
  }

  ::grpc::Status
  ChannelServerGrpc::ServiceImpl::ProcessControlService::get_db_state(
    ::grpc::ServerContext*,
    const pc::GetDbStateRequest*,
    pc::GetDbStateResponse* response)
  {
    return owner_.get_db_state_(*response);
  }

  ::grpc::Status
  ChannelServerGrpc::ServiceImpl::ProcessControlService::set_db_state(
    ::grpc::ServerContext*,
    const pc::SetDbStateRequest* request,
    pc::SetDbStateResponse*)
  {
    return owner_.set_db_state_(*request);
  }

  ChannelServerGrpc::ServiceImpl::ServiceImpl(
    ChannelServerCorePtr core,
    std::shared_ptr<AtomicStats> stats)
    : process_control_service_(*this),
      core_(std::move(core)),
      stats_(std::move(stats))
  {
    add_grpc_service(&process_control_service_);
  }

  void
  ChannelServerGrpc::ServiceImpl::get_status_(
    pc::GetStatusResponse& response) const
  {
    try
    {
      const bool ready = core_->ready();
      response.set_ready(ready);
      if (!ready)
      {
        response.set_description(core_->comment());
      }
    }
    catch (const eh::Exception& ex)
    {
      response.set_ready(false);
      response.set_description(ex.what());
    }
  }

  ::grpc::Status
  ChannelServerGrpc::ServiceImpl::get_db_state_(
    pc::GetDbStateResponse& response) const
  {
    try
    {
      response.set_enabled(core_->get_db_state());
      return ::grpc::Status::OK;
    }
    catch (const Commons::DbStateChanger::NotSupported& ex)
    {
      return AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNIMPLEMENTED,
        ex.what());
    }
    catch (const ChannelServerCore::Exception& ex)
    {
      return AdServer::Grpc::error_status(
        ::grpc::StatusCode::FAILED_PRECONDITION,
        ex.what());
    }
    catch (const eh::Exception& ex)
    {
      return AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  ::grpc::Status
  ChannelServerGrpc::ServiceImpl::set_db_state_(
    const pc::SetDbStateRequest& request) const
  {
    try
    {
      core_->change_db_state(request.enabled());
      return ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::Exception& ex)
    {
      return AdServer::Grpc::error_status(
        ::grpc::StatusCode::FAILED_PRECONDITION,
        ex.what());
    }
    catch (const eh::Exception& ex)
    {
      return AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  ChannelServerGrpc::ServiceImpl::match(
    const adserver::channel_svcs::channel_server::MatchRequest& request,
    adserver::channel_svcs::channel_server::MatchResponse& response,
    ::grpc::Status& result_status) const
  {
    InProgressGuard in_progress(
      stats_->call_in_progress,
      stats_->match_in_progress);

    try
    {
      ChannelServerCore::MatchQuery query;
      query.request_id = request.request_id();
      query.first_url = request.first_url();
      query.first_url_words = request.first_url_words();
      query.urls = request.urls();
      query.urls_words = request.urls_words();
      query.pwords = request.pwords();
      query.swords = request.swords();
      query.uid = GrpcAlgs::unpack_user_id(request.uid());
      query.statuses[0] = request.statuses().empty() ?
        '\0' : request.statuses()[0];
      query.statuses[1] = request.statuses().size() > 1 ?
        request.statuses()[1] : '\0';
      query.non_strict_word_match = request.non_strict_word_match();
      query.non_strict_url_match = request.non_strict_url_match();
      query.return_negative = request.return_negative();
      query.simplify_page = request.simplify_page();
      query.fill_content = request.fill_content();

      ChannelServerCore::MatchResult result;
      core_->match(query, result);
      pack_match_result(result, response);

      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::NotConfigured& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const ChannelServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  ChannelServerGrpc::ServiceImpl::get_ccg_traits(
    const adserver::channel_svcs::channel_server::GetCcgTraitsRequest& request,
    adserver::channel_svcs::channel_server::GetCcgTraitsResponse& response,
    ::grpc::Status& result_status) const
  {
    InProgressGuard in_progress(
      stats_->call_in_progress,
      stats_->get_ccg_traits_in_progress);

    try
    {
      std::vector<unsigned long> ids;
      ids.reserve(request.ids_size());
      for (int i = 0; i < request.ids_size(); ++i)
      {
        ids.push_back(request.ids(i));
      }

      ChannelServerCore::TraitsResult result;
      core_->get_ccg_traits(ids, result);
      pack_traits_result(result, response);

      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::NotConfigured& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.what());
    }
    catch (const ChannelServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  ChannelServerGrpc::ServiceImpl::set_sources(
    const adserver::channel_svcs::channel_server::SetSourcesRequest& request,
    adserver::channel_svcs::channel_server::SetSourcesResponse&,
    ::grpc::Status& result_status) const
  {
    try
    {
      ChannelServerCore::DBSourceInfo db_info;
      db_info.pg_connection = request.pg_connection();
      db_info.colo = request.colo();
      db_info.version = request.version();
      db_info.count_chunks = request.count_chunks();
      db_info.check_sum = request.check_sum();

      db_info.campaign_refs.reserve(request.campaign_refs_size());
      for (const auto& ref : request.campaign_refs())
      {
        db_info.campaign_refs.emplace_back(ref);
      }

      core_->set_sources(db_info, unpack_sources(request.sources()));
      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  void
  ChannelServerGrpc::ServiceImpl::set_proxy_sources(
    const adserver::channel_svcs::channel_server::SetProxySourcesRequest& request,
    adserver::channel_svcs::channel_server::SetProxySourcesResponse&,
    ::grpc::Status& result_status) const
  {
    try
    {
      ChannelServerCore::ProxySourceInfo proxy_info;
      proxy_info.count_chunks = request.count_chunks();
      proxy_info.colo = request.colo();
      proxy_info.version = request.version();
      proxy_info.check_sum = request.check_sum();
      unpack_local_groups(
        request.local_groups(),
        proxy_info.local_descriptor);

      proxy_info.campaign_refs.reserve(request.campaign_refs_size());
      for (const auto& ref : request.campaign_refs())
      {
        proxy_info.campaign_refs.emplace_back(ref);
      }

      proxy_info.proxy_refs.reserve(request.proxy_refs_size());
      for (const auto& ref : request.proxy_refs())
      {
        proxy_info.proxy_refs.emplace_back(ref);
      }

      core_->set_proxy_sources(proxy_info, unpack_sources(request.sources()));
      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelServerCore::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
    catch (const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << ex;
      const String::SubString error_substr = ostr.str();
      const std::string error(error_substr.data(), error_substr.size());
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        error.c_str());
    }
  }

  ChannelServerGrpc::ChannelServerGrpc(
    ChannelServerCorePtr core,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      stats_(std::make_shared<AtomicStats>()),
      impl_(std::make_shared<Impl>(
        logger,
        channel_server_grpc_aspect,
        bind_address_,
        std::make_unique<ServiceImpl>(std::move(core), stats_)))
  {
    add_child_object(impl_);
  }

  ChannelServerGrpc::Stats
  ChannelServerGrpc::stats() const noexcept
  {
    return Stats{
      stats_->call_in_progress.load(std::memory_order_relaxed),
      stats_->match_in_progress.load(std::memory_order_relaxed),
      stats_->get_ccg_traits_in_progress.load(std::memory_order_relaxed)
    };
  }

  ChannelServerGrpc::~ChannelServerGrpc() noexcept = default;
}
