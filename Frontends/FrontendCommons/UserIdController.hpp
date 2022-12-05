#ifndef FRONTENDS_USERIDCONTROLLER_HPP
#define FRONTENDS_USERIDCONTROLLER_HPP

#include <memory>
#include <eh/Exception.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <String/SubString.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/BoundedMap.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Commons/UserInfoManip.hpp>
#include <Commons/IPCrypter.hpp>
#include <Commons/UserIdBlackList.hpp>

namespace AdServer
{
  class UserIdController: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    enum KeyType
    {
      PERSISTENT,
      TEMPORARY,
      SSP
    };

  public:
    UserIdController(
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
      /*throw(eh::Exception)*/;

    Generics::SignedUuid
    verify(
      const String::SubString& uid_str,
      KeyType key_type = PERSISTENT) const
      /*throw(eh::Exception)*/;

    Generics::SignedUuid
    generate() const /*throw(eh::Exception)*/;

    bool
    is_blacklisted(const AdServer::Commons::UserId& user_id) const
      noexcept;

    bool
    null_blacklisted(AdServer::Commons::UserId& user_id) const
      noexcept;

    Generics::SignedUuid
    sign(
      const AdServer::Commons::UserId& uid,
      KeyType key_type = PERSISTENT) const
      /*throw(eh::Exception)*/;

    // convert user id(persistent) to ssp user id
    Generics::Uuid
    ssp_uuid(
      const Generics::Uuid& uuid,
      const String::SubString& source_id)
      /*throw(eh::Exception)*/;

    static std::string
    ssp_uuid_string(const Generics::Uuid& uuid)
      noexcept;

    // convert ssp user id to user id(persistent)
    Generics::Uuid
    get_by_ssp_user_id(
      const Generics::Uuid& uuid,
      const String::SubString& source_id,
      uint8_t data_marker)
      /*throw(eh::Exception)*/;

  protected:
    virtual
    ~UserIdController() noexcept
    {}

  private:
    typedef Generics::BoundedMap<
      Generics::StringHashAdapter, Generics::SignedUuid>
      CheckUserIdMap;

    typedef Generics::BoundedMap<
      Generics::StringHashAdapter,
      AdServer::Commons::AesEncryptKey>
      SourceEncryptKeyMap;

    typedef Generics::BoundedMap<
      Generics::StringHashAdapter,
      AdServer::Commons::AesDecryptKey>
      SourceDecryptKeyMap;

    typedef std::unique_ptr<Generics::SignedUuidVerifier> UuidVerifierPtr;

  private:
    static Generics::SignedUuid verify_(
      CheckUserIdMap& cache,
      const Generics::SignedUuidVerifier* verifier,
      const String::SubString& uid_str,
      const Commons::UserIdBlackList& user_id_black_list,
      bool data_expected)
      /*throw(eh::Exception)*/;

    template<typename KeyType>
    KeyType
    init_source_key_(
      const String::SubString& src)
      noexcept;

    Generics::Uuid
    get_by_ssp_user_id_v0_(
      const Generics::Uuid& uuid,
      const String::SubString& source_id)
      /*throw(eh::Exception)*/;

  private:
    static const unsigned long KEY_SIZE_ = 16;

    UuidVerifierPtr uid_verifier_;
    UuidVerifierPtr temp_uid_verifier_;
    UuidVerifierPtr ssp_uid_verifier_;

    std::unique_ptr<Generics::SignedUuidGenerator> uid_generator_;
    std::unique_ptr<Generics::SignedUuidGenerator> ssp_uid_generator_;
    Generics::ArrayAutoPtr<unsigned char> ssp_uid_encrypt_global_key_; // 16 bytes

    mutable CheckUserIdMap user_id_cache_;
    mutable CheckUserIdMap temp_user_id_cache_;
    mutable CheckUserIdMap ssp_user_id_cache_;
    mutable SourceEncryptKeyMap source_encrypt_key_cache_;
    mutable SourceDecryptKeyMap source_decrypt_key_cache_;

    Commons::UserIdBlackList user_id_black_list_;
  };

  typedef ReferenceCounting::SmartPtr<UserIdController>
    UserIdController_var;

  class SetUidController: public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct SetUid
    {
      Generics::SignedUuid client_id;
      AdServer::CampaignSvcs::UserStatus user_status;

      SetUid(
        Generics::SignedUuid&& id,
        AdServer::CampaignSvcs::UserStatus status)
        noexcept;
    };

    typedef std::unique_ptr<SetUid> SetUidPtr;

    enum SetUidOption
    {
      SUO_ALWAYS,
      SUO_PARAM,
      SUO_NEVER
    };

  public:
    SetUidController(
      UserIdController* user_id_controller,
      const String::SubString& set_uid_flag,
      bool use_probe_uid)
      /*throw(eh::Exception)*/;

    std::unique_ptr<SetUid>
    generate_if_allowed(
      AdServer::CampaignSvcs::UserStatus curr_user_status,
      const AdServer::Commons::UserId& curr_client_id,
      bool set_uid_param)
      /*throw(eh::Exception)*/;

  protected:
    virtual
    ~SetUidController() noexcept
    {}

    bool
    allow_set_uid_(bool set_uid_param) const noexcept;

  private:
    UserIdController_var user_id_controller_;
    SetUidOption allow_set_uid_flag_;
    bool use_probe_uid_;
  };

  typedef ReferenceCounting::SmartPtr<SetUidController>
    SetUidController_var;
}

namespace AdServer
{
  inline
  SetUidController::SetUid::SetUid(
    Generics::SignedUuid&& id,
    AdServer::CampaignSvcs::UserStatus status)
    noexcept
    : client_id(id), user_status(status)
  {}
}

#endif /*FRONTENDS_USERIDCONTROLLER_HPP*/
