// UNIXCOMMONS
#include <String/StringManip.hpp>

// THIS
#include "FCGI.hpp"

namespace FCGI
{

namespace
{
const std::string REQUEST_URI("REQUEST_URI");
const std::string REQUEST_METHOD("REQUEST_METHOD");
const std::string QUERY_STRING("QUERY_STRING");
const std::string HTTPS("HTTPS");
const std::string REMOTE_ADDR("REMOTE_ADDR");

const std::string GET("GET");
const std::string POST("POST");
const std::string HEAD("HEAD");

const std::string PASS_REMOTE_ADDR(".remotehost");
const std::string PASS_SECURE("secure");
const std::string PASS_SECURE_VALUE("1");

const String::SubString HEADER_SEP(": ");
const String::SubString CONTENT_TYPE_AND_SEP("Content-Type: ");
const String::SubString SET_COOKIE_AND_SEP("Set-Cookie: ");
const String::SubString STATUS_AND_SEP("Status: ");
const String::SubString CONTENT_LENGTH_AND_SEP("Content-Length: ");

const std::string CRLF("\r\n");
const std::string STATUS_200("OK");
const std::string STATUS_204("No Content");
const std::string STATUS_301("Moved Permanently");
const std::string STATUS_302("Found");
const std::string STATUS_303("See Other");
const std::string STATUS_307("Temporary Redirect");
const std::string STATUS_400("Bad Request");
const std::string STATUS_403("Forbidden");
const std::string STATUS_404("Not Found");
const std::string STATUS_500("Internal Server Error");
const size_t STATUS_MSG_SIZE = 256;
} // namespace

HttpResponseFCGI::HttpResponseFCGI(uint16_t id) noexcept
  : status_msg_(id, wbuf_, STATUS_MSG_SIZE),
    headers_msg_(id, wbuf_ + STATUS_MSG_SIZE, sizeof(wbuf_) / 2 - STATUS_MSG_SIZE),
    end_msg_(id, wbuf_ + sizeof(wbuf_) / 2, sizeof(wbuf_) / 2),
    output_stream_(this),
    body_size_(0),
    cookie_installed_(false)
{}

void HttpResponseFCGI::add_header(
  const String::SubString& name,
  const String::SubString& value)
{
  headers_msg_.append(FCGI_STDOUT, String::SubString(name))
    .append(FCGI_STDOUT, HEADER_SEP)
    .append(FCGI_STDOUT, value)
    .append(FCGI_STDOUT, CRLF);
}

void HttpResponseFCGI::add_header(
  std::string&& name,
  std::string&& value)
{
  add_header(
    String::SubString(name.data(), name.size()),
    String::SubString(value.data(), value.size()));
}

void HttpResponseFCGI::set_content_type(
  const String::SubString& value)
{
  headers_msg_.append(FCGI_STDOUT, CONTENT_TYPE_AND_SEP)
    .append(FCGI_STDOUT, value)
    .append(FCGI_STDOUT, CRLF);
}

void HttpResponseFCGI::add_cookie(const char* value)
{
  headers_msg_.append(FCGI_STDOUT, SET_COOKIE_AND_SEP)
    .append(FCGI_STDOUT, String::SubString(value))
    .append(FCGI_STDOUT, CRLF);

  cookie_installed_ = true;
}

void HttpResponseFCGI::add_cookie(std::string&& value)
{
  add_cookie(value.c_str());
}

FrontendCommons::OutputStream&
HttpResponseFCGI::get_output_stream() noexcept
{
  return output_stream_;
}

ssize_t HttpResponseFCGI::write(
  const String::SubString& str) noexcept
{
  size_t saved_size = 0;

  while(str.size() > saved_size)
  {
    if(body_messages_.empty() ||
      body_messages_.back()->msg.size() >= body_messages_.back()->msg.capacity())
    {
      body_messages_.push_back(
        std::unique_ptr<MessageHolder>(new MessageHolder(status_msg_.id())));
    }

    tinyfcgi::message& target_body_msg = body_messages_.back()->msg;

    size_t msg_size = std::min(
      str.size() - saved_size,
      target_body_msg.capacity() - target_body_msg.size());

    target_body_msg.append(FCGI_STDOUT, str.substr(saved_size, msg_size));

    assert(target_body_msg);

    saved_size += msg_size;
  }

  body_size_ += str.size();

  return str.size();
}

ssize_t HttpResponseFCGI::write(std::string&& str) noexcept
{
  return write(String::SubString(str.data(), str.size()));
}

size_t HttpResponseFCGI::end_response(
  std::vector<String::SubString>& res,
  int status)
  noexcept
{
  char buf[256];
  int n = snprintf(buf, sizeof(buf), "%d ", status);
  String::SubString status_text;

  switch(status)
  {
    case 200: status_text = STATUS_200; break;
    case 204: status_text = STATUS_204; break;
    case 301: status_text = STATUS_301; break;
    case 302: status_text = STATUS_302; break;
    case 303: status_text = STATUS_303; break;
    case 307: status_text = STATUS_307; break;
    case 400: status_text = STATUS_400; break;
    case 403: status_text = STATUS_403; break;
    case 404: status_text = STATUS_404; break;
    case 500: status_text = STATUS_500; break;
    default: status_text = String::SubString(buf, n);
  }

  status_msg_.append(FCGI_STDOUT, STATUS_AND_SEP)
    .append(FCGI_STDOUT, String::SubString(buf, n))
    .append(FCGI_STDOUT, status_text)
    .append(FCGI_STDOUT, CRLF)
    .clear_padding();

  n = snprintf(buf, sizeof(buf), "%d", (int)body_size_);

  headers_msg_.append(FCGI_STDOUT, CONTENT_LENGTH_AND_SEP)
    .append(FCGI_STDOUT, String::SubString(buf, n))
    .append(FCGI_STDOUT, CRLF)
    .append(FCGI_STDOUT, CRLF) // terminate headers
    .clear_padding();

  end_msg_.end_request(0, FCGI_REQUEST_COMPLETE);

  res.reserve(3 + body_messages_.size());

  res.push_back(status_msg_.str());
  res.push_back(headers_msg_.str());

  for(auto it = body_messages_.begin(); it != body_messages_.end(); ++it)
  {
    res.push_back((*it)->msg.str());
  }

  res.push_back(end_msg_.str());

  size_t res_size = 0;
  for(auto res_it = res.begin(); res_it != res.end(); ++res_it)
  {
    res_size += res_it->size();
  }

  return res_size;
}

FrontendCommons::HttpResponse_var
HttpResponseFactory::create()
{
  return FrontendCommons::HttpResponse_var(new HttpResponseFCGI(1));
}

ParseRes HttpRequestFCGI::parse(char* buf, size_t size)
{
  tinyfcgi::const_message message(buf, size);

  uint16_t id = 0;
  bool need_more = true;

  auto stdin_i = message.end();

  for(auto header_it = message.begin(); header_it != message.end(); ++header_it)
  {
    const tinyfcgi::header& header = *header_it;

    if (!header.valid())
    {
      return PARSE_INVALID_HEADER; // invalid header
    }

    if (id == 0)
    {
      if (header.type != FCGI_BEGIN_REQUEST)
      {
        return PARSE_BEGIN_REQUEST_EXPECTED; // begin request expected
      }
      id = header.id();
    }
    else
    {
      if (id != header.id())
      {
        return PARSE_INVALID_ID; // all headers should have same id
      }
    }

    if (header.type == FCGI_STDIN)
    {
      if (header.size() == 0)
      {
        need_more = false;
        break;
      }

      if (stdin_i == message.end())
      {
        stdin_i = header_it;
      }
      else
      {
        return PARSE_FRAGMENTED_STDIN; // fragmented stdin not supported
      }

      auto next_i = header_it;
      ++next_i;
      if (next_i.valid() && next_i->valid() &&
          next_i->type == FCGI_STDIN && next_i->size() > 0)
      {
        tinyfcgi::header* mh = (tinyfcgi::header*)&header;
        mh->merge_next();
        continue;
      }
    }
  }

  if(need_more)
  {
    return PARSE_NEED_MORE;
  }

  if (stdin_i.valid())
  {
    body_ = stdin_i->str();
  }

  input_stream_.set_buf(body_);
  for(auto i = m.begin(); i != m.end(); ++i)
  {
    const tinyfcgi::header& h = *i;
    if (h.type == FCGI_PARAMS)
    {
      tinyfcgi::const_params params(h.str());
      for(auto i = params.begin(); i != params.end(); ++i)
      {
        String::SubString name, value;
        i->read(name, value);

        if(name == QUERY_STRING)
        {
          query_string_ = value;
        }
        else if(name == REQUEST_URI)
        {
          uri_ = value;
        }
        else if(name == REQUEST_METHOD)
        {
          if(value == GET)
          {
            method_ = RM_GET;
          }
          else if(value == HEAD)
          {
            method_ = RM_GET;
            header_only_ = true;
          }
          else if(value == POST)
          {
            method_ = RM_POST;
          }
        }
        else if(name == HTTPS)
        {
          secure_ = true;
          headers_.push_back(HTTP::SubHeader(PASS_SECURE, PASS_SECURE_VALUE));
        }
        else if(name == REMOTE_ADDR)
        {
          headers_.push_back(HTTP::SubHeader(PASS_REMOTE_ADDR, value));
        }
        else if(name.size() > 5 && name.compare(0, 5, "HTTP_") == 0)
        {
          headers_.push_back(HTTP::SubHeader(name.substr(5), value));
        }
      }
    }
  }

  return PARSE_OK;
}

} // namespace FCGI
