
#include <Generics/Time.hpp>
#include <array>
#include <utility>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/Containers.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>

#include <Frontends/FrontendCommons/UserInfoClientConfig.hpp>

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

namespace
{
  std::string
  pack_request_id(const AdServer::Commons::RequestId& request_id)
  {
    return std::string(
      reinterpret_cast<const char*>(request_id.begin()),
      reinterpret_cast<const char*>(request_id.end()));
  }

  std::string
  pack_time(const Generics::Time& time)
  {
    std::array<unsigned char, Generics::Time::TIME_PACK_LEN> packed_time;
    time.pack(packed_time.data());
    return std::string(
      reinterpret_cast<const char*>(packed_time.data()),
      packed_time.size());
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
    Configuration* frontend_config,
    Logging::Logger* logger,
    CommonModule* common_module)
    /*throw(eh::Exception)*/
    : Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().PassFeConfiguration()->Logger().log_level())),
        0,
        Aspect::PASS_FRONTEND,
        0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module))
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
  Frontend::handle_request(
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
  Frontend::handle_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    const FCGI::HttpRequest& request = request_holder->request();

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());
    FCGI::HttpResponse& response = *response_ptr;
    int http_status = handle_request_(request, response);
    response_writer->write(http_status, response_ptr);
  }

  int
  Frontend::handle_request_(
    const FCGI::HttpRequest& request,
    FCGI::HttpResponse& response) noexcept
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
    const FCGI::HttpRequest& request,
    FCGI::HttpResponse& response)
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
      auto info = std::make_shared<
        adserver::campaign_svcs::campaign_manager::ConsiderPassbackRequest>();
      info->set_request_id(pack_request_id(passback_info.request_id));
      info->set_time(pack_time(passback_info.time));
      if(passback_info.user_id_hash_mod.present())
      {
        auto* user_id_hash_mod = info->mutable_user_id_hash_mod();
        user_id_hash_mod->set_defined(true);
        user_id_hash_mod->set_value(*passback_info.user_id_hash_mod);
      }

      campaign_manager_->consider_passback(
        *info,
        [this, info](
          const grpc::Status& status,
          const adserver::campaign_svcs::campaign_manager::
            ConsiderPassbackResponse&)
        {
          if(!status.ok())
          {
            Stream::Error ostr;
            ostr << FUN << ": CampaignManager::consider_passback(): "
              "gRPC call failed: code=" <<
              static_cast<int>(status.error_code()) <<
              ", message=" << status.error_message();
            logger()->log(
              ostr.str(),
              Logging::Logger::ERROR,
              Aspect::PASS_FRONTEND,
              "ADS-IMPL-194");
          }
        });
    }

    if(!passback_info.pubpixel_accounts.empty() &&
       !passback_info.current_user_id.is_null())
    {
      // save freq caps
      if(user_info_client_)
      {
        try
        {
          auto confirm_request = std::make_shared<
            adserver::user_info_svcs::user_info_manager::
              ConfirmUserFreqCapsRequest>();
          confirm_request->set_user_id(GrpcAlgs::pack_user_id(
            passback_info.current_user_id));
          confirm_request->set_time(GrpcAlgs::pack_time(passback_info.time));
          confirm_request->set_request_id(GrpcAlgs::pack_request_id(
            Commons::RequestId()));
          for(const auto account_id : passback_info.pubpixel_accounts)
          {
            confirm_request->add_exclude_pubpixel_accounts(account_id);
          }

          user_info_client_->confirm_user_freq_caps(
            *confirm_request,
            [this, confirm_request](
              const grpc::Status& status,
              const adserver::user_info_svcs::user_info_manager::
                ConfirmUserFreqCapsResponse&)
            {
              if(!status.ok())
              {
                Stream::Error ostr;
                ostr << "UserInfoManagerGrpc::confirm_user_freq_caps(): "
                  "gRPC call failed: code=" <<
                  static_cast<int>(status.error_code()) <<
                  ", message=" << status.error_message();
                logger()->log(
                  ostr.str(),
                  status.error_code() == grpc::StatusCode::UNAVAILABLE ?
                    Logging::Logger::WARNING :
                    Logging::Logger::EMERGENCY,
                  Aspect::PASS_FRONTEND,
                  status.error_code() == grpc::StatusCode::UNAVAILABLE ?
                    "" : "ADS-IMPL-123");
              }
            });
        }
        catch(const eh::Exception& e)
        {
          Stream::Error ostr;
          ostr << FUN << ": confirm_user_freq_caps preparation failed: " <<
            e.what();

          logger()->log(ostr.str(),
            Logging::Logger::EMERGENCY,
            Aspect::PASS_FRONTEND,
            "ADS-IMPL-123");
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

        workers_ = new FrontendCommons::FrontendWorkers(
          callback(),
          config_->threads());
        add_child_object(workers_);

        grpc_executor_ = std::make_shared<AdServer::Grpc::GrpcExecutor>(
          common_config_->grpc_executor_threads());
        add_child_object(grpc_executor_);

        auto campaign_manager = std::make_shared<
          AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
            FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
            AdServer::Grpc::BatchingOptions(),
            grpc_executor_);
        campaign_manager_ = campaign_manager;
        add_child_object(campaign_manager);

        user_info_client_ =
          AdServer::UserInfoSvcs::create_distributed_user_info_client(
            *common_config_,
            grpc_executor_,
            logger(),
            this);

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
    deactivate_object();
    wait_object();
    campaign_manager_.reset();
    user_info_client_.reset();

    logger()->log(String::SubString(
        "Frontend::shutdown(): frontend terminated"),
      Logging::Logger::INFO, Aspect::PASS_FRONTEND);
  }

} /*Passback*/
} /*AdServer*/
