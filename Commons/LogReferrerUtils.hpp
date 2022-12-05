#ifndef _COMMONS_LOGREFERRERUTILS_HPP__
#define _COMMONS_LOGREFERRERUTILS_HPP__

#include <string>

#include <String/SubString.hpp>
#include <HTTP/UrlAddress.hpp>

namespace AdServer
{
namespace Commons
{
namespace LogReferrer
{
  enum Setting
  {
    LR_EMPTY,
    LR_HOST,
    LR_PATH
  };

  inline
  Setting
  read_log_referrer_settings(const String::SubString& conf_data) noexcept
  {
    return (conf_data == "empty" ? LR_EMPTY : (
      conf_data == "domain" ? LR_HOST : LR_PATH));
  }

  inline
  std::string
  normalize_referrer(
    const String::SubString& in,
    Setting lr_setting,
    const std::string& empty_ref = "",
    bool truncate = true)
  {
    std::string res_referrer;

    if(lr_setting != LR_EMPTY && !in.empty())
    {
      try
      {
        HTTP::BrowserAddress addr(in);

        if(lr_setting == LR_PATH)
        {
          addr.get_view(
            HTTP::HTTPAddress::VW_PROTOCOL |
              HTTP::HTTPAddress::VW_HOSTNAME |
              HTTP::HTTPAddress::VW_NDEF_PORT |
              HTTP::HTTPAddress::VW_PATH,
            res_referrer);
        }
        else if(lr_setting == LR_HOST)
        {
          addr.get_view(
            HTTP::HTTPAddress::VW_PROTOCOL |
              HTTP::HTTPAddress::VW_HOSTNAME,
            res_referrer);
        }
      }
      catch(const eh::Exception&)
      {}
    }

    if(res_referrer.empty())
    {
      return empty_ref;
    }

    // truncate path for DB
    if(truncate && res_referrer.size() > 2048)
    {
      res_referrer.resize(2048);
    }

    return res_referrer;
  }
}
}
}
#endif //_COMMONS_LOGREFERRERUTILS_HPP__

