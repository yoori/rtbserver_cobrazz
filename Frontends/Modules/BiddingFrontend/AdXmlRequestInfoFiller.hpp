#ifndef BIDDINGFRONTEND_ADXMLREQUESTINFOFILLER_HPP_
#define BIDDINGFRONTEND_ADXMLREQUESTINFOFILLER_HPP_

#include <string>

#include <CampaignSvcs/CampaignManager/CampaignManager.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <Frontends/FrontendCommons/FCGI.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>

namespace AdServer
{
namespace Bidding
{
  struct RequestInfo;
  class RequestInfoFiller;

  class AdXmlRequestInfoFiller
  {
  public:
    AdXmlRequestInfoFiller(RequestInfoFiller* request_info_filler);

    virtual
    ~AdXmlRequestInfoFiller() = default;

    void
    fill_by_request(
      ::AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      RequestInfo& request_info,
      std::string& keywords,
      const FCGI::HttpRequest& request,
      bool require_icon,
      const String::SubString& client,
      const String::SubString& size);

  protected:
    struct Context;

    typedef FrontendCommons::RequestParamProcessor<Context>
      RequestParamProcessor;
    typedef ReferenceCounting::SmartPtr<RequestParamProcessor>
      RequestParamProcessor_var;
    typedef Generics::GnuHashTable<
      Generics::SubStringHashAdapter, RequestParamProcessor_var>
      ParamProcessorMap;

  protected:
    void
    add_param_processor_(
      const String::SubString& name,
      RequestParamProcessor* processor)
      noexcept;

  protected:
    RequestInfoFiller* request_info_filler_;
    ParamProcessorMap param_processors_;
  };
}
}

#endif /*BIDDINGFRONTEND_ADXMLREQUESTINFOFILLER_HPP_*/
