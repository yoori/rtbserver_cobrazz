#ifndef DAOBIDREQUESTTASK_HPP_
#define DAOBIDREQUESTTASK_HPP_

#include <iostream>
#include "BidRequestTask.hpp"
#include "OpenRtbBidRequestTask.hpp"

namespace AdServer
{
namespace Bidding
{
  //
  // DAOBidRequestTask
  //
  class DAOBidRequestTask: public OpenRtbBidRequestTask
  {
  public:
    DAOBidRequestTask(
      Frontend* bid_frontend,
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time)
      /*throw(Invalid)*/;

    virtual void
    print_request(std::ostream& out) const noexcept;

    // read_request used from OpenRtbBidRequestTask

    virtual bool
    write_response(
      const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult&
        campaign_match_result)
      noexcept;

    virtual void
    write_empty_response(unsigned int code)
      noexcept;

    virtual void
    clear() noexcept;

  public:
    //std::string bid_response;
    const std::string uri;

  protected:
    struct Context;

  protected:
    virtual
    ~DAOBidRequestTask() noexcept = default;

    void
    fill_by_adjson_request_(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      RequestInfo& request_info,
      std::string& keywords,
      const FrontendCommons::HttpRequest& request)
      noexcept;

    void
    fill_response_(
      std::string& response,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const AdServer::CampaignSvcs::CampaignManager::
        RequestCreativeResult& campaign_match_result)
      noexcept;

  private:
    std::string uri_;
  };
}
}

#endif /*DAOBIDREQUESTTASK_HPP_*/
