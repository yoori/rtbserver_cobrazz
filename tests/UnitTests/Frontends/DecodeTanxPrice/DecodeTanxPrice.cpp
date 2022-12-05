
#include <iostream>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <Generics/ArrayAutoPtr.hpp>
#include <String/StringManip.hpp>
#include <Stream/MemoryStream.hpp>
#include <eh/Exception.hpp>

namespace UnitTests
{
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    bool
    decode_tanx_price(
      int32_t& real_price,
      const String::SubString& base64_src,
      const unsigned char* g_key,
      bool verbose = true)
    {
      const unsigned int VERSION_LENGTH = 1;
      const unsigned int BIDID_LENGTH = 16;
      const unsigned int ENCODEPRICE_LENGTH = 4;
      const unsigned int CRC_LENGTH = 4;
      const unsigned int KEY_LENGTH = 16;
      const unsigned int ALL_LENGTH = VERSION_LENGTH + BIDID_LENGTH + 
        ENCODEPRICE_LENGTH + CRC_LENGTH;
      const unsigned int VERSION_OFFSITE = 0;
      const unsigned int BIDID_OFFSITE = VERSION_OFFSITE + VERSION_LENGTH;
      const unsigned int ENCODEPRICE_OFFSITE = BIDID_OFFSITE + BIDID_LENGTH;
      const unsigned int CRC_OFFSITE = ENCODEPRICE_OFFSITE + ENCODEPRICE_LENGTH;
      //const unsigned int MAGICTIME_OFFSET = 7;
      /*
      const int32_t __endianCheck = 1;
      const bool IS_BIG_ENDIAN = (*(char*)&__endianCheck == 0);
      */

      std::string encrypted_value;

      try
      {
        String::StringManip::base64mod_decode(
          encrypted_value,
          base64_src,
          true // padding
          );
      }
      catch(const eh::Exception& e)
      {
        if(verbose)
        {
          std::cerr << "Can't make base64 decode \"" << base64_src.str() << "\"." << std::endl;
        }
        return false;
      }

      unsigned char* src = reinterpret_cast<unsigned char*>(
        &*encrypted_value.begin());

      if(encrypted_value.size() < ALL_LENGTH)
      {
        if(verbose)
        {
          std::cerr << "The length of value " << encrypted_value.size() << " is shorter "
            << ALL_LENGTH << " \"" << encrypted_value << "\"." << std::endl;
        }
        return false;
      }

      // get version and check
      int v = *src;
      if (v != 1)
      {
        if(verbose)
        {
          std::cerr << "version " << v << " isn't equal 1" << std::endl;
        }
        return false;
      }

      // copy bidid + key
      unsigned char buf[64];
      memcpy(buf, src + BIDID_OFFSITE, BIDID_LENGTH);
      memcpy(buf + BIDID_LENGTH, g_key, KEY_LENGTH);

      // ctx_buf = MD5(bidid + key)
      unsigned char ctx_buf[16];
      MD5(buf, 32, ctx_buf);

      // get settle price
      int32_t price = 0;
      unsigned char* p1 = reinterpret_cast<unsigned char*>(&price);
      const unsigned char* p2 = src + ENCODEPRICE_OFFSITE;
      unsigned char* p3 = ctx_buf;
      for (int i = 0; i < 4; ++i)
      {
        *p1++ = *p2++ ^ *p3++;
      }

      real_price = price;
      if(verbose)
      {
        std::cerr << "Not verified price is " << real_price << std::endl;
      }

      // calc crc and compare with src
      // if not match, it is illegal

      // copy(version + bidid + settle_price + key)
      const int vb = VERSION_LENGTH + BIDID_LENGTH;
      unsigned char* pbuf = buf;
      ::memcpy(pbuf, src, vb);// copy version+bidid
      pbuf += vb;

      // notice: here is price not real_price
      // more important for big endian !
      ::memcpy(pbuf, &price, 4); // copy settle_price
      pbuf += 4;
      ::memcpy(pbuf, g_key, KEY_LENGTH); // copy key

      // MD5(version + bidid + settle_price + key)
      MD5(buf, VERSION_LENGTH + BIDID_LENGTH + 4 + KEY_LENGTH, ctx_buf);

      if (0 != ::memcmp(ctx_buf, src + CRC_OFFSITE, CRC_LENGTH))
      {
        if(verbose)
        {
          std::cerr << "checksum error!" << std::endl;
        }
        return false;
      }

      //time = ntohl(*(uint32_t*)(src + MAGICTIME_OFFSET));
      return true;
    }

    void
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
          throw Exception(ostr);
        }

        *res_it = (high_val << 4) +
          static_cast<unsigned char>(
            String::AsciiStringManip::hex_to_int(*it));
      }
    }
}

int main(int argc, char* argv[])
{
  /*
  static unsigned char g_key[] = {
    0xb0, 0x03, 0x6c, 0xe2, 0xa7, 0xed, 0xb0, 0x2c,
    0x06, 0xfa, 0xb9, 0xca, 0xc2, 0xbe, 0xe3, 0x33
  };*/
  Generics::ArrayAutoPtr<unsigned char> g_key;

  if(argc < 3)
  {
    std::cerr << "using: DecodeTanxPrice tanx_price key" << std::endl;
    return 1;
  }

  try
  {
    UnitTests::hex_encode(g_key, String::SubString(argv[2]));
  }
  catch(const eh::Exception& e)
  {
    std::cerr << "Error reading key: " << e.what() << std::endl;
    return 1;
  }

  std::string decoded_price;
  String::StringManip::mime_url_decode(
    String::SubString(argv[1]), decoded_price);
  int32_t real_price;
  if(UnitTests::decode_tanx_price(
      real_price, String::SubString(decoded_price), g_key.get()))
  {
    std::cout << "Decoded price is " << real_price << '.' << std::endl;
  }
  else
  {
    std::cerr << "Can't decode price \""  << decoded_price << "\"." << std::endl;
    return 1;
  }
  return 0;
}

