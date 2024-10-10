#ifndef BIDDINGFRONTENDGRPC_APPNEXUSBIDREQUESTTASK_HPP
#define BIDDINGFRONTENDGRPC_APPNEXUSBIDREQUESTTASK_HPP

// THIS
#include <Frontends/Modules/BiddingFrontendGrpc/BidRequestTask.hpp>

namespace AdServer::Bidding::Grpc
{
  class AppNexusBidRequestTask final : public BidRequestTask
  {
  public:
    AppNexusBidRequestTask(
      Frontend* bid_frontend,
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time) /*throw(Invalid)*/;

    void print_request(std::ostream& out) const noexcept override;

    // processing stages
    // read request & transform it to holder
    bool read_request() noexcept override;

    bool write_response(
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult&
        campaign_match_result) noexcept override;

    void write_empty_response(unsigned int code) noexcept override;

    void clear() noexcept override;

  protected:
    ~AppNexusBidRequestTask() override = default;

  private:
    JsonProcessingContext context_;
  };
} // namespace AdServer::Bidding::Grpc

#endif /*BIDDINGFRONTENDGRPC_APPNEXUSBIDREQUESTTASK_HPP*/