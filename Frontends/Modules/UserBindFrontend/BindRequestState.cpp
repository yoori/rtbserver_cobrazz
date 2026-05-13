#include "BindRequestState.hpp"

#include <utility>

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
    std::string dns_bind_request_id_val,
    ProcessRequestCallback callback_val)
    : frontend(frontend_val),
      request_info(std::move(request_info_val)),
      dns_bind_request_id(std::move(dns_bind_request_id_val)),
      callback(std::move(callback_val)),
      http_status(200),
      result_user_id(request_info->user_id),
      result_user_id_type(RUIT_COOKIE),
      app_request(request_info->external_id.compare(0, 4, "ifa/") == 0),
      opted_out(request_info->user_status == AdServer::CampaignSvcs::US_OPTOUT),
      cresolve_failed(false),
      create_user_profile(false),
      resolved_ext_user_i(0)
  {
    if(app_request)
    {
      result_user_id = AdServer::Commons::UserId();
    }
  }

  void
  UserBindFrontend::BindRequestState::start() noexcept
  {
    if(request_info->delete_op)
    {
      delete_bind_stage_();
      return;
    }

    country_filter_stage_();
  }

  void
  UserBindFrontend::BindRequestState::country_filter_stage_() noexcept
  {
    FrontendCommons::CountryFilter_var country_filter =
      frontend->common_module_->country_filter();
    if(country_filter.in() && (
        request_info->location.in() == 0 ||
        !country_filter->enabled(request_info->location->country)))
    {
      finish_result_stage_();
      return;
    }

    cookie_resolve_stage_();
  }

  void
  UserBindFrontend::BindRequestState::cookie_resolve_stage_() noexcept
  {
    if(!result_user_id.is_null() && frontend->has_user_bind_client_())
    {
      const std::string cookie_external_id_str =
        std::string("c/") + result_user_id.to_string();

      adserver::user_info_svcs::user_bind::GetUserIdRequest get_request;
      get_request.set_id(cookie_external_id_str);
      get_request.set_timestamp(GrpcAlgs::pack_time(request_info->time));
      get_request.set_silent(true);
      get_request.set_generate_user_id(false);
      get_request.set_for_set_cookie(true);
      get_request.set_create_timestamp(
        GrpcAlgs::pack_time(Generics::Time::ZERO));
      get_request.set_current_user_id(
        GrpcAlgs::pack_user_id(result_user_id));

      auto self = shared_from_this();
      frontend->get_user_id_(
        get_request,
        [self](
          const grpc::Status& status,
          const adserver::user_info_svcs::user_bind::GetUserIdResponse&
            response)
        {
          self->cookie_resolve_done_stage_(status, response);
        });
    }
    else
    {
      if(!result_user_id.is_null() && !frontend->has_user_bind_client_())
      {
        cresolve_failed = true;
      }
      external_ids_prepare_stage_();
    }
  }

  void
  UserBindFrontend::BindRequestState::cookie_resolve_done_stage_(
    const grpc::Status& status,
    const adserver::user_info_svcs::user_bind::GetUserIdResponse& response)
    noexcept
  {
    if(status.ok())
    {
      if(response.invalid_operation())
      {
        cresolve_failed = true;
      }
      else
      {
        Commons::UserId cresolved_user_id =
          GrpcAlgs::unpack_user_id(response.user_id());
        if(!cresolved_user_id.is_null())
        {
          result_user_id = cresolved_user_id;
          result_user_id_type = RUIT_CRESOLVE;
        }
      }
    }
    else
    {
      cresolve_failed = true;
      http_status = 500;
    }

    external_ids_prepare_stage_();
  }

  void
  UserBindFrontend::BindRequestState::delete_bind_stage_() noexcept
  {
    if(request_info->external_id.empty() || !frontend->has_user_bind_client_())
    {
      finish_(BindResult(request_info->user_id));
      return;
    }

    adserver::user_info_svcs::user_bind::AddUserIdRequest add_user_request;
    add_user_request.set_id(request_info->external_id);
    add_user_request.set_user_id(GrpcAlgs::pack_user_id(Commons::UserId()));
    add_user_request.set_timestamp(GrpcAlgs::pack_time(request_info->time));

    auto self = shared_from_this();
    frontend->add_user_id_(
      add_user_request,
      [self](
        const grpc::Status& status,
        const adserver::user_info_svcs::user_bind::AddUserIdResponse&)
      {
        self->delete_bind_done_stage_(status);
      });
  }

  void
  UserBindFrontend::BindRequestState::delete_bind_done_stage_(
    const grpc::Status& status)
    noexcept
  {
    finish_(BindResult(request_info->user_id), status.ok() ? 204 : 500);
  }

  void
  UserBindFrontend::BindRequestState::external_ids_prepare_stage_() noexcept
  {
    if(cresolve_failed)
    {
      finish_result_stage_();
      return;
    }

    if(frontend->has_user_bind_client_())
    {
      external_ids = {
        ExternalId{request_info->ga_user_id, false, true},
        ExternalId{request_info->gclu_user_id, false, true},
        ExternalId{request_info->ym_user_id, false, true},
        ExternalId{request_info->external_id, frontend->config_->set_uid(), !app_request}
      };

      resolved_ext_user_i = external_ids.size();
      if(result_user_id.is_null())
      {
        external_id_resolve_stage_(0);
        return;
      }
    }

    external_id_add_stage_(0);
  }

  void
  UserBindFrontend::BindRequestState::external_id_resolve_stage_(
    std::size_t index)
    noexcept
  {
    for(; index < external_ids.size(); ++index)
    {
      if(!external_ids[index].id.empty())
      {
        break;
      }
    }

    if(index >= external_ids.size())
    {
      external_id_add_stage_(0);
      return;
    }

    adserver::user_info_svcs::user_bind::GetUserIdRequest get_request;
    get_request.set_id(external_ids[index].id);
    get_request.set_timestamp(GrpcAlgs::pack_time(request_info->time));
    get_request.set_silent(true);
    get_request.set_generate_user_id(external_ids[index].set_uid);
    get_request.set_for_set_cookie(!app_request);
    get_request.set_create_timestamp(
      GrpcAlgs::pack_time(Generics::Time::ZERO));

    auto self = shared_from_this();
    frontend->get_user_id_(
      get_request,
      [self, index](
        const grpc::Status& status,
        const adserver::user_info_svcs::user_bind::GetUserIdResponse&
          response)
      {
        self->external_id_resolve_done_stage_(index, status, response);
      });
  }

  void
  UserBindFrontend::BindRequestState::external_id_resolve_done_stage_(
    std::size_t index,
    const grpc::Status& status,
    const adserver::user_info_svcs::user_bind::GetUserIdResponse& response)
    noexcept
  {
    if(status.ok())
    {
      if(response.invalid_operation())
      {
        resolve_failed_external_ids.insert(external_ids[index].id);
        frontend->report_bad_user_(*request_info);
      }
      else
      {
        AdServer::Commons::UserId resolved_user_id =
          GrpcAlgs::unpack_user_id(response.user_id());
        if(!resolved_user_id.is_null())
        {
          result_user_id = resolved_user_id;
          result_user_id_type =
            external_ids[index].can_be_in_cookie ?
              RUIT_EXTIDRESOLVE : RUIT_EXTIDRESOLVE_NOCOOKIE;
          frontend->common_module_->user_id_controller()->
            null_blacklisted(result_user_id);

          if(frontend->config_->create_profile())
          {
            create_user_profile = response.created();
          }
        }
      }
    }
    else
    {
      http_status = 500;
    }

    if(!result_user_id.is_null())
    {
      resolved_ext_user_i = index;
      external_id_add_stage_(0);
    }
    else
    {
      external_id_resolve_stage_(index + 1);
    }
  }

  void
  UserBindFrontend::BindRequestState::external_id_add_stage_(
    std::size_t index)
    noexcept
  {
    if(!frontend->has_user_bind_client_() ||
      (!opted_out && result_user_id.is_null()))
    {
      generate_user_id_stage_();
      return;
    }

    for(; index < external_ids.size(); ++index)
    {
      const auto& external_id = external_ids[index].id;
      if(!external_id.empty() &&
        index != resolved_ext_user_i &&
        resolve_failed_external_ids.find(external_id) ==
          resolve_failed_external_ids.end())
      {
        break;
      }
    }

    if(index >= external_ids.size())
    {
      generate_user_id_stage_();
      return;
    }

    adserver::user_info_svcs::user_bind::AddUserIdRequest add_user_request;
    add_user_request.set_id(external_ids[index].id);
    add_user_request.set_user_id(GrpcAlgs::pack_user_id(
      !opted_out ? result_user_id : Commons::UserId()));
    add_user_request.set_timestamp(GrpcAlgs::pack_time(request_info->time));

    auto self = shared_from_this();
    frontend->add_user_id_(
      add_user_request,
      [self, index](
        const grpc::Status& status,
        const adserver::user_info_svcs::user_bind::AddUserIdResponse& response)
      {
        self->external_id_add_done_stage_(index, status, response);
      });
  }

  void
  UserBindFrontend::BindRequestState::external_id_add_done_stage_(
    std::size_t index,
    const grpc::Status& status,
    const adserver::user_info_svcs::user_bind::AddUserIdResponse& response)
    noexcept
  {
    if(status.ok())
    {
      if(response.invalid_operation())
      {
        frontend->report_bad_user_(*request_info);
      }
    }
    else
    {
      http_status = 500;
    }
    external_id_add_stage_(index + 1);
  }

  void
  UserBindFrontend::BindRequestState::generate_user_id_stage_() noexcept
  {
    if(!opted_out &&
      !app_request && (
        !result_user_id.is_null() || resolve_failed_external_ids.empty()))
    {
      Generics::Uuid generated_user_id = result_user_id.is_null() ?
        Generics::Uuid::create_random_based() :
        result_user_id;

      if(request_info->generate_external_id)
      {
        result_ssp_user_id =
          frontend->common_module_->user_id_controller()->ssp_uuid(
            generated_user_id,
            request_info->source_id);
      }

      if(frontend->config_->set_uid())
      {
        result_user_id = generated_user_id;
        result_user_id_type = RUIT_COOKIE;
      }
    }

    finish_result_stage_();
  }

  void
  UserBindFrontend::BindRequestState::finish_result_stage_() noexcept
  {
    BindResult bind_result;
    bind_result.result_user_id = result_user_id;
    if(!result_ssp_user_id.is_null())
    {
      bind_result.ssp_user_id = result_ssp_user_id;
    }
    else if(!app_request && !result_user_id.is_null())
    {
      bind_result.ssp_user_id =
        frontend->common_module_->user_id_controller()->ssp_uuid(
          result_user_id,
          request_info->source_id);
    }

    user_match_stage_();
    bind_request_stage_(bind_result);
  }

  void
  UserBindFrontend::BindRequestState::user_match_stage_() noexcept
  {
    std::string ifa_str;
    if(request_info->external_id.compare(0, 4, "ifa/") == 0)
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

  void
  UserBindFrontend::BindRequestState::bind_request_stage_(
    const BindResult& bind_result)
    noexcept
  {
    if(dns_bind_request_id.empty() || !frontend->has_user_bind_client_())
    {
      finish_(bind_result);
      return;
    }

    AdServer::Commons::ExternalUserIdArray user_ids;
    if(!result_user_id.is_null())
    {
      if(result_user_id_type == RUIT_CRESOLVE ||
        result_user_id_type == RUIT_EXTIDRESOLVE_NOCOOKIE)
      {
        user_ids.push_back(std::string("/") + result_user_id.to_string());
      }
      else
      {
        user_ids.push_back(std::string("c/") + result_user_id.to_string());
      }
    }

    if(!app_request &&
      !request_info->user_id.is_null() &&
      !(request_info->user_id == result_user_id))
    {
      user_ids.push_back(std::string("c/") + request_info->user_id.to_string());
    }

    if(!request_info->ga_user_id.empty())
    {
      user_ids.push_back(request_info->ga_user_id);
    }

    if(!request_info->ym_user_id.empty())
    {
      user_ids.push_back(request_info->ym_user_id);
    }

    if(!request_info->external_id.empty())
    {
      user_ids.push_back(request_info->external_id);
    }

    if(!request_info->add_user_id.is_null() &&
      request_info->add_user_id != result_user_id &&
      request_info->add_user_id != request_info->user_id)
    {
      user_ids.push_back(std::string("/") +
        request_info->add_user_id.to_string());
    }

    adserver::user_info_svcs::user_bind::AddBindRequestRequest bind_request;
    bind_request.set_request_id(dns_bind_request_id);
    bind_request.set_timestamp(GrpcAlgs::pack_time(request_info->time));
    for(const auto& user_id : user_ids)
    {
      bind_request.add_bind_user_ids(user_id);
    }

    auto self = shared_from_this();
    frontend->add_bind_request_(
      bind_request,
      [self, bind_result](
        const grpc::Status&,
        const adserver::user_info_svcs::user_bind::AddBindRequestResponse&)
      {
        self->bind_request_done_stage_(bind_result);
      });
  }

  void
  UserBindFrontend::BindRequestState::bind_request_done_stage_(
    const BindResult& bind_result)
    noexcept
  {
    finish_(bind_result);
  }

  void
  UserBindFrontend::BindRequestState::finish_(
    const BindResult& bind_result,
    int status)
    noexcept
  {
    callback(status >= 0 ? status : http_status, bind_result);
  }
}
