#include <sstream>

#include <Stream/MemoryStream.hpp>

#include "SecToken.hpp"

namespace AdServer
{
  namespace CampaignSvcs
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
      const Commons::AesEncryptKey& key,
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
      const Config& config,
      Logging::Logger* logger)
      /*throw(Exception, eh::Exception)*/
      : logger_(ReferenceCounting::add_ref(logger))
    {
      for (std::size_t i = 0; i < config.aes_keys.size(); ++i)
      {
        keys_.push_back(make_key_(config.aes_keys[i]));
      }

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
      /*throw(Exception, eh::Exception)*/
    {
      const Generics::Time now = dependency_container_->get_time_of_day();

      PlainTextToken plain_token(
        client_address,
        now,
        dependency_container_->safe_rand());

      unsigned char crypto_data[PlainTextToken::size + KEYID_BYTES];
      const std::size_t key_id = current_key_pos_(now);
      const Commons::AesEncryptKey& key = keys_[key_id];
      plain_token.encrypt(key, crypto_data);

      reinterpret_cast<uint16_t*>(crypto_data + PlainTextToken::size)[0] =
        htons(key_id);

      return serialize_(crypto_data);
    }

    std::size_t
    SecTokenGenerator::current_key_pos_(const Generics::Time& now) const noexcept
    {
      const time_t UTC_ZERO = 252460800;
      const Generics::Time time_unit(24 * 60 * 60, 0);

      const Generics::Time ts =
        Generics::Time(now.tv_sec - (now.tv_sec - UTC_ZERO) % time_unit.tv_sec);

      const Generics::Time now_gmt = ts.get_gm_time();
      const time_t delta = now_gmt.tv_sec - UTC_ZERO;
      const std::size_t key_pos = ((delta / time_unit.tv_sec) % keys_.size());

      return key_pos;
    }

    std::string
    SecTokenGenerator::serialize_(const unsigned char* ctoken) const
    {
      std::string tmp;
      String::StringManip::base64mod_encode(
        tmp, ctoken, PlainTextToken::size + KEYID_BYTES);
      return tmp;
    }

    Commons::AesEncryptKey
    SecTokenGenerator::make_key_(std::string base64_buf)
      /*throw(Exception, eh::Exception)*/
    {
      static const char* FUN = "SecTokenGenerator::make_key_()";

      const std::size_t AES_KEY_BYTES_BASE64 = PROXYSENSE_BASE64LEN(PlainTextToken::size);

      if (AES_KEY_BYTES_BASE64 != base64_buf.length())
      {
        Stream::Error ostr;
        ostr << FUN << ": Invalid key length: '" << base64_buf << "'";
        throw Exception(ostr.str());
      }

      if (base64_buf[AES_KEY_BYTES_BASE64 - 1] != '=' ||
          base64_buf[AES_KEY_BYTES_BASE64 - 2] != '=')
      {
        Stream::Error ostr;
        ostr << FUN << ": invalid key format '" << base64_buf <<
          "', '==' at the end expected";
        throw Exception(ostr.str());
      }

      base64_buf[AES_KEY_BYTES_BASE64 - 1] = '|';
      base64_buf[AES_KEY_BYTES_BASE64 - 2] = '|';

      std::string key;

      try
      {
        String::StringManip::base64mod_decode(key,
          String::SubString(base64_buf.c_str(), AES_KEY_BYTES_BASE64));
      }
      catch(const eh::Exception& e)
      {
        std::ostringstream os;
        os << FUN << ": '" << base64_buf <<
          "' base64 decode failed: " << e.what();
        throw Exception(os.str());
      }

      if (key.length() != PlainTextToken::size)
      {
        std::ostringstream os;
        os << FUN << ": '" << base64_buf <<
          "' invalid key length: " << key.length();
        throw Exception(os.str());
      }

      return Commons::AesEncryptKey(
        reinterpret_cast<const unsigned char*>(key.c_str()), key.size());
    }
  }
}
