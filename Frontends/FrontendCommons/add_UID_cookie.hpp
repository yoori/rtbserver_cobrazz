
#pragma once

#include "CookieManager.hpp"
#include "Cookies.hpp"

namespace FrontendCommons
{
  template<typename HttpRequestType, typename HttpResponseType>
  inline void
  add_UID_cookie(
    HttpResponseType& response,
    const HttpRequestType& request,
    const FrontendCommons::CookieManager<
      HttpRequestType, HttpResponseType>& cookie_manager,
    const std::string& signed_client_id)
    /*throw(eh::Exception)*/
  {
    if(!signed_client_id.empty())
    {
      /* renew expiration time for uid cookie */
      if(request.secure())
      {
        // don't set cookie on non secure connection - this can override
        // existing cookie and nullify it
        cookie_manager.set(
          response,
          request,
          FrontendCommons::Cookies::CLIENT_ID,
          signed_client_id);
      }

      // try set cookie in classic way
      /*
      cookie_manager.set(
        response,
        request,
        FrontendCommons::Cookies::CLIENT_ID2,
        signed_client_id,
        Generics::Time::ZERO,
        false);
      */
    }
  }

}
