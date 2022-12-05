#include <tests/AutoTests/Commons/Common.hpp>

std::string
tolower(size_t size, const char* str)
{
  std::string ret;
  ret.reserve(size);
  for(const char* i = str; *i != 0; ++i)
  {
    ret += ::tolower(*i);
  }
  return ret;
}

std::string
toupper(size_t size, const char* str)
{
  std::string ret;
  ret.reserve(size);
  for(const char* i = str; *i != 0; ++i)
  {
    ret += ::toupper(*i);
  }
  return ret;
}

namespace AutoTest
{
  std::string time_to_gmt_str( time_t time )
  {
    return Generics::Time(time).get_gm_time().format("%d-%m-%Y:%H-%M-%S");
  }

  time_t gmt_str_to_time( const char* gmt_str ) /*throw(eh::Exception)*/
  {
    return Generics::Time(String::SubString(gmt_str),
      "%d-%m-%Y:%H-%M-%S").tv_sec;
  }
  
  Comparable::~Comparable() noexcept
  {
  }

  bool
  ComparableRegExp::compare(const char* other) const
  {
    try
    {
      String::RegEx re(
        std::string("\\A") + value_ + "\\Z",
        PCRE_MULTILINE | PCRE_DOTALL);
      return re.match( String::SubString(other));
    }
    catch (const eh::Exception&)
    {
      return value_ == other;
    }
  }

  bool
  precisely_number::compare(const char* other) const
  {
    precisely_number other_number(other, precisely_);
    return *this == other_number;
  }

  std::string
  precisely_number::str() const
  {
    std::ostringstream ost;
    ost << *this;
    return ost.str();
  }

  StringList
  parse_list (const char* str)
  {
    StringList ret;
    String::StringManip::Tokenizer tokenizer(String::SubString(str), ",");
    String::SubString token;
    while (tokenizer.get_token(token))
    {
      String::StringManip::trim(token);
      ret.push_back(token.str());
    }
    return ret;
  }
}
