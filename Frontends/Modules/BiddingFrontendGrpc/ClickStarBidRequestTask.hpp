#ifndef BIDDINGFRONTENDGRPC_CLICKSTARBIDREQUESTTASKGRPC_HPP_
#define BIDDINGFRONTENDGRPC_CLICKSTARBIDREQUESTTASKGRPC_HPP_

// STD
#include <iostream>

// THIS
#include <Frontends/Modules/BiddingFrontendGrpc/BidRequestTask.hpp>

namespace AdServer::Bidding::Grpc
{
  class ClickStarBidRequestTask final : public BidRequestTask
  {
  public:
    ClickStarBidRequestTask(
      Frontend* bid_frontend,
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time) /*throw(Invalid)*/;

    void print_request(std::ostream& out) const noexcept override;

    bool read_request() noexcept override;

    bool write_response(
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult&
        campaign_match_result) noexcept override;

    void write_empty_response(unsigned int code) noexcept override;

    void clear() noexcept override;

  protected:
    ~ClickStarBidRequestTask() override = default;

    void fill_response_(
      std::string& response,
      const RequestInfo& request_info,
      const FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult& campaign_match_result) noexcept;

    void fill_response_adslot_(
      AdServer::Commons::JsonFormatter* json_formatter,
      const AdServer::CampaignSvcs::Proto::AdSlotResult& ad_slot_result) noexcept;

  private:
    std::string uri_;
  };
} // namespace AdServer::Bidding::Grpc

#endif /*BIDDINGFRONTENDGRPC_CLICKSTARBIDREQUESTTASKGRPC_HPP_*/
