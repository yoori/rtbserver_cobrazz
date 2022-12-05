#ifndef COMMONS_SECTOKEN_HPP
#define COMMONS_SECTOKEN_HPP

#include <vector>
#include <string>
#include <Generics/CRC.hpp>
#include <Generics/Rand.hpp>
#include <Generics/Time.hpp>
#include <String/StringManip.hpp>

#include "IPCrypter.hpp"

#define PROXYSENSE_ALIGN(n, base) \
          (((n) / (base) + ((n) % (base) ? 1 : 0)) * (base))
#define PROXYSENSE_BASE64LEN(n) \
          (PROXYSENSE_ALIGN(n, 3) / 3 * 4)

namespace AdServer
{
  namespace Commons
  {
    typedef uint8_t TokenIDType;

    /*
     * @class PlainTextToken
     * @brief Security token definition
     */
    class PlainTextToken
    {
    public:
      static const size_t size = 16;

    public:
      
      /**
       * @brief Constructor
       *
       * @param ip address
       * @param now (token timestamp)
       * @param random
       * @param user data
       */
      PlainTextToken(
        in_addr_t ip,
        const Generics::Time& now,
        uint32_t random_value,
        uint16_t udata = 0)
        noexcept;

      /**
       * @brief Encrypt token
       *
       * @param AES key for encryption
       * @param [out] encrypted token
       */
      void
      encrypt(
        const AesEncryptKey& key,
        unsigned char *out) const
        noexcept;

    private:
      unsigned char data_[size];

    private:
     /*
      * @struct TokenStruct
      * @brief Token structure
      */
      struct TokenStruct
      {
        uint32_t ip_hash;    
        uint32_t timestamp;  
        uint32_t rnd8   : 8; 
        uint32_t data16 : 16;
        uint32_t type8  : 8; 
        uint32_t checksum;    
      };

    private:
      /**
       * @brief calc CRC32 for token
       *
       * @return CRC32
       */
      uint32_t
      crc32_calc_() const noexcept;
    };

    
    /*
     * @class DependencyContainer
     */
    class DependencyContainer : public ReferenceCounting::AtomicImpl
    {
    public:
      virtual Generics::Time
      get_time_of_day() const noexcept
      {
        return Generics::Time::get_time_of_day();
      }

      virtual uint32_t
      safe_rand() const noexcept
      {
        return Generics::safe_rand();
      }

    protected:
      virtual
      ~DependencyContainer() noexcept
      {}
    };

    /*
     * @class SecKeyStorage
     * @brief Keys storage
     */
    template <typename Key>
    class SecKeyStorage
    {
    public:

      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      /**
       * @brief Constructor
       *
       * @param key string vector
       */
      SecKeyStorage(
        const std::vector<std::string>& keys);

      /**
       * @brief Get key by time
       *
       * @param timestamp
       * @param [out] index of key (day of week)
       */
      const Key&
      get_key(
        const Generics::Time& now,
        std::size_t& key_id) const;
   
    private:
      
      /**
       * @brief Make key from b64 string
       *
       * @param b64 string
       * @return key
       */
      Key
      make_key_(
        std::string base64_buf)
        /*throw(Exception, eh::Exception)*/;
      
      typedef std::vector<Key> Keys;
      Keys keys_;
    };

    typedef ReferenceCounting::SmartPtr<DependencyContainer>
      DependencyContainer_var;

    /*
     * @class SecTokenGenerator
     * @brief Security token generator
     */
    class SecTokenGenerator : public ReferenceCounting::AtomicImpl
    {
    public:

      typedef in_addr_t IPAdress;

      /*
       * @struct Config
       * @brief Security token generator config
       */
      struct Config
      {
        std::vector<std::string> aes_keys;
        DependencyContainer_var custom_dependency_container;
      };

    private:

      enum
      {
        MAX_KEYS    = 0xffff,
        KEYID_BYTES = 2
      };

    private:
      DependencyContainer_var dependency_container_;
      SecKeyStorage<AesEncryptKey> keys_;

    public:

      /*
       * @brief Constructor
       * @param config
       */
      SecTokenGenerator(
        const Config& config)
        /*throw(eh::Exception)*/;

      /*
       * @brief Generate token
       * @param ip address
       * @return security token string
       */
      std::string
      get_text_token(
        in_addr_t client_address) const
        /*throw(eh::Exception)*/;

    private:

      /*
       * @brief Serialize token
       * @param token
       */
      std::string
      serialize_(
        const unsigned char* ctoken) const;

    protected:
      virtual
      ~SecTokenGenerator() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<SecTokenGenerator>
      SecTokenGenerator_var;

    /*
     * @class SecTokenValidator
     * @brief Security token generator
     */
    class SecTokenValidator : public ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      /*
       * @brief Constructor
       * @param key string vector
       */
      SecTokenValidator(
        const std::vector<std::string>&  keys);

      /*
       * @brief Verify token
       * @param received token
       * @param client IP address
       * @param timestamp
       * @param token exparation timeot (typically 600 sec)
       */
      bool
      verify(
        const String::SubString& token,
        const String::SubString& ip_addr,
        const Generics::Time& timestamp,
        const Generics::Time& timeout);
      
    protected:
      virtual
      ~SecTokenValidator() noexcept
     {}
      
    private:
     
      SecKeyStorage<AesDecryptKey> keys_;
      
    };
    
    typedef ReferenceCounting::SmartPtr<SecTokenValidator>
    SecTokenValidator_var;
  }
}

#include "SecToken.tpp"

#endif  // FRONTENDCOMMONS_SECTOKEN_HPP
