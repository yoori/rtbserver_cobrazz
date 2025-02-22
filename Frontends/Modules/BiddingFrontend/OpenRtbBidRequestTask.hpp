#ifndef OPENRTBBIDREQUESTTASK_HPP_
#define OPENRTBBIDREQUESTTASK_HPP_

#include <iostream>
#include <Frontends/Modules/BiddingFrontend/BidRequestTask.hpp>

namespace AdServer
{
namespace Bidding
{
  //
  // OpenRtbBidRequestTask
  //
  class OpenRtbBidRequestTask: public BidRequestTask
  {
  public:
    OpenRtbBidRequestTask(
      Frontend* bid_frontend,
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer,
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
    virtual
    ~OpenRtbBidRequestTask() noexcept = default;

    void
    fill_openrtb_response_(
      std::string& response,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const JsonProcessingContext& context,
      const AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult& campaign_match_result,
      bool fill_yandex_attributes)
      noexcept;

    void
    fill_yandex_response_(
      std::string& response,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::CampaignManager::
        RequestParams& request_params,
      const JsonProcessingContext& context,
      const AdServer::CampaignSvcs::CampaignManager::
        RequestCreativeResult& campaign_match_result)
      noexcept;

    void
    fill_native_response_(
      AdServer::Commons::JsonObject* root_json,
      const JsonAdSlotProcessingContext::Native& native_context,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result,
      bool need_escape,
      bool add_root_native,
      SourceTraits::NativeAdsInstantiateType instantiate_type);

    void
    add_response_notice_(
      AdServer::Commons::JsonObject& bid_object,
      const String::SubString& url,
      SourceTraits::NoticeInstantiateType notice_instantiate_type);

    static void
    fill_buzsape_nroa_contract_(
      AdServer::Commons::JsonObject& contract_obj,
      const AdServer::CampaignSvcs::CampaignManager::ExtContractInfo& ext_contract_info)
      noexcept;

    static void
    fill_buzsape_nroa_(
      AdServer::Commons::JsonObject& nroa_obj,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result)
      noexcept;

    static void
    fill_ext0_nroa_(
      AdServer::Commons::JsonObject& nroa_obj,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result)
      noexcept;

  private:
    std::string uri_;
    JsonProcessingContext context_;
  };
}
}

#endif /*OPENRTBBIDREQUESTTASK_HPP_*/
