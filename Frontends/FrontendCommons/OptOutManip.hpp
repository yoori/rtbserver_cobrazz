
#ifndef OPT_OUT_MANIP_HPP
#define OPT_OUT_MANIP_HPP

#include <string>
#include <eh/Exception.hpp>
#include <Generics/Time.hpp>
#include <String/StringManip.hpp>

namespace AdServer
{
  class OptInDays
  {
    public:
    DECLARE_EXCEPTION(InvalidParam, eh::DescriptiveException);

    static
    void
    load_opt_in_days(const String::SubString& value, Generics::Time& val)
      /*throw(InvalidParam)*/;

    static const char* save_opt_in_days(
      const Generics::Time& val,
      std::string& buf)
      noexcept;
  };
}

namespace AdServer
{
  inline
  void
  OptInDays::load_opt_in_days(const String::SubString& value,
    Generics::Time& val) /*throw(InvalidParam)*/
  {
    std::string decoded;
    String::StringManip::base64mod_decode(decoded, value);
    if(decoded.size() != Generics::Time::TIME_PACK_LEN)
    {
      throw InvalidParam(
        "FrontendCommons::load_opt_in_days(): "
        "noncorrect input buffer length.");
    }

    val.unpack(decoded.data());
  }

  inline
  const char* OptInDays::save_opt_in_days(
    const Generics::Time& val,
    std::string& buf)
    noexcept
  {
    char tbuf[Generics::Time::TIME_PACK_LEN];
    val.pack(tbuf);
    String::StringManip::base64mod_encode(buf, tbuf, sizeof(tbuf));

    return buf.c_str();
  }
}

#endif//OPT_OUT_MANIP_HPP
