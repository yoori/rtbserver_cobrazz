#include "RequestInfoFiller.hpp"
#include <Commons/CorbaAlgs.hpp>
#include <HTTP/HTTPCookie.hpp>

namespace
{
  namespace Aspect
  {
    const char CLICK_FRONTEND[] = "ClickFrontend";
  }
}

namespace AdServer
{
  namespace Request
  {
    namespace Param
    {
      const String::SubString COLOCATION_ID("colo");
      const String::SubString TAG_ID("tid");
      const String::SubString TAG_SIZE_ID("tsid");
      const String::SubString CCID("ccid");
      const String::SubString CREATIVE_ID("crid");
      const String::SubString DATA("d");
      const String::SubString CCG_KEYWORD_ID("ccgkeyword");
      const String::SubString REQUEST_ID("requestid");
      const String::SubString USER_ID_DISTRIBUTION_HASH("h");
      const String::SubString RELOCATE("relocate");
      const String::SubString PRECLICK("preclick");
      const String::SubString CLICK_PREFIX("clickpref");
      const String::SubString CLIENT_ID("u");
      const String::SubString CAMPAIGN_MANAGER_INDEX("cmi");
      const String::SubString F_FLAG("f");
      const String::SubString MARKERS("m");
      const String::SubString CLICK_RATE("cr");
      const String::SubString TOKEN_PREFIX("t.");
      const String::SubString BID_TIME("bt");

      // debug params
      const String::SubString DEBUG_CURRENT_TIME("debug-time");
      const String::SubString DEBUG_IP_ADDRESS("debug-ip");
    }

    namespace Headers
    {
      const String::AsciiStringManip::Caseless REM_HOST(".remotehost");
      const String::AsciiStringManip::Caseless REM_HOST_TEST("remote_addr");
      const String::AsciiStringManip::Caseless REFERER("referer");
    }
  }

  template <typename RequestInfoType>
  class CreativeIdParamProcessor:
    public FrontendCommons::RequestParamProcessor<RequestInfoType>
  {
    typedef const String::AsciiStringManip::Char1Category<'-'>
      CreativeIdSepCategory;

  public:
    CreativeIdParamProcessor(
      unsigned long RequestInfoType::* field,
      bool RequestInfoType::* error_flag)
    :
      field_(field),
      error_flag_(error_flag)
    {}

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const
    {
      String::StringManip::Splitter<CreativeIdSepCategory> tokenizer(value);
      String::SubString cur;
      while (tokenizer.get_token(cur))
      {
        unsigned long creative_id = 0;
        if(!String::StringManip::str_to_int(
             cur, creative_id))
        {
          request_info.*error_flag_ = true;
        }

        request_info.*field_ = creative_id;
      }
    }

  private:
    unsigned long RequestInfoType::* field_;
    bool  RequestInfoType::* error_flag_;
  };

  template <typename RequestInfoType, typename NumberType>
  class NumberWithFlagParamProcessor:
    public FrontendCommons::RequestParamProcessor<RequestInfoType>
  {
  public:
    NumberWithFlagParamProcessor(
      NumberType RequestInfoType::* field_number,
      bool RequestInfoType::* field_flag)
      : field_number_(field_number),
        field_flag_(field_flag)
    {}

    virtual void
    process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    NumberType RequestInfoType::* field_number_;
    bool RequestInfoType::* field_flag_;
  };

  template <typename RequestInfoType>
  class MarkerParamProcessor:
    public FrontendCommons::RequestParamProcessor<RequestInfoType>
  {
  public:
    virtual void
    process(
      RequestInfoType& request_info,
      const String::SubString& value)
      const noexcept;
  };

  template <typename RequestInfoType>
  class TokensParamProcessor:
    public FrontendCommons::ExtRequestParamProcessor<RequestInfoType>
  {
  public:
    TokensParamProcessor()
    {}

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& name,
      const String::SubString& value) const
    {
      if(name.size() >= Request::Param::TOKEN_PREFIX.size() &&
         name.compare(
           0,
           Request::Param::TOKEN_PREFIX.size(),
           Request::Param::TOKEN_PREFIX.data()) == 0)
      {
        request_info.tokens.insert(
          std::make_pair(
            name.substr(Request::Param::TOKEN_PREFIX.size()).str(),
            value.str()));
      }
    }

  protected:
    virtual ~TokensParamProcessor() noexcept
    {}
  };

  template<typename RequestInfoType, typename NumberType>
  void
  NumberWithFlagParamProcessor<RequestInfoType, NumberType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    if (String::StringManip::str_to_int(value, request_info.*field_number_))
    {
      request_info.*field_flag_ = true;
    }
  }

  const String::AsciiStringManip::Caseless
    MIME_ENCODED_HTTP_PREFIX("http%3a%2f%2f");
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_HTTPS_PREFIX("https%3a%2f%2f");
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_SHEME_RELATIVE_PREFIX("%2f%2f");

  template <typename RequestInfoType>
  class UrlWithEncodingParamProcessor:
    public FrontendCommons::RequestParamProcessor<RequestInfoType>
  {
  public:
    static const unsigned long MAX_URL_LENGTH = 8 * 1024;

    enum MimeDecode
    {
      MD_NONE = 0,
      MD_YES = 1,
      MD_POSSIBLE = 2
    };

    UrlWithEncodingParamProcessor(
      std::string RequestInfoType::* field,
      MimeDecode md_flags = MD_NONE,
      unsigned long max_len = MAX_URL_LENGTH,
      unsigned long view_flags = HTTP::HTTPAddress::VW_FULL);

    virtual void process(
      RequestInfoType& request_info,
      const String::SubString& value) const;

  private:
    std::string RequestInfoType::* field_;
    const MimeDecode md_flags_;
    const unsigned long max_len_;
    const unsigned long view_flags_;
  };

  template<typename RequestInfoType>
  UrlWithEncodingParamProcessor<RequestInfoType>::UrlWithEncodingParamProcessor(
    std::string RequestInfoType::* field,
    MimeDecode md_flags,
    unsigned long max_len,
    unsigned long view_flags)
    : field_(field),
      md_flags_(md_flags),
      max_len_(max_len),
      view_flags_(view_flags)
  {}

  template<typename RequestInfoType>
  void
  UrlWithEncodingParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const String::SubString& value) const
  {
    if(!value.empty())
    {
      if(value.size() > max_len_)
      {
        Stream::Error ostr;
        ostr << "Value length(" << value.size() << ") exceed";
        throw FrontendCommons::HTTPExceptions::InvalidParamException(ostr);
      }

      try
      {
        std::string decoded_value;
        String::SubString addr_value = value;

        if ((md_flags_ == MD_YES) ||
            ((md_flags_ == MD_POSSIBLE) &&
              (value.substr(0, MIME_ENCODED_HTTP_PREFIX.str.size()) == MIME_ENCODED_HTTP_PREFIX ||
                value.substr(0, MIME_ENCODED_HTTPS_PREFIX.str.size()) == MIME_ENCODED_HTTPS_PREFIX)))
        {
          String::StringManip::mime_url_decode(value, decoded_value);
          addr_value = String::SubString(decoded_value);
        }

        HTTP::BrowserAddress addr(addr_value);
        addr.get_view(HTTP::HTTPAddress::VW_FULL, request_info.*field_);
      }
      catch(...)
      {}
    }
  }

  template<typename RequestInfoType>
  void
  MarkerParamProcessor<RequestInfoType>::process(
    RequestInfoType& request_info,
    const String::SubString& value)
    const noexcept
  {
    const std::string fstr("f");
    bool ff = false;
    for(auto it = value.cbegin(); it != value.cend(); ++it)
    {
      bool local_ff = *it == 'f' || *it == 'F';

      if(!local_ff && (*it == 'r' || *it == 'R'))
      {
        request_info.markers.push_back("r");
      }

      if(ff)
      {
        request_info.markers.push_back(
          *it >= '0' && *it <= '9' ? fstr + std::string(1, *it) : fstr);
      }

      ff = local_ff;
    }

    if(ff)
    {
      request_info.markers.push_back(fstr);
    }
  }

  namespace ClickFE
  {

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
    const FrontendCommons::HttpRequest& request,
    const FrontendCommons::ParsedParamsMap& parsed_params) const
    /*throw(InvalidParamException, ForbiddenException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill()";

    request_info.user_id_hash_mod_value = 0;
    request_info.user_id_hash_mod_defined = false;

    request_info.colo_id = 0;
    request_info.tag_id = 0;
    request_info.tag_size_id = 0;
    request_info.ccid = 0;
    request_info.ccg_keyword_id = 0;
    request_info.creative_id = 0;
    request_info.ctr = CampaignSvcs::RevenueDecimal::ZERO;
    request_info.creative_id_error_flag = false;
    request_info.f_flag_value = 0;

    request_info.request_time = Generics::Time::get_time_of_day();
    request_info.bid_time = request_info.request_time;

    const HTTP::SubHeaderList& headers = request.headers();

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

      for(auto it = parsed_params.begin();
        it != parsed_params.end(); ++it)
      {
        ParamProcessorMap::const_iterator param_it =
          param_processors_.find(it->first);

        if(param_it != param_processors_.end())
        {
          param_it->second->process(request_info, it->second);
        }
        else
        {
          tokens_processor_->process(request_info, it->first, it->second);
        }
      }

      {
        HTTP::CookieList cookies;

        try
        {
          cookies.load_from_headers(request.headers());

          for(HTTP::CookieList::const_iterator it = cookies.begin();
            it != cookies.end(); ++it)
          {
            const String::SubString& name = it->name;
            const String::SubString& value = it->value;

            if(name == FrontendCommons::Cookies::CLIENT_ID)
            {
              try
              {
                AdServer::Commons::UserId uid =
                  common_module_->user_id_controller()->verify(
                    value, UserIdController::PERSISTENT).uuid();
                if(!uid.is_null())
                {
                  request_info.cookie_user_id = uid;
                  request_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
                }
              }
              catch(const eh::Exception&)
              {}
            }
            else if(name == FrontendCommons::Cookies::OPTOUT &&
              value == FrontendCommons::Cookies::OPTOUT_TRUE_VALUE)
            {
              request_info.user_status = AdServer::CampaignSvcs::US_OPTOUT;
            }
            else if (name == FrontendCommons::Cookies::SET_UID)
            {
              if(value.size() == 1 && (
                value[0] == '0' || value[0] == '1'))
              {
                request_info.set_uid_param = (value[0] == '1');
              }
            }
          }
        }
        catch(HTTP::CookieList::InvalidArgument&)
        {}
      }

      if (request_info.creative_id_error_flag)
      {
        throw InvalidParamException("ClickFrontend::RequestInfoFiller: error parsing crid parameter");
      }

      // f flag with bit-value of 0x2 indicates that
      //   click template is NOT to be used during click processing.
      if(request_info.f_flag_value == 2)
      {
        request_info.use_click_template = false;
      }
      else if(request_info.f_flag_value == 3)
      {
        request_info.use_click_template = (!request_info.relocate.empty() ||
          !request_info.preclick_url.empty() ||
          !request_info.click_prefix.empty());
      }
      else
      {
        request_info.use_click_template = true;
      }

      // "crid" parameter must be ignored if defined ccid
      if (request_info.ccid != 0)
      {
        request_info.creative_id = 0;
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

    if(request_info.ctr < AdServer::CampaignSvcs::RevenueDecimal::ZERO ||
      request_info.ctr > AdServer::CampaignSvcs::REVENUE_ONE)
    {
      request_info.ctr = AdServer::CampaignSvcs::RevenueDecimal::ZERO;
    }
  }

  // RequestInfoFiller
  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    CommonModule* common_module,
    const char* geo_ip_path)
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
          Aspect::CLICK_FRONTEND,
          "ADS-IMPL-102");
      }
    }

    tokens_processor_ = new TokensParamProcessor<RequestInfo>();

    // Headers
    add_processor_(true, false, Request::Headers::REM_HOST.str,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::peer_ip));

    add_processor_(true, false, Request::Headers::REFERER.str,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::referer));

    // Parameters
    add_processor_(false, true, Request::Param::RELOCATE,
      new UrlWithEncodingParamProcessor<RequestInfo>(&RequestInfo::relocate,
        UrlWithEncodingParamProcessor<RequestInfo>::MD_POSSIBLE));

    add_processor_(false, true, Request::Param::PRECLICK,
      new UrlWithEncodingParamProcessor<RequestInfo>(&RequestInfo::preclick_url,
        UrlWithEncodingParamProcessor<RequestInfo>::MD_YES));

    add_processor_(false, true, Request::Param::CLICK_PREFIX,
      new UrlWithEncodingParamProcessor<RequestInfo>(&RequestInfo::click_prefix,
        UrlWithEncodingParamProcessor<RequestInfo>::MD_YES));

    add_processor_(false, true, Request::Param::REQUEST_ID,
      new FrontendCommons::RequestIdParamProcessor<RequestInfo>(&RequestInfo::request_id));

    add_processor_(false, true, Request::Param::COLOCATION_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(&RequestInfo::colo_id));

    add_processor_(false, true, Request::Param::TAG_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(&RequestInfo::tag_id));

    add_processor_(false, true, Request::Param::TAG_SIZE_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(&RequestInfo::tag_size_id));

    add_processor_(false, true, Request::Param::CCID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(&RequestInfo::ccid));

    add_processor_(false, true, Request::Param::CCG_KEYWORD_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(&RequestInfo::ccg_keyword_id));

    add_processor_(false, true, Request::Param::CLICK_RATE,
      new FrontendCommons::DecimalParamProcessor<
        RequestInfo, CampaignSvcs::RevenueDecimal>(&RequestInfo::ctr));

    add_processor_(false, true, Request::Param::CAMPAIGN_MANAGER_INDEX,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::campaign_manager_index));

    add_processor_(false, true, Request::Param::BID_TIME,
      new FrontendCommons::UnixTimeParamProcessor<RequestInfo>(&RequestInfo::bid_time));

    add_processor_(false, true, Request::Param::DEBUG_CURRENT_TIME,
      new FrontendCommons::TimeParamProcessor<RequestInfo>(&RequestInfo::request_time));

    add_processor_(false, true, Request::Param::CLIENT_ID,
      new FrontendCommons::UuidParamProcessor<RequestInfo>(&RequestInfo::match_user_id));

    add_processor_(false, true, Request::Param::F_FLAG,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(&RequestInfo::f_flag_value));

    add_processor_(false, true, Request::Param::USER_ID_DISTRIBUTION_HASH,
      new NumberWithFlagParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::user_id_hash_mod_value,
        &RequestInfo::user_id_hash_mod_defined));

    add_processor_(false, true, Request::Param::CREATIVE_ID,
      new CreativeIdParamProcessor<RequestInfo>(
        &RequestInfo::creative_id, &RequestInfo::creative_id_error_flag));

    add_processor_(false, true, Request::Param::DATA,
      new FrontendCommons::DataParamProcessor<RequestInfo>(param_processors_, tokens_processor_));

    add_processor_(false, true, Request::Param::MARKERS,
      new MarkerParamProcessor<RequestInfo>());
  }
  } //namespace ClickFE
}
