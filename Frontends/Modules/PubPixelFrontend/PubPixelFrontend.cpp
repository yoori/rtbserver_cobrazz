#include <sstream>

#include <Generics/Rand.hpp>
#include <Logger/StreamLogger.hpp>
#include <String/StringManip.hpp>
#include <String/AsciiStringManip.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>

#include <CORBACommons/CorbaAdapters.hpp>

#include <Commons/CorbaConfig.hpp>
#include <Commons/CorbaAlgs.hpp>
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
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        frontend_config->get().PubPixelFeConfiguration()->threads(),
        0), // max pending tasks
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      campaign_managers_(this->logger(), Aspect::PUBPIXEL_FRONTEND)
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

      corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();
      
      campaign_managers_.resolve(
        *common_config_, corba_client_adapter_);

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
      corba_client_adapter_.reset();

      log(String::SubString(
          "PubPixel::Frontend::shutdown: frontend terminated"),
        Logging::Logger::INFO,
        Aspect::PUBPIXEL_FRONTEND);
    }
    catch(...)
    {}
  }

  void
  Frontend::handle_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::HttpResponseWriter_var response_writer)
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
    FCGI::HttpResponse& response)
    noexcept
  {
    static const char* FUN = "Frontend::handle_request_()";

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
        return 403; // HTTP_FORBIDDEN
      }

      RequestInfo request_info;

      request_info_filler_->fill(request_info, request.headers(), request.params());

      if(request_info.user_status == CampaignSvcs::US_UNDEFINED)
      {
        http_status = 400; // HTTP_BAD_REQUEST
      }
      else
      {
        AdServer::CampaignSvcs::StringSeq_var pub_pixels_ptr;
        AdServer::CampaignSvcs::PublisherAccountIdSeq publisher_account_ids;

        CorbaAlgs::fill_sequence(
          request_info.publisher_account_ids.begin(),
          request_info.publisher_account_ids.end(),
          publisher_account_ids);

        pub_pixels_ptr =
          campaign_managers_.get_pub_pixels(
            request_info.country.present() ? request_info.country->c_str() : "",
            request_info.user_status,
            publisher_account_ids);

        const AdServer::CampaignSvcs::StringSeq& pub_pixels = *pub_pixels_ptr;

        response.set_content_type(FrontendCommons::ContentType::TEXT_HTML);
        if(common_config_->ResponseHeaders().present())
        {
          FrontendCommons::add_headers(
            *(common_config_->ResponseHeaders()),
            response);
        }

        if (pub_pixels.length())
        {
          static const char HEAD[] = "<!DOCTYPE html><html><head><title></title></head><body>";
          response.get_output_stream().write(HEAD, sizeof(HEAD) - 1);

          for(CORBA::ULong pixel_i = 0; pixel_i < pub_pixels.length(); ++pixel_i)
          {
            response.get_output_stream().write(
              pub_pixels[pixel_i].in(),
              ::strlen(pub_pixels[pixel_i].in()));
          }

          static const char TAIL[] = "</body></html>";
          response.get_output_stream().write(TAIL, sizeof(TAIL) - 1);
        }
        else
        {
          http_status = 204; // HTTP_NO_CONTENT
        }
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

    return http_status;
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
