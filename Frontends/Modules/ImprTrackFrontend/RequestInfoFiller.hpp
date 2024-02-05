#ifndef IMPRTRACKFRONTEND_REQUESTINFOFILLER_HPP_
#define IMPRTRACKFRONTEND_REQUESTINFOFILLER_HPP_

#include <Generics/GnuHashTable.hpp>
#include <GeoIP/IPMap.hpp>
#include <HTTP/Http.hpp>
#include <Logger/Logger.hpp>

#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>

#include <xsd/Frontends/FeConfig.hpp>

namespace AdServer
{
namespace ImprTrack
{
  typedef std::list<AdServer::Commons::RequestId> RequestIdList;

  struct RequestInfo
  {
    typedef std::list<unsigned long> AccountIdList;

    struct CreativeInfo
    {
      CreativeInfo()
        : ccid(0),
          ctr(CampaignSvcs::RevenueDecimal::ZERO)
      {}

      unsigned long ccid;
      CampaignSvcs::RevenueDecimal ctr;
    };

    typedef std::list<CreativeInfo> CreativeList;

    RequestInfo()
      : verify_type(AdServer::CampaignSvcs::RVT_IMPRESSION),
        request_type(CampaignSvcs::AR_NORMAL),
        pub_imp_revenue_type(CampaignSvcs::RT_NONE),
        pub_imp_revenue(AdServer::CampaignSvcs::RevenueDecimal::ZERO),
        publisher_account_id(0),
        publisher_site_id(0),
        colo_id(0),
        user_status(AdServer::CampaignSvcs::US_UNDEFINED),
        viewability(-1),
        skip(false),
        use_template_file(false),
        set_cookie(true),
        secure(false)
    {}

    AdServer::CampaignSvcs::RequestVerificationType verify_type;
    Commons::RequestId common_request_id;
    // creatives and request_ids ordered
    CreativeList creatives;
    RequestIdList request_ids;
    AdServer::Commons::Optional<unsigned long> user_id_hash_mod;
    AdServer::Commons::UserId current_user_id;
    AdServer::Commons::UserId actual_user_id;
    Generics::Time time;
    Generics::Time bid_time;
    AdServer::CampaignSvcs::AdRequestType request_type;
    AdServer::CampaignSvcs::RevenueType pub_imp_revenue_type;
    AdServer::CampaignSvcs::RevenueDecimal pub_imp_revenue;
    unsigned long publisher_account_id;
    unsigned long publisher_site_id;
    unsigned long colo_id;
    std::string peer_ip;
    std::string cohort;
    AdServer::CampaignSvcs::UserStatus user_status;
    std::string referer;
    AccountIdList pubpixel_accounts;
    long viewability;
    bool skip;
    std::string action_name;

    std::string source_id;

    std::string external_user_id;

    std::string google_encoded_price;
    std::string openx_encoded_price;

    bool use_template_file;

    bool set_cookie;

    std::string redirect_url;

    bool secure;
  };

  typedef FrontendCommons::RequestParamProcessor<RequestInfo>
    RequestInfoParamProcessor;

  typedef ReferenceCounting::SmartPtr<RequestInfoParamProcessor>
    RequestInfoParamProcessor_var;

  class RequestInfoFiller: public FrontendCommons::HTTPExceptions
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct EncryptionKeys: public ReferenceCounting::AtomicImpl
    {
      EncryptionKeys()
        : openx_encryption_key_size(0),
          openx_integrity_key_size(0),
          google_encryption_key_size(0),
          google_integrity_key_size(0)
      {}


      Generics::ArrayAutoPtr<unsigned char> openx_encryption_key;
      unsigned long openx_encryption_key_size;
      Generics::ArrayAutoPtr<unsigned char> openx_integrity_key;
      unsigned long openx_integrity_key_size;

      Generics::ArrayAutoPtr<unsigned char> google_encryption_key;
      unsigned long google_encryption_key_size;
      Generics::ArrayAutoPtr<unsigned char> google_integrity_key;
      unsigned long google_integrity_key_size;

    protected:
      virtual ~EncryptionKeys() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<EncryptionKeys> EncryptionKeys_var;
    typedef std::map<unsigned long, EncryptionKeys_var> EncryptionKeysMap;

    typedef Configuration::ImprTrackFeConfigurationType
      ImprTrackFeConfiguration;

  public:
    RequestInfoFiller(
      Logging::Logger* logger,
      const char* geo_ip_path,
      CommonModule* common_module,
      unsigned long colo_id,
      const RequestInfoFiller::EncryptionKeys* default_keys,
      const RequestInfoFiller::EncryptionKeysMap& account_keys,
      const RequestInfoFiller::EncryptionKeysMap& site_keys)
      /*throw(eh::Exception)*/;

    void
    fill(
      RequestInfo& request_info,
      const FrontendCommons::HttpRequest& request)
      /*throw(InvalidParamException, Exception)*/;

    const Logging::Logger_var&
    logger() const noexcept;

  private:
    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter, RequestInfoParamProcessor_var>
      ParamProcessorMap;

  private:
    void
    add_processor_(
      bool headers,
      bool parameters,
      const String::SubString& name,
      RequestInfoParamProcessor* processor)
      noexcept;

    void
    cookies_processing_(
      RequestInfo& request_info,
      const FrontendCommons::HttpRequest& request)
      /*throw(InvalidParamException, Exception)*/;

    void
    params_processing_(
      RequestInfo& request_info,
      const FrontendCommons::HttpRequest& request)
      /*throw(InvalidParamException, Exception)*/;

    void
    headers_processing_(
      RequestInfo& request_info,
      const FrontendCommons::HttpRequest& request)
      /*throw(InvalidParamException, Exception)*/;

  private:
    Logging::Logger_var logger_;
    std::unique_ptr<GeoIPMapping::IPMapCity2> ip_map_;
    CommonModule_var common_module_;
    const unsigned long colo_id_;

    const ReferenceCounting::ConstPtr<EncryptionKeys>  default_keys_;
    const EncryptionKeysMap account_keys_;
    const EncryptionKeysMap site_keys_;

    ParamProcessorMap header_processors_;
    ParamProcessorMap param_processors_;
    ParamProcessorMap cookie_processors_;
  };
}
}

namespace AdServer
{
namespace ImprTrack
{
  inline
  const Logging::Logger_var&
  RequestInfoFiller::logger() const noexcept
  {
    return logger_;
  }
}
}

#endif /* IMPRTRACKFRONTEND_REQUESTINFOFILLER_HPP_ */
