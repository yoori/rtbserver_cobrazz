#include <Generics/Time.hpp>
#include <array>
#include <utility>
#include <Logger/StreamLogger.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>
#include <String/StringManip.hpp>
#include <PrivacyFilter/Filter.hpp>
#include <Commons/ErrorHandler.hpp>

#include "WebStatFrontend.hpp"

namespace AdServer
{
namespace WebStat
{
  namespace
  {
    struct FrontendConstrainTraits
    {
      static const unsigned long MAX_NUMBER_PARAMS = 50;
      static const unsigned long MAX_LENGTH_PARAM_NAME = 30;
      static const unsigned long MAX_LENGTH_PARAM_VALUE = 500;
    };

    typedef FrontendCommons::DefaultConstrain<
      FrontendCommons::OnlyGetAndPostAllowed,
      FrontendCommons::ParamConstrainDefault,
      FrontendConstrainTraits>
      FrontendHTTPConstrain;

    namespace Config
    {
      const char CONFIG_FILE[] = "WebStatFrontend_Config";
      const char ENABLE[] = "WebStatFrontend_Enable";
    }

    namespace Aspect
    {
      const char WEBSTAT_FRONTEND[] = "WebStatFrontend";
    }

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

  /**
   * Frontend implementation
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
            frontend_config->get().WebStatFeConfiguration()->Logger().log_level())),
        "WebStat::Frontend",
        Aspect::WEBSTAT_FRONTEND,
        0),
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        frontend_config->get().WebStatFeConfiguration()->threads(),
        0), // max pending tasks
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module))
  {}

  void
  Frontend::parse_config_() /*throw(Exception)*/
  {
    ::Config::ErrorHandler error_handler;

    const auto& fe_config = frontend_config_->get();

    if(!fe_config.CommonFeConfiguration().present())
    {
      throw Exception("CommonFeConfiguration isn't present");
    }

    common_config_ = CommonConfigPtr(
      new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

    if(!fe_config.WebStatFeConfiguration().present())
    {
      throw Exception("WebStatFeConfiguration isn't present");
    }

    config_ = ConfigPtr(
      new WebStatFeConfiguration(*fe_config.WebStatFeConfiguration()));
  }

  bool
  Frontend::will_handle(const String::SubString& uri) noexcept
  {
    std::string found_uri;
    bool result =
      FrontendCommons::find_uri(config_->UriList().Uri(), uri, found_uri) ||
      FrontendCommons::find_uri(config_->YandexNotificationUriList().Uri(), uri, found_uri)
      ;

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << "WebStat::Frontend::will_handle(" << uri << "), result " << result;

      logger()->log(ostr.str(),
        Logging::Logger::TRACE,
        Aspect::WEBSTAT_FRONTEND);
    }

    return result;
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
    FCGI::HttpResponse& response)
    noexcept
  {
    static const char* FUN = "WebStat::Frontend::handle_request_()";

    int http_result = 0;

    try
    {
      //FrontendHTTPConstrain::apply(request);

      std::string found_uri;
      std::vector<RequestInfo> request_info_list;

      if(FrontendCommons::find_uri(
        config_->UriList().Uri(), request.uri(), found_uri))
      {
        RequestInfo request_info;
        request_info_filler_->fill(request_info, request.headers(), request.params());
        request_info_list.emplace_back(std::move(request_info));
      }
      else if(FrontendCommons::find_uri(
        config_->YandexNotificationUriList().Uri(), request.uri(), found_uri))
      {
        Stream::BinaryStreamReader request_reader(
          &request.get_input_stream());

        std::string bid_request;
        char buf[1024];

        while(!request_reader.eof() && !request_reader.bad())
        {
          request_reader.read(buf, sizeof(buf));
          bid_request.append(buf, request_reader.gcount());
        }

        request_info_filler_->fill_by_yandex_notification(
          request_info_list,
          request.headers(),
          request.params(),
          bid_request.c_str());
      }

      for(auto req_it = request_info_list.begin(); req_it != request_info_list.end(); ++req_it)
      {
        const RequestInfo& request_info = *req_it;

        adserver::campaign_svcs::campaign_manager::ConsiderWebOperationRequest
          web_op_info;
        //web_op_info.check_args = true;
        web_op_info.set_time(pack_time(request_info.time));
        web_op_info.set_colo_id(request_info.colo_id);
        web_op_info.set_tag_id(request_info.tag_id);
        web_op_info.set_cc_id(request_info.cc_id);
        web_op_info.set_ct(request_info.ct);
        web_op_info.set_curct(request_info.curct);
        web_op_info.set_browser(request_info.browser);
        web_op_info.set_os(request_info.os);
        web_op_info.set_app(request_info.application);
        web_op_info.set_source(request_info.source);
        web_op_info.set_operation(request_info.operation);
        web_op_info.set_result(request_info.result);
        web_op_info.set_user_status(request_info.user_status);
        web_op_info.set_test_request(request_info.test_request);
        web_op_info.set_referer(request_info.referer);
        web_op_info.set_ip_address(request_info.peer_ip);
        web_op_info.set_external_user_id(request_info.external_user_id);
        web_op_info.set_user_agent(request_info.user_agent);
        if (!request_info.request_ids.empty())
        {
          for(RequestIdSet::const_iterator rit = request_info.request_ids.begin();
              rit != request_info.request_ids.end(); ++rit)
          {
            web_op_info.add_request_ids(
              rit->is_null() ? std::string() : pack_request_id(*rit));
          }
        }
        if (!request_info.global_request_id.is_null())
        {
          web_op_info.set_global_request_id(
            pack_request_id(request_info.global_request_id));
        }

        AdServer::Grpc::sync_call<
          adserver::campaign_svcs::campaign_manager::ConsiderWebOperationResponse>(
            [this, &web_op_info](auto callback)
            {
              campaign_manager_->consider_web_operation(
                web_op_info,
                std::move(callback));
            },
            [](const grpc::Status& status)
            {
              if(status.error_code() == grpc::StatusCode::INVALID_ARGUMENT)
              {
                throw FrontendCommons::HTTPExceptions::InvalidParamException(
                  "incorrect argument");
              }

              AdServer::Grpc::throw_grpc_error(status);
            });
      }

      if(!request_info_list.empty() && !request_info_list.begin()->origin.empty())
      {
        response.add_header_nocopy_name(
          String::SubString("Access-Control-Allow-Origin"),
          request_info_list.begin()->origin);

        response.add_header_nocopy(
          String::SubString("Access-Control-Allow-Credentials"),
          String::SubString("true"));
      }

      // additional response headers
      if(common_config_->ResponseHeaders().present())
      {
        FrontendCommons::add_headers(
          *(common_config_->ResponseHeaders()),
          response);
      }

      if(request.method() == FCGI::HttpRequest::RM_GET)
      {
        response.set_content_type_nocopy(String::SubString("image/gif"));

        FileCache::BufferHolder_var buffer = pixel_->get();
        response.get_output_stream().write((*buffer)->data(), (*buffer)->size());
      }
    }
    catch (const ForbiddenException& ex)
    {
      http_result = 403;
      logger()->sstream(Logging::Logger::TRACE, Aspect::WEBSTAT_FRONTEND) <<
        FUN << ": ForbiddenException caught: " << ex.what();
    }
    catch (const InvalidParamException& ex)
    {
      http_result = 400;
    }
    catch(const eh::Exception& e)
    {
      http_result = 500;
      logger()->sstream(
        Logging::Logger::EMERGENCY,
        Aspect::WEBSTAT_FRONTEND,
        "ADS-IMPL-139") << FUN << ": eh::Exception has been caught: " << e.what();
    }

    return http_result;
  }

  void
  Frontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "WebStat::Frontend::init()";

    if(true) // module_used())
    {
      try
      {
        parse_config_();

        pixel_ = FileCachePtr(
          new FileCache(config_->pixel_path().c_str()));

        campaign_manager_ =
          std::make_shared<AdServer::CampaignSvcs::CampaignManagerCorbaClient>(
            FrontendCommons::read_campaign_manager_refs(*common_config_));

        request_info_filler_.reset(new RequestInfoFiller(
          config_->rid_public_key().c_str(),
          common_module_.in()));

        activate_object();
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": eh::Exception caught: " << ex.what();
        throw Exception(ostr);
      }

      logger()->log(String::SubString(
          "WebStat::Frontend::init(): frontend is running ..."),
        Logging::Logger::INFO,
        Aspect::WEBSTAT_FRONTEND);
    }
  }

  void
  Frontend::shutdown() noexcept
  {
    campaign_manager_.reset();

    logger()->log(String::SubString(
      "WebStat::Frontend::shutdown(): frontend terminated"),
      Logging::Logger::INFO,
      Aspect::WEBSTAT_FRONTEND);
  }
} /*WebStat*/
} /*AdServer*/
