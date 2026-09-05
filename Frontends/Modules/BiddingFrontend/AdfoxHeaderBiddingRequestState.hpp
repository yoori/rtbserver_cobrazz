#pragma once

#include <string>

#include "AdfoxRequestInfoFiller.hpp"
#include "BidRequestState.hpp"

namespace AdServer::Bidding
{
  class AdfoxHeaderBiddingRequestState final: public BidRequestState
  {
  public:
    AdfoxHeaderBiddingRequestState(
      BiddingFrontendCore* bid_frontend,
      FCGI::HttpRequestHolder_var request_holder,
      FCGI::BaseHttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time)
      /*throw(Invalid)*/;

    void
    print_request(std::ostream& out) const noexcept override;

    bool
    read_request() noexcept override;

    bool
    write_response(
      const AdServer::Bidding::CampaignManager::RequestCreativeResult& campaign_match_result)
      noexcept override;

    void
    write_empty_response(unsigned int code, bool response_claimed = false) noexcept override;

    void
    clear() noexcept override;

  private:
    void
    write_json_response_(int code, std::string&& body, bool response_claimed = false) noexcept;

    std::string
    make_response_(
      const AdServer::Bidding::CampaignManager::RequestCreativeResult& campaign_match_result)
      const;

    std::string uri_;
    AdfoxRequestContext context_;
  };
}
