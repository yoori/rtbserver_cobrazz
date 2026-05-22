#include "BidRequestState.hpp"

namespace AdServer::Bidding
{
  BidRequestState::BidRequestState(
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
      timeout_interrupted_(false),
      request_params_(new RequestParamsHolder()),
      response_writer_(std::move(response_writer)),
      response_sent_(false)
  {}

  void
  BidRequestState::execute() noexcept
  {
    if(!parse_request_())
    {
      finish_(false);
      return;
    }

    bid_frontend_->co_process_bid_request_(
      BidRequestState_var(ReferenceCounting::add_ref(this))).start_detached(nullptr);
  }

  void
  BidRequestState::init_debug_info() noexcept
  {
    try
    {
      if(request_info_.require_debug_info.empty() && request_holder_.in())
      {
        bid_frontend_->request_info_filler_->fill(
          request_info_,
          request_holder_->request(),
          start_processing_time_);
      }

      debug_sink_.set(String::SubString(request_info_.require_debug_info));
    }
    catch(...)
    {}
  }

  bool
  BidRequestState::parse_request_() noexcept
  {
    set_current_stage(Stage::RequestParsing);

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
      return false;
    }
    debug_sink_.set(String::SubString(request_info_.require_debug_info));

    // check interrupt
    if(check_interrupt_(Stage::RequestParsing))
    {
      this->write_empty_response(0);
      return false;
    }

    return true;
  }

  void
  BidRequestState::complete_request_(
    bool not_interrupted,
    AdServer::Bidding::CampaignManager::RequestCreativeResult&
      campaign_match_result)
    noexcept
  {
    if(!not_interrupted)
    {
      finish_(true);
      return;
    }

    if(check_interrupt_(Stage::CampaignSelection))
    {
      finish_(true);
      return;
    }

    if(campaign_match_result.ad_slots.size())
    {
      set_current_stage(Stage::CampaignSelectionConsidering);

      if (!bid_frontend_->consider_campaign_selection_(
        resolved_user_id_,
        request_info_.current_time,
        campaign_match_result,
        hostname_))
      {
        finish_(true);
        return;
      }
    }

    debug_sink_.print_creative_selection_debug_info(campaign_match_result);
    request_time_metering_.total_time =
      Generics::Time::get_time_of_day() - start_processing_time_;
    debug_sink_.print_time_metering_debug_info(request_time_metering_);

    if(check_interrupt_(Stage::CampaignSelectionConsidering))
    {
      finish_(true);
      return;
    }

    if(campaign_match_result.ad_slots.size())
    {
      // check that any campaign selected (in any slot)
      bool ad_selected = false;

      for(std::size_t ad_slot_i = 0;
          ad_slot_i < campaign_match_result.ad_slots.size();
          ++ad_slot_i)
      {
        const AdServer::Bidding::CampaignManager::
          AdSlotResult& ad_slot_result = campaign_match_result.ad_slots[ad_slot_i];

        if(ad_slot_result.selected_creatives.size() > 0)
        {
          ad_selected = true;
          break;
        }
      }

      if(ad_selected)
      {
        this->write_response(campaign_match_result);
        finish_(false);
        return;
      }
    }

    finish_(true);
  }

  void
  BidRequestState::finish_(bool write_empty_response) noexcept
  {
    if(write_empty_response)
    {
      this->write_empty_response(0);
    }

    clear();

    bid_frontend_->bid_task_count_ += -1;
  }

  void
  BidRequestState::interrupt() noexcept
  {
    write_interrupted_empty_response(
      convert_stage_to_string(get_current_stage()));
  }

  void
  BidRequestState::write_interrupted_empty_response(
    const String::SubString& interrupted_step) noexcept
  {
    timeout_interrupted_.store(true, std::memory_order_relaxed);
    request_time_metering_.total_time =
      Generics::Time::get_time_of_day() - start_processing_time_;
    print_available_request_debug_info_();
    const auto in_progress_stats =
      bid_frontend_->stats_->rtb_request_in_progress_stats();
    debug_sink_.print_interrupt_debug_info(
      interrupted_step,
      in_progress_stats.request,
      in_progress_stats.user_resolving,
      in_progress_stats.trigger_match,
      in_progress_stats.history_match,
      in_progress_stats.campaign_selection);
    debug_sink_.print_time_metering_debug_info(request_time_metering_);
    write_empty_response(0);
  }

  bool
  BidRequestState::check_interrupt_(const Stage stage)
    noexcept
  {
    return bid_frontend_->check_interrupt_("", stage, this);
  }

  void
  BidRequestState::write_response_(
    int code,
    FCGI::HttpResponse_var response)
    noexcept
  {
    bool send_response = (to_interrupt_.exchange_and_add(1) == 0);

    if(send_response)
    {
      debug_sink_.set(String::SubString(request_info_.require_debug_info));
      debug_sink_.write_response(response, code, resolved_user_id_);
      response_writer_->write(code, response);
      response_writer_ = FCGI::BaseHttpResponseWriter_var();
    response_sent_ = true;
    }
  }

  void
  BidRequestState::print_available_request_debug_info_() noexcept
  {
    if(!request_debug_info_printed_ &&
       get_current_stage() != Stage::Initial &&
       request_params_.in())
    {
      debug_sink_.print_request_debug_info(
        request_info_,
        *request_params_,
        resolved_user_id_,
        keywords_);
      request_debug_info_printed_ = true;
    }
  }

  void
  BidRequestState::clear() noexcept
  {
    request_holder_ = FCGI::HttpRequestHolder_var();
    request_info_ = RequestInfo();
    hostname_.clear();
    request_params_ = RequestParamsHolder_var();
    keywords_.clear();
  }
}
