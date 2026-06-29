#pragma once

#include <string>
#include <memory_resource>

#include "CampaignManagerTypes.hpp"
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <Frontends/FrontendCommons/HttpResponse.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>

namespace AdServer::Bidding
{
  struct RequestInfo;
  using PmrString = std::pmr::string;
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
