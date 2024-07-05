#include <optional>

#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <Commons/DecimalUtils.hpp>

#include "RequestInfoFiller.hpp"

namespace AdServer
{
namespace ImprTrack
{
  namespace
  {
    namespace Aspect
    {
      const char IMPR_TRACK_FRONTEND[] = "ImprTrackFrontend";
    }

    namespace Param
    {
      const String::SubString GLOBAL_REQUEST_ID("rid");
      const String::SubString REQUEST_ID("irid");
      const String::SubString OLD_REQUEST_ID("requestid");
      const String::SubString CREATIVE_LIST("ad");
      const String::SubString USER_ID_DISTRIBUTION_HASH("h");
      const String::SubString PUBLISHER_ACCOUNT_ID("aid");
      const String::SubString PUBLISHER_SITE_ID("sid");
      const String::SubString USER_ID("u");
      const String::SubString SOURCE_ID("src");
      const String::SubString REQUEST_TYPE("t");
      const String::SubString COLOCATION_ID("colo");
      const String::SubString VIEWABILITY("v");
      const String::SubString SET_COOKIE("sc");

      const String::SubString EXTERNAL_USER_ID("xid");

      const String::SubString CUSTOM_ACTION_NAME("nm");
      const String::SubString BID_TIME("bt");

      const String::SubString REDIRECT_URL("r");

      const String::SubString DATA("d");

      // cost variants
      const String::SubString OPEN_SETTLE_PRICE("p");
      const String::SubString OPEN_SETTLE_PRICE2("p2");
      const String::SubString GOOGLE_SETTLE_PRICE("gp");
      const String::SubString OPENX_SETTLE_PRICE("oxp");
      const String::SubString LIVERAIL_SETTLE_PRICE("lrp");
      const String::SubString PUBLISHER_PIXEL_ACCOUNT_IDS("paid");

      // debug params
      const String::SubString DEBUG_CURRENT_TIME("debug-time");
      const String::SubString IP_ADDRESS("debug.ip");
    }

    namespace Cookie
    {
      //const String::SubString USER_ID("uid");
      const String::SubString COHORT("ct");
    }

    namespace Header
    {
      const String::SubString REM_HOST(".remotehost");
      const String::SubString REFERER("referer");
      const String::SubString SECURE("secure");
    }

    typedef const String::AsciiStringManip::Char2Category<',', ' '>
      ListParameterSepCategory;

    inline
    uint64_t
    ntohll(uint64_t host_int)
    {
#if defined(__LITTLE_ENDIAN)
      return
        static_cast<uint64_t>(ntohl(static_cast<uint32_t>(host_int >> 32))) |
        (static_cast<uint64_t>(ntohl(static_cast<uint32_t>(host_int))) << 32);
#elif defined(__BIG_ENDIAN)
      return host_int;
#else
      #error Could not determine endianness.
#endif
    }

    bool
    decode_openx_price(
      uint64_t& real_price,
      const String::SubString& base64_src,
      const RequestInfoFiller::EncryptionKeys& keys)
    {
      const unsigned char* encription_key = keys.openx_encryption_key.get();
      const unsigned long& encription_key_size = keys.openx_encryption_key_size;
      const unsigned char* integrity_key = keys.openx_integrity_key.get();
      const unsigned long& integrity_key_size = keys.openx_integrity_key_size;

      const unsigned int IV_SIZE = 16;
      const unsigned int CIPHERTEXT_SIZE = 8;
      const unsigned int INTEGRITY_SIZE = 4;
      const unsigned int ALL_LENGTH = IV_SIZE + CIPHERTEXT_SIZE + INTEGRITY_SIZE;
      const unsigned int HMAC_SHA1_HASH_SIZE = 20;

      std::string encrypted_value;

      try
      {
        String::StringManip::base64mod_decode(
          encrypted_value,
          base64_src,
          false // padding
          );
      }
      catch(const eh::Exception&)
      {
        return false;
      }

      if(encrypted_value.size() < ALL_LENGTH)
      {
        return false;
      }

      // copy bidid + key
      unsigned char pad[HMAC_SHA1_HASH_SIZE];
      unsigned int pad_size;
      HMAC(
        EVP_sha1(),
        encription_key,
        encription_key_size,
        reinterpret_cast<const unsigned char*>(encrypted_value.data()),
        IV_SIZE,
        pad,
        &pad_size);

      unsigned char unciphered_iv[CIPHERTEXT_SIZE + IV_SIZE];

      for(unsigned int i = 0; i < CIPHERTEXT_SIZE; ++i)
      {
        unciphered_iv[i] = pad[i] ^ static_cast<unsigned char>(
          encrypted_value[IV_SIZE + i]);
      }

      ::memcpy(
        unciphered_iv + CIPHERTEXT_SIZE,
        encrypted_value.data(),
        IV_SIZE);

      unsigned char signature[HMAC_SHA1_HASH_SIZE];
      unsigned int signature_size;
      HMAC(
        EVP_sha1(),
        integrity_key,
        integrity_key_size,
        unciphered_iv,
        CIPHERTEXT_SIZE + IV_SIZE,
        signature,
        &signature_size);

      // check signature
      if(::memcmp(
           signature,
           encrypted_value.data() + IV_SIZE + CIPHERTEXT_SIZE,
           INTEGRITY_SIZE) != 0)
      {
        std::cerr << "Incorrect signature" << std::endl;
        return false;
      }

      real_price =
        (static_cast<uint64_t>(
          htonl(*reinterpret_cast<const uint32_t*>(unciphered_iv))) << 32) |
        static_cast<uint64_t>(
          htonl(*(reinterpret_cast<const uint32_t*>(unciphered_iv) + 1)));

      return true;
    }

    bool
    decode_winning_price(
      int64_t& real_price,
      const String::SubString& base64_src,
      const unsigned char* encryption_key,
      const unsigned long& encryption_key_size,
      const unsigned char* integrity_key,
      const unsigned long& integrity_key_size)
    {
      static const int INITIALIZATION_VECTOR_SIZE = 16;
      static const int CIPHER_TEXT_SIZE = 8;
      static const int SIGNATURE_SIZE = 4;
      static const int HASH_OUTPUT_SIZE = 20;
      const unsigned int ALL_LENGTH = INITIALIZATION_VECTOR_SIZE +
        CIPHER_TEXT_SIZE + SIGNATURE_SIZE;

      std::string encrypted_value;

      try
      {
        String::StringManip::base64mod_decode(
          encrypted_value,
          base64_src,
          false // padding
          );
      }
      catch(const eh::Exception&)
      {
        return false;
      }

      if(encrypted_value.size() < ALL_LENGTH)
      {
        return false;
      }

      // Compute plaintext.
      const uint8_t* initialization_vector = reinterpret_cast<const uint8_t*>(
        encrypted_value.data());
      // len(ciphertext_bytes) = 8 bytes
      const uint8_t* ciphertext_bytes =
        initialization_vector + INITIALIZATION_VECTOR_SIZE;
      // signature = initialization_vector + INITIALIZATION_VECTOR_SIZE(16) + CIPHER_TEXT_SIZE(8)
      // len(signature) = 4 bytes
      const uint8_t* signature = ciphertext_bytes + CIPHER_TEXT_SIZE;

      uint32_t pad_size = HASH_OUTPUT_SIZE;
      uint8_t price_pad[HASH_OUTPUT_SIZE];

      // get price_pad using openssl/hmac.h
      if (!HMAC(
            EVP_sha1(),
            encryption_key,
            encryption_key_size,
            initialization_vector,
            INITIALIZATION_VECTOR_SIZE,
            price_pad,
            &pad_size))
      {
        return false;
      }

      uint8_t plaintext_bytes[CIPHER_TEXT_SIZE];
      for(int32_t i = 0; i < CIPHER_TEXT_SIZE; ++i)
      {
        plaintext_bytes[i] = price_pad[i] ^ ciphertext_bytes[i];
      }

      // value = ntohllprice_pad ^ ciphertext_bytes)
      ::memcpy(&real_price, plaintext_bytes, CIPHER_TEXT_SIZE);
      real_price = ntohll(real_price);  // Switch to host byte order.

      // Verify integrity bits.
      uint32_t integrity_hash_size = HASH_OUTPUT_SIZE;
      uint8_t integrity_hash[HASH_OUTPUT_SIZE];
      const int32_t INPUT_MESSAGE_SIZE = CIPHER_TEXT_SIZE + INITIALIZATION_VECTOR_SIZE;
      uint8_t input_message[INPUT_MESSAGE_SIZE];

      ::memcpy(input_message, plaintext_bytes, CIPHER_TEXT_SIZE);
      ::memcpy(input_message + CIPHER_TEXT_SIZE,
        initialization_vector,
        INITIALIZATION_VECTOR_SIZE);

      if(!HMAC(
           EVP_sha1(),
           integrity_key,
           integrity_key_size,
           input_message,
           INPUT_MESSAGE_SIZE,
           integrity_hash,
           &integrity_hash_size))
      {
        return false;
      }

      return ::memcmp(integrity_hash, signature, SIGNATURE_SIZE) == 0;
    }


    struct VerifyTypeParamProcessor:
      public FrontendCommons::RequestParamProcessor<RequestInfo>
    {
      void
      process(RequestInfo& request_info, const String::SubString& value)
        const
      {
        if(value == "i")
        {
          request_info.verify_type = AdServer::CampaignSvcs::RVT_IMPRESSION;
        }
        else if(value == "b")
        {
          request_info.verify_type = AdServer::CampaignSvcs::RVT_IMPRESSION;
          request_info.use_template_file = true;
        }
        else if(value == "n")
        {
          request_info.verify_type = AdServer::CampaignSvcs::RVT_NOTICE;
        }
        else if(value == "c")
        {
          request_info.verify_type = AdServer::CampaignSvcs::RVT_CUSTOM;
        }
      }
    };

    class OpenPriceProcessor:
      public FrontendCommons::RequestParamProcessor<RequestInfo>
    {
    public:
      OpenPriceProcessor(
        Logging::Logger* logger,
        uint64_t divider)
        : logger_(ReferenceCounting::add_ref(logger)),
          divider_(divider)
      {}

      void
      process(RequestInfo& request_info, const String::SubString& value)
        const
      {
        try
        {
          CampaignSvcs::ExtRevenueDecimal ex_pub_imp_revenue(
            Commons::extract_decimal<CampaignSvcs::ExtRevenueDecimal>(value, Generics::DMR_ROUND, true));

          // OpenPriceProcessor only non negative price allowed here
          if (ex_pub_imp_revenue < CampaignSvcs::ExtRevenueDecimal::ZERO)
          {
            Stream::Error ostr;
            ostr << "Non correct price (negative)'" << value << "'";

            logger_->log(ostr.str(),
              Logging::Logger::NOTICE,
              Aspect::IMPR_TRACK_FRONTEND,
              "ADS-PROT-7700");

            return;
          }

          ex_pub_imp_revenue = CampaignSvcs::ExtRevenueDecimal::div(
            ex_pub_imp_revenue,
            CampaignSvcs::ExtRevenueDecimal(false, divider_, 0));

          ex_pub_imp_revenue.ceil(CampaignSvcs::RevenueDecimal::FRACTION_RANK);

          request_info.pub_imp_revenue = CampaignSvcs::RevenueDecimal(
            ex_pub_imp_revenue.str());

          request_info.pub_imp_revenue_type = CampaignSvcs::RT_ABSOLUTE;
          request_info.request_type = CampaignSvcs::AR_OPENRTB;
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "Non correct price '" << value << "': " << ex.what();

          logger_->log(ostr.str(),
            Logging::Logger::NOTICE,
            Aspect::IMPR_TRACK_FRONTEND,
            "ADS-PROT-7700");
        }
      }

    private:
      Logging::Logger_var logger_;
      uint64_t divider_;
    };

    class LiveRailPriceProcessor:
      public FrontendCommons::RequestParamProcessor<RequestInfo>
    {
    public:
      LiveRailPriceProcessor(
        Logging::Logger* logger)
        : logger_(ReferenceCounting::add_ref(logger))
      {}

      void
      process(RequestInfo& request_info, const String::SubString& value)
        const
      {
        try
        {
          request_info.pub_imp_revenue =
            CampaignSvcs::RevenueDecimal(value);

          static const CampaignSvcs::RevenueDecimal MAX_LRP(false, 1, 0);

          if (request_info.pub_imp_revenue > MAX_LRP)
          {
            request_info.pub_imp_revenue = MAX_LRP;

            Stream::Error ostr;
            ostr << "Non correct price share '" << value << "': greater than '1.0'. Set it to '1.0'.";

            logger_->log(ostr.str(),
              Logging::Logger::NOTICE,
              Aspect::IMPR_TRACK_FRONTEND,
              "ADS-PROT-7700");
          }

          request_info.pub_imp_revenue_type = CampaignSvcs::RT_SHARE;
          request_info.request_type = CampaignSvcs::AR_OPENRTB;
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << "Non correct price share '" << value << "': " << ex.what();

          logger_->log(ostr.str(),
            Logging::Logger::NOTICE,
            Aspect::IMPR_TRACK_FRONTEND,
            "ADS-PROT-7700");
        }
      }

    private:
      Logging::Logger_var logger_;
    };

   class UuidParamProcessor:
      public RequestInfoParamProcessor
    {
    public:
      UuidParamProcessor(
        Generics::Uuid RequestInfo::* field)
        noexcept
        : field_(field)
      {}

      virtual void process(
        RequestInfo& request_info,
        const String::SubString& value) const
      {
        try
        {
          request_info.*field_ = Generics::Uuid(value);
        }
        catch(const eh::Exception& ex)
        {
          throw RequestInfoFiller::InvalidParamException(ex.what());
        }
      }

    protected:
      Generics::Uuid RequestInfo::* field_;
    };
  }

  class CreativeListParamProcessor: public RequestInfoParamProcessor
  {
  public:
    virtual void process(
      RequestInfo& request_info,
      const String::SubString& value) const
    {
      String::StringManip::Splitter<ListParameterSepCategory> tokenizer(value);
      String::SubString cur;
      while(tokenizer.get_token(cur))
      {
        RequestInfo::CreativeInfo creative_info;
        creative_info.ctr = CampaignSvcs::RevenueDecimal::ZERO;

        String::SubString::SizeType sep_pos = cur.find(':');

        if(sep_pos != String::SubString::NPOS)
        {
          if(!String::StringManip::str_to_int(
               cur.substr(0, sep_pos), creative_info.ccid))
          {
            throw RequestInfoFiller::InvalidParamException("");
          }

          String::SubString ctr_str = cur.substr(sep_pos + 1);
          try
          {
            creative_info.ctr = CampaignSvcs::RevenueDecimal(ctr_str);
          }
          catch(...)
          {
            throw RequestInfoFiller::InvalidParamException("");
          }

          if(creative_info.ctr < AdServer::CampaignSvcs::RevenueDecimal::ZERO ||
            creative_info.ctr > AdServer::CampaignSvcs::REVENUE_ONE)
          {
            creative_info.ctr = AdServer::CampaignSvcs::RevenueDecimal::ZERO;
          }
        }
        else
        {
          if(!String::StringManip::str_to_int(cur, creative_info.ccid))
          {
            throw RequestInfoFiller::InvalidParamException("");
          }
        }

        request_info.creatives.push_back(creative_info);
      }
    }
  };

  RequestInfoFiller::RequestInfoFiller(
    Logging::Logger* logger,
    const char* geo_ip_path,
    CommonModule* common_module,
    unsigned long colo_id,
    const RequestInfoFiller::EncryptionKeys* default_keys,
    const RequestInfoFiller::EncryptionKeysMap& account_keys,
    const RequestInfoFiller::EncryptionKeysMap& site_keys)
    /*throw(eh::Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      common_module_(ReferenceCounting::add_ref(common_module)),
      colo_id_(colo_id),
      default_keys_(ReferenceCounting::add_ref(default_keys)),
      account_keys_(account_keys),
      site_keys_(site_keys)
  {
    static const char* FUN = "RequestInfoFiller::RequestInfoFiller()";

    if(geo_ip_path)
    {
      try
      {
        ip_map_.reset(new GeoIPMapping::IPMapCity2(geo_ip_path));
      }
      catch (const GeoIPMapping::IPMap::Exception& e)
      {
        Stream::Error ostr;
        ostr << FUN << ": GeoIPMapping::IPMap::Exception caught: " << e.what();

        logger->log(ostr.str(),
          Logging::Logger::CRITICAL,
          Aspect::IMPR_TRACK_FRONTEND,
          "ADS-IMPL-102");
      }
    }

    add_processor_(false, true, Param::GLOBAL_REQUEST_ID,
      new FrontendCommons::UuidParamProcessor<RequestInfo>(
        &RequestInfo::common_request_id));

    RequestInfoParamProcessor_var request_id_processor =
      new FrontendCommons::ContainerParamProcessor<
        RequestInfo,
        RequestIdList,
        ListParameterSepCategory,
        FrontendCommons::RequestIdConverter>(
          &RequestInfo::request_ids);

    add_processor_(false, true, Param::REQUEST_ID,
      ReferenceCounting::add_ref(request_id_processor));
    add_processor_(false, true, Param::OLD_REQUEST_ID,
      ReferenceCounting::add_ref(request_id_processor));
    add_processor_(false, true, Param::CREATIVE_LIST,
      new CreativeListParamProcessor());

    add_processor_(false, true, Param::USER_ID_DISTRIBUTION_HASH,
      new FrontendCommons::NumberParamProcessor<
        RequestInfo,
        std::optional<unsigned long>,
        unsigned long>(
        &RequestInfo::user_id_hash_mod));
    add_processor_(false, true, Param::PUBLISHER_ACCOUNT_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::publisher_account_id));
    add_processor_(false, true, Param::PUBLISHER_SITE_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::publisher_site_id));
    add_processor_(false, true, Param::USER_ID,
      new UuidParamProcessor(
        &RequestInfo::current_user_id));
    add_processor_(false, true, Param::SOURCE_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::source_id));
    add_processor_(false, true, Param::CUSTOM_ACTION_NAME,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::action_name));
    add_processor_(false, true, Param::REQUEST_TYPE,
      new VerifyTypeParamProcessor());
    add_processor_(false, true, Param::COLOCATION_ID,
      new FrontendCommons::NumberParamProcessor<RequestInfo, unsigned long>(
        &RequestInfo::colo_id));
    add_processor_(false, true, Param::VIEWABILITY,
      new FrontendCommons::NumberParamProcessor<RequestInfo, long>(
        &RequestInfo::viewability));
    add_processor_(false, true, Param::SET_COOKIE,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::set_cookie));
    add_processor_(false, true, Param::PUBLISHER_PIXEL_ACCOUNT_IDS,
      new FrontendCommons::NumberContainerParamProcessor<
        RequestInfo,
        RequestInfo::AccountIdList,
        String::AsciiStringManip::SepComma>(
          &RequestInfo::pubpixel_accounts));

    add_processor_(false, true, Param::EXTERNAL_USER_ID,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::external_user_id));

    add_processor_(false, true, Param::OPEN_SETTLE_PRICE,
      new OpenPriceProcessor(logger, 1000));
    add_processor_(false, true, Param::OPEN_SETTLE_PRICE2,
      new OpenPriceProcessor(logger, 1000000));
    add_processor_(false, true, Param::LIVERAIL_SETTLE_PRICE,
      new LiveRailPriceProcessor(logger));
    add_processor_(false, true, Param::GOOGLE_SETTLE_PRICE,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::google_encoded_price));
    add_processor_(false, true, Param::OPENX_SETTLE_PRICE,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::openx_encoded_price));
    add_processor_(false, true, Param::BID_TIME,
      new FrontendCommons::TimeParamProcessor<RequestInfo>(
        &RequestInfo::bid_time));

    add_processor_(false, true, Param::DEBUG_CURRENT_TIME,
      new FrontendCommons::TimeParamProcessor<RequestInfo>(
        &RequestInfo::time));
    add_processor_(false, true, Param::IP_ADDRESS,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::peer_ip));
    add_processor_(false, true, Param::REDIRECT_URL,
      new FrontendCommons::UrlParamProcessor<RequestInfo>(
        &RequestInfo::redirect_url));

    add_processor_(false, true, Param::DATA,
      new FrontendCommons::DataParamProcessor<RequestInfo>(param_processors_));
    
    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::CLIENT_ID,
      RequestInfoParamProcessor_var(
        new FrontendCommons::SignedUuidParamProcessor<RequestInfo>(
          &RequestInfo::actual_user_id,
          common_module->user_id_controller(),
          UserIdController::PERSISTENT))));
    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::CLIENT_ID2,
      RequestInfoParamProcessor_var(
        new FrontendCommons::SignedUuidParamProcessor<RequestInfo>(
          &RequestInfo::actual_user_id,
          common_module->user_id_controller(),
          UserIdController::PERSISTENT,
          false))));

    cookie_processors_.insert(std::make_pair(
      Cookie::COHORT,
      RequestInfoParamProcessor_var(
        new FrontendCommons::StringParamProcessor<RequestInfo>(
          &RequestInfo::cohort))));
    cookie_processors_.insert(std::make_pair(
      FrontendCommons::Cookies::OPTOUT,
      RequestInfoParamProcessor_var(
        new FrontendCommons::OptOutParamProcessor<RequestInfo>(
          &RequestInfo::user_status))));
    add_processor_(true, false, Header::REM_HOST,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::peer_ip));
    add_processor_(true, false, Header::REFERER,
      new FrontendCommons::StringParamProcessor<RequestInfo>(
        &RequestInfo::referer));
    add_processor_(true, false, Header::SECURE,
      new FrontendCommons::BoolParamProcessor<RequestInfo>(
        &RequestInfo::secure));
  }

  void
  RequestInfoFiller::fill(
    RequestInfo& request_info,
    const FrontendCommons::HttpRequest& request)
    /*throw(InvalidParamException, Exception)*/
  {
    request_info.time = Generics::Time::get_time_of_day();
    request_info.bid_time = request_info.time;

    cookies_processing_(request_info, request);
    headers_processing_(request_info, request);
    params_processing_(request_info, request);

    if(request_info.viewability > 1 || request_info.viewability < -1)
    {
      request_info.viewability = -1;
    }

    if (!request_info.openx_encoded_price.empty() ||
      !request_info.google_encoded_price.empty())
    {
      const EncryptionKeys* keys = 0;

      if (request_info.publisher_site_id)
      {
        EncryptionKeysMap::const_iterator site_it =
          site_keys_.find(request_info.publisher_site_id);

        if (site_it != site_keys_.end())
        {
          keys = site_it->second;
        }
      }

      if(!keys)
      {
        EncryptionKeysMap::const_iterator acc_it =
          account_keys_.find(request_info.publisher_account_id);

        if(acc_it != account_keys_.end())
        {
          keys = acc_it->second;
        }
      }

      if(!keys)
      {
        keys = default_keys_;
      }

      // decode prices with using account_id
      if(!request_info.openx_encoded_price.empty())
      {
        if(keys->openx_encryption_key_size > 0 &&
          keys->openx_integrity_key_size > 0)
        {
          uint64_t settle_price = 0;

          if(decode_openx_price(
            settle_price,
            request_info.openx_encoded_price,
            *keys))
          {
            request_info.request_type = CampaignSvcs::AR_OPENX;
            request_info.pub_imp_revenue_type = CampaignSvcs::RT_ABSOLUTE;
            request_info.pub_imp_revenue = CampaignSvcs::RevenueDecimal::div(
              CampaignSvcs::RevenueDecimal(false, settle_price, 0),
              CampaignSvcs::RevenueDecimal(false, 1000000, 0));
          }
          else
          {
            Stream::Error ostr;
            ostr << "Non correct OpenX price '" <<
              request_info.openx_encoded_price << "'";

            logger()->log(ostr.str(),
              Logging::Logger::NOTICE,
              Aspect::IMPR_TRACK_FRONTEND,
              "ADS-PROT-7700");
          }
        }
        else
        {
          logger()->log(String::SubString(
            "Request type/account_id mismatch, openx keys not found"),
            Logging::Logger::NOTICE,
            Aspect::IMPR_TRACK_FRONTEND);
        }
      }

      if(!request_info.google_encoded_price.empty())
      {
        if(keys->google_encryption_key_size > 0 &&
          keys->google_integrity_key_size > 0)
        {
          int64_t settle_price = 0;

          if(decode_winning_price(
            settle_price,
            request_info.google_encoded_price,
            keys->google_encryption_key.get(),
            keys->google_encryption_key_size,
            keys->google_integrity_key.get(),
            keys->google_integrity_key_size))
          {
            request_info.request_type = CampaignSvcs::AR_GOOGLE;
            request_info.pub_imp_revenue_type = CampaignSvcs::RT_ABSOLUTE;
            request_info.skip = settle_price == 0;
            request_info.pub_imp_revenue = CampaignSvcs::RevenueDecimal::div(
              CampaignSvcs::RevenueDecimal(false, settle_price, 0),
              CampaignSvcs::RevenueDecimal(false, 1000000, 0));
          }
          else
          {
            Stream::Error ostr;
            ostr << "Non correct Google price '" <<
              request_info.google_encoded_price << "' for account '" <<
              request_info.publisher_account_id << "'";

            logger()->log(ostr.str(),
              Logging::Logger::NOTICE,
              Aspect::IMPR_TRACK_FRONTEND,
              "ADS-PROT-7700");
          }
        }
        else
        {
          Stream::Error ostr;
          ostr << "Request type/account_id mismatch, " <<
            "google keys not found for account '" <<
            request_info.publisher_account_id << "'";
          
          logger()->log(ostr.str(),
            Logging::Logger::NOTICE,
            Aspect::IMPR_TRACK_FRONTEND);
        }
      }
    }

    if (request_info.pub_imp_revenue < CampaignSvcs::RevenueDecimal::ZERO)
    {
      logger()->stream(Logging::Logger::NOTICE, Aspect::IMPR_TRACK_FRONTEND) <<
        "RequestInfoFiller::fill(): get negative pub_imp_revenue: " << 
        request_info.pub_imp_revenue << ", set pub_imp_revenue to 0";

      request_info.pub_imp_revenue = CampaignSvcs::RevenueDecimal::ZERO;
      // Also skip to default values also pub_imp_revenue_type and request_type?
      request_info.pub_imp_revenue_type =  CampaignSvcs::RT_NONE;
      request_info.request_type = CampaignSvcs::AR_NORMAL;
    }

    if(request_info.colo_id == 0)
    {
      // resolve colo by IP + cohort
      FrontendCommons::IPMatcher_var ip_matcher = common_module_->ip_matcher();

      try
      {
        FrontendCommons::IPMatcher::MatchResult ip_match_result;
        if(ip_matcher.in() && !request_info.peer_ip.empty() &&
          ip_matcher->match(
            ip_match_result,
            request_info.peer_ip,
            request_info.cohort))
        {
          request_info.colo_id = ip_match_result.colo_id;
        }
      }
      catch(const FrontendCommons::IPMatcher::InvalidParameter&)
      {}
    }

    if(request_info.colo_id == 0)
    {
      request_info.colo_id = colo_id_;
    }

    if (request_info.user_status == AdServer::CampaignSvcs::US_OPTOUT ||
      !request_info.set_cookie)
    {
      request_info.actual_user_id = AdServer::Commons::UserId();
    }
  }

  void
  RequestInfoFiller::params_processing_(
    RequestInfo& request_info,
    const FrontendCommons::HttpRequest& request)
    /*throw(InvalidParamException, Exception)*/
  {
    const HTTP::ParamList& params = request.params();

    for(HTTP::ParamList::const_iterator it = params.begin();
        it != params.end(); ++it)
    {
      ParamProcessorMap::const_iterator param_it =
        param_processors_.find(it->name);

      if(param_it != param_processors_.end())
      {
        try
        {
          param_it->second->process(request_info, it->value);
        }
        catch(const eh::Exception&)
        {
          // skip invalid parameters
        }
      }
    }
  }

  void
  RequestInfoFiller::add_processor_(
    bool headers,
    bool parameters,
    const String::SubString& name,
    RequestInfoParamProcessor* processor)
    noexcept
  {
    RequestInfoParamProcessor_var processor_ptr(processor);

    if(headers)
    {
      header_processors_.insert(
        std::make_pair(name, processor_ptr));
    }

    if(parameters)
    {
      param_processors_.insert(
        std::make_pair(name, processor_ptr));
    }
  }

  void
  RequestInfoFiller::headers_processing_(
    RequestInfo& request_info,
    const FrontendCommons::HttpRequest& request)
    /*throw(InvalidParamException, Exception)*/
  {
    const HTTP::SubHeaderList& headers = request.headers();

    for (HTTP::SubHeaderList::const_iterator it = headers.begin();
      it != headers.end(); ++it)
    {
      std::string header_name = it->name.str();
      String::AsciiStringManip::to_lower(header_name);

      ParamProcessorMap::const_iterator param_it =
        header_processors_.find(header_name);

      if(param_it != header_processors_.end())
      {
        param_it->second->process(request_info, it->value);
      }
    }
  }

  void
  RequestInfoFiller::cookies_processing_(
    RequestInfo& request_info,
    const FrontendCommons::HttpRequest& request)
    /*throw(InvalidParamException, Exception)*/
  {
    static const char* FUN = "RequestInfoFiller::cookies_processing_()";

    HTTP::CookieList cookies;

    try
    {
      cookies.load_from_headers(request.headers());
    }
    catch(HTTP::CookieList::InvalidArgument& ex)
    {
      if(logger()->log_level() >= Logging::Logger::TRACE)
      {
        Stream::Error ostr;
        ostr << FUN << ": caught HTTP::CookieList::InvalidArgument: " <<
          ex.what();
        logger()->log(ostr.str(),
          Logging::Logger::TRACE,
          Aspect::IMPR_TRACK_FRONTEND);
      }

      throw InvalidParamException("");
    }

    for(HTTP::CookieList::const_iterator it = cookies.begin();
        it != cookies.end(); ++it)
    {
      ParamProcessorMap::const_iterator param_it =
        cookie_processors_.find(it->name);

      if(param_it != cookie_processors_.end())
      {
        param_it->second->process(request_info, it->value);
      }
    }
  }
}
}
