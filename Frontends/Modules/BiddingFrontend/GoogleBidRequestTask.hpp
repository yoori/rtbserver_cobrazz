#ifndef GOOGLEBIDREQUESTTASK_HPP_
#define GOOGLEBIDREQUESTTASK_HPP_

#include <Frontends/Modules/BiddingFrontend/BidRequestTask.hpp>

namespace AdServer
{
namespace Bidding
{
  //
  // GoogleBidRequestTask
  //
  class GoogleBidRequestTask: public BidRequestTask
  {
  public:
    GoogleBidRequestTask(
      Frontend* bid_frontend,
      FrontendCommons::HttpRequestHolder_var request_holder,
      FrontendCommons::HttpResponseWriter_var response_writer,
      const Generics::Time& start_processing_time)
      /*throw(Invalid)*/;

    virtual void
    print_request(std::ostream& out) const noexcept;

    // processing stages
    // read request & transform it to holder
    virtual bool
    read_request() noexcept;

    // fill parameters by request
    /*
    virtual bool
    fill_request_info(std::string& keywords)
      noexcept;
    */

    // write response (convert holder to response)
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

  protected:
    virtual
    ~GoogleBidRequestTask() noexcept = default;

  private:
    Google::BidRequest bid_request_;
    //Google::BidResponse bid_response_;
    GoogleAdSlotContextArray ad_slots_context_;
  };
}
}

#endif /*GOOGLEBIDREQUESTTASK_HPP_*/
