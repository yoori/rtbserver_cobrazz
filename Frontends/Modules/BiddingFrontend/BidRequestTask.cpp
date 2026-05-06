#include "BidRequestTask.hpp"

namespace AdServer
{
namespace Bidding
{
  BidRequestTask::BidRequestTask(
    Frontend* bid_frontend,
    FCGI::HttpRequestHolder_var request_holder,
    FCGI::BaseHttpResponseWriter_var response_writer,
    const Generics::Time& start_processing_time)
    /*throw(Invalid)*/
    : bid_frontend_(bid_frontend),
      request_holder_(std::move(request_holder)),
      start_processing_time_(start_processing_time),
      debug_sink_(bid_frontend->server_id_),
      to_interrupt_(0),
      request_params_(new RequestParamsHolder()),
      response_writer_(std::move(response_writer)),
      response_sent_(false)
  {}

  void
  BidRequestTask::execute() noexcept
  {
    if(!execute_())
    {
      this->write_empty_response(0);
    }

    clear();

    bid_frontend_->bid_task_count_ += -1;
  }

  bool
  BidRequestTask::execute_() noexcept
  {
    // fill request info by url parameters
    bid_frontend_->request_info_filler_->fill(
      request_info_,
      request_holder_->request(),
      start_processing_time_);
    debug_sink_.set(String::SubString(request_info_.require_debug_info));

    // fill request info & request type specific parameters
    if(!this->read_request())
    {
      this->write_empty_response(400);
      return true;
    }

    // check interrupt
    if(check_interrupt_(Stage::RequestParsing))
    {
      return false;
    }

    AdServer::CampaignSvcs::CampaignManager::RequestCreativeResult_var
      campaign_match_result;

    bool not_interrupted = bid_frontend_->process_bid_request_(
      "", // FUN
      campaign_match_result.out(),
      resolved_user_id_,
      this,
      request_info_,
      keywords_);

    if(!not_interrupted)
    {
      return false;
    }

    debug_sink_.print_request_debug_info(
      request_info_,
      *request_params_,
      resolved_user_id_);

    if(check_interrupt_(Stage::CampaignSelection))
    {
      return false;
    }

    if(campaign_match_result)
    {
      if (!bid_frontend_->consider_campaign_selection_(
        resolved_user_id_,
        request_info_.current_time,
        *campaign_match_result,
        hostname_))
      {
        return false;
      }

      debug_sink_.print_creative_selection_debug_info(*campaign_match_result);

      if(check_interrupt_(Stage::CampaignSelectionConsidering))
      {
        return false;
      }

      // check that any campaign selected (in any slot)
      bool ad_selected = false;

      for(CORBA::ULong ad_slot_i = 0;
          ad_slot_i < campaign_match_result->ad_slots.length();
          ++ad_slot_i)
      {
        const AdServer::CampaignSvcs::CampaignManager::
          AdSlotResult& ad_slot_result = campaign_match_result->ad_slots[ad_slot_i];

        if(ad_slot_result.selected_creatives.length() > 0)
        {
          ad_selected = true;
          break;
        }
      }

      if(ad_selected)
      {
        return this->write_response(campaign_match_result);
      }
    }

    return false;
  }

  void
  BidRequestTask::interrupt() noexcept
  {
    write_empty_response(0);
  }

  bool
  BidRequestTask::check_interrupt_(const Stage stage)
    noexcept
  {
    return bid_frontend_->check_interrupt_("", stage, this);
  }

  void
  BidRequestTask::write_response_(
    int code,
    FCGI::HttpResponse_var response)
    noexcept
  {
    bool send_response = (to_interrupt_.exchange_and_add(1) == 0);

    if(send_response)
    {
      debug_sink_.write_response(response, code, resolved_user_id_);
      response_writer_->write(code, response);
      response_writer_ = FCGI::BaseHttpResponseWriter_var();
      response_sent_ = true;
    }
  }

  void
  BidRequestTask::clear() noexcept
  {
    request_holder_ = FCGI::HttpRequestHolder_var();
    request_info_ = RequestInfo();
    hostname_.clear();
    request_params_ = RequestParamsHolder_var();
    keywords_.clear();
  }
}
}
