#include <Generics/CRC.hpp>
#include <Generics/Rand.hpp>
#include <String/StringManip.hpp>
#include <Stream/MemoryStream.hpp>

#include "IPCrypter.hpp"

namespace AdServer
{
namespace Commons
{
  //
  // AesEncryptKey
  //
  AesEncryptKey::AesEncryptKey() noexcept
  {}

  AesEncryptKey::AesEncryptKey(const unsigned char *key, unsigned long size)
    noexcept
  {
    AES_set_encrypt_key(key, size*8, &key_);
  }

  AesEncryptKey::~AesEncryptKey() noexcept
  {
    OPENSSL_cleanse(&key_, sizeof(key_));
  }

  void
  AesEncryptKey::encrypt(void* out, const void* in) const
    noexcept
  {
    AES_encrypt(
      static_cast<const unsigned char*>(in),
      static_cast<unsigned char*>(out),
      &key_);
  }

  //
  // AesDecryptKey
  //
  AesDecryptKey::AesDecryptKey() noexcept
  {}

  AesDecryptKey::AesDecryptKey(const unsigned char *key, unsigned long size)
    noexcept
  {
    AES_set_decrypt_key(key, size*8, &key_);
  }

  AesDecryptKey::~AesDecryptKey() noexcept
  {
    OPENSSL_cleanse(&key_, sizeof(key_));
  }

  void
  AesDecryptKey::decrypt(void* out, const void* in) const
    noexcept
  {
    AES_decrypt(
      static_cast<const unsigned char*>(in),
      static_cast<unsigned char*>(out),
      &key_);
  }

  // IPCrypter
  IPCrypter::IPCrypter(const String::SubString& key)
    /*throw(InvalidKey)*/
    : encrypt_key_(make_key_<AesEncryptKey>(key)),
      decrypt_key_(make_key_<AesDecryptKey>(key))
  {}

  void
  IPCrypter::encrypt(
    std::string& encrypted_ip,
    const char* ip)
    /*throw(InvalidParams)*/
  {
    static const char* FUN = "IPCrypter::encrypt()";

    uint32_t ip_address_buf[4];
    uint32_t ip_address;
    if (inet_pton(AF_INET, ip, ip_address_buf) != 0)
    {
      ip_address = ip_address_buf[0];      
    }
    else if(inet_pton(AF_INET6, ip, ip_address_buf) != 0)
    {
      ip_address = ip_address_buf[3];
    }
    else
    {
      Stream::Error ostr;
      ostr << FUN << ": incorrect ip value '" << ip << "'";
      throw InvalidParams(ostr);
    }

    uint32_t encrypted_buf[AES_BLOCK_SIZE / 4 + 1];
    uint32_t buf[AES_BLOCK_SIZE / 4 + 1];
    *buf = ip_address;
    for(uint32_t* salt = buf + 1;
        salt < buf + AES_BLOCK_SIZE / 4 - 1; ++salt)
    {
      *salt = Generics::safe_rand();
    }
    *(buf + AES_BLOCK_SIZE / 4 - 1) = Generics::CRC::quick(
      0, buf, (AES_BLOCK_SIZE / 4 - 1) * 4);

    encrypt_key_.encrypt(encrypted_buf, buf);

    String::StringManip::base64mod_encode(encrypted_ip,
      encrypted_buf, AES_BLOCK_SIZE, false);
  }

  void
  IPCrypter::decrypt(
    std::string& ip,
    const String::SubString& encrypted_ip)
    /*throw(InvalidParams)*/
  {
    static const char* FUN = "IPCrypter::decrypt()";

    std::string encrypted_buf;

    try
    {
      String::StringManip::base64mod_decode(
        encrypted_buf, encrypted_ip, false);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't do base64 decode: " << ex.what();
      throw InvalidParams(ostr);
    }

    if(encrypted_buf.size() < AES_BLOCK_SIZE)
    {
      Stream::Error ostr;
      ostr << FUN << ": incorrect size of encrypted buffer " <<
        encrypted_buf.size() << " instead " << AES_BLOCK_SIZE;
      throw InvalidParams(ostr);
    }

    uint32_t buf[AES_BLOCK_SIZE / 4 + 1];
    decrypt_key_.decrypt(buf, encrypted_buf.data());

    // check CRC
    if(Generics::CRC::quick(0, buf, (AES_BLOCK_SIZE / 4 - 1) * 4) !=
       *(buf + 3))
    {
      Stream::Error ostr;
      ostr << FUN << ": incorrect CRC sum";
      throw InvalidParams(ostr);
    }

    char str_ip_holder[INET_ADDRSTRLEN];
    if(inet_ntop(AF_INET, buf, str_ip_holder, sizeof(str_ip_holder)))
    {
      ip = str_ip_holder;
    }
    else
    {
      // EAFNOSUPPORT, ENOSPC unexpected here
      assert(0);
    }
  }

  template<typename KeyType>
  KeyType
  IPCrypter::make_key_(const String::SubString& base64_key)
    /*throw(InvalidKey)*/
  {
    static const char* FUN = "IPCrypter::make_key_()";

    std::string key;
    try
    {
      String::StringManip::base64mod_decode(
        key, base64_key, false);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": invalid key '" << base64_key <<
        "', base64 decode failed: " << ex.what();
      throw InvalidKey(ostr);
    }

    if(key.size() < AES_BLOCK_SIZE)
    {
      Stream::Error ostr;
      ostr << FUN << ": invalid key '" << base64_key <<
        "', size " << key.size() << " less then minimal size " << AES_BLOCK_SIZE;
      throw InvalidKey(ostr);
    }

    return KeyType(
      reinterpret_cast<const unsigned char*>(key.c_str()), AES_BLOCK_SIZE);
  }
}
}
