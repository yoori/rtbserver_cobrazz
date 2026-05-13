#include <Logger/StreamLogger.hpp>
#include <HTTP/HTTPCookie.hpp>

#include <utility>
#include <Generics/Uuid.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>

#include <Commons/ConfigUtils.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/ExternalUserIdUtils.hpp>
#include <Commons/Base32.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/JsonFormatter.hpp>
#include <UserInfoSvcs/UserInfoClient/UserInfoGrpcAlgs.hpp>

#include <Frontends/CommonModule/CommonModule.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/add_UID_cookie.hpp>

#include <Frontends/FrontendCommons/UserInfoClientConfig.hpp>

#include "UserBindFrontend.hpp"

namespace Config
{
  const char ENABLE[] = "UserBindFrontend_Enable";
  const char CONFIG_FILES[] = "UserBindFrontend_Config";
  const char CONFIG_FILE[] = "UserBindFrontend_ConfigFile";
}

namespace Aspect
{
  const char USER_BIND_FRONTEND[] = "UserBindFrontend";
}

namespace UrlPath
{
  const String::SubString kGetUserId("/get_user_id");
  const String::SubString kGetSegments("/segments");
}

namespace Response
{
  namespace Type
  {
    const String::SubString JSON("application/json");
  }
}

namespace TemplateParams
{
  const String::SubString MARKER("##");
  const String::SubString SIGNEDUID("SIGNEDUID");
  const String::SubString SIGNEDCOOKIEUID("SIGNEDCOOKIEUID");
  const String::SubString SSPUID("SSPUID");
  const String::SubString UNSIGNEDSSPUID("UNSIGNEDSSPUID");
  const String::SubString UNSIGNEDUID("UNSIGNEDUID");
  const String::SubString EXTERNALID("EXTERNALID");
  const String::SubString SHORTEXTERNALID("SHORTEXTERNALID");
  const String::SubString YANDEXSIGN("YANDEXSIGN");
  const String::SubString PASSBACK_URL("PASSBACK_URL");
  const String::SubString RANDOM("RANDOM");
  const String::SubString PUSH_DATA("PUSH_DATA");
  const String::SubString BINDREQUESTID("BINDREQUESTID");
  const String::SubString LOCATION("LOCATION");
}

namespace
{
  const String::SubString HTTP_PREFIX("http:");
  const String::SubString HTTPS_PREFIX("https:");
  const String::SubString IDFA_KNOWN_KEYWORD("rtbidfaknown");

  const String::SubString GOOGLE_ERRORS[] = {
    String::SubString(
      "User has a Google cookie, but has opted "
      "out of any tracking using this cookie"),
    String::SubString(
      "No valid operations specified. e.g., a "
      "no-op request was received"),
    String::SubString(
      "User does not have a Google cookie. "
      "Google will not set the cookie via the "
      "Cookie Matching Service"),
    String::SubString(
      "Conflicting operations specified. You are "
      "not allowed to specify both the google_push "
      "and google_cm flags on the same request since "
      "they have conflicting purposes"),
    String::SubString(
      "An invalid google_push parameter was passed in "
      "a redirect to a Google server as part of a Pixel "
      "Matching Service request. Your redirect must set "
      "google_push to the same value passed to you in "
      "the initial pixel request")
  };

  struct ChannelMatch
  {
    ChannelMatch(unsigned long channel_id_val,
                 unsigned long channel_trigger_id_val)
      :
      channel_id(channel_id_val),
      channel_trigger_id(channel_trigger_id_val)
    {}

    bool operator<(const ChannelMatch& right) const
    {
      return
        (channel_id < right.channel_id ||
         (channel_id == right.channel_id &&
          channel_trigger_id < right.channel_trigger_id));
    }

    unsigned long channel_id;
    unsigned long channel_trigger_id;
  };

  struct GetChannelTriggerId
  {
    ChannelMatch
    operator() (
      const adserver::channel_svcs::channel_server::ChannelAtom& atom)
      noexcept
    {
      return ChannelMatch(atom.id(), atom.trigger_channel_id());
    }
  };

  /*
  void google_error(
    Stream::Error& error_strm,
    unsigned long error_code)
  {
    error_strm << "'" << error_code << "': ";
    if (error_code <= sizeof(GOOGLE_ERRORS) / sizeof(GOOGLE_ERRORS[0]) &&
        error_code > 0)
    {
      error_strm << GOOGLE_ERRORS[error_code - 1];
    }
    else
    {
      error_strm << "unknown";
    }
  }
  */

  namespace WebStats
  {
    const String::SubString APPLICATION("adserver");
    const String::SubString SOURCE("userbind");
    const String::SubString OPERATION("mapping");
    const String::SubString INVALID_MAPPING_OPERATION("invalid-mapping");
  }
}

namespace AdServer
{
  enum ResultUserIdType
  {
    RUIT_COOKIE,
    RUIT_CRESOLVE,
    RUIT_EXTIDRESOLVE,
    RUIT_EXTIDRESOLVE_NOCOOKIE
  };

  class KeyArgsCallback: public String::TextTemplate::ArgsCallback
  {
    virtual bool
    get_argument(const String::SubString& key, std::string& result,
      bool /*value*/) const /*throw(eh::Exception)*/
    {
      key.assign_to(result);
      return true;
    }
  };

  struct UserBindFrontend::BindResult
  {
    BindResult() {}

    BindResult(const AdServer::Commons::UserId& result_user_id_val)
      : result_user_id(result_user_id_val)
    {}

    AdServer::Commons::UserId ssp_user_id; // will be used for init redirect template
    AdServer::Commons::UserId result_user_id;
  };

  class UserBindFrontend::BindProcessingState:
    public std::enable_shared_from_this<BindProcessingState>
  {
  public:
    BindProcessingState(
      UserBindFrontend* frontend_val,
      UserBind::RequestInfo_var request_info_val,
      std::string dns_bind_request_id_val,
      ProcessRequestCallback callback_val)
      : frontend(frontend_val),
        request_info(std::move(request_info_val)),
        dns_bind_request_id(std::move(dns_bind_request_id_val)),
        callback(std::move(callback_val)),
        http_status(200),
        result_user_id(request_info->user_id),
        result_user_id_type(RUIT_COOKIE),
        app_request(request_info->external_id.compare(0, 4, "ifa/") == 0),
        opted_out(request_info->user_status == AdServer::CampaignSvcs::US_OPTOUT),
        cresolve_failed(false),
        create_user_profile(false),
        resolved_ext_user_i(0)
    {
      if(app_request)
      {
        result_user_id = AdServer::Commons::UserId();
      }
    }

    void start() noexcept
    {
      if(request_info->delete_op)
      {
        run_delete();
        return;
      }

      FrontendCommons::CountryFilter_var country_filter =
        frontend->common_module_->country_filter();
      if(country_filter.in() && (
          request_info->location.in() == 0 ||
          !country_filter->enabled(request_info->location->country)))
      {
        finish_result();
        return;
      }

      if(!result_user_id.is_null() && frontend->user_bind_client_)
      {
        const std::string cookie_external_id_str =
          std::string("c/") + result_user_id.to_string();

        adserver::user_info_svcs::user_bind::GetUserIdRequest get_request;
        get_request.set_id(cookie_external_id_str);
        get_request.set_timestamp(GrpcAlgs::pack_time(request_info->time));
        get_request.set_silent(true);
        get_request.set_generate_user_id(false);
        get_request.set_for_set_cookie(true);
        get_request.set_create_timestamp(
          GrpcAlgs::pack_time(Generics::Time::ZERO));
        get_request.set_current_user_id(
          GrpcAlgs::pack_user_id(result_user_id));

        auto self = shared_from_this();
        frontend->user_bind_client_->get_user_id(
          get_request,
          [self](
            const grpc::Status& status,
            const adserver::user_info_svcs::user_bind::GetUserIdResponse&
              response)
          {
            self->frontend->workers_->post(
              [self, status, response]()
              {
                if(status.ok())
                {
                  if(response.invalid_operation())
                  {
                    self->cresolve_failed = true;
                  }
                  else
                  {
                    Commons::UserId cresolved_user_id =
                      GrpcAlgs::unpack_user_id(response.user_id());
                    if(!cresolved_user_id.is_null())
                    {
                      self->result_user_id = cresolved_user_id;
                      self->result_user_id_type = RUIT_CRESOLVE;
                    }
                  }
                }
                else
                {
                  self->cresolve_failed = true;
                  self->http_status = 500;
                }
                self->after_cookie_resolve();
              });
          });
      }
      else
      {
        if(!result_user_id.is_null() && !frontend->user_bind_client_)
        {
          cresolve_failed = true;
        }
        after_cookie_resolve();
      }
    }

  private:
    struct ExternalId
    {
      std::string id;
      bool set_uid;
      bool can_be_in_cookie;
    };

    void run_delete() noexcept
    {
      if(request_info->external_id.empty() || !frontend->user_bind_client_)
      {
        finish(BindResult(request_info->user_id));
        return;
      }

      adserver::user_info_svcs::user_bind::AddUserIdRequest add_user_request;
      add_user_request.set_id(request_info->external_id);
      add_user_request.set_user_id(GrpcAlgs::pack_user_id(Commons::UserId()));
      add_user_request.set_timestamp(GrpcAlgs::pack_time(request_info->time));

      auto self = shared_from_this();
      frontend->user_bind_client_->add_user_id(
        add_user_request,
        [self](
          const grpc::Status& status,
          const adserver::user_info_svcs::user_bind::AddUserIdResponse&)
        {
          self->frontend->workers_->post(
            [self, status]()
            {
              self->finish(BindResult(self->request_info->user_id),
                status.ok() ? 204 : 500);
            });
        });
    }

    void after_cookie_resolve() noexcept
    {
      if(cresolve_failed)
      {
        finish_result();
        return;
      }

      if(frontend->user_bind_client_)
      {
        external_ids = {
          ExternalId{request_info->ga_user_id, false, true},
          ExternalId{request_info->gclu_user_id, false, true},
          ExternalId{request_info->ym_user_id, false, true},
          ExternalId{request_info->external_id, frontend->config_->set_uid(), !app_request}
        };

        resolved_ext_user_i = external_ids.size();
        if(result_user_id.is_null())
        {
          resolve_external_id(0);
          return;
        }
      }

      add_external_id(0);
    }

    void resolve_external_id(std::size_t index) noexcept
    {
      for(; index < external_ids.size(); ++index)
      {
        if(!external_ids[index].id.empty())
        {
          break;
        }
      }

      if(index >= external_ids.size())
      {
        add_external_id(0);
        return;
      }

      adserver::user_info_svcs::user_bind::GetUserIdRequest get_request;
      get_request.set_id(external_ids[index].id);
      get_request.set_timestamp(GrpcAlgs::pack_time(request_info->time));
      get_request.set_silent(true);
      get_request.set_generate_user_id(external_ids[index].set_uid);
      get_request.set_for_set_cookie(!app_request);
      get_request.set_create_timestamp(
        GrpcAlgs::pack_time(Generics::Time::ZERO));

      auto self = shared_from_this();
      frontend->user_bind_client_->get_user_id(
        get_request,
        [self, index](
          const grpc::Status& status,
          const adserver::user_info_svcs::user_bind::GetUserIdResponse&
            response)
        {
          self->frontend->workers_->post(
            [self, index, status, response]()
            {
              if(status.ok())
              {
                if(response.invalid_operation())
                {
                  self->resolve_failed_external_ids.insert(
                    self->external_ids[index].id);
                  self->frontend->report_bad_user_(*self->request_info);
                }
                else
                {
                  AdServer::Commons::UserId resolved_user_id =
                    GrpcAlgs::unpack_user_id(response.user_id());
                  if(!resolved_user_id.is_null())
                  {
                    self->result_user_id = resolved_user_id;
                    self->result_user_id_type =
                      self->external_ids[index].can_be_in_cookie ?
                        RUIT_EXTIDRESOLVE : RUIT_EXTIDRESOLVE_NOCOOKIE;
                    self->frontend->common_module_->user_id_controller()->
                      null_blacklisted(self->result_user_id);

                    if(self->frontend->config_->create_profile())
                    {
                      self->create_user_profile = response.created();
                    }
                  }
                }
              }
              else
              {
                self->http_status = 500;
              }

              if(!self->result_user_id.is_null())
              {
                self->resolved_ext_user_i = index;
                self->add_external_id(0);
              }
              else
              {
                self->resolve_external_id(index + 1);
              }
            });
        });
    }

    void add_external_id(std::size_t index) noexcept
    {
      if(!frontend->user_bind_client_ ||
        (!opted_out && result_user_id.is_null()))
      {
        finish_user_id();
        return;
      }

      for(; index < external_ids.size(); ++index)
      {
        const auto& external_id = external_ids[index].id;
        if(!external_id.empty() &&
          index != resolved_ext_user_i &&
          resolve_failed_external_ids.find(external_id) ==
            resolve_failed_external_ids.end())
        {
          break;
        }
      }

      if(index >= external_ids.size())
      {
        finish_user_id();
        return;
      }

      adserver::user_info_svcs::user_bind::AddUserIdRequest add_user_request;
      add_user_request.set_id(external_ids[index].id);
      add_user_request.set_user_id(GrpcAlgs::pack_user_id(
        !opted_out ? result_user_id : Commons::UserId()));
      add_user_request.set_timestamp(GrpcAlgs::pack_time(request_info->time));

      auto self = shared_from_this();
      frontend->user_bind_client_->add_user_id(
        add_user_request,
        [self, index](
          const grpc::Status& status,
          const adserver::user_info_svcs::user_bind::AddUserIdResponse&
            response)
        {
          self->frontend->workers_->post(
            [self, index, status, response]()
            {
              if(status.ok())
              {
                if(response.invalid_operation())
                {
                  self->frontend->report_bad_user_(*self->request_info);
                }
              }
              else
              {
                self->http_status = 500;
              }
              self->add_external_id(index + 1);
            });
        });
    }

    void finish_user_id() noexcept
    {
      if(!opted_out &&
        !app_request && (
          !result_user_id.is_null() || resolve_failed_external_ids.empty()))
      {
        Generics::Uuid generated_user_id = result_user_id.is_null() ?
          Generics::Uuid::create_random_based() :
          result_user_id;

        if(request_info->generate_external_id)
        {
          result_ssp_user_id =
            frontend->common_module_->user_id_controller()->ssp_uuid(
              generated_user_id,
              request_info->source_id);
        }

        if(frontend->config_->set_uid())
        {
          result_user_id = generated_user_id;
          result_user_id_type = RUIT_COOKIE;
        }
      }

      finish_result();
    }

    void finish_result() noexcept
    {
      BindResult bind_result;
      bind_result.result_user_id = result_user_id;
      if(!result_ssp_user_id.is_null())
      {
        bind_result.ssp_user_id = result_ssp_user_id;
      }
      else if(!app_request && !result_user_id.is_null())
      {
        bind_result.ssp_user_id =
          frontend->common_module_->user_id_controller()->ssp_uuid(
            result_user_id,
            request_info->source_id);
      }

      schedule_user_match();
      add_bind_request_or_finish(bind_result);
    }

    void schedule_user_match() noexcept
    {
      std::string ifa_str;
      if(request_info->external_id.compare(0, 4, "ifa/") == 0)
      {
        ifa_str = request_info->external_id.substr(4);
        String::AsciiStringManip::to_lower(ifa_str);
      }

      frontend->schedule_user_match_(
        result_user_id,
        merge_user_id,
        create_user_profile,
        !ifa_str.empty() ? IDFA_KNOWN_KEYWORD : String::SubString(),
        ifa_str,
        request_info->referer,
        request_info->colo_id,
        request_info->location,
        request_info->source_id);
    }

    void add_bind_request_or_finish(const BindResult& bind_result) noexcept
    {
      if(dns_bind_request_id.empty() || !frontend->user_bind_client_)
      {
        finish(bind_result);
        return;
      }

      AdServer::Commons::ExternalUserIdArray user_ids;
      if(!result_user_id.is_null())
      {
        if(result_user_id_type == RUIT_CRESOLVE ||
          result_user_id_type == RUIT_EXTIDRESOLVE_NOCOOKIE)
        {
          user_ids.push_back(std::string("/") + result_user_id.to_string());
        }
        else
        {
          user_ids.push_back(std::string("c/") + result_user_id.to_string());
        }
      }

      if(!app_request &&
        !request_info->user_id.is_null() &&
        !(request_info->user_id == result_user_id))
      {
        user_ids.push_back(std::string("c/") + request_info->user_id.to_string());
      }

      if(!request_info->ga_user_id.empty())
      {
        user_ids.push_back(request_info->ga_user_id);
      }

      if(!request_info->ym_user_id.empty())
      {
        user_ids.push_back(request_info->ym_user_id);
      }

      if(!request_info->external_id.empty())
      {
        user_ids.push_back(request_info->external_id);
      }

      if(!request_info->add_user_id.is_null() &&
        request_info->add_user_id != result_user_id &&
        request_info->add_user_id != request_info->user_id)
      {
        user_ids.push_back(std::string("/") +
          request_info->add_user_id.to_string());
      }

      adserver::user_info_svcs::user_bind::AddBindRequestRequest bind_request;
      bind_request.set_request_id(dns_bind_request_id);
      bind_request.set_timestamp(GrpcAlgs::pack_time(request_info->time));
      for(const auto& user_id : user_ids)
      {
        bind_request.add_bind_user_ids(user_id);
      }

      auto self = shared_from_this();
      frontend->user_bind_client_->add_bind_request(
        bind_request,
        [self, bind_result](
          const grpc::Status&,
          const adserver::user_info_svcs::user_bind::AddBindRequestResponse&)
        {
          self->frontend->workers_->post(
            [self, bind_result]()
            {
              self->finish(bind_result);
            });
        });
    }

    void finish(const BindResult& bind_result, int status = -1) noexcept
    {
      callback(status >= 0 ? status : http_status, bind_result);
    }

  private:
    UserBindFrontend* frontend;
    UserBind::RequestInfo_var request_info;
    std::string dns_bind_request_id;
    ProcessRequestCallback callback;

    int http_status;
    AdServer::Commons::UserId result_user_id;
    AdServer::Commons::UserId result_ssp_user_id;
    ResultUserIdType result_user_id_type;
    AdServer::Commons::UserId merge_user_id;
    bool app_request;
    bool opted_out;
    bool cresolve_failed;
    bool create_user_profile;
    std::vector<ExternalId> external_ids;
    std::set<std::string> resolve_failed_external_ids;
    std::size_t resolved_ext_user_i;
  };

  void
  UserBindFrontend::process_request_async_(
    UserBind::RequestInfo_var request_info,
    std::string dns_bind_request_id,
    ProcessRequestCallback callback)
    noexcept
  {
    std::make_shared<BindProcessingState>(
      this,
      std::move(request_info),
      std::move(dns_bind_request_id),
      std::move(callback))->start();
  }

  void
  UserBindFrontend::schedule_user_match_(
    const Commons::UserId& result_user_id,
    const Commons::UserId& merge_user_id,
    bool create_user_profile,
    const String::SubString& keywords,
    const String::SubString& cohort,
    const String::SubString& referer,
    unsigned long colo_id,
    const FrontendCommons::Location* location,
    const String::SubString& source)
    noexcept
  {
    if(!config_->enable_profiling() ||
      !match_workers_ ||
      result_user_id.is_null() ||
      (referer.empty() &&
        cohort.empty() &&
        merge_user_id.is_null() &&
        !create_user_profile))
    {
      return;
    }

    unsigned long cur_task_count =
      match_task_count_.exchange_and_add(1) + 1;

    if(cur_task_count > config_->match_pending_task_limit() +
      config_->match_threads())
    {
      match_task_count_ += -1;
      return;
    }

    FrontendCommons::Location_var location_holder(
      ReferenceCounting::add_ref(
        const_cast<FrontendCommons::Location*>(location)));

    match_workers_->post(
      [this,
        result_user_id,
        merge_user_id,
        create_user_profile,
        keywords = keywords.str(),
        cohort = cohort.str(),
        referer = referer.str(),
        colo_id,
        location = location_holder,
        source = source.str()]()
      {
        match_task_count_ += -1;
        user_match_(
          result_user_id,
          merge_user_id,
          create_user_profile,
          keywords,
          cohort,
          referer,
          colo_id,
          location,
          source);
      });
  }

  //
  // UserBindFrontend implementation
  //
  UserBindFrontend::UserBindFrontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module)
    /*throw(eh::Exception)*/
    : Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().ContentFeConfiguration()->Logger().log_level())),
        "UserBindFrontend",
        Aspect::USER_BIND_FRONTEND,
        0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      match_task_count_(0)
  {}

  bool
  UserBindFrontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;

    bool result =
      FrontendCommons::find_uri(
        config_->PathUriList().Uri(), uri, found_uri, 0, false) ||
      FrontendCommons::find_uri(
        config_->UriList().Uri(), uri, found_uri) ||
      FrontendCommons::find_uri(
        config_->PixelUriList().Uri(), uri, found_uri);

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "UserBindFrontend::will_handle(" << uri <<
        "), service: '" << found_uri << "'";

      logger()->log(ostr.str());
    }

    return result;
  }

  void
  UserBindFrontend::handle_request(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    workers_->post(
      [this,
        request_holder = std::move(request_holder),
        response_writer = std::move(response_writer)]() mutable
      {
        handle_request_(
          std::move(request_holder),
          std::move(response_writer));
      });
  }

  void
  UserBindFrontend::parse_configs_() /*throw(Exception)*/
  {
    static const char* FUN = "UserBindFrontend::parse_configs_()";

    /* load common configuration */
    Config::ErrorHandler error_handler;

    try
    {
      typedef Configuration::FeConfig Config;
      const Config& fe_config = frontend_config_->get();

      if(!fe_config.CommonFeConfiguration().present())
      {
        throw Exception("CommonFeConfiguration not presented.");
      }

      common_config_.reset(
        new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      if(!fe_config.UserBindFeConfiguration().present())
      {
        throw Exception("BiddingFeConfiguration isn't present");
      }

      config_.reset(
        new UserBindFeConfiguration(*fe_config.UserBindFeConfiguration()));

    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't parse config file '" << fe_config_path_ << "': " <<
        e.what();
      throw Exception(ostr);
    }
  }

  void
  UserBindFrontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "UserBindFrontend::init()";

    if(true) // module_used())
    {
      try
      {
        parse_configs_();

        workers_ = new FrontendCommons::FrontendWorkers(
          callback(),
          config_->threads());
        add_child_object(workers_);

        /*
        callback_holder_.reset(new Logging::LoggerCallbackHolder(
          Logging::Logger_var(
            new Logging::SeveritySelectorLogger(
              logger,
              0,
              config_->Logger().log_level())),
            "UserBindFrontend",
          Aspect::USER_BIND_FRONTEND,
          0));
        */
        grpc_executor_ = std::make_shared<AdServer::Grpc::GrpcExecutor>(
          common_config_->grpc_executor_threads());
        add_child_object(grpc_executor_);

        auto user_bind_objects =
          AdServer::UserInfoSvcs::create_distributed_user_bind_client(
            *common_config_,
            grpc_executor_,
            logger());
        if(user_bind_objects.client)
        {
          user_bind_client_ = user_bind_objects.client;
          add_child_object(user_bind_objects.active_object);
        }

        user_info_client_ =
          AdServer::UserInfoSvcs::create_distributed_user_info_client(
            *common_config_,
            grpc_executor_,
            logger(),
            this);

        auto channel_client_objects =
          AdServer::ChannelSvcs::create_distributed_channel_client(
            *common_config_,
            grpc_executor_);
        channel_client_ = channel_client_objects.client;
        add_child_object(channel_client_objects.active_object);

        auto campaign_manager = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
            FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
            AdServer::Grpc::BatchingOptions(),
            grpc_executor_);
        campaign_manager_ = campaign_manager;
        add_child_object(campaign_manager);

        cookie_manager_.reset(
          new FrontendCommons::CookieManager<
            FCGI::HttpRequest, FCGI::HttpResponse>(
              common_config_->Cookies()));

        for (UserBindFeConfiguration::Source_sequence::const_iterator
             it = config_->Source().begin(); it != config_->Source().end(); ++it)
        {
          SourceEntity_var& source = sources_[it->id()];
          if(!source.in())
          {
            source = new SourceEntity();
          }

          if(!it->redirect().empty())
          {
            std::string keywords = it->keywords();
            String::SubString keywords_ss(keywords);

            RedirectRule::AllowedParams allowed_params;
            auto it_redirect_param = it->redirectUrlParam().begin();
            const auto it_redirect_param_end = it->redirectUrlParam().end();
            for (; it_redirect_param != it_redirect_param_end; ++it_redirect_param)
            {
              allowed_params.try_emplace(
                it_redirect_param->name(),
                it_redirect_param->token());
            }

            source->rules.push_back(
              init_redirect_rule_(
                it->redirect(),
                !keywords.empty() ? &keywords_ss : 0,
                it->passback(),
                it->weight(),
                it->location(),
                it->redirect_empty_uid(),
                std::move(allowed_params)));
          }
        }

        const char* geo_ip_path = 0;

        if(common_config_->GeoIP().present())
        {
          geo_ip_path = common_config_->GeoIP()->path().c_str();
        }

        UserBind::RequestInfoFiller::ExternalUserIdSet skip_external_ids;

        if (common_config_->SkipExternalIds().present())
        {
          for(CommonFeConfiguration::SkipExternalIds_type::Id_sequence::const_iterator
                it = common_config_->SkipExternalIds()->Id().begin();
              it != common_config_->SkipExternalIds()->Id().end(); ++it)
          {
            skip_external_ids.insert(it->value());
          }

          String::SubString skip_ids =
            common_config_->SkipExternalIds()->skip_external_ids();

          if (!skip_ids.empty())
          {
            String::StringManip::SplitNL tokenizer(skip_ids);
            for (String::SubString skip_id; tokenizer.get_token(skip_id);)
            {
              skip_external_ids.insert(skip_id.str());
            }
          }
        }

        UserBind::RequestInfoFiller::AllowedPassbackDomainArray
          allowed_passback_domains;

        for(auto it = config_->AllowedPassback().begin();
          it != config_->AllowedPassback().end(); ++it)
        {
          if(!it->domain().empty())
          {
            allowed_passback_domains.push_back(it->domain());
          }
        }

        request_info_filler_.reset(
          new UserBind::RequestInfoFiller(
            logger(),
            common_module_,
            geo_ip_path,
            skip_external_ids,
            allowed_passback_domains,
            common_config_->colo_id()));

        if (config_->pixel_path().present())
        {
          pixel_ = FileCachePtr(new FileCache((*config_->pixel_path()).c_str()));
        }

        pixel_content_type_ = config_->pixel_content_type();

        if(config_->match_threads() > 0)
        {
          match_workers_ = new FrontendCommons::FrontendWorkers(
            callback(),
            config_->match_threads());
          add_child_object(match_workers_);
        }

        activate_object();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      logger()->log(String::SubString(
          "UserBindFrontend::init(): frontend is running ..."),
        Logging::Logger::INFO,
        Aspect::USER_BIND_FRONTEND);
    }
  }

  void
  UserBindFrontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();

      Stream::Error ostr;
      ostr << "UserBindFrontend::shutdown(): frontend terminated (pid = " <<
        ::getpid() << ").";

      logger()->log(ostr.str(),
        Logging::Logger::INFO,
        Aspect::USER_BIND_FRONTEND);
    }
    catch(...)
    {}
  }

  uint32_t
  UserBindFrontend::calc_yandex_sign_(
    const UserBind::RequestInfo& request_info,
    const std::string& user_data,
    const std::string* location,
    const std::string& secure_key)
    noexcept
  {
    std::string res;
    size_t pos = static_cast<size_t>(-1);
    for(auto i = 0; i < 3; ++i)
    {
      pos = request_info.peer_ip.find('.', pos + 1);
      if(pos == std::string::npos)
      {//not valid ip
        return 0;
      }
    }
    res = request_info.peer_ip.substr(0, pos);
    res += request_info.referer;
    res += request_info.user_agent;
    if(location)
    {
      res += *location;
    }
    res += user_data;
    res += secure_key;

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      logger()->sstream(Logging::Logger::TRACE, Aspect::USER_BIND_FRONTEND) <<
        "string for calculation of yandex sum '" << res << "'";
    }

    return Generics::CRC::reversed(0, res.data(), res.size());
  }

  void
  UserBindFrontend::handle_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    static const char* FUN = "UserBindFrontend::handle_request_()";
    const FCGI::HttpRequest& request = request_holder->request();

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());
    FCGI::HttpResponse& response = *response_ptr;

    auto finish_response =
      [this,
        request_holder,
        response_writer,
        response_ptr](int http_status) mutable
      {
        const FCGI::HttpRequest& request = request_holder->request();
        FCGI::HttpResponse& response = *response_ptr;
        std::string found_uri;
        if(http_status == 204 &&
          FrontendCommons::find_uri(
            config_->PixelUriList().Uri(),
            request.uri(),
            found_uri) &&
          pixel_.in())
        {
          http_status = 200;
          response.set_content_type_nocopy(pixel_content_type_);

          FileCache::BufferHolder_var buffer = pixel_->get();
          response.get_output_stream().write((*buffer)->data(), (*buffer)->size());
        }

        response_writer->write(http_status, response_ptr);
      };

    try
    {
      std::string path_args;
      std::string found_uri;
      if(FrontendCommons::find_uri(
           config_->PathUriList().Uri(), request.uri(), found_uri, 0, false))
      {
        path_args = request.uri().substr(found_uri.length()).str();
      }

      UserBind::RequestInfo_var request_info_holder =
        new UserBind::RequestInfo();
      request_info_filler_->fill(
        *request_info_holder,
        request,
        path_args);

      const UserBind::RequestInfo& request_info = *request_info_holder;

      FrontendCommons::CORS::set_headers(request, response);

      if (request.uri().substr(0, UrlPath::kGetUserId.size()) ==
          UrlPath::kGetUserId &&
        (request.uri().size() == UrlPath::kGetUserId.size() ||
          (request.uri().size() > UrlPath::kGetUserId.size()
          && request.uri()[UrlPath::kGetUserId.size()] == '?')))
      {
        std::ostringstream stream;
        if (!request_info.user_id.is_null())
        {
          stream << "{\"uid\":\""
                 << request_info.user_id.to_string()
                 << "\"}";
        }
        else
        {
          stream << "{}";
        }

        response.set_content_type_nocopy(Response::Type::JSON);
        response.write(stream.str());
        finish_response(200);
        return;
      }

      if (request.uri().substr(0, UrlPath::kGetSegments.size()) ==
          UrlPath::kGetSegments &&
        (request.uri().size() == UrlPath::kGetSegments.size() ||
          (request.uri().size() > UrlPath::kGetSegments.size()
          && request.uri()[UrlPath::kGetSegments.size()] == '?')))
      {
        handle_user_channels_request_async_(
          request_info_holder,
          response_ptr,
          std::move(finish_response));
        return;
      }

      if(!request.secure() &&
         config_->nosecure_redirect().present() &&
         !config_->nosecure_redirect()->empty() &&
         !request_info.disable_secure_redirect)
      {
        std::string result_url = *(config_->nosecure_redirect()) + request.uri();
        finish_response(FrontendCommons::redirect(result_url, response));
        return;
      }

      if(request_info.google_error != 0)
      {
        finish_response(204);
        return;
      }

      const bool opted_out =
        request_info.user_status == AdServer::CampaignSvcs::US_OPTOUT;
      if(opted_out)
      {
        if(!request_info.user_id.is_null())
        {
          throw InvalidParamException("");
        }

        finish_response(204);
        return;
      }

      if(request_info.external_id.empty() &&
        !request_info.generate_external_id)
      {
        throw InvalidParamException("");
      }

      std::vector<UserBindFrontend::RedirectRule_var> redirect_rules;
      std::vector<UserBindFrontend::RedirectRule_var> keyword_redirect_rules;

      const SourceMap::iterator source_it =
        sources_.find(request_info.source_id);
      if(source_it != sources_.end())
      {
        const SourceEntity& source = *source_it->second;

        std::vector<std::string> keywords;
        FrontendCommons::get_ip_keywords(keywords, request_info.peer_ip);

        for(auto bind_rule_it = source.rules.begin();
          bind_rule_it != source.rules.end();
          ++bind_rule_it)
        {
          if((*bind_rule_it)->use_keywords)
          {
            if(!(*bind_rule_it)->passback ||
              ((*bind_rule_it)->passback && request_info.passback))
            {
              if((*bind_rule_it)->keywords.empty())
              {
                redirect_rules.push_back(*bind_rule_it);
              }
              else
              {
                for(auto keyword_it = keywords.begin();
                  keyword_it != keywords.end();
                  ++keyword_it)
                {
                  if((*bind_rule_it)->keywords.find(*keyword_it) !=
                    (*bind_rule_it)->keywords.end())
                  {
                    keyword_redirect_rules.push_back(*bind_rule_it);
                  }
                }
              }
            }
          }
          else if(!(*bind_rule_it)->passback ||
            ((*bind_rule_it)->passback && request_info.passback))
          {
            redirect_rules.push_back(*bind_rule_it);
          }
        }
      }

      const std::vector<UserBindFrontend::RedirectRule_var>& select_rules =
        !keyword_redirect_rules.empty() ? keyword_redirect_rules : redirect_rules;

      unsigned long sum_weight = 0;
      for(auto rule_it = select_rules.begin();
        rule_it != select_rules.end();
        ++rule_it)
      {
        sum_weight += (*rule_it)->weight;
      }

      UserBindFrontend::RedirectRule_var redirect_rule;
      if(!select_rules.empty())
      {
        redirect_rule = *select_rules.begin();

        unsigned long sel_weight = Generics::safe_rand(sum_weight);
        sum_weight = 0;
        for(auto rule_it = select_rules.begin();
          rule_it != select_rules.end();
          ++rule_it)
        {
          if(sum_weight <= sel_weight &&
            sel_weight < sum_weight + (*rule_it)->weight)
          {
            redirect_rule = *rule_it;
            break;
          }
          sum_weight += (*rule_it)->weight;
        }
      }

      std::string dns_bind_request_id;
      if(redirect_rule && redirect_rule->init_bind_request)
      {
        AdServer::Commons::RequestId gen_id =
          AdServer::Commons::RequestId::create_random_based();
        AdServer::Commons::base32_encode(
          dns_bind_request_id,
          String::SubString(reinterpret_cast<const char*>(&*gen_id.begin()), 16));
        dns_bind_request_id = std::string("r") + dns_bind_request_id;
      }

      process_request_async_(
        request_info_holder,
        dns_bind_request_id,
        [this,
          request_holder,
          response_ptr,
          finish_response = std::move(finish_response),
          request_info_holder,
          redirect_rule,
          dns_bind_request_id](
            int process_status,
            BindResult bind_result) mutable
        {
          const FCGI::HttpRequest& request = request_holder->request();
          FCGI::HttpResponse& response = *response_ptr;
          const UserBind::RequestInfo& request_info = *request_info_holder;
          int http_status = process_status == 200 ? 204 : process_status;
          std::string redirect_url;

          try
          {
            if(redirect_rule.in() && (
              !request_info.user_id.is_null() ||
              redirect_rule->redirect_empty_uid))
            {
              String::TextTemplate::Args templ_args;

              const auto& params = request.params();
              const auto& allowed_params = redirect_rule->allowed_params;
              if(!allowed_params.empty())
              {
                for(const auto& param : params)
                {
                  const auto it = allowed_params.find(param.name);
                  if(it != std::end(allowed_params))
                  {
                    templ_args[it->second] = param.value;
                  }
                }
              }

              templ_args[TemplateParams::EXTERNALID] = request_info.external_id;
              templ_args[TemplateParams::SHORTEXTERNALID] =
                request_info.short_external_id;
              templ_args[TemplateParams::RANDOM] =
                String::StringManip::IntToStr(
                  Generics::safe_rand()).str().str();

              if(!request_info.passback_url.empty())
              {
                templ_args[TemplateParams::PASSBACK_URL] =
                  request_info.passback_url;
              }

              if(!bind_result.result_user_id.is_null())
              {
                templ_args[TemplateParams::UNSIGNEDUID] =
                  bind_result.result_user_id.to_string();
                templ_args[TemplateParams::SIGNEDUID] =
                  common_module_->user_id_controller()->sign(
                    bind_result.result_user_id).str();
              }

              if(!request_info.user_id.is_null())
              {
                templ_args[TemplateParams::SIGNEDCOOKIEUID] =
                  common_module_->user_id_controller()->sign(
                    request_info.user_id).str();
              }

              std::string ssp_user_id_str;
              if(!bind_result.ssp_user_id.is_null() ||
                !request_info.user_id.is_null())
              {
                Commons::UserId result_ssp_user_id = bind_result.ssp_user_id;
                if(result_ssp_user_id.is_null())
                {
                  result_ssp_user_id =
                    common_module_->user_id_controller()->ssp_uuid(
                      request_info.user_id,
                      request_info.source_id);
                }

                ssp_user_id_str =
                  common_module_->user_id_controller()->ssp_uuid_string(
                    result_ssp_user_id);

                templ_args[TemplateParams::SSPUID] =
                  common_module_->user_id_controller()->sign(
                    bind_result.ssp_user_id, UserIdController::SSP).str();

                templ_args[TemplateParams::UNSIGNEDSSPUID] = ssp_user_id_str;
              }

              if(!request_info.push_data.empty())
              {
                templ_args[TemplateParams::PUSH_DATA] = request_info.push_data;
              }

              if(!dns_bind_request_id.empty())
              {
                templ_args[TemplateParams::BINDREQUESTID] = dns_bind_request_id;
              }

              String::TextTemplate::DefaultValue args_with_default(&templ_args);
              String::TextTemplate::ArgsEncoder args_with_encoding(
                &args_with_default);

              std::string location;
              if(redirect_rule->location)
              {
                location = redirect_rule->redirect.instantiate(args_with_encoding);
                templ_args[TemplateParams::LOCATION] = location;
              }

              uint32_t yandex_sign = 0;
              if(request_info.external_id.empty() && config_->Keys().present())
              {
                yandex_sign = calc_yandex_sign_(
                  request_info,
                  ssp_user_id_str,
                  !location.empty() ? &location : 0,
                  config_->Keys().get().yandex_key().present() ?
                    config_->Keys().get().yandex_key().get() : "" );
              }

              templ_args[TemplateParams::YANDEXSIGN] =
                String::StringManip::IntToStr(yandex_sign).str().str();

              std::string redirect =
                redirect_rule->redirect.instantiate(args_with_encoding);

              if(!redirect.empty())
              {
                if(!request_info.ssp_id.empty())
                {
                  std::string mimed_ssp_id;
                  String::StringManip::mime_url_encode(
                    request_info.ssp_id,
                    mimed_ssp_id);

                  redirect += redirect.find('?') == std::string::npos ?
                    '?' : '&';
                  redirect += "ssp=";
                  redirect += mimed_ssp_id;
                }

                redirect_url = FrontendCommons::normalize_abs_url(
                  HTTP::BrowserAddress(redirect),
                  HTTP::HTTPAddress::VW_FULL,
                  FrontendCommons::is_secure_request(request) ||
                    request_info.secure ?
                      HTTPS_PREFIX : HTTP_PREFIX);
              }
            }
          }
          catch(const eh::Exception& e)
          {
            Stream::Error ostr;
            ostr << FUN << ": source processing failed for external_id = '" <<
              request_info.external_id << "'; " << e.what();
            logger()->log(ostr.str(),
              Logging::Logger::ERROR,
              Aspect::USER_BIND_FRONTEND);
          }

          if(!bind_result.result_user_id.is_null())
          {
            const Generics::SignedUuid signed_uid =
              common_module_->user_id_controller()->sign(
                bind_result.result_user_id);
            FrontendCommons::add_UID_cookie(
              response,
              request,
              *cookie_manager_,
              signed_uid.str());
          }

          if(!redirect_url.empty())
          {
            http_status = FrontendCommons::redirect(redirect_url, response);
          }

          if(http_status == 204 && !request_info.passback_url.empty())
          {
            http_status = FrontendCommons::redirect(
              request_info.passback_url,
              response);
          }

          finish_response(http_status);
        });
    }
    catch(const InvalidParamException&)
    {
      response.add_header_nocopy(
        String::SubString("X-Status"),
        String::SubString("Bad request"));
      finish_response(204);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << e.what();
      logger()->log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::USER_BIND_FRONTEND,
        "ADS-IMPL-109");
      finish_response(500);
    }
  }

  void
  UserBindFrontend::report_bad_user_(
    const UserBind::RequestInfo& /*request_info*/)
    noexcept
  {
    try
    {
    }
    catch (const Exception&)
    {}
    catch(const eh::Exception&)
    {}
  }

  void
  UserBindFrontend::log_cookie_mapping_(
    const UserBind::RequestInfo& request_info)
    noexcept
  {
    static const char* FUN = "UserBindFrontend::log_cookie_mapping_()";

    if (config_->cookie_mapping_log())
    {
      adserver::campaign_svcs::campaign_manager::ConsiderWebOperationRequest
        web_op_info;
      web_op_info.set_time(GrpcAlgs::pack_time(request_info.time));
      web_op_info.set_colo_id(request_info.colo_id);
      web_op_info.set_tag_id(0);
      web_op_info.set_cc_id(0);
      web_op_info.set_app(WebStats::APPLICATION.str());
      web_op_info.set_source(WebStats::SOURCE.str());
      web_op_info.set_operation(WebStats::OPERATION.str());
      web_op_info.set_result('U');
      web_op_info.set_user_status(
        static_cast<unsigned long>(request_info.user_status));
      web_op_info.set_test_request(false);
      web_op_info.set_user_bind_src(request_info.source_id);

      campaign_manager_->consider_web_operation(
        web_op_info,
        [this](
          const grpc::Status& status,
          const adserver::campaign_svcs::campaign_manager::
            ConsiderWebOperationResponse&)
        {
          workers_->post(
            [this, status]()
            {
              if(!status.ok() &&
                status.error_code() != grpc::StatusCode::INVALID_ARGUMENT)
              {
                Stream::Error ostr;
                ostr << FUN << ": CampaignManager::consider_web_operation() "
                  "gRPC call failed: code=" <<
                  static_cast<int>(status.error_code()) <<
                  ", message=" << status.error_message();
                logger()->log(ostr.str(),
                  Logging::Logger::ERROR,
                  Aspect::USER_BIND_FRONTEND,
                  "ADS-IMPL-7805");
              }
            });
        });
    }
  }

  void
  UserBindFrontend::user_match_(
    const Commons::UserId& result_user_id,
    const Commons::UserId& merge_user_id,
    bool create_user_profile,
    const String::SubString& keywords,
    const String::SubString& cohort,
    const String::SubString& referer,
    unsigned long colo_id,
    const FrontendCommons::Location* location,
    const String::SubString& source)
    noexcept
  {
    static const char* FUN = "UserBindFrontend::user_match_()";

    struct MatchState final: public std::enable_shared_from_this<MatchState>
    {
      UserBindFrontend* frontend;
      Commons::UserId result_user_id;
      Commons::UserId merge_user_id;
      bool create_user_profile;
      std::string keywords;
      std::string cohort;
      std::string referer;
      unsigned long colo_id;
      FrontendCommons::Location_var location;
      std::string source;
      Generics::Time now;
      adserver::channel_svcs::channel_server::MatchResponse trigger_match_result;
      bool trigger_match_result_present = false;
      AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult_var history_match_result;

      void log_channel_error(const grpc::Status& status)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught ChannelServerGrpcAsyncClient error: "
          "code=" << static_cast<int>(status.error_code()) <<
          ", message=" << status.error_message();
        frontend->logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::USER_BIND_FRONTEND,
          "ADS-IMPL-117");
      }

      void log_user_info_error(
        const char* operation,
        const grpc::Status& status)
      {
        Stream::Error ostr;
        ostr << FUN << ": " << operation << " gRPC call failed: user_id = '" <<
          result_user_id.to_string() << "'; code=" <<
          static_cast<int>(status.error_code()) <<
          ", message=" << status.error_message();
        frontend->logger()->log(ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::USER_BIND_FRONTEND,
          status.error_code() == grpc::StatusCode::UNAVAILABLE ?
            "ADS-IMPL-7804" : "ADS-IMPL-7803");
      }

      void start()
      {
        now = Generics::Time::get_time_of_day();

        if((!referer.empty() || !keywords.empty()) &&
          !result_user_id.is_null())
        {
          adserver::channel_svcs::channel_server::MatchRequest channel_request;
          channel_request.set_non_strict_word_match(false);
          channel_request.set_non_strict_url_match(false);
          channel_request.set_return_negative(false);
          channel_request.set_simplify_page(false);
          channel_request.set_fill_content(false);
          channel_request.set_statuses("A", 2);
          channel_request.set_pwords(keywords);
          channel_request.set_first_url(referer);

          auto self = shared_from_this();
          frontend->channel_client_->match(
            channel_request,
            [self](
              const grpc::Status& status,
              const adserver::channel_svcs::channel_server::MatchResponse& response)
            {
              self->frontend->workers_->post(
                [self, status, response]()
                {
                  if(status.ok())
                  {
                    self->trigger_match_result = response;
                    self->trigger_match_result_present = true;
                  }
                  else
                  {
                    self->log_channel_error(status);
                  }
                  self->run_history();
                });
            });
        }
        else
        {
          run_history();
        }
      }

      void fill_history_match_request(
        adserver::user_info_svcs::user_info_manager::MatchRequest&
          history_match_request)
      {
        auto* grpc_match_params = history_match_request.mutable_match_params();

        if(trigger_match_result_present)
        {
          const auto& matched_channels =
            trigger_match_result.matched_channels();
          typedef std::set<ChannelMatch> ChannelMatchSet;

          ChannelMatchSet page_channels;
          std::transform(
            matched_channels.page_channels().begin(),
            matched_channels.page_channels().end(),
            std::inserter(page_channels, page_channels.end()),
            GetChannelTriggerId());
          for(const auto& channel : page_channels)
          {
            auto* channel_match = grpc_match_params->add_page_channel_ids();
            channel_match->set_channel_id(channel.channel_id);
            channel_match->set_channel_trigger_id(channel.channel_trigger_id);
          }

          ChannelMatchSet url_channels;
          std::transform(
            matched_channels.url_channels().begin(),
            matched_channels.url_channels().end(),
            std::inserter(url_channels, url_channels.end()),
            GetChannelTriggerId());
          for(const auto& channel : url_channels)
          {
            auto* channel_match = grpc_match_params->add_url_channel_ids();
            channel_match->set_channel_id(channel.channel_id);
            channel_match->set_channel_trigger_id(channel.channel_trigger_id);
          }

          ChannelMatchSet url_keyword_channels;
          std::transform(
            matched_channels.url_keyword_channels().begin(),
            matched_channels.url_keyword_channels().end(),
            std::inserter(url_keyword_channels, url_keyword_channels.end()),
            GetChannelTriggerId());
          for(const auto& channel : url_keyword_channels)
          {
            auto* channel_match =
              grpc_match_params->add_url_keyword_channel_ids();
            channel_match->set_channel_id(channel.channel_id);
            channel_match->set_channel_trigger_id(channel.channel_trigger_id);
          }
        }

        grpc_match_params->set_use_empty_profile(false);
        grpc_match_params->set_silent_match(false);
        grpc_match_params->set_no_match(false);
        grpc_match_params->set_no_result(true);
        grpc_match_params->set_ret_freq_caps(false);
        grpc_match_params->set_provide_channel_count(false);
        grpc_match_params->set_provide_persistent_channels(false);
        grpc_match_params->set_change_last_request(false);
        grpc_match_params->set_filter_contextual_triggers(false);
        grpc_match_params->set_publishers_optin_timeout(
          GrpcAlgs::pack_time(Generics::Time::ZERO));
        grpc_match_params->set_cohort(cohort);

        auto* grpc_user_info = history_match_request.mutable_user_info();
        grpc_user_info->set_user_id(GrpcAlgs::pack_user_id(result_user_id));
        grpc_user_info->set_last_colo_id(colo_id);
        grpc_user_info->set_request_colo_id(colo_id);
        grpc_user_info->set_current_colo_id(-1);
        grpc_user_info->set_temporary(false);
        grpc_user_info->set_time(now.tv_sec);
      }

      bool need_history() const
      {
        return !merge_user_id.is_null() ||
          create_user_profile ||
          !cohort.empty() ||
          !keywords.empty() ||
          (!result_user_id.is_null() && trigger_match_result_present && (
            trigger_match_result.matched_channels().page_channels_size() > 0 ||
            trigger_match_result.matched_channels().url_channels_size() > 0 ||
            trigger_match_result.matched_channels().url_keyword_channels_size() > 0));
      }

      void run_history()
      {
        if(!need_history() || !frontend->user_info_client_)
        {
          run_campaign();
          return;
        }

        auto history_match_request = std::make_shared<
          adserver::user_info_svcs::user_info_manager::MatchRequest>();
        fill_history_match_request(*history_match_request);

        if(!merge_user_id.is_null())
        {
          adserver::user_info_svcs::user_info_manager::GetUserProfileRequest
            get_profile_request;
          get_profile_request.set_user_id(GrpcAlgs::pack_user_id(merge_user_id));
          get_profile_request.set_temporary(false);
          auto* profile_request =
            get_profile_request.mutable_profile_request();
          profile_request->set_base_profile(true);
          profile_request->set_add_profile(true);
          profile_request->set_history_profile(true);
          profile_request->set_freq_cap_profile(true);
          profile_request->set_pref_profile(false);

          auto self = shared_from_this();
          frontend->user_info_client_->get_user_profile(
            get_profile_request,
            [self, history_match_request](
              const grpc::Status& status,
              const adserver::user_info_svcs::user_info_manager::
                GetUserProfileResponse& response)
            {
              self->frontend->workers_->post(
                [self, history_match_request, status, response]()
                {
                  if(!status.ok())
                  {
                    self->log_user_info_error(
                      "UserInfoManager::get_user_profile()",
                      status);
                    self->run_campaign();
                    return;
                  }

                  if(response.found())
                  {
                    self->run_merge(history_match_request, response);
                  }
                  else
                  {
                    self->run_match(history_match_request);
                  }
                });
            });
        }
        else
        {
          run_match(history_match_request);
        }
      }

      void run_merge(
        std::shared_ptr<adserver::user_info_svcs::user_info_manager::MatchRequest>
          history_match_request,
        const adserver::user_info_svcs::user_info_manager::
          GetUserProfileResponse& get_profile_response)
      {
        adserver::user_info_svcs::user_info_manager::MergeRequest merge_request;
        *merge_request.mutable_user_info() = history_match_request->user_info();
        *merge_request.mutable_match_params() =
          history_match_request->match_params();
        *merge_request.mutable_merge_user_profile() =
          get_profile_response.user_profile();

        auto self = shared_from_this();
        frontend->user_info_client_->merge(
          merge_request,
          [self](
            const grpc::Status& status,
            const adserver::user_info_svcs::user_info_manager::MergeResponse&)
          {
            self->frontend->workers_->post(
              [self, status]()
              {
                if(!status.ok())
                {
                  self->log_user_info_error(
                    "UserInfoManager::merge()",
                    status);
                  self->run_campaign();
                  return;
                }
                self->run_remove_merged_profile();
              });
          });
      }

      void run_remove_merged_profile()
      {
        adserver::user_info_svcs::user_info_manager::RemoveUserProfileRequest
          remove_request;
        remove_request.set_user_id(GrpcAlgs::pack_user_id(merge_user_id));

        auto self = shared_from_this();
        frontend->user_info_client_->remove_user_profile(
          remove_request,
          [self](
            const grpc::Status& status,
            const adserver::user_info_svcs::user_info_manager::
              RemoveUserProfileResponse&)
          {
            self->frontend->workers_->post(
              [self, status]()
              {
                if(!status.ok())
                {
                  self->log_user_info_error(
                    "UserInfoManager::remove_user_profile()",
                    status);
                }
                self->run_campaign();
              });
          });
      }

      void run_match(
        std::shared_ptr<adserver::user_info_svcs::user_info_manager::MatchRequest>
          history_match_request)
      {
        auto self = shared_from_this();
        frontend->user_info_client_->match(
          *history_match_request,
          [self](
            const grpc::Status& status,
            const adserver::user_info_svcs::user_info_manager::MatchResponse&
              response)
          {
            self->frontend->workers_->post(
              [self, status, response]()
              {
                if(status.ok())
                {
                  self->history_match_result =
                    AdServer::UserInfoSvcs::GrpcAlgs::
                      make_history_match_result(response);
                }
                else
                {
                  self->log_user_info_error(
                    "UserInfoManager::match()",
                    status);
                }
                self->run_campaign();
              });
          });
      }

      void run_campaign()
      {
        adserver::campaign_svcs::campaign_manager::ProcessMatchRequestRequest
          process_match_request;
        frontend->fill_match_request_info_(
          *process_match_request.mutable_match_request_info(),
          result_user_id,
          now,
          trigger_match_result_present ? &trigger_match_result : nullptr,
          history_match_result,
          location,
          referer,
          source);

        auto self = shared_from_this();
        frontend->campaign_manager_->process_match_request(
          process_match_request,
          [self](
            const grpc::Status& status,
            const adserver::campaign_svcs::campaign_manager::
              ProcessMatchRequestResponse&)
          {
            self->frontend->workers_->post(
              [self, status]()
              {
                if(!status.ok())
                {
                  Stream::Error ostr;
                  ostr << FUN << ": Can't process match request. "
                    "Possible problem with Campaignmanager. "
                    "gRPC call failed: code=" <<
                    static_cast<int>(status.error_code()) <<
                    ", message=" << status.error_message();
                  self->frontend->logger()->log(ostr.str(),
                    Logging::Logger::EMERGENCY,
                    Aspect::USER_BIND_FRONTEND,
                    "ADS-ICON-4");
                }
              });
          });
      }
    };

    auto state = std::make_shared<MatchState>();
    state->frontend = this;
    state->result_user_id = result_user_id;
    state->merge_user_id = merge_user_id;
    state->create_user_profile = create_user_profile;
    state->keywords = keywords.str();
    state->cohort = cohort.str();
    state->referer = referer.str();
    state->colo_id = colo_id;
    state->location = ReferenceCounting::add_ref(
      const_cast<FrontendCommons::Location*>(location));
    state->source = source.str();
    state->start();
  }

  void
  UserBindFrontend::fill_match_request_info_(
    adserver::campaign_svcs::campaign_manager::MatchRequestInfo& mri,
    const AdServer::Commons::UserId& user_id,
    const Generics::Time& now,
    const adserver::channel_svcs::channel_server::MatchResponse*
      trigger_match_result,
    const AdServer::UserInfoSvcs::UserInfoMatcher::MatchResult* history_match_result,
    const FrontendCommons::Location* location,
    const String::SubString& referer,
    const String::SubString& source)
    const noexcept
  {
    /*
      Don't fill:
        mri.match_info.coord_location
    */

    auto* match_info = mri.mutable_match_info();
    match_info->set_colo_id(common_config_->colo_id());
    mri.set_user_id(GrpcAlgs::pack_user_id(user_id));
    mri.set_request_time(GrpcAlgs::pack_time(now));
    match_info->set_full_referer(referer.str());
    mri.set_source(source.str());

    if(trigger_match_result)
    {
      const auto& page_channels =
        trigger_match_result->matched_channels().page_channels();
      const int result_len = page_channels.size();
      for(int i = 0; i < result_len; ++i)
      {
        auto* pkw_channel = match_info->add_pkw_channels();
        pkw_channel->set_channel_id(page_channels[i].id());
        pkw_channel->set_channel_trigger_id(page_channels[i].trigger_channel_id());
      }
    }

    if(history_match_result)
    {
      CORBA::ULong result_len =
        history_match_result->channels.length();
      for(CORBA::ULong i = 0; i < result_len; ++i)
      {
        match_info->add_channels(history_match_result->channels[i].channel_id);
      }
    }

    if (location)
    {
      auto* geo_info = match_info->add_location();
      geo_info->set_country(location->country);
      geo_info->set_region(location->region);
      geo_info->set_city(location->city);
    }
  }

  UserBindFrontend::RedirectRule_var
  UserBindFrontend::init_redirect_rule_(
    const String::SubString& redirect,
    const String::SubString* keywords,
    const bool passback,
    const unsigned long weight,
    const String::SubString& location,
    const bool redirect_empty_uid,
    RedirectRule::AllowedParams&& allowed_params)
    /*throw(UserBindFrontend::InvalidSource)*/
  {
    static const char* FUN = "UserBindFrontend::init_redirect_rule_";

    try
    {
      UserBindFrontend::RedirectRule_var redirect_rule = new UserBindFrontend::RedirectRule();

      init_redirect_template_(redirect_rule->redirect, redirect);

      if(!location.empty())
      {
        redirect_rule->location.reset(new String::TextTemplate::IStream());
        init_redirect_template_(*(redirect_rule->location), location);
      }

      if(keywords)
      {
        String::StringManip::Splitter<String::AsciiStringManip::SepNL> splitter(
          *keywords);
        String::SubString token;
        while(splitter.get_token(token))
        {
          String::StringManip::trim(token);
          if(!token.empty())
          {
            redirect_rule->keywords.insert(Generics::StringHashAdapter(token));
          }
        }

        redirect_rule->use_keywords = true;
      }
      else
      {
        redirect_rule->use_keywords = false;
      }

      redirect_rule->passback = passback;
      redirect_rule->init_bind_request = false;
      redirect_rule->weight = weight;
      redirect_rule->redirect_empty_uid = redirect_empty_uid;
      redirect_rule->allowed_params = std::move(allowed_params);

      {
        String::TextTemplate::Keys keys;

        KeyArgsCallback null_args;
        String::TextTemplate::DefaultValue default_cont(&null_args);
        String::TextTemplate::ArgsEncoder encoder(&default_cont);
        redirect_rule->redirect.keys(encoder, keys);

        if(keys.find(TemplateParams::BINDREQUESTID.str()) != keys.end())
        {
          redirect_rule->init_bind_request = true;
        }
      }

      return redirect_rule;
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw InvalidSource(ostr);
    }
  }

  void
  UserBindFrontend::init_redirect_template_(
    String::TextTemplate::IStream& templ,
    const String::SubString& text)
    /*throw(UserBindFrontend::InvalidSource)*/
  {
    static const char* FUN = "UserBindFrontend::init_redirect_template_()";

    Stream::Parser istr(text);
    templ.init(istr, TemplateParams::MARKER, TemplateParams::MARKER);

    AdServer::Commons::UserId test_uid;
    std::string test_uid_str = test_uid.to_string(false);

    String::TextTemplate::Args templ_args;
    templ_args[TemplateParams::SSPUID] = "";
    templ_args[TemplateParams::UNSIGNEDSSPUID] = test_uid_str;
    templ_args[TemplateParams::EXTERNALID] = "";
    templ_args[TemplateParams::SHORTEXTERNALID] = "";
    templ_args[TemplateParams::YANDEXSIGN] = "0";
    templ_args[TemplateParams::RANDOM] = "0";
    templ_args[TemplateParams::PUSH_DATA] = "";
    templ_args[TemplateParams::BINDREQUESTID] = "r1";

    String::TextTemplate::DefaultValue args_with_default(&templ_args);
    String::TextTemplate::ArgsEncoder args_with_encoding(&args_with_default);

    // check template
    const std::string str = templ.instantiate(args_with_encoding);
    if(!str.empty())
    {
      HTTP::HTTPChecker checker;
      std::string error;

      if (!checker(str, &error, false))
      {
        Stream::Error ostr;
        ostr << FUN << ": invalid redirect template = '" << text <<
          "', error = " << error;
        throw InvalidSource(ostr);
      }
    }
  }

  void UserBindFrontend::handle_user_channels_request_async_(
    UserBind::RequestInfo_var request_info,
    FCGI::HttpResponse_var response,
    std::function<void(int)> callback)
    noexcept
  {
    static const char* FUN = "UserBindFrontend::handle_user_channels_request_()";

    if (request_info->user_id.is_null())
    {
      callback(400);
      return;
    }
    if (!user_info_client_)
    {
      callback(500);
      return;
    }

    adserver::user_info_svcs::user_info_manager::MatchRequest
      history_match_request;
    auto* match_params = history_match_request.mutable_match_params();
    match_params->set_use_empty_profile(false);
    match_params->set_silent_match(false);
    match_params->set_no_match(false);
    match_params->set_no_result(false);
    match_params->set_ret_freq_caps(false);
    match_params->set_provide_channel_count(false);
    match_params->set_provide_persistent_channels(false);
    match_params->set_change_last_request(false);
    match_params->set_publishers_optin_timeout(
      GrpcAlgs::pack_time(Generics::Time::ZERO));
    auto* user_info = history_match_request.mutable_user_info();
    user_info->set_user_id(GrpcAlgs::pack_user_id(request_info->user_id));
    user_info->set_huser_id(GrpcAlgs::pack_user_id(
      AdServer::Commons::UserId{}));
    user_info->set_last_colo_id(request_info->colo_id);
    user_info->set_request_colo_id(request_info->colo_id);
    user_info->set_current_colo_id(-1);
    user_info->set_temporary(false);
    user_info->set_time(Generics::Time::get_time_of_day().tv_sec);

    user_info_client_->match(
      history_match_request,
      [this, request_info, response, callback = std::move(callback)](
        const grpc::Status& status,
        const adserver::user_info_svcs::user_info_manager::MatchResponse&
          history_match_response) mutable
      {
        workers_->post(
          [this,
            request_info,
            response,
            callback = std::move(callback),
            status,
            history_match_response]() mutable
          {
            static const String::SubString JSON_SESSION_ID_NAME("session_id");
            static const String::SubString JSON_CL_ID_NAME("cl_id");
            static const String::SubString JSON_SEGMENTS_NAME("segments");

            if(!status.ok())
            {
              Stream::Error ostr;
              ostr << FUN << ": UserInfoManager::match() gRPC call failed: "
                "user_id = '" << request_info->user_id.to_string() <<
                "'; code=" << static_cast<int>(status.error_code()) <<
                ", message=" << status.error_message();
              logger()->log(ostr.str(),
                Logging::Logger::EMERGENCY,
                Aspect::USER_BIND_FRONTEND,
                status.error_code() == grpc::StatusCode::UNAVAILABLE ?
                  "ADS-IMPL-7804" : "ADS-IMPL-7803");
              callback(500);
              return;
            }

            auto history_match_result =
              AdServer::UserInfoSvcs::GrpcAlgs::make_history_match_result(
                history_match_response);

            AdServer::UserInfoSvcs::UserInfoMatcher::ChannelWeightSeq&
              history_matched_channels = history_match_result->channels;
            std::vector<unsigned long> return_channel_ids;

            for(CORBA::ULong i = 0;
              i < history_matched_channels.length();
              ++i)
            {
              if(request_info->channels_wl.empty() ||
                request_info->channels_wl.find(
                  history_matched_channels[i].channel_id) !=
                  request_info->channels_wl.end())
              {
                return_channel_ids.emplace_back(
                  history_matched_channels[i].channel_id);
              }
            }

            std::ostringstream response_string;
            {
              AdServer::Commons::JsonFormatter root_json(response_string);
              if(!request_info->session_id.empty())
              {
                root_json.add(JSON_SESSION_ID_NAME, request_info->session_id);
              }

              if(!request_info->cl_id.empty())
              {
                root_json.add(JSON_CL_ID_NAME, request_info->cl_id);
              }

              AdServer::Commons::JsonObject segment_array(
                root_json.add_array(JSON_SEGMENTS_NAME));
              for(const auto& user_channel : return_channel_ids)
              {
                segment_array.add_number(user_channel);
              }
            }

            response->set_content_type_nocopy(Response::Type::JSON);
            response->write(response_string.str());
            callback(200);
          });
      });
  }
}
