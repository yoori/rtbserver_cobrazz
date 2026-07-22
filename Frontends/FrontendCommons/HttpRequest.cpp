#include "HttpRequest.hpp"

namespace
{
}

namespace FCGI
{
void
HttpRequest::parse_params(
  std::string_view str,
  HTTP::ParamList& params)
  /*throw(String::StringManip::InvalidFormatException, eh::Exception)*/
{
  if(str.empty())
  {
    return;
  }

  const String::SubString input(str.data(), str.size());
  String::StringManip::SplitAmp tokenizer(input);
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

}
