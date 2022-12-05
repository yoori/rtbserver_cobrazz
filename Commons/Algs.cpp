#include "Algs.hpp"

namespace Algs
{
  DECLARE_EXCEPTION(InvalidHex, eh::DescriptiveException);

  unsigned long
  hex_encode(
    Generics::ArrayAutoPtr<unsigned char>& res,
    const String::SubString& src)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "hex_encode()";

    res.reset(src.size() / 2);
    unsigned char* res_it = res.get();
    for(String::SubString::ConstPointer it = src.begin();
        it != src.end(); ++it, ++res_it)
    {
      unsigned char high_val = static_cast<unsigned char>(
      String::AsciiStringManip::hex_to_int(*it++));
      if(it == src.end())
      {
        Stream::Error ostr;
        ostr << FUN << ": unexpected end of value";
        throw InvalidHex(ostr);
      }

      *res_it = (high_val << 4) +
        static_cast<unsigned char>(
          String::AsciiStringManip::hex_to_int(*it));
    }

    return src.size() / 2;
  }
}
