#include <sstream>
#include <algorithm>
#include <HTTP/UrlAddress.hpp>
#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  namespace Cookie
  {

    static const unsigned long ONE_YEAR = 365 * 24 * 60 * 60;

    /**
     * @class CookieDomain
     */
    struct CookieIsNotDomain : std::binary_function <HTTP::PersistentCookieDef, std::string, bool>
    {
      bool
      operator() (
        const HTTP::PersistentCookieDef& cookie,
        const std::string& domain) const
      {
        return cookie.secure || domain != cookie.domain;
      }
    };

    /**
     * @class CookieName
     */
    struct CookieName : std::binary_function <HTTP::PersistentCookieDef, std::string, bool>
    {
      bool
      operator() (
        const HTTP::PersistentCookieDef& cookie,
        const std::string& name) const
      {
        return name == cookie.name; 
      }
    };

    // UnitCookieList
    
    UnitCookieList::~UnitCookieList() noexcept
    { }

    UnitCookieList::UnitCookieList()
      /*throw(eh::Exception)*/
    { }

    UnitCookieList::UnitCookieList(
      const UnitCookieList& src)
      /*throw(eh::Exception)*/
      : ClientCookieFacility(src)
    { }

    UnitCookieList::UnitCookieList(
      const UnitCookieList& src,
      const std::string& domain)
      /*throw(eh::Exception)*/ :
      ClientCookieFacility(src)
    {
      expire_();
      
      remove_if(
        std::bind2nd(
          CookieIsNotDomain(),
          domain));
    }

    bool
    UnitCookieList::find_value(
      const std::string& name,
      std::string& value) const
      /*throw(eh::Exception)*/
    {
      const_iterator it = std::find_if(
        begin(), end(),
        std::bind2nd(
          CookieName(), name));

      if (it == end()) return false;
      value = it->value;
      return true;
    }

    void
    UnitCookieList::set_value(
      const std::string& name,
      const std::string& value,
      const std::string& domain,
      const std::string& path,
      bool secure) /*throw(eh::Exception)*/
    {
      iterator it = std::find_if(
        begin(), end(),
        std::bind2nd(
          CookieName(), name));
      if ( it == end())
      {
        HTTP::PersistentCookieDef cookie(
          name, value, domain, path,
          Generics::Time::get_time_of_day() + ONE_YEAR,
          secure);
        push_back(cookie);
      }
      else
      {
        it->value = value;
      }
    }

    void
    UnitCookieList::remove_cookie(
      const std::string& name) /*throw(eh::Exception)*/
    {
      remove_if(
        std::bind2nd(
          CookieName(), name));
    }

    bool
    UnitCookieList::find_cookie(
      const std::string& name,
      HTTP::CookieDef& cookie)
      /*throw(eh::Exception)*/
    {
      iterator it = std::find_if(
        begin(), end(),
        std::bind2nd(
          CookieName(), name));

      if ( it  != end() )
      {
        cookie.operator=(*it);
      }

      return it != end();
    }

  } //namespace Cookie
} //namespace AutoTest

