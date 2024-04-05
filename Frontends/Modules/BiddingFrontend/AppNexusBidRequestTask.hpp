#ifndef APPNEXUSBIDREQUESTTASK_HPP_
#define APPNEXUSBIDREQUESTTASK_HPP_

#include "BidRequestTask.hpp"

namespace AdServer
{
namespace Bidding
{
  //
  // AppNexusBidRequestTask
  //
  class AppNexusBidRequestTask: public BidRequestTask
  {
  public:
    AppNexusBidRequestTask(
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

  protected:
    virtual
    ~AppNexusBidRequestTask() noexcept = default;

  private:
    //std::string bid_request_;
    JsonProcessingContext context_;
  };
}
}

#endif /*APPNEXUSBIDREQUESTTASK_HPP_*/
