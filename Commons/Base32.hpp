#ifndef COMMONS_BASE32_HPP_
#define COMMONS_BASE32_HPP_

#include <assert.h>
#include <string.h>
#include <string>
#include <iostream>
#include <String/SubString.hpp>

namespace AdServer
{
namespace Commons
{
  void
  base32_encode(
    std::string& res,
    const String::SubString& src)
    noexcept;

  bool
  base32_decode(
    std::string& res,
    const String::SubString& src)
    noexcept;
}
}

namespace AdServer
{
namespace Commons
{
  namespace
  {
    const char BASE32_LOW_ENCODE[] = {
      'a', 'b', 'c', 'd', 'e', 'f',
      'g', 'h', 'i', 'j', 'k', 'l',
      'm', 'n', 'o', 'p', 'q', 'r',
      's', 't', 'u', 'v', 'w', 'x',
      'y', 'z', '2', '3', '4', '5',
      '6', '7'
    };

    const uint8_t BASE32_LOW_DECODE[256] =
    {
      0000, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*000-007*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*010-017*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*020-027*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*030-037*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*040-047*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*050-057*/
      0177, 0177, 0032, 0033, 0034, 0035, 0036, 0037, /*060-067*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*070-077*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*100-107*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*110-117*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*120-127*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*130-137*/
      0177, 0000, 0001, 0002, 0003, 0004, 0005, 0006, /*140-147*/
      0007, 0010, 0011, 0012, 0013, 0014, 0015, 0016, /*150-157*/
      0017, 0020, 0021, 0022, 0023, 0024, 0025, 0026, /*160-167*/
      0027, 0030, 0031, 0177, 0177, 0177, 0177, 0177, /*170-177*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*200-207*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*210-217*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*220-227*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*230-237*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*240-247*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*250-257*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*260-267*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*270-277*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*300-307*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*310-317*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*320-327*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*330-337*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*350-357*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*360-367*/
      0177, 0177, 0177, 0177, 0177, 0177, 0177, 0177, /*370-377*/
    };
  }

  inline void
  base32_encode_block(
    char* res,
    const char* src)
    noexcept
  {
    // pack 5 bytes
    uint64_t buffer = 0;

    for(int i = 0; i < 5; i++)
    {
      if(i != 0)
      {
        buffer = (buffer << 8);
      }

      buffer = buffer | static_cast<unsigned char>(src[i]);
    }

    // output 8 bytes
    for(int j = 7; j >= 0; --j)
    {
      buffer = buffer << (24 + (7 - j) * 5);
      buffer = buffer >> (24 + (7 - j) * 5);

      unsigned char c = static_cast<unsigned char>(buffer >> (j * 5));
      assert(c < 32);

      res[7 - j] = BASE32_LOW_ENCODE[c];
    }
  }

  inline bool
  base32_decode_block(
    char* res, // 5 bytes
    const char* src // 8 bytes
    )
    noexcept
  {
    // pack 8 bytes
    uint64_t buffer = 0;
    for(int i = 0; i < 8; ++i)
    {
      // input check
      if(i != 0)
      {
        buffer = (buffer << 5);
      }

      uint8_t dec = BASE32_LOW_DECODE[static_cast<unsigned char>(src[i])];
      if(dec >= 32)
      {
        std::cerr << "Invalid code: " <<
          static_cast<unsigned int>(static_cast<unsigned char>(src[i])) << "' => '" <<
          static_cast<unsigned int>(dec) << "'" << std::endl;
        return false;
      }

      buffer = buffer | dec;
    }

    // output 5 bytes
    for(int j = 4; j >= 0; --j)
    {
      res[4 - j] = static_cast<unsigned char>(buffer >> (j * 8));
    }

    return true;
  }

  inline void
  base32_encode(
    std::string& res,
    const String::SubString& src)
    noexcept
  {
    int d = src.size() / 5;
    int r = src.size() % 5;

    res.reserve((d + (r > 0 ? 1 : 0)) * 8);

    char out_buf[8];

    for(int j = 0; j < d; j++)
    {
      base32_encode_block(out_buf, src.data() + j * 5);
      res.append(out_buf, out_buf + 8);
    }

    char padd[5];
    ::memset(padd, 0, 5);
    for(int i = 0; i < r; i++)
    {
      padd[i] = src[src.size() - r + i];
    }

    base32_encode_block(out_buf, padd);

    int encode_bits = r * 8;
    int encode_len = encode_bits / 5 + ((encode_bits % 5) > 0 ? 1 : 0);

    res.append(out_buf, out_buf + encode_len);
  }

  inline bool
  base32_decode(
    std::string& res,
    const String::SubString& src)
    noexcept
  {
    int d = src.size() / 8;
    int r = src.size() % 8;

    res.reserve((d + (r > 0 ? 1 : 0)) * 5);

    char out_buf[5];

    for(int j = 0; j < d; ++j)
    {
      if(!base32_decode_block(out_buf, src.data() + j * 8))
      {
        return false;
      }

      res.append(out_buf, out_buf + 5);
    }

    char padd[8];
    ::memset(padd, 0, 8);

    for(int i = 0; i < r; i++)
    {
      padd[i] = src[src.size() - r + i];
    }

    if(!base32_decode_block(out_buf, padd))
    {
      return false;
    }

    res.append(out_buf, out_buf + r * 5 / 8);

    return true;
  }  
}
}

#endif /*COMMONS_BASE32_HPP_*/
