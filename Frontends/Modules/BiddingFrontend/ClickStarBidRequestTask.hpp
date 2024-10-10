#ifndef CLICKSTARBIDREQUESTTASK_HPP_
#define CLICKSTARBIDREQUESTTASK_HPP_

#include <iostream>
#include <Frontends/Modules/BiddingFrontend/BidRequestTask.hpp>

namespace AdServer
{
namespace Bidding
{
  //
  // ClickStarBidRequestTask
  //
  class ClickStarBidRequestTask: public BidRequestTask
  {
  public:
    ClickStarBidRequestTask(
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
    struct Context;

  protected:
    virtual
    ~ClickStarBidRequestTask() noexcept = default;

    void
    fill_response_(
      std::string& response,
      const RequestInfo& request_info,
      const AdServer::CampaignSvcs::CampaignManager::RequestParams& request_params,
      const AdServer::CampaignSvcs::CampaignManager::
        RequestCreativeResult& campaign_match_result)
      noexcept;

    void
    fill_response_adslot_(
      AdServer::Commons::JsonFormatter* json_formatter,
      const AdServer::CampaignSvcs::CampaignManager::AdSlotResult& ad_slot_result)
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
}

#endif /*CLICKSTARBIDREQUESTTASK_HPP_*/
