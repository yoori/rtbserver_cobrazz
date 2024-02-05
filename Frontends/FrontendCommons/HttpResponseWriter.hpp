#ifndef FRONTENDCOMMONS_HTTPRESPONSEWRITER
#define FRONTENDCOMMONS_HTTPRESPONSEWRITER

// UNIXCOMMONS
#include <ReferenceCounting/AtomicImpl.hpp>

// THIS
#include <Frontends/FrontendCommons/HttpResponse.hpp>

namespace FrontendCommons
{

class HttpResponseWriter: public virtual ReferenceCounting::AtomicImpl
{
public:
  HttpResponseWriter() = default;

  virtual ~HttpResponseWriter() = default;

  virtual void write(int code, FrontendCommons::HttpResponse* response) = 0;
};

using HttpResponseWriter_var = ReferenceCounting::SmartPtr<HttpResponseWriter>;

} // namespace FrontendCommons

#endif //FRONTENDCOMMONS_HTTPRESPONSEWRITER
