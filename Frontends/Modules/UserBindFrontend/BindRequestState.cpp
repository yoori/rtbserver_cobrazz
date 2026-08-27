#include "BindRequestState.hpp"

#include <utility>

#include <google/protobuf/arena.h>

#include <Commons/ExternalUserIdUtils.hpp>
#include <Commons/GrpcAlgs.hpp>
#include <Frontends/CommonModule/CommonModule.hpp>

namespace
{
  const String::SubString IDFA_KNOWN_KEYWORD("rtbidfaknown");
}

namespace AdServer
{
  UserBindFrontend::BindRequestState::BindRequestState(
    UserBindFrontend* frontend_val,
    UserBind::RequestInfo_var request_info_val,
    std::string dns_bind_request_id_val)
    : frontend(frontend_val),
      request_info(std::move(request_info_val)),
      dns_bind_request_id(std::move(dns_bind_request_id_val)),
      http_status(200),
      result_user_id(request_info->user_id),
      result_user_id_type(RUIT_COOKIE),
      app_request(request_info->external_id.compare(0, 4, "ifa/") == 0),
      opted_out(request_info->user_status == AdServer::CampaignSvcs::US_OPTOUT),
      create_user_profile(false),
      resolved_ext_user_i(0)
  {
    if (app_request)
    {
      result_user_id = AdServer::Commons::UserId();
    }
  }

  UserBindFrontend::ProcessRequestTask
  UserBindFrontend::BindRequestState::co_process_() noexcept
  {
    try
    {
      if (request_info->delete_op)
      {
        if (request_info->external_id.empty() || !frontend->has_user_bind_client_())
        {
          co_return ProcessRequestResult{http_status, BindResult(request_info->user_id)};
        }

        google::protobuf::Arena arena;
        auto* add_user_request = google::protobuf::Arena::CreateMessage<
          adserver::user_info_svcs::user_bind::AddUserIdRequest>(&arena);
        add_user_request->set_id(request_info->external_id);
        add_user_request->set_user_id(GrpcAlgs::pack_user_id(Commons::UserId()));
        add_user_request->set_timestamp(GrpcAlgs::pack_time(request_info->time));

        auto add_result = co_await frontend->user_bind_client_coro_->co_add_user_id(
          *add_user_request);
        co_return ProcessRequestResult{
          add_result.status.ok() ? 204 : 500,
          BindResult(request_info->user_id)};
      }

      FrontendCommons::CountryFilter_var country_filter =
        frontend->common_module_->country_filter();
      if (country_filter.in() && (
          !request_info->location ||
          !country_filter->enabled(request_info->location->country)))
      {
        co_return ProcessRequestResult{http_status, make_bind_result_()};
      }

      if (frontend->has_user_bind_client_())
      {
        external_ids = {
          ExternalId{request_info->ga_user_id, false, true},
          ExternalId{request_info->gclu_user_id, false, true},
          ExternalId{request_info->ym_user_id, false, true},
          ExternalId{request_info->external_id, frontend->config_->set_uid(), !app_request}
        };

        resolved_ext_user_i = external_ids.size();
        if (result_user_id.is_null())
        {
          for (std::size_t index = 0; index < external_ids.size(); ++index)
          {
            if (external_ids[index].id.empty())
            {
              continue;
            }

            google::protobuf::Arena arena;
            auto* get_request = google::protobuf::Arena::CreateMessage<
              adserver::user_info_svcs::user_bind::GetUserIdRequest>(&arena);
            get_request->set_id(external_ids[index].id);
            get_request->set_timestamp(GrpcAlgs::pack_time(request_info->time));
            get_request->set_silent(true);
            get_request->set_generate_user_id(external_ids[index].set_uid);
            get_request->set_for_set_cookie(!app_request);
            get_request->set_create_timestamp(GrpcAlgs::pack_time(Generics::Time::ZERO));

            auto get_result = co_await
              frontend->user_bind_client_coro_->co_get_user_id(*get_request);
            if (get_result.status.ok())
            {
              if (get_result.response.invalid_operation())
              {
                resolve_failed_external_ids.insert(external_ids[index].id);
                frontend->report_bad_user_(*request_info);
              }
              else
              {
                AdServer::Commons::UserId resolved_user_id =
                  GrpcAlgs::unpack_user_id(get_result.response.user_id());
                if (!resolved_user_id.is_null())
                {
                  result_user_id = resolved_user_id;
                  result_user_id_type =
                    external_ids[index].can_be_in_cookie ?
                      RUIT_EXTIDRESOLVE : RUIT_EXTIDRESOLVE_NOCOOKIE;
                  frontend->common_module_->user_id_controller()->
                    null_blacklisted(result_user_id);

                  if (frontend->config_->create_profile())
                  {
                    create_user_profile = get_result.response.created();
                  }
                  resolved_ext_user_i = index;
                  break;
                }
              }
            }
            else
            {
              http_status = 500;
            }
          }
        }
      }

      if (frontend->has_user_bind_client_() && (opted_out || !result_user_id.is_null()))
      {
        for (std::size_t index = 0; index < external_ids.size(); ++index)
        {
          const auto& external_id = external_ids[index].id;
          if (external_id.empty() ||
            index == resolved_ext_user_i ||
            resolve_failed_external_ids.find(external_id) !=
              resolve_failed_external_ids.end())
          {
            continue;
          }

          google::protobuf::Arena arena;
          auto* add_user_request = google::protobuf::Arena::CreateMessage<
            adserver::user_info_svcs::user_bind::AddUserIdRequest>(&arena);
          add_user_request->set_id(external_id);
          add_user_request->set_user_id(GrpcAlgs::pack_user_id(
            !opted_out ? result_user_id : Commons::UserId()));
          add_user_request->set_timestamp(GrpcAlgs::pack_time(request_info->time));

          auto add_result = co_await
            frontend->user_bind_client_coro_->co_add_user_id(*add_user_request);
          if (add_result.status.ok())
          {
            if (add_result.response.invalid_operation())
            {
              frontend->report_bad_user_(*request_info);
            }
          }
          else
          {
            http_status = 500;
          }
        }
      }

      if (!opted_out &&
        !app_request && (!result_user_id.is_null() || resolve_failed_external_ids.empty()))
      {
        Generics::Uuid generated_user_id = result_user_id.is_null() ?
          Generics::Uuid::create_random_based() :
          result_user_id;

        if (request_info->generate_external_id)
        {
          result_ssp_user_id =
            frontend->common_module_->user_id_controller()->ssp_uuid(
              generated_user_id,
              request_info->source_id);
        }

        if (frontend->config_->set_uid())
        {
          result_user_id = generated_user_id;
          result_user_id_type = RUIT_COOKIE;
        }
      }

      BindResult bind_result = make_bind_result_();

      user_match_stage_();

      if (!dns_bind_request_id.empty() && frontend->has_user_bind_client_())
      {
        AdServer::Commons::ExternalUserIdArray user_ids;
        if (!result_user_id.is_null())
        {
          if (result_user_id_type == RUIT_EXTIDRESOLVE_NOCOOKIE)
          {
            user_ids.push_back(std::string("/") + result_user_id.to_string());
          }
          else
          {
            user_ids.push_back(std::string("c/") + result_user_id.to_string());
          }
        }

        if (!app_request &&
          !request_info->user_id.is_null() &&
          !(request_info->user_id == result_user_id))
        {
          user_ids.push_back(std::string("c/") + request_info->user_id.to_string());
        }

        if (!request_info->ga_user_id.empty())
        {
          user_ids.push_back(request_info->ga_user_id);
        }

        if (!request_info->ym_user_id.empty())
        {
          user_ids.push_back(request_info->ym_user_id);
        }

        if (!request_info->external_id.empty())
        {
          user_ids.push_back(request_info->external_id);
        }

        if (!request_info->add_user_id.is_null() &&
          request_info->add_user_id != result_user_id &&
          request_info->add_user_id != request_info->user_id)
        {
          user_ids.push_back(std::string("/") + request_info->add_user_id.to_string());
        }

        google::protobuf::Arena arena;
        auto* bind_request = google::protobuf::Arena::CreateMessage<
          adserver::user_info_svcs::user_bind::AddBindRequestRequest>(&arena);
        bind_request->set_request_id(dns_bind_request_id);
        bind_request->set_timestamp(GrpcAlgs::pack_time(request_info->time));
        for (const auto& user_id : user_ids)
        {
          bind_request->add_bind_user_ids(user_id);
        }

        co_await frontend->user_bind_client_coro_->co_add_bind_request(*bind_request);
      }

      co_return ProcessRequestResult{http_status, bind_result};
    }
    catch(const eh::Exception&)
    {
      co_return ProcessRequestResult{500, BindResult(request_info->user_id)};
    }
  }

  UserBindFrontend::BindResult
  UserBindFrontend::BindRequestState::make_bind_result_() const
  {
    BindResult bind_result;
    bind_result.result_user_id = result_user_id;
    if (!result_ssp_user_id.is_null())
    {
      bind_result.ssp_user_id = result_ssp_user_id;
    }
    else if (!app_request && !result_user_id.is_null())
    {
      bind_result.ssp_user_id =
        frontend->common_module_->user_id_controller()->ssp_uuid(
          result_user_id,
            request_info->source_id);
    }

    return bind_result;
  }

  void
  UserBindFrontend::BindRequestState::user_match_stage_() noexcept
  {
    std::string ifa_str;
    if (request_info->external_id.compare(0, 4, "ifa/") == 0)
    {
      ifa_str = request_info->external_id.substr(4);
      String::AsciiStringManip::to_lower(ifa_str);
    }

    frontend->schedule_user_match_(
      result_user_id,
      merge_user_id,
      create_user_profile,
      !ifa_str.empty() ? IDFA_KNOWN_KEYWORD : String::SubString(),
      ifa_str,
      request_info->referer,
      request_info->colo_id,
      request_info->location,
      request_info->source_id);
  }

}
