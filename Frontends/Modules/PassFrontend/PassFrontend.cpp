
#include <Generics/Time.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/CorbaAlgs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/CorbaConfig.hpp>
#include <Commons/Containers.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserverConfigUtils.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>

#include "PassFrontend.hpp"

namespace
{
  struct PassFrontendConstrainTraits
  {
    static const unsigned long MAX_NUMBER_PARAMS = 50;
    static const unsigned long MAX_LENGTH_PARAM_NAME = 30;
    static const unsigned long MAX_LENGTH_PARAM_VALUE = 2000;
  };

  typedef FrontendCommons::DefaultConstrain<
    FrontendCommons::OnlyGetAllowed,
    FrontendCommons::ParamConstrainDefault,
    PassFrontendConstrainTraits>
    PassFrontendHTTPConstrain;
}

namespace Config
{
  const char CONFIG_FILE[] = "PassFrontend_Config";
  const char ENABLE[] = "PassFrontend_Enable";
}

namespace Aspect
{
  const char PASS_FRONTEND[] = "PassbackFrontend";
}

namespace Request
{
  namespace Params
  {
    const char PASSBACK[] = "passback";
    const char REQUEST_ID[] = "requestid";
  }
}

namespace AdServer
{
namespace Passback
{
  /**
   * Passback::Frontend implementation
   */
  Frontend::Frontend(
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
            frontend_config->get().PassFeConfiguration()->Logger().log_level())),
        0,
        Aspect::PASS_FRONTEND,
        0),
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        response_factory,
        frontend_config->get().PassFeConfiguration()->threads(),
        0), // max pending tasks
      grpc_container_(grpc_container),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      campaign_managers_(this->logger(), Aspect::PASS_FRONTEND)
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
        throw Exception("CommonFeConfiguration isn't present");
      }

      common_config_ = CommonConfigPtr(
        new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      if(!fe_config.PassFeConfiguration().present())
      {
        throw Exception("PassFeConfiguration isn't present");
      }

      config_ = ConfigPtr(
        new PassFeConfiguration(*(fe_config.PassFeConfiguration())));

      request_info_filler_.reset(
        new RequestInfoFiller(logger(), common_module_));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't parse config file '" << config_file_ << "'." <<
        ": " << e.what();
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
      ostr << "Passback::Frontend::will_handle(" <<
        uri << "), result " << result;

      logger()->log(ostr.str(), TraceLevel::MIDDLE, Aspect::PASS_FRONTEND);
    }

    return result;
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
    FrontendCommons::HttpResponse& response) noexcept
  {
    static const char* FUN = "Frontend::handle_request_()";

    try
    {
      // Checking requests validity
      PassFrontendHTTPConstrain::apply(request);

      return handle_redirect_request(
        request,
        response);
    }
    catch (const ForbiddenException& ex)
    {
      logger()->sstream(TraceLevel::LOW, Aspect::PASS_FRONTEND) <<
        FUN << ": ForbiddenException caught: " << ex.what();

      return 403;
    }
    catch (const InvalidParamException& ex)
    {
      logger()->sstream(TraceLevel::MIDDLE, Aspect::PASS_FRONTEND) <<
        FUN << ": InvalidParamException caught: " << ex.what();

      return 400;
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception has been caught: " << e.what();
      logger()->log(ostr.str(), Logging::Logger::EMERGENCY,
        Aspect::PASS_FRONTEND, "ADS-IMPL-191");

      return 500;
    }
  }

  int
  Frontend::handle_redirect_request(
    const FrontendCommons::HttpRequest& request,
    FrontendCommons::HttpResponse& response)
    /*throw(ForbiddenException, InvalidParamException, eh::Exception)*/
  {
    static const char* FUN = "Frontend::handle_redirect_request()";

    int http_status = 200;

    PassbackInfo passback_info;
    request_info_filler_->fill(
      passback_info,
      request.headers(),
      request.params());

    if(passback_info.passback_url.compare(0, 2, "//") == 0)
    {
      passback_info.passback_url =
        std::string(request.secure() ? "https:" : "http:") +
        passback_info.passback_url;
     }

    if(passback_info.request_id.is_null())
    {
      // make only redirect
      if(!passback_info.passback_url.empty())
      {
        http_status = FrontendCommons::redirect(
          passback_info.passback_url,
          response);
      }
      else
      {
        http_status = 204;
      }

      return http_status;
    }

    try
    {
      if(common_config_->ResponseHeaders().present())
      {
        FrontendCommons::add_headers(
          *(common_config_->ResponseHeaders()),
          response);
      }

      if(!passback_info.passback_url.empty())
      {
        http_status = FrontendCommons::redirect(
          passback_info.passback_url,
          response);
      }
      else
      {
        http_status = 204;
      }
    }
    catch(const eh::Exception& e)
    {
      http_status = 500;

      Stream::Error ostr;
      ostr << FUN << ":Caught eh::Exception on redirect. Url:'" <<
        passback_info.passback_url << "': " << e.what();
      logger()->log(ostr.str(), Logging::Logger::ERROR,
        Aspect::PASS_FRONTEND, "ADS-IMPL-194");
    }

    if(!passback_info.test_request)
    {
      AdServer::CampaignSvcs::CampaignManager::PassbackInfo info;
      info.request_id = CorbaAlgs::pack_request_id(passback_info.request_id);
      if(passback_info.user_id_hash_mod.present())
      {
        info.user_id_hash_mod.defined = true;
        info.user_id_hash_mod.value = *passback_info.user_id_hash_mod;
      }
      else
      {
        info.user_id_hash_mod.defined = false;
      }

      info.time = CorbaAlgs::pack_time(passback_info.time);

      campaign_managers_.consider_passback(info);
    }

    if(!passback_info.pubpixel_accounts.empty() &&
       !passback_info.current_user_id.is_null())
    {
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
            std::begin(passback_info.pubpixel_accounts),
            std::end(passback_info.pubpixel_accounts));

          auto response = grpc_distributor->confirm_user_freq_caps(
            GrpcAlgs::pack_user_id(passback_info.current_user_id),
            passback_info.time,
            GrpcAlgs::pack_request_id(Commons::RequestId{}),
            pubpixel_accounts);
          if (!response || response->has_error())
          {
            is_grpc_success = false;
            GrpcAlgs::print_grpc_error_response(
              response,
              logger(),
              Aspect::PASS_FRONTEND);
          }
        }
        catch (const eh::Exception& exc)
        {
          is_grpc_success = false;
          Stream::Error stream;
          stream << FUN
                 << ": "
                 << exc.what();
          logger()->error(stream.str(), Aspect::PASS_FRONTEND);
        }
        catch (...)
        {
          is_grpc_success = false;
          Stream::Error stream;
          stream << FUN
                 << ": Unknown error";
          logger()->error(stream.str(), Aspect::PASS_FRONTEND);
        }
      }

      // save freq caps
      AdServer::UserInfoSvcs::UserInfoMatcher_var
        uim_session = user_info_client_->user_info_session();

      if(!is_grpc_success && uim_session.in())
      {
        try
        {
          CORBACommons::IdSeq pubpixel_accounts;
          CorbaAlgs::fill_sequence(
            passback_info.pubpixel_accounts.begin(),
            passback_info.pubpixel_accounts.end(),
            pubpixel_accounts);

          uim_session->confirm_user_freq_caps(
            CorbaAlgs::pack_user_id(passback_info.current_user_id),
            CorbaAlgs::pack_time(passback_info.time),
            CorbaAlgs::pack_request_id(Commons::RequestId()),
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
            Aspect::PASS_FRONTEND,
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
            Aspect::PASS_FRONTEND);
        }
        catch (const CORBA::SystemException& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": CORBA::Exception caught: " << e;

          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::PASS_FRONTEND,
            "ADS-ICON-2");
        }
      }
    }

    return http_status;
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

        corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

        campaign_managers_.resolve(
          *common_config_, corba_client_adapter_);

        user_info_client_ = new FrontendCommons::UserInfoClient(
          common_config_->UserInfoManagerControllerGroup(),
          corba_client_adapter_.in(),
          logger(),
          grpc_container_->grpc_user_info_operation_distributor.in());

        add_child_object(user_info_client_);

        activate_object();
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      logger()->log(String::SubString(
          "Frontend::init(): frontend is running ..."),
        Logging::Logger::INFO, Aspect::PASS_FRONTEND);
    }
  }

  void
  Frontend::shutdown() noexcept
  {
    logger()->log(String::SubString(
        "Frontend::shutdown(): frontend terminated"),
      Logging::Logger::INFO, Aspect::PASS_FRONTEND);
  }

} /*Passback*/
} /*AdServer*/
