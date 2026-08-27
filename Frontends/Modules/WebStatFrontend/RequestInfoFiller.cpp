#include <String/AsciiStringManip.hpp>
#include <String/UTF8Case.hpp>
#include <Generics/Rand.hpp>
#include <HTTP/UrlAddress.hpp>

#include <Commons/ErrorHandler.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/Algs.hpp>

//#include <Frontends/ApacheCommonModule/ApacheCommonModule.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include "RequestInfoFiller.hpp"

namespace
{
  namespace Aspect
  {
    const char WEBSTAT_FRONTEND[] = "WebStatFrontend";
  }

  namespace Request
  {
    const String::AsciiStringManip::CharCategory COHORT_CHARS(
      String::AsciiStringManip::ALPHA_NUM,
      String::AsciiStringManip::CharCategory("-."));

    namespace Cookies
    {
      const String::SubString USER_ID("uid");
      const String::SubString OPTOUT("OPTED_OUT");
      const String::SubString OPTOUT_TRUE_VALUE("YES");
      const String::SubString COHORT("ct");
    }

    namespace Header
    {
      const String::SubString USER_AGENT("user-agent");
      const String::SubString FCGI_USER_AGENT("user_agent");
      const String::SubString ORIGIN("origin");
      const String::SubString REFERER("referer");
      const String::SubString REM_HOST(".remotehost");
    }

    namespace Parameters
    {
      const String::SubString COHORT("ct");
      const String::SubString APPLICATION("app");
      const String::SubString SOURCE("src");
      const String::SubString OPERATION("op");
      const String::SubString RESULT("res");
      const String::SubString TEST_REQUEST("testrequest");
      const String::SubString TAG_ID("tid");
      const String::SubString CC_ID("ccid");
      const String::SubString SIGNED_REQUEST_ID("srequestid");
      const String::SubString SIGNED_GLOBAL_REQUEST_ID("srid");
      const String::SubString REFERER("ref");
      const String::SubString REM_HOST("ip");
      const String::SubString EXTERNAL_USER_ID("xid");

      /* debug params */
      const String::SubString DEBUG_TIME("debug.time");
    }
  }

  namespace YandexNotificationRequest
  {
    const String::SubString NOTIFICATIONS("notifications");

    const String::SubString CR_ID("crid");
    const String::SubString REQUEST_ID("requestid");
    const String::SubString IMPRESSION_ID("impressionid");
    const String::SubString STATUS("status");
    const String::SubString REASONS("reasons");
    const String::SubString PAYLOAD("payload");
  }
}

namespace AdServer::WebStat
{
  namespace
  {
    class ResParamProcessor: public RequestInfoParamProcessor
    {
    public:
      ResParamProcessor(char RequestInfo::* field)
        : field_(field)
      {}

      virtual void process(RequestInfo& request_info, const String::SubString& value) const
      {
        if (value.size() == 1)
        {
          char val = String::AsciiStringManip::to_upper(*value.begin());
          if (val != 'U' && val != 'S' && val != 'F')
          {
            throw RequestInfoFiller::InvalidParamException("");
          }
          request_info.*field_ = val;
        }
        else
        {
          throw RequestInfoFiller::InvalidParamException("");
        }
      }

    private:
      char RequestInfo::* field_;
    };

    class UuidParamProcessor: public RequestInfoParamProcessor
    {
    public:
      virtual void process(RequestInfo& request_info, const String::SubString& value) const
        /*throw(RequestInfoFiller::InvalidParamException)*/
      {
        if (value == AdServer::Commons::PROBE_USER_ID.to_string())
        {
          request_info.user_status = AdServer::CampaignSvcs::US_PROBE;
        }
        else
        {
          request_info.user_status = AdServer::CampaignSvcs::US_OPTIN;
        }
      }

    protected:
      virtual ~UuidParamProcessor() noexcept {}
    };

    class SignedRequestIdParamProcessor: public RequestInfoParamProcessor
    {
    public:
      SignedRequestIdParamProcessor(
        AdServer::Commons::RequestId RequestInfo::* field,
        const Generics::SignedUuidVerifier* request_id_verifier)
      :
        field_(field),
        request_id_verifier_(request_id_verifier)
      {}

      virtual void process(RequestInfo& request_info, const String::SubString& value) const
        /*throw(RequestInfoFiller::InvalidParamException)*/
      {
        try
        {
          request_info.*field_ = request_id_verifier_->verify(value).uuid();
        }
        catch(const eh::Exception&)
        {}
      }

    protected:
      virtual ~SignedRequestIdParamProcessor() noexcept {}

    private:
      AdServer::Commons::RequestId RequestInfo::* field_;
      const Generics::SignedUuidVerifier* request_id_verifier_;
    };


    class SignedRequestIdArrayParamProcessor: public RequestInfoParamProcessor
    {
    public:
      SignedRequestIdArrayParamProcessor(
        RequestIdSet RequestInfo::* field,
        const Generics::SignedUuidVerifier* request_id_verifier)
      :
        field_(field),
        request_id_verifier_(request_id_verifier)
      {}

      virtual void process(RequestInfo& request_info, const String::SubString& value) const
        /*throw(RequestInfoFiller::InvalidParamException)*/
      {
        typedef const String::AsciiStringManip::Char2Category<',', ' '>
          ListParameterSepCategory;

        String::StringManip::Splitter<ListParameterSepCategory> tokenizer(value);
        String::SubString token;
        while (tokenizer.get_token(token))
        {
          try
          {
            Generics::SignedUuid val = request_id_verifier_->verify(token);
            (request_info.*field_).insert((request_info.*field_).end(), val.uuid());
          }
          catch(const eh::Exception&)
          {}
        }
      }

    protected:
      virtual ~SignedRequestIdArrayParamProcessor() noexcept {}

    private:
      RequestIdSet RequestInfo::* field_;
      const Generics::SignedUuidVerifier* request_id_verifier_;
    };


    class OptedOutParamProcessor: public RequestInfoParamProcessor
    {
    public:
      virtual void process(
        RequestInfo& request_info,
        const String::SubString& /*value*/) const
        /*throw(RequestInfoFiller::InvalidParamException)*/
      {
        // user id defined
        request_info.user_status = AdServer::CampaignSvcs::US_OPTOUT;
      }

    protected:
      virtual ~OptedOutParamProcessor() noexcept {}
    };
  }

  namespace
  {
    using FastJsonParser = AdServer::Commons::FastJsonParser<>;
    using ValueProcessor = FastJsonParser::ValueProcessor;

    YandexNotificationProcessingContext&
    yandex_context_(void* context) noexcept
    {
      return *static_cast<YandexNotificationProcessingContext*>(context);
    }

    YandexNotificationProcessingElementContext*
    current_yandex_element_(void* context) noexcept
    {
      auto& elements = yandex_context_(context).elements;
      return elements.empty() ? nullptr : &elements.back();
    }

    void
    assign_string_(std::string& target, std::string_view value)
    {
      target.assign(value.data(), value.size());
    }

    class YandexNotificationsProcessor final:
      public ValueProcessor
    {
    public:
      void
      array_started(std::string_view, void*) const override
      {}

      void
      object_started(std::string_view, void* context) const override
      {
        yandex_context_(context).elements.emplace_back();
      }
    };

    class YandexNotificationStringProcessor final:
      public ValueProcessor
    {
    public:
      explicit
      YandexNotificationStringProcessor(
        std::string YandexNotificationProcessingElementContext::* field)
        : field_(field)
      {}

      void
      process_string(std::string_view value, std::string_view, void* context) const override
      {
        if (auto* element = current_yandex_element_(context))
        {
          assign_string_(element->*field_, value);
        }
      }

      void
      process_string(std::string&& value, std::string_view, void* context) const override
      {
        if (auto* element = current_yandex_element_(context))
        {
          element->*field_ = std::move(value);
        }
      }

    private:
      std::string YandexNotificationProcessingElementContext::* field_;
    };

    class YandexNotificationStatusProcessor final:
      public ValueProcessor
    {
    public:
      void
      process_integer(int64_t value, std::string_view, void* context) const override
      {
        if (auto* element = current_yandex_element_(context))
        {
          element->status = static_cast<int>(value);
        }
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        if (auto* element = current_yandex_element_(context))
        {
          element->status = static_cast<int>(value);
        }
      }
    };

    class YandexNotificationReasonsProcessor final:
      public ValueProcessor
    {
    public:
      void
      array_started(std::string_view, void*) const override
      {}

      void
      process_integer(int64_t value, std::string_view, void* context) const override
      {
        if (auto* element = current_yandex_element_(context))
        {
          element->reasons.emplace_back(static_cast<int>(value));
        }
      }

      void
      process_float(double value, std::string_view, void* context) const override
      {
        if (auto* element = current_yandex_element_(context))
        {
          element->reasons.emplace_back(static_cast<int>(value));
        }
      }
    };
  }

  /** RequestInfoFiller */
  RequestInfoFiller::RequestInfoFiller(const char* public_key, CommonModule* common_module)
    /*throw(eh::Exception)*/
    : common_module_(ReferenceCounting::add_ref(common_module))
  {
    if (public_key)
    {
      request_id_verifier_.reset(new Generics::SignedUuidVerifier(public_key));
    }
    cookie_processors_.insert(std::make_pair(
      Request::Cookies::COHORT,
      RequestInfoParamProcessor_var(
        new FrontendCommons::StringCheckParamProcessor<RequestInfo,
          String::AsciiStringManip::CharCategory>(&RequestInfo::curct,
            Request::COHORT_CHARS, 50))));

    cookie_processors_.insert(std::make_pair(
      Request::Cookies::USER_ID,
      RequestInfoParamProcessor_var(new UuidParamProcessor())));

    cookie_processors_.insert(std::make_pair(
      Request::Cookies::OPTOUT,
      RequestInfoParamProcessor_var(new OptedOutParamProcessor())));

    add_processor_(false, true, Request::Parameters::APPLICATION,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::application));
    add_processor_(false, true, Request::Parameters::SOURCE,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::source));
    add_processor_(false, true, Request::Parameters::OPERATION,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::operation));
    add_processor_(false, true, Request::Parameters::COHORT,
      new FrontendCommons::StringCheckParamProcessor<RequestInfo,
        String::AsciiStringManip::CharCategory>(&RequestInfo::ct, Request::COHORT_CHARS, 50));
    add_processor_(false, true, Request::Parameters::RESULT,
      new ResParamProcessor(&RequestInfo::result));
    add_processor_(false, true, Request::Parameters::TEST_REQUEST,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(&RequestInfo::test_request));
    add_processor_(false, true, Request::Parameters::TAG_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(&RequestInfo::tag_id));
    add_processor_(false, true, Request::Parameters::CC_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(&RequestInfo::cc_id));
    add_processor_(false, true, Request::Parameters::REFERER,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(&RequestInfo::referer));
    add_processor_(false, true, Request::Parameters::REM_HOST,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::peer_ip));
    add_processor_(false, true, Request::Parameters::EXTERNAL_USER_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::external_user_id));

    if (request_id_verifier_)
    {
      add_processor_(false, true, Request::Parameters::SIGNED_REQUEST_ID,
        new SignedRequestIdArrayParamProcessor(
          &RequestInfo::request_ids, request_id_verifier_.get()));
      add_processor_(false, true, Request::Parameters::SIGNED_GLOBAL_REQUEST_ID,
        new SignedRequestIdParamProcessor(
          &RequestInfo::global_request_id, request_id_verifier_.get()));
    }

    add_processor_(true, false, Request::Header::USER_AGENT,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::user_agent));
    add_processor_(true, false, Request::Header::FCGI_USER_AGENT,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::user_agent));
    add_processor_(true, false, Request::Header::ORIGIN,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::origin));
    add_processor_(true, false, Request::Header::REFERER,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::referer));
    add_processor_(true, false, Request::Header::REM_HOST,
      new FrontendCommons::StringParamProcessor<RequestInfo>(&RequestInfo::peer_ip));

    /* debug parameters */
    add_processor_(false, true, Request::Parameters::DEBUG_TIME,
      new FrontendCommons::TimeParamProcessor<RequestInfo>(
        &RequestInfo::time, Generics::Time::ONE_DAY));

    // init yandex notification filler
    {
      FastJsonParser::ProcessorSet processors;
      const std::string base_path = std::string(YandexNotificationRequest::NOTIFICATIONS.str());
      processors.add_processor(base_path, std::make_shared<YandexNotificationsProcessor>());
      processors.add_processor(
        base_path + "." + YandexNotificationRequest::CR_ID.str(),
        std::make_shared<YandexNotificationStringProcessor>(
          &YandexNotificationProcessingElementContext::cr_id));
      processors.add_processor(
        base_path + "." + YandexNotificationRequest::REQUEST_ID.str(),
        std::make_shared<YandexNotificationStringProcessor>(
          &YandexNotificationProcessingElementContext::request_id));
      processors.add_processor(
        base_path + "." + YandexNotificationRequest::IMPRESSION_ID.str(),
        std::make_shared<YandexNotificationStringProcessor>(
          &YandexNotificationProcessingElementContext::imp_id));
      processors.add_processor(
        base_path + "." + YandexNotificationRequest::STATUS.str(),
        std::make_shared<YandexNotificationStatusProcessor>());
      processors.add_processor(
        base_path + "." + YandexNotificationRequest::REASONS.str(),
        std::make_shared<YandexNotificationReasonsProcessor>());
      processors.add_processor(
        base_path + "." + YandexNotificationRequest::PAYLOAD.str(),
        std::make_shared<YandexNotificationStringProcessor>(
          &YandexNotificationProcessingElementContext::payload));
      yn_json_parser_ = std::make_unique<FastJsonParser>(std::move(processors));
    }
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

    if (headers)
    {
      header_processors_.insert(std::make_pair(name, processor_ptr));
    }

    if (parameters)
    {
      param_processors_.insert(std::make_pair(name, processor_ptr));
    }

    yn_reasons_.insert(std::make_pair(0, "win"));
    yn_reasons_.insert(std::make_pair(1, "internal error"));
    yn_reasons_.insert(std::make_pair(3, "invalid json"));
    yn_reasons_.insert(std::make_pair(9, "invalid cost"));
    yn_reasons_.insert(std::make_pair(102, "lose"));
    yn_reasons_.insert(std::make_pair(200, "unknown creative reason"));
    yn_reasons_.insert(std::make_pair(201, "creative on moderation"));
    yn_reasons_.insert(std::make_pair(202, "creative disapproved"));
    yn_reasons_.insert(std::make_pair(205, "creative domain is invalid"));
    yn_reasons_.insert(std::make_pair(1001, "no cid in response"));
    yn_reasons_.insert(std::make_pair(1002, "creative documents required"));
  }

  void
  RequestInfoFiller::fill_by_yandex_notification(
    std::vector<RequestInfo>& request_info_list,
    const HTTP::SubHeaderList& /*headers*/,
    const HTTP::ParamList& /*params*/,
    const char* bid_request) const
  {
    static const char* FUN = "RequestInfoFiller::fill_by_yandex_notification()";

    YandexNotificationProcessingContext context;

    try
    {
      yn_json_parser_->parse(bid_request, &context);
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": parsing error: " << e.what();
      throw InvalidParamException(ostr);
    }

    for (auto it = context.elements.begin(); it != context.elements.end(); ++it)
    {
      for (auto reason_it = it->reasons.begin(); reason_it != it->reasons.end(); ++reason_it)
      {
        RequestInfo request_info;
        request_info.time = Generics::Time::get_time_of_day();
        request_info.colo_id = 0;
        request_info.tag_id = 0;
        //request_info.cc_id; already filled
        request_info.ct = it->cr_id;
        //request_info.curct = reason_to_str_(*reason_it);
        //request_info.browser;
        //request_info.os;
        request_info.application = "yandex";
        request_info.source = "notification";
        request_info.operation = reason_to_str_(*reason_it);
        request_info.result = 'U';
        request_info.user_status = AdServer::CampaignSvcs::US_UNDEFINED;
        request_info.test_request = false;
        //request_info.user_agent;
        //request_info.origin;
        //request_info.request_ids;
        //request_info.global_request_id;
        //request_info.referer;
        //request_info.peer_ip;
        request_info_list.emplace_back(std::move(request_info));
      }

      /*
      std::cerr << " { cr_id = " << it->cr_id <<
        ", request_id = " << it->request_id <<
        ", imp_id = " << it->imp_id <<
        ", status = " << it->status <<
        ", reasons = [";
      Algs::print(std::cerr, it->reasons.begin(), it->reasons.end());
      std::cerr << "]"
        ", payload = " << it->payload <<
        "}";
      std::cerr << std::endl;
      */
    }
  }

  void
  RequestInfoFiller::fill(
    RequestInfo& request_info,
    const HTTP::SubHeaderList& headers,
    const HTTP::ParamList& params) const
    /*throw(InvalidParamException, ForbiddenException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill()";

    /* fill ad request parameters */
    request_info.user_status = AdServer::CampaignSvcs::US_UNDEFINED;

    try
    {
      for (HTTP::SubHeaderList::const_iterator it = headers.begin(); it != headers.end(); ++it)
      {
        std::string name = it->name.str();
        String::AsciiStringManip::to_lower(name);

        ParamProcessorMap::const_iterator param_it = header_processors_.find(name);

        if (param_it != header_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      } /* headers processing */

      for (HTTP::ParamList::const_iterator it = params.begin(); it != params.end(); ++it)
      {
        ParamProcessorMap::const_iterator param_it = param_processors_.find(it->name);

        if (param_it != param_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      } /* url parameters processing */

      HTTP::CookieList cookies;

      try
      {
        cookies.load_from_headers(headers);
      }
      catch(HTTP::CookieList::InvalidArgument&)
      {
        throw InvalidParamException("");
      }

      for (HTTP::CookieList::const_iterator it = cookies.begin(); it != cookies.end(); ++it)
      {
        ParamProcessorMap::const_iterator param_it = cookie_processors_.find(it->name);

        if (param_it != cookie_processors_.end())
        {
          param_it->second->process(request_info, it->value);
        }
      } /* cookies processing */

      if (request_info.time == Generics::Time::ZERO)
      {
        request_info.time = Generics::Time::get_time_of_day();
      }

      if (!request_info.user_agent.empty())
      {
        FrontendCommons::WebBrowserMatcher_var web_browser_matcher =
          common_module_->web_browser_matcher();
        FrontendCommons::PlatformMatcher_var platform_matcher = common_module_->platform_matcher();

        if (web_browser_matcher.in())
        {
          web_browser_matcher->match(request_info.browser, request_info.user_agent);
        }

        if (platform_matcher.in())
        {
          std::string short_os;
          platform_matcher->match(0, short_os, request_info.os, request_info.user_agent);
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
        "Can't fill request info. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  std::string
  RequestInfoFiller::reason_to_str_(int reason) const noexcept
  {
    auto reason_it = yn_reasons_.find(reason);
    if (reason_it != yn_reasons_.end())
    {
      return reason_it->second;
    }

    return std::string("unknown");
  }
} /*WebStat*/
