#ifndef ADFOXBIDREQUESTTASK_HPP_
#define ADFOXBIDREQUESTTASK_HPP_

#include <iostream>
#include "BidRequestTask.hpp"
#include "OpenRtbBidRequestTask.hpp"

namespace AdServer
{
namespace Bidding
{
  //
  // AdFoxBidRequestTask
  //
  class AdFoxBidRequestTask: public OpenRtbBidRequestTask
  {
  public:
    AdFoxBidRequestTask(
      Frontend* bid_frontend,
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::HttpResponseWriter_var response_writer,
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
    ~AdFoxBidRequestTask() noexcept = default;

    void
    fill_by_adjson_request_(
      AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      RequestInfo& request_info,
      std::string& keywords,
      const FCGI::HttpRequest& request)
      noexcept;

    void
    fill_response_(
      std::ostream& response_ostr,
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

#endif /*ADFOXBIDREQUESTTASK_HPP_*/
