#include <Frontends/FrontendCommons/Cookies.hpp>

#include "RequestInfoFiller.hpp"

namespace
{
  namespace Aspect
  {
    const char PASSBACK_FRONTEND[] = "Passback::Frontend";
  }

  namespace Request
  {
    namespace Parameters
    {
      const String::SubString DATA_PASSBACK_URL("dp");
      const String::SubString PASSBACK_URL("passback");
      const String::SubString PASSBACK_URL2("redir");
      const String::SubString REQUEST_ID("requestid");
      const String::SubString USER_ID_DISTRIBUTION_HASH("h");
      const String::SubString TEST_REQUEST("testrequest");
      const String::SubString PUBLISHER_PIXEL_ACCOUNT_IDS("paid");
      const String::SubString TOKEN_PREFIX("t.");
      const String::SubString ENCODED_TOKENS("dt");

      /* debug params */
      const String::SubString DEBUG_TIME("debug-time");
    }
  }

  namespace Tokens
  {
    const String::SubString RANDOM("RANDOM");
  }

  const std::string TOKEN_SUBS_PREFIX("##");
}

namespace AdServer
{
namespace Passback
{
  namespace
  {
    class UserIdHashModProcessor: public PassbackParamProcessor
    {
    public:
      virtual void process(
        PassbackInfo& passback_info,
        const String::SubString& value) const
        /*throw(RequestInfoFiller::InvalidParamException)*/
      {
        unsigned long val;
        if(String::StringManip::str_to_int(value, val))
        {
          passback_info.user_id_hash_mod = val;
        }
      }

    protected:
      virtual ~UserIdHashModProcessor() noexcept {}
    };

    class DataPassbackUrlParamProcessor: public PassbackParamProcessor
    {
    public:
      virtual void
      process(
        PassbackInfo& passback_info,
        const String::SubString& value) const
        /*throw(RequestInfoFiller::InvalidParamException)*/
      {
        std::string decoded;

        try
        {
          String::StringManip::base64mod_decode(decoded, value);
        }
        catch(const String::StringManip::InvalidFormatException&)
        {}

        if(!decoded.empty())
        {
          passback_info.passback_url_templ = decoded;
        }
      }

    protected:
      virtual
      ~DataPassbackUrlParamProcessor() noexcept {}
    };

    class DataTokensParamProcessor: public PassbackParamProcessor
    {
    public:
      static const char AMP = '&';
      static const char EQ = '=';

      DataTokensParamProcessor()
      {
        // fill short aliases
        token_aliases_[String::SubString("adid")] = "ADVERTISING_ID";
        token_aliases_[String::SubString("tdt")] = "TNS_COUNTER_DEVICE_TYPE";
        token_aliases_[String::SubString("ep")] = "EXT_TRACK_PARAMS";
        token_aliases_[String::SubString("appid")] = "APPLICATION_ID";
      }

      virtual void
      process(
        PassbackInfo& passback_info,
        const String::SubString& value) const
        /*throw(RequestInfoFiller::InvalidParamException)*/
      {
        std::string decoded;

        try
        {
          String::StringManip::base64mod_decode(decoded, value);
        }
        catch(const String::StringManip::InvalidFormatException&)
        {
          return;
        }

        // fill tokens
        std::string::const_iterator start_name = decoded.begin();
        std::string::const_iterator end_name = decoded.begin();
        bool replace_double_amp = false;
        bool found_eq = false;
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
              String::SubString param_name(&*start_name, &*end_name);
              String::SubString param_value(&*(end_name + 1), it - end_name - 1);
              if(replace_double_amp)
              {
                String::StringManip::replace(
                  param_value,
                  purify_value,
                  String::SubString("&&", 2),
                  String::SubString("&", 1));
                param_value = purify_value;
              }

              auto alias_it = token_aliases_.find(param_name);

              if(alias_it != token_aliases_.end())
              {
                passback_info.tokens.insert(std::make_pair(
                  alias_it->second,
                  param_value.str()));
              }
              else
              {
                passback_info.tokens.insert(std::make_pair(
                  param_name.str(),
                  param_value.str()));
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

    protected:
      typedef Generics::GnuHashTable<
        Generics::SubStringHashAdapter, std::string>
        TokenNameAliasMap;

    protected:
      virtual
      ~DataTokensParamProcessor() noexcept
      {}

    protected:
      TokenNameAliasMap token_aliases_;
    };
  }

  /** RequestInfoFiller */
  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    CommonModule* common_module)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger))
  {
    param_processors_.insert(std::make_pair(
      Request::Parameters::PASSBACK_URL,
      PassbackParamProcessor_var(
        new FrontendCommons::StringParamProcessor<PassbackInfo>(
          &PassbackInfo::passback_url_templ))));
    param_processors_.insert(std::make_pair(
      Request::Parameters::PASSBACK_URL2,
      PassbackParamProcessor_var(
        new FrontendCommons::StringParamProcessor<PassbackInfo>(
          &PassbackInfo::passback_url_templ))));
    param_processors_.insert(std::make_pair(
      Request::Parameters::DATA_PASSBACK_URL,
      PassbackParamProcessor_var(
        new DataPassbackUrlParamProcessor())));
    param_processors_.insert(std::make_pair(
      Request::Parameters::REQUEST_ID,
      PassbackParamProcessor_var(
        new FrontendCommons::RequestIdParamProcessor<PassbackInfo>(
          &PassbackInfo::request_id))));
    param_processors_.insert(std::make_pair(
      Request::Parameters::USER_ID_DISTRIBUTION_HASH,
      PassbackParamProcessor_var(new UserIdHashModProcessor())));
    param_processors_.insert(std::make_pair(
      Request::Parameters::TEST_REQUEST,
      PassbackParamProcessor_var(
        new FrontendCommons::BoolParamProcessor<PassbackInfo>(
          &PassbackInfo::test_request))));
    param_processors_.insert(std::make_pair(
      Request::Parameters::DEBUG_TIME,
      PassbackParamProcessor_var(
        new FrontendCommons::TimeParamProcessor<PassbackInfo>(
          &PassbackInfo::time, Generics::Time::ONE_DAY))));
    param_processors_.insert(std::make_pair(
      Request::Parameters::PUBLISHER_PIXEL_ACCOUNT_IDS,
      PassbackParamProcessor_var(
        new FrontendCommons::NumberContainerParamProcessor<
          PassbackInfo,
          PassbackInfo::AccountIdList,
          String::AsciiStringManip::SepComma>(
            &PassbackInfo::pubpixel_accounts))));
    param_processors_.insert(std::make_pair(
      Request::Parameters::ENCODED_TOKENS,
      PassbackParamProcessor_var(
        new DataTokensParamProcessor())));

    // cookie_processors_
    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::CLIENT_ID,
      PassbackParamProcessor_var(
        new FrontendCommons::SignedUuidParamProcessor<PassbackInfo>(
          &PassbackInfo::current_user_id,
          common_module->user_id_controller(),
          UserIdController::PERSISTENT))));
  }

  void
  RequestInfoFiller::fill(
    PassbackInfo& passback_info,
    const HTTP::SubHeaderList& headers,
    const HTTP::ParamList& params) const
    /*throw(InvalidParamException, ForbiddenException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::fill()";

    try
    {
      for(HTTP::ParamList::const_iterator it = params.begin();
          it != params.end(); ++it)
      {
        PassbackProcessorMap::const_iterator param_it =
          param_processors_.find(it->name);

        if(param_it != param_processors_.end())
        {
          param_it->second->process(passback_info, it->value);
        }
        else if(it->name.size() >= Request::Parameters::TOKEN_PREFIX.size() &&
          it->name.compare(
            0,
            Request::Parameters::TOKEN_PREFIX.size(),
            Request::Parameters::TOKEN_PREFIX.data()) == 0)
        {
          passback_info.tokens.insert(
            std::make_pair(
              it->name.substr(Request::Parameters::TOKEN_PREFIX.size()),
              it->value));
        }
      } // url parameters processing

      if(passback_info.time == Generics::Time::ZERO)
      {
        passback_info.time = Generics::Time::get_time_of_day();
      }

      cookies_processing_(passback_info, headers);
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

    // post process passback_url
    std::string passback_url;

    if(passback_info.passback_url_templ.find(TOKEN_SUBS_PREFIX) == std::string::npos)
    {
      // inst passback_url
      passback_url = passback_info.passback_url_templ;
    }
    else
    {
      try
      {
        char random_str[40];
        unsigned long random = Generics::safe_rand();
        String::StringManip::int_to_str(random, random_str, sizeof(random_str));
        passback_info.tokens[Tokens::RANDOM] = random_str;

        String::TextTemplate::String text_template(
          passback_info.passback_url_templ,
          TOKEN_SUBS_PREFIX,
          TOKEN_SUBS_PREFIX);
        String::TextTemplate::DefaultValue default_cont(
          &passback_info.tokens);
        String::TextTemplate::ArgsEncoder encoder(
          &default_cont);
        passback_url = text_template.instantiate(encoder);
      }
      catch(const eh::Exception&)
      {
        passback_url = passback_info.passback_url_templ;
      }
    }

    if(!passback_url.empty())
    {
      try
      {
        HTTP::BrowserAddress addr(passback_url);
        passback_info.passback_url = addr.url();
      }
      catch(...)
      {}
    }
  }

  void
  RequestInfoFiller::cookies_processing_(
    PassbackInfo& passback_info,
    const HTTP::SubHeaderList& headers) const
    /*throw(InvalidParamException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::cookies_processing_()";

    HTTP::CookieList cookies;

    try
    {
      cookies.load_from_headers(headers);
    }
    catch(HTTP::CookieList::InvalidArgument& ex)
    {
      if(logger()->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught HTTP::CookieList::InvalidArgument: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::TRACE,
          Aspect::PASSBACK_FRONTEND);
      }

      throw InvalidParamException("");
    }

    for(HTTP::CookieList::const_iterator it = cookies.begin();
        it != cookies.end(); ++it)
    {
      const auto param_it = cookie_processors_.find(it->name);

      if(param_it != cookie_processors_.end())
      {
        param_it->second->process(passback_info, it->value);
      }
    }
  }
} /*Passback*/
} /*AdServer*/
