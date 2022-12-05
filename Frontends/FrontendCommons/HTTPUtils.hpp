/// @file FrontendCommons/HTTPUtils.hpp

#ifndef FRONTENDCOMMONS_HTTPUTILS_HPP
#define FRONTENDCOMMONS_HTTPUTILS_HPP

#include <openssl/md5.h>
#include <string>

#include <eh/Exception.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <HTTP/Http.hpp>
#include <HTTP/UrlAddress.hpp>
#include <Language/BLogic/NormalizeTrigger.hpp>

#include <xsd/Frontends/FeConfig.hpp>

#include "HTTPExceptions.hpp"

namespace MergeMessage
{
  const char SOURCE_IS_PROBE[] = "source profile - probe user";
  const char SOURCE_IS_UNKNOWN[] = "source profile - unknown user";
  const char SOURCE_NOT_READY[] = "source profile - not ready";
  const char SOURCE_EXCEPTION[] = "source profile - exception";
  const char SOURCE_IS_UNAVAILABLE[] = "source profile - unavailable";
  const char SOURCE_IS_INVALID[] = "source profile - invalid uid";

  const char MERGE_EXCEPTION[] = "merge - exception";
  const char MERGE_NOT_READY[] = "merge - not ready";
  const char MERGE_UNAVAILABLE[] = "merge - unavailable";
}

namespace FrontendCommons
{
  namespace ContentType
  {
    const String::SubString TEXT_HTML("text/html");
  }

  const Generics::SubStringHashAdapter UNSECURE_INSTANTIATE_TYPE(
    String::SubString("unsecure"));
  const Generics::SubStringHashAdapter SECURE_INSTANTIATE_TYPE(
    String::SubString("secure"));
  const String::AsciiStringManip::Caseless SECURE_PROTOCOL_NAME(
    "ssl/tls filter");

  typedef std::map<std::string, std::string> ParsedParamsMap;

  void
  ip_hash(
    std::string& hash,
    const String::SubString& ip_address,
    const String::SubString& ip_salt)
    noexcept;

  /**
   * Most used scheme of front-end's constraints checking.
   * Control HTTP method, HTTP params amount and each params requirements
   */
  template <
    typename HTTPMethodConstrain,
    typename HTTPParamsConstrain,
    typename ConstrainTraits>
  struct DefaultConstrain : public HTTPExceptions
  {
    template<typename HttpRequest>
    static void
    apply(const HttpRequest& request)
      /*throw(ForbiddenException, InvalidParamException)*/
    {
      static const char* FUN = "DefaultConstrain::apply()";

      HTTPMethodConstrain::apply(request);

      const HTTP::ParamList& params = request.params();
      if (params.size() > ConstrainTraits::MAX_NUMBER_PARAMS)
      {
        Stream::Error ostr;
        ostr << FUN << ": Params number(" << params.size() << ") exceed";

        throw InvalidParamException(ostr);
      }
      for (HTTP::ParamList::const_iterator it = params.begin();
        it != params.end(); ++it)
      {
        HTTPParamsConstrain::template apply<ConstrainTraits>(*it);
      }
    }
  };

  /** HTTP GET method allowed only */
  struct OnlyGetAllowed: public HTTPExceptions
  {
    template<typename HttpRequest>
    static void
    apply(const HttpRequest& request) /*throw(ForbiddenException)*/
    {
      if (request.method() != 0 /* M_GET */ || request.header_only())
      {
        throw ForbiddenException(
          "HTTPConstrain::apply(): Forbidden HTTP method");
      }
    }
  };

  /** HTTP GET and POST methods allowed only */
  struct OnlyGetAndPostAllowed: public HTTPExceptions
  {
    template<typename HttpRequest>
    static void
    apply(const HttpRequest& request) /*throw(ForbiddenException)*/
    {
      if ((request.method() != 0 /* M_GET */ && request.method() != 2 /* M_POST */) ||
        request.header_only())
      {
        throw ForbiddenException(
          "HTTP_constrain::apply(): Forbidden HTTP method");
      }
    }
  };

  struct GetPostPutAllowed
  {
    template<typename HttpRequest>
    static void
    apply(const HttpRequest&)
    {}
  };

  /** Most used HTTP param requirements */
  struct ParamConstrainDefault : HTTPExceptions
  {
    template <typename ConstrainTraits>
    static void
    apply(const HTTP::Param& param) /*throw(InvalidParamException)*/
    {
      static const char* FUN = "ParamConstrainDefault::apply()";

      if (param.name.size() > ConstrainTraits::MAX_LENGTH_PARAM_NAME)
      {
        Stream::Error ostr;
        ostr << FUN << ": param name length(" << param.name.size() << ") exceed";

        throw InvalidParamException(ostr);
      }
      else if(param.value.size() > ConstrainTraits::MAX_LENGTH_PARAM_VALUE)
      {
        Stream::Error ostr;
        ostr << FUN << ": param value length(" << param.value.size() <<
          ") exceed";

        throw InvalidParamException(ostr);
      }
    }
  };

  /**
   * some util functions for reduction code
   * works with http & configuration instances
   */

  /*
  inline
  void
  add_cookie(
    const HTTP::CookieDef& cookie,
    Apache::HttpResponse& response)
  {
    HTTP::CookieDefList cookies;
    cookies.push_back(cookie);

    HTTP::HeaderList headers;
    cookies.set_cookie_header(headers);

    for (HTTP::HeaderList::const_iterator it = headers.begin();
         it != headers.end(); it++)
    {
      response.add_header(it->name.c_str(), it->value.c_str());
    }
  }
  */

  template<typename HttpRequest>
  inline
  bool
  is_secure_request(const HttpRequest& request) noexcept
  {
    return request.secure();
  }

  template<typename HttpRequest>
  inline
  Generics::SubStringHashAdapter
  deduce_instantiate_type(
    bool* secure,
    const HttpRequest& request)
    noexcept
  {
    if(secure && *secure)
    {
      return SECURE_INSTANTIATE_TYPE;
    }

    if (is_secure_request(request))
    {
      if(secure)
      {
        *secure = true;
      }

      return SECURE_INSTANTIATE_TYPE;
    }

    return UNSECURE_INSTANTIATE_TYPE;
  }

  template<typename HttpResponse>
  inline
  void
  add_headers(
    const xsd::AdServer::Configuration::ResponseHeadersType& headers,
    HttpResponse& response)
  {
    for(xsd::AdServer::Configuration::ResponseHeadersType::
        Header_sequence::const_iterator
          it = headers.Header().begin();
        it != headers.Header().end(); ++it)
    {
      response.add_header(it->name(), it->value());
    }
  }

  inline
  bool
  find_uri(
    const xsd::AdServer::Configuration::UriListType::Uri_sequence& uri_list,
    const char* uri,
    std::string& found_uri,
    std::string* marker = 0,
    bool strict_equal = true)
    noexcept
  {
    String::SubString suri(uri);
    typedef xsd::AdServer::Configuration::UriListType::Uri_sequence UriSeq;

    for(UriSeq::const_iterator uri_it = uri_list.begin();
        uri_it != uri_list.end(); ++uri_it)
    {
      const std::string& cur_uri = uri_it->path();
      if (suri.substr(0, cur_uri.size()) == cur_uri &&
          (!strict_equal ||
            (suri.size() == cur_uri.size() ||
             (suri.size() > cur_uri.size() && uri[cur_uri.size()] == '?'))))
      {
        if(marker && uri_it->marker().present())
        {
          *marker = *uri_it->marker();
        }
        found_uri = cur_uri;
        return true;
      }
    }

    return false;
  }

  inline
  bool
  find_uri(
    const xsd::AdServer::Configuration::UriListType::Uri_sequence& uri_list,
    const String::SubString suri,
    std::string& found_uri,
    std::string* marker = 0,
    bool strict_equal = true)
    noexcept
  {
    typedef xsd::AdServer::Configuration::UriListType::Uri_sequence UriSeq;

    for(UriSeq::const_iterator uri_it = uri_list.begin();
        uri_it != uri_list.end(); ++uri_it)
    {
      const std::string& cur_uri = uri_it->path();
      if (suri.substr(0, cur_uri.size()) == cur_uri &&
          (!strict_equal ||
            (suri.size() == cur_uri.size() ||
             (suri.size() > cur_uri.size() && suri[cur_uri.size()] == '?'))))
      {
        if(marker && uri_it->marker().present())
        {
          *marker = *uri_it->marker();
        }
        found_uri = cur_uri;
        return true;
      }
    }

    return false;
  }

  inline
  bool
  find_suburi(
    const xsd::AdServer::Configuration::UriListType::Uri_sequence& uri_list,
    const char* uri,
    const char* suburi,
    std::string& found_uri)
    noexcept
  {
    String::SubString suri(uri), ssuburi(suburi);
    typedef xsd::AdServer::Configuration::UriListType::Uri_sequence UriSeq;

    for(UriSeq::const_iterator uri_it = uri_list.begin();
        uri_it != uri_list.end(); ++uri_it)
    {
      const std::string& cur_uri = uri_it->path();
      if (suri.substr(0, cur_uri.size()) == cur_uri)
      {
        if (suri.substr(cur_uri.size(), ssuburi.size()) == ssuburi)
        {
          found_uri = cur_uri;
          return true;
        }

        return false;
      }
    }

    return false;
  }

  /*
  inline
  void
  remove_cookies(
    const char* cookie_path,
    const char* cookie_domain_val,
    const HTTP::CookieList& input_cookies,
    const CookieNameList& remove_cookies,
    Apache::HttpResponse& response)
  {
    typedef std::map<std::string, unsigned long> CookieCounter;

    CookieCounter cookie_counter;
    std::string cookie_domain(cookie_domain_val);

    try
    {
      struct tm parsed_time =
        (Generics::Time::get_time_of_day() - 365 * 24 * 60 * 60).get_gm_time();

      HTTP::HeaderList out_cookie_headers;

      for(HTTP::CookieList::const_iterator it = input_cookies.begin();
          it != input_cookies.end(); it++)
      {
        const char* cookie_name = it->name.c_str();

        for (CookieNameList::const_iterator rem_it =
               remove_cookies.begin();
             rem_it != remove_cookies.end(); ++rem_it)
        {
          if(!strcasecmp(cookie_name, rem_it->c_str()))
          {
            HTTP::CookieDefList out_cookies;

            out_cookies.push_back(
              HTTP::CookieDef(
                rem_it->c_str(),
                "na",
                cookie_domain.c_str(),
                cookie_path,
                parsed_time,
                false));

            out_cookies.set_cookie_header(out_cookie_headers);

            break;
          }
        }

        CookieCounter::iterator cit = cookie_counter.find(cookie_name);

        if(cit == cookie_counter.end())
        {
          cookie_counter[cookie_name] = 1;
        }
        else
        {
          cit->second++;
        }
      }

      if(!cookie_domain.empty())
      {
        for(CookieCounter::iterator it = cookie_counter.begin();
            it != cookie_counter.end(); it++)
        {
          if(it->second > 1)
          {
            // Karen: This is to fix the situation with Firefox happened
            // after importing cookies from IE, when
            // Firefox places cookies from "hostname" to
            // domain ".hostname"

            HTTP::CookieDefList out_cookies;

            out_cookies.push_back(
              HTTP::CookieDef(
                it->first.c_str(),
                "na",
                cookie_domain.c_str(),
                cookie_path,
                parsed_time,
                false));

            out_cookies.set_cookie_header(out_cookie_headers);
          }
        }
      }

      for (HTTP::HeaderList::const_iterator it =
             out_cookie_headers.begin();
           it != out_cookie_headers.end(); ++it)
      {
        response.add_header(it->name.c_str(), it->value.c_str());
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "FrontendCommons::remove_cookies: "
        "Can't do clearing. "
        "Caught eh::Exception. "
        ": "
        << ex.what();

      throw RemoveCookiesHelper::Exception(ostr);
    }
  }
  */

  template<typename HttpResponse>
  inline
  int
  redirect(const String::SubString& url, HttpResponse& response)
    /*throw(eh::Exception)*/
  {
    response.add_header(String::SubString("Location"), url);
    return 302; // HTTP_MOVED_TEMPORARILY; // HTTP_SEE_OTHER;
  }

  template<typename HttpResponse>
  inline
  void
  no_cache(HttpResponse& response) noexcept
  {
    response.add_header("Cache-Control", "no-store, no-cache");
    response.add_header("Expires", "Sat, 26 Jul 1997 05:00:00 GMT");
    response.add_header("Vary", "Cookie");
  }

  inline
  void
  parse_args(
    std::map<std::string, std::string>& parsed_params,
    const String::SubString& args,
    const String::SubString& amp,
    const String::SubString& eql)
  {
    String::SubString arguments = args;

    bool parse = true;

    while(parse)
    {
      String::SubString::SizeType pos = arguments.find(amp);

      String::SubString arg_pair;

      if(pos == std::string::npos)
      {
        arg_pair = arguments;
        parse = false;
      }
      else
      {
        arg_pair = arguments.substr(0, pos);
        arguments = arguments.substr(pos + amp.size());
      }

      pos = arg_pair.find(eql);

      String::SubString name;
      String::SubString value;

      if(pos == String::SubString::NPOS)
      {
        name = arg_pair;
      }
      else
      {
        name = arg_pair.substr(0, pos);
        value = arg_pair.substr(pos + eql.size());
      }

      parsed_params[name.str()] = value.str();
    }
  }

  template<typename HttpRequest>
  inline
  void
  print_request(
    std::ostringstream& ostr,
    const HttpRequest& request)
    noexcept
  {
    ostr << "Uri: " << request.uri() << std::endl <<
      "Params ("<< request.params().size() << "):"  << std::endl;

    for(HTTP::ParamList::const_iterator it =
          request.params().begin(); it != request.params().end(); it++)
    {
      ostr << "    " << it->name << " : " << it->value << std::endl;
    }

    ostr << "Headers ("<< request.headers().size() << "):"  << std::endl;

    for (HTTP::SubHeaderList::const_iterator it =
           request.headers().begin(); it != request.headers().end(); ++it)
    {
      ostr << "    " << it->name << " : " << it->value << std::endl;
    }
  }


  inline
  void
  extract_url_keywords(
    std::string& result,
    const String::SubString& _url,
    const Language::Segmentor::SegmentorInterface* segmentor) noexcept
  {
    try
    {
      String::SubString url(_url);
      String::StringManip::trim(url);
      if (!url.empty())
      {
        std::string kw_from_http = HTTP::keywords_from_http_address(url);
        Language::Trigger::normalize_phrase(kw_from_http, result, segmentor);
      }
    }
    catch (const Language::Trigger::Exception&)
    {}
    catch (const eh::Exception&)
    {}
    catch (...)
    {}
  }
}

namespace FrontendCommons
{
  inline
  void
  ip_hash(
    std::string& result_hash,
    const String::SubString& ip_address,
    const String::SubString& ip_salt)
    noexcept
  {
    if(!ip_salt.empty())
    {
      MD5_CTX ctx;
      MD5_Init(&ctx);
      MD5_Update(&ctx, ip_address.data(), ip_address.size());
      MD5_Update(&ctx, ip_salt.data(), ip_salt.size());
      unsigned char hash[MD5_DIGEST_LENGTH];
      MD5_Final(hash, &ctx);
      String::StringManip::base64_encode(result_hash, hash, sizeof(hash), false);
    }
    else
    {
      result_hash = ip_address.str();
    }
  }

  inline
  std::string
  normalize_abs_url(
    const HTTP::HTTPAddress& url_address,
    unsigned long get_view_flags = HTTP::HTTPAddress::VW_FULL,
    const String::SubString& prefix = HTTP::HTTP_PREFIX.str) /*throw(eh::Exception)*/
  {
    std::string result;
    url_address.get_view(get_view_flags, result);

    if (url_address.scheme().empty())
    {
      result = prefix + result;
    }

    return result;
  }

  inline
  unsigned long
  referer_hash(const String::SubString& s) noexcept
  {
    std::size_t value = 0;
    {
      Generics::Murmur32v3Hash hasher(value);
      hasher.add(s.data(), s.size());
    }
    return value;
  }

  inline
  unsigned long
  short_referer_hash(const String::SubString& s) noexcept
  {
    try
    {
      HTTP::BrowserAddress addr(s);
      std::string short_referer;
      short_referer = FrontendCommons::normalize_abs_url(
        addr,
        HTTP::HTTPAddress::VW_PROTOCOL |
          HTTP::HTTPAddress::VW_HOSTNAME |
          HTTP::HTTPAddress::VW_NDEF_PORT |
          HTTP::HTTPAddress::VW_PATH);

      return FrontendCommons::referer_hash(short_referer);
    }
    catch(const eh::Exception&)
    {}

    return 0;
  }

  class CORS
  {
  public:
    template<typename HttpRequest, typename HttpResponse>
    static void
    set_headers(
      const HttpRequest& request,
      HttpResponse& response)
      /*throw(eh::Exception)*/
    {
      const String::SubString ORIGIN("origin");
      const HTTP::SubHeaderList& headers = request.headers();
      String::SubString origin;

      for (HTTP::SubHeaderList::const_iterator ci = headers.begin();
           ci != headers.end(); ++ci)
      {
        std::string name = ci->name.str();
        String::AsciiStringManip::to_lower(name);

        if (name == ORIGIN)
        {
          origin = ci->value;
          break;
        }
      }

      if (!origin.empty())
      {
        // set Access-Control-Allow-Origin = Origin for force:
        //   The value of the 'Access-Control-Allow-Origin' header in
        //   the response must not be the wildcard '*' when the request's credentials
        //   mode is 'include'
        response.add_header(
          String::SubString("Access-Control-Allow-Origin"),
          origin);
      }
      else
      {
        response.add_header(
          String::SubString("Access-Control-Allow-Origin"),
          String::SubString("*"));
      }

      response.add_header(
        String::SubString("Access-Control-Allow-Credentials"),
        String::SubString("true"));
      response.add_header(
        String::SubString("Vary"),
        String::SubString("Origin"));
    }
  };

  inline void
  get_ip_keywords(std::vector<std::string>& keywords, const String::SubString& addr)
  {
    std::string res;

    String::StringManip::Splitter<String::AsciiStringManip::SepPeriod> tokenizer(addr);
    String::SubString token1;
    String::SubString token2;
    String::SubString token3;
    String::SubString token4;

    if(tokenizer.get_token(token1) &&
      tokenizer.get_token(token2) &&
      tokenizer.get_token(token3) &&
      tokenizer.get_token(token4))
    {
      std::string res_keyword_ip3;
      res_keyword_ip3 += token1.str();
      res_keyword_ip3 += 'x';
      res_keyword_ip3 += token2.str();
      res_keyword_ip3 += 'x';
      res_keyword_ip3 += token3.str();

      std::string res_keyword_ip4(res_keyword_ip3);
      res_keyword_ip4 += 'x';
      res_keyword_ip4 += token4.str();

      keywords.push_back(std::string("rtbip") + res_keyword_ip3);
      keywords.push_back(std::string("rtbip") + res_keyword_ip4);
    }
  }
}

#endif /*FRONTENDCOMMONS_HTTPUTILS_HPP*/
