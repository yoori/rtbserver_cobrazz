
#include "Request.hpp"
#include <tests/AutoTests/Commons/Checkers/ServerResponseCheckers/OpenRTBResponseChecker.hpp>
#include <Commons/Gason.hpp>

namespace
{
  class EmptyResponse : public ResponseInformation
  {
  public:

    EmptyResponse(HttpMethod method, const char* request) noexcept;

    ~EmptyResponse() noexcept;

    virtual HttpMethod method() const noexcept;

    virtual const char*
    http_request() const noexcept;

    virtual const HeaderList&
    headers() const noexcept;

    virtual int
    response_code() const noexcept;

    virtual const HeaderList&
    response_headers() const noexcept;

    virtual String::SubString
    body() const noexcept;

  private:
    HttpMethod method_;
    const char* request_;
    HeaderList headers_;
  };
}

// EmptyResponse class

EmptyResponse::EmptyResponse(
  HttpMethod method,
  const char* request) noexcept :
  method_(method),
  request_(request)
{ }

EmptyResponse::~EmptyResponse() noexcept
{ }

HttpMethod
EmptyResponse::method() const noexcept
{
  return method_;
}

const char*
EmptyResponse::http_request() const noexcept
{
  return  request_;
}

const HeaderList&
EmptyResponse::headers() const noexcept
{
  return headers_;
}

int
EmptyResponse::response_code() const noexcept
{
  return 0;
}

const HeaderList&
EmptyResponse::response_headers() const noexcept
{
  return headers_;
}

String::SubString
EmptyResponse::body() const noexcept
{
  return String::SubString();
}

// BaseRequest class

BaseRequest::BaseRequest(
  QuerySenderBase* owner,
  unsigned long client_id,
  bool optout,
  const RequestConfig_var& config,
  const char* url) :
  request_handled_(false),
  owner_(owner),
  client_id_(client_id),
  optout_(optout),
  config_(config),
  url_(url)
{}

BaseRequest::~BaseRequest() noexcept
{}


void
BaseRequest::on_response(
  const ResponseInformation& data) noexcept
{
  if (!request_handled_)
  {
    request_handled_ = true;
    if (_check_response_code(data.response_code()))
    {
      _on_response(data);
    }
    else
    {
      Stream::Error err;
      err << "error: Got invalid response status " << data.response_code();
      err << this << std::endl;
      owner_->on_error(err.str(), data, optout_);
    }
  }
}

void
BaseRequest::on_error(
  const String::SubString& description,
  const RequestInformation& data) noexcept
{
  if (!request_handled_)
  {
    request_handled_ = true;
    Stream::Error err;
    err << "error: " << description << std::endl;;
    err << this << std::endl;
    owner_->on_error(
      err.str(),
      EmptyResponse(
        data.method(),
        data.http_request()),
      optout_);
  }
}

const std::string&
BaseRequest::url()
{
  return _url();
}

const std::string&
BaseRequest::body()
{
  return _body();
}

const std::string&
BaseRequest::_body()
{
  return body_;
}

void
BaseRequest::headers(HeaderList& headers) const
{
  SelectorPolicyList::const_iterator it_header =
    config_->headers().begin();
  for(; it_header != config_->headers().end(); ++it_header)
  {
    std::string header_value;
    (*it_header)->get(header_value, PO_NO_NEED_ENCODE);
    headers.push_back(
      Header(
        (*it_header)->entity_name,
        header_value.c_str()));
  }
}

bool
BaseRequest::_check_response_code(
  unsigned long response_code)
{
  if (response_code < 200 || response_code >= 400)
  {
    return false;
  }
  return true;
}

bool
BaseRequest::isGet() const
{
  return config_->method.empty() ||
    config_->method == "get";
}

unsigned long
BaseRequest::client_id() const
{
  return client_id_;
}

bool
BaseRequest::optout() const
{
  return optout_;
}

std::ostream&
BaseRequest::dump(
  std::ostream& out) const
{
  if (optout_)
  {
    out << "OO Client";
  }
  {
    out << "Client#" << client_id_ << " ";
  }
  if (isGet())
  {
    out << " GET: " << url_ << std::endl;
  }
  else
  {
    out << " POST: " << url_ << std::endl;
  }
  HeaderList headers_;
  this->headers(headers_);
  HeaderList::iterator h_it(headers_.begin());
  out << "  Headers: " << std::endl;
  for (; h_it != headers_.end(); ++h_it)
  {
    out << "    " << h_it->name << "=" << h_it->value << std::endl;
  }
  out << "  Request Body: " << std::endl << body_ << std::endl;
  return out;
}

// ParamsRequest class

ParamsRequest::ParamsRequest(
  QuerySenderBase* owner,
  unsigned long client_id,
  bool optout,
  const RequestConfig_var& config,
  const char* server,
  bool cfg_ad_all_optout) :
  BaseRequest(
    owner, client_id,
    optout, config),
  server_(server),
  cfg_ad_all_optout_(cfg_ad_all_optout)
{}

ParamsRequest::~ParamsRequest() noexcept
{}

const std::string&
ParamsRequest::_url(
  bool generate)
{
  if (!generate) return url_;
  url_ = server_ + config_->url;
  SelectorPolicyList::const_iterator it_param =
    config_->parameters().begin();
  unsigned int param_count = 0;
  for(; it_param != config_->parameters().end(); ++it_param)
  {
    std::string param_value;
    unsigned short flags = 0;
    if (optout_ && cfg_ad_all_optout_)
    {
      flags |= PO_ALWAYS_SET;
    }
    (*it_param)->get(param_value, flags);
    // Don't use 'setuid' parameter for optout users
    if (optout_ && (*it_param)->entity_name == "setuid")
    {
      continue;
    }
    if (!param_value.empty())
    {
      if (!param_count)
      {
        url_+= ("?" + ((*it_param)->entity_name)) +
          ("=" + param_value);
      }
      else
      {
        url_+= ("&" + ((*it_param)->entity_name)) +
          ("=" + param_value);
      }
      param_count++;
    }
  }
  return url_;
}

void
ParamsRequest::_on_response(
  const ResponseInformation& data)
{
  HTTP::HeaderList headers;
  data.find_headers("Debug-Info", headers);
  if (!headers.empty())
  {
    std::string debug_info_string(headers.front().value);
    debug_info_.parse(debug_info_string);
    try
    {
      _process_response(data);
    }
    catch (eh::Exception& e)
    {
      owner_->on_error(String::SubString(e.what()), data, optout_);
    }
    return;
  }
  owner_->on_response(client_id_,
                      data,
                      optout_);
}

// NSLookup request

NSLookupRequest::NSLookupRequest(
  QuerySenderBase* owner,
  unsigned long client_id,
  bool optout,
  const RequestConfig_var& config,
  const char* server,
  bool cfg_ad_all_optout) :
  ParamsRequest(
    owner, client_id, optout,
    config, server, cfg_ad_all_optout)
{ }

NSLookupRequest::~NSLookupRequest() noexcept
{ }

void
NSLookupRequest::_process_response(
  const ResponseInformation& data)
{
  unsigned long ccid = atoi(debug_info_.ccid.value().c_str());
  HTTP::HeaderList location;
  data.find_headers("Location", location);
  std::string action_url("");
  std::string passback_url("");
  if(!debug_info_.selected_creatives.empty())
  {
    action_url = debug_info_.selected_creatives.first().action_adv_url.value();
  }
  if (!location.empty())
  {
    passback_url = location.front().value;
  }

  AdvertiserResponse_var ad_response =
    AdvertiserResponse_var(
      new AdvertiserResponse(
        ccid,
        debug_info_.click_url.value().c_str(),
        action_url.c_str(),
        passback_url.c_str(),
        debug_info_.trigger_channels.size() != 0?
          debug_info_.trigger_channels.size():
            atoi(debug_info_.trigger_channels_count.value().c_str()),
        debug_info_.history_channels.size() != 0?
          debug_info_.history_channels.size():
            atoi(debug_info_.history_channels_count.value().c_str()),
        debug_info_.selected_creatives.size() != 0 ?
          debug_info_.selected_creatives.size():
            atoi(debug_info_.ccids.value().c_str()),
        debug_info_.trigger_match_time.value().c_str(),
        debug_info_.request_fill_time.value().c_str(),
        debug_info_.history_match_time.value().c_str(),
        debug_info_.creative_selection_time.value().c_str(),
        optout_));
  owner_->on_response(
    client_id_,
    data,
    optout_,
    ccid,
    ad_response.get());
}

// SimpleRequest class

SimpleRequest::SimpleRequest(
  QuerySenderBase* owner,
  unsigned long client_id,
  bool optout,
  const RequestConfig_var& config,
  const char* url) :
  BaseRequest(
    owner, client_id, optout, config, url)
{ }

SimpleRequest::~SimpleRequest() noexcept
{ }

const std::string&
SimpleRequest::_url(bool)
{
  return url_;
}

void
SimpleRequest::_on_response(
  const ResponseInformation& data)
{
  String::SubString r(data.http_request());
  String::RegEx::Result result;
  unsigned long ccid  = 0;
  String::RegEx re(String::SubString("ccid\\*eql\\*([\\d]+)"));
  if (re.search(result, r))
  {
    if (result.size() == 2)
    {
      if (!String::StringManip::str_to_int(result[1], ccid))
      {
        ccid = 0;
      }
    }
  }
  else
  {
    String::RegEx re(String::SubString("&ccid=([\\d]+)"));
    if (re.search(result, r))
    {
      if (result.size() == 2)
      {
        if (!String::StringManip::str_to_int(result[1], ccid))
        {
          ccid = 0;
        }
      }
    }
  }
  owner_->on_response(client_id_,
                      data,
                      optout_, ccid);
}


// ClickRequest class

ClickRequest::ClickRequest(
  QuerySenderBase* owner,
  unsigned long client_id,
  bool optout,
  const RequestConfig_var& config,
  const char* url) :
  SimpleRequest(
    owner, client_id, optout,
    config, url)
{ }

ClickRequest::~ClickRequest() noexcept
{ }

bool
ClickRequest::_check_response_code(
  unsigned long response_code)
{
  if (response_code != 200 && response_code != 302)
  {
    return false;
  }
  return true;
}

// ActionRequest class

ActionRequest::ActionRequest(
  QuerySenderBase* owner,
  unsigned long client_id,
  bool optout,
  const RequestConfig_var& config,
  const char* url) :
  SimpleRequest(
    owner, client_id,
    optout, config, url)
{ }

ActionRequest::~ActionRequest() noexcept
{ }

bool
ActionRequest::_check_response_code(
  unsigned long response_code)
{
  if (response_code != 200)
  {
    return false;
  }
  return true;
}

// PassbackRequest class

PassbackRequest::PassbackRequest(
  QuerySenderBase* owner,
  unsigned long client_id,
  bool optout,
  const RequestConfig_var& config,
  const char* url) :
  SimpleRequest(
    owner, client_id,
    optout, config, url)
{ }

PassbackRequest::~PassbackRequest() noexcept
{ }

bool
PassbackRequest::_check_response_code(
  unsigned long response_code)
{
  if (response_code != 200 && response_code != 302)
  {
    return false;
  }
  return true;
}

// UserBindRequest

UserBindRequest::UserBindRequest(
  QuerySenderBase* owner,
  unsigned long client_id,
  const std::string& ssp_user_id,
  const RequestConfig_var& config,
  const char* server) :
  BaseRequest(
    owner,client_id, false, config),
  server_(server),
  ssp_user_id_(ssp_user_id)
{ }

UserBindRequest::~UserBindRequest() noexcept
{ }

const std::string&
UserBindRequest::_url(bool)
{
  std::ostringstream url_stream;
  url_stream << server_ << config_->url;
  SelectorPolicyList::const_iterator it_param =
    config_->parameters().begin();
  unsigned int param_count = 0;
  for(; it_param != config_->parameters().end(); ++it_param)
  {
    std::string param_value;
    (*it_param)->get(param_value, PO_ALWAYS_SET);
    // Don't set ssp_user_id
    if ((*it_param)->entity_name == "ssp_user_id")
    {
      continue;
    }
    if (!param_value.empty())
    {
      url_stream << (param_count++? '&': '?') <<
        (*it_param)->entity_name << "=" << param_value;
    }
  }
  url_stream << (param_count? '&': '?') <<
     "ssp_user_id=" << ssp_user_id_;
  url_ = url_stream.str();
  return url_;
}

void
UserBindRequest::_on_response(
  const ResponseInformation& data)
{
  owner_->on_response(
    client_id_, data, false, 0);
}

// OpenRTBRequest

const OpenRTBRequest::OpenRTBParam
OpenRTBRequest::PARAMS[OpenRTBRequest::PARAM_COUNT] =
{
  {"aid", &AutoTest::OpenRTBRequest::aid},
  {"debug.ccg", &AutoTest::OpenRTBRequest::debug_ccg},
  {"debug.size", &AutoTest::OpenRTBRequest::debug_size},
  {"height", &AutoTest::OpenRTBRequest::height},
  {"id", &AutoTest::OpenRTBRequest::id},
  {"ip", &AutoTest::OpenRTBRequest::ip},
  {"min_cpm_price", &AutoTest::OpenRTBRequest::min_cpm_price},
  {"min_cpm_price_currency_code", &AutoTest::OpenRTBRequest::min_cpm_price_currency_code},
  {"pos", &AutoTest::OpenRTBRequest::pos},
  {"referer", &AutoTest::OpenRTBRequest::referer},
  {"request_id", &AutoTest::OpenRTBRequest::request_id},
  {"src", &AutoTest::OpenRTBRequest::src},
  {"user_agent", &AutoTest::OpenRTBRequest::user_agent},
  {"user_id", &AutoTest::OpenRTBRequest::user_id},
  {"width", &AutoTest::OpenRTBRequest::width}
};

OpenRTBRequest::OpenRTBRequest(
  QuerySenderBase* owner,
  unsigned long client_id,
  const std::string& ssp_user_id,
  const RequestConfig_var& config,
  const char* server) :
  BaseRequest(owner, client_id, false, config),
  server_(server),
  ssp_user_id_(ssp_user_id)
{ }

OpenRTBRequest::~OpenRTBRequest() noexcept
{ }

const std::string&
OpenRTBRequest::_url(bool)
{
  _init();
  return url_;
}

const std::string&
OpenRTBRequest::_body()
{
  _init();
  return body_;
}

void
OpenRTBRequest::_on_response(
  const ResponseInformation& data)
{
  try
  {
    std::string body(data.body().str());
    AutoTest::OpenRTBResponse openrtb_response(body);

    if (openrtb_response.status() == JSON_PARSE_OK)
    {
      unsigned long ccid = openrtb_response.bids().size()?
        openrtb_response.bids().front().cid: 0;

      (void)ccid;

      AdvertiserResponse_var ad_response =
        AdvertiserResponse_var(
          new AdvertiserResponse(
            ccid,
            "",
            "",
            "",
            0,
            0,
            openrtb_response.bids().size(),
            "0:000000 (sec:usec)",
            "0:000000 (sec:usec)",
            "0:000000 (sec:usec)",
            "0:000000 (sec:usec)",
            false));

      owner_->on_response(
        client_id_,
        data,
        false,
        ccid,
        ad_response.get());

      return;

    }
    else
    {
      Stream::Error err;
      err << "error: OpenRTB response parser status: " << openrtb_response.status();
      err << this << std::endl;
      owner_->on_error(err.str(), data, false);
    }
  }
  catch  (eh::Exception& e)
  {
    owner_->on_error(String::SubString(e.what()), data, false);
  }

  owner_->on_response(client_id_, data, false);
}

void
OpenRTBRequest::_init()
{
  AutoTest::OpenRTBRequest request;

  SelectorPolicyList::const_iterator it_param =
    config_->parameters().begin();
  for(; it_param != config_->parameters().end(); ++it_param)
  {
    int param_idx = _get_param_idx(
      (*it_param)->entity_name);

    if (param_idx >= 0)
    {
      std::string param_value;
      (*it_param)->get(param_value, PO_ALWAYS_SET);
      if (!param_value.empty())
      {
        (PARAMS[param_idx].param)(request, param_value);
      }
    }
  }

  request.external_user_id = ssp_user_id_;

  url_ = server_ + request.url();
  body_ = request.body();
}

int
OpenRTBRequest::_get_param_idx(
  const std::string& name)
{
  for (size_t i = 0; i < PARAM_COUNT; ++i)
  {
    if (strcmp(name.c_str(), PARAMS[i].name) == 0)
    {
      return i;
    }
  }
  return -1;
}

