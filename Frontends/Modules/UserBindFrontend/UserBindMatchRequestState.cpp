#include "UserBindMatchRequestState.hpp"

#include <algorithm>
#include <set>
#include <utility>

#include <Commons/GrpcAlgs.hpp>
#include <Stream/MemoryStream.hpp>

#include "UserBindFrontend.hpp"

namespace
{
  const char FUN[] = "UserBindMatchRequestState";
  const char USER_BIND_FRONTEND_ASPECT[] = "UserBindFrontend";

  struct ChannelMatch
  {
    ChannelMatch(
      unsigned long channel_id_val,
      unsigned long channel_trigger_id_val)
      : channel_id(channel_id_val),
        channel_trigger_id(channel_trigger_id_val)
    {}

    bool
    operator<(const ChannelMatch& right) const
    {
      return
        channel_id < right.channel_id ||
        (channel_id == right.channel_id &&
          channel_trigger_id < right.channel_trigger_id);
    }

    unsigned long channel_id;
    unsigned long channel_trigger_id;
  };

  struct GetChannelTriggerId
  {
    ChannelMatch
    operator()(
      const adserver::channel_svcs::channel_server::ChannelAtom& atom)
      const noexcept
    {
      return ChannelMatch(atom.id(), atom.trigger_channel_id());
    }
  };
}

namespace AdServer
{
  UserBindMatchRequestState::UserBindMatchRequestState(
    UserBindFrontend* frontend,
    const Commons::UserId& result_user_id,
    const Commons::UserId& merge_user_id,
    bool create_user_profile,
    const String::SubString& keywords,
    const String::SubString& cohort,
    const String::SubString& referer,
    unsigned long colo_id,
    FrontendCommons::Location_var location,
    const String::SubString& source)
    : frontend_(frontend),
      result_user_id_(result_user_id),
      merge_user_id_(merge_user_id),
      create_user_profile_(create_user_profile),
      keywords_(keywords.str()),
      cohort_(cohort.str()),
      referer_(referer.str()),
      colo_id_(colo_id),
      location_(std::move(location)),
      source_(source.str()),
      trigger_match_result_present_(false)
  {}

  void
  UserBindMatchRequestState::start()
  {
    auto self = shared_from_this();
    co_process_(std::move(self)).start_detached(nullptr);
  }

  FrontendCommons::RequestTask
  UserBindMatchRequestState::co_process_(
    std::shared_ptr<UserBindMatchRequestState> self)
    noexcept
  {
    (void)self;
    now_ = Generics::Time::get_time_of_day();
    if((!referer_.empty() || !keywords_.empty()) &&
      !result_user_id_.is_null())
    {
      co_await co_channel_match_();
    }
    co_await co_history_();
    co_await co_campaign_();
    co_return FrontendCommons::RequestResult{};
  }

  FrontendCommons::RequestTask
  UserBindMatchRequestState::co_channel_match_() noexcept
  {
    if(frontend_->channel_client_coro_)
    {
      adserver::channel_svcs::channel_server::MatchRequest channel_request;
      channel_request.set_non_strict_word_match(false);
      channel_request.set_non_strict_url_match(false);
      channel_request.set_return_negative(false);
      channel_request.set_simplify_page(false);
      channel_request.set_fill_content(false);
      channel_request.set_statuses("A", 2);
      channel_request.set_pwords(keywords_);
      channel_request.set_first_url(referer_);

      auto match_result = co_await frontend_->channel_client_coro_->match(
        std::move(channel_request));
      if(match_result.status.ok())
      {
        trigger_match_result_ = std::move(match_result.response);
        trigger_match_result_present_ = true;
      }
      else
      {
        log_channel_error_(match_result.status);
      }
    }

    co_return FrontendCommons::RequestResult{};
  }

  FrontendCommons::RequestTask
  UserBindMatchRequestState::co_history_() noexcept
  {
    if(!need_history_() || !frontend_->user_info_client_coro_)
    {
      co_return FrontendCommons::RequestResult{};
    }

    adserver::user_info_svcs::user_info_manager::MatchRequest
      history_match_request;
    fill_history_match_request_(history_match_request);

    if(!merge_user_id_.is_null())
    {
      adserver::user_info_svcs::user_info_manager::GetUserProfileRequest
        get_profile_request;
      get_profile_request.set_user_id(GrpcAlgs::pack_user_id(merge_user_id_));
      get_profile_request.set_temporary(false);
      auto* profile_request = get_profile_request.mutable_profile_request();
      profile_request->set_base_profile(true);
      profile_request->set_add_profile(true);
      profile_request->set_history_profile(true);
      profile_request->set_freq_cap_profile(true);
      profile_request->set_pref_profile(false);

      auto get_profile_result =
        co_await frontend_->user_info_client_coro_->get_user_profile(
          std::move(get_profile_request));
      if(!get_profile_result.status.ok())
      {
        log_user_info_error_(
          "UserInfoManager::get_user_profile()",
          get_profile_result.status);
        co_return FrontendCommons::RequestResult{};
      }

      if(get_profile_result.response.found())
      {
        adserver::user_info_svcs::user_info_manager::MergeRequest
          merge_request;
        *merge_request.mutable_user_info() = history_match_request.user_info();
        *merge_request.mutable_match_params() =
          history_match_request.match_params();
        *merge_request.mutable_merge_user_profile() =
          get_profile_result.response.user_profile();

        auto merge_result = co_await frontend_->user_info_client_coro_->merge(
          std::move(merge_request));
        if(!merge_result.status.ok())
        {
          log_user_info_error_("UserInfoManager::merge()", merge_result.status);
          co_return FrontendCommons::RequestResult{};
        }

        adserver::user_info_svcs::user_info_manager::RemoveUserProfileRequest
          remove_request;
        remove_request.set_user_id(GrpcAlgs::pack_user_id(merge_user_id_));
        auto remove_result =
          co_await frontend_->user_info_client_coro_->remove_user_profile(
            std::move(remove_request));
        if(!remove_result.status.ok())
        {
          log_user_info_error_(
            "UserInfoManager::remove_user_profile()",
            remove_result.status);
        }

        co_return FrontendCommons::RequestResult{};
      }
    }

    auto match_result = co_await frontend_->user_info_client_coro_->match(
      std::move(history_match_request));
    if(match_result.status.ok())
    {
      history_match_result_ = std::make_shared<
        adserver::user_info_svcs::user_info_manager::MatchResponse>(
          std::move(match_result.response));
    }
    else
    {
      log_user_info_error_("UserInfoManager::match()", match_result.status);
    }

    co_return FrontendCommons::RequestResult{};
  }

  FrontendCommons::RequestTask
  UserBindMatchRequestState::co_campaign_() noexcept
  {
    adserver::campaign_svcs::campaign_manager::ProcessMatchRequestRequest
      process_match_request;
    frontend_->fill_match_request_info_(
      *process_match_request.mutable_match_request_info(),
      result_user_id_,
      now_,
      trigger_match_result_present_ ? &trigger_match_result_ : nullptr,
      history_match_result_.get(),
      location_.get(),
      referer_,
      source_);

    if(frontend_->campaign_manager_coro_)
    {
      auto campaign_result =
        co_await frontend_->campaign_manager_coro_->process_match_request(
          std::move(process_match_request));
      if(!campaign_result.status.ok())
      {
        Stream::Error ostr;
        ostr << FUN << ": Can't process match request. "
          "Possible problem with Campaignmanager. "
          "gRPC call failed: code=" <<
          static_cast<int>(campaign_result.status.error_code()) <<
          ", message=" << campaign_result.status.error_message();
        frontend_->logger()->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          USER_BIND_FRONTEND_ASPECT,
          "ADS-ICON-4");
      }
    }

    co_return FrontendCommons::RequestResult{};
  }

  void
  UserBindMatchRequestState::fill_history_match_request_(
    adserver::user_info_svcs::user_info_manager::MatchRequest&
      history_match_request) const
  {
    auto* grpc_match_params = history_match_request.mutable_match_params();

    if(trigger_match_result_present_)
    {
      const auto& matched_channels = trigger_match_result_.matched_channels();
      typedef std::set<ChannelMatch> ChannelMatchSet;

      ChannelMatchSet page_channels;
      std::transform(
        matched_channels.page_channels().begin(),
        matched_channels.page_channels().end(),
        std::inserter(page_channels, page_channels.end()),
        GetChannelTriggerId());
      for(const auto& channel : page_channels)
      {
        auto* channel_match = grpc_match_params->add_page_channel_ids();
        channel_match->set_channel_id(channel.channel_id);
        channel_match->set_channel_trigger_id(channel.channel_trigger_id);
      }

      ChannelMatchSet url_channels;
      std::transform(
        matched_channels.url_channels().begin(),
        matched_channels.url_channels().end(),
        std::inserter(url_channels, url_channels.end()),
        GetChannelTriggerId());
      for(const auto& channel : url_channels)
      {
        auto* channel_match = grpc_match_params->add_url_channel_ids();
        channel_match->set_channel_id(channel.channel_id);
        channel_match->set_channel_trigger_id(channel.channel_trigger_id);
      }

      ChannelMatchSet url_keyword_channels;
      std::transform(
        matched_channels.url_keyword_channels().begin(),
        matched_channels.url_keyword_channels().end(),
        std::inserter(url_keyword_channels, url_keyword_channels.end()),
        GetChannelTriggerId());
      for(const auto& channel : url_keyword_channels)
      {
        auto* channel_match = grpc_match_params->add_url_keyword_channel_ids();
        channel_match->set_channel_id(channel.channel_id);
        channel_match->set_channel_trigger_id(channel.channel_trigger_id);
      }
    }

    grpc_match_params->set_use_empty_profile(false);
    grpc_match_params->set_silent_match(false);
    grpc_match_params->set_no_match(false);
    grpc_match_params->set_no_result(true);
    grpc_match_params->set_ret_freq_caps(false);
    grpc_match_params->set_provide_channel_count(false);
    grpc_match_params->set_provide_persistent_channels(false);
    grpc_match_params->set_change_last_request(false);
    grpc_match_params->set_filter_contextual_triggers(false);
    grpc_match_params->set_publishers_optin_timeout(
      GrpcAlgs::pack_time(Generics::Time::ZERO));
    grpc_match_params->set_cohort(cohort_);

    auto* grpc_user_info = history_match_request.mutable_user_info();
    grpc_user_info->set_user_id(GrpcAlgs::pack_user_id(result_user_id_));
    grpc_user_info->set_last_colo_id(colo_id_);
    grpc_user_info->set_request_colo_id(colo_id_);
    grpc_user_info->set_current_colo_id(-1);
    grpc_user_info->set_temporary(false);
    grpc_user_info->set_time(now_.tv_sec);
  }

  bool
  UserBindMatchRequestState::need_history_() const
  {
    return !merge_user_id_.is_null() ||
      create_user_profile_ ||
      !cohort_.empty() ||
      !keywords_.empty() ||
      (!result_user_id_.is_null() && trigger_match_result_present_ && (
        trigger_match_result_.matched_channels().page_channels_size() > 0 ||
        trigger_match_result_.matched_channels().url_channels_size() > 0 ||
        trigger_match_result_.matched_channels().url_keyword_channels_size() > 0));
  }

  void
  UserBindMatchRequestState::log_channel_error_(
    const grpc::Status& status) const
  {
    Stream::Error ostr;
    ostr << FUN << ": caught ChannelServerGrpcAsyncClient error: code=" <<
      static_cast<int>(status.error_code()) <<
      ", message=" << status.error_message();
    frontend_->logger()->log(
      ostr.str(),
      Logging::Logger::EMERGENCY,
      USER_BIND_FRONTEND_ASPECT,
      "ADS-IMPL-117");
  }

  void
  UserBindMatchRequestState::log_user_info_error_(
    const char* operation,
    const grpc::Status& status) const
  {
    Stream::Error ostr;
    ostr << FUN << ": " << operation << " gRPC call failed: user_id = '" <<
      result_user_id_.to_string() << "'; code=" <<
      static_cast<int>(status.error_code()) <<
      ", message=" << status.error_message();
    frontend_->logger()->log(
      ostr.str(),
      Logging::Logger::EMERGENCY,
      USER_BIND_FRONTEND_ASPECT,
      status.error_code() == grpc::StatusCode::UNAVAILABLE ?
        "ADS-IMPL-7804" : "ADS-IMPL-7803");
  }
}
