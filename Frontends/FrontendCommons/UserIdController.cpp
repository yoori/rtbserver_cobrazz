#include "UserIdController.hpp"

namespace
{
  Generics::SignedUuidProbe probe_generator(
    AdServer::Commons::PROBE_USER_ID);
  Generics::SignedUuidProbe null_generator{
    Generics::Uuid()};
  const std::string PROBE_USER_ID_STR = 
    AdServer::Commons::PROBE_USER_ID.to_string();
  const uint32_t CURRENT_SSP_USER_ID_MARKER = 1;
}

namespace AdServer
{
  // UserIdController impl
  UserIdController::UserIdController(
    const char* public_key,
    const char* temp_public_key,
    const char* private_key,
    const char* ssp_public_key,
    const char* ssp_private_key,
    const String::SubString& ssp_uid_key,
    unsigned long cache_limit,
    unsigned long temp_cache_limit,
    unsigned long ssp_cache_limit,
    Commons::UserIdBlackList& user_id_black_list)
    /*throw(eh::Exception)*/
    : user_id_cache_(cache_limit, Generics::Time::ZERO),
      temp_user_id_cache_(temp_cache_limit, Generics::Time::ZERO),
      ssp_user_id_cache_(ssp_cache_limit, Generics::Time::ZERO),
      source_encrypt_key_cache_(10*1024, Generics::Time::ZERO),
      source_decrypt_key_cache_(10*1024, Generics::Time::ZERO)
  {
    user_id_black_list_.swap(user_id_black_list);

    uid_verifier_.reset(
      new Generics::SignedUuidVerifier(public_key));

    temp_uid_verifier_.reset(
      new Generics::SignedUuidVerifier(temp_public_key));

    uid_generator_.reset(
      new Generics::SignedUuidGenerator(private_key));

    ssp_uid_verifier_.reset(
      new Generics::SignedUuidVerifier(ssp_public_key));

    ssp_uid_generator_.reset(
      new Generics::SignedUuidGenerator(ssp_private_key));

    ssp_uid_encrypt_global_key_.reset(KEY_SIZE_);

    ::memset(ssp_uid_encrypt_global_key_.get(), 0, KEY_SIZE_);

    if(!ssp_uid_key.empty())
    {
      Generics::ArrayByte ssp_uid_encrypt_global_key_buf;
      unsigned long ssp_uid_encrypt_global_key_size =
        String::StringManip::hex_decode(ssp_uid_key,
          ssp_uid_encrypt_global_key_buf);

      ::memcpy(
        ssp_uid_encrypt_global_key_.get(),
        ssp_uid_encrypt_global_key_buf.get(),
        ssp_uid_encrypt_global_key_size);
    }
  }

  Generics::SignedUuid
  UserIdController::verify(
    const String::SubString& uid_str,
    KeyType key_type) const
    /*throw(eh::Exception)*/
  {
    CheckUserIdMap* cache = 0;
    const Generics::SignedUuidVerifier* verifier = 0;

    switch (key_type)
    {
    case PERSISTENT:
      cache = &user_id_cache_;
      verifier = uid_verifier_.get();
      break;

    case TEMPORARY:
      cache = &temp_user_id_cache_;
      verifier = temp_uid_verifier_.get();
      break;

    case SSP:
      cache = &ssp_user_id_cache_;
      verifier = ssp_uid_verifier_.get();
      break;
    }

    return verify_(*cache, verifier, uid_str, user_id_black_list_, key_type == SSP);
  }

  Generics::SignedUuid
  UserIdController::generate() const /*throw(eh::Exception)*/
  {
    return uid_generator_->generate();
  }

  Generics::SignedUuid
  UserIdController::sign(
    const AdServer::Commons::UserId& user_id,
    KeyType key_type) const
    /*throw(eh::Exception)*/
  {
    if (key_type == SSP)
    {
      return ssp_uid_generator_->sign(user_id, CURRENT_SSP_USER_ID_MARKER);
    }

    return uid_generator_->sign(user_id);
  }

  bool
  UserIdController::is_blacklisted(const AdServer::Commons::UserId& user_id) const
    noexcept
  {
    return user_id_black_list_.is_blacklisted(user_id);
  }

  bool
  UserIdController::null_blacklisted(AdServer::Commons::UserId& user_id) const
    noexcept
  {
    return user_id_black_list_.null_blacklisted(user_id);
  }

  Generics::SignedUuid
  UserIdController::verify_(
    CheckUserIdMap& cache,
    const Generics::SignedUuidVerifier* verifier,
    const String::SubString& uid_str,
    const Commons::UserIdBlackList& user_id_black_list,
    bool data_expected)
    /*throw(eh::Exception)*/
  {
    if (PROBE_USER_ID_STR == uid_str) 
    {
      return probe_generator.construct();
    }
    Generics::StringHashAdapter uid_str_hash(uid_str);
    CheckUserIdMap::const_iterator it = cache.find(uid_str_hash);
    if(it != cache.end())
    {
      return it->second;
    }

    Generics::SignedUuid res = verifier->verify(uid_str, data_expected);
    if (user_id_black_list.is_blacklisted(res.uuid()))
    {
      // user_id present in blacklist, return NULL UID.
      return null_generator.construct();
    }
    // Put in cache only uids passed blacklist check
    cache.insert(CheckUserIdMap::value_type(uid_str_hash, res));

    return res;
  }

  template<typename SourceKeyType>
  SourceKeyType
  UserIdController::init_source_key_(
    const String::SubString& source_id)
    noexcept
  {
    uint32_t seed = Generics::CRC::quick(
      0, source_id.data(), source_id.length());

    Generics::ArrayAutoPtr<unsigned char> res_ssp_uid_encrypt_global_key;
    res_ssp_uid_encrypt_global_key.reset(KEY_SIZE_);
    ::memcpy(res_ssp_uid_encrypt_global_key.get(),
      ssp_uid_encrypt_global_key_.get(),
      KEY_SIZE_);

    for(unsigned long i = 0; i < KEY_SIZE_ / 4; ++i)
    {
      *(reinterpret_cast<uint32_t*>(
        res_ssp_uid_encrypt_global_key.get()) + i) ^= seed;
    }

    return SourceKeyType(res_ssp_uid_encrypt_global_key.get(), KEY_SIZE_);
  }

  Generics::Uuid
  UserIdController::ssp_uuid(
    const Generics::Uuid& uuid,
    const String::SubString& /*source_id*/)
    /*throw(eh::Exception)*/
  {
    /*
    SourceEncryptKeyMap::const_iterator it =
      source_encrypt_key_cache_.find(source_id);
    AdServer::Commons::AesEncryptKey source_key;

    if(it != source_encrypt_key_cache_.end())
    {
      source_key = it->second;
    }
    else
    {
      source_key = init_source_key_<AdServer::Commons::AesEncryptKey>(source_id);
      source_encrypt_key_cache_.insert(
        SourceEncryptKeyMap::value_type(source_id, source_key));
    }

    Generics::Uuid res;
    source_key.encrypt(&*res.begin(), &*uuid.begin());
    return res;
    */
    return uuid;
  }

  std::string
  UserIdController::ssp_uuid_string(const Generics::Uuid& ssp_user_id)
    noexcept
  {
    std::string ssp_user_id_str;
    String::StringManip::base64mod_encode(
      ssp_user_id_str,
      ssp_user_id.begin(),
      ssp_user_id.size(),
      false,
      CURRENT_SSP_USER_ID_MARKER);
    return ssp_user_id_str;
  }

  Generics::Uuid
  UserIdController::get_by_ssp_user_id(
    const Generics::Uuid& ssp_user_id,
    const String::SubString& /*source_id*/,
    uint8_t /*data_marker*/)
    /*throw(eh::Exception)*/
  {
    return ssp_user_id;

    /*
    Generics::Uuid res;

    if(data_marker == 0)
    {
      // old format
      res = get_by_ssp_user_id_v0_(ssp_user_id, source_id);
    }
    else if(data_marker == CURRENT_SSP_USER_ID_MARKER)
    {
      SourceDecryptKeyMap::const_iterator it = source_decrypt_key_cache_.find(source_id);
      AdServer::Commons::AesDecryptKey source_key;

      if(it != source_decrypt_key_cache_.end())
      {
        source_key = it->second;
      }
      else
      {
        source_key = init_source_key_<AdServer::Commons::AesDecryptKey>(source_id);
        source_decrypt_key_cache_.insert(
          SourceDecryptKeyMap::value_type(source_id, source_key));
      }

      source_key.decrypt(&*res.begin(), &*ssp_user_id.begin());
    }
    else
    {
      Stream::Error err;
      err << FUN << ": uid have incorrect data value = " << data_marker;
      throw Exception(err);
    }

    // use specific of ssp user id generation - it is symmetric
    if (is_blacklisted(res))
    {
      return Generics::Uuid();
    }
    return res;
    */
  }

  Generics::Uuid
  UserIdController::get_by_ssp_user_id_v0_(
    const Generics::Uuid& uuid,
    const String::SubString& source_id)
    /*throw(eh::Exception)*/
  {
    uint32_t seed = Generics::CRC::quick(
      0, source_id.data(), source_id.length());

    Generics::Uuid ssp_uuid(uuid);
    uint32_t* as_words = reinterpret_cast<uint32_t*>(ssp_uuid.begin());
    as_words[0] ^= seed;
    as_words[1] ^= seed;
    as_words[2] ^= seed;
    as_words[3] ^= seed;
    return ssp_uuid;
  }

  /*
   * SetUidController
   */
  SetUidController::SetUidController(
    UserIdController* user_id_controller,
    const String::SubString& set_uid_flag,
    bool use_probe_uid)
    /*throw(eh::Exception)*/
    : user_id_controller_(ReferenceCounting::add_ref(user_id_controller)),
      use_probe_uid_(use_probe_uid)
  {
    static const char* FUN = "SetUidController::SetUidController()";

    const String::AsciiStringManip::Caseless set_uid_option(set_uid_flag);

    if(set_uid_option == String::SubString("always"))
    {
      allow_set_uid_flag_ = SUO_ALWAYS;
    }
    else if(set_uid_option == String::SubString("parameter"))
    {
      allow_set_uid_flag_ = SUO_PARAM;
    }
    else if(set_uid_option == String::SubString("never"))
    {
      allow_set_uid_flag_ = SUO_NEVER;
    }
    else
    {
      Stream::Error err;
      err << FUN << ": bad set_uid value in config '" << set_uid_flag << "'";
      throw Exception(err);
    }
  }

  SetUidController::SetUidPtr
  SetUidController::generate_if_allowed(
    AdServer::CampaignSvcs::UserStatus curr_user_status,
    const AdServer::Commons::UserId& curr_client_id,
    bool set_uid_param)
    /*throw(eh::Exception)*/
  {
    SetUidPtr res;

    if (curr_user_status != AdServer::CampaignSvcs::US_OPTOUT &&
        allow_set_uid_(set_uid_param))
    {//need set cookie
      if((curr_client_id.is_null() && !use_probe_uid_) ||
          curr_client_id == AdServer::Commons::PROBE_USER_ID)
      {
        res.reset(new SetUid(
          user_id_controller_->generate(),
          AdServer::CampaignSvcs::US_OPTIN));
      }
      else if(curr_client_id.is_null() && use_probe_uid_)
      {
        /*
         * Client have no identifier. Need to generate it and drop through
         * HTTP cookie, but first will probe if client is cookie enabled. So
         * will drop special id {PPPPPPPPPPPPPPPPPPPPPP..} first. If see this
         * id in incoming HTTP cookie, will replace it with generated regular
         * client id.
         */
        res.reset(new SetUid(
          Generics::SignedUuidProbe(AdServer::Commons::PROBE_USER_ID).construct(),
          AdServer::CampaignSvcs::US_PROBE));
      }
    }

    return res;
  }

  bool
  SetUidController::allow_set_uid_(bool set_uid_param) const noexcept
  {
    return (allow_set_uid_flag_ == SUO_ALWAYS ||
      (allow_set_uid_flag_ == SUO_PARAM && set_uid_param));
  }
}
