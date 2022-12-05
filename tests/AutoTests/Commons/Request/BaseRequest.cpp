
#include "BaseRequest.hpp"
#include <String/AsciiStringManip.hpp>

#include <utility>
#include <functional>
#include <algorithm>

namespace AutoTest
{
  // Utils
  static const char ENCODED_SEPARATOR[] = "?";
  static const char ENCODED_PREFIX[] = "&";
  static const char ENCODED_EQUAL[] = "=";
  static const char NOT_ENCODED_SEPARATOR[] = "";
  static const char NOT_ENCODED_PREFIX[] = "*amp*";
  static const char NOT_ENCODED_EQUAL[] = "*eql*";

  // OldTime
  const char*
  OldTime::format()
  {
    return DEBUG_TIME_FORMAT;
  }

  // NewTime
  const char*
  NewTime::format()
  {
    return DEBUG_TIME_NEW_FORMAT;
  }

  bool
  EqualHeaderName::operator() (
    const HTTP::Header& header,
    const std::string& name) const
  {
    return header.name == name;
  }

  // class Time
  Time::Time() noexcept :
    Generics::Time(Generics::Time::get_time_of_day())
  { }
  
  Time::Time(const timeval& time) noexcept :
    Generics::Time(time)
  { }
  
  Time::Time(
    time_t time_sec,
    suseconds_t usec) noexcept :
    Generics::Time(time_sec, usec)
  { }
  
  Time::Time(
    const std::string& value)
    /*throw(InvalidArgument, Exception, eh::Exception)*/ :
    Generics::Time(String::SubString(value), DEBUG_TIME_FORMAT)
  { }
  
  Time::Time(
    const char* value)
    /*throw(InvalidArgument, Exception, eh::Exception)*/ :
    Generics::Time(String::SubString(value), DEBUG_TIME_FORMAT)
  { }
  
  Time::Time(
    const String::SubString& value)
    /*throw(InvalidArgument, Exception, eh::Exception)*/ :
    Generics::Time(value, DEBUG_TIME_FORMAT)
  { }
    
  Time::Time(
    const Time& t) noexcept :
    Generics::Time(t)
  { }

  Time::Time(
    const Generics::Time& t) noexcept :
    Generics::Time(t)
  { }

  Time&
  Time::operator= (
    const Generics::Time& t)
  {
    Generics::Time::operator=(t);
    return *this;
  }

  Time&
  Time::operator= (
    const Time& t)
  {
    Generics::Time::operator=(t);
    return *this;
  }

  Time&
  Time::operator++ ()
  {
    operator+=(1);
    return *this;
  }

  Time
  Time::operator++ ( int )
  {
    Time ret(*this);
    operator+=(1);
    return ret;
  }

  Time& Time::operator-- ()
  {
    operator-=(1);
    return *this;
  }

  Time Time::operator-- ( int )
  {
    Time ret(*this);
    operator-=(1);
    return ret;
  }

  std::ostream&
  operator <<(
    std::ostream& ostr,
    const Time& time)
    /*throw(eh::Exception)*/
  {
    ostr << time.get_gm_time().format(DEBUG_TIME_FORMAT);
    return ostr;
  }

  // class BaseParamsContainer

  BaseParamsContainer::BaseParamsContainer() :
    params_(0)
  { }
  
  BaseParamsContainer::~BaseParamsContainer() noexcept
  { }

  void BaseParamsContainer::add_param(BaseParam* param)
  {
    params_.push_back(param);
  }

  // class BaseRequest

  BaseRequest::BaseRequest(
    const char* base_url,
    RequestType req_type) :
    url_(base_url),
    req_type_(req_type)
  { }

  BaseRequest::~BaseRequest() noexcept
  { }
  
  std::ostream& BaseRequest::print (std::ostream& out) const
  {
    out << url_;
    return print_params_(out);
  }

  std::string BaseRequest::url() const
  {
    std::ostringstream ostr;
    print(ostr);
    return ostr.str();
  }

  void BaseRequest::decode_(const char* _url)
    /*throw(HTTP::URLAddress::InvalidURL, eh::Exception)*/
  {
    HTTP::BrowserAddress url{String::SubString(_url)};
    const String::SubString& path = url.path();
    const String::SubString& query = url.query().empty()? path: url.query();
    const String::SubString prefix(params_prefix());
    if (path.find(url_) == std::string::npos)
    {
      Stream::Error error;
      error << "'" << _url << "' contain incorect path '"
            << path << "', expected path '" << url_ << "'";
      throw HTTP::URLAddress::InvalidURL(error);
    }
    for(auto i = params_.begin(); i != params_.end(); i++)
    {
      std::size_t pos = query.find((*i)->name_ + eql());
      if (pos != std::string::npos)
      {
        std::size_t value_start_pos = query.find(eql(),pos);
        value_start_pos+=strlen(eql());
        std::size_t value_end_pos = query.find(prefix, pos);
        std::size_t value_length = value_end_pos - value_start_pos;
        (*i)->set_param_val(query.substr(value_start_pos, value_length));
      }
    }
  }

  std::ostream& BaseRequest::print_params_(std::ostream& out) const
  {
    const char *param_prefix = params_prefix(true);
    for(auto i = params_.begin(); i != params_.end(); i++)
    {
      if (!(*i)->empty() &&
        (*i)->print(out, param_prefix, eql()))
      {
        param_prefix = params_prefix();
      }
    }
    return out;
  }

  const HTTP::HeaderList&
  BaseRequest::headers() const
  {
    return headers_;
  }

  HTTP::HeaderList&
  BaseRequest::headers()
  {
    return headers_;
  }

  std::string
  BaseRequest::body() const
  {
    return std::string();
  }

  const char*
  BaseRequest::params_prefix(bool is_first) const
  {
    if (req_type_ == BaseRequest::RT_NOT_ENCODED)
    {
      return is_first?
        NOT_ENCODED_SEPARATOR: NOT_ENCODED_PREFIX;
    }
    return is_first?
      ENCODED_SEPARATOR: ENCODED_PREFIX;
  }

  
  const char* BaseRequest::eql() const
  {
    return req_type_ == BaseRequest::RT_NOT_ENCODED?
      NOT_ENCODED_EQUAL: ENCODED_EQUAL;
  }

  bool
  BaseRequest::need_encode() const
  {
    return req_type_ == RT_ENCODED;
  }

  void
  BaseRequest::set_decoder(
    ClientRequest* request) const
  {
    request->reset_decoder();
  }

  // class BaseParam

  BaseParam::~BaseParam() noexcept
  { }
  
  bool
  BaseParam::print(
    std::ostream& out,
    const char* prefix,
    const char* eql) const
  {
    out << prefix << name_  << eql << str();
    return true;
  }

  // StringParam
  StringParam::~StringParam() noexcept
  { }

  void
  StringParam::clear()
  {
    param_value_.clear();
    empty_ = true;
  }

  bool
  StringParam::empty () const
  {
    return empty_;
  }

  void
  StringParam::not_encode ()
  {
    need_encode_ = false;
  }

  std::string
  StringParam::str() const
  {
    if (request_->need_encode() && need_encode_)
    {
      std::string ret;
      String::StringManip::mime_url_encode(param_value_, ret);
      return ret;
    }
    return param_value_;
  }

  std::string
  StringParam::raw_str() const
  {
    return param_value_;
  }

  void
  StringParam::set_param_val(
    const String::SubString& val)
  {
    param_value_ = val.str();
    empty_ = false;
  }

  void
  StringParam::set_param_val(
    const std::string& val)
  {
    set_param_val(String::SubString(val));
  }

  void
  StringParam::set_param_val(
    const char* val)
  {
    if (val)
    {
      set_param_val(String::SubString(val, strlen(val)));
    }
    else
    {
      clear();
    }
  }

  // SearchParam

  const char* SearchParam::SEARCH_URL = "http://www.google.ru/search?hl=ru&q=";

  SearchParam::~SearchParam() noexcept
  { }

  void
  SearchParam::set_param_val(
    const String::SubString& val)
  {
    if (!val.empty())
    {
      param_value_ = SEARCH_URL + val.str();
    }
  }

  std::string
  SearchParam::raw_str() const
  {
    std::string v(param_value_);
    return v.erase(0, strlen(SEARCH_URL));
  }

  // RequestMemberBase
  RequestMemberBase::~RequestMemberBase() noexcept
  { }

  // Constants
  const char* DEBUG_TIME_FORMAT = "%d-%m-%Y:%H-%M-%S";    // 'debug-time' format
  const char* DEBUG_TIME_NEW_FORMAT = "%F %T";  // 'debug-time' new format
  const char* DEFAULT_USER_AGENT =
    "Mozilla/5.0 (Windows NT 6.0; rv:19.0) Gecko/20100101 Firefox/19.0";
}
