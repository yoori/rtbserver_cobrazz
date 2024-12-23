#include <Generics/Time.hpp>
#include <Logger/StreamLogger.hpp>
#include <Commons/ErrorHandler.hpp>
#include <Commons/CorbaAlgs.hpp>

#include "PassPixelFrontend.hpp"

namespace
{
  struct PassPixelFrontendConstrainTraits
  {
    static const unsigned long MAX_NUMBER_PARAMS = 50;
    static const unsigned long MAX_LENGTH_PARAM_NAME = 30;
    static const unsigned long MAX_LENGTH_PARAM_VALUE = 2000;
  };

  typedef FrontendCommons::DefaultConstrain<
    FrontendCommons::OnlyGetAllowed,
    FrontendCommons::ParamConstrainDefault,
    PassPixelFrontendConstrainTraits>
    PassPixelFrontendHTTPConstrain;
}

namespace Config
{
  const char CONFIG_FILE[] = "PassPixelFrontend_Config";
  const char ENABLE[] = "PassPixelFrontend_Enable";
}

namespace Aspect
{
  const char PASS_PIXEL_FRONTEND[] = "PassbackPixelFrontend";
}

namespace AdServer
{
namespace PassbackPixel
{
  /**
   * PassbackPixel::Frontend implementation
   */
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
            frontend_config->get().PassPixelFeConfiguration()->Logger().log_level())),
        0,
        Aspect::PASS_PIXEL_FRONTEND,
        0),
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        response_factory,
        frontend_config->get().PassPixelFeConfiguration()->threads(),
        0), // max pending tasks
      helper_task_processor_(helper_task_processor),
      grpc_container_(grpc_container),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      campaign_managers_(this->logger(), Aspect::PASS_PIXEL_FRONTEND)
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

      if(!fe_config.PassPixelFeConfiguration().present())
      {
        throw Exception("PassPixelFeConfiguration isn't present");
      }

      config_ = ConfigPtr(new PassPixelFeConfiguration(
        *fe_config.PassPixelFeConfiguration()));

      request_info_filler_.reset(new RequestInfoFiller(
        logger(),
        common_config_->colo_id(),
        common_config_->GeoIP().present() ? common_config_->GeoIP()->path().c_str() : 0));

      track_pixel_ = FileCachePtr(
        new FileCache(config_->track_pixel_path().c_str()));
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
      ostr << "Frontend::will_handle(" << uri << "), result " << result;

      logger()->log(ostr.str(), TraceLevel::MIDDLE, Aspect::PASS_PIXEL_FRONTEND);
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
      PassPixelFrontendHTTPConstrain::apply(request);

      return handle_track_request(request, response);
    }
    catch (const ForbiddenException& ex)
    {
      logger()->sstream(TraceLevel::LOW, Aspect::PASS_PIXEL_FRONTEND) <<
        FUN << ": ForbiddenException caught: " << ex.what();

      return 403;
    }
    catch (const InvalidParamException& ex)
    {
      logger()->sstream(TraceLevel::MIDDLE, Aspect::PASS_PIXEL_FRONTEND) <<
        FUN << ": InvalidParamException caught: " << ex.what();

      return 400;
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception has been caught: " << e.what();
      logger()->log(ostr.str(), Logging::Logger::EMERGENCY,
        Aspect::PASS_PIXEL_FRONTEND, "ADS-IMPL-191");

      return 500;
    }
  }

  int
  Frontend::handle_track_request(
    const FrontendCommons::HttpRequest& request,
    FrontendCommons::HttpResponse& response)
    /*throw(ForbiddenException, InvalidParamException, eh::Exception)*/
  {
    static const char* FUN = "Frontend::handle_track_request()";

    int http_status = 200;

    PassbackTrackInfo passback_track_info;

    request_info_filler_->fill_track(
      passback_track_info,
      request.headers(),
      request.params());

    if(!passback_track_info.tag_id)
    {
      Stream::Error ostr;
      ostr << "Not correct tag_id";
      throw InvalidParamException(ostr);
    }

    try
    {
      if(common_config_->ResponseHeaders().present())
      {
        FrontendCommons::add_headers(
          *(common_config_->ResponseHeaders()),
          response);
      }

      response.set_content_type(String::SubString("image/gif"));

      FileCache::BufferHolder_var buffer = track_pixel_->get();
      response.get_output_stream().write((*buffer)->data(), (*buffer)->size());
    }
    catch(const eh::Exception& e)
    {
      http_status = 500;
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << e.what();
      logger()->log(ostr.str(), Logging::Logger::ERROR,
        Aspect::PASS_PIXEL_FRONTEND, "ADS-IMPL-194");
    }

    bool is_grpc_success = false;
    auto grpc_campaign_manager_pool = grpc_container_->grpc_campaign_manager_pool;
    if (grpc_campaign_manager_pool)
    {
      is_grpc_success = true;

      auto response = grpc_campaign_manager_pool->consider_passback_track(
        passback_track_info.time,
        passback_track_info.country,
        passback_track_info.colo_id,
        passback_track_info.tag_id,
        passback_track_info.user_status);
      if (!response || response->has_error())
      {
        is_grpc_success = false;
      }
    }

    if (!is_grpc_success)
    {
      AdServer::CampaignSvcs::CampaignManager::PassbackTrackInfo info;
      info.time = CorbaAlgs::pack_time(passback_track_info.time);
      info.country << passback_track_info.country;
      info.colo_id = passback_track_info.colo_id;
      info.tag_id = passback_track_info.tag_id;
      info.user_status = passback_track_info.user_status;
      campaign_managers_.consider_passback_track(info);
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
      Logging::Logger::INFO, Aspect::PASS_PIXEL_FRONTEND);
    }
  }

  void
  Frontend::shutdown() noexcept
  {
    logger()->log(String::SubString(
        "Frontend::shutdown(): frontend terminated"),
      Logging::Logger::INFO, Aspect::PASS_PIXEL_FRONTEND);
  }
} /*PassbackPixel*/
} /*AdServer*/
