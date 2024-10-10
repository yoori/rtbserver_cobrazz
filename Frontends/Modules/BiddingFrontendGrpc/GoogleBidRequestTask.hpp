#ifndef BIDDINGFRONTENDGRPC_GOOGLEBIDREQUESTTASK_HPP_
#define BIDDINGFRONTENDGRPC_GOOGLEBIDREQUESTTASK_HPP_

// THIS
#include <Frontends/Modules/BiddingFrontendGrpc/BidRequestTask.hpp>

namespace AdServer::Bidding::Grpc
{
  class GoogleBidRequestTask final : public BidRequestTask
  {
  public:
    GoogleBidRequestTask(
      Frontend* bid_frontend,
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time) /*throw(Invalid)*/;

    void print_request(std::ostream& out) const noexcept override;

    // processing stages
    // read request & transform it to holder
    bool read_request() noexcept override;

    // write response (convert holder to response)
    bool write_response(
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult&
        campaign_match_result) noexcept override;

    void write_empty_response(unsigned int code) noexcept override;

    void clear() noexcept override;

  protected:
    ~GoogleBidRequestTask() override = default;

  private:
    Google::BidRequest bid_request_;

    GoogleAdSlotContextArray ad_slots_context_;
  };
} // namespace AdServer::Bidding::Grpc

#endif /*BIDDINGFRONTENDGRPC_GOOGLEBIDREQUESTTASK_HPP_*/
