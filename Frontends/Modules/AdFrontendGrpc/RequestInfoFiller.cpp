// STD
#include <functional>

// UNIXCOMMONS
#include <Generics/Rand.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Language/SegmentorCommons/SegmentorInterface.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>
#include <String/UTF8Case.hpp>

 // THIS
#include <Commons/ErrorHandler.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Frontends/FrontendCommons/Cookies.hpp>
#include <Frontends/FrontendCommons/OptOutManip.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/Modules/AdFrontendGrpc/AdFrontend.hpp>
#include <Frontends/Modules/AdFrontendGrpc/RequestInfoFiller.hpp>

namespace Aspect
{
  const char AD_FRONTEND[] = "AdFrontend";
} // namespace Aspect

namespace Request
{
  namespace Cookie
  {
    const String::SubString OPT_IN_TRIAL("trialoptin");
    const String::SubString LAST_COLOCATION_ID("lc");
    const String::SubString COHORT("ct");
    const String::SubString CCID("ccid");
    const String::SubString TEST("test");
  } // namespace Cookie

  namespace Header
  {
    const String::SubString REM_HOST(".remotehost");
    const String::SubString USER_AGENT("user-agent");
    const String::SubString FCGI_USER_AGENT("user_agent");
    const String::AsciiStringManip::Caseless REFERER("referer");
  } // namespace Header

  namespace Context
  {
    const String::SubString REFERER_TRIGGERS("referer-tr");
    const String::SubString CLID_ID("clid");
    const String::SubString MERGE_CLIENT_ID("muid");
    const String::SubString TEMPORARY_CLIENT_ID("tuid");
    const String::SubString REMOVE_MERGED_CLIENT_ID("rm-muid");
    const String::SubString HOUSEHOLD_CLIENT_ID("hid");

    const String::SubString TAG_ID("tid");
    const String::SubString LOCATION_NAME("loc.name");
    const String::SubString LOCATION_COORD("loc.coord");
    const String::SubString CLIENT_VERSION("v");
    const String::SubString RANDOM("random");
    const String::SubString APPLICATION("app");
    const String::SubString ORIGINAL_USER_AGENT("oua");
    const String::SubString ORIGINAL_URL("orig");
    const String::SubString PASSBACK_URL("pb");
    const String::SubString PASSBACK_TYPE("pt");
    const String::SubString COLOCATION_ID("colo");
    const String::SubString LAST_COLOCATION_ID("last_colo");
    const String::SubString FORMAT("format");
    const String::SubString CCID("ccid");

    const String::SubString TEST_REQUEST("testrequest");
    const String::SubString TEST("test");

    const String::SubString PAGE_LOAD_ID("pl");
    const String::SubString KEYWORDS_NORMALIZED("kn");

    const String::SubString UP_EXPAND_SPACE("spup");
    const String::SubString RIGHT_EXPAND_SPACE("sprt");
    const String::SubString LEFT_EXPAND_SPACE("splt");
    const String::SubString TAG_VISIBILITY("vis");
    const String::SubString REQUEST_TOKEN("tok");
    const String::SubString PRECLICK_URL("preclick");
    const String::SubString PUBLISHER_IMPRESSION_TRACKING_URL("imptrck");
    const String::SubString EXT_TAG_ID("etid");
    const String::SubString PUBLISHER_PARAMETER("pp");

    /* debug params */
    const String::SubString REQUIRE_DEBUG_INFO("require-debug-info");
    const String::SubString NO_FRAUD("debug.nofraud");
    const String::SubString DEBUG_EXPECTED_CCG("debug.ccg");
    const String::SubString DEBUG_CURRENT_TIME("debug-time");
    const String::SubString DEBUG_SILENT_MATCH("debug.silent-match");
    const String::SubString SET_UID("setuid");
    const String::SubString PARTLY_MATCH("debug.partly-match");
    const String::SubString NO_MATCH("debug.no_match");
    const String::SubString NO_RESULT("debug.no_result");
    const String::SubString IP_ADDRESS("debug.ip");

    const String::AsciiStringManip::Caseless REFERER("referer");
    const String::AsciiStringManip::Caseless REFERER_WORDS("referer-kw");
    const String::AsciiStringManip::Caseless FULL_TEXT_WORDS("ft");
    const String::AsciiStringManip::Caseless DATA("d");
    const String::AsciiStringManip::Caseless SECURE("secure");

    const String::AsciiStringManip::CharCategory PP_CHARS(
      String::AsciiStringManip::ALPHA_NUM,
      String::AsciiStringManip::CharCategory("-_."));

    /* ADSC-10677 aliases */
    const String::SubString TAG_ID_ALIAS("t");
    const String::SubString EXT_TAG_ID_ALIAS("et");
    const String::SubString FORMAT_ALIAS("fm");
    const String::SubString APPLICATION_ALIAS("a");
    const String::SubString RANDOM_ALIAS("rnd");
    const String::AsciiStringManip::Caseless REFERER_WORDS_ALIAS("kw");
    const String::SubString COLOCATION_ID_ALIAS("c");
    const String::SubString LOCATION_NAME_ALIAS("l.n");
    const String::SubString LOCATION_COORD_ALIAS("l.c");
    const String::SubString CHANNELS("ch");
  } // namespace Context

  using ListParameterSepCategory = const String::AsciiStringManip::Char2Category<',', ' '>;
} // namespace Request

namespace AdServer::Grpc
{
  struct AdFrontendParamConstrain : FrontendCommons::HTTPExceptions
  {
    template <typename ConstrainTraits>
    static void apply(const HTTP::SubParam& param) /*throw(InvalidParamException)*/
    {
      static const char* FUN = "AdFrontendParamConstrain::apply()";

      if (param.name.size() > ConstrainTraits::MAX_LENGTH_PARAM_NAME)
      {
        Stream::Error ostr;
        ostr << FUN << ": Param name length(" << param.name.size() <<
          ") exceed";

        throw InvalidParamException(ostr);
      }

      if (param.value.size() > ConstrainTraits::MAX_LENGTH_PARAM_VALUE)
      {
        if (param.name == Request::Context::REFERER)
        {
          if (param.value.size() >
             ConstrainTraits::MAX_LENGTH_REFERER_PARAM_VALUE)
          {
            Stream::Error ostr;
            ostr << FUN << ": Param 'referer' value length(" <<
              param.value.size() << ") exceed";

            throw InvalidParamException(ostr);
          }
        }
        else if (param.name == Request::Context::REFERER_WORDS ||
          param.name == Request::Context::REFERER_WORDS_ALIAS)
        {
          if (param.value.size() >
             ConstrainTraits::MAX_LENGTH_REFERER_KW_PARAM_VALUE)
          {
            Stream::Error ostr;
            ostr << FUN << ": Param 'referer-kw' value length(" <<
              param.value.size() << ") exceed";

            throw InvalidParamException(ostr);
          }
        }
        else if (param.name != Request::Context::FULL_TEXT_WORDS &&
                 param.name != Request::Context::DATA &&
                 param.name != Request::Context::PRECLICK_URL)
        {
          Stream::Error ostr;
          ostr << FUN << ": Param value length(" << param.value.size() <<
            ") exceed";

          throw InvalidParamException(ostr);
        }
      }
    }
  };


  namespace
  {
    struct RequestFrontendConstrainTraits
    {
      static const unsigned long MAX_NUMBER_PARAMS = 50;
      static const unsigned long MAX_LENGTH_PARAM_NAME = 30;
      static const unsigned long MAX_LENGTH_PARAM_VALUE = 540;
      static const unsigned long MAX_LENGTH_REFERER_PARAM_VALUE = 4 * 1024;
      static const unsigned long MAX_LENGTH_REFERER_KW_PARAM_VALUE = 6 * 1024;
    };

    typedef FrontendCommons::DefaultConstrain<
      FrontendCommons::OnlyGetAndPostAllowed,
      AdServer::Grpc::AdFrontendParamConstrain,
      RequestFrontendConstrainTraits>
        AdFrontendHTTPConstrain;

    class DataParamProcessor:
      public FrontendCommons::RequestParamProcessor<RequestInfo>
    {//proccessor for d parameter
    public:
      static const char AMP = '&';
      static const char EQ = '=';
      DataParamProcessor(const RequestInfoFiller::ParamProcessorMap& processor) noexcept
        : proccessor_(processor)
      {
      }

      void process(
        RequestInfo& request_info,
        const String::SubString& value) const override
      {
        std::string decoded;
        try
        {
          String::StringManip::base64mod_decode(decoded, value);
        }
        catch(const String::StringManip::InvalidFormatException&)
        {
          throw RequestInfoFiller::InvalidParamException("");
        }
        std::string::const_iterator start_name = decoded.begin();
        std::string::const_iterator end_name = decoded.begin();
        bool replace_double_amp = false, found_eq = false;
        for(std::string::const_iterator it = decoded.begin();
            it <= decoded.end(); ++it)
        {
          if(it == decoded.end() || *it == AMP)
          {
            if(it + 1 < decoded.end() && *(it + 1) == AMP)
            {
              replace_double_amp = true;
              ++it;// skip next amp
            }
            else if(start_name < end_name)
            {
              std::string purify_value;
              HTTP::SubParam param(
                String::SubString(&*start_name, &*end_name),
                String::SubString(&*(end_name + 1), it - end_name - 1));
              if(replace_double_amp)
              {
                String::StringManip::replace(
                  param.value,
                  purify_value,
                  String::SubString("&&", 2),
                  String::SubString("&", 1)); 
                param.value = purify_value;
              }
              AdFrontendParamConstrain::apply<RequestFrontendConstrainTraits>(
                param);
              RequestInfoFiller::ParamProcessorMap::const_iterator param_it =
                proccessor_.find(param.name);
              if(param_it != proccessor_.end())
              {
                param_it->second->process(request_info, param.value); 
              }
              replace_double_amp = false;
              found_eq = false;
              start_name = it + 1;
              end_name = start_name;
            }
          }
          else if(!found_eq && *it == EQ)
          {
            end_name = it;
            found_eq = true;
          }
        }
      }

    private:
      const RequestInfoFiller::ParamProcessorMap& proccessor_;
    };

    class StringEqualParamProcessor: public RequestInfoParamProcessor
    {
    public:
      StringEqualParamProcessor(
        bool RequestInfo::* field,
        const String::SubString& true_value)
        : field_(field),
          true_value_(true_value)
      {
      }

      ~StringEqualParamProcessor() override = default;

      void process(
        RequestInfo& request_info,
        const String::SubString& value) const override
      {
        request_info.*field_ = (true_value_ == value);
      }

    private:
      bool RequestInfo::* field_;
      String::SubString true_value_;
    };

    class ExpandSpaceParamProcessor:
      public FrontendCommons::NumberParamProcessor<
        RequestInfo,
        std::optional<unsigned long>,
        unsigned long>
    {
    public:
      ExpandSpaceParamProcessor(
        std::optional<unsigned long> RequestInfo::* field)
        : FrontendCommons::NumberParamProcessor<
            RequestInfo,
            std::optional<unsigned long>,
            unsigned long>(field)
      {}

      void process(
        RequestInfo& request_info,
        const String::SubString& value) const override
      {
        FrontendCommons::NumberParamProcessor<
          RequestInfo,
          std::optional<unsigned long>,
          unsigned long>::process(
            request_info, value);
        request_info.down_expand_space = 0x0FFFFFFF;
      }
    };

    class TimeParamProcessor: public RequestInfoParamProcessor
    {
    public:
      TimeParamProcessor(Generics::Time RequestInfo::* field,
        const Generics::Time& min = Generics::Time::ZERO)
        : field_(field),
          min_(min)
      {
      }

      void process(
        RequestInfo& request_info,
        const String::SubString& value) const override
      {
        try
        {
          Generics::Time time(value,
            value.size() > 2 && value[2] == '-' ?
            "%d-%m-%Y:%H-%M-%S" : "%Y-%m-%d %H:%M:%S");
          if(time >= min_ && time.tv_sec <= FrontendCommons::MAX_TIME_SEC)
          {
            request_info.*field_ = time;
          }
        }
        catch(const eh::Exception& ex)
        {
        }
      }

    private:
      Generics::Time RequestInfo::* field_;
      Generics::Time min_;
    };

    class UuidParamProcessor: public RequestInfoParamProcessor
    {
    public:
      UuidParamProcessor(
        const RequestInfoFiller* filler,
        bool persistent,
        bool household,
        bool merged_persistent,
        bool rewrite_persistent)
        : filler_(filler),
          persistent_(persistent),
          household_(household),
          merged_persistent_(merged_persistent),
          rewrite_persistent_(rewrite_persistent)
      {
      }

      virtual void process(
        RequestInfo& request_info,
        const String::SubString& value) const
        /*throw(RequestInfoFiller::InvalidParamException)*/
      {
        try
        {
          filler_->adapt_client_id_(
            value,
            request_info,
            persistent_,
            household_,
            merged_persistent_,
            rewrite_persistent_);
        }
        catch(...)
        {
          if(!persistent_ || merged_persistent_)
          {
            throw;
          }
        }
      }

    private:
      const RequestInfoFiller* filler_;
      bool persistent_;
      bool household_;
      bool merged_persistent_;
      bool rewrite_persistent_;
    };

    class TestRequestParamProcessor: public RequestInfoParamProcessor
    {
    public:
      void process(
        RequestInfo& request_info,
        const String::SubString& value) const override
      {
        request_info.log_as_test = true;
        request_info.test_request = true;

        if (value.length() == 1)
        {
          if (value[0] == '0')
          {
            request_info.log_as_test = false;
            request_info.test_request = false;
          }
          else if (value[0] == '1')
          {
            request_info.log_as_test = true;
            request_info.test_request = true;
          }
          else if (value[0] == '2')
          {
            request_info.log_as_test = true;
            request_info.test_request = false;
          }
        }
      }
    };
  }

  /** RequestInfoFiller */
  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    unsigned long colo_id,
    CommonModule* common_module,
    const char* geo_ip_path,
    const char* user_agent_filter_path,
    SetUidController* set_uid_controller,
    std::set<std::string>* ip_list,
    const std::set<int>& colo_list,
    Commons::LogReferrer::Setting log_referrer_site_stas_setting)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      colo_id_(colo_id),
      colo_list_(colo_list),
      use_ip_list_(false),
      log_referrer_setting_(log_referrer_site_stas_setting),
      common_module_(ReferenceCounting::add_ref(common_module)),
      set_uid_controller_(ReferenceCounting::add_ref(set_uid_controller))
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
          Aspect::AD_FRONTEND,
          "ADS-IMPL-102");
      }
    }

    if(user_agent_filter_path[0])
    {
      user_agent_matcher_.init(user_agent_filter_path);
    }
    if(ip_list)
    {
      use_ip_list_ = true;
      ip_list_ = *ip_list;
    }

    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::CLIENT_ID,
      RequestInfoParamProcessor_var(new UuidParamProcessor(this, true, false, false, true))));
    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::CLIENT_ID2,
      RequestInfoParamProcessor_var(new UuidParamProcessor(this, true, false, false, false))));

    cookie_processors_.insert(std::make_pair(
      Request::Context::TEMPORARY_CLIENT_ID,
      RequestInfoParamProcessor_var(new UuidParamProcessor(this, false, false, false, false))));
    cookie_processors_.insert(std::make_pair(
      Request::Context::MERGE_CLIENT_ID,
      RequestInfoParamProcessor_var(new UuidParamProcessor(this, true, false, true, false))));
    cookie_processors_.insert(std::make_pair(
      Request::Context::HOUSEHOLD_CLIENT_ID,
      RequestInfoParamProcessor_var(new UuidParamProcessor(this, true, true, false, false))));
    cookie_processors_.insert(std::make_pair(
      Request::Cookie::LAST_COLOCATION_ID,
      RequestInfoParamProcessor_var(
        new FrontendCommons::NumberParamProcessor<RequestInfo, int>(
          &RequestInfo::last_colo_id))));
    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::OPTOUT,
      RequestInfoParamProcessor_var(
        new FrontendCommons::OptOutParamProcessor<RequestInfo>(
          &RequestInfo::user_status))));
    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::OPTIN,
      RequestInfoParamProcessor_var(
        new FrontendCommons::NumberParamProcessor<RequestInfo, int>(
          &RequestInfo::opt_in_cookie))));
    cookie_processors_.insert(std::make_pair(
      Request::Cookie::COHORT,
      RequestInfoParamProcessor_var(
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::curct, 50))));
    cookie_processors_.insert(std::make_pair(
      Request::Cookie::CCID,
      RequestInfoParamProcessor_var(
        new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
          &RequestInfo::ccid))));
    cookie_processors_.insert(std::make_pair(
      Request::Cookie::TEST,
      RequestInfoParamProcessor_var(
        new TestRequestParamProcessor())));

    add_processor_(true, true, FrontendCommons::Cookies::CLIENT_ID,
      new UuidParamProcessor(this, true, false, false, true));
    add_processor_(true, true, Request::Context::CLID_ID,
      new UuidParamProcessor(this, true, false, false, false));
    add_processor_(true, true, Request::Context::TEMPORARY_CLIENT_ID,
      new UuidParamProcessor(this, false, false, false, false));
    add_processor_(true, true, Request::Context::MERGE_CLIENT_ID,
      new UuidParamProcessor(this, true, false, true, false));
    add_processor_(true, true, Request::Context::HOUSEHOLD_CLIENT_ID,
      new UuidParamProcessor(this, true, true, false, false));

    add_processor_(true, true, Request::Context::ORIGINAL_USER_AGENT,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::original_user_agent));
    add_processors_(true, true,
      { Request::Context::REFERER_WORDS.str, Request::Context::REFERER_WORDS_ALIAS.str },
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::page_words));
    add_processor_(true, true, Request::Context::REFERER_TRIGGERS,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::page_words));
    add_processor_(true, true, Request::Context::FULL_TEXT_WORDS.str,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::full_text_words));
    add_processor_(false, true, Request::Context::DATA.str,
      new DataParamProcessor(param_processors_));

    add_processor_(true, false, Request::Header::REM_HOST,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::peer_ip));
    add_processor_(true, false, Request::Header::USER_AGENT,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::user_agent));
    add_processor_(true, false, Request::Header::FCGI_USER_AGENT,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::user_agent));
    add_processor_(true, true, Request::Header::REFERER.str,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::referer,
        RequestFrontendConstrainTraits::MAX_LENGTH_REFERER_PARAM_VALUE));

    add_processors_(true, true,
      { Request::Context::FORMAT, Request::Context::FORMAT_ALIAS },
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
    add_processors_(true, true,
      { Request::Context::TAG_ID, Request::Context::TAG_ID_ALIAS },
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::tag_id));
    add_processors_(true, true,
      { Request::Context::EXT_TAG_ID , Request::Context:: EXT_TAG_ID_ALIAS },
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::ext_tag_id, 50, false, true));
    add_processors_(true, true,
      { Request::Context::COLOCATION_ID, Request::Context::COLOCATION_ID_ALIAS },
      new FrontendCommons::NumberParamProcessor<RequestInfo, int>(
        &RequestInfo::colo_id));
    add_processors_(true, true,
      { Request::Context::LOCATION_NAME, Request::Context::LOCATION_NAME_ALIAS },
      new FrontendCommons::LocationNameParamProcessor<RequestInfo>(
        &RequestInfo::location));
    add_processors_(true, true,
      { Request::Context::LOCATION_COORD, Request::Context::LOCATION_COORD_ALIAS },
      new FrontendCommons::LocationCoordParamProcessor<RequestInfo>(
        &RequestInfo::coord_location));
    add_processor_(true, true, Request::Context::CLIENT_VERSION,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::client_app_version));
    add_processors_(true, true,
      { Request::Context::APPLICATION, Request::Context::APPLICATION_ALIAS },
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::client_app));
    add_processor_(true, true, Request::Context::REMOVE_MERGED_CLIENT_ID,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::remove_merged_uid));
    add_processors_(true, true,
      { Request::Context::RANDOM, Request::Context::RANDOM_ALIAS },
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::random));
    add_processor_(true, true, Request::Context::SET_UID,
      new FrontendCommons::BoolParamProcessor<RequestInfo, std::optional<bool> >(
        &RequestInfo::set_uid));
    add_processor_(true, true, Request::Context::TEST_REQUEST,
      new TestRequestParamProcessor());
    add_processor_(true, true, Request::Context::TEST,
      new TestRequestParamProcessor());
    add_processor_(true, true, Request::Context::PAGE_LOAD_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::page_load_id));
    add_processor_(true, true, Request::Context::KEYWORDS_NORMALIZED,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::keywords_normalized));
    add_processor_(true, true, Request::Context::UP_EXPAND_SPACE,
      new ExpandSpaceParamProcessor(
        &RequestInfo::up_expand_space));
    add_processor_(true, true, Request::Context::RIGHT_EXPAND_SPACE,
      new ExpandSpaceParamProcessor(
        &RequestInfo::right_expand_space));
    add_processor_(true, true, Request::Context::LEFT_EXPAND_SPACE,
      new ExpandSpaceParamProcessor(
        &RequestInfo::left_expand_space));
    add_processor_(true, true, Request::Context::TAG_VISIBILITY,
      new FrontendCommons::NumberParamProcessor<
        RequestInfo,
        std::optional<unsigned long>,
        unsigned long>(
          &RequestInfo::tag_visibility));
    add_processor_(true, true, Request::Context::REQUEST_TOKEN,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::request_token));
    add_processor_(false, true, Request::Context::PRECLICK_URL,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::preclick_url));
    add_processor_(false, true,
      Request::Context::PUBLISHER_IMPRESSION_TRACKING_URL,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::pub_impr_track_url));
    
    add_processor_(false, true,
      Request::Context::PUBLISHER_PARAMETER,
      new FrontendCommons::StringCheckParamProcessor<
         RequestInfo, String::AsciiStringManip::CharCategory>(
           &RequestInfo::pub_param, Request::Context::PP_CHARS, 50));

    add_processor_(true, false, Request::Context::SECURE.str,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::secure));
    add_processor_(false, true, Request::Context::CCID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::ccid));

    add_processor_(false, true, Request::Context::CHANNELS,
      new FrontendCommons::NumberContainerParamProcessor<
        RequestInfo,
        AdServer::CampaignSvcs::ChannelIdArray,
        Request::ListParameterSepCategory>(&RequestInfo::hit_channel_ids));

    /* debug parameters */
    add_processor_(false, true, Request::Context::DEBUG_CURRENT_TIME,
      new FrontendCommons::TimeParamProcessor<RequestInfo>(
        &RequestInfo::current_time, Generics::Time::ONE_DAY));
    add_processor_(false, true, Request::Context::DEBUG_EXPECTED_CCG,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::debug_ccg));
    add_processor_(false, true, Request::Context::DEBUG_SILENT_MATCH,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::silent_match));
    add_processor_(false, true, Request::Context::NO_MATCH,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::no_match));
    add_processor_(false, true, Request::Context::NO_RESULT,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::no_result));
    add_processor_(false, true, Request::Context::PARTLY_MATCH,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::partly_match));
    add_processor_(false, true, Request::Context::NO_FRAUD,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::disable_fraud_detection));
    add_processor_(false, true, Request::Context::LAST_COLOCATION_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, int>(
        &RequestInfo::last_colo_id));
    add_processor_(false, true, Request::Context::IP_ADDRESS,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::peer_ip));
    add_processor_(false, true, Request::Context::REQUIRE_DEBUG_INFO,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::require_debug_info));
  }

  void RequestInfoFiller::add_processor_(
    bool headers,
    bool parameters,
    const String::SubString& name,
    const RequestInfoParamProcessor_var& processor) noexcept
  {
    if(headers)
    {
      header_processors_.insert(
        std::make_pair(name, processor));
    }

    if(parameters)
    {
      param_processors_.insert(
        std::make_pair(name, processor));
    }
  }

  void RequestInfoFiller::add_processors_(
    bool headers,
    bool parameters,
    const std::initializer_list<String::SubString>& names,
    const RequestInfoParamProcessor_var& processor) noexcept
  {
    using namespace std::placeholders;
   
    auto add = std::bind(
      &RequestInfoFiller::add_processor_,
      this,
      headers,
      parameters,
      _1,
      processor); 

    std::for_each(
      names.begin(),
      names.end(),
      add);
  }

  void RequestInfoFiller::fill(RequestInfo& request_info,
    DebugInfoStatus* debug_info,
    const FrontendCommons::HttpRequest& request) const
    /*throw(InvalidParamException, ForbiddenException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill()";

    AdFrontendHTTPConstrain::apply(request);

    const HTTP::SubHeaderList& headers = request.headers();
    const HTTP::ParamList& params = request.params();

    /* fill ad request parameters */
    request_info.user_status = AdServer::CampaignSvcs::US_UNDEFINED;
    request_info.random = CampaignSvcs::RANDOM_PARAM_MAX;
    request_info.do_opt_out = false;
    request_info.do_passback = false;
    request_info.secure = false;
    request_info.remove_merged_uid = false; /* by default don't remove old user profile */

    request_info.last_colo_id = -1;
    request_info.colo_id = colo_id_;
    request_info.format = "html";
    request_info.tag_id = 0;
    request_info.client_app = "UNKNOWN";
    request_info.current_time = Generics::Time::get_time_of_day();
    request_info.test_request = false;
    request_info.log_as_test = false;
    std::string real_peer;

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
      } /* headers processing */

      if(use_ip_list_)
      {
        real_peer = request_info.peer_ip;
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
      } /* url parameters processing */

      /* check for require-debug-info value */
      DebugInfo debug_info_status = DI_NONE;
      if(!request_info.require_debug_info.empty())
      {
        String::AsciiStringManip::Caseless require_debug_info_val("");

        require_debug_info_val.str = request_info.require_debug_info;

        if (require_debug_info_val == String::SubString("yes") ||
          require_debug_info_val == String::SubString("header"))
        {
          debug_info_status = DI_HEADER;
        }
        else if (require_debug_info_val == String::SubString("header-short"))
        {
          debug_info_status = DI_HEADER_SHORT;
        }
        else if(require_debug_info_val == String::SubString("body"))
        {
          debug_info_status = DI_BODY;
        }

        if(debug_info_status != DI_NONE)
        {
          if(use_ip_list_)
          {
            if(ip_list_.find(real_peer) == ip_list_.end())
            {
              debug_info->set(DI_NONE);
              throw FrontendCommons::HTTPExceptions::ForbiddenException(
                "Debug info is not allowed for this peer");
            }
          }

          if(!colo_list_.empty())
          {
            if(colo_list_.find(request_info.colo_id) == colo_list_.end())
            {
              debug_info->set(DI_NONE);
              throw FrontendCommons::HTTPExceptions::ForbiddenException(
                "Debug info is not allowed for this colocation");
            }
          }
        }
      }
      debug_info->set(debug_info_status);

      HTTP::CookieList cookies;

      try
      {
        cookies.load_from_headers(headers);
      }
      catch(HTTP::CookieList::InvalidArgument&)
      {
        throw InvalidParamException("");
      }

      Generics::Time opt_in_time;
      bool check_trial_oo = false;

      for(HTTP::CookieList::const_iterator it = cookies.begin();
          it != cookies.end(); ++it)
      {
        ParamProcessorMap::const_iterator param_it =
          cookie_processors_.find(it->name);

        if(param_it != cookie_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
        else if(it->name == Request::Cookie::OPT_IN_TRIAL)
        {
          try
          {
            AdServer::OptInDays::load_opt_in_days(
              it->value, opt_in_time);
            check_trial_oo = true;
          }
          catch(const AdServer::OptInDays::InvalidParam& ex)
          {
            Stream::Error ostr;
            ostr << "Non correct format of optintrial '" <<
              it->value << "': " << ex.what();

            logger()->log(ostr.str(),
              Logging::Logger::NOTICE,
              Aspect::AD_FRONTEND);
          }
        }
      } /* cookies processing */

      if(request_info.tag_id)
      {
        request_info.request_id =
          AdServer::Commons::RequestId::create_random_based();
      }

      if(request_info.random >= CampaignSvcs::RANDOM_PARAM_MAX)
      {
        request_info.random = Generics::safe_rand(
          CampaignSvcs::RANDOM_PARAM_MAX);
      }

      if(!request_info.user_agent.empty() &&
         user_agent_matcher_.match(request_info.user_agent.c_str()))
      {
        Stream::Error ostr;
        ostr << FUN << ": Request from robot, User-Agent: " <<
          request_info.user_agent;
        logger()->log(ostr.str(), Logging::Logger::TRACE, Aspect::AD_FRONTEND);

        throw ForbiddenException(ostr);
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

      bool filter_request_by_country = false;

      FrontendCommons::CountryFilter_var country_filter =
        common_module_->country_filter();

      if(country_filter.in() && (
          request_info.location.in() == 0 ||
          !country_filter->enabled(request_info.location->country)))
      {
        filter_request_by_country = true;
        request_info.user_status = AdServer::CampaignSvcs::US_FOREIGN;
        request_info.client_id = AdServer::Commons::UserId();
        request_info.temp_client_id = AdServer::Commons::UserId();
        request_info.signed_client_id.clear();
      }
      else if(check_trial_oo &&
         opt_in_time <= request_info.current_time)
      {
        request_info.do_opt_out = true;
        request_info.user_status = AdServer::CampaignSvcs::US_OPTOUT;
        request_info.signed_client_id.clear();
      }
      else if(request_info.user_status != AdServer::CampaignSvcs::US_OPTOUT)
      {
        if(request_info.client_id.is_null() ||
           request_info.client_id == AdServer::Commons::PROBE_USER_ID)
        {
          if(!request_info.temp_client_id.is_null())
          {
            request_info.user_status = AdServer::CampaignSvcs::US_TEMPORARY;
            request_info.client_id = request_info.temp_client_id;
            request_info.temp_client_id = AdServer::Commons::UserId();
          }
          else if(request_info.client_id == AdServer::Commons::PROBE_USER_ID)
          {
            request_info.user_status = AdServer::CampaignSvcs::US_PROBE;
          }
        }
        else 
        {
          request_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
        }

        AdServer::SetUidController::SetUidPtr set_uid =
          set_uid_controller_->generate_if_allowed(
            request_info.user_status,
            request_info.client_id,
            request_info.set_uid && *request_info.set_uid);

        if (set_uid)
        {
          request_info.user_status = set_uid->user_status;
          request_info.client_id = set_uid->client_id.uuid();
          request_info.signed_client_id = set_uid->client_id.str();
          request_info.have_uid_cookie = true;
        }
      }

      ColoFlagsMap_var colocations = get_colocations_();

      bool hid_profile = false;

      if(filter_request_by_country)
      {
        request_info.passback_by_colocation = true;
      }
      else if(colocations.in())
      {
        ColoFlagsMap::const_iterator colo_it =
          colocations->find(request_info.colo_id);

        if(colo_it != colocations->end())
        {
          request_info.passback_by_colocation = (
            colo_it->second.flags == CampaignSvcs::CS_NONE);
          hid_profile = colo_it->second.hid_profile;
        }
      }

      if(!hid_profile ||
         request_info.user_status != AdServer::CampaignSvcs::US_OPTIN)
      {
        request_info.household_client_id = AdServer::Commons::UserId();
      }

      if(request_info.original_user_agent.empty())
      {
        request_info.original_user_agent = request_info.user_agent;
      }

      if((request_info.passback_type != "html" &&
          request_info.passback_type != "js" &&
          request_info.passback_type != "redir") ||
         request_info.passback_url.empty())
      {
        request_info.passback_type.clear();
      }

      FrontendCommons::UrlMatcher_var url_matcher = common_module_->url_matcher();
      FrontendCommons::WebBrowserMatcher_var web_browser_matcher =
        common_module_->web_browser_matcher();
      FrontendCommons::PlatformMatcher_var platform_matcher =
        common_module_->platform_matcher();
      Language::Segmentor::SegmentorInterface_var segmentor =
        common_module_->segmentor();

      if(!request_info.referer.empty())
      {
        FrontendCommons::extract_url_keywords(
          request_info.referer_url_words,
          request_info.referer,
          common_module_->segmentor());

        if(url_matcher.in())
        {
          try
          {
            unsigned long search_engine_id;
            std::string search_words;

            if(url_matcher->match(
                 search_engine_id,
                 search_words,
                 request_info.referer,
                 segmentor))
            {
              request_info.search_engine_id = search_engine_id;
              request_info.search_words.swap(search_words);
            }
          }
          catch(const eh::Exception& ex)
          {
            Stream::Error ostr;
            ostr << FUN << ": Url matching exception: " << ex.what();
            logger()->log(
              ostr.str(),
              Logging::Logger::EMERGENCY,
              Aspect::AD_FRONTEND,
              "ADS-IMPL-109");
          }
        }

        request_info.full_referer_hash =
          FrontendCommons::referer_hash(request_info.referer);
        request_info.short_referer_hash =
          FrontendCommons::short_referer_hash(request_info.referer);

        // send referer for ad requests and requests
        // with detected search engine
        if((log_referrer_setting_ != Commons::LogReferrer::LR_EMPTY && request_info.tag_id != 0) ||
           request_info.search_engine_id != 0)
        {
          try
          {
            HTTP::BrowserAddress addr(request_info.referer);

            if(log_referrer_setting_ == Commons::LogReferrer::LR_PATH)
            {
              request_info.allowable_referer = 
                FrontendCommons::normalize_abs_url(
                  addr,
                  HTTP::HTTPAddress::VW_PROTOCOL |
                  HTTP::HTTPAddress::VW_HOSTNAME |
                  HTTP::HTTPAddress::VW_NDEF_PORT |
                  HTTP::HTTPAddress::VW_PATH);
            }
            else if(log_referrer_setting_ == Commons::LogReferrer::LR_HOST ||
              request_info.search_engine_id != 0)
            {
              request_info.allowable_referer =
                FrontendCommons::normalize_abs_url(
                  addr,
                  HTTP::HTTPAddress::VW_PROTOCOL |
                  HTTP::HTTPAddress::VW_HOSTNAME |
                  HTTP::HTTPAddress::VW_NDEF_PORT);
            }
          }
          catch(const eh::Exception&)
          {
          }
        }
      }

      if(!request_info.user_agent.empty())
      {
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
              Aspect::AD_FRONTEND,
              "ADS-IMPL-109");
          }
        }

        if(!filter_request_by_country && platform_matcher.in())
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
              Aspect::AD_FRONTEND,
              "ADS-IMPL-109");
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

    if(logger()->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error ostr;
      ostr << FUN << ": Result: " << std::endl <<
        "  platform: " << request_info.full_platform << std::endl <<
        "  web_browser: " << request_info.web_browser << std::endl <<
        "  search words: " << request_info.search_words;

      logger()->log(ostr.str());
    }
  }

  void RequestInfoFiller::adapt_client_id_(
    const String::SubString& in,
    RequestInfo& request_info,
    bool persistent,
    bool household,
    bool merged_persistent,
    bool rewrite_persistent)
    const
    /*throw(InvalidParamException)*/
  {
    try
    {
      if(persistent)
      {
        if (merged_persistent)
        {
          Generics::SignedUuid uid = common_module_->user_id_controller()->verify(in);
          if (!uid.uuid().is_null())
          {
            request_info.merge_persistent_client_id = uid.uuid();
          }
        }
        else if (household)
        {
          if(in != AdServer::Commons::PROBE_USER_ID.to_string())
          {
            Generics::SignedUuid uid = common_module_->user_id_controller()->verify(in);
            if (!uid.uuid().is_null())
            {
              request_info.household_client_id = uid.uuid();
            }
          }
        }
        else if(in == AdServer::Commons::PROBE_USER_ID.to_string())
        {
          request_info.client_id = AdServer::Commons::PROBE_USER_ID;
          request_info.signed_client_id = AdServer::Commons::PROBE_USER_ID.to_string();
          request_info.have_uid_cookie = rewrite_persistent;
        }
        else if(request_info.signed_client_id.empty() || rewrite_persistent)
        {
          Generics::SignedUuid uid = common_module_->user_id_controller()->verify(in);
          if (!uid.uuid().is_null())
          {
            request_info.client_id = uid.uuid();
            request_info.signed_client_id = uid.str();
            request_info.have_uid_cookie = rewrite_persistent;
          }
        }
      }
      else
      {
        Generics::SignedUuid uid = common_module_->user_id_controller()->verify(
          in, UserIdController::TEMPORARY);
        request_info.temp_client_id = uid.uuid();
      }
    }
    catch(const eh::Exception& ex)
    {
      throw InvalidParamException(ex.what());
    }
  }

  void RequestInfoFiller::sign_client_id_(
    std::string& signed_uid,
    const AdServer::Commons::UserId& user_id) const noexcept
  {
    try
    {
      signed_uid = common_module_->user_id_controller()->sign(user_id).str();
    }
    catch(...)
    {
    }
  }
} // namespace AdServer::Grpc
