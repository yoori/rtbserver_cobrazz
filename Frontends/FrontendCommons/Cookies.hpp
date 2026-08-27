#pragma once

#include <String/SubString.hpp>

namespace FrontendCommons::Cookies
{
  const String::SubString OPTIN("OPTED_IN");
  const String::SubString OPTOUT("OPTED_OUT");
  const String::SubString CLIENT_ID("uid");
  const String::SubString CLIENT_ID2("uid2"); // only read, don't set
  const String::SubString OPTOUT_TRUE_VALUE("YES");
  const String::SubString SET_UID("setuid");
}
