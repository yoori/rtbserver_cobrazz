#ifndef BIDDINGFRONTENDGRPC_OPENRTBBIDREQUESTTASK_HPP_
#define BIDDINGFRONTENDGRPC_OPENRTBBIDREQUESTTASK_HPP_

// STD
#include <iostream>

// THIS
#include <Frontends/FrontendCommons/GrpcCampaignManagerPool.hpp>
#include <Frontends/Modules/BiddingFrontendGrpc/BidRequestTask.hpp>

namespace AdServer::Bidding::Grpc
{
  class OpenRtbBidRequestTask final : public BidRequestTask
  {
  public:
    OpenRtbBidRequestTask(
      Frontend* bid_frontend,
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time);

    void print_request(std::ostream& out) const noexcept override;

    bool read_request() noexcept override;

    bool write_response(
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult&
        campaign_match_result) noexcept override;

    void write_empty_response(unsigned int code) noexcept override;

    void clear() noexcept override;

  protected:
    ~OpenRtbBidRequestTask() override = default;

    void fill_openrtb_response_(
      std::string& response,
      const RequestInfo& request_info,
      const FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const JsonProcessingContext& context,
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult& campaign_match_result,
      bool fill_yandex_attributes) noexcept;

    void fill_yandex_response_(
      std::string& response,
      const RequestInfo& request_info,
      const FrontendCommons::GrpcCampaignManagerPool::RequestParams& request_params,
      const JsonProcessingContext& context,
      const AdServer::CampaignSvcs::Proto::RequestCreativeResult& campaign_match_result) noexcept;

    void fill_native_response_(
      AdServer::Commons::JsonObject* root_json,
      const JsonAdSlotProcessingContext::Native& native_context,
      const AdServer::CampaignSvcs::Proto::AdSlotResult& ad_slot_result,
      bool need_escape,
      bool add_root_native,
      SourceTraits::NativeAdsInstantiateType instantiate_type);

    void add_response_notice_(
      AdServer::Commons::JsonObject& bid_object,
      const String::SubString& url,
      SourceTraits::NoticeInstantiateType notice_instantiate_type);

    static void fill_buzsape_nroa_contract_(
      AdServer::Commons::JsonObject& contract_obj,
      const AdServer::CampaignSvcs::Proto::ExtContractInfo& ext_contract_info) noexcept;

    static void fill_buzsape_nroa_(
      AdServer::Commons::JsonObject& nroa_obj,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::Proto::AdSlotResult& ad_slot_result) noexcept;

    static void fill_ext0_nroa_(
      AdServer::Commons::JsonObject& nroa_obj,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::Proto::AdSlotResult& ad_slot_result) noexcept;

  private:
    std::string uri_;

    JsonProcessingContext context_;
  };
} // namespace AdServer::Bidding::Grpc

#endif /*BIDDINGFRONTENDGRPC_OPENRTBBIDREQUESTTASK_HPP_*/
