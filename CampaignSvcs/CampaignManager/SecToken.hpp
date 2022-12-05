#ifndef CAMPAIGNMANAGER_SECTOKEN_HPP_
#define CAMPAIGNMANAGER_SECTOKEN_HPP_

#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <Generics/CRC.hpp>
#include <Generics/Rand.hpp>
#include <Generics/Time.hpp>
#include <Logger/Logger.hpp>

#include <Commons/IPCrypter.hpp>

#define PROXYSENSE_ALIGN(n, base) \
          (((n) / (base) + ((n) % (base) ? 1 : 0)) * (base))
#define PROXYSENSE_BASE64LEN(n) \
          (PROXYSENSE_ALIGN(n, 3) / 3 * 4)

namespace AdServer
{
  namespace CampaignSvcs
  {
    typedef uint8_t TokenIDType;

    class PlainTextToken
    {
    public:
      static const size_t size = 16;

    public:
      PlainTextToken(
        in_addr_t ip,
        const Generics::Time& now,
        uint32_t random_value,
        uint16_t udata = 0)
        noexcept;

      void
      encrypt(
        const Commons::AesEncryptKey& key,
        unsigned char *out) const
        noexcept;

    private:
      unsigned char data_[size];

    private:
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
      uint32_t
      crc32_calc_() const noexcept;
    };

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

    typedef ReferenceCounting::SmartPtr<DependencyContainer>
      DependencyContainer_var;

    class SecTokenGenerator : public ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      typedef in_addr_t IPAdress;

      struct Config
      {
        std::vector<std::string> aes_keys;
        DependencyContainer_var custom_dependency_container;
      };

    private:
      typedef std::vector<Commons::AesEncryptKey> Keys;

      enum
      {
        MAX_KEYS    = 0xffff,
        KEYID_BYTES = 2
      };

    private:
      Logging::Logger_var logger_;
      DependencyContainer_var dependency_container_;
      Keys keys_;

    public:
      SecTokenGenerator(
        const Config& config,
        Logging::Logger* logger)
        /*throw(Exception, eh::Exception)*/;

      std::string
      get_text_token(
        in_addr_t client_address) const
        /*throw(Exception, eh::Exception)*/;

    private:
      std::size_t
      current_key_pos_(const Generics::Time& now) const noexcept;

      std::string
      serialize_(const unsigned char* ctoken) const;

      Commons::AesEncryptKey
      make_key_(std::string base64_buf)
        /*throw(Exception, eh::Exception)*/;

    protected:
      virtual
      ~SecTokenGenerator() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<SecTokenGenerator>
      SecTokenGenerator_var;
  }
}

#endif /* CAMPAIGNMANAGER_SECTOKEN_HPP_ */
