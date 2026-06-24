#include "BidRequestState.hpp"

#include <UserInfoSvcs/UserInfoClient/UserInfoDistributedGrpcClient.hpp>

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

      {
        std::lock_guard lock(debug_sink_mutex_);
        debug_sink_.set(String::SubString(request_info_.require_debug_info));
      }
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
    {
      std::lock_guard lock(debug_sink_mutex_);
      debug_sink_.set(String::SubString(request_info_.require_debug_info));
    }

    // fill request info & request type specific parameters
    if(!this->read_request())
    {
      this->write_empty_response(400);
      return false;
    }
    {
      std::lock_guard lock(debug_sink_mutex_);
      debug_sink_.set(String::SubString(request_info_.require_debug_info));
    }

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
    std::shared_ptr<
      const AdServer::Bidding::CampaignManager::RequestCreativeResult>
      campaign_match_result)
    noexcept
  {
    const bool write_empty_response = complete_request_impl_(
      not_interrupted,
      campaign_match_result);
    finish_(write_empty_response);
  }

  bool
  BidRequestState::complete_request_impl_(
    bool not_interrupted,
    std::shared_ptr<
      const AdServer::Bidding::CampaignManager::RequestCreativeResult>
      campaign_match_result)
    noexcept
  {
    if(!not_interrupted)
    {
      return true;
    }

    if(check_interrupt_(Stage::CampaignSelection))
    {
      return true;
    }

    const auto& campaign_match_result_ref = *campaign_match_result;

    if(campaign_match_result_ref.ad_slots.size())
    {
      set_current_stage(Stage::CampaignSelectionConsidering);

      if (!bid_frontend_->consider_campaign_selection_(
        resolved_user_id_,
        request_info_.current_time,
        campaign_match_result,
        hostname_))
      {
        return true;
      }
    }

    {
      std::lock_guard lock(debug_sink_mutex_);
      if(debug_sink_.require_debug_info())
      {
        debug_sink_.print_creative_selection_debug_info(
          campaign_match_result_ref,
          request_time_metering_.creative_selection ?
            &*request_time_metering_.creative_selection : nullptr);
        if(!time_metering_debug_info_printed_)
        {
          request_time_metering_.total_time =
            Generics::Time::get_time_of_day() - start_processing_time_;
          debug_sink_.print_time_metering_debug_info(request_time_metering_);
          time_metering_debug_info_printed_ = true;
        }
      }
    }

    if(check_interrupt_(Stage::CampaignSelectionConsidering))
    {
      return true;
    }

    if(campaign_match_result_ref.ad_slots.size())
    {
      // check that any campaign selected (in any slot)
      bool ad_selected = false;

      for(std::size_t ad_slot_i = 0;
          ad_slot_i < campaign_match_result_ref.ad_slots.size();
          ++ad_slot_i)
      {
        const AdServer::Bidding::CampaignManager::
          AdSlotResult& ad_slot_result =
            campaign_match_result_ref.ad_slots[ad_slot_i];

        if(ad_slot_result.selected_creatives.size() > 0)
        {
          ad_selected = true;
          break;
        }
      }

      if(ad_selected)
      {
        this->write_response(campaign_match_result_ref);
        return false;
      }
    }

    return true;
  }

  void
  BidRequestState::finish_(bool write_empty_response) noexcept
  {
    if(write_empty_response)
    {
      if(require_debug_info_())
      {
        print_time_metering_debug_info_();
      }
      this->write_empty_response(0);
    }

    clear();

    bid_frontend_->bid_task_count_ += -1;
  }

  void
  BidRequestState::interrupt() noexcept
  {
    if(!claim_response_())
    {
      return;
    }

    write_interrupted_empty_response_(
      convert_stage_to_string(get_current_stage()));
  }

  void
  BidRequestState::write_interrupted_empty_response(
    const String::SubString& interrupted_step) noexcept
  {
    if(!claim_response_())
    {
      return;
    }

    write_interrupted_empty_response_(interrupted_step);
  }

  void
  BidRequestState::write_interrupted_empty_response_(
    const String::SubString& interrupted_step) noexcept
  {
    timeout_interrupted_.store(true, std::memory_order_relaxed);

    if (require_debug_info_())
    {
      print_available_request_debug_info_();
      const auto in_progress_stats =
        bid_frontend_->stats_->rtb_request_in_progress_stats();
      AdServer::Grpc::Stats user_bind_client_stats;
      if (bid_frontend_->user_bind_client_)
      {
        user_bind_client_stats = bid_frontend_->user_bind_client_->stats();
      }
      AdServer::Grpc::Stats user_info_client_stats;
      if (bid_frontend_->user_info_distributed_client_)
      {
        user_info_client_stats =
          bid_frontend_->user_info_distributed_client_->stats();
      }
      AdServer::Grpc::Stats channel_client_stats;
      if (bid_frontend_->channel_client_)
      {
        channel_client_stats = bid_frontend_->channel_client_->stats();
      }
      AdServer::Grpc::Stats campaign_client_stats;
      if (bid_frontend_->campaign_manager_)
      {
        campaign_client_stats = bid_frontend_->campaign_manager_->stats();
      }
      {
        std::lock_guard lock(debug_sink_mutex_);
        debug_sink_.print_interrupt_debug_info(
          interrupted_step,
          hostname_,
          in_progress_stats.request,
          in_progress_stats.user_resolving,
          in_progress_stats.trigger_match,
          in_progress_stats.history_match,
          in_progress_stats.campaign_selection,
          in_progress_stats.history_post_match,
          user_bind_client_stats,
          user_info_client_stats,
          channel_client_stats,
          campaign_client_stats);
        if(!time_metering_debug_info_printed_)
        {
          request_time_metering_.total_time =
            Generics::Time::get_time_of_day() - start_processing_time_;
          debug_sink_.print_time_metering_debug_info(request_time_metering_);
          time_metering_debug_info_printed_ = true;
        }
      }
    }
    write_empty_response(0, true);
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
    FCGI::HttpResponse_var response,
    bool response_claimed)
    noexcept
  {
    bool send_response = response_claimed || claim_response_();

    if(send_response)
    {
      if(require_debug_info_())
      {
        std::lock_guard lock(debug_sink_mutex_);
        debug_sink_.write_response(response, code, resolved_user_id_);
      }
      response_writer_->write(code, response);
      response_writer_ = FCGI::BaseHttpResponseWriter_var();
      response_sent_ = true;
    }
  }

  bool
  BidRequestState::claim_response_() noexcept
  {
    return to_interrupt_.exchange_and_add(1) == 0;
  }

  bool
  BidRequestState::require_debug_info_() noexcept
  {
    std::lock_guard lock(debug_sink_mutex_);
    return debug_sink_.require_debug_info();
  }

  void
  BidRequestState::print_user_resolving_debug_info_(
    const DebugSink::UserResolvingDebugInfo& user_resolving_debug_info,
    const StageResult* stage) noexcept
  {
    std::lock_guard lock(debug_sink_mutex_);
    debug_sink_.print_user_resolving_debug_info(
      user_resolving_debug_info,
      stage);
  }

  void
  BidRequestState::print_channel_matching_debug_info_(
    const adserver::channel_svcs::channel_server::MatchResponse& response,
    const StageResult* stage) noexcept
  {
    std::lock_guard lock(debug_sink_mutex_);
    debug_sink_.print_channel_matching_debug_info(response, stage);
  }

  void
  BidRequestState::print_history_matching_debug_info_(
    const adserver::user_info_svcs::user_info_manager::MatchResult&
      match_result,
    const StageResult* stage) noexcept
  {
    std::lock_guard lock(debug_sink_mutex_);
    debug_sink_.print_history_matching_debug_info(match_result, stage);
  }

  void
  BidRequestState::print_available_request_debug_info_() noexcept
  {
    std::lock_guard lock(debug_sink_mutex_);
    if(debug_sink_.require_debug_info() &&
      !request_debug_info_printed_ &&
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
  BidRequestState::print_time_metering_debug_info_() noexcept
  {
    std::lock_guard lock(debug_sink_mutex_);
    if(debug_sink_.require_debug_info() && !time_metering_debug_info_printed_)
    {
      request_time_metering_.total_time =
        Generics::Time::get_time_of_day() - start_processing_time_;
      debug_sink_.print_time_metering_debug_info(request_time_metering_);
      time_metering_debug_info_printed_ = true;
    }
  }

  void
  BidRequestState::clear() noexcept
  {
    request_holder_ = FCGI::HttpRequestHolder_var();
    request_info_.reset();
    hostname_.clear();
    request_params_ = RequestParamsHolder_var();
    keywords_.clear();
  }
}
