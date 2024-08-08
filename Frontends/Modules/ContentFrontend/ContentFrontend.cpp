#include <Generics/Time.hpp>
#include <Logger/StreamLogger.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Commons/PathManip.hpp>

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

  typedef FrontendCommons::DefaultConstrain<
    FrontendCommons::OnlyGetAllowed,
    FrontendCommons::ParamConstrainDefault,
    ContentFrontendConstrainTraits>
    ContentFrontendHTTPConstrain;

  const char SECURE_PROTOCOL_NAME[] = "ssl/tls filter";

  const char HANDLE_COMMAND_ERROR[] =
    "ContentFrontend::handle_command: an error occurred";
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

namespace Request
{
  namespace Parameters
  {
    const String::AsciiStringManip::Caseless FILE("file");
    const String::AsciiStringManip::Caseless CLICK_URL("c");
    const String::AsciiStringManip::Caseless PRECLICK_URL("prck");
    const String::AsciiStringManip::Caseless RESOURCE_URL_SUFFIX("rs");
    const String::AsciiStringManip::Caseless RANDOM("r");
    const String::AsciiStringManip::Caseless CAMPAIGN_MANAGER_INDEX("cmi");
  }

  namespace Header
  {
    const String::AsciiStringManip::Caseless SECURE("secure");
  }

  const String::AsciiStringManip::CharCategory
    RESOURCE_URL_SUFFIX_CATEGORY(
      String::AsciiStringManip::ALPHA_NUM,
      String::AsciiStringManip::CharCategory("/"));
}

namespace Response
{
  namespace Header
  {
    const String::SubString CONTENT_TYPE("Content-Type");
  }

  namespace Type
  {
    const String::SubString TEXT_HTML("text/html");
  }
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
    const GrpcContainerPtr& grpc_container,
    Configuration* frontend_config,
    Logging::Logger* logger,
    FrontendCommons::HttpResponseFactory* response_factory) /*throw(eh::Exception)*/
    : FrontendCommons::FrontendInterface(response_factory),
      Logging::LoggerCallbackHolder(
        Logging::Logger_var(
          new Logging::SeveritySelectorLogger(
            logger,
            0,
            frontend_config->get().ContentFeConfiguration()->Logger().log_level())),
        "ContentFrontend",
        Aspect::CONTENT_FRONTEND, 0),
      FrontendCommons::FrontendTaskPool(
        this->callback(),
        response_factory,
        frontend_config->get().ContentFeConfiguration()->threads(),
        0), // max pending tasks
      grpc_container_(grpc_container),
      frontend_config_(ReferenceCounting::add_ref(frontend_config)),
      campaign_managers_(this->logger(), Aspect::CONTENT_FRONTEND)
  {}

  void ContentFrontend::parse_configs_() /*throw(Exception)*/
  {
    static const char* FUN = "ContentFrontend::parse_configs_()";

    try
    {
      typedef Configuration::FeConfig Config;
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

      typedef Configuration::FeConfig::CommonFeConfiguration_type CommonType;
      
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
      corba_client_adapter_ = new CORBACommons::CorbaClientAdapter();
      
      campaign_managers_.resolve(
        *common_config_, corba_client_adapter_);
      
      template_files_ = new Commons::TextTemplateCache(
        config_->TemplateCache().size(),
        Generics::Time(config_->TemplateCache().timeout()),
        Commons::TextTemplateCacheConfiguration<Commons::TextTemplate>(
          Generics::Time::ONE_SECOND, new CreativesUpdater(campaign_managers_)));

      activate_object();
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
      corba_client_adapter_.reset();

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
    const FrontendCommons::HttpRequest& request,
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

  void
  ContentFrontend::handle_request_noparams_(
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer)
    noexcept
  {
    FrontendCommons::HttpRequest& request = request_holder->request();

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
      FrontendCommons::HttpRequest::parse_params(request.body(), params);
    }
    
    request.set_params(std::move(params));

    handle_request_(std::move(request_holder), std::move(response_writer));
  }

  void
  ContentFrontend::handle_request_(
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
  ContentFrontend::handle_request_(
    const FrontendCommons::HttpRequest& request,
    FrontendCommons::HttpResponse& response)
    noexcept
  {
    static const char* FUN = "ContentFrontend::handle_request()";

    int http_status = 0; // OK

    try
    {
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
        return 403; // HTTP_FORBIDDEN
      }

      Commons::TextTemplate_var templ;

      file = config_->TemplateCache().root() + file;

      try
      {
        templ = template_files_->get(file, campaign_manager_index.c_str());
      }
      catch(const eh::Exception&)
      {
        return 404; // HTTP_NOT_FOUND
      }

      typedef std::map<String::SubString, std::string> ArgMap;
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

      const bool click_url0_contains_args = click_url0.find('?') != std::string::npos;
      const char* LOCAL_AMP = click_url0_contains_args ? "&" : "*amp*";
      const char* LOCAL_EQL = click_url0_contains_args ? "=" : "*eql*";
      const std::string f_marker = std::string(LOCAL_AMP) + "m" + LOCAL_EQL + "f";
      const std::string relocate_suffix = std::string(LOCAL_AMP) + "relocate" + LOCAL_EQL;
      const std::string pub_preclick_param = !pub_preclick_url.empty() ?
        std::string(LOCAL_AMP) + "preclick" + LOCAL_EQL + mime_pub_preclick_url :
        std::string();

      // init click tokens without pub preclick
      const std::string click_url0_f = click_url0 + f_marker;
      args_cont[Tokens::CLICK0] = click_url0;
      args_cont[Tokens::PRECLICK0] = click_url0 + relocate_suffix;
      args_cont[Tokens::CLICKF0] = click_url0_f;
      args_cont[Tokens::PRECLICKF0] = click_url0_f + relocate_suffix;

      // init click tokens with pub preclick
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

      response.set_content_type(Response::Type::TEXT_HTML);

      response.get_output_stream().write(
        response_content.data(),
        response_content.size());

      return 0; // OK
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

    return http_status;
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
