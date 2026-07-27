#include <Generics/Time.hpp>
#include <optional>
#include <Logger/StreamLogger.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Commons/PathManip.hpp>
#include <Frontends/FrontendCommons/TextTemplateAwaiter.hpp>

#include "ContentFrontend.hpp"

namespace
{
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_HTTP_PREFIX("http%3a%2f%2f");
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_HTTPS_PREFIX("https%3a%2f%2f");
  const String::AsciiStringManip::Caseless
    MIME_ENCODED_SHEME_RELATIVE_PREFIX("%2f%2f");

  struct ContentFrontendConstrainTraits
  {
    static const unsigned long MAX_NUMBER_PARAMS = 50;
    static const unsigned long MAX_LENGTH_PARAM_NAME = 30;
    static const unsigned long MAX_LENGTH_PARAM_VALUE = 2000;
  };

  using ContentFrontendHTTPConstrain = FrontendCommons::DefaultConstrain<
    FrontendCommons::OnlyGetAllowed,
    FrontendCommons::ParamConstrainDefault,
    ContentFrontendConstrainTraits>;

  const char SECURE_PROTOCOL_NAME[] = "ssl/tls filter";

  const char HANDLE_COMMAND_ERROR[] =
    "ContentFrontend::handle_command: an error occurred";

  using TextTemplateCache = AdServer::Commons::TextTemplateCache;

  using TextTemplateUpdateCallback = TextTemplateCache::FarUpdateCallback;

  const unsigned TEXT_TEMPLATE_LOAD_THREADS = 2;

  FrontendCommons::RequestTask
  co_update_text_template_(
    std::shared_ptr<AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>
      campaign_manager_coro,
    std::string key,
    std::string service_index,
    TextTemplateUpdateCallback callback)
    noexcept
  {
    try
    {
      adserver::campaign_svcs::campaign_manager::GetFileRequest request;
      request.set_file_name(key);
      if(!service_index.empty())
      {
        request.set_service_index(service_index);
      }

      auto result = co_await campaign_manager_coro->co_get_file(
        std::move(request));
      if(!result.status.ok())
      {
        callback(std::optional<std::string>());
        co_return FrontendCommons::RequestResult::written();
      }

      callback(result.response.file());
    }
    catch(...)
    {
      callback(std::optional<std::string>());
    }

    co_return FrontendCommons::RequestResult::written();
  }
}

namespace Config
{
  const char ENABLE[] = "ContentFrontend_Enable";
  const char CONFIG_FILE[] = "ContentFrontend_Config";
}

namespace Aspect
{
  const char CONTENT_FRONTEND[] = "ContentFrontend";
}

namespace Request::Parameters
{
    const String::AsciiStringManip::Caseless FILE("file");
    const String::AsciiStringManip::Caseless CLICK_URL("c");
    const String::AsciiStringManip::Caseless PRECLICK_URL("prck");
    const String::AsciiStringManip::Caseless RESOURCE_URL_SUFFIX("rs");
    const String::AsciiStringManip::Caseless RANDOM("r");
    const String::AsciiStringManip::Caseless CAMPAIGN_MANAGER_INDEX("cmi");
  }

namespace Request::Header
  {
    const String::AsciiStringManip::Caseless SECURE("secure");
  }

namespace Request
{
    const String::AsciiStringManip::CharCategory
      RESOURCE_URL_SUFFIX_CATEGORY(
        String::AsciiStringManip::ALPHA_NUM,
        String::AsciiStringManip::CharCategory("/"));
}

namespace Response::Header
{
    const String::SubString CONTENT_TYPE("Content-Type");
  }

namespace Response::Type
  {
    const String::SubString TEXT_HTML("text/html");
  }

namespace Tokens
{
  const String::SubString CLICK("CLICK");
  const String::SubString CLICKF("CLICKF");
  const String::SubString CLICK0("CLICK0");
  const String::SubString CLICKF0("CLICKF0");

  const String::SubString PRECLICK("PRECLICK");
  const String::SubString PRECLICKF("PRECLICKF");
  const String::SubString PRECLICK0("PRECLICK0");
  const String::SubString PRECLICKF0("PRECLICKF0");

  const String::SubString RANDOM("RANDOM");
  const String::SubString ADIMAGE_PATH_PREFIX("ADIMAGE-PATH-PREFIX");
  const String::SubString ADIMAGE_PATH("ADIMAGE-PATH");
  const String::SubString CRVBASE("CRVBASE");
}

namespace AdServer
{
  /**
   * ContentFrontend implementation
   */
  ContentFrontend::ContentFrontend(
    Configuration* frontend_config,
    Logging::Logger* logger,
    std::shared_ptr<AdServer::Commons::ExecutorPool> request_workers,
    CommonModule* common_module) /*throw(eh::Exception)*/
    : Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().ContentFeConfiguration()->Logger().log_level())),
        "ContentFrontend",
        Aspect::CONTENT_FRONTEND, 0),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      workers_(std::move(request_workers))
  {}

  void ContentFrontend::parse_configs_() /*throw(Exception)*/
  {
    static const char* FUN = "ContentFrontend::parse_configs_()";

    try
    {
      using Config = Configuration::FeConfig;
      const Config& fe_config = frontend_config_->get();

      if(!fe_config.ContentFeConfiguration().present())
      {
        throw Exception("ContentFeConfiguration isn't present");
      }

      common_config_ = CommonConfigPtr(
        new CommonFeConfiguration(*fe_config.CommonFeConfiguration()));

      config_ = ConfigPtr(
        new ContentFeConfiguration(*fe_config.ContentFeConfiguration()));
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << e.what();
      throw Exception(ostr);
    }
  }

  void
  ContentFrontend::init() /*throw(eh::Exception)*/
  {
    static const char* FUN = "ContentFrontend::init()";

    try
    {
      parse_configs_();

      using CommonType = Configuration::FeConfig::CommonFeConfiguration_type;

      for(CommonType::TemplateRule_sequence::
            const_iterator rule_it =
            common_config_->TemplateRule().begin();
          rule_it != common_config_->TemplateRule().end();
          ++rule_it)
      {
        strings_.push_back(rule_it->name());

        TemplateRule& template_rule = template_rules_[
          String::SubString(strings_.back())];

        for(CommonType::TemplateRule_type::XsltToken_sequence::
              const_iterator token_it =
              rule_it->XsltToken().begin();
            token_it != rule_it->XsltToken().end();
            ++token_it)
        {
          if(Tokens::ADIMAGE_PATH_PREFIX == token_it->name())
          {
            template_rule.resource_url_prefix = token_it->value();
          }
          else
          {
            template_rule.tokens[token_it->name()] = token_it->value();
          }
        }
      }
        grpc_executor_ = common_module_->grpc_executor();

      auto campaign_manager = std::make_shared<
        AdServer::CampaignSvcs::CampaignManagerDistributedGrpcClient>(
          FrontendCommons::read_campaign_manager_grpc_refs(*common_config_),
          FrontendCommons::read_campaign_manager_grpc_batching_options(
            *common_config_),
          grpc_executor_,
          common_module_->grpc_coalesce_runner());
      campaign_manager_coro_ = std::make_shared<
        AdServer::CampaignSvcs::CampaignManagerGrpcCoroClient>(
          campaign_manager,
          workers_);
      add_child_object(campaign_manager);

      template_file_task_runner_ =
        new Generics::TaskRunner(callback(), TEXT_TEMPLATE_LOAD_THREADS);
      add_child_object(template_file_task_runner_.in());

      template_files_ = std::make_shared<Commons::TextTemplateCache>(
        config_->TemplateCache().size(),
        template_file_task_runner_.in(),
        Generics::Time(config_->TemplateCache().timeout()),
        Generics::Time::ONE_SECOND,
        [
          campaign_manager_coro = campaign_manager_coro_
        ](
          std::string key,
          std::string service_index,
          TextTemplateCache::FarUpdateCallback callback) noexcept
        {
          const std::string::size_type slash_pos = key.rfind('/');
          if(slash_pos != std::string::npos)
          {
            key.erase(0, slash_pos + 1);
          }

          co_update_text_template_(
            campaign_manager_coro,
            std::move(key),
            std::move(service_index),
            std::move(callback)).start_detached(nullptr);
        });
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  ContentFrontend::shutdown() noexcept
  {
    try
    {
      deactivate_object();
      wait_object();
      campaign_manager_coro_.reset();

      log(String::SubString(
          "ContentFrontend::shutdown: frontend terminated"),
        Logging::Logger::INFO,
        Aspect::CONTENT_FRONTEND);
    }
    catch (...)
    {
    }
  }

  bool
  ContentFrontend::will_handle(
    const String::SubString& uri) noexcept
  {
    std::string found_uri;

    bool result = !uri.empty() && FrontendCommons::find_uri(
      config_->UriList().Uri(), uri, found_uri);

    if(logger()->log_level() >= TraceLevel::MIDDLE)
    {
      Stream::Error ostr;
      ostr << "ContentFrontend::will_handle(" <<
        uri << "), result " << result;

      logger()->log(ostr.str(), TraceLevel::MIDDLE, Aspect::CONTENT_FRONTEND);
    }

    return result;
  }

  void ContentFrontend::parse_headers_(
    const FCGI::HttpRequest& request,
    bool& secure) noexcept
  {
    secure = false;
    const HTTP::SubHeaderList& headers = request.headers();
    for (HTTP::SubHeaderList::const_iterator it = headers.begin();
      it != headers.end(); ++it)
    {
      const String::SubString& name = it->name;
      if(name == Request::Header::SECURE)
      {
        const String::SubString& value = it->value;
        if(value.size() == 1 && (*value.begin() == '1'))
        {
          secure = true;
        }
        break;
      }
    }
  }

  FrontendCommons::RequestTask
  ContentFrontend::co_handle_request_noparams(
    FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    co_await AdServer::Commons::ExecutorPool::yield(workers_);
    auto result = co_await handle_request_noparams_(std::move(request_holder));
    co_return std::move(result);
  }

  FrontendCommons::RequestTask
  ContentFrontend::handle_request_noparams_(
    FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    FCGI::HttpRequest& request = request_holder->request();

    HTTP::ParamList params;

    if (!request.args().empty())
    {
      String::StringManip::SplitAmp tokenizer(request.args());
      String::SubString token;

      while (tokenizer.get_token(token))
      {
        String::SubString enc_name;
        String::SubString enc_value;
        String::SubString::SizeType pos = token.find('=');

        if (pos == String::SubString::NPOS)
        {
          enc_name = token;
        }
        else
        {
          enc_name = token.substr(0, pos);
          enc_value = token.substr(pos + 1);
        }

        try
        {
          HTTP::Param param;
          String::StringManip::mime_url_decode(enc_name, param.name);

          if (param.name == Request::Parameters::CLICK_URL)
          {
            // 'c' (click_url) is a last parameter
            // support 'c' isn't last in encoded case only
            if (!MIME_ENCODED_HTTP_PREFIX.start(enc_value) &&
              !MIME_ENCODED_HTTPS_PREFIX.start(enc_value) &&
              !MIME_ENCODED_SHEME_RELATIVE_PREFIX.start(enc_value))
            {
              param.value.assign(enc_value.data(), enc_value.size()); // Till the very end
              params.push_back(std::move(param));
              break;
            }
          }

          String::StringManip::mime_url_decode(enc_value, param.value);
          params.push_back(std::move(param));
        }
        catch (const String::StringManip::InvalidFormatException&)
        {
        }
      }
    }

    if (!request.body().empty())
    {
      FCGI::HttpRequest::parse_params(request.body(), params);
    }

    request.set_params(std::move(params));

    auto result = co_await co_process_request_(std::move(request_holder));
    co_return std::move(result);
  }

  FrontendCommons::RequestTask
  ContentFrontend::co_handle_request(
    FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    co_await AdServer::Commons::ExecutorPool::yield(workers_);
    auto result = co_await co_process_request_(std::move(request_holder));
    co_return std::move(result);
  }

  FrontendCommons::RequestTask
  ContentFrontend::co_process_request_(
    FCGI::HttpRequestHolder_var request_holder)
    noexcept
  {
    FCGI::HttpResponse_var response_ptr(new FCGI::HttpResponse());
    auto result = co_await process_request_(
      std::move(request_holder),
      std::move(response_ptr));
    co_return std::move(result);
  }

  FrontendCommons::RequestTask
  ContentFrontend::process_request_(
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::HttpResponse_var response)
    noexcept
  {
    static const char* FUN = "ContentFrontend::handle_request()";

    int http_status = 0; // OK

    try
    {
      const FCGI::HttpRequest& request = request_holder->request();

      ContentFrontendHTTPConstrain::apply(request);

      bool secure;
      parse_headers_(request, secure);

      Generics::SubStringHashAdapter instantiate_type =
        FrontendCommons::deduce_instantiate_type(&secure, request);

      std::string file;
      std::string click_url0;
      std::string pub_preclick_url;
      std::string resource_url_suffix;
      std::string random_str("0");
      std::string campaign_manager_index;

      for(HTTP::ParamList::const_iterator it =
            request.params().begin();
          it != request.params().end(); ++it)
      {
        if (it->name == Request::Parameters::FILE)
        {
          file = it->value;
        }
        else if (it->name == Request::Parameters::CLICK_URL)
        {
          try
          {
            HTTP::BrowserAddress addr(it->value);
            click_url0 = addr.url();
          }
          catch(...)
          {
            throw InvalidParamException("incorrect click url parameter value");
          }
        }
        else if (it->name == Request::Parameters::PRECLICK_URL)
        {
          try
          {
            HTTP::BrowserAddress addr(it->value);
            pub_preclick_url = addr.url();
          }
          catch(...)
          {
            throw InvalidParamException("incorrect preclick url parameter value");
          }
        }
        else if (it->name == Request::Parameters::RESOURCE_URL_SUFFIX)
        {
          const char* sym = Request::RESOURCE_URL_SUFFIX_CATEGORY.find_nonowned(
            it->value.c_str());
          if(sym && *sym)
          {
            throw InvalidParamException("incorrect resource suffix parameter value");
          }

          resource_url_suffix = it->value;
        }
        else if (it->name == Request::Parameters::RANDOM)
        {
          unsigned long random;
          if(!String::StringManip::str_to_int(it->value, random))
          {
            throw InvalidParamException("incorrect random parameter value");
          }
          random_str = it->value;
        }
        else if (it->name == Request::Parameters::CAMPAIGN_MANAGER_INDEX)
        {
          campaign_manager_index = it->value;
        }
      }

      if(click_url0.empty())
      {
        throw InvalidParamException("click url isn't defined");
      }

      // check file
      if(!AdServer::PathManip::normalize_path(file))
      {
        co_return FrontendCommons::RequestResult{
          403,
          response,
          false};
      }

      file = config_->TemplateCache().root() + file;

      Commons::TextTemplatePtr templ =
        co_await FrontendCommons::co_get_text_template(
          template_files_,
          workers_,
          std::move(file),
          std::move(campaign_manager_index));

      if(!templ)
      {
        http_status = 404;
      }
      else
      {
        try
        {
          http_status = fill_response_(
            *response,
            templ.get(),
            instantiate_type,
            click_url0,
            pub_preclick_url,
            resource_url_suffix,
            random_str);
        }
        catch(const eh::Exception& e)
        {
          http_status = 500;
          Stream::Error ostr;
          ostr << "ContentFrontend::handle_request(): "
            "eh::Exception has been caught: " << e.what();
          logger()->log(ostr.str(), Logging::Logger::EMERGENCY,
            Aspect::CONTENT_FRONTEND, "ADS-IMPL-191");
        }
      }
    }
    catch (const ForbiddenException& ex)
    {
      http_status = 403; // HTTP_FORBIDDEN
      logger()->sstream(TraceLevel::LOW, Aspect::CONTENT_FRONTEND) <<
        FUN << ": ForbiddenException caught: " << ex.what();
    }
    catch (const InvalidParamException& ex)
    {
      http_status = 400; // HTTP_BAD_REQUEST
      logger()->sstream(TraceLevel::MIDDLE, Aspect::CONTENT_FRONTEND) <<
        FUN << ": InvalidParamException caught: " << ex.what();
    }
    catch(const eh::Exception& e)
    {
      http_status = 500; // HTTP_INTERNAL_SERVER_ERROR
      Stream::Error ostr;
      ostr << FUN << ": eh::Exception has been caught: " << e.what();
      logger()->log(ostr.str(), Logging::Logger::EMERGENCY,
        Aspect::CONTENT_FRONTEND, "ADS-IMPL-191");
    }

    co_return FrontendCommons::RequestResult{
      http_status,
      response,
      false};
  }

  int
  ContentFrontend::fill_response_(
    FCGI::HttpResponse& response,
    Commons::TextTemplate* templ,
    const Generics::SubStringHashAdapter& instantiate_type,
    const std::string& click_url0,
    const std::string& pub_preclick_url,
    const std::string& resource_url_suffix,
    const std::string& random_str)
    const /*throw(eh::Exception)*/
  {
    using ArgMap = std::map<String::SubString, std::string>;
    ArgMap args_cont;

    TemplateRuleMap::const_iterator rule_it = template_rules_.find(instantiate_type);

    if(rule_it != template_rules_.end())
    {
      for(TokenValueMap::const_iterator it = rule_it->second.tokens.begin();
          it != rule_it->second.tokens.end(); ++it)
      {
        args_cont[it->first] = it->second;
      }

      args_cont[Tokens::CRVBASE] = args_cont[Tokens::ADIMAGE_PATH] =
        rule_it->second.resource_url_prefix + resource_url_suffix;
    }

    std::string mime_pub_preclick_url;
    String::StringManip::mime_url_encode(
      pub_preclick_url,
      mime_pub_preclick_url);

    const bool click_url0_contains_args =
      click_url0.find('?') != std::string::npos;
    const char* LOCAL_AMP = click_url0_contains_args ? "&" : "*amp*";
    const char* LOCAL_EQL = click_url0_contains_args ? "=" : "*eql*";
    const std::string f_marker =
      std::string(LOCAL_AMP) + "m" + LOCAL_EQL + "f";
    const std::string relocate_suffix =
      std::string(LOCAL_AMP) + "relocate" + LOCAL_EQL;
    const std::string pub_preclick_param = !pub_preclick_url.empty() ?
      std::string(LOCAL_AMP) + "preclick" + LOCAL_EQL + mime_pub_preclick_url :
      std::string();

    const std::string click_url0_f = click_url0 + f_marker;
    args_cont[Tokens::CLICK0] = click_url0;
    args_cont[Tokens::PRECLICK0] = click_url0 + relocate_suffix;
    args_cont[Tokens::CLICKF0] = click_url0_f;
    args_cont[Tokens::PRECLICKF0] = click_url0_f + relocate_suffix;

    const std::string click_url = click_url0 + pub_preclick_param;
    const std::string click_url_f = click_url + f_marker;
    args_cont[Tokens::CLICK] = click_url;
    args_cont[Tokens::PRECLICK] = click_url + relocate_suffix;
    args_cont[Tokens::CLICKF] = click_url_f;
    args_cont[Tokens::PRECLICKF] = click_url_f + relocate_suffix;
    args_cont[Tokens::RANDOM] = random_str;

    String::TextTemplate::ArgsContainer<ArgMap> args(&args_cont);
    String::TextTemplate::DefaultValue default_cont(&args);
    String::TextTemplate::ArgsEncoder encoder(&default_cont);
    std::string response_content = templ->instantiate(encoder);

    response.set_content_type_nocopy(Response::Type::TEXT_HTML);

    response.get_output_stream().write(
      response_content.data(),
      response_content.size());

    return 0;
  }

  bool
  ContentFrontend::log(
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

}
