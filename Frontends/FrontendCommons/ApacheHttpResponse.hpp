#ifndef FRONTENDCOMMONS_APACHEHTTPRESPONSE_HPP
#define FRONTENDCOMMONS_APACHEHTTPRESPONSE_HPP

#include <Apache/Adapters.hpp>

namespace FrontendCommons
{
  //
  // HttpResponse
  //
  class HttpResponse: public Apache::HttpResponse
  {
  public:
    HttpResponse(request_rec* r) noexcept;

    void
    add_cookie(const char* value)
      /*throw(eh::Exception)*/;

    bool
    cookie_installed() const noexcept;

  private:
    bool cookie_installed_;
  };
}

namespace FrontendCommons
{
  // HttpResponse
  inline
  HttpResponse::HttpResponse(request_rec* r) noexcept
    : Apache::HttpResponse(r),
      cookie_installed_(false)
  {}

  inline bool
  HttpResponse::cookie_installed() const noexcept
  {
    return cookie_installed_;
  }

  inline void
  HttpResponse::add_cookie(const char* value) /*throw(eh::Exception)*/
  {
    cookie_installed_ = true;
    Apache::HttpResponse::add_cookie(value);
  }
}

#endif /*FRONTENDCOMMONS_APACHEHTTPRESPONSE_HPP*/
