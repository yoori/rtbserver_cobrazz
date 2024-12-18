
#include <sstream>

#include <Generics/Time.hpp>
#include <Logger/StreamLogger.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>
#include <String/StringManip.hpp>
#include <PrivacyFilter/Filter.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/UserInfoManip.hpp>

#include <Frontends/FrontendCommons/add_UID_cookie.hpp>
#include <Frontends/FrontendCommons/OptOutManip.hpp>

#include "OptoutFrontend.hpp"

namespace
{
  struct OptoutFrontendConstrainTraits
  {
    static const unsigned long MAX_NUMBER_PARAMS = 50;
    static const unsigned long MAX_LENGTH_PARAM_NAME = 30;
    static const unsigned long MAX_LENGTH_PARAM_VALUE = 500;
  };

  typedef FrontendCommons::DefaultConstrain<
    FrontendCommons::OnlyGetAllowed,
    FrontendCommons::ParamConstrainDefault,
    OptoutFrontendConstrainTraits>
      OptoutFrontendHTTPConstrain;

  const unsigned long SECONDS_IN_DAY  = 24 * 60 * 60;

  const char LOG_DELIMITER[] = " :: ";
}

namespace Config
{
  const char CONFIG_FILE[] = "OptOutFrontend_Config";
  const char ENABLE[] = "OptOutFrontend_Enable";
}

namespace Aspect
{
  extern const char OPTOUT_FRONTEND[] = "OptoutFrontend";
  const char OPTOUT[] = "Opt-Out";
  const char OPTIN[] = "Opt-In";
  const char STATUS[] = "Status";
}

namespace Request
{
  namespace Cookie
  {
    const String::AsciiStringManip::Caseless OPTOUT_TRUE_VALUE("YES");
    const Generics::SubStringHashAdapter OPTOUT(String::SubString("OPTED_OUT"));
  }

  namespace Context
  {
    const Generics::SubStringHashAdapter CLIENT_ID(String::SubString("uid"));
  }
}

namespace AdServer
{
  /**
   * OptoutFrontend implementation
   */
  OptoutFrontend::OptoutFrontend(
    TaskProcessor& helper_task_processor,
    const GrpcContainerPtr& grpc_container,
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module,
    FrontendCommons::HttpResponseFactory* response_factory) /*throw(eh::Exception)*/
    : FrontendCommons::FrontendInterface(response_factory),
      Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().OptOutFeConfiguration()->Logger().log_level())),
        "OptoutFrontend",
        Aspect::OPTOUT_FRONTEND,
        0),
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        response_factory,
        frontend_config->get().OptOutFeConfiguration()->threads(),
        0), // max pending tasks
      helper_task_processor_(helper_task_processor),
      grpc_container_(grpc_container),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      campaign_managers_(this->logger(), Aspect::OPTOUT_FRONTEND)
  {}

  void
  OptoutFrontend::parse_config_() /*throw(Exception)*/
  {
    Config::ErrorHandler error_handler;

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

      if(!fe_config.OptOutFeConfiguration().present())
      {
        throw Exception("OptOutFeConfiguration isn't present");
      }

      config_.reset(
        new OptOutFeConfiguration(*fe_config.OptOutFeConfiguration()));

      cookie_manager_.reset(
        new FrontendCommons::CookieManager<
          FrontendCommons::HttpRequest, FrontendCommons::HttpResponse>(
            common_config_->Cookies()));

    }
    catch(const xml_schema::parsing& e)
    {
      std::string str;
      Stream::Error ostr;
      ostr << "Can't parse config file '" << config_file_ << "': " <<
        error_handler.text(str);
      throw Exception(ostr);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "Can't parse config file '" << config_file_ << "': " <<
        e.what();
      throw Exception(ostr);
    }
  }

  bool
  OptoutFrontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;

    bool result = FrontendCommons::find_uri(
      config_->UriList().Uri(), uri, found_uri);

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "OptoutFrontend::will_handle(" << uri << "), result " << result;

      logger()->log(ostr.str(),
        TraceLevel::MIDDLE,
        Aspect::OPTOUT_FRONTEND);
    }

    return result;
  }

  void
  OptoutFrontend::handle_request_(
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
  OptoutFrontend::handle_request_(
    const FrontendCommons::HttpRequest& request,
    HttpResponse& response)
    noexcept
  {
    static const char* FUN = "OptoutFrontend::handle_request_()";
    int http_result = 200;

    OptOut::RequestInfo request_info;
    std::string verify_referer;// FIXME: not used

    AdServer::CampaignSvcs::CampaignManager::OptOperation ver_operation =
      AdServer::CampaignSvcs::CampaignManager::OO_OUT;
    const char* local_aspect = Aspect::OPTIN;

    try
    {
      try
      {
        OptoutFrontendHTTPConstrain::apply(request);

        HTTP::HTTPChecker checker;

        //-----------------------------------------------------------------------
        // 1. Load required request parameters
        //-----------------------------------------------------------------------
        const HTTP::SubHeaderList& headers = request.headers();
        HTTP::CookieList cookies;

        request_info_filler_->fill(
          request_info,
          headers,
          request.params(),
          cookies);

        //-----------------------------------------------------------------------
        // 2. Handle oo Cookie request
        //-----------------------------------------------------------------------

        unsigned long status = 0;

        AdServer::Commons::UserId new_user_id;

        if (request_info.oo_operation != OO_STATUS)
        {
          bool oo_to_set =
           (request_info.old_oo_type != request_info.oo_operation);
          std::string new_client_id;

          // Place log record to apache log
          if (request_info.oo_operation == OO_OUT) /* Opt-Out */
          {
            ver_operation = AdServer::CampaignSvcs::CampaignManager::OO_OUT;

            std::list<std::string> remove_cookie_list_holder;
            FrontendCommons::CookieNameSet remove_cookie_list;
            for(auto it = common_config_->OptOutRemoveCookies().Cookie().begin();
              it != common_config_->OptOutRemoveCookies().Cookie().end(); ++it)
            {
              remove_cookie_list_holder.push_back(it->name());
              remove_cookie_list.insert(remove_cookie_list_holder.back());
            }

            cookie_manager_->remove(response, request, cookies, remove_cookie_list);

            set_OO_cookie(
              Request::Cookie::OPTOUT_TRUE_VALUE.str,
              response,
              request,
              request_info.cookie_expire_time);
          }
          else if (request_info.oo_operation == OO_IN)
          {
            ver_operation = AdServer::CampaignSvcs::CampaignManager::OO_IN;

            if(oo_to_set)
            {
              // generate uid
              Generics::SignedUuid signed_uid =
                common_module_->user_id_controller()->generate();
              new_user_id = signed_uid.uuid();
              new_client_id = signed_uid.str();

              FrontendCommons::add_UID_cookie(
                response,
                request,
                *cookie_manager_,
                new_client_id);

              FrontendCommons::CookieNameSet remove_cookies;
              remove_cookies.insert(Request::Cookie::OPTOUT);
              cookie_manager_->remove(response, request, cookies, remove_cookies);
            }
          }
          status =
            calculate_status_(request_info.oo_operation,
              request_info.old_oo_type);

          if(oo_to_set)
          {
            if (!request_info.oo_success_redirect_url.empty())
            {
              http_result = FrontendCommons::redirect(
                request_info.oo_success_redirect_url, response);
            }
          }
          else
          {
            if (!request_info.oo_already_redirect_url.empty())
            {
              http_result = FrontendCommons::redirect(
                request_info.oo_already_redirect_url, response);
            }
          }

          if(logger()->log_level() >= Logging::Logger::TRACE)
          {
            // Log info about request
            std::ostringstream ostr;
            ostr << (request_info.oo_operation == OO_OUT ?
                request_info.user_id.to_string() : new_client_id) <<
              LOG_DELIMITER <<
              PrivacyFilter::filter(
                request_info.peer_ip.c_str(), "IP") << LOG_DELIMITER <<
              request_info.user_agent << LOG_DELIMITER <<
              verify_referer << LOG_DELIMITER <<
              status << LOG_DELIMITER <<
              request_info.cookie_expire_time.tv_sec;

            logger()->log(
              ostr.str(),
              Logging::Logger::TRACE,
              local_aspect);
          }
        } /* oo_operation != OO_STATUS */
        else
        {
          http_result = handle_status_operation_(
            request_info.old_oo_type,
            request_info.oo_status_in_redirect_url,
            request_info.oo_status_out_redirect_url,
            request_info.oo_status_undef_redirect_url, response);
          ver_operation = AdServer::CampaignSvcs::CampaignManager::OO_STATUS;
          status = (request_info.old_oo_type == OO_IN ? 3 :
            (request_info.old_oo_type == OO_OUT ? 4 : 5));
        }

        bool is_grpc_success = false;
        const auto grpc_campaign_manager_pool = grpc_container_->grpc_campaign_manager_pool;
        if (grpc_campaign_manager_pool)
        {
          using OptOperation = AdServer::CampaignSvcs::Proto::OptOperation;
          OptOperation ver_operation_proto = OptOperation::OO_OUT;
          switch (ver_operation)
          {
            case AdServer::CampaignSvcs::CampaignManager::OO_IN:
            {
              ver_operation_proto = OptOperation::OO_IN;
              break;
            }
            case AdServer::CampaignSvcs::CampaignManager::OO_OUT:
            {
              ver_operation_proto = OptOperation::OO_OUT;
              break;
            }
            case AdServer::CampaignSvcs::CampaignManager::OO_STATUS:
            {
              ver_operation_proto = OptOperation::OO_STATUS;
              break;
            }
            case AdServer::CampaignSvcs::CampaignManager::OO_FORCED_IN:
            {
              ver_operation_proto = OptOperation::OO_FORCED_IN;
              break;
            }
            default:
            {
              assert(false);
            }
          }
          auto response = grpc_campaign_manager_pool->verify_opt_operation(
            request_info.debug_time.tv_sec,
            request_info.colo_id,
            verify_referer,
            ver_operation_proto,
            static_cast<CORBA::ULong>(status),
            static_cast<CORBA::ULong>(request_info.user_status),
            request_info.log_as_test,
            request_info.browser,
            request_info.os,
            request_info.ct,
            request_info.curct,
            new_user_id);
          if (response && response->has_info())
          {
            is_grpc_success = true;
          }
        }

        if (!is_grpc_success)
        {
          campaign_managers_.verify_opt_operation(
            request_info.debug_time.tv_sec,
            request_info.colo_id,
            verify_referer.c_str(),
            ver_operation,
            static_cast<CORBA::ULong>(status),
            static_cast<CORBA::ULong>(request_info.user_status),
            request_info.log_as_test,
            request_info.browser.c_str(),
            request_info.os.c_str(),
            request_info.ct.c_str(),
            request_info.curct.c_str(),
            request_info.local_aspect,
            new_user_id);
        }

        if(stats_)
        {
          stats_->consider_request(request_info.oo_operation);
        }

        // additional response headers
        if(common_config_->ResponseHeaders().present())
        {
          FrontendCommons::add_headers(
            *(common_config_->ResponseHeaders()),
            response);
        }

        if (request_info.oo_debug == "yes")
        {
          std::ostringstream dostr;
          dostr << "Debug time: "
            << request_info.debug_time.get_gm_time().format(
              "%d-%m-%Y %H:%M:%S GMT ")
            << std::endl
            << " IP: " << PrivacyFilter::filter(
              request_info.peer_ip.c_str(), "IP")
            << std::endl
            << " User-ID: " << request_info.user_id.to_string() << std::endl
            << " Status: "
            << (request_info.old_oo_type == OO_IN ? 3 :
              (request_info.old_oo_type == OO_OUT ? 4 : 5));

          const std::string& res = dostr.str();
          response.get_output_stream().write(res.data(), res.size());
        }
      }
      catch (const ForbiddenException& ex)
      {
        http_result = 403;
        logger()->sstream(TraceLevel::LOW, Aspect::OPTOUT_FRONTEND)
          << FUN << ": ForbiddenException caught: " << ex.what();
      }
      catch (const InvalidParamException& ex)
      {
        http_result = 400;
        logger()->sstream(TraceLevel::MIDDLE, Aspect::OPTOUT_FRONTEND)
          << FUN << ": InvalidParamException caught: " << ex.what();
      }
      catch(const BadParameter& e)
      {
        Stream::Error ostr;

        ostr << __func__ <<
          ": BadParameter exception has been caught: " <<
          e.what();

        logger()->log(
          ostr.str(),
          Logging::Logger::NOTICE,
          Aspect::OPTOUT_FRONTEND);

        http_result = 400;
      }
      catch(const Exception& e)
      {
        Stream::Error ostr;

        ostr << FUN <<
          ": Exception has been caught: " <<
          e.what();

        logger()->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          Aspect::OPTOUT_FRONTEND,
          "ADS-IMPL-139");

        throw RequestFailure("");
      }
    }
    catch(const RequestFailure& e)
    {
      try
      {
        if(request_info.oo_operation != OO_NOT_DEFINED)
        {
          std::ostringstream ostr;

          ostr << (request_info.oo_operation == OO_OUT ?
            request_info.user_id.to_string().c_str() : "") << LOG_DELIMITER <<
            PrivacyFilter::filter(
              request_info.peer_ip.c_str(), "IP") << LOG_DELIMITER <<
            request_info.user_agent << LOG_DELIMITER <<
            verify_referer << LOG_DELIMITER <<
            "0"/*failure*/;

          bool is_grpc_success = false;
          const auto grpc_campaign_manager_pool = grpc_container_->grpc_campaign_manager_pool;
          if (grpc_campaign_manager_pool)
          {
            using OptOperation = AdServer::CampaignSvcs::Proto::OptOperation;
            OptOperation ver_operation_proto = OptOperation::OO_OUT;
            switch (ver_operation)
            {
              case AdServer::CampaignSvcs::CampaignManager::OO_IN:
              {
                ver_operation_proto = OptOperation::OO_IN;
                break;
              }
              case AdServer::CampaignSvcs::CampaignManager::OO_OUT:
              {
                ver_operation_proto = OptOperation::OO_OUT;
                break;
              }
              case AdServer::CampaignSvcs::CampaignManager::OO_STATUS:
              {
                ver_operation_proto = OptOperation::OO_STATUS;
                break;
              }
              case AdServer::CampaignSvcs::CampaignManager::OO_FORCED_IN:
              {
                ver_operation_proto = OptOperation::OO_FORCED_IN;
                break;
              }
              default:
              {
                assert(false);
              }
            }
            auto response = grpc_campaign_manager_pool->verify_opt_operation(
              request_info.debug_time.tv_sec,
              request_info.colo_id,
              verify_referer,
              ver_operation_proto,
              0,
              0,
              request_info.log_as_test,
              request_info.browser,
              request_info.os,
              request_info.ct,
              request_info.curct,
              AdServer::Commons::UserId{});
            if (response && response->has_info())
            {
              is_grpc_success = true;
            }
          }

          if (!is_grpc_success)
          {
            campaign_managers_.verify_opt_operation(
              request_info.debug_time.tv_sec,
              request_info.colo_id,
              verify_referer.c_str(),
              ver_operation,
              0,
              request_info.log_as_test,
              request_info.browser.c_str(),
              request_info.os.c_str(),
              request_info.ct.c_str(),
              request_info.curct.c_str(),
              request_info.local_aspect);
          }

          logger()->log(
            ostr.str(),
            Logging::Logger::INFO,
            (request_info.oo_operation == OO_OUT ?
               (Aspect::OPTOUT): (Aspect::OPTIN)));
        }

        if (request_info.oo_failure_redirect_url.empty())
        {
          http_result = 400;
        }
        else
        {
          http_result = FrontendCommons::redirect(
            request_info.oo_failure_redirect_url, response);
        }
      }
      catch(...)
      {
        http_result = 500;
      }
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << e.what();

      logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::OPTOUT_FRONTEND,
        "ADS-IMPL-139");
      http_result = 500;
    }

    return http_result;
  }

  void
  OptoutFrontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "OptoutFrontend::init()";

    if(true) // module_used())
    {
      try
      {
        parse_config_();

        corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();

        campaign_managers_.resolve(
          *common_config_, corba_client_adapter_);

        if(common_config_->StatsDumper().present())
        {
          CORBACommons::CorbaObjectRef dumper_ref;

          Config::CorbaConfigReader::read_corba_ref(
            common_config_->StatsDumper().get().StatsDumperRef(),
            dumper_ref);

          stats_ = new OptOutFrontendStat(
            logger(),
            dumper_ref,
            0,
            Generics::Time(common_config_->StatsDumper().get().period()),
            callback());

          add_child_object(stats_);
        }

        request_info_filler_.reset(
          new OptOut::RequestInfoFiller(config_.get(), logger(), common_module_));

        activate_object();
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      logger()->log(String::SubString(
          "OptoutFrontend::init: frontend is running ..."),
        Logging::Logger::INFO,
        Aspect::OPTOUT_FRONTEND);
    }
  }

  void
  OptoutFrontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();

      logger()->log(String::SubString(
            "OptoutFrontend::shutdown: frontend terminated"),
          Logging::Logger::INFO,
          Aspect::OPTOUT_FRONTEND);
    }
    catch(...)
    {
    }
  }

  //
  // private partition
  //

  void
  OptoutFrontend::set_OO_cookie(
    const String::SubString& oo_value,
    HttpResponse& response,
    const FrontendCommons::HttpRequest& request,
    const Generics::Time& cookie_expire_time)
    /*throw(eh::Exception)*/
  {
    cookie_manager_->set(
      response,
      request,
      Request::Cookie::OPTOUT,
      oo_value,
      cookie_expire_time);
  }

  unsigned long OptoutFrontend::calculate_status_(
    OptOperation operation,
    OptOperation old_operation) noexcept
  {
    if(operation == OO_OUT)
    {
      if(old_operation == OO_OUT)
      {
        return 2; //FAILED - OPTED_OUT present
      }
      else if(old_operation == OO_IN)
      {
        return 11; //OK - UID present
      }
      else
      {
        return 10; //OK - UID adsent
      }
    }
    else if(operation == OO_IN)
    {
      if(old_operation == OO_IN)
      {
        return 2; //FAILED UID present
      }
      else if(old_operation == OO_OUT)
      {
        return 10; //OK - OPTED_OUT present
      }
      else
      {
        return 11; //OK - OPTED_OUT absent
      }
    }
    return 0; //FAILED
  }

  int OptoutFrontend::handle_status_operation_(
    const OptOperation old_oo_type,
    const std::string& oo_status_in_redirect_url,
    const std::string& oo_status_out_redirect_url,
    const std::string& oo_status_undef_redirect_url,
    HttpResponse& response) noexcept
  {
    const std::string* r_url = 0;
    switch(old_oo_type)
    {
      case OO_IN:
        r_url = &oo_status_in_redirect_url;
        break;
      case OO_OUT:
        r_url = &oo_status_out_redirect_url;
        break;
      default:
        r_url =& oo_status_undef_redirect_url;
    }
    if(!r_url->empty())
    {
      return FrontendCommons::redirect(*r_url, response);
    }
    return 200;
  }

}
