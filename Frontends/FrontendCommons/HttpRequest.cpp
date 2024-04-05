// STD
#include <cstring>

// THIS
#include <Frontends/FrontendCommons/HttpRequest.hpp>
#include <String/StringManip.hpp>

namespace FrontendCommons
{

InputStream::InputStream() noexcept
  : pos_(0)
{
}

InputStream::InputStream(const String::SubString& buf) noexcept
  : buf_(buf),
    pos_(0)
{
}

void InputStream::set_buf(const String::SubString& buf) noexcept
{
  buf_ = buf;
  pos_ = 0;
}

Stream::BinaryInputStream& InputStream::read(
  char_type* s,
  streamsize n)
{
  if (n > buf_.size() - pos_)
  {
    n = buf_.size() - pos_;
    if (n == 0)
    {
      setstate(std::ios_base::eofbit | std::ios_base::failbit);
      gcount_ = 0;
      return *this;
    }
  }
  std::memcpy(s, buf_.data() + pos_, n);
  pos_ += n;
  gcount_ = n;
  return *this;
}

void InputStream::has_body(bool /*val*/) noexcept
{
}

void HttpRequest::parse_params(
  const String::SubString& str,
  HTTP::ParamList& params)
{
  String::StringManip::SplitAmp tokenizer(str);
  String::SubString token;
  while (tokenizer.get_token(token))
  {
    String::SubString enc_name;
    String::SubString enc_value;
    String::SubString::SizeType pos = token.find('=');
    if (pos == String::SubString::NPOS)
    {
      enc_name = token;
    }
    else
    {
      enc_name = token.substr(0, pos);
      enc_value = token.substr(pos + 1);
    }

    try
    {
      HTTP::Param param;

      String::StringManip::mime_url_decode(enc_name, param.name);
      String::StringManip::mime_url_decode(enc_value, param.value);

      params.push_back(std::move(param));
    }
    catch (const String::StringManip::InvalidFormatException&)
    {
    }
  }
}

} // namespace FrontendCommons