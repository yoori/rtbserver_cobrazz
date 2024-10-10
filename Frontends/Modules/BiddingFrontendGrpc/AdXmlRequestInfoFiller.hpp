#ifndef BIDDINGFRONTENDGRPC_ADXMLREQUESTINFOFILLER_HPP
#define BIDDINGFRONTENDGRPC_ADXMLREQUESTINFOFILLER_HPP

// STD
#include <string>

// THIS
#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>
#include <CampaignSvcs/CampaignCommons/CampaignSvcsVersionAdapter.hpp>
#include <Frontends/FrontendCommons/GrpcCampaignManagerPool.hpp>
#include <Frontends/FrontendCommons/HttpRequest.hpp>
#include <Frontends/FrontendCommons/RequestParamProcessor.hpp>

namespace AdServer::Bidding::Grpc
{
  struct RequestInfo;
  class RequestInfoFiller;

  class AdXmlRequestInfoFiller
  {
  public:
    using RequestParams = FrontendCommons::GrpcCampaignManagerPool::RequestParams;
    using HttpRequest = FrontendCommons::HttpRequest;

  protected:
    struct Context;

    using RequestParamProcessor = FrontendCommons::RequestParamProcessor<Context>;
    using RequestParamProcessor_var = ReferenceCounting::SmartPtr<RequestParamProcessor>;
    using ParamProcessorMap = Generics::GnuHashTable<
      Generics::SubStringHashAdapter, RequestParamProcessor_var>;

  public:
    AdXmlRequestInfoFiller(RequestInfoFiller* request_info_filler);

    virtual ~AdXmlRequestInfoFiller() = default;

    void fill_by_request(
      RequestParams& request_params,
      RequestInfo& request_info,
      std::string& keywords,
      const HttpRequest& request,
      const bool require_icon,
      const String::SubString& client,
      const String::SubString& size);

  protected:
    void add_param_processor_(
      const String::SubString& name,
      RequestParamProcessor* processor) noexcept;

  protected:
    RequestInfoFiller* request_info_filler_;

    ParamProcessorMap param_processors_;
  };
} // namespace AdServer::Bidding::Grpc

#endif /*BIDDINGFRONTENDGRPC_ADXMLREQUESTINFOFILLER_HPP*/
