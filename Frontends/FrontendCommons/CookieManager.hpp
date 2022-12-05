#ifndef FRONTENDCOMMONS_COOKIEMANAGER_HPP
#define FRONTENDCOMMONS_COOKIEMANAGER_HPP

#include <string>
#include <list>
#include <map>
#include <iostream>

#include <eh/Exception.hpp>

#include <HTTP/Http.hpp>
#include <HTTP/HTTPCookie.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>

#include <xsd/Frontends/FeConfig.hpp>
#include "Cookies.hpp"

namespace FrontendCommons
{
  namespace
  {
    const std::string SAME_SITE_NONE("; SameSite=None; Secure");
  }

  typedef Generics::GnuHashSet<Generics::SubStringHashAdapter> CookieNameSet;

  //
  // CookieManager
  //
  template<typename HttpRequestType, typename HttpResponseType>
  class CookieManager
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    CookieManager(
      const xsd::AdServer::Configuration::CookiesType& cookies_config)
      noexcept;

    void
    set_uid(
      HttpResponseType& response,
      const HttpRequestType& request,
      const String::SubString& value,
      const Generics::Time& expire = Generics::Time::ZERO) const
      /*throw(Exception)*/;

    void
    set(HttpResponseType& response,
      const HttpRequestType& request,
      const Generics::SubStringHashAdapter& name,
      const String::SubString& value,
      const Generics::Time& expire = Generics::Time::ZERO,
      bool same_site_none = true) const
      /*throw(Exception)*/;

    void
    remove(HttpResponseType& response,
      const HttpRequestType& request,
      const Generics::SubStringHashAdapter& name) const
      /*throw(Exception)*/;

    void
    remove(HttpResponseType& response,
      const HttpRequestType& request,
      const HTTP::CookieList& input_cookies,
      const CookieNameSet& names) const
      /*throw(Exception)*/;

  private:
    class CookieHeadersWriter;

    struct CookiePath
    {
      std::string domain;
      std::string path;
    };

    struct CookieParams: public CookiePath
    {
      Generics::Time expires;
    };

    typedef std::list<CookiePath> CookiePathList;
    typedef Generics::GnuHashTable<Generics::SubStringHashAdapter, CookiePathList>
      RemoveCookieMap;
    typedef Generics::GnuHashTable<Generics::SubStringHashAdapter, CookieParams>
      CookieParamsMap;
    typedef Generics::GnuHashTable<Generics::StringHashAdapter, std::string>
      ResponseHeaderMap;

  private:
    void
    remove_(HttpResponseType& response,
      const HttpRequestType& request,
      const Generics::SubStringHashAdapter& name) const
      /*throw(Exception)*/;

    void
    get_cookie_domain_(
      const HttpRequestType& request,
      const CookiePath& cookie_params,
      std::string& res_domain) const;

  private:
    CookieParams default_cookies_;

    // cookie_names_holder_: hold string used in internal SubStringHashAdapter's
    std::list<std::string> cookie_names_holder_;
    CookieParamsMap set_cookies_;
    RemoveCookieMap remove_cookies_;
    ResponseHeaderMap response_headers_;
  };
}

namespace FrontendCommons
{
  /**
   * If first cookie was installed in response add specified headers to response
   */
  template<typename HttpRequestType, typename HttpResponseType>
  class CookieManager<HttpRequestType, HttpResponseType>::CookieHeadersWriter
  {
  public:
    CookieHeadersWriter(
      HttpResponseType& response,
      const ResponseHeaderMap& response_headers)
      noexcept;

    ~CookieHeadersWriter() noexcept;

  private:
    HttpResponseType& response_;
    const ResponseHeaderMap& response_headers_;
    const bool old_cookie_installed_;
  };

  //
  // CookieManager::CookieHeadersWriter implementation
  //
  template<typename HttpRequestType, typename HttpResponseType>
  CookieManager<HttpRequestType, HttpResponseType>::
  CookieHeadersWriter::CookieHeadersWriter(
    HttpResponseType& response,
    const ResponseHeaderMap& response_headers)
    noexcept
    : response_(response),
      response_headers_(response_headers),
      old_cookie_installed_(response.cookie_installed())
  {}

  template<typename HttpRequestType, typename HttpResponseType>
  CookieManager<HttpRequestType, HttpResponseType>::
  CookieHeadersWriter::~CookieHeadersWriter() noexcept
  {
    if(response_.cookie_installed() != old_cookie_installed_)
    {
      try
      {
        for(ResponseHeaderMap::const_iterator it = response_headers_.begin();
          it != response_headers_.end(); ++it)
        {
          response_.add_header(it->first.text(), it->second);
        }
      }
      catch (const eh::Exception&)
      {}
    }
  }

  // CookieManager impl
  template<typename HttpRequestType, typename HttpResponseType>
  CookieManager<HttpRequestType, HttpResponseType>::CookieManager(
    const xsd::AdServer::Configuration::CookiesType& cookies_config)
    noexcept
  {
    default_cookies_.domain = cookies_config.domain();
    default_cookies_.path = cookies_config.path();
    default_cookies_.expires =
      Generics::Time(cookies_config.expires());

    for (xsd::AdServer::Configuration::CookiesType::
      ResponseHeader_sequence::const_iterator
        it = cookies_config.ResponseHeader().begin();
        it != cookies_config.ResponseHeader().end(); ++it)
    {
      response_headers_.insert(std::make_pair(it->name(), it->value()));
    }

    for(xsd::AdServer::Configuration::CookiesType::Cookie_sequence::const_iterator
          cookie_it = cookies_config.Cookie().begin();
        cookie_it != cookies_config.Cookie().end(); ++cookie_it)
    {
      CookieParams cookie_params;
      cookie_params.domain = cookie_it->domain().present() ?
        cookie_it->domain()->c_str() : default_cookies_.domain.c_str();
      cookie_params.path = cookie_it->path().present() ?
        cookie_it->path()->c_str() : default_cookies_.path.c_str();
      cookie_params.expires = cookie_it->expires().present() ?
        Generics::Time(*cookie_it->expires()) :
        default_cookies_.expires;

      cookie_names_holder_.push_back(cookie_it->name());
      set_cookies_.insert(std::make_pair(cookie_names_holder_.back(), cookie_params));
    }

    for(xsd::AdServer::Configuration::CookiesType::RemoveCookie_sequence::const_iterator
          cookie_it = cookies_config.RemoveCookie().begin();
        cookie_it != cookies_config.RemoveCookie().end(); ++cookie_it)
    {
      CookiePath cookie_params;
      cookie_params.domain = cookie_it->domain().present() ?
        cookie_it->domain()->c_str() : default_cookies_.domain.c_str();
      cookie_params.path = cookie_it->path().present() ?
        cookie_it->path()->c_str() : default_cookies_.path.c_str();

      cookie_names_holder_.push_back(cookie_it->name());
      remove_cookies_[Generics::SubStringHashAdapter(cookie_names_holder_.back())
        ].push_back(cookie_params);
    }
  }

  template<typename HttpRequestType, typename HttpResponseType>
  void
  CookieManager<HttpRequestType, HttpResponseType>::set_uid(
    HttpResponseType& response,
    const HttpRequestType& request,
    const String::SubString& value,
    const Generics::Time& expire) const
    /*throw(Exception)*/
  {
    set(
      response,
      request,
      FrontendCommons::Cookies::CLIENT_ID,
      value,
      expire,
      false);

    /*
    set(
      response,
      request,
      FrontendCommons::Cookies::CLIENT_ID2,
      value,
      expire,
      true);
    */
  }

  template<typename HttpRequestType, typename HttpResponseType>
  void
  CookieManager<HttpRequestType, HttpResponseType>::set(
    HttpResponseType& response,
    const HttpRequestType& request,
    const Generics::SubStringHashAdapter& name,
    const String::SubString& value,
    const Generics::Time& expire,
    bool same_site_none) const
    /*throw(Exception)*/
  {
    static const char* FUN = "CookieManager::set()";

    try
    {
      typename CookieParamsMap::const_iterator it = set_cookies_.find(name);
      const CookieParams& cookie_params = (
        it != set_cookies_.end() ? it->second : default_cookies_);

      std::string res_domain;
      get_cookie_domain_(request, cookie_params, res_domain);

      HTTP::CookieDef cookie(
        name,
        value,
        res_domain,
        cookie_params.path,
        (expire == Generics::Time::ZERO ?
          Generics::Time::get_time_of_day() + cookie_params.expires :
          Generics::Time::get_time_of_day() + expire).get_gm_time(),
        false);

      std::string header;
      HTTP::cookie_header_plain(cookie, header);
      if(request.secure() && same_site_none)
      {
        header += SAME_SITE_NONE;
      }
      CookieHeadersWriter guard(response, response_headers_);
      response.add_cookie(header.c_str());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't set cookie value. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  template<typename HttpRequestType, typename HttpResponseType>
  void
  CookieManager<HttpRequestType, HttpResponseType>::remove(
    HttpResponseType& response,
    const HttpRequestType& request,
    const Generics::SubStringHashAdapter& name) const
    /*throw(Exception)*/
  {
    CookieHeadersWriter guard(response, response_headers_);
    remove_(response, request, name);
  }

  template<typename HttpRequestType, typename HttpResponseType>
  void
  CookieManager<HttpRequestType, HttpResponseType>::remove(
    HttpResponseType& response,
    const HttpRequestType& request,
    const HTTP::CookieList& input_cookies,
    const CookieNameSet& names) const
    /*throw(Exception)*/
  {
    static const char* FUN = "CookieManager::remove()";

    try
    {
      CookieHeadersWriter guard(response, response_headers_);

      for(HTTP::CookieList::const_iterator it = input_cookies.begin();
          it != input_cookies.end(); ++it)
      {
        CookieNameSet::const_iterator rm_it = names.find(it->name);
        if(rm_it != names.end())
        {
          remove_(response, request, *rm_it);
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't do clearing. Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename HttpRequestType, typename HttpResponseType>
  void
  CookieManager<HttpRequestType, HttpResponseType>::remove_(
    HttpResponseType& response,
    const HttpRequestType& request,
    const Generics::SubStringHashAdapter& name) const
    /*throw(Exception)*/
  {
    static const char* FUN = "CookieManager::remove_()";

    try
    {
      Generics::Time expired_time =
        Generics::Time::get_time_of_day() - Generics::Time::ONE_DAY * 365;

      typename RemoveCookieMap::const_iterator rm_cookies_it =
        remove_cookies_.find(name);

      if(rm_cookies_it != remove_cookies_.end())
      {
        for(auto rem_it = rm_cookies_it->second.begin();
          rem_it != rm_cookies_it->second.end(); ++rem_it)
        {
          std::string res_domain;
          get_cookie_domain_(request, *rem_it, res_domain);

          HTTP::CookieDef cookie(
            rm_cookies_it->first,
            String::SubString("na"),
            res_domain,
            rem_it->path,
            expired_time.get_gm_time(),
            false);
          std::string header;
          HTTP::cookie_header_plain(cookie, header);
          response.add_cookie(header.c_str());
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't do clearing. Caught eh::Exception: " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  template<typename HttpRequestType, typename HttpResponseType>
  void
  CookieManager<HttpRequestType, HttpResponseType>::get_cookie_domain_(
    const HttpRequestType& request,
    const CookiePath& cookie_params,
    std::string& res_domain) const
  {
    if(cookie_params.domain.empty())
    {
      res_domain = request.server_name().str();
    }
    else if(cookie_params.domain == "2")
    {
      // find second level domain
      String::SubString server_name(request.server_name());
      String::SubString::SizeType pos = server_name.rfind('.');
      if(pos == String::SubString::NPOS || pos == 0)
      {
        res_domain = server_name.str();
      }
      else
      {
        pos = server_name.rfind('.', pos - 1);
        if(pos == String::SubString::NPOS)
        {
          res_domain = server_name.str();
        }
        else
        {
          res_domain = server_name.substr(pos + 1).str();
        }
      }
    }
    else
    {
      res_domain = cookie_params.domain;
    }    
  }
}

#endif /*FRONTENDCOMMONS_COOKIEMANAGER_HPP*/
