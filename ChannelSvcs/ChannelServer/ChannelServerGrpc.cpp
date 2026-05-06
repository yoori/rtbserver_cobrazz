#include "ChannelServerGrpc.hpp"

#include <grpcpp/grpcpp.h>

#include <algorithm>
#include <string>

#include <Commons/CorbaAlgs.hpp>
#include <Commons/Grpc/GrpcServer.hpp>

#include <ChannelSvcs/ChannelServer/ChannelServerGrpc.grpc.pb.h>

namespace AdServer::ChannelSvcs
{
  namespace
  {
    constexpr const char channel_server_grpc_aspect[] = "ChannelServerGrpc";

    template<typename CorbaOctSeq>
    std::string
    pack_oct_seq(const CorbaOctSeq& seq)
    {
      return std::string(
        reinterpret_cast<const char*>(seq.get_buffer()),
        reinterpret_cast<const char*>(seq.get_buffer()) + seq.length());
    }

    void
    unpack_user_id(
      const std::string& value,
      CORBACommons::UserIdInfo& result)
    {
      result.length(value.size());
      std::copy(value.begin(), value.end(), result.get_buffer());
    }

    void
    pack_channel_atoms(
      const ChannelServerBase::ChannelAtomSeq& source,
      google::protobuf::RepeatedPtrField<
        adserver::channel_svcs::channel_server::ChannelAtom>* target)
    {
      for (CORBA::ULong i = 0; i < source.length(); ++i)
      {
        auto* atom = target->Add();
        atom->set_id(source[i].id);
        atom->set_trigger_channel_id(source[i].trigger_channel_id);
      }
    }

    void
    pack_match_result(
      const ChannelServer::MatchResult& source,
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

      for (
        CORBA::ULong i = 0;
        i < source.matched_channels.uid_channels.length();
        ++i)
      {
        matched_channels->add_uid_channels(
          source.matched_channels.uid_channels[i]);
      }

      for (CORBA::ULong i = 0; i < source.content_channels.length(); ++i)
      {
        auto* content_channel = target.add_content_channels();
        content_channel->set_id(source.content_channels[i].id);
        content_channel->set_weight(source.content_channels[i].weight);
      }

      target.set_no_adv(source.no_adv);
      target.set_no_track(source.no_track);
      target.set_match_time(pack_oct_seq(source.match_time));
    }

    void
    pack_traits_result(
      const ChannelServer::TraitsResult& source,
      adserver::channel_svcs::channel_server::GetCcgTraitsResponse& target)
    {
      for (CORBA::ULong i = 0; i < source.ccg_keywords.length(); ++i)
      {
        const auto& source_keyword = source.ccg_keywords[i];
        auto* keyword = target.add_ccg_keywords();
        keyword->set_ccg_keyword_id(source_keyword.ccg_keyword_id);
        keyword->set_ccg_id(source_keyword.ccg_id);
        keyword->set_channel_id(source_keyword.channel_id);
        keyword->set_max_cpc(pack_oct_seq(source_keyword.max_cpc));
        keyword->set_ctr(pack_oct_seq(source_keyword.ctr));
        keyword->set_click_url(source_keyword.click_url.in());
        keyword->set_original_keyword(source_keyword.original_keyword.in());
      }

      for (CORBA::ULong i = 0; i < source.neg_ccg.length(); ++i)
      {
        target.add_neg_ccg(source.neg_ccg[i]);
      }
    }
  }

  class ChannelServerGrpc::ServiceImpl final:
    public AdServer::Grpc::GrpcAsyncServiceBase<
      ChannelServerGrpc::ServiceImpl,
      adserver::channel_svcs::channel_server::ChannelServerGrpc,
      adserver::channel_svcs::channel_server::ChannelServerGrpc::AsyncService>
  {
    using AsyncService =
      adserver::channel_svcs::channel_server::ChannelServerGrpc::AsyncService;

  public:
    explicit ServiceImpl(ChannelServerCustomImpl* server_impl);

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
          get_ccg_traits));
    }

    void match(
      const adserver::channel_svcs::channel_server::MatchRequest& request,
      adserver::channel_svcs::channel_server::MatchResponse& response,
      ::grpc::Status& result_status) const;

    void get_ccg_traits(
      const adserver::channel_svcs::channel_server::GetCcgTraitsRequest& request,
      adserver::channel_svcs::channel_server::GetCcgTraitsResponse& response,
      ::grpc::Status& result_status) const;

  private:
    ChannelServerCustomImpl_var server_impl_;
  };

  ChannelServerGrpc::ServiceImpl::ServiceImpl(
    ChannelServerCustomImpl* server_impl)
    : server_impl_(ReferenceCounting::add_ref(server_impl))
  {}

  void
  ChannelServerGrpc::ServiceImpl::match(
    const adserver::channel_svcs::channel_server::MatchRequest& request,
    adserver::channel_svcs::channel_server::MatchResponse& response,
    ::grpc::Status& result_status) const
  {
    try
    {
      ChannelServerBase::MatchQuery query;
      query.request_id << request.request_id();
      query.first_url << request.first_url();
      query.first_url_words << request.first_url_words();
      query.urls << request.urls();
      query.urls_words << request.urls_words();
      query.pwords << request.pwords();
      query.swords << request.swords();
      unpack_user_id(request.uid(), query.uid);
      query.statuses[0] = request.statuses().empty() ?
        '\0' : request.statuses()[0];
      query.statuses[1] = request.statuses().size() > 1 ?
        request.statuses()[1] : '\0';
      query.non_strict_word_match = request.non_strict_word_match();
      query.non_strict_url_match = request.non_strict_url_match();
      query.return_negative = request.return_negative();
      query.simplify_page = request.simplify_page();
      query.fill_content = request.fill_content();

      ChannelServer::MatchResult_var result;
      server_impl_->match(query, result.out());
      pack_match_result(*result, response);

      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelSvcs::NotConfigured& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.description.in());
    }
    catch (const ChannelSvcs::ImplementationException& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.description.in());
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
    try
    {
      ChannelIdSeq ids;
      ids.length(request.ids_size());
      for (int i = 0; i < request.ids_size(); ++i)
      {
        ids[i] = request.ids(i);
      }

      ChannelServer::TraitsResult_var result;
      server_impl_->get_ccg_traits(ids, result.out());
      pack_traits_result(*result, response);

      result_status = ::grpc::Status::OK;
    }
    catch (const ChannelSvcs::NotConfigured& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::UNAVAILABLE,
        ex.description.in());
    }
    catch (const ChannelSvcs::ImplementationException& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.description.in());
    }
    catch (const eh::Exception& ex)
    {
      result_status = AdServer::Grpc::error_status(
        ::grpc::StatusCode::INTERNAL,
        ex.what());
    }
  }

  ChannelServerGrpc::ChannelServerGrpc(
    ChannelServerCustomImpl* server_impl,
    Logging::Logger* logger,
    std::string_view bind_address,
    unsigned int bind_port)
    : bind_address_(std::string(bind_address) + ":" + std::to_string(bind_port)),
      impl_(new Impl(
        logger,
        channel_server_grpc_aspect,
        bind_address_,
        std::make_unique<ServiceImpl>(server_impl)))
  {
    add_child_object(impl_);
  }

  ChannelServerGrpc::~ChannelServerGrpc() noexcept = default;
}
