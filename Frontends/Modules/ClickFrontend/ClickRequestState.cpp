#include "ClickRequestState.hpp"

#include <algorithm>
#include <set>
#include <sstream>

#include <Logger/Logger.hpp>
#include <Stream/MemoryStream.hpp>

#include <Commons/GrpcAlgs.hpp>

#include "ClickFrontend.hpp"

namespace
{
  const char FUN[] = "ClickRequestState";
  const char CLICK_FRONTEND_ASPECT[] = "ClickFrontend";

  DECLARE_EXCEPTION(GrpcCallException, eh::DescriptiveException);

  void
  throw_grpc_exception_(
    const char* name,
    const grpc::Status& status)
  {
    Stream::Error ostr;
    ostr << name << ": gRPC call failed: code=" <<
      static_cast<int>(status.error_code()) <<
      ", message=" << status.error_message();
    throw GrpcCallException(ostr);
  }

  void
  throw_user_bind_exception_(const grpc::Status& status)
  {
    const std::string message = status.error_message();
    switch(status.error_code())
    {
    case grpc::StatusCode::UNAVAILABLE:
      throw AdServer::UserInfoSvcs::UserBindClient::NotReady(
        message.c_str());
    case grpc::StatusCode::NOT_FOUND:
      throw AdServer::UserInfoSvcs::UserBindClient::ChunkNotFound(
        message.c_str());
    default:
      throw AdServer::UserInfoSvcs::UserBindClient::ImplementationException(
        message.c_str());
    }
  }

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
  ClickRequestState::ClickRequestState(
    ClickFrontend* frontend,
    const AdServer::Commons::UserId& user_id,
    const AdServer::Commons::UserId& cookie_user_id,
    const Generics::Time& now,
    unsigned long campaign_id,
    unsigned long advertiser_id,
    const String::SubString& peer_ip,
    const std::list<std::string>& markers)
    : frontend_(frontend),
      user_id_(user_id),
      cookie_user_id_(cookie_user_id),
      resolved_cookie_user_id_(cookie_user_id),
      now_(now),
      campaign_id_(campaign_id),
      advertiser_id_(advertiser_id),
      peer_ip_(peer_ip.str()),
      markers_(markers),
      trigger_match_result_present_(false)
  {}

  void
  ClickRequestState::start()
  {
    channel_match_stage_();
  }

  void
  ClickRequestState::channel_match_stage_()
  {
    adserver::channel_svcs::channel_server::MatchRequest channel_request;
    channel_request.set_non_strict_word_match(false);
    channel_request.set_non_strict_url_match(false);
    channel_request.set_return_negative(false);
    channel_request.set_simplify_page(false);
    channel_request.set_fill_content(false);
    channel_request.set_statuses("A", 2);

    std::ostringstream keywords_ostr;
    keywords_ostr << "poadclick poadclicka" << advertiser_id_ <<
      " poadclickc" << campaign_id_;
    for(const auto& marker : markers_)
    {
      const std::string base_trigger = std::string("poad") + marker + "click";
      keywords_ostr << " " << base_trigger <<
        " " << base_trigger << "a" << advertiser_id_ <<
        " " << base_trigger << "c" << campaign_id_;
    }
    channel_request.set_pwords(keywords_ostr.str());

    auto self = shared_from_this();
    frontend_->channel_client_->match(
      channel_request,
      [self](
        const grpc::Status& status,
        const adserver::channel_svcs::channel_server::MatchResponse& response)
      {
        self->frontend_->workers_->post(
          [self, status, response]()
          {
            self->channel_match_done_stage_(status, response);
          });
      });
  }

  void
  ClickRequestState::channel_match_done_stage_(
    const grpc::Status& status,
    const adserver::channel_svcs::channel_server::MatchResponse& response)
  {
    try
    {
      if(!status.ok())
      {
        throw_grpc_exception_("ChannelServer::match()", status);
      }
      trigger_match_result_ = response;
      trigger_match_result_present_ = true;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << "::channel_match_done_stage_(): "
        "caught ChannelServerGrpcAsyncClient error: " << ex.what();
      frontend_->logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        CLICK_FRONTEND_ASPECT,
        "ADS-IMPL-117");
    }

    resolve_cookie_stage_();
  }

  void
  ClickRequestState::resolve_cookie_stage_()
  {
    if(cookie_user_id_.is_null() || user_id_ == cookie_user_id_)
    {
      match_history_user_stage_();
      return;
    }

    const std::string cookie_external_id_str =
      std::string("c/") + cookie_user_id_.to_string();

    adserver::user_info_svcs::user_bind::GetUserIdRequest get_request_info;
    get_request_info.set_id(cookie_external_id_str);
    get_request_info.set_timestamp(GrpcAlgs::pack_time(now_));
    get_request_info.set_silent(true);
    get_request_info.set_generate_user_id(false);
    get_request_info.set_for_set_cookie(false);
    get_request_info.set_create_timestamp(
      GrpcAlgs::pack_time(Generics::Time::ZERO));
    get_request_info.set_current_user_id(
      GrpcAlgs::pack_user_id(cookie_user_id_));

    auto self = shared_from_this();
    frontend_->user_bind_client_->get_user_id(
      get_request_info,
      [self](
        const grpc::Status& status,
        const adserver::user_info_svcs::user_bind::GetUserIdResponse& response)
      {
        self->frontend_->workers_->post(
          [self, status, response]()
          {
            self->resolve_cookie_done_stage_(status, response);
          });
      });
  }

  void
  ClickRequestState::resolve_cookie_done_stage_(
    const grpc::Status& status,
    const adserver::user_info_svcs::user_bind::GetUserIdResponse& response)
  {
    try
    {
      if(!status.ok())
      {
        throw_user_bind_exception_(status);
      }
      resolved_cookie_user_id_ = GrpcAlgs::unpack_user_id(response.user_id());
    }
    catch(const eh::Exception& ex)
    {
      log_user_bind_error_(ex);
    }

    match_history_user_stage_();
  }

  void
  ClickRequestState::match_history_user_stage_()
  {
    if(!trigger_match_result_present_ ||
      trigger_match_result_.matched_channels().page_channels_size() == 0)
    {
      return;
    }

    if(user_id_ == AdServer::Commons::PROBE_USER_ID)
    {
      match_history_cookie_stage_();
      return;
    }

    auto request = make_history_match_request_(user_id_);
    auto self = shared_from_this();
    frontend_->user_info_client_->match(
      request,
      [self](
        const grpc::Status& status,
        const adserver::user_info_svcs::user_info_manager::MatchResponse& response)
      {
        self->frontend_->workers_->post(
          [self, status, response]()
          {
            self->match_history_user_done_stage_(status, response);
          });
      });
  }

  void
  ClickRequestState::match_history_user_done_stage_(
    const grpc::Status& status,
    const adserver::user_info_svcs::user_info_manager::MatchResponse& response)
  {
    try
    {
      if(!status.ok())
      {
        throw_grpc_exception_("UserInfoManager::match()", status);
      }
      history_match_result_ = std::make_shared<
        adserver::user_info_svcs::user_info_manager::MatchResponse>(response);
    }
    catch(const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FUN << "::match_history_user_done_stage_(): "
        "history match failed: " << e.what();

      frontend_->logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        CLICK_FRONTEND_ASPECT,
        "ADS-IMPL-112");
    }

    match_history_cookie_stage_();
  }

  void
  ClickRequestState::match_history_cookie_stage_()
  {
    if(user_id_ == resolved_cookie_user_id_ ||
      resolved_cookie_user_id_.is_null())
    {
      process_match_stage_();
      return;
    }

    auto request = make_history_match_request_(resolved_cookie_user_id_);
    auto self = shared_from_this();
    frontend_->user_info_client_->match(
      request,
      [self](const grpc::Status& status, const auto&)
      {
        self->frontend_->workers_->post(
          [self, status]()
          {
            self->match_history_cookie_done_stage_(status);
          });
      });
  }

  void
  ClickRequestState::match_history_cookie_done_stage_(
    const grpc::Status& status)
  {
    if(!status.ok())
    {
      try
      {
        throw_grpc_exception_("UserInfoManager::match()", status);
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << "::match_history_cookie_done_stage_(): "
          "cookie history match failed: " << ex.what();
        frontend_->logger()->log(
          ostr.str(),
          Logging::Logger::EMERGENCY,
          CLICK_FRONTEND_ASPECT,
          "ADS-IMPL-112");
      }
    }

    process_match_stage_();
  }

  void
  ClickRequestState::process_match_stage_()
  {
    adserver::campaign_svcs::campaign_manager::ProcessMatchRequestRequest
      process_match_request;
    frontend_->fill_match_request_info_(
      *process_match_request.mutable_match_request_info(),
      user_id_,
      now_,
      &trigger_match_result_,
      history_match_result_.get(),
      peer_ip_);

    auto self = shared_from_this();
    frontend_->campaign_manager_->process_match_request(
      process_match_request,
      [self](const grpc::Status& status, const auto&)
      {
        self->frontend_->workers_->post(
          [self, status]()
          {
            self->process_match_done_stage_(status);
          });
      });
  }

  void
  ClickRequestState::process_match_done_stage_(const grpc::Status& status)
  {
    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << "::process_match_done_stage_(): "
        "Can't process match request. Possible problem with Campaignmanager. "
        "gRPC code=" << static_cast<int>(status.error_code()) <<
        ", message=" << status.error_message();
      frontend_->logger()->log(
        ostr.str(),
        Logging::Logger::EMERGENCY,
        CLICK_FRONTEND_ASPECT,
        "ADS-ICON-4");
    }
  }

  adserver::user_info_svcs::user_info_manager::MatchRequest
  ClickRequestState::make_history_match_request_(
    const AdServer::Commons::UserId& match_user_id) const
  {
    adserver::user_info_svcs::user_info_manager::MatchRequest request;
    auto* match_params = request.mutable_match_params();
    match_params->set_use_empty_profile(false);
    match_params->set_silent_match(false);
    match_params->set_no_match(false);
    match_params->set_no_result(true);
    match_params->set_ret_freq_caps(false);
    match_params->set_provide_channel_count(false);
    match_params->set_provide_persistent_channels(false);
    match_params->set_change_last_request(false);
    match_params->set_filter_contextual_triggers(false);
    match_params->set_publishers_optin_timeout(
      GrpcAlgs::pack_time(Generics::Time::ZERO));

    auto* user_info = request.mutable_user_info();
    user_info->set_user_id(GrpcAlgs::pack_user_id(match_user_id));
    user_info->set_last_colo_id(-1);
    user_info->set_request_colo_id(frontend_->common_config_->colo_id());
    user_info->set_current_colo_id(-1);
    user_info->set_temporary(false);
    user_info->set_time(now_.tv_sec);

    typedef std::set<ChannelMatch> ChannelMatchSet;
    ChannelMatchSet page_channels;
    std::transform(
      trigger_match_result_.matched_channels().page_channels().begin(),
      trigger_match_result_.matched_channels().page_channels().end(),
      std::inserter(page_channels, page_channels.end()),
      GetChannelTriggerId());

    auto* page_channel_ids =
      request.mutable_match_params()->mutable_page_channel_ids();
    for(const auto& channel_match : page_channels)
    {
      auto* result = page_channel_ids->Add();
      result->set_channel_id(channel_match.channel_id);
      result->set_channel_trigger_id(channel_match.channel_trigger_id);
    }

    return request;
  }

  void
  ClickRequestState::log_user_bind_error_(
    const eh::Exception& ex) const noexcept
  {
    Stream::Error ostr;
    ostr << FUN << "::log_user_bind_error_(): "
      "caught UserBindClient exception: " << ex.what();
    frontend_->logger()->log(
      ostr.str(),
      Logging::Logger::ERROR,
      CLICK_FRONTEND_ASPECT,
      "ADS-IMPL-109");
  }
}
