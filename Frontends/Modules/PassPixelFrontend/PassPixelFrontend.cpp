#include <Generics/Time.hpp>
#include <array>
#include <utility>
#include <Logger/StreamLogger.hpp>
#include <Commons/ErrorHandler.hpp>

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

namespace
{
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
namespace PassbackPixel
{
  /**
   * PassbackPixel::Frontend implementation
   */
  Frontend::Frontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers,
    CommonModule* common_module)
    /*throw(eh::Exception)*/
    : Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().PassPixelFeConfiguration()->Logger().log_level())),
        0,
        Aspect::PASS_PIXEL_FRONTEND,
        0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      workers_(std::move(request_workers))
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

  FrontendCommons::RequestTask
  Frontend::handle_request_coro(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer)
    noexcept
  {
    (void)response_writer;
    co_await AdServer::Commons::ExecutorPool::yield(workers_);

    const FCGI::HttpRequest& request = request_holder->request();

    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());
    FCGI::HttpResponse& response = *response_ptr;
    int http_status = handle_request_(request, response);
    co_return FrontendCommons::RequestResult{
      http_status,
      response_ptr,
      false};
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
    const FCGI::HttpRequest& request,
    FCGI::HttpResponse& response)
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

      response.set_content_type_nocopy(String::SubString("image/gif"));

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

    auto info = std::make_shared<
      adserver::campaign_svcs::campaign_manager::
        ConsiderPassbackTrackRequest>();
    info->set_time(pack_time(passback_track_info.time));
    info->set_country(passback_track_info.country);
    info->set_colo_id(passback_track_info.colo_id);
    info->set_tag_id(passback_track_info.tag_id);
    info->set_user_status(passback_track_info.user_status);
    campaign_manager_->consider_passback_track(
      *info,
      [this, info](
        const grpc::Status& status,
        const adserver::campaign_svcs::campaign_manager::
          ConsiderPassbackTrackResponse&)
      {
        if(!status.ok())
        {
          Stream::Error ostr;
          ostr << FUN << ": CampaignManager::consider_passback_track(): "
            "gRPC call failed: code=" <<
            static_cast<int>(status.error_code()) <<
            ", message=" << status.error_message();
          logger()->log(
            ostr.str(),
            Logging::Logger::ERROR,
            Aspect::PASS_PIXEL_FRONTEND,
            "ADS-IMPL-194");
        }
      });

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
        "Frontend::init(): frontend is running ..."),
      Logging::Logger::INFO, Aspect::PASS_PIXEL_FRONTEND);
    }
  }

  void
  Frontend::shutdown() noexcept
  {
    deactivate_object();
    wait_object();
    campaign_manager_.reset();

    logger()->log(String::SubString(
        "Frontend::shutdown(): frontend terminated"),
      Logging::Logger::INFO, Aspect::PASS_PIXEL_FRONTEND);
  }
} /*PassbackPixel*/
} /*AdServer*/
