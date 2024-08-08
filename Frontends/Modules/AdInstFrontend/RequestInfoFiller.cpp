#include <optional>

#include <String/AsciiStringManip.hpp>
#include <String/UTF8Case.hpp>
#include <String/StringManip.hpp>
#include <Generics/Rand.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/UserInfoManip.hpp>

#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/Cookies.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace Instantiate
{
  typedef const String::AsciiStringManip::Char2Category<',', ' '>
    ListParameterSepCategory;

  typedef const String::AsciiStringManip::Char1Category<'-'>
    CreativeIdSepCategory;

  namespace Aspect
  {
    const char AD_INST_FRONTEND[] = "Instantiate::Frontend";
  }

  namespace Request
  {
    namespace Cookie
    {
      const String::SubString OPTIN("OPTED_IN");
      const String::SubString COHORT("ct");
    }

    namespace Header
    {
      const String::SubString REM_HOST(".remotehost");
      const String::SubString USER_AGENT("user-agent");
      const String::SubString FCGI_USER_AGENT("user_agent");
      const String::AsciiStringManip::Caseless REFERER("referer");
    }

    namespace Context
    {
      const String::SubString GLOBAL_REQUEST_ID("rid");
      const String::SubString TEMPORARY_CLIENT_ID("tuid");
      const String::SubString REMOVE_MERGED_CLIENT_ID("rm-muid");

      const String::SubString TAG_ID("tid");
      const String::SubString TAG_SIZE_ID("tsid");
      const String::SubString RANDOM("r");
      const String::SubString ORIGINAL_URL("orig");
      const String::SubString PASSBACK_URL("pb");
      const String::SubString PASSBACK_TYPE("pt");
      const String::SubString COLOCATION_ID("colo");
      const String::SubString FORMAT("format");
      const String::SubString TEST("test");
      const String::SubString BID_TIME("bt");

      const String::SubString PRECLICK_URL("preclick");
      const String::SubString CLICKPREFIX_URL("clickpref");
      const String::SubString PUBLISHER_IMPRESSION_TRACKING_URL("imptrck");
      const String::SubString SOURCE_ID("src");
      const String::SubString EXTERNAL_USER_ID("eid");
      const String::SubString PUBLISHER_ACCOUNT_ID("aid");
      const String::SubString PUBLISHER_SITE_ID("sid");
      const String::SubString IMPRESSION_REQUEST_IDS("irid");
      const String::SubString CREATIVE_LIST("ad");
      const String::SubString CREATIVE_ID("crid");
      const String::SubString USER_ID_DISTRIBUTION_HASH("h");
      const String::SubString USER_ID("u");
      const String::SubString PUBLISHER_PIXEL_ACCOUNT_IDS("paid");
      const String::SubString DATA("d");

      const String::SubString LOCATION_NAME("loc.name");
      const String::SubString LOCATION_COORD("loc.coord");
      const String::SubString SECURITY_TOKEN("tok");
      const String::SubString ENCRYPTED_USER_IP("euip");
      const String::SubString EXT_TAG_ID("etid");

      const String::SubString TANX_SETTLE_PRICE("txp");
      const String::SubString OPEN_SETTLE_PRICE("p");
      const String::SubString OPENX_SETTLE_PRICE("oxp");
      const String::SubString LIVERAIL_SETTLE_PRICE("lrp");
      const String::SubString BAIDU_SETTLE_PRICE("bdp");
      const String::SubString GOOGLE_SETTLE_PRICE("gp");
      const String::SubString FLAGS("f");
      const String::SubString BID_PRICE("bp");
      const String::SubString CLIENT_VERSION("v");
      const String::SubString APPLICATION("app");

      const String::SubString CAMPAIGN_MANAGER_INDEX("cmi");
      const String::SubString PUB_POSITION_BOTTOM("hpos");
      const String::SubString VIDEO_WIDTH("vw");
      const String::SubString VIDEO_HEIGHT("vh");
      const String::SubString EXT_TRACK_PARAMS("ep");
      const String::SubString TOKEN_PREFIX("t.");
      const String::SubString SET_COOKIE("sc");

      /* debug params */
      const String::SubString IP_ADDRESS("debug.ip");
      const String::AsciiStringManip::Caseless SECURE("secure");
    }
  }

  class TestRequestParamProcessor: public RequestInfoParamProcessor
  {
  public:
    virtual void process(
      RequestInfo& request_info,
      const String::SubString& value) const
    {
      request_info.log_as_test = (value[0] != '0');
      request_info.test_request = (value[0] == '1');
    }
  };

  class CreativeListParamProcessor: public RequestInfoParamProcessor
  {
  public:
    virtual void process(
      RequestInfo& request_info,
      const String::SubString& value) const
    {
      typedef String::StringManip::Splitter<
        String::AsciiStringManip::SepColon, true> SplitColonEmpty;

      String::StringManip::Splitter<ListParameterSepCategory> tokenizer(value);
      String::SubString cur;
      while (tokenizer.get_token(cur))
      {
        RequestInfo::CreativeInfo creative_info;

        SplitColonEmpty sub_tokenizer(cur);
        String::SubString ccid_str;
        String::SubString ccg_keyword_id_str;
        String::SubString ctr_str;
        sub_tokenizer.get_token(ccid_str);
        sub_tokenizer.get_token(ccg_keyword_id_str);
        sub_tokenizer.get_token(ctr_str);

        if(!ccid_str.empty() &&
           !String::StringManip::str_to_int(ccid_str, creative_info.ccid))
        {
          throw RequestInfoFiller::InvalidParamException("");
        }

        if(!ccg_keyword_id_str.empty() &&
           !String::StringManip::str_to_int(
             ccg_keyword_id_str, creative_info.ccg_keyword_id))
        {
          throw RequestInfoFiller::InvalidParamException("");
        }

        if(!ctr_str.empty())
        {
          try
          {
            creative_info.ctr = CampaignSvcs::RevenueDecimal(ctr_str);
          }
          catch(...)
          {}
        }
        else
        {
          // for external inst requests use max ctr (
          //   these requests will be ignored at ctr optimization)
          creative_info.ctr = CampaignSvcs::RevenueDecimal(false, 1, 0);
        }

        request_info.creatives.push_back(creative_info);
      }
    }      
  };

  class CreativeIdParamProcessor: public RequestInfoParamProcessor
  {
  public:
    virtual void process(
      RequestInfo& request_info,
      const String::SubString& value) const
    {
      String::StringManip::Splitter<CreativeIdSepCategory> tokenizer(value);
      String::SubString cur;
      while (tokenizer.get_token(cur))
      {
        unsigned long creative_id;
        if(!String::StringManip::str_to_int(
             cur, creative_id))
        {
          throw RequestInfoFiller::InvalidParamException("");
        }

        request_info.creative_id = creative_id;
      }
    }      
  };

  class FlagsProcessor: public RequestInfoParamProcessor
  {
  public:
    virtual void process(
      RequestInfo& request_info,
      const String::SubString& value) const
    {
      if(value.find('R') != String::SubString::NPOS ||
        value.find('r') != String::SubString::NPOS)
      {
        request_info.consider_request = true;
      }

      if(value.find('N') != String::SubString::NPOS ||
        value.find('n') != String::SubString::NPOS)
      {
        request_info.enabled_notice = true;
      }

      if(value.find('U') != String::SubString::NPOS ||
        value.find('u') != String::SubString::NPOS)
      {
        request_info.set_uid = true;
      }
      
      if(value.find('C') != String::SubString::NPOS ||
        value.find('c') != String::SubString::NPOS)
      {
        request_info.emulate_click = true;
      }
    }
  };

  class BidPriceProcessor: public RequestInfoParamProcessor
  {
  public:
    virtual void process(
      RequestInfo& request_info,
      const String::SubString& value) const
    {
      try
      {
        request_info.pub_imp_revenue =
          Commons::extract_decimal<AdServer::CampaignSvcs::RevenueDecimal>(
            value, Generics::DMR_CEIL, true);
      }
      catch(const eh::Exception&)
      {}
    }
  };

  // RequestInfoFiller
  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    CommonModule* common_module,
    const char* geo_ip_path,
    const String::SubString& ip_key)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      common_module_(ReferenceCounting::add_ref(common_module))
  {
    static const char* FUN = "RequestInfoFiller::RequestInfoFiller()";

    if(geo_ip_path)
    {
      try
      {
        ip_map_ = IPMapPtr(new GeoIPMapping::IPMapCity2(geo_ip_path));
      }
      catch (const GeoIPMapping::IPMap::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": GeoIPMapping::IPMap::Exception caught: " << e.what();

        logger->log(ostr.str(),
          Logging::Logger::CRITICAL,
          Aspect::AD_INST_FRONTEND,
          "ADS-IMPL-102");
      }
    }

    if(!ip_key.empty())
    {
      try
      {
        ip_crypter_ = new Commons::IPCrypter(ip_key);
      }
      catch (const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't init ip crypter: " << e.what();

        logger->log(ostr.str(),
          Logging::Logger::CRITICAL,
          Aspect::AD_INST_FRONTEND,
          "ADS-IMPL-102");
      }
    }

    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::CLIENT_ID,
      RequestInfoParamProcessor_var(
        new FrontendCommons::SignedUuidParamProcessor<RequestInfo>(
          &RequestInfo::user_id,
          &RequestInfo::signed_user_id,
          common_module->user_id_controller(),
          UserIdController::PERSISTENT))));
    cookie_processors_.insert(std::make_pair(
      Request::Context::TEMPORARY_CLIENT_ID,
      RequestInfoParamProcessor_var(
        new FrontendCommons::SignedUuidParamProcessor<RequestInfo>(
          &RequestInfo::temp_client_id,
          common_module->user_id_controller(),
          UserIdController::TEMPORARY))));

    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::OPTOUT,
      RequestInfoParamProcessor_var(
        new FrontendCommons::OptOutParamProcessor<RequestInfo>(
          &RequestInfo::user_status))));
    cookie_processors_.insert(std::make_pair(
      Request::Cookie::COHORT,
      RequestInfoParamProcessor_var(
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::cohort, 50))));

    add_processor_(true, true, Request::Context::GLOBAL_REQUEST_ID,
      new FrontendCommons::RequestIdParamProcessor<RequestInfo>(
        &RequestInfo::global_request_id));
    add_processor_(false, true, Request::Context::CREATIVE_LIST,
      new CreativeListParamProcessor());
    add_processor_(false, true, Request::Context::CREATIVE_ID,
      new CreativeIdParamProcessor());
    add_processor_(true, true, Request::Context::TEMPORARY_CLIENT_ID,
      new FrontendCommons::SignedUuidParamProcessor<RequestInfo>(
        &RequestInfo::temp_client_id,
        common_module->user_id_controller(),
        UserIdController::TEMPORARY));
    add_processor_(false, true, Request::Context::BID_TIME,
      new FrontendCommons::UnixTimeParamProcessor<RequestInfo>(
        &RequestInfo::bid_time));

    add_processor_(true, false, Request::Header::REM_HOST,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::peer_ip));
    add_processor_(false, true, Request::Header::REFERER.str,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::referer));

    add_processor_(true, true, Request::Context::FORMAT,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::format));
    add_processor_(true, true, Request::Context::PASSBACK_URL,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::passback_url));
    add_processor_(true, true, Request::Context::PASSBACK_TYPE,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::passback_type));
    add_processor_(true, true, Request::Context::ORIGINAL_URL,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::original_url));
    add_processor_(true, true, Request::Context::TAG_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::tag_id));
    add_processor_(true, true, Request::Context::EXT_TAG_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::ext_tag_id, 50, false, true));
    add_processor_(true, true, Request::Context::PUBLISHER_ACCOUNT_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::publisher_account_id));
    add_processor_(true, true, Request::Context::TAG_SIZE_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::tag_size_id));
    add_processor_(true, true, Request::Context::COLOCATION_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::colo_id));
    add_processor_(true, true, Request::Context::SOURCE_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::source_id));
    add_processor_(true, true, Request::Context::EXTERNAL_USER_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::external_user_id));
    add_processor_(true, true, Request::Context::PUBLISHER_SITE_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::publisher_site_id));
    add_processor_(true, true, Request::Context::LOCATION_NAME,
      new FrontendCommons::LocationNameParamProcessor<RequestInfo>(
        &RequestInfo::location));
    add_processor_(true, true, Request::Context::LOCATION_COORD,
      new FrontendCommons::LocationCoordParamProcessor<RequestInfo>(
        &RequestInfo::coord_location));
    add_processor_(true, true, Request::Context::USER_ID_DISTRIBUTION_HASH,
      new FrontendCommons::NumberParamProcessor<
        RequestInfo, std::optional<unsigned long>, unsigned long>(
          &RequestInfo::user_id_hash_mod));
    add_processor_(true, true, Request::Context::USER_ID,
      new FrontendCommons::UuidParamProcessor<RequestInfo>(&RequestInfo::track_user_id));
    add_processor_(false, true, Request::Context::PUBLISHER_PIXEL_ACCOUNT_IDS,
      new FrontendCommons::NumberContainerParamProcessor<
        RequestInfo,
        RequestInfo::AccountIdList,
        String::AsciiStringManip::SepComma>(&RequestInfo::pubpixel_accounts));
    add_processor_(true, true, Request::Context::REMOVE_MERGED_CLIENT_ID,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::remove_merged_uid));
    add_processor_(true, true, Request::Context::RANDOM,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::random));
    add_processor_(true, true, Request::Context::TEST,
      new TestRequestParamProcessor());
    add_processor_(true, true, Request::Context::SECURITY_TOKEN,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::security_token));
    add_processor_(true, true, Request::Context::ENCRYPTED_USER_IP,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::encrypted_user_ip));
    add_processor_(false, true, Request::Context::PRECLICK_URL,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::preclick_url));
    add_processor_(false, true, Request::Context::CLICKPREFIX_URL,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::click_prefix_url));
    add_processor_(false, true,
      Request::Context::PUBLISHER_IMPRESSION_TRACKING_URL,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::pub_impr_track_url));
    add_processor_(false, true, Request::Context::IMPRESSION_REQUEST_IDS,
      new FrontendCommons::RequestIdContainerParamProcessor<
        RequestInfo,
        RequestInfo::RequestIdList,
        ListParameterSepCategory>(&RequestInfo::request_ids));
    add_processor_(true, true, Request::Context::TANX_SETTLE_PRICE,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::tanx_price));
    add_processor_(true, true, Request::Context::OPEN_SETTLE_PRICE,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::open_price));
    add_processor_(true, true, Request::Context::OPENX_SETTLE_PRICE,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::openx_price));
    add_processor_(true, true, Request::Context::LIVERAIL_SETTLE_PRICE,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::liverail_price));
    add_processor_(true, true, Request::Context::BAIDU_SETTLE_PRICE,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::baidu_price));
    add_processor_(true, true, Request::Context::GOOGLE_SETTLE_PRICE,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::google_price));
    add_processor_(true, true, Request::Context::SECURE.str,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::secure));
    add_processor_(true, true, Request::Context::FLAGS,
      new FlagsProcessor());
    add_processor_(true, true, Request::Context::BID_PRICE,
      new BidPriceProcessor());
    add_processor_(true, true, Request::Context::CAMPAIGN_MANAGER_INDEX,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::campaign_manager_index));
    add_processor_(true, true, Request::Context::PUB_POSITION_BOTTOM,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::hpos));
    add_processor_(true, true, Request::Context::VIDEO_WIDTH,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::video_width));
    add_processor_(true, true, Request::Context::VIDEO_HEIGHT,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::video_height));
    add_processor_(true, true, Request::Context::EXT_TRACK_PARAMS,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::ext_track_params));
    add_processor_(true, true, Request::Context::CLIENT_VERSION,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::client_app_version));
    add_processor_(true, true, Request::Context::SET_COOKIE,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::set_cookie));
    add_processor_(true, true, Request::Context::APPLICATION,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::client_app));
    add_processor_(true, false, Request::Header::USER_AGENT,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::user_agent));
    add_processor_(true, false, Request::Header::FCGI_USER_AGENT,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::user_agent));

    add_processor_(false, true, Request::Context::DATA,
      new FrontendCommons::DataParamProcessor<RequestInfo>(param_processors_));

    // debug parameters
    add_processor_(false, true, Request::Context::IP_ADDRESS,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::peer_ip));
  }

  void
  RequestInfoFiller::add_processor_(
    bool headers,
    bool parameters,
    const String::SubString& name,
    RequestInfoParamProcessor* processor)
    noexcept
  {
    RequestInfoParamProcessor_var processor_ptr(processor);

    if(headers)
    {
      header_processors_.insert(
        std::make_pair(name, processor_ptr));
    }

    if(parameters)
    {
      param_processors_.insert(
        std::make_pair(name, processor_ptr));
    }
  }

  void
  RequestInfoFiller::fill(
    RequestInfo& request_info,
    const FrontendCommons::HttpRequest& request) const
    /*throw(InvalidParamException, ForbiddenException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill()";

    request_info.time = Generics::Time::get_time_of_day();
    request_info.bid_time = request_info.time;

    const HTTP::SubHeaderList& headers = request.headers();
    const HTTP::ParamList& params = request.params();

    try
    {
      for (HTTP::SubHeaderList::const_iterator it = headers.begin();
        it != headers.end(); ++it)
      {
        std::string header_name = it->name.str();
        String::AsciiStringManip::to_lower(header_name);

        ParamProcessorMap::const_iterator param_it =
          header_processors_.find(header_name);

        if(param_it != header_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      }

      for(HTTP::ParamList::const_iterator it = params.begin();
          it != params.end(); ++it)
      {
        ParamProcessorMap::const_iterator param_it =
          param_processors_.find(it->name);

        if(param_it != param_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
        else if(it->name.size() >= Request::Context::TOKEN_PREFIX.size() &&
          it->name.compare(
            0,
            Request::Context::TOKEN_PREFIX.size(),
            Request::Context::TOKEN_PREFIX.data()) == 0)
        {
          request_info.tokens.insert(
            std::make_pair(
              it->name.substr(Request::Context::TOKEN_PREFIX.size()),
              it->value));
        }
      }

      HTTP::CookieList cookies;

      try
      {
        cookies.load_from_headers(headers);
      }
      catch(HTTP::CookieList::InvalidArgument&)
      {
        throw InvalidParamException("");
      }

      for(HTTP::CookieList::const_iterator it = cookies.begin();
          it != cookies.end(); ++it)
      {
        ParamProcessorMap::const_iterator param_it =
          cookie_processors_.find(it->name);

        if(param_it != cookie_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      }

      if(request_info.global_request_id.is_null())
      {
        if(!request_info.request_ids.empty())
        {
          request_info.global_request_id = *request_info.request_ids.begin();
        }
      }

      if(request_info.random >= CampaignSvcs::RANDOM_PARAM_MAX)
      {
        request_info.random = Generics::safe_rand(
          CampaignSvcs::RANDOM_PARAM_MAX);
      }

      if(request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT)
      {
        if(request_info.user_id == AdServer::Commons::PROBE_USER_ID)
        {
          request_info.user_status = AdServer::CampaignSvcs::US_PROBE;
          request_info.user_id = AdServer::Commons::UserId();
        }
        else if (!request_info.user_id.is_null())
        {
          request_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
        }
      }
      else
      {
        request_info.user_id = AdServer::Commons::UserId();
      }

      if(ip_crypter_ && !request_info.encrypted_user_ip.empty())
      {
        try
        {
          ip_crypter_->decrypt(
            request_info.peer_ip, request_info.encrypted_user_ip);

          // don't use external SECTOK if euip defined and valid
          request_info.security_token.clear();
        }
        catch(const eh::Exception&)
        {}
      }

      if (!request_info.location.in() &&
          !request_info.peer_ip.empty() &&
          ip_map_.get())
      {
        try
        {
          GeoIPMapping::IPMapCity2::CityLocation geo_location;

          if(ip_map_->city_location_by_addr(
               request_info.peer_ip.c_str(),
               geo_location,
               false))
          {
            request_info.location = new FrontendCommons::Location();
            request_info.location->country = geo_location.country_code.str();
            geo_location.region.assign_to(request_info.location->region);
            request_info.location->city = geo_location.city.str();
            request_info.location->normalize();
          }
        }
        catch(const eh::Exception&)
        {}
      }

      if(request_info.creatives.empty() && !request_info.creative_id)
      {
        Stream::Error ostr;
        ostr << FUN << ": 'ad' or 'crid' isn't defined";
        throw InvalidParamException(ostr.str());
      }

      if(request_info.tag_id == 0 && request_info.publisher_account_id == 0)
      {
        Stream::Error ostr;
        ostr << FUN << ": 'tid' or 'aid' isn't defined";
        throw InvalidParamException(ostr.str());
      }

      // fill parameters required for consider request
      if(request_info.consider_request)
      {
        request_info.full_referer_hash =
          FrontendCommons::referer_hash(request_info.referer);

        request_info.short_referer_hash =
          FrontendCommons::short_referer_hash(request_info.referer);

        if(!request_info.user_agent.empty())
        {
          FrontendCommons::WebBrowserMatcher_var web_browser_matcher =
            common_module_->web_browser_matcher();

          FrontendCommons::PlatformMatcher_var platform_matcher =
            common_module_->platform_matcher();

          if(web_browser_matcher.in())
          {
            try
            {
              web_browser_matcher->match(
                request_info.web_browser,
                request_info.user_agent);
            }
            catch(const eh::Exception& ex)
            {
              Stream::Error ostr;
              ostr << FUN << ": Web browser matching exception: " << ex.what();
              logger()->log(
                ostr.str(),
                Logging::Logger::EMERGENCY,
                Aspect::AD_INST_FRONTEND,
                "ADS-IMPL-109");
            }
          }

          if(platform_matcher.in())
          {
            try
            {
              platform_matcher->match(
                &request_info.platform_ids,
                request_info.platform,
                request_info.full_platform,
                request_info.user_agent);
            }
            catch(const eh::Exception& ex)
            {
              Stream::Error ostr;
              ostr << FUN << ": Web browser matching exception: " << ex.what();
              logger()->log(
                ostr.str(),
                Logging::Logger::EMERGENCY,
                Aspect::AD_INST_FRONTEND,
                "ADS-IMPL-109");
            }
          }
        }
      }
    }
    catch(const InvalidParamException&)
    {
      throw;
    }
    catch(const ForbiddenException&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": "
        "Can't fill request info. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }

    // do specific parameters adaptation
    if(!request_info.creatives.empty())
    {
      // ad override crid
      request_info.creative_id = 0;
    }
  }
} // Instantiate
} // AdServer
