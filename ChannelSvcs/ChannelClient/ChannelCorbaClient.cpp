#include "ChannelCorbaClient.hpp"

#include <algorithm>

#include <grpcpp/support/status.h>

namespace AdServer::ChannelSvcs
{
  namespace
  {
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

  }

  ChannelCorbaClient::PoolConfig::Resolver::Resolver(
    const CORBACommons::CorbaClientAdapter* corba_client_adapter)
    noexcept
    : corba_client_adapter_(ReferenceCounting::add_ref(corba_client_adapter))
  {}

  template<typename PoolType>
  PoolType*
  ChannelCorbaClient::PoolConfig::Resolver::resolve(
    const ObjectRef& ref)
    /*throw(Exception)*/
  {
    static const char* FUN = "ChannelCorbaClient::PoolConfig::Resolver::resolve()";

    try
    {
      CORBA::Object_var obj = corba_client_adapter_->resolve_object(ref);

      ChannelManagerController_var controller =
        ChannelManagerController::_narrow(obj);
      if(CORBA::is_nil(controller))
      {
        return nullptr;
      }

      ChannelServerSession_var channel_session =
        controller->get_channel_session();

      return ChannelServerSession::_narrow(channel_session);
    }
    catch (const ImplementationException& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": failed to resolve channel reference '" <<
        ref.object_ref << "'. AdServer::ChannelSvcs::"
        "ImplementationException caught: " <<
        ex.description;
      throw Exception(ostr);
    }
    catch (const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": failed to resolve channel reference '" <<
        ref.object_ref << "'. CORBA::Exception caught: " << ex;
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": failed to resolve channel reference '" <<
        ref.object_ref << "'. eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  ChannelCorbaClient::ChannelCorbaClient(
    const CORBACommons::CorbaObjectRefList& controller_refs,
    const CORBACommons::CorbaClientAdapter* corba_client_adapter,
    Generics::ActiveObjectCallback* callback)
    /*throw(Exception)*/
  {
    static const char* FUN = "ChannelCorbaClient::ChannelCorbaClient()";

    try
    {
      if (controller_refs.size() == 1)
      {
        try
        {
          CORBA::Object_var obj = corba_client_adapter->resolve_object(
            controller_refs.front());
          direct_channel_server_ = ChannelServer::_narrow(obj);
        }
        catch (...)
        {
        }
      }

      ChannelServerSessionFactory::init(
        *corba_client_adapter,
        &channel_session_factory_,
        0,
        callback,
        0);

      PoolConfig pool_config(corba_client_adapter);
      if (CORBA::is_nil(direct_channel_server_))
      {
        std::copy(controller_refs.begin(),
          controller_refs.end(),
          std::back_inserter(pool_config.iors_list));
        pool_config.timeout = Generics::Time(10);

        pool_.reset(new PoolImpl(
          pool_config, CORBACommons::ChoosePolicyType::PT_LOOP));
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  ChannelCorbaClient::pack_get_ccg_traits_request_(
    const ChannelIdSeq& source,
    adserver::channel_svcs::channel_server::GetCcgTraitsRequest& target)
  {
    for (CORBA::ULong i = 0; i < source.length(); ++i)
    {
      target.add_ids(source[i]);
    }
  }

  ChannelServerBase::CCGKeywordSeq*
  ChannelCorbaClient::unpack_get_ccg_traits_response_(
    const adserver::channel_svcs::channel_server::GetCcgTraitsResponse& source)
  {
    ChannelServerBase::CCGKeywordSeq_var result =
      new ChannelServerBase::CCGKeywordSeq;
    result->length(source.ccg_keywords_size());
    for (int i = 0; i < source.ccg_keywords_size(); ++i)
    {
      const auto& source_keyword = source.ccg_keywords(i);
      auto& target_keyword = (*result)[i];
      target_keyword.ccg_keyword_id = source_keyword.ccg_keyword_id();
      target_keyword.ccg_id = source_keyword.ccg_id();
      target_keyword.channel_id = source_keyword.channel_id();
      unpack_oct_seq(source_keyword.max_cpc(), target_keyword.max_cpc);
      unpack_oct_seq(source_keyword.ctr(), target_keyword.ctr);
      target_keyword.click_url = source_keyword.click_url().c_str();
      target_keyword.original_keyword = source_keyword.original_keyword().c_str();
    }

    return result._retn();
  }

  void
  ChannelCorbaClient::match(
    const adserver::channel_svcs::channel_server::MatchRequest& request,
    MatchCallback callback)
  {
    adserver::channel_svcs::channel_server::MatchResponse response;
    try
    {
      ChannelServerBase::MatchQuery query;
      query.request_id = request.request_id().c_str();
      query.first_url = request.first_url().c_str();
      query.first_url_words = request.first_url_words().c_str();
      query.urls = request.urls().c_str();
      query.urls_words = request.urls_words().c_str();
      query.pwords = request.pwords().c_str();
      query.swords = request.swords().c_str();
      unpack_oct_seq(request.uid(), query.uid);
      query.statuses[0] = request.statuses().empty() ?
        '\0' : request.statuses()[0];
      query.statuses[1] = request.statuses().size() > 1 ?
        request.statuses()[1] : '\0';
      query.non_strict_word_match = request.non_strict_word_match();
      query.non_strict_url_match = request.non_strict_url_match();
      query.return_negative = request.return_negative();
      query.simplify_page = request.simplify_page();
      query.fill_content = request.fill_content();

      ChannelServerBase::MatchResult_var result;
      match(query, result.out());
      const auto& matched_channels = result->matched_channels;
      auto pack_atoms = [](
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
      };
      auto* response_channels = response.mutable_matched_channels();
      pack_atoms(
        matched_channels.page_channels,
        response_channels->mutable_page_channels());
      pack_atoms(
        matched_channels.search_channels,
        response_channels->mutable_search_channels());
      pack_atoms(
        matched_channels.url_channels,
        response_channels->mutable_url_channels());
      pack_atoms(
        matched_channels.url_keyword_channels,
        response_channels->mutable_url_keyword_channels());
      for (CORBA::ULong i = 0; i < matched_channels.uid_channels.length(); ++i)
      {
        response_channels->add_uid_channels(matched_channels.uid_channels[i]);
      }
      for (CORBA::ULong i = 0; i < result->content_channels.length(); ++i)
      {
        auto* content_channel = response.add_content_channels();
        content_channel->set_id(result->content_channels[i].id);
        content_channel->set_weight(result->content_channels[i].weight);
      }
      response.set_no_adv(result->no_adv);
      response.set_no_track(result->no_track);
      if (result->match_time.length() > 0)
      {
        response.set_match_time(pack_oct_seq(result->match_time[0]));
      }
      callback(grpc::Status::OK, response);
    }
    catch(const NotConfigured& ex)
    {
      callback(
        exception_status(grpc::StatusCode::UNAVAILABLE, ex.description.in()),
        response);
    }
    catch(const ImplementationException& ex)
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
  ChannelCorbaClient::get_ccg_traits(
    const adserver::channel_svcs::channel_server::GetCcgTraitsRequest& request,
    GetCcgTraitsCallback callback)
  {
    adserver::channel_svcs::channel_server::GetCcgTraitsResponse response;
    try
    {
      ChannelIdSeq ids;
      ids.length(request.ids_size());
      for (int i = 0; i < request.ids_size(); ++i)
      {
        ids[i] = request.ids(i);
      }

      ChannelServerBase::CCGKeywordSeq_var result = get_ccg_traits(ids);
      for (CORBA::ULong i = 0; i < result->length(); ++i)
      {
        const auto& source_keyword = (*result)[i];
        auto* keyword = response.add_ccg_keywords();
        keyword->set_ccg_keyword_id(source_keyword.ccg_keyword_id);
        keyword->set_ccg_id(source_keyword.ccg_id);
        keyword->set_channel_id(source_keyword.channel_id);
        keyword->set_max_cpc(pack_oct_seq(source_keyword.max_cpc));
        keyword->set_ctr(pack_oct_seq(source_keyword.ctr));
        keyword->set_click_url(source_keyword.click_url.in());
        keyword->set_original_keyword(source_keyword.original_keyword.in());
      }
      callback(grpc::Status::OK, response);
    }
    catch(const NotConfigured& ex)
    {
      callback(
        exception_status(grpc::StatusCode::UNAVAILABLE, ex.description.in()),
        response);
    }
    catch(const ImplementationException& ex)
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
  ChannelCorbaClient::check_configuration(
    const adserver::channel_svcs::channel_server::CheckConfigurationRequest&,
    CheckConfigurationCallback callback)
  {
    callback(
      grpc::Status(
        grpc::StatusCode::UNIMPLEMENTED,
        "check_configuration is not supported by ChannelCorbaClient"),
      adserver::channel_svcs::channel_server::CheckConfigurationResponse());
  }

  void
  ChannelCorbaClient::set_sources(
    const adserver::channel_svcs::channel_server::SetSourcesRequest&,
    SetSourcesCallback callback)
  {
    callback(
      grpc::Status(
        grpc::StatusCode::UNIMPLEMENTED,
        "set_sources isn't supported by ChannelCorbaClient"),
      adserver::channel_svcs::channel_server::SetSourcesResponse());
  }

  void
  ChannelCorbaClient::set_proxy_sources(
    const adserver::channel_svcs::channel_server::SetProxySourcesRequest&,
    SetProxySourcesCallback callback)
  {
    callback(
      grpc::Status(
        grpc::StatusCode::UNIMPLEMENTED,
        "set_proxy_sources isn't supported by ChannelCorbaClient"),
      adserver::channel_svcs::channel_server::SetProxySourcesResponse());
  }

  void
  ChannelCorbaClient::match(
    const ChannelServerBase::MatchQuery& query,
    ChannelServerBase::MatchResult_out result)
    /*throw(Exception)*/
  {
    static const char* FUN = "ChannelCorbaClient::match()";

    try
    {
      if (!CORBA::is_nil(direct_channel_server_))
      {
        ChannelServer::MatchResult_var direct_result;
        direct_channel_server_->match(query, direct_result.out());

        result = new ChannelServerBase::MatchResult;
        result->matched_channels = direct_result->matched_channels;
        result->content_channels = direct_result->content_channels;
        result->no_adv = direct_result->no_adv;
        result->no_track = direct_result->no_track;
        result->match_time.length(1);
        result->match_time[0] = direct_result->match_time;
        return;
      }

      ChannelServerHandler channel_server =
        pool_->get_object<Exception>();

      try
      {
        channel_server->match(query, result);
      }
      catch(const ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Caught ChannelServer::ImplementationException: " <<
          ex.description;

        throw Exception(ostr);
      }
      catch(const NotConfigured& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Caught ChannelServer::NotConfigured: " <<
          ex.description;

        channel_server.release_bad(ostr.str());

        throw Exception(ostr);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  ChannelServerBase::CCGKeywordSeq*
  ChannelCorbaClient::get_ccg_traits(
    const ChannelIdSeq& ids)
    /*throw(Exception)*/
  {
    static const char* FUN = "ChannelCorbaClient::get_ccg_traits()";

    try
    {
      if (!CORBA::is_nil(direct_channel_server_))
      {
        ChannelServer::TraitsResult_var direct_result;
        direct_channel_server_->get_ccg_traits(ids, direct_result.out());
        ChannelServerBase::CCGKeywordSeq_var result =
          new ChannelServerBase::CCGKeywordSeq;
        result->length(direct_result->ccg_keywords.length());
        for (CORBA::ULong i = 0; i < direct_result->ccg_keywords.length(); ++i)
        {
          (*result)[i].ccg_keyword_id =
            direct_result->ccg_keywords[i].ccg_keyword_id;
          (*result)[i].ccg_id = direct_result->ccg_keywords[i].ccg_id;
          (*result)[i].channel_id = direct_result->ccg_keywords[i].channel_id;
          (*result)[i].max_cpc = direct_result->ccg_keywords[i].max_cpc;
          (*result)[i].ctr = direct_result->ccg_keywords[i].ctr;
          (*result)[i].click_url =
            direct_result->ccg_keywords[i].click_url.in();
          (*result)[i].original_keyword =
            direct_result->ccg_keywords[i].original_keyword.in();
        }
        return result._retn();
      }

      ChannelServerHandler channel_server =
        pool_->get_object<Exception>();

      try
      {
        return channel_server->get_ccg_traits(ids);
      }
      catch(const ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Caught ChannelServer::ImplementationException: " <<
          ex.description;

        throw Exception(ostr);
      }
      catch(const NotConfigured& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": Caught ChannelServer::NotConfigured: " <<
          ex.description;

        channel_server.release_bad(ostr.str());

        throw Exception(ostr);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
}
