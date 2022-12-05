#ifndef FRONTENDS_PUBPIXELFRONTEND_REQUESTINFOFILLER_HPP
#define FRONTENDS_PUBPIXELFRONTEND_REQUESTINFOFILLER_HPP

#include <string>
#include <map>
#include <vector>

#include <Sync/SyncPolicy.hpp>
#include <GeoIP/IPMap.hpp>
#include <Logger/Logger.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/GnuHashTable.hpp>

#include <HTTP/Http.hpp>

#include <Commons/Containers.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <Frontends/FrontendCommons/HTTPUtils.hpp>
#include <Frontends/FrontendCommons/RequestMatchers.hpp>
#include <Frontends/FrontendCommons/UserAgentMatcher.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>

namespace AdServer
{
namespace PubPixel
{
  struct RequestInfo
  {
    RequestInfo()
      : user_status(AdServer::CampaignSvcs::US_UNDEFINED)
    {};

    AdServer::Commons::Optional<std::string> country;
    AdServer::CampaignSvcs::UserStatus user_status;
    std::string peer_ip;
    typedef std::vector<unsigned long> PublisherAccountIDs;
    PublisherAccountIDs publisher_account_ids;
  };

  typedef FrontendCommons::RequestParamProcessor<RequestInfo>
    RequestParamProcessor;

  typedef ReferenceCounting::SmartPtr<RequestParamProcessor>
    RequestParamProcessor_var;

  class RequestInfoFiller: public FrontendCommons::HTTPExceptions
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    RequestInfoFiller(
      Logging::Logger* logger,
      const char* geo_ip_path)
      /*throw(eh::Exception)*/;

    void fill(RequestInfo& request_info,
      const HTTP::SubHeaderList& headers,
      const HTTP::ParamList& params) const
      /*throw(InvalidParamException, ForbiddenException, Exception)*/;

    const Logging::Logger_var&
    logger() const noexcept
    {
      return logger_;
    }

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;
    
    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter, RequestParamProcessor_var>
      RequestParamProcessorMap;

    typedef std::unique_ptr<GeoIPMapping::IPMapCity2> IPMapPtr;

  private:
    Logging::Logger_var logger_;

    IPMapPtr ip_map_;

    RequestParamProcessorMap param_processors_;
    RequestParamProcessorMap header_processors_;
  };
} /*PubPixel*/
} /*AdServer*/

#endif /*FRONTENDS_PUBPIXELFRONTEND_REQUESTINFOFILLER_HPP*/
