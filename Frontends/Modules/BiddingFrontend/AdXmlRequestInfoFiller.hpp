#pragma once

#include <string>

#include "CampaignManagerTypes.hpp"
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>

#include "HashMaps.hpp"

namespace AdServer::Bidding
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
      RequestInfo& request_info,
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
    using ParamProcessorMap = SubStringHashAdapterFlatMap<RequestParamProcessor_var>;

  protected:
    void
    add_param_processor_(const String::SubString& name, RequestParamProcessor* processor)
      noexcept;

  protected:
    RequestInfoFiller* request_info_filler_;
    ParamProcessorMap param_processors_;
  };
}
