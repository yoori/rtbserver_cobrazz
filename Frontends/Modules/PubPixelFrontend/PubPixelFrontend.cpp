#include <sstream>
#include <utility>

#include <Generics/Rand.hpp>
#include <Logger/StreamLogger.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/UserInfoManip.hpp>

#include <Frontends/FrontendCommons/HTTPUtils.hpp>

#include "PubPixelFrontend.hpp"

namespace
{
  namespace Aspect
  {
    const char PUBPIXEL_FRONTEND[] = "PubPixel::Frontend";
  }
}

namespace AdServer
{
namespace PubPixel
{
  struct FrontendConstrainTraits
  {
    static const unsigned long MAX_NUMBER_PARAMS = 30;
    static const unsigned long MAX_LENGTH_PARAM_NAME = 20;
    static const unsigned long MAX_LENGTH_PARAM_VALUE = 1024;
  };

  Frontend::Frontend(
    Configuration* frontend_config,
    Logging::Logger* logger)
    /*throw(eh::Exception)*/
    : Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().PubPixelFeConfiguration()->Logger().log_level())),
        "PubPixelFrontend",
        Aspect::PUBPIXEL_FRONTEND, 0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config))
  {}

  bool
  Frontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;
    bool result = false;

    if (!uri.empty())
    {
      result = FrontendCommons::find_uri(
        config_->UriList().Uri(), uri, found_uri);
    }

    if(log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "PubPixel::Frontend::will_handle(" << uri <<
        "), result " << result << ", for service: '" <<
        found_uri << "'";

      log(ostr.str());
    }

    return result;
  }

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

      if(!fe_config.PubPixelFeConfiguration().present())
      {
        throw Exception("PubPixelFeConfiguration not presented.");
      }

      config_ = ConfigPtr(
        new PubPixelFeConfiguration(*fe_config.PubPixelFeConfiguration()));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << e.what();
      throw Exception(ostr);
    }
  }

  void
  Frontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "Frontend::init()";

    try
    {
      parse_config_();

      request_info_filler_.reset(new RequestInfoFiller(
        logger(),
        common_config_->GeoIP().present() ?
        common_config_->GeoIP()->path().c_str() : 0));

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

      activate_object();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }

    logger()->log(String::SubString(
      "PubPixel::Frontend::init(): frontend is running ..."),
      Logging::Logger::INFO,
      Aspect::PUBPIXEL_FRONTEND);
  }

  void
  Frontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();
      campaign_manager_.reset();

      log(String::SubString(
          "PubPixel::Frontend::shutdown: frontend terminated"),
        Logging::Logger::INFO,
        Aspect::PUBPIXEL_FRONTEND);
    }
    catch(...)
    {}
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
    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());
    process_request_(
      std::move(request_holder),
      std::move(response_writer),
      std::move(response_ptr));
  }

  void
  Frontend::process_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer,
    FCGI::HttpResponse_var response)
    noexcept
  {
    static const char* FUN = "Frontend::handle_request_()";
    const FCGI::HttpRequest& request = request_holder->request();

    log(String::SubString("Frontend::handle_request: entered"),
      TraceLevel::MIDDLE,
      Aspect::PUBPIXEL_FRONTEND);

    int http_status = 0; // OK

    try
    {
      std::string found_uri;

      if(!FrontendCommons::find_uri(
           config_->UriList().Uri(), request.uri(), found_uri))
      {
        response_writer->write(403, response);
        return;
      }

      RequestInfo request_info;

      request_info_filler_->fill(request_info, request.headers(), request.params());

      if(request_info.user_status == CampaignSvcs::US_UNDEFINED)
      {
        http_status = 400; // HTTP_BAD_REQUEST
      }
      else
      {
        auto pub_pixels_request = std::make_shared<
          adserver::campaign_svcs::campaign_manager::GetPubPixelsRequest>();
        pub_pixels_request->set_country(
          request_info.country.present() ? *request_info.country : "");
        pub_pixels_request->set_user_status(request_info.user_status);
        for(const auto publisher_account_id :
          request_info.publisher_account_ids)
        {
          pub_pixels_request->add_publisher_account_ids(publisher_account_id);
        }

        campaign_manager_->get_pub_pixels(
          *pub_pixels_request,
          [
            this,
            response_writer,
            response,
            pub_pixels_request
          ](
            const grpc::Status& status,
            const adserver::campaign_svcs::campaign_manager::
              GetPubPixelsResponse& pub_pixels)
          {
            int http_status = 0;
            if(!status.ok())
            {
              http_status = 500;
              Stream::Error error;
              error << "CampaignManager get_pub_pixels failed: code=" <<
                static_cast<int>(status.error_code()) <<
                ", message=" << status.error_message();
              log(
                error.str(),
                Logging::Logger::EMERGENCY,
                Aspect::PUBPIXEL_FRONTEND);
            }
            else
            {
              response->set_content_type_nocopy(
                FrontendCommons::ContentType::TEXT_HTML);
              if(common_config_->ResponseHeaders().present())
              {
                FrontendCommons::add_headers(
                  *(common_config_->ResponseHeaders()),
                  *response);
              }

              if(pub_pixels.pixels_size())
              {
                static const char HEAD[] =
                  "<!DOCTYPE html><html><head><title></title></head><body>";
                response->get_output_stream().write(HEAD, sizeof(HEAD) - 1);

                for(const auto& pixel : pub_pixels.pixels())
                {
                  response->get_output_stream().write(
                    pixel.data(),
                    pixel.size());
                }

                static const char TAIL[] = "</body></html>";
                response->get_output_stream().write(TAIL, sizeof(TAIL) - 1);
              }
              else
              {
                http_status = 204; // HTTP_NO_CONTENT
              }
            }

            response_writer->write(http_status, response);
          });
        return;
      }
    }
    catch (const ForbiddenException& ex)
    {
      http_status = 403; // HTTP_FORBIDDEN
      logger()->sstream(TraceLevel::LOW, Aspect::PUBPIXEL_FRONTEND) <<
        FUN << ": ForbiddenException caught: " << ex.what();
    }
    catch (const InvalidParamException& ex)
    {
      http_status = 400; // HTTP_BAD_REQUEST
      logger()->sstream(TraceLevel::MIDDLE, Aspect::PUBPIXEL_FRONTEND) <<
        FUN << ": InvalidParamException caught: " << ex.what();
    }
    catch(const eh::Exception& e)
    {
      http_status = 500; // HTTP_INTERNAL_SERVER_ERROR
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught:" << e.what();

      log(ostr.str(),
        Logging::Logger::EMERGENCY,
        Aspect::PUBPIXEL_FRONTEND);
    }

    response_writer->write(http_status, response);
  }

  bool
  Frontend::log(
    const String::SubString& text,
    unsigned long severity,
    const char* aspect,
    const char* error_code) const
    noexcept
  {
    if (!logger())
    {
      try
      {
        std::cerr << text << std::endl;
        return true;
      }
      catch(...)
      {
        return false;
      }
    }
    else
    {
      return logger()->log(text, severity, aspect, error_code);
    }
  }
} /* PubPixel */
} /* AdServer */
