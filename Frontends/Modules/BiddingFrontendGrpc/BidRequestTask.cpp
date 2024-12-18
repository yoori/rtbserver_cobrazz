// THIS
#include <Frontends/Modules/BiddingFrontendGrpc/BidRequestTask.hpp>

namespace AdServer::Bidding::Grpc
{
  BidRequestTask::BidRequestTask(
    Frontend* bid_frontend,
    FrontendCommons::HttpRequestHolder_var request_holder,
    FrontendCommons::HttpResponseWriter_var response_writer,
    const Generics::Time& start_processing_time)
    : bid_frontend_(bid_frontend),
      request_holder_(std::move(request_holder)),
      start_processing_time_(start_processing_time),
      to_interrupt_(0),
      request_params_(new RequestParamsHolder()),
      response_writer_(std::move(response_writer)),
      response_sent_(false)
  {}

  void BidRequestTask::execute() noexcept
  {
    if(!execute_())
    {
      this->write_empty_response(0);
    }

    clear();
  }

  bool BidRequestTask::execute_() noexcept
  {
    // fill request info by url parameters
    bid_frontend_->request_info_filler_->fill(
      request_info_,
      request_holder_->request(),
      start_processing_time_);

    // fill request info & request type specific parameters
    if (!read_request())
    {
      write_empty_response(400);
      return true;
    }

    // check interrupt
    if (check_interrupt_(Stage::RequestParsing))
    {
      return false;
    }

    AdServer::Commons::UserId user_id;
    std::unique_ptr<AdServer::CampaignSvcs::Proto::RequestCreativeResult>
      campaign_match_result;

    bool not_interrupted = bid_frontend_->process_bid_request_(
      "", // FUN
      campaign_match_result,
      user_id,
      this,
      request_info_,
      keywords_);

    if (!not_interrupted)
    {
      return false;
    }

    if (check_interrupt_(Stage::CampaignSelection))
    {
      return false;
    }

    if (campaign_match_result)
    {
      if (!bid_frontend_->consider_campaign_selection_(
        user_id,
        request_info_.current_time,
        *campaign_match_result))
      {
        return false;
      }

      if (check_interrupt_(Stage::CampaignSelectionConsidering))
      {
        return false;
      }

      // check that any campaign selected (in any slot)
      bool ad_selected = false;

      const auto& ad_slots = campaign_match_result->ad_slots();
      for (const auto& ad_slot : ad_slots)
      {
        if (!ad_slot.selected_creatives().empty())
        {
          ad_selected = true;
          break;
        }
      }

      if (ad_selected)
      {
        return write_response(*campaign_match_result);
      }
    }

    return false;
  }

  void BidRequestTask::interrupt() noexcept
  {
    write_empty_response(0);
  }

  bool BidRequestTask::check_interrupt_(const Stage stage) noexcept
  {
    return bid_frontend_->check_interrupt_("", stage, this);
  }

  void BidRequestTask::write_response_(
    int code,
    FrontendCommons::HttpResponse_var response) noexcept
  {
    bool send_response = (to_interrupt_.exchange_and_add(1) == 0);
    if (send_response)
    {
      response_writer_->write(code, response);
      response_writer_ = FrontendCommons::HttpResponseWriter_var();
      response_sent_ = true;
    }
  }

  void BidRequestTask::clear() noexcept
  {
    request_holder_ = FrontendCommons::HttpRequestHolder_var();
    request_info_ = RequestInfo();
    hostname_.clear();
    request_params_ = RequestParamsHolder_var();
    keywords_.clear();
  }
} // namespace AdServer::Bidding::Grpc
