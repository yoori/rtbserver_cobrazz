#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{

  namespace
  {
    
    HTTP::HTTPAddress get_address(
      const std::string& base_url,
      const std::string& request_url)
    {
      if (strstr(request_url.c_str(), "://") != 0 ||
               base_url.empty())
      {
        HTTP::HTTPAddress request_addr(request_url);
        HTTP::HTTPAddress base_addr(base_url);
        return HTTP::HTTPAddress(
          base_addr.host(),
          request_addr.path(),
          request_addr.query(),
          request_addr.fragment(),
          base_addr.port_number(),
          request_addr.secure(),
          request_addr.userinfo());
      }
      else
      {
        return HTTP::HTTPAddress(base_url + request_url);
      }
    }
  }

  Logging::Logger_var BaseAdClient::NULL_LOGGER_(new Logging::Null::Logger());

  BaseAdClient::BaseAdClient(
    const char* base_url, 
    LoggerType log_type)
    /*throw(BaseAdClient::Exception, eh::Exception)*/:
    base_url_(base_url != 0 ? base_url : ""),
    cookies_(),
    time_(),
    request_(new ClientRequest()),
    stored_request_(new ClientRequest()),
    log_type_(log_type)
  {
  }
    
  BaseAdClient::BaseAdClient(const BaseAdClient& src)
    /*throw(BaseAdClient::Exception, eh::Exception)*/:
    base_url_(src.base_url_),
    cookies_(src.cookies_),
    time_(src.time_),
    request_(new ClientRequest(*(src.request_))),
    stored_request_(new ClientRequest(*(src.stored_request_))),
    log_type_(src.log_type_)
  {
  }
      
  BaseAdClient::~BaseAdClient()
    noexcept
  {
  }
       
  BaseAdClient& BaseAdClient::operator=
    (const BaseAdClient& client)
  {
    request_ = new ClientRequest(*(client.request_));
    base_url_ = client.base_url_;
    cookies_ = client.cookies_;
    time_ = client.time_;
    stored_request_ = new ClientRequest(*(client.stored_request_));
    return *this;
  }

  bool 
  BaseAdClient::find_header_value(const char* name,
                                  std::string& value) const
    /*throw(Exception)*/
  {
    try
    {
      bool found = false;
      HTTP::HeaderList& response_headers = 
        request_->response_headers();
      HTTP::HeaderList::iterator it = 
        response_headers.begin();
          
      for (; it != response_headers.end(); ++it)
      {          
        if (strcasecmp(it->name.c_str(), name) == 0)
        {
          value = it->value;
          found = true;
          break;
        }
      }
             
      return found;
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "BaseAdClient::find_header_value. eh::Exception exception caught. "
           << ": " << e.what();
        
      throw Exception(ostr);
    }
  }
    
  bool 
  BaseAdClient::process_request(const char* request_url,
                                HTTP::HTTP_Connection::HTTP_Method method)
    /*throw(BaseAdClient::Exception, InvalidArgument)*/
  {
    try
    {
      if (request_url == 0)
      {
        throw InvalidArgument("Got invalid parameter: request_url == 0");
      }

      HTTP::HTTPAddress url(get_address(base_url_, request_url));

      bool result;
      time_.set(0);
      std::string cookie = cookies_.cookie_header(url);

      // Store URL
      request_->address(url);

      if (logger().log_level() >= Logging::Logger::TRACE + 2)
      {
        std::ostringstream trace_str;
        trace_str << "Sending request:\n    " << url.url() << std::endl;
        for (HTTP::HeaderList::iterator i1 = 
               request_->request_headers().begin(); 
             i1 != request_->request_headers().end(); ++i1)
        {
          trace_str << "    " << i1->name << ": " << i1->value << std::endl;
        }
        trace_str << "    Active cookies: ";
        if (!cookie.empty())
        {
          trace_str << cookie;
        }
        trace_str << std::endl;
        if (method == HTTP::HTTP_Connection::HM_Post)
        {
          trace_str << "    Request body:";
          request_->print(trace_str, ClientRequest::Decoder::T_REQUEST);
        }

        logger().log(trace_str.str(), Logging::Logger::TRACE + 2);
      }
        
      if (!cookie.empty())
      {
        request_->request_headers().push_back(
          HTTP::Header("Cookie", cookie.c_str()));
      }

      try
      {
        stored_request_ = new ClientRequest(*request_);
        result = request_->send(method, true, time_);
      }
      catch (...)
      {
        cookies_.load_from_headers(request_->response_headers(), url);
        print_request_data(url.url());
        clear_request_data();

        throw;
      }

      cookies_.load_from_headers(request_->response_headers(), url);
      print_request_data(url.url());
      clear_request_data();
        
      return result;
    }
    catch(const InvalidArgument& e)
    {
      Stream::Error ostr;
      ostr << "BaseAdClient::process_request. InvalidArgument "
           << "exception exception caught. : " << e.what();
        
      throw InvalidArgument(ostr);
    }
    catch(const Exception& e)
    {
      Stream::Error ostr;
      ostr << "BaseAdClient::process_request. Exception exception caught. "
           << ": " << e.what();
        
      throw Exception(ostr);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "BaseAdClient::process_request. eh::Exception exception caught. "
           << ": " << e.what();
        
      throw Exception(ostr);
    }
  }
    
  void 
  BaseAdClient::print_request_data(const std::string url)
  {
    if (logger().log_level() >= Logging::Logger::TRACE + 2)
    {
      std::ostringstream trace_str;
        
      trace_str << "Got Response data. Status=" << request_->status() 
                << "\n\n    Response_headers:\n";
                  
      for (HTTP::HeaderList::iterator i1 = 
             request_->response_headers().begin(); 
           i1 != request_->response_headers().end(); ++i1)
      {
        trace_str << "        " << i1->name << "=" << i1->value << "\n";
      }
      trace_str << "\n\n";
        
      trace_str << "    Cookies for current url:\n" 
                << "        "
                << cookies_.cookie_header(HTTP::HTTPAddress(url))
                << "\n\n";
      trace_str << "    Body:";
      request_->print(trace_str, ClientRequest::Decoder::T_RESPONSE);
      logger().log(trace_str.str(), Logging::Logger::TRACE + 2);
    }
                
  }
    
  void 
  BaseAdClient::clear_request_data()
    /*throw(eh::Exception)*/
  {
    request_->request_headers().clear();
    request_->request_data().clear();
    request_->reset_decoder();
  }

  void
  BaseAdClient::change_base_url(const char* base_url)
  {
    base_url_ = base_url;
  }
      
  
  std::string
  BaseAdClient::get_domain_from_url(const std::string& url)
    /*throw(Exception)*/
  {
    try
    {
      std::string::size_type beg = 0;
      std::string nul_str("");

      if((beg = url.find("ww.")) != std::string::npos)
      {
        beg += 3;
      }
      else if ((beg = url.find("://")) != std::string::npos)
      {
        if ((beg = url.find('.', beg + 3)) == std::string::npos)
          return nul_str;
      }
      else
      {
        return nul_str;
      }
      std::string::size_type end = std::min(url.find('/', beg), url.find(':', beg));
      return (end != std::string::npos) ? url.substr(beg, end - beg) : url.substr(beg);
    }
    catch (eh::Exception& e)
    {
      Stream::Error error;
      error << "BaseAdClient::get_domain_from_url. eh::Exception caught. "
            << ": " << e.what();
      throw Exception(error);
    }
  }

  std::string 
  BaseAdClient::get_domain ()
    /*throw(Exception)*/
  {
    return get_domain_from_url(base_url_);
  }
    

  const std::string
  BaseAdClient::get_host_from_url(const std::string& url)
    /*throw(Exception)*/
  {
    try
    {
      std::string::size_type beg = 0;
      if ((beg = url.find("://")) != std::string::npos)
      {
        beg += 3;
      }
      std::string::size_type end = std::min(url.find('/', beg), url.find(':', beg));
      return end != std::string::npos? url.substr(beg, end - beg): url.substr(beg);
    }
    catch (eh::Exception& e)
    {
      Stream::Error error;
      error << "BaseAdClient::get_host_from_url. eh::Exception caught. "
            << ": " << e.what();
      throw Exception(error);
    }
  }

  const std::string 
  BaseAdClient::get_host ()
    /*throw(Exception)*/
  {
    return get_host_from_url(base_url_);
  }

  const std::string& 
  BaseAdClient::get_base_url () const
    /*throw(Exception)*/
  {
    return base_url_;
  }
    
  void 
  BaseAdClient::add_http_header(const std::string& header_name,
                                const std::string& header_val)
    /*throw(Exception)*/
  {
    try
    {
      request_->request_headers().push_back(HTTP::Header(header_name, 
                                                         header_val));        
    }      
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << "BaseAdClient::add_http_header. eh::Exception exception caught. "
           << ": " << e.what();
        
      throw Exception(ostr);
    }
  }

  Logging::Logger&
  BaseAdClient::logger() const
  {
    switch(log_type_)
    {
    case LT_BASE:
      return Logger::thlog();
    case LT_NULL:
      return *NULL_LOGGER_;
    }
    return Logger::thlog();
  }
} //namespace AutoTest
