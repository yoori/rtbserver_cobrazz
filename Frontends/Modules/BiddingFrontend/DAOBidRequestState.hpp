#pragma once

#include <iostream>
#include "BidRequestState.hpp"
#include "OpenRtbBidRequestState.hpp"

namespace AdServer
{
namespace Bidding
{
  //
  // DAOBidRequestState
  //
  class DAOBidRequestState: public OpenRtbBidRequestState
  {
  public:
    DAOBidRequestState(
      Frontend* bid_frontend,
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time)
      /*throw(Invalid)*/;

    virtual void
    print_request(std::ostream& out) const noexcept;

    // read_request used from OpenRtbBidRequestState

    virtual bool
    write_response(
      const AdServer::Bidding::CampaignManager::RequestCreativeResult&
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
    ~DAOBidRequestState() noexcept = default;

    void
    fill_by_adjson_request_(
      AdServer::Bidding::CampaignManager::RequestParams& request_params,
      RequestInfo& request_info,
      std::string& keywords,
      const FCGI::HttpRequest& request)
      noexcept;

    void
    fill_response_(
      std::ostream& response_ostr,
      const RequestInfo& request_info,
      const AdServer::Bidding::CampaignManager::RequestParams& request_params,
      const AdServer::Bidding::CampaignManager::
        RequestCreativeResult& campaign_match_result)
      noexcept;

  private:
    std::string uri_;
  };
}
}
