#ifndef FRONTENDCOMMONS_COOKIES_HPP
#define FRONTENDCOMMONS_COOKIES_HPP

#include <String/SubString.hpp>

namespace FrontendCommons
{
  namespace Cookies
  {
    const String::SubString OPTIN("OPTED_IN");
    const String::SubString OPTOUT("OPTED_OUT");
    const String::SubString CLIENT_ID("uid");
    const String::SubString CLIENT_ID2("uid2"); // only read, don't set
    const String::SubString OPTOUT_TRUE_VALUE("YES");
    const String::SubString SET_UID("setuid");
  }
}

#endif /*FRONTENDCOMMONS_COOKIES_HPP*/
