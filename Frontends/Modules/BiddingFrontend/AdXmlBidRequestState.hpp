#pragma once

#include <iostream>
#include "BidRequestState.hpp"

namespace AdServer::Bidding
{
  //
  // AdXmlBidRequestState
  //
  class AdXmlBidRequestState: public BidRequestState
  {
  public:
    AdXmlBidRequestState(
      BiddingFrontendCore* bid_frontend,
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time)
      /*throw(Invalid)*/;

    virtual void
    print_request(std::ostream& out) const noexcept;

    virtual bool
    read_request() noexcept;

    /*
    virtual bool
    fill_request_info(std::string& keywords)
      noexcept;
    */

    virtual bool
    write_response(
      const AdServer::Bidding::CampaignManager::RequestCreativeResult&
        campaign_match_result)
      noexcept;

    virtual void
    write_empty_response(
      unsigned int code,
      bool response_claimed = false)
      noexcept;

    virtual void
    clear() noexcept;

    virtual
    ~AdXmlBidRequestState() noexcept = default;

  public:
    //std::string bid_response;
    const std::string uri;

  protected:
    struct Context;

  protected:
    void
    fill_by_adxml_request_(
      RequestInfo& request_info,
      const FCGI::HttpRequest& request)
      noexcept;

    void
    fill_response_(
      std::ostream& response_ostr,
      const RequestInfo& request_info,
      const AdServer::Bidding::CampaignManager::
        RequestCreativeResult& campaign_match_result)
      noexcept;

    void
    fill_response_adslot_(
      std::ostream& response_ostr,
      const AdServer::Bidding::CampaignManager::AdSlotResult& ad_slot_result)
      noexcept;

    void
    add_xml_escaped_string_(
      std::ostream& response_ostr,
      const char* str)
      noexcept;

  private:
    std::string uri_;
  };
}
