
namespace AdServer
{
  namespace Commons
  {

    // SecKeyStorage
    template <typename Key>
    SecKeyStorage<Key>::SecKeyStorage(
      const std::vector<std::string>&  keys)
    {
      auto make_key =
        std::bind(&SecKeyStorage::make_key_,
          this, std::placeholders::_1);
      
      keys_.resize(keys.size());
      
      std::transform(keys.begin(), keys.end(),
        keys_.begin(), make_key);
    }


    template <typename Key>
    const Key&
    SecKeyStorage<Key>::get_key(
      const Generics::Time& now,
      std::size_t& key_id) const
    {
      
      const time_t UTC_ZERO = 252460800;
      const Generics::Time time_unit(24 * 60 * 60, 0);
      
      const Generics::Time ts =
        Generics::Time(now.tv_sec - (now.tv_sec - UTC_ZERO) % time_unit.tv_sec);
      
      const Generics::Time now_gmt = ts.get_gm_time();
      const time_t delta = now_gmt.tv_sec - UTC_ZERO;
      
      key_id = ((delta / time_unit.tv_sec) % keys_.size());
      
      return keys_[key_id];
    }

    template <typename Key>
    Key
    SecKeyStorage<Key>::make_key_(
      std::string base64_buf)
      /*throw (Exception, eh::Exception)*/
    {
      static const char* FUN = "SecKeyStorage::make_key_()";
      
      const std::size_t AES_KEY_BYTES_BASE64 = PROXYSENSE_BASE64LEN(PlainTextToken::size);
      
      if (AES_KEY_BYTES_BASE64 != base64_buf.length())
      {
        Stream::Error ostr;
        ostr << FUN << ": Invalid key length: '" << base64_buf << "'";
        throw Exception(ostr);
      }
      
      if (base64_buf[AES_KEY_BYTES_BASE64 - 1] != '=' ||
        base64_buf[AES_KEY_BYTES_BASE64 - 2] != '=')
      {
        Stream::Error ostr;
        ostr << FUN << ": invalid key format '" << base64_buf <<
          "', '==' at the end expected";
        throw Exception(ostr);
      }
      
      base64_buf[AES_KEY_BYTES_BASE64 - 1] = '|';
      base64_buf[AES_KEY_BYTES_BASE64 - 2] = '|';
      
      std::string key;
      
      try
      {
        String::StringManip::base64mod_decode(key,
          String::SubString(base64_buf.data(), AES_KEY_BYTES_BASE64));
      }
      catch(const eh::Exception& e)
      {
        Stream::Error os;
        os << FUN << ": '" << base64_buf <<
          "' base64 decode failed: " << e.what();
        throw Exception(os);
      }
      
      if (key.length() != PlainTextToken::size)
      {
        Stream::Error os;
        os << FUN << ": '" << base64_buf <<
          "' invalid key length: " << key.length();
        throw Exception(os);
      }
        
      return Key(
        reinterpret_cast<const unsigned char*>(key.data()), key.size());
    }
    
  }
}

