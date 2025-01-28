
#include <sstream>

#include <Logger/StreamLogger.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/ExternalUserIdUtils.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserverConfigUtils.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/add_UID_cookie.hpp>
#include <Frontends/FrontendCommons/GeoInfoUtils.hpp>

#include "ImprTrackFrontend.hpp"

namespace
{
  struct ImprTrackFrontendConstrainTraits
  {
    static const unsigned long MAX_NUMBER_PARAMS = 30;
    static const unsigned long MAX_LENGTH_PARAM_NAME = 20;
    static const unsigned long MAX_LENGTH_PARAM_VALUE = 1024;
  };

  typedef const String::AsciiStringManip::Char2Category<',', ' '> ListParameterSepCategory;

  typedef FrontendCommons::DefaultConstrain<
    FrontendCommons::OnlyGetAllowed,
    FrontendCommons::ParamConstrainDefault,
    ImprTrackFrontendConstrainTraits>
      ImprTrackFrontendHTTPConstrain;

  namespace Tokens
  {
    const String::SubString BIND_URL_TOKEN("BINDURL");
    const String::SubString DNS_ENCODED_UIDS_TOKEN("DNSUIDS");
    const String::SubString RANDOM("RANDOM");
    const String::SubString SOURCE_ID("SOURCEID");
    const String::SubString EXTERNAL_USER_ID("EXTERNALID");
    const String::SubString ADD_USER_ID("ADDUSERID");
  }

  const bool USER_PROFILE_MERGE_ENABLED = false;

  const String::SubString HTTPS_PREFIX("https:");
  const String::SubString HTTP_PREFIX("http:");
}

namespace Config
{
  const char ENABLE[] = "ImprTrackFrontend_Enable";
  const char CONFIG_FILE[] = "ImprTrackFrontend_Config";
}

namespace Aspect
{
  const char IMPR_TRACK_FRONTEND[] = "ImprTrackFrontend";
}

namespace AdServer
{
namespace ImprTrack
{
  namespace Request
  {
    namespace Cookie
    {
      const Generics::SubStringHashAdapter USER_ID(String::SubString("uid"));
    }
  }

  namespace WebStats
  {
    const String::SubString APPLICATION("adserver");
    const String::SubString SOURCE("imprtrack");
    const String::SubString INVALID_MAPPING_OPERATION("invalid-mapping");    
  }

  namespace
  {
    enum ResultUserIdType
    {
      RUIT_COOKIE,
      RUIT_CRESOLVE,
      RUIT_EXTIDRESOLVE
    };

    struct GetChannelTriggerId
    {
      ChannelMatch
      operator() (
        const AdServer::ChannelSvcs::ChannelServerBase::ChannelAtom& atom)
        noexcept
      {
        return ChannelMatch(atom.id, atom.trigger_channel_id);
      }
    };
  };

  class Frontend::MatchChannelsTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    MatchChannelsTask(
      Frontend* impr_frontend,
      const AdServer::Commons::UserId& user_id,
      const AdServer::Commons::UserId& cookie_user_id,
      const Generics::Time& now,
      const std::vector<std::uint32_t>& campaign_ids,
      const std::vector<std::uint32_t>& advertiser_ids,
      const String::SubString& peer_ip,
      const std::list<std::string>& markers)
      noexcept
      : impr_frontend_(impr_frontend),
        user_id_(user_id),
        cookie_user_id_(cookie_user_id),
        now_(now),
        campaign_ids_(campaign_ids),
        advertiser_ids_(advertiser_ids),
        peer_ip_(peer_ip.str()),
        markers_(markers)
    {
    }

    virtual
    void
    execute() noexcept
    {
      impr_frontend_->match_channels_(
        user_id_,
        cookie_user_id_,
        now_,
        campaign_ids_,
        advertiser_ids_,
        peer_ip_,
        markers_);
    }

  protected:
    virtual
    ~MatchChannelsTask() noexcept
    {}

  private:
    Frontend* impr_frontend_;
    AdServer::Commons::UserId user_id_;
    AdServer::Commons::UserId cookie_user_id_;
    Generics::Time now_;
    std::vector<CORBA::ULong> campaign_ids_;
    std::vector<CORBA::ULong> advertiser_ids_;
    const std::string peer_ip_;
    const std::list<std::string> markers_;
  };

  Frontend::Frontend(
    TaskProcessor& helper_task_processor,
    const GrpcContainerPtr& grpc_container,
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module,
    FrontendCommons::HttpResponseFactory* response_factory)
    /*throw(eh::Exception)*/
    : FrontendCommons::FrontendInterface(response_factory),
      Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().ImprTrackFeConfiguration()->Logger().log_level())),
        "ImprTrackFrontend",
        Aspect::IMPR_TRACK_FRONTEND,
        0),
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        response_factory,
        frontend_config->get().ImprTrackFeConfiguration()->threads(),
        0), // max pending tasks
      helper_task_processor_(helper_task_processor),
      grpc_container_(grpc_container),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      campaign_managers_(this->logger(), Aspect::IMPR_TRACK_FRONTEND)
  {}

  void
  Frontend::parse_config_() /*throw(Exception)*/
  {
    static const char* FUN = "Frontend::parse_config_()";

    try
    {
      typedef Configuration::FeConfig Config;
      const Config& fe_config = frontend_config_->get();

      if(!fe_config.CommonFeConfiguration().present())
      {
        throw Exception("CommonFeConfiguration not presented.");
      }

      common_config_ = CommonConfigPtr(
        new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      if(!fe_config.ImprTrackFeConfiguration().present())
      {
        throw Exception("ImprTrackFeConfiguration not presented.");
      }

      config_ = ConfigPtr(
        new ImprTrackFeConfiguration(*fe_config.ImprTrackFeConfiguration()));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't parse config file: " << e.what();
      throw Exception(ostr);
    }
  }

  bool
  Frontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;
    bool result = FrontendCommons::find_uri(
      config_->UriList().Uri(), uri, found_uri);

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "ImprTrack::Frontend::will_handle(" << uri << "), service: '" <<
        found_uri << "'";

      logger()->log(ostr.str());
    }

    return result;
  }

  void
  Frontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "Frontend::init()";

    if(true) // module_used())
    {
      try
      {
        parse_config_();

        try
        {
          if (common_config_->GeoIP().present())
          {
            ip_map_ = IPMapPtr(new GeoIPMapping::IPMapCity2(
              common_config_->GeoIP()->path().c_str()));
          }
        }
        catch (const GeoIPMapping::IPMap::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": GeoIPMapping::IPMap::Exception caught: " << e.what();

          logger()->log(ostr.str(),
            Logging::Logger::CRITICAL,
            Aspect::IMPR_TRACK_FRONTEND,
            "ADS-IMPL-102");
        }

        corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

        campaign_managers_.resolve(
          *common_config_, corba_client_adapter_);

        cookie_manager_.reset(
          new FrontendCommons::CookieManager<
            FrontendCommons::HttpRequest, FrontendCommons::HttpResponse>(common_config_->Cookies()));

        task_runner_ = new Generics::TaskRunner(
          callback(), config_->match_threads(), 0, config_->match_task_limit());
        add_child_object(task_runner_);

        if(!common_config_->UserBindControllerGroup().empty())
        {
          user_bind_client_ = new FrontendCommons::UserBindClient(
            common_config_->UserBindControllerGroup(),
            corba_client_adapter_.in(),
            logger(),
            grpc_container_->grpc_user_bind_operation_distributor.in());
          add_child_object(user_bind_client_);
        }

        user_info_client_ = new FrontendCommons::UserInfoClient(
          common_config_->UserInfoManagerControllerGroup(),
          corba_client_adapter_.in(),
          logger(),
          grpc_container_->grpc_user_info_operation_distributor.in());
        add_child_object(user_info_client_);

        CORBACommons::CorbaObjectRefList channel_server_controller_refs;

        Config::CorbaConfigReader::read_multi_corba_ref(
          common_config_->ChannelManagerControllerRefs().get(),
          channel_server_controller_refs);

        channel_servers_.reset(
          new FrontendCommons::ChannelServerSessionPool(
            channel_server_controller_refs,
            corba_client_adapter_,
            callback()));

        track_pixel_ = FileCachePtr(
          new FileCache(config_->track_pixel_path().c_str()));
        track_pixel_content_type_ = config_->track_pixel_content_type();

        RequestInfoFiller::EncryptionKeys_var default_keys =
          read_keys_(config_->DefaultKeys());
        RequestInfoFiller::EncryptionKeysMap account_keys;
        RequestInfoFiller::EncryptionKeysMap site_keys;

        // fill account and site keys
        for (ImprTrackFeConfiguration::AccountTraits_sequence::
             const_iterator it = config_->AccountTraits().begin();
           it != config_->AccountTraits().end(); ++it)
        {
          account_keys[it->account_id()] = read_keys_(*it);
        }

        for (ImprTrackFeConfiguration::SiteTraits_sequence::
            const_iterator it = config_->SiteTraits().begin();
          it != config_->SiteTraits().end(); ++it)
        {
          site_keys[it->site_id()] = read_keys_(*it);
        }

        template_files_ = new Commons::TextTemplateCache(
          static_cast<unsigned long>(-1),
          Generics::Time::ONE_MINUTE,
          Commons::TextTemplateCacheConfiguration<Commons::TextTemplate>(Generics::Time::ONE_SECOND));

        for(auto bind_url_it = config_->BindURL().begin();
          bind_url_it != config_->BindURL().end(); ++bind_url_it)
        {
          BindURLRule_var bind_url_rule = new BindURLRule();
          bind_url_rule->url_template = new Commons::TextTemplate(
            bind_url_it->template_());
          bind_url_rule->use_keywords = bind_url_it->use_keywords();

          if(bind_url_it->use_keywords())
          {
            String::StringManip::Splitter<String::AsciiStringManip::SepNL> splitter(
              bind_url_it->keywords());
            String::SubString token;
            while(splitter.get_token(token))
            {
              String::StringManip::trim(token);
              if(!token.empty())
              {
                bind_url_rule->keywords.insert(Generics::StringHashAdapter(token));
              }
            }
          }

          bind_url_rules_.push_back(bind_url_rule);
        }

        track_template_file_ = config_->template_file();

        request_info_filler_.reset(
          new RequestInfoFiller(
            logger(),
            common_config_->GeoIP().present() ?
              common_config_->GeoIP()->path().c_str() : 0,
            common_module_,
            common_config_->colo_id(),
            default_keys,
            account_keys,
            site_keys));

        activate_object();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      logger()->log(String::SubString(
          "Frontend::init(): frontend is running ..."),
        Logging::Logger::INFO,
        Aspect::IMPR_TRACK_FRONTEND);
    }
  }

  void
  Frontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();

      corba_client_adapter_.reset();

      Stream::Error ostr;
      ostr << "Frontend::shutdown: frontend terminated (pid = "
        << ::getpid() << ").";

      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::IMPR_TRACK_FRONTEND);
    }
    catch(...)
    {
    }
  }

  void
  Frontend::handle_request_(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer)
    noexcept
  {
    const FrontendCommons::HttpRequest& request = request_holder->request();

    FrontendCommons::HttpResponse_var response_ptr = create_response();
    FrontendCommons::HttpResponse& response = *response_ptr;
    int http_status = handle_request_(request, response);
    response_writer->write(http_status, response_ptr);
  }

  int
  Frontend::handle_request_(
    const FrontendCommons::HttpRequest& request,
    FrontendCommons::HttpResponse& response)
    noexcept
  {
    static const char* FUN = "ImprTrack::Frontend::handle_request()";

    logger()->log(String::SubString(
        "ImprTrack::Frontend::handle_request(): entered"),
      TraceLevel::MIDDLE,
      Aspect::IMPR_TRACK_FRONTEND);

    int http_status = 204;

    try
    {
      ImprTrackFrontendHTTPConstrain::apply(request);

      RequestInfo request_info;
      request_info_filler_->fill(request_info, request);

      if(logger()->log_level() >= TraceLevel::MIDDLE)
      {
        std::ostringstream ostr;
        ostr << FUN << ": " << std::endl <<
          "Uri: " << request.uri() << std::endl <<
          "Params ("<< request.params().size() << "):"  << std::endl;

        for(HTTP::ParamList::const_iterator it =
              request.params().begin(); it != request.params().end(); ++it)
        {
          ostr << "    " << it->name << " : " << it->value << std::endl;
        }

        ostr << "Headers ("<< request.headers().size() << "):"  << std::endl;

        for (HTTP::SubHeaderList::const_iterator it =
          request.headers().begin(); it != request.headers().end(); ++it)
        {
          ostr << "    " << it->name << " : " << it->value << std::endl;
        }

        logger()->log(ostr.str(),
          TraceLevel::MIDDLE,
          Aspect::IMPR_TRACK_FRONTEND);
      }

      AdServer::Commons::UserId result_user_id = request_info.actual_user_id;
      [[maybe_unused]] ResultUserIdType result_user_id_type = RUIT_COOKIE;

      std::vector<std::uint32_t> impression_campaign_ids;
      std::vector<std::uint32_t> impression_advertiser_ids;

      if (!request_info.skip)
      {
        if (request_info.user_status == AdServer::CampaignSvcs::US_OPTOUT &&
          !request_info.actual_user_id.is_null())
        {
          throw InvalidParamException("");
        }

        // confirm impression for stats (CampaignManager)
        if(!request_info.request_ids.empty())
        {
          bool is_grpc_success = false;
          const auto& grpc_campaign_manager_pool = grpc_container_->grpc_campaign_manager_pool;
          if (grpc_campaign_manager_pool)
          {
            using TrackCreativeInfo = FrontendCommons::GrpcCampaignManagerPool::TrackCreativeInfo;
            using UserIdHashModInfo = FrontendCommons::GrpcCampaignManagerPool::UserIdHashModInfo;

            try
            {
              std::vector<TrackCreativeInfo> creatives;
              creatives.reserve(request_info.request_ids.size());
              std::size_t ri = 0;
              auto cr_it = request_info.creatives.begin();
              for (auto rit = request_info.request_ids.begin();
                   rit != request_info.request_ids.end();
                   ++rit, ++ri)
              {
                const auto& request_id = *rit;
                // ccg_keyword_id non used now,
                // it can't define specific impression cost (only click)
                const std::uint32_t ccg_keyword_id = 0;
                std::uint32_t ccid = 0;
                AdServer::CampaignSvcs::CTRDecimal ctr =
                  CampaignSvcs::RevenueDecimal::ZERO;
                if (cr_it != std::end(request_info.creatives))
                {
                  ccid = cr_it->ccid;
                  ctr = cr_it->ctr;
                  ++cr_it;
                }

                creatives.emplace_back(
                  ccid,
                  ccg_keyword_id,
                  request_id,
                  ctr);
              }

              auto response = grpc_campaign_manager_pool->verify_impression(
                request_info.time,                                      // time
                request_info.bid_time,                                  // bid_time
                UserIdHashModInfo{request_info.user_id_hash_mod},       // user_id_hash_mod
                creatives,                                              // creatives
                request_info.pub_imp_revenue_type,                      // pub_imp_revenue_type
                request_info.pub_imp_revenue,                           // pub_imp_revenue
                request_info.request_type,                              // request_type
                request_info.verify_type,                               // verify_type
                request_info.actual_user_id,                            // user_id
                request_info.referer,                                   // referer
                request_info.viewability,                               // viewability
                request_info.action_name);                              // action_name
              if (response && response->has_info())
              {
                is_grpc_success = true;
                const auto& info_proto = response->info();
                const auto& creatives_proto = info_proto.creatives();
                impression_campaign_ids.reserve(creatives_proto.size());
                impression_advertiser_ids.reserve(creatives_proto.size());
                for (const auto& creative_proto : creatives_proto)
                {
                  impression_campaign_ids.emplace_back(creative_proto.campaign_id());
                  impression_advertiser_ids.emplace_back(creative_proto.advertiser_id());
                }
              }
            }
            catch (const eh::Exception& exc)
            {
              is_grpc_success = false;
              Stream::Error stream;
              stream << FNS
                     << exc.what();
              logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
            }
            catch (...)
            {
              is_grpc_success = false;
              Stream::Error stream;
              stream << FNS
                     << "Unknown error";
              logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
            }
          }

          if (!is_grpc_success)
          {
            AdServer::CampaignSvcs::CampaignManager::ImpressionInfo
              verify_impression_info;
            verify_impression_info.verify_type = request_info.verify_type;
            verify_impression_info.time = CorbaAlgs::pack_time(request_info.time);
            verify_impression_info.bid_time = CorbaAlgs::pack_time(request_info.bid_time);
            verify_impression_info.pub_imp_revenue_type =
              request_info.pub_imp_revenue_type;
            verify_impression_info.pub_imp_revenue = CorbaAlgs::pack_decimal(
              request_info.pub_imp_revenue);
            verify_impression_info.request_type = request_info.request_type;
            verify_impression_info.user_id = CorbaAlgs::pack_user_id(
              request_info.actual_user_id);
            verify_impression_info.referer << request_info.referer;
            verify_impression_info.viewability = request_info.viewability;
            verify_impression_info.action_name << request_info.action_name;

            if (request_info.user_id_hash_mod)
            {
              verify_impression_info.user_id_hash_mod.defined = true;
              verify_impression_info.user_id_hash_mod.value =
                *request_info.user_id_hash_mod;
            }
            else
            {
              verify_impression_info.user_id_hash_mod.defined = false;
            }

            verify_impression_info.creatives.length(request_info.request_ids.size());
            CORBA::ULong ri = 0;
            RequestInfo::CreativeList::const_iterator cr_it =
              request_info.creatives.begin();
            for (RequestIdList::const_iterator rit = request_info.request_ids.begin();
                 rit != request_info.request_ids.end();
                 ++rit, ++ri)
            {
              auto& cr_info = verify_impression_info.creatives[ri];
              cr_info.request_id = CorbaAlgs::pack_request_id(*rit);
              // ccg_keyword_id non used now,
              // it can't define specific impression cost (only click)
              cr_info.ccg_keyword_id = 0;
              if (cr_it != request_info.creatives.end())
              {
                cr_info.ccid = cr_it->ccid;
                cr_info.ctr = CorbaAlgs::pack_decimal(cr_it->ctr);
                ++cr_it;
              }
              else
              {
                cr_info.ccid = 0;
                cr_info.ctr = CorbaAlgs::pack_decimal(CampaignSvcs::RevenueDecimal::ZERO);
              }
            }

            AdServer::CampaignSvcs::CampaignManager::ImpressionResultInfo_var
              impression_result_info;
            campaign_managers_.verify_impression(verify_impression_info, impression_result_info);

            if (impression_result_info.ptr())
            {
              const auto& creatives = impression_result_info->creatives;
              impression_campaign_ids.reserve(creatives.length());
              impression_advertiser_ids.reserve(creatives.length());
              for (CORBA::ULong i = 0; i < creatives.length(); ++i)
              {
                impression_campaign_ids.emplace_back(creatives[i].campaign_id);
                impression_advertiser_ids.emplace_back(creatives[i].advertiser_id);
              }
            }
          }
        }

        bool invalid_bind_operation = false;
        bool opted_out = (request_info.user_status == AdServer::CampaignSvcs::US_OPTOUT);

        // make user bind operations
        if(opted_out)
        {
          result_user_id = AdServer::Commons::UserId();
        }
        else if(user_bind_client_) // not opt out
        {
          // resolve actual user id (cookies)
          assert(user_bind_client_.in());

          const auto grpc_distributor = user_bind_client_->grpc_distributor();
          bool is_grpc_success = false;
          if (grpc_distributor)
          {
            try
            {
              is_grpc_success = true;
              bool cresolve_failed = false;

              // for apps result_user_id is null
              if (!result_user_id.is_null())
              {
                const std::string cookie_external_id_str =
                  std::string("c/") + result_user_id.to_string();

                auto response = grpc_distributor->get_user_id(
                  cookie_external_id_str,
                  result_user_id,
                  request_info.time,
                  Generics::Time::ZERO,
                  true,
                  false,
                  request_info.set_cookie);
                if (!response || response->has_error())
                {
                  throw Exception ("get_user_id is failed");
                }

                const auto& info_proto = response->info();
                if (info_proto.invalid_operation())
                {
                  cresolve_failed = true;
                  invalid_bind_operation = true;
                  report_bad_user_(request_info);
                }
                else
                {
                  const auto cresolved_user_id =
                    GrpcAlgs::unpack_user_id(info_proto.user_id());
                  if(!cresolved_user_id.is_null())
                  {
                    result_user_id = cresolved_user_id;
                    result_user_id_type = RUIT_CRESOLVE;
                  }
                }

                if(!cresolve_failed)
                {
                  // rebind external user id
                  // optimization: don't do resolve if user id in parameters equal to
                  //   cookies user id
                  // current_user_id passed in parameters
                  // result_user_id from cookie or got by cookie resoving uid
                  if(!(request_info.current_user_id == result_user_id) &&
                    !request_info.external_user_id.empty())
                  {
                    const std::string external_user_id = request_info.external_user_id;

                    if(!result_user_id.is_null())
                    {
                      auto response = grpc_distributor->add_user_id(
                        external_user_id,
                        request_info.time,
                        GrpcAlgs::pack_user_id(result_user_id));
                      if (!response || response->has_error())
                      {
                        throw Exception("add_user_id is failed");
                      }

                      if(response->info().invalid_operation())
                      {
                        invalid_bind_operation = true;
                        report_bad_user_(request_info);
                      }
                    }
                    else
                    {
                      // reconstruct cookie uid by UserBind table
                      // don't use current_user_id - this allow to use ImprTrack frontend
                      // for sign any uid
                      auto response = grpc_distributor->get_user_id(
                        external_user_id,
                        {},
                        request_info.time,
                        Generics::Time::ZERO,
                        true,
                        false,
                        request_info.set_cookie);
                      if (!response || response->has_error())
                      {
                        throw Exception("get_user_id is failed");
                      }

                      const auto& info_proto = response->info();
                      if (info_proto.invalid_operation())
                      {
                        invalid_bind_operation = true;
                        report_bad_user_(request_info);
                      }
                      else
                      {
                        const auto resolved_user_id =
                          GrpcAlgs::unpack_user_id(info_proto.user_id());
                        if(!resolved_user_id.is_null())
                        {
                          result_user_id = resolved_user_id;
                          result_user_id_type = RUIT_EXTIDRESOLVE;
                          common_module_->user_id_controller()->null_blacklisted(result_user_id);
                        }
                      }
                    }
                  }
                }
              }
            }
            catch (const eh::Exception& exc)
            {
              is_grpc_success = false;
              Stream::Error stream;
              stream << FUN
                     << ": "
                     << exc.what();
              logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
            }
            catch (...)
            {
              is_grpc_success = false;
              Stream::Error stream;
              stream << FUN
                     << ": Unknown error";
              logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
            }
          }

          if (!is_grpc_success)
          {
            bool cresolve_failed = false;

            AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper =
              user_bind_client_->user_bind_mapper();

            try
            {
              // for apps result_user_id is null
              if (!result_user_id.is_null())
              {
                const std::string cookie_external_id_str =
                  std::string("c/") + result_user_id.to_string();

                AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo get_request_info;
                get_request_info.id << cookie_external_id_str;
                get_request_info.timestamp = CorbaAlgs::pack_time(request_info.time);
                get_request_info.silent = true;
                get_request_info.generate_user_id = false;
                get_request_info.for_set_cookie = request_info.set_cookie;
                get_request_info.create_timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
                get_request_info.current_user_id = CorbaAlgs::pack_user_id(result_user_id);

                AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo_var prev_user_bind_info =
                  user_bind_mapper->get_user_id(get_request_info);

                if(prev_user_bind_info->invalid_operation)
                {
                  cresolve_failed = true;
                  invalid_bind_operation = true;
                  report_bad_user_(request_info);
                }
                else
                {
                  Commons::UserId cresolved_user_id =
                    CorbaAlgs::unpack_user_id(prev_user_bind_info->user_id);
                  if(!cresolved_user_id.is_null())
                  {
                    result_user_id = CorbaAlgs::unpack_user_id(prev_user_bind_info->user_id);
                    result_user_id_type = RUIT_CRESOLVE;
                  }
                }
              }
            }
            catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady&)
            {
              cresolve_failed = true;
            }
            catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& )
            {
              cresolve_failed = true;
            }
            catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
            {
              cresolve_failed = true;
            }
            catch(const CORBA::SystemException& e)
            {
              cresolve_failed = true;
            }

            if(!cresolve_failed)
            {
              try
              {
                // rebind external user id
                // optimization: don't do resolve if user id in parameters equal to
                //   cookies user id
                // current_user_id passed in parameters
                // result_user_id from cookie or got by cookie resoving uid
                if(!(request_info.current_user_id == result_user_id) &&
                  !request_info.external_user_id.empty())
                {
                  const std::string external_user_id = request_info.external_user_id;

                  if(!result_user_id.is_null())
                  {
                    // result_user_id got from cookie or by cookie resolving
                    AdServer::UserInfoSvcs::UserBindMapper::AddUserRequestInfo
                      add_user_request_info;
                    add_user_request_info.id << external_user_id;
                    add_user_request_info.user_id = CorbaAlgs::pack_user_id(result_user_id);
                    add_user_request_info.timestamp = CorbaAlgs::pack_time(request_info.time);

                    AdServer::UserInfoSvcs::UserBindServer::AddUserResponseInfo_var
                      prev_user_bind_info =
                        user_bind_mapper->add_user_id(add_user_request_info);

                    if(prev_user_bind_info->invalid_operation)
                    {
                      invalid_bind_operation = true;
                      report_bad_user_(request_info);
                    }

                    /* INVALID: use cookie user id
                    result_user_id = CorbaAlgs::unpack_user_id(
                      prev_user_bind_info->merge_user_id);
                    common_module_->user_id_controller()->null_blacklisted(
                      result_user_id);
                    */

                    (void)prev_user_bind_info;
                  }
                  else
                  {
                    // reconstruct cookie uid by UserBind table
                    // don't use current_user_id - this allow to use ImprTrack frontend
                    // for sign any uid
                    AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo get_request_info;
                    get_request_info.id << external_user_id;
                    get_request_info.timestamp = CorbaAlgs::pack_time(request_info.time);
                    get_request_info.silent = true;
                    get_request_info.generate_user_id = false;
                    get_request_info.for_set_cookie = request_info.set_cookie;
                    get_request_info.create_timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
                    // get_request_info.current_user_id is null

                    AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo_var
                      prev_user_bind_info =
                        user_bind_mapper->get_user_id(get_request_info);

                    if(prev_user_bind_info->invalid_operation)
                    {
                      invalid_bind_operation = true;
                      report_bad_user_(request_info);
                    }
                    else
                    {
                      AdServer::Commons::UserId resolved_user_id =
                        CorbaAlgs::unpack_user_id(prev_user_bind_info->user_id);
                      if(!resolved_user_id.is_null())
                      {
                        result_user_id = resolved_user_id;
                        result_user_id_type = RUIT_EXTIDRESOLVE;
                        common_module_->user_id_controller()->null_blacklisted(result_user_id);
                      }
                    }
                  }
                }
              }
              catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady&)
              {
                Stream::Error ostr;
                ostr << FUN << ": caught UserBindServer::NotReady";
                logger()->log(ostr.str(),
                  Logging::Logger::EMERGENCY,
                  Aspect::IMPR_TRACK_FRONTEND,
                  "ADS-IMPL-109");
              }
              catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& )
              {
                Stream::Error ostr;
                ostr << FUN << ": caught UserBindMapper::ChunkNotFound";
                logger()->log(ostr.str(),
                  Logging::Logger::ERROR,
                  Aspect::IMPR_TRACK_FRONTEND,
                  "ADS-IMPL-109");
              }
              catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
              {
                Stream::Error ostr;
                ostr << FUN << ": caught UserBindMapper::ImplementationException: " <<
                  ex.description;
                logger()->log(ostr.str(),
                  Logging::Logger::ERROR,
                  Aspect::IMPR_TRACK_FRONTEND,
                  "ADS-IMPL-109");
              }
              catch(const CORBA::SystemException& e)
              {
                Stream::Error ostr;
                ostr << FUN << ": caught CORBA::SystemException: " << e;
                logger()->log(ostr.str(),
                  Logging::Logger::ERROR,
                  Aspect::IMPR_TRACK_FRONTEND,
                  "ADS-ICON-6");
              }
            }
          }
        } // if(user_bind_client_)

        // merge user ids if required
        if(!invalid_bind_operation &&
           USER_PROFILE_MERGE_ENABLED &&
           !result_user_id.is_null() && // null if user_status is US_OPTOUT
           !request_info.current_user_id.is_null() &&
           !(result_user_id == request_info.current_user_id))
        {
          AdServer::UserInfoSvcs::UserInfoMatcher_var
            uim_session = user_info_client_->user_info_session();
          const auto grpc_distributor = user_info_client_->grpc_distributor();

          bool is_grpc_success = false;
          if (grpc_distributor)
          {
            using UserProfiles = AdServer::UserInfoSvcs::Types::UserProfiles;
            using ProfilesRequestInfo = AdServer::UserInfoSvcs::Types::ProfilesRequestInfo;
            using UserInfo = AdServer::UserInfoSvcs::Types::UserInfo;
            using MatchParams = AdServer::UserInfoSvcs::Types::MatchParams;

            try
            {
              is_grpc_success = true;

              ProfilesRequestInfo profiles_request;
              profiles_request.base_profile = true;
              profiles_request.add_profile = true;
              profiles_request.history_profile = true;
              profiles_request.freq_cap_profile = true;

              auto get_user_profile_response =
                grpc_distributor->get_user_profile(
                  GrpcAlgs::pack_user_id(request_info.current_user_id),
                  false, // persistent profile
                  profiles_request);
              if (!get_user_profile_response || get_user_profile_response->has_error())
              {
                throw Exception(std::string("get_user_profile is failed"));
              }

              const auto& user_profile_response_info =
                get_user_profile_response->info();
              if (user_profile_response_info.return_value())
              {
                UserInfo user_info;
                user_info.user_id = GrpcAlgs::pack_user_id(result_user_id);
                user_info.last_colo_id = request_info.colo_id;
                user_info.request_colo_id = request_info.colo_id;
                user_info.current_colo_id = -1;
                user_info.temporary = false;
                user_info.time = request_info.time.tv_sec;

                MatchParams merge_match_params;
                merge_match_params.use_empty_profile = false;
                merge_match_params.silent_match = false;
                merge_match_params.no_match = false;
                merge_match_params.no_result = false;
                merge_match_params.provide_persistent_channels = false;
                merge_match_params.change_last_request = false;
                merge_match_params.publishers_optin_timeout =
                  Generics::Time::ZERO;

                const auto& user_profiles_proto = user_profile_response_info.user_profiles();

                UserProfiles merge_user_profiles;
                merge_user_profiles.add_user_profile = user_profiles_proto.add_user_profile();
                merge_user_profiles.base_user_profile = user_profiles_proto.base_user_profile();
                merge_user_profiles.pref_profile = user_profiles_proto.pref_profile();
                merge_user_profiles.history_user_profile = user_profiles_proto.history_user_profile();
                merge_user_profiles.freq_cap = user_profiles_proto.freq_cap();

                auto merge_response = grpc_distributor->merge(
                  user_info,
                  merge_match_params,
                  merge_user_profiles);
                if (!merge_response || merge_response->has_error())
                {
                  throw Exception(std::string("merge is failed"));
                }

                auto remove_user_profile_response = grpc_distributor->remove_user_profile(
                  GrpcAlgs::pack_user_id(request_info.current_user_id));
                if (!remove_user_profile_response || remove_user_profile_response->has_error())
                {
                  throw Exception(std::string("remove_user_profile is failed"));
                }
              }
            }
            catch (const eh::Exception& exc)
            {
              is_grpc_success = false;
              Stream::Error stream;
              stream << FUN
                     << ": "
                     << exc.what();
              logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
            }
            catch (...)
            {
              is_grpc_success = false;
              Stream::Error stream;
              stream << FUN
                     << ": Unknown error";
              logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
            }
          }

          if(!is_grpc_success && uim_session.in())
          {
            try
            {
              AdServer::UserInfoSvcs::UserProfiles_var merge_user_profile;

              AdServer::UserInfoSvcs::ProfilesRequestInfo profiles_request;
              profiles_request.base_profile = true;
              profiles_request.add_profile = true;
              profiles_request.history_profile = true;
              profiles_request.freq_cap_profile = true;

              if(uim_session->get_user_profile(
                   CorbaAlgs::pack_user_id(request_info.current_user_id),
                   false, // persistent profile
                   profiles_request,
                   merge_user_profile.out()))
              {
                AdServer::UserInfoSvcs::UserInfo user_info;
                user_info.user_id = CorbaAlgs::pack_user_id(result_user_id);
                user_info.last_colo_id = request_info.colo_id;
                user_info.request_colo_id = request_info.colo_id;
                user_info.current_colo_id = -1;
                user_info.temporary = false;
                user_info.time = request_info.time.tv_sec;

                AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams merge_match_params;
                merge_match_params.use_empty_profile = false;
                merge_match_params.silent_match = false;
                merge_match_params.no_match = false;
                merge_match_params.no_result = false;
                merge_match_params.provide_persistent_channels = false;
                merge_match_params.change_last_request = false;
                merge_match_params.publishers_optin_timeout =
                  CorbaAlgs::pack_time(Generics::Time::ZERO);

                bool merge_success;
                CORBACommons::TimestampInfo_var last_request;

                uim_session->merge(
                  user_info,
                  merge_match_params,
                  merge_user_profile.in(),
                  merge_success,
                  last_request);

                uim_session->remove_user_profile(
                  CorbaAlgs::pack_user_id(request_info.current_user_id));
              }
            }
            catch(const AdServer::UserInfoSvcs::UserInfoMatcher::ImplementationException& e)
            {
              Stream::Error ostr;
              ostr << FUN << ": caught UserInfoMatcher::ImplementationException: " <<
                "user_id = '" << result_user_id.to_string() << "'; " <<
                e.description;
              logger()->log(ostr.str(),
                Logging::Logger::EMERGENCY,
                Aspect::IMPR_TRACK_FRONTEND,
                "ADS-IMPL-7503");
            }
            catch(const AdServer::UserInfoSvcs::UserInfoMatcher::NotReady& e)
            {
              Stream::Error ostr;
              ostr << FUN << ": caught UserInfoMatcher::NotReady.";
              logger()->log(ostr.str(),
                Logging::Logger::EMERGENCY,
                Aspect::IMPR_TRACK_FRONTEND,
                "ADS-IMPL-7504");
            }
            catch(const CORBA::SystemException& e)
            {
              Stream::Error ostr;
              ostr << FUN << ": caught CORBA::SystemException: " << e;
              logger()->log(ostr.str(),
                Logging::Logger::EMERGENCY,
                Aspect::IMPR_TRACK_FRONTEND,
               "ADS-ICON-2");
            }
          }
        }

        // confirm freq cap in merged profile
        AdServer::Commons::UserId freq_cap_user_id =
          !invalid_bind_operation &&
          !result_user_id.is_null() &&
            !(result_user_id == AdServer::Commons::PROBE_USER_ID) ?
          result_user_id :
          request_info.current_user_id;

        if((!request_info.common_request_id.is_null() ||
            !request_info.pubpixel_accounts.empty()) &&
           !freq_cap_user_id.is_null())
        {
          // save freq caps
          AdServer::UserInfoSvcs::UserInfoMatcher_var
            uim_session = user_info_client_->user_info_session();
          const auto grpc_distributor = user_info_client_->grpc_distributor();

          bool is_grpc_success = false;
          if (grpc_distributor)
          {
            using ExcludePubpixelAccounts =
              AdServer::UserInfoSvcs::Types::ExcludePubpixelAccounts;
            try
            {
              is_grpc_success = true;

              ExcludePubpixelAccounts pubpixel_accounts;
              pubpixel_accounts.insert(
                std::end(pubpixel_accounts),
                std::begin(request_info.pubpixel_accounts),
                std::end(request_info.pubpixel_accounts));

              auto response = grpc_distributor->confirm_user_freq_caps(
                GrpcAlgs::pack_user_id(freq_cap_user_id),
                request_info.time,
                GrpcAlgs::pack_request_id(request_info.common_request_id),
                pubpixel_accounts);
              if (!response || response->has_error())
              {
                is_grpc_success = false;
              }
            }
            catch (const eh::Exception& exc)
            {
              is_grpc_success = false;
              Stream::Error stream;
              stream << FUN
                     << ": "
                     << exc.what();
              logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
            }
            catch (...)
            {
              is_grpc_success = false;
              Stream::Error stream;
              stream << FUN
                     << ": Unknown error";
              logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
            }
          }

          if(!is_grpc_success && uim_session.in())
          {
            try
            {
              CORBACommons::IdSeq pubpixel_accounts;
              CorbaAlgs::fill_sequence(
                request_info.pubpixel_accounts.begin(),
                request_info.pubpixel_accounts.end(),
                pubpixel_accounts);

              uim_session->confirm_user_freq_caps(
                CorbaAlgs::pack_user_id(freq_cap_user_id),
                CorbaAlgs::pack_time(request_info.time),
                CorbaAlgs::pack_request_id(request_info.common_request_id),
                pubpixel_accounts);
            }
            catch (const AdServer::UserInfoSvcs::
                   UserInfoMatcher::ImplementationException& e)
            {
              Stream::Error ostr;
              ostr << FUN <<
                ": UserInfoMatcher::ImplementationException caught: " <<
                e.description;

              logger()->log(ostr.str(),
                Logging::Logger::EMERGENCY,
                Aspect::IMPR_TRACK_FRONTEND,
                "ADS-IMPL-123");
            }
            catch (const AdServer::UserInfoSvcs::
                   UserInfoMatcher::NotReady& e)
            {
              Stream::Error ostr;
              ostr << FUN << ": UserInfoMatcher::NotReady caught: " <<
                e.description;

              logger()->log(ostr.str(),
                Logging::Logger::WARNING,
                Aspect::IMPR_TRACK_FRONTEND);
            }
            catch (const CORBA::SystemException& e)
            {
              Stream::Error ostr;
              ostr << FUN << ": CORBA::Exception caught: " << e;

              logger()->log(ostr.str(),
                Logging::Logger::EMERGENCY,
                Aspect::IMPR_TRACK_FRONTEND,
                "ADS-ICON-2");
            }
          }
        }

        // match channels
        if((!result_user_id.is_null() && !(result_user_id == AdServer::Commons::PROBE_USER_ID)) ||
          (!request_info.current_user_id.is_null() && !(request_info.current_user_id == AdServer::Commons::PROBE_USER_ID)))
        {
          try
          {
            const bool is_coroutin_thread =
              userver::engine::current_task::IsTaskProcessorThread();
            if (is_coroutin_thread)
            {
              auto& current_task_processor =
                userver::engine::current_task::GetTaskProcessor();
              // delay match click channels
              userver::engine::AsyncNoSpan(
                current_task_processor,
                [
                  impr_track_frontend = ReferenceCounting::SmartPtr<Frontend>(
                    ReferenceCounting::add_ref(this)),
                  user_id = result_user_id,
                  cookie_user_id = request_info.current_user_id,
                  now = request_info.time,
                  campaign_ids = impression_campaign_ids,
                  advertiser_ids = impression_advertiser_ids,
                  peer_ip = request_info.peer_ip] () {
                    impr_track_frontend->match_channels_(
                      user_id,
                      cookie_user_id,
                      now,
                      campaign_ids,
                      advertiser_ids,
                      peer_ip,
                      {});
                  }).Detach();
            }
            else
            {
              // delay match click channels
              task_runner_->enqueue_task(new MatchChannelsTask(
                this,
                result_user_id,
                request_info.current_user_id,
                request_info.time,
                impression_campaign_ids,
                impression_advertiser_ids,
                request_info.peer_ip,
                std::list<std::string>()));
            }
          }
          catch (const Generics::TaskRunner::Overflow& ex)
          {
            logger()->sstream(
              Logging::Logger::ERROR,
              Aspect::IMPR_TRACK_FRONTEND,
              "ADS-IMPL-198") << FUN <<
              ": the limit of simultaneous matching tasks has been reached: " <<
              ex.what();
          }
        }

        if(request_info.set_cookie &&
          !invalid_bind_operation &&
          !result_user_id.is_null())
        {
          // set new user id
          const Generics::SignedUuid signed_uid =
            common_module_->user_id_controller()->sign(result_user_id);
          FrontendCommons::add_UID_cookie(
            response,
            request,
            *cookie_manager_,
            signed_uid.str());
        }
      } // request_info.skip

      FrontendCommons::CORS::set_headers(request, response);

      if(!request_info.redirect_url.empty())
      {
        const std::string redirect_url_str = FrontendCommons::normalize_abs_url(
          HTTP::BrowserAddress(request_info.redirect_url),
          HTTP::HTTPAddress::VW_FULL,
          FrontendCommons::is_secure_request(request) || request_info.secure ?
            HTTPS_PREFIX : HTTP_PREFIX);

        http_status = FrontendCommons::redirect(redirect_url_str, response);
      }
      else if(request_info.verify_type != AdServer::CampaignSvcs::RVT_NOTICE)
        // don't try to make bind redirect on notice calls
      {
        // find templates that match traits
        TextTemplateArray inst_templates;

        {
          std::vector<std::string> keywords;
          FrontendCommons::get_ip_keywords(keywords, request_info.peer_ip);

          for(auto bind_rule_it = bind_url_rules_.begin(); bind_rule_it != bind_url_rules_.end(); ++bind_rule_it)
          {
            if((*bind_rule_it)->use_keywords)
            {
              for(auto keyword_it = keywords.begin(); keyword_it != keywords.end(); ++keyword_it)
              {
                if((*bind_rule_it)->keywords.find(*keyword_it) != (*bind_rule_it)->keywords.end())
                {
                  inst_templates.push_back((*bind_rule_it)->url_template);
                  break;
                }
              }
            }
            else
            {
              inst_templates.push_back((*bind_rule_it)->url_template);
            }
          }
        }

        std::vector<std::string> bind_urls;

        // prepare BINDURL tokens
        if(!inst_templates.empty())
        {
          char random_str[40];
          unsigned long random = Generics::safe_rand();
          String::StringManip::int_to_str(random, random_str, sizeof(random_str));

          typedef std::map<String::SubString, std::string> ArgMap;
          ArgMap sub_args_cont;
          sub_args_cont[Tokens::RANDOM] = random_str;
          sub_args_cont[Tokens::EXTERNAL_USER_ID] = request_info.external_user_id;
          sub_args_cont[Tokens::SOURCE_ID] = request_info.source_id;
          if(!request_info.current_user_id.is_null())
          {
            sub_args_cont[Tokens::ADD_USER_ID] = request_info.current_user_id.to_string();
          }

          String::TextTemplate::ArgsContainer<ArgMap> args(&sub_args_cont);
          String::TextTemplate::DefaultValue args_with_default(&args);
          String::TextTemplate::ArgsEncoder args_with_encoding(&args_with_default);

          for(auto bind_url_templ_it = inst_templates.begin();
            bind_url_templ_it != inst_templates.end();
            ++bind_url_templ_it)
          {
            bind_urls.push_back((*bind_url_templ_it)->instantiate(args_with_encoding));
          }
        }

        if(request_info.use_template_file)
        {
          try
          {
            // instantiate imp template
            Commons::TextTemplate_var templ = template_files_->get(track_template_file_);

            typedef std::map<String::SubString, std::string> ArgMap;

            ArgMap args_cont;

            String::TextTemplate::ArgsContainer<ArgMap> args(&args_cont);
            String::TextTemplate::DefaultValue args_with_default(&args);
            String::TextTemplate::ArgsEncoder args_with_encoding(&args_with_default);

            unsigned long i = 1;
            char i_str[40];
            for(auto bind_url_it = bind_urls.begin(); bind_url_it != bind_urls.end(); ++bind_url_it, ++i)
            {
              String::StringManip::int_to_str(i, i_str, sizeof(i_str));
              std::string token = Tokens::BIND_URL_TOKEN.str();
              token += i_str;
              args_cont[token] = *bind_url_it;
            }

            std::string response_content = templ->instantiate(args_with_encoding);

            response.set_content_type(FrontendCommons::ContentType::TEXT_HTML);

            response.get_output_stream().write(
              response_content.data(), response_content.size());

            http_status = 200;
          }
          catch(const eh::Exception& ex)
          {
            logger()->sstream(
              Logging::Logger::EMERGENCY,
              Aspect::IMPR_TRACK_FRONTEND,
              "ADS-IMPL-?") <<
              FUN << ": eh::Exception has been caught: " << ex.what();

            http_status = 204;
          }
        }
        else if(!bind_urls.empty())
        {
          const std::string redirect_url_str = FrontendCommons::normalize_abs_url(
            HTTP::BrowserAddress(*bind_urls.begin()),
            HTTP::HTTPAddress::VW_FULL,
            FrontendCommons::is_secure_request(request) || request_info.secure ?
            HTTPS_PREFIX : HTTP_PREFIX);

          http_status = FrontendCommons::redirect(redirect_url_str, response);
        }
        else
        {
          response.set_content_type(track_pixel_content_type_);

          if(common_config_->ResponseHeaders().present())
          {
            FrontendCommons::add_headers(
              *(common_config_->ResponseHeaders()),
              response);
          }

          FileCache::BufferHolder_var buffer = track_pixel_->get();
          response.get_output_stream().write((*buffer)->data(), (*buffer)->size());
        } // request_info.redirect_url.empty()
      }
    }
    catch (const ForbiddenException& ex)
    {
      http_status = 403;
      logger()->sstream(TraceLevel::LOW, Aspect::IMPR_TRACK_FRONTEND) <<
        FUN << ": ForbiddenException caught: " << ex.what();
    }
    catch (const InvalidParamException& ex)
    {
      http_status = 400;
      logger()->sstream(TraceLevel::MIDDLE, Aspect::IMPR_TRACK_FRONTEND) <<
        FUN << ": InvalidParamException caught: " << ex.what();
    }
    catch(const eh::Exception& e)
    {
      http_status = 500;
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();

      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::IMPR_TRACK_FRONTEND,
        "ADS-IMPL-134");
    }

    return http_status;
  }

  RequestInfoFiller::EncryptionKeys_var
  Frontend::read_keys_(
    const xsd::AdServer::Configuration::EncryptionKeysType& src)
    /*throw(eh::Exception)*/
  {
    RequestInfoFiller::EncryptionKeys_var res = new RequestInfoFiller::EncryptionKeys();

    if(src.openx_encryption_key().present())
    {
      res->openx_encryption_key_size = String::StringManip::hex_decode(
        *src.openx_encryption_key(), res->openx_encryption_key);
    }

    if(src.openx_integrity_key().present())
    {
      res->openx_integrity_key_size = String::StringManip::hex_decode(
        *src.openx_integrity_key(), res->openx_integrity_key);
    }

    if(src.google_encryption_key().present())
    {
      res->google_encryption_key_size = String::StringManip::hex_decode(
        *src.google_encryption_key(), res->google_encryption_key);
    }

    if(src.google_integrity_key().present())
    {
      res->google_integrity_key_size = String::StringManip::hex_decode(
        *src.google_integrity_key(), res->google_integrity_key);
    }

    return res;
  }

  void
  Frontend::report_bad_user_(
    const RequestInfo& request_info)
    noexcept
  {
    bool is_grpc_success = false;
    const auto& grpc_campaign_manager_pool = grpc_container_->grpc_campaign_manager_pool;
    if (grpc_campaign_manager_pool)
    {
      try
      {
        auto response = grpc_campaign_manager_pool->consider_web_operation(
          request_info.time,                                          // timecolo_id
          request_info.colo_id,                                       // colo_id
          0,                                                          // tag_id
          0,                                                          // cc_id
          {},                                                         // ct
          request_info.external_user_id,                              // curct
          {},                                                         // browser
          {},                                                         // os
          WebStats::APPLICATION.str(),                                // app
          WebStats::SOURCE.str(),                                     // source
          WebStats::INVALID_MAPPING_OPERATION.str(),                  // operation
          request_info.source_id,                                     // user_bind_src
          'F',                                                        // result
          static_cast<std::uint32_t>(request_info.user_status),       // user_status
          false,                                                      // test_request
          {},                                                         // request_ids
          {},                                                         // global_request_id
          {},                                                         // referer
          {},                                                         // ip_address
          {},                                                         // external_user_id
          {});                                                        // user_agent
        if (response && response->has_info())
        {
          is_grpc_success = true;
        }
      }
      catch (const eh::Exception &exc)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
      }
      catch (...)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FNS
               << "Unknown error";
        logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
      }
    }

    if (is_grpc_success)
    {
      return;
    }

    try
    {
      AdServer::CampaignSvcs::CampaignManager::WebOperationInfo web_op;
      web_op.time = CorbaAlgs::pack_time(request_info.time);
      web_op.colo_id = request_info.colo_id;
      web_op.tag_id = 0;
      web_op.cc_id = 0;
      web_op.curct << request_info.external_user_id;
      web_op.app << WebStats::APPLICATION;
      web_op.source << WebStats::SOURCE;
      web_op.operation << WebStats::INVALID_MAPPING_OPERATION;
      web_op.user_bind_src << request_info.source_id;
      web_op.result = 'F';
      web_op.user_status = static_cast<unsigned long>(request_info.user_status);
      web_op.test_request = false;

      campaign_managers_.consider_web_operation(web_op);
    }
    catch(const AdServer::CampaignSvcs::CampaignManager::IncorrectArgument&)
    {}
    catch(const eh::Exception&)
    {}
  }

  void
  Frontend::match_channels_(
    const AdServer::Commons::UserId& user_id,
    const AdServer::Commons::UserId& cookie_user_id,
    const Generics::Time& now,
    const std::vector<CORBA::ULong>& campaign_ids,
    const std::vector<CORBA::ULong>& advertiser_ids,
    const String::SubString& peer_ip,
    const std::list<std::string>& // markers
    ) noexcept
  {
    static const char* FUN = "ClickFrontend::match_channels_()";

    std::vector<ChannelMatch> trigger_match_result_page_channels;

    // do trigger match
    bool is_grpc_success = false;
    const auto& grpc_channel_operation_pool = grpc_container_->grpc_channel_operation_pool;
    if (grpc_channel_operation_pool)
    {
      try
      {
        std::ostringstream keywords_ostr;
        keywords_ostr << "poadimp";

        for (auto campaign_id_it = campaign_ids.begin(); campaign_id_it != campaign_ids.end(); ++campaign_id_it)
        {
          keywords_ostr << " poadimpc" << *campaign_id_it;
        }

        for (auto advertiser_id_it = advertiser_ids.begin(); advertiser_id_it != advertiser_ids.end(); ++advertiser_id_it)
        {
          keywords_ostr << " poadimpa" << *advertiser_id_it;
        }

        auto response = grpc_channel_operation_pool->match(
          {},
          {},
          {},
          {},
          {},
          keywords_ostr.str(),
          {},
          {},
          {'A', '\0'},
          false,
          false,
          false,
          false,
          false);
        if (response && response->has_info())
        {
          is_grpc_success = true;
          const auto& info_proto = response->info();
          const auto& matched_channels_proto = info_proto.matched_channels();
          const auto& page_channels_proto = matched_channels_proto.page_channels();

          trigger_match_result_page_channels.reserve(page_channels_proto.size());
          for (const auto& data : page_channels_proto)
          {
            trigger_match_result_page_channels.emplace_back(
              data.id(),
              data.trigger_channel_id());
          }
        }
      }
      catch (const eh::Exception& exc)
      {
        trigger_match_result_page_channels.clear();

        is_grpc_success = false;
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
      }
      catch (...)
      {
        trigger_match_result_page_channels.clear();

        is_grpc_success = false;
        Stream::Error stream;
        stream << FNS
               << "Unknown error";
        logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
      }
    }

    if (!is_grpc_success)
    {
      try
      {
        AdServer::ChannelSvcs::ChannelServerBase::MatchQuery query;
        query.non_strict_word_match = false;
        query.non_strict_url_match = false;
        query.return_negative = false;
        query.simplify_page = false;
        query.statuses[0] = 'A';
        query.statuses[1] = '\0';
        query.fill_content = false;
        std::ostringstream keywords_ostr;
        keywords_ostr << "poadimp";

        for(auto campaign_id_it = campaign_ids.begin(); campaign_id_it != campaign_ids.end(); ++campaign_id_it)
        {
          keywords_ostr << " poadimpc" << *campaign_id_it;
        }

        for(auto advertiser_id_it = advertiser_ids.begin(); advertiser_id_it != advertiser_ids.end(); ++advertiser_id_it)
        {
          keywords_ostr << " poadimpa" << *advertiser_id_it;
        }

        query.pwords << keywords_ostr.str();

        AdServer::ChannelSvcs::ChannelServerBase::MatchResult_var trigger_match_result;
        channel_servers_->match(query, trigger_match_result);

        if (trigger_match_result.ptr())
        {
          const auto& page_channels = trigger_match_result->matched_channels.page_channels;
          const std::size_t size = page_channels.length();
          trigger_match_result_page_channels.reserve(size);
          for (std::size_t i = 0; i < size; ++i)
          {
            const auto& page_channel = page_channels[i];
            trigger_match_result_page_channels.emplace_back(
              page_channel.id,
              page_channel.trigger_channel_id);
          }
        }
      }
      catch(const FrontendCommons::ChannelServerSessionPool::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": caught ChannelServerSessionPool::Exception: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::IMPR_TRACK_FRONTEND,
          "ADS-IMPL-117");
      }
    }

    // resolve actual user id (cookies)
    AdServer::Commons::UserId resolved_cookie_user_id = cookie_user_id;

    assert(user_bind_client_.in());

    const auto grpc_distributor = user_bind_client_->grpc_distributor();
    is_grpc_success = false;
    if (grpc_distributor)
    {
      // resolve cookie user id
      try
      {
        is_grpc_success = true;

        if(!cookie_user_id.is_null() && user_id != cookie_user_id)
        {
          const std::string cookie_external_id_str =
            std::string("c/") + cookie_user_id.to_string();

          auto response = grpc_distributor->get_user_id(
            cookie_external_id_str,
            cookie_user_id,
            now,
            Generics::Time::ZERO,
            true,
            false,
            false);

          if (response && response->has_info())
          {
            const auto& info_proto = response->info();
            resolved_cookie_user_id = GrpcAlgs::unpack_user_id(info_proto.user_id());
          }
          else
          {
            is_grpc_success = false;
          }
        }
      }
      catch (const eh::Exception& exc)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FUN
               << ": "
               << exc.what();
        logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
      }
      catch (...)
      {
        is_grpc_success = false;
        Stream::Error stream;
        stream << FUN
               << ": Unknown error";
        logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
      }
    }

    if (!is_grpc_success)
    {
      AdServer::UserInfoSvcs::UserBindMapper_var user_bind_mapper =
        user_bind_client_->user_bind_mapper();

      // resolve cookie user id
      try
      {
        // resolve cookie user id only if user id in params not equal to cookie user id
        if(!cookie_user_id.is_null() && user_id != cookie_user_id)
        {
          const std::string cookie_external_id_str =
            std::string("c/") + cookie_user_id.to_string();

          AdServer::UserInfoSvcs::UserBindMapper::GetUserRequestInfo get_request_info;
          get_request_info.id << cookie_external_id_str;
          get_request_info.timestamp = CorbaAlgs::pack_time(now);
          get_request_info.silent = true;
          get_request_info.generate_user_id = false;
          get_request_info.for_set_cookie = false;
          get_request_info.create_timestamp = CorbaAlgs::pack_time(Generics::Time::ZERO);
          get_request_info.current_user_id = CorbaAlgs::pack_user_id(cookie_user_id);

          AdServer::UserInfoSvcs::UserBindServer::GetUserResponseInfo_var prev_user_bind_info =
            user_bind_mapper->get_user_id(get_request_info);

          resolved_cookie_user_id = CorbaAlgs::unpack_user_id(prev_user_bind_info->user_id);
        }
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::NotReady&)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindServer::NotReady";
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::IMPR_TRACK_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::ChunkNotFound& )
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindMapper::ChunkNotFound";
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::IMPR_TRACK_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const AdServer::UserInfoSvcs::UserBindMapper::ImplementationException& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught UserBindMapper::ImplementationException: " <<
          ex.description;
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::IMPR_TRACK_FRONTEND,
          "ADS-IMPL-109");
      }
      catch(const CORBA::SystemException& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught CORBA::SystemException: " << e;
        logger()->log(ostr.str(),
          Logging::Logger::ERROR,
          Aspect::IMPR_TRACK_FRONTEND,
          "ADS-ICON-6");
      }
    }

    // do history match
    std::vector<std::uint32_t> history_match_result_channel_ids;
    if (!trigger_match_result_page_channels.empty())
    {
      AdServer::UserInfoSvcs::UserInfoMatcher_var
        uim_session = user_info_client_->user_info_session();
      const auto grpc_distributor = user_info_client_->grpc_distributor();

      is_grpc_success = false;
      if (grpc_distributor)
      {
        using ChannelMatchSet = std::set<ChannelMatch>;
        using MatchParams = AdServer::UserInfoSvcs::Types::MatchParams;
        using UserInfo = AdServer::UserInfoSvcs::Types::UserInfo;

        try
        {
          is_grpc_success = true;

          // call UIM only if any page channel matched
          MatchParams match_params;
          match_params.use_empty_profile = false;
          match_params.silent_match = false;
          match_params.no_match = false;
          match_params.no_result = true;
          match_params.ret_freq_caps = false;
          match_params.provide_channel_count = false;
          match_params.provide_persistent_channels = false;
          match_params.change_last_request = false;
          match_params.filter_contextual_triggers = false;
          match_params.publishers_optin_timeout = Generics::Time::ZERO;

          ChannelMatchSet page_channels;
          std::copy(
            std::begin(trigger_match_result_page_channels),
            std::end(trigger_match_result_page_channels),
            std::inserter(page_channels, page_channels.end()));

          match_params.page_channel_ids.reserve(page_channels.size());
          for (const auto& page_channel : page_channels)
          {
            match_params.page_channel_ids.emplace_back(
              page_channel.channel_id,
              page_channel.channel_trigger_id);
          }

          UserInfo user_info;
          user_info.user_id = GrpcAlgs::pack_user_id(user_id);
          user_info.last_colo_id = -1;
          user_info.request_colo_id = common_config_->colo_id();
          user_info.current_colo_id = -1;
          user_info.temporary = false;
          user_info.time = now.tv_sec;

          if (user_id != AdServer::Commons::PROBE_USER_ID)
          {
            user_info.user_id = GrpcAlgs::pack_user_id(user_id);
            auto response = grpc_distributor->match(
              user_info,
              match_params);
            if (!response || response->has_error())
            {
              throw Exception(std::string("match is failed"));
            }
          }

          if (user_id != resolved_cookie_user_id && !resolved_cookie_user_id.is_null())
          {
            user_info.user_id = GrpcAlgs::pack_user_id(resolved_cookie_user_id);
            auto response = grpc_distributor->match(
              user_info,
              match_params);
            if (!response || response->has_error())
            {
              throw Exception(std::string("match is failed"));
            }
          }
        }
        catch (const eh::Exception& exc)
        {
          is_grpc_success = false;
          Stream::Error stream;
          stream << FUN
                 << ": "
                 << exc.what();
          logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
        }
        catch (...)
        {
          is_grpc_success = false;
          Stream::Error stream;
          stream << FUN
                 << ": Unknown error";
          logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
        }
      }

      if (!is_grpc_success)
      {
        try
        {
          // call UIM only if any page channel matched
          AdServer::UserInfoSvcs::UserInfoMatcher::MatchParams match_params;
          match_params.use_empty_profile = false;
          match_params.silent_match = false;
          match_params.no_match = false;
          match_params.no_result = true;
          match_params.ret_freq_caps = false;
          match_params.provide_channel_count = false;
          match_params.provide_persistent_channels = false;
          match_params.change_last_request = false;
          match_params.filter_contextual_triggers = false;
          match_params.publishers_optin_timeout =
            CorbaAlgs::pack_time(Generics::Time::ZERO);
        
          typedef std::set<ChannelMatch> ChannelMatchSet;
          ChannelMatchSet page_channels;
          std::copy(
            std::begin(trigger_match_result_page_channels),
            std::end(trigger_match_result_page_channels),
            std::inserter(page_channels, page_channels.end()));

          match_params.page_channel_ids.length(page_channels.size());
          CORBA::ULong res_ch_i = 0;
          for (ChannelMatchSet::const_iterator ch_it = page_channels.begin();
               ch_it != page_channels.end(); ++ch_it, ++res_ch_i)
          {
            match_params.page_channel_ids[res_ch_i].channel_id = ch_it->channel_id;
            match_params.page_channel_ids[res_ch_i].channel_trigger_id =
              ch_it->channel_trigger_id;
          }

          AdServer::UserInfoSvcs::UserInfo user_info;
          user_info.user_id = CorbaAlgs::pack_user_id(user_id);
          user_info.last_colo_id = -1;
          user_info.request_colo_id = common_config_->colo_id();
          user_info.current_colo_id = -1;
          user_info.temporary = false;
          user_info.time = now.tv_sec;

          if (user_id != AdServer::Commons::PROBE_USER_ID)
          {
            AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var history_match_result;
            user_info.user_id = CorbaAlgs::pack_user_id(user_id);
            uim_session->match(
              user_info,
              match_params,
              history_match_result.out());

            if (history_match_result.ptr())
            {
              const auto length = history_match_result->channels.length();
              history_match_result_channel_ids.reserve(length);
              for (CORBA::ULong i = 0; i < length; ++i)
              {
                history_match_result_channel_ids.emplace_back(
                  history_match_result->channels[i].channel_id);
              }
            }
          }

          if (user_id != resolved_cookie_user_id && !resolved_cookie_user_id.is_null())
          {
            user_info.user_id = CorbaAlgs::pack_user_id(resolved_cookie_user_id);
            AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var local_history_match_result;
            uim_session->match(
              user_info,
              match_params,
              local_history_match_result.out());
          }
        }
        catch(const UserInfoSvcs::UserInfoMatcher::ImplementationException& e)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": UserInfoSvcs::UserInfoMatcher::ImplementationException caught: " <<
            e.description;

          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::IMPR_TRACK_FRONTEND,
            "ADS-IMPL-112");
        }
        catch(const UserInfoSvcs::UserInfoMatcher::NotReady& e)
        {
          logger()->log(
            String::SubString("UserInfoManager not ready for matching."),
            TraceLevel::MIDDLE,
            Aspect::IMPR_TRACK_FRONTEND);
        }
        catch(const CORBA::SystemException& ex)
        {
          Stream::Error ostr;
          ostr << FUN <<
            ": Can't match history channels. Caught CORBA::SystemException: " <<
            ex;

          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::IMPR_TRACK_FRONTEND,
            "ADS-ICON-2");
        }
      }

      bool is_grpc_success = false;
      const auto& grpc_campaign_manager_pool = grpc_container_->grpc_campaign_manager_pool;
      if (grpc_campaign_manager_pool)
      {
        using ChannelTriggerMatchInfo = FrontendCommons::GrpcCampaignManagerPool::ChannelTriggerMatchInfo;
        using GeoInfo = FrontendCommons::GrpcCampaignManagerPool::GeoInfo;

        try
        {
          std::vector<ChannelTriggerMatchInfo> pkw_channels;
          const auto pkw_channels_size = trigger_match_result_page_channels.size();
          pkw_channels.reserve(pkw_channels_size);
          for (const auto& trigger_match_page_channel : trigger_match_result_page_channels)
          {
            pkw_channels.emplace_back(
              trigger_match_page_channel.channel_trigger_id,
              trigger_match_page_channel.channel_id);
          }

          std::vector<GeoInfo> geo_infos;
          if (!peer_ip.empty() && ip_map_.get())
          {
            try
            {
              GeoIPMapping::IPMapCity2::CityLocation geo_location;

              if(ip_map_->city_location_by_addr(
                peer_ip.str().c_str(),
                geo_location,
                false))
              {
                FrontendCommons::Location_var location = new FrontendCommons::Location();
                location->country = geo_location.country_code.str();
                geo_location.region.assign_to(location->region);
                location->city = geo_location.city.str();
                location->normalize();

                geo_infos.emplace_back(
                  location->country,
                  location->region,
                  location->city);
              }
            }
            catch(const eh::Exception&)
            {
            }
          }

          auto response = grpc_campaign_manager_pool->process_match_request(
            user_id,                                        // user_id
            {},                                             // household_id
            now,                                            // request_time
            {},                                             // source
            history_match_result_channel_ids,               // channels
            pkw_channels,                                   // pkw_channels
            {},                                             // hid_channels
            common_config_->colo_id(),                      // colo_id
            geo_infos,                                      // location
            {},                                             // coord_location
            {});                                            // full_referer
          if (response && response->has_info())
          {
            is_grpc_success = true;
          }
        }
        catch (const eh::Exception& exc)
        {
          is_grpc_success = false;
          Stream::Error stream;
          stream << FNS
                 << exc.what();
          logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
        }
        catch (...)
        {
          is_grpc_success = false;
          Stream::Error stream;
          stream << FNS
                 << "Unknown error";
          logger()->error(stream.str(), Aspect::IMPR_TRACK_FRONTEND);
        }
      }

      if (is_grpc_success)
      {
        return;
      }

      try
      {
        AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo request_info;
        fill_match_request_info_(
          request_info,
          user_id,
          now,
          trigger_match_result_page_channels,
          history_match_result_channel_ids,
          peer_ip);

        campaign_managers_.process_match_request(request_info);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't process match request. "
          "Possible problem with Campaignmanager. Caught Exception: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::IMPR_TRACK_FRONTEND,
          "ADS-ICON-4");
      }
    } // trigger_match_result.ptr() != 0 && trigger_match_result->matched_channels.page_channels.length() != 0
  }

  void
  Frontend::fill_match_request_info_(
    AdServer::CampaignSvcs::CampaignManager::MatchRequestInfo& mri,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& now,
    const std::vector<ChannelMatch>& trigger_match_result_page_channels,
    const std::vector<std::uint32_t>& history_match_result_channel_ids,
    const String::SubString& peer_ip_val)
    const noexcept
  {
    /*
      Don't fill:
        mri.household_id
        mri.match_info.hid_channels
        mri.match_info.coord_location
    */

    mri.match_info.colo_id = common_config_->colo_id();
    mri.user_id = CorbaAlgs::pack_user_id(user_id);
    mri.request_time = CorbaAlgs::pack_time(now);

    CORBA::ULong result_len = trigger_match_result_page_channels.size();
    mri.match_info.pkw_channels.length(result_len);
    for (CORBA::ULong i = 0; i < result_len; ++i)
    {
      mri.match_info.pkw_channels[i].channel_id =
        trigger_match_result_page_channels[i].channel_id;
      mri.match_info.pkw_channels[i].channel_trigger_id =
        trigger_match_result_page_channels[i].channel_trigger_id;
    }

    result_len = history_match_result_channel_ids.size();
    mri.match_info.channels.length(result_len);
    for (CORBA::ULong i = 0; i < result_len; ++i)
    {
      mri.match_info.channels[i] = history_match_result_channel_ids[i];
    }

    if (!peer_ip_val.empty() && ip_map_.get())
    {
      try
      {
        GeoIPMapping::IPMapCity2::CityLocation geo_location;

        if(ip_map_->city_location_by_addr(
             peer_ip_val.str().c_str(),
             geo_location,
             false))
        {
          FrontendCommons::Location_var location = new FrontendCommons::Location();
          location->country = geo_location.country_code.str();
          geo_location.region.assign_to(location->region);
          location->city = geo_location.city.str();
          location->normalize();

          FrontendCommons::fill_geo_location_info(
            mri.match_info.location,
            location);
        }
      }
      catch(const eh::Exception&)
      {
      }
    }
  }
}
}
