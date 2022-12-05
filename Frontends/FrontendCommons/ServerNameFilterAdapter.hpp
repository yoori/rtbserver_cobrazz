#ifndef VIRTUALSERVERADAPTER_HPP
#define VIRTUALSERVERADAPTER_HPP

#include <Apache/Adapters.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Commons/Algs.hpp>

namespace Apache
{
  /* apache module wrapper for enable module at VirtualHost level */
  template <typename ModuleType, typename HttpResponse = Apache::HttpResponse>
  class ServerNameFilterAdapter:
    public QuickHandlerAdapter<ModuleType, HttpResponse>
  {
  public:
    ServerNameFilterAdapter(int where = APR_HOOK_LAST) noexcept;

    void add_module_server(const server_rec* server) noexcept;

    bool module_used() const noexcept;

    // filter requests by hosts
    virtual int quick_handler(request_rec* r, int) noexcept;

    virtual bool
    will_handle(const char* uri) noexcept = 0;

  protected:
    virtual ~ServerNameFilterAdapter() noexcept
    {}

  private:
    typedef Generics::GnuHashSet<
      Algs::ConstPointerHashAdapter<server_rec> >
      ServerSet;

  private:
    ServerSet hosts_;
  };
}

namespace Apache
{
  template<typename ModuleType, typename HttpResponse>
  ServerNameFilterAdapter<ModuleType, HttpResponse>::
    ServerNameFilterAdapter(int where)
    noexcept
    : QuickHandlerAdapter<ModuleType, HttpResponse>(where)
  {}

  template<typename ModuleType, typename HttpResponse>
  void
  ServerNameFilterAdapter<ModuleType, HttpResponse>::
    add_module_server(const server_rec* server)
    noexcept
  {
    hosts_.insert(server);
  }

  template<typename ModuleType, typename HttpResponse>
  bool
  ServerNameFilterAdapter<ModuleType, HttpResponse>::module_used() const
    noexcept
  {
    return !hosts_.empty();
  }

  template<typename ModuleType, typename HttpResponse>
  int
  ServerNameFilterAdapter<ModuleType, HttpResponse>::
    quick_handler(request_rec* r, int)
    noexcept
  {
    switch (r->method_number)
    {
    case M_GET:
    case M_POST:
    case M_PUT:
      break;

    default:
      return DECLINED;
    }

    if(hosts_.find(r->server) == hosts_.end() ||
       !will_handle(r->uri))
    {
      return DECLINED;
    }

    try
    {
      HttpRequest request(r);
      HttpResponse response(r);

      // handle request
      return this->handle_request_noparams(request, response);
    }
    catch (const HttpRequest::Exception& e)
    {
      ap_log_error(APLOG_MARK, APLOG_WARNING, 0, r->server, e.what());

      return e.error_code();
    }
    catch (const eh::Exception& e)
    {
      ap_log_error(APLOG_MARK, APLOG_WARNING, 0, r->server, e.what());

      return HTTP_INTERNAL_SERVER_ERROR;
    }
  }
}

#endif /*VIRTUALSERVERADAPTER_HPP*/
