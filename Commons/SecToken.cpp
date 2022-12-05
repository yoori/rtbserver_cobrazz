
#include "SecToken.hpp"

#include <functional>
#include <algorithm>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <Stream/MemoryStream.hpp>

namespace AdServer
{
  namespace Commons
  {
    //
    // PlainTextToken
    //
    PlainTextToken::PlainTextToken(
      in_addr_t ip,
      const Generics::Time& now,
      uint32_t random_value,
      uint16_t udata)
      noexcept
    {
      TokenStruct* token(reinterpret_cast<TokenStruct*>(data_));

      token->ip_hash = htonl(Generics::CRC::quick(0, &ip, sizeof(ip)));
      token->timestamp = htonl(now.tv_sec);
      token->rnd8 = uint8_t(random_value);
      token->data16 = udata != 0 ? htons(udata) : uint16_t(random_value >> 8);
      static const TokenIDType TT_COMMON = 0;
      token->type8 = TT_COMMON;
      token->checksum = htonl(crc32_calc_());
    }

    void
    PlainTextToken::encrypt(
      const AesEncryptKey& key,
      unsigned char *out) const
      noexcept
    {
      key.encrypt(out, data_);
    }

    uint32_t
    PlainTextToken::crc32_calc_() const noexcept
    {
      return Generics::CRC::quick(0, data_, size - sizeof(uint32_t));
    }

    //
    // SecTokenGenerator
    //
    SecTokenGenerator::SecTokenGenerator(
      const Config& config)
      /*throw(eh::Exception)*/  :
      keys_(config.aes_keys)
    {
      if (config.custom_dependency_container)
      {
        dependency_container_ = config.custom_dependency_container;
      }
      else
      {
        dependency_container_ = new DependencyContainer();
      }
    }

    std::string
    SecTokenGenerator::get_text_token(
      in_addr_t client_address) const
      /*throw(eh::Exception)*/
    {
      const Generics::Time now = dependency_container_->get_time_of_day();

      PlainTextToken plain_token(
        client_address,
        now,
        dependency_container_->safe_rand());

      unsigned char crypto_data[PlainTextToken::size + KEYID_BYTES];
      std::size_t key_id;
      const AesEncryptKey& key = keys_.get_key(now, key_id);
      plain_token.encrypt(key, crypto_data);

      reinterpret_cast<uint16_t*>(crypto_data + PlainTextToken::size)[0] =
        htons(key_id);

      return serialize_(crypto_data);
    }

    std::string
    SecTokenGenerator::serialize_(const unsigned char* ctoken) const
    {
      std::string tmp;
      String::StringManip::base64mod_encode(
        tmp, ctoken, PlainTextToken::size + KEYID_BYTES);
      return tmp;
    }

    //
    // SecTokenValidator
    //
    SecTokenValidator::SecTokenValidator(
      const std::vector<std::string>&  keys) :
      keys_(keys)
    { }

    bool
    SecTokenValidator::verify(
      const String::SubString& token,
      const String::SubString& ip_addr,
      const Generics::Time& ts,
      const Generics::Time& timeout)
    {
      std::string encrypted_buf;

      try
      {
        String::StringManip::base64mod_decode(
          encrypted_buf, token, false);
      }
      catch(const eh::Exception& ex)
      {
        return false;
      }

      if(encrypted_buf.size() < AES_BLOCK_SIZE)
      {
        return false;
      }

      std::size_t key_id;
      const AesDecryptKey& key = keys_.get_key(ts, key_id);

      std::size_t tokenWords = PlainTextToken::size / sizeof(uint32_t);

      uint32_t buf[tokenWords];
      
      key.decrypt(buf, encrypted_buf.data());

      uint32_t crc = Generics::CRC::quick(
        0,
        buf,
        PlainTextToken::size - sizeof(uint32_t));

      for (std::size_t i = 0; i < tokenWords; ++i)
      {
        buf[i] = ntohl(buf[i]);
      }

      if (buf[tokenWords - 1] == crc)
      {
        uint32_t addr;
        
        if (inet_pton(AF_INET, ip_addr.data(), &addr))
        {
          return buf[0] == Generics::CRC::quick(0, &addr, sizeof(addr)) &&
            ts - Generics::Time(buf[1]) <= timeout;
        }
      }

      return false;
    }
    
  }
}

