// THIS
#include <Commons/CorbaAlgs.hpp>
#include <Frontends/FrontendCommons/HTTPExceptions.hpp>
#include <Frontends/Modules/BiddingFrontend/JsonFormatter.hpp>
#include <Frontends/Modules/UserBindFrontend/UserChannelsHandler.hpp>

namespace AdServer
{

namespace Aspect
{

const char USER_CHANNELS_HANDLER[] = "UserChannelsHandler";

} // namespace Aspect

namespace Response::Type
{

const String::SubString JSON("application/json");

} // namespace Type

UserChannelsHandler::UserChannelsHandler(
  const GrpcContainerPtr& grpc_container,
  UserInfoClient* user_info_client,
  Logger* logger)
  : grpc_container_(grpc_container),
    user_info_client_(ReferenceCounting::add_ref(user_info_client)),
    logger_(ReferenceCounting::add_ref(logger))
{
}

int UserChannelsHandler::handle(
  const RequestInfo& request_info,
  HttpResponse& response)
{
  using ProfilesRequestInfoProto = AdServer::UserInfoSvcs::Types::ProfilesRequestInfo;
  using ProfilesRequestInfoCorba = AdServer::UserInfoSvcs::ProfilesRequestInfo;
  using ChannelIdSeq_var = AdServer::UserInfoSvcs::ChannelIdSeq_var;
  using WlChannelIdSeq = AdServer::UserInfoSvcs::WlChannelIdSeq;
  using InvalidParamException = FrontendCommons::HTTPExceptions::InvalidParamException;
  using Exception = FrontendCommons::HTTPExceptions::Exception;
  using UserChannels = std::vector<std::uint32_t>;
  using JsonFormatter = AdServer::Commons::JsonFormatter;
  using JsonObject = AdServer::Commons::JsonObject;

  static const String::SubString kJsonSessionIdName("session_id");
  static const String::SubString kJsonClIdName("cl_id");
  static const String::SubString kJsonSegmentsdName("segments");

  AdServer::Commons::UserId user_id;
  if (!request_info.param_user_id.empty())
  {
    try
    {
      user_id = AdServer::Commons::UserId(
        request_info.param_user_id.data(),
        request_info.param_user_id.size());
    }
    catch (...)
    {
      Stream::Error ostream;
      ostream << FNS
              << "user_id from params is bad";
      throw InvalidParamException(ostream);
    }
  }

  if (user_id.is_null())
  {
    user_id = request_info.user_id;
  }

  if (user_id.is_null())
  {
    Stream::Error ostream;
    ostream << FNS
            << "user_id is failed";
    throw InvalidParamException(ostream);
  }

  UserChannels user_channels;
  auto& grpc_user_info_operation_distributor =
    grpc_container_->grpc_user_info_operation_distributor;
  if (/*grpc_user_info_operation_distributor*/false)
  {
    ProfilesRequestInfoProto profiles_request;
    profiles_request.base_profile = true;
    profiles_request.add_profile = false;
    profiles_request.history_profile = false;
    profiles_request.freq_cap_profile = false;

    auto response = grpc_user_info_operation_distributor->get_user_channels(
      GrpcAlgs::pack_user_id(user_id),
      profiles_request,
      request_info.channels_wl);
    if (!response)
    {
      Stream::Error ostream;
      ostream << FNS
              << "GrpcUserInfoOperationDistributor::get_user_channels is failed";
      logger_->log(
        ostream.str(),
        Logging::Logger::ERROR,
        Aspect::USER_CHANNELS_HANDLER);
      throw Exception(ostream);
    }

    if (response->has_error())
    {
      GrpcAlgs::print_grpc_error_response(
        response,
        logger_,
        Aspect::USER_CHANNELS_HANDLER);
      Stream::Error ostream;
      ostream << FNS
              << "GrpcUserInfoOperationDistributor::get_user_channels is failed";
      throw Exception(ostream);
    }

    const auto& info_proto = response->info();
    const auto& channels_ids_proto = info_proto.channels_ids();
    user_channels.reserve(channels_ids_proto.size());
    user_channels.insert(
      std::end(user_channels),
      std::begin(channels_ids_proto),
      std::end(channels_ids_proto));
  }
  else
  {
    try
    {
      AdServer::UserInfoSvcs::UserInfoMatcher_var uim_session =
        user_info_client_->user_info_session();
      if (!uim_session)
      {
        throw Exception("uim_session is null");
      }

      ProfilesRequestInfoCorba profiles_request;
      profiles_request.base_profile = true;
      profiles_request.add_profile = false;
      profiles_request.history_profile = false;
      profiles_request.freq_cap_profile = false;

      WlChannelIdSeq wl_channel_ids;
      if constexpr (std::is_same_v<
        WlChannelIdSeq::value_type,
        decltype(request_info.channels_wl)::value_type>)
      {
        if (!request_info.channels_wl.empty())
        {
          wl_channel_ids.length(request_info.channels_wl.size());
          std::memcpy(
            wl_channel_ids.get_buffer(),
            request_info.channels_wl.data(),
            request_info.channels_wl.size() * sizeof(WlChannelIdSeq::value_type));
        }
      }
      else
      {
        if (!request_info.channels_wl.empty())
        {
          wl_channel_ids.length(request_info.channels_wl.size());
          const std::size_t size = request_info.channels_wl.size();
          for (std::size_t i = 0; i < size; i += 1)
          {
            wl_channel_ids[i] = request_info.channels_wl[i];
          }
        }
      }

      ChannelIdSeq_var channels_ids_out;
      uim_session->get_user_channels(
        CorbaAlgs::pack_user_id(user_id),
        profiles_request,
        wl_channel_ids,
        channels_ids_out.out());

      if (!channels_ids_out.ptr())
      {
        Stream::Error stream;
        stream << FNS
               << "UserInfoMatcher::get_user_channels is failed. "
                  "match_result is null.";
        logger_->log(
          stream.str(),
          Logging::Logger::ERROR,
          Aspect::USER_CHANNELS_HANDLER);
        throw Exception(stream);
      }

      user_channels.insert(
        std::end(user_channels),
        channels_ids_out->get_buffer(),
        channels_ids_out->get_buffer() + channels_ids_out->length());
    }
    catch (const eh::Exception& exception)
    {
      Stream::Error ostream;
      ostream << FNS
              << exception.what();
      logger_->log(
        ostream.str(),
        Logging::Logger::ERROR,
        Aspect::USER_CHANNELS_HANDLER);
      throw;
    }
    catch (...)
    {
      Stream::Error ostream;
      ostream << FNS
              << "Unknown error";
      logger_->log(
        ostream.str(),
        Logging::Logger::ERROR,
        Aspect::USER_CHANNELS_HANDLER);
      throw;
    }
  }

  std::string response_string;
  {
    JsonFormatter root_json(response_string);
    if (!request_info.session_id.empty())
    {
      root_json.add(kJsonSessionIdName, request_info.session_id);
    }
    if (!request_info.cl_id.empty())
    {
      root_json.add(kJsonClIdName, request_info.cl_id);
    }

    {
      JsonObject segment_array(
        root_json.add_array(kJsonSegmentsdName));
      for (const auto user_channel : user_channels)
      {
        segment_array.add_number(user_channel);
      }
    }
  }

  response.set_content_type(Response::Type::JSON);
  response.write(std::move(response_string));

  return 200;
}

} // namespace AdServer