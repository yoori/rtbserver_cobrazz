#ifndef BIDDINGFRONTENDGRPC_ADXMLBIDREQUESTTASK_HPP
#define BIDDINGFRONTENDGRPC_ADXMLBIDREQUESTTASK_HPP

// STD
#include <iostream>

// THIS
#include <Frontends/Modules/BiddingFrontendGrpc/BidRequestTask.hpp>

namespace AdServer::Bidding::Grpc
{
  class AdXmlBidRequestTask final : public BidRequestTask
  {
  public:
    AdXmlBidRequestTask(
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
    ~AdXmlBidRequestTask() override = default;

    void fill_by_adxml_request_(
      FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      RequestInfo& request_info,
      std::string& keywords,
      const FrontendCommons::HttpRequest& request) noexcept;

    void fill_response_(
      std::ostream& response_ostr,
      const RequestInfo& request_info,
      const FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult& campaign_match_result) noexcept;

    void fill_response_adslot_(
      std::ostream& response_ostr,
      const AdServer::CampaignSvcs::Proto::AdSlotResult& ad_slot_result) noexcept;

    void add_xml_escaped_string_(
      std::ostream& response_ostr,
      const std::string& str) noexcept;

  private:
    std::string uri_;
  };
} // namespace AdServer::Bidding::Grpc

#endif /*BIDDINGFRONTENDGRPC_ADXMLBIDREQUESTTASK_HPP*/
