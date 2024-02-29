#ifndef USERINFOCOMMONS_STATISTICS_HPP_
#define USERINFOCOMMONS_STATISTICS_HPP_

// STD
#include <map>
#include <string>

// UNIXCOMMONS
#include <UServerUtils/Grpc/Statistics/TimeStatisticsProvider.hpp>

namespace AdServer::UserInfoSvcs
{

extern const std::string time_provider_name;

enum class TimeStatisticId
{
  UserBindServer_GetUserIdCorba,
  UserBindServer_GetUserIdGrpc,
  UserBindServer_AddUserIdCorba,
  UserBindServer_AddUserIdGrpc,
  UserInfoManager_GetUserProfile,
  UserInfoManager_Match,
  Max
};

class TimeStatisticIdToStringConverter final
{
public:
  TimeStatisticIdToStringConverter() = default;

  ~TimeStatisticIdToStringConverter() = default;

  auto operator()()
  {
    const std::map<TimeStatisticId, std::string> id_to_name = {
      {TimeStatisticId::UserBindServer_GetUserIdCorba, "UserBindServer:get_user_id_corba"},
      {TimeStatisticId::UserBindServer_GetUserIdGrpc, "UserBindServer:get_user_id_grpc"},
      {TimeStatisticId::UserBindServer_AddUserIdCorba, "UserBindServer:add_user_id_corba"},
      {TimeStatisticId::UserBindServer_AddUserIdGrpc, "UserBindServer:add_user_id_grpc"},
      {TimeStatisticId::UserInfoManager_GetUserProfile, "UserInfoManager:get_user_profile"},
      {TimeStatisticId::UserInfoManager_Match, "UserInfoManager:match"}
    };

    return id_to_name;
  }
};

inline auto& get_time_statistics_provider()
{
  return UServerUtils::Statistics::get_time_statistics_provider<
    TimeStatisticId,
    TimeStatisticIdToStringConverter,
    4,
    50>(time_provider_name);
}

} // namespace AdServer::UserInfoSvcs

#define DO_TIME_STATISTIC_USER_INFO(id) \
  auto measure = AdServer::UserInfoSvcs::get_time_statistics_provider()->make_measure(id);

#endif // USERINFOCOMMONS_STATISTICS_HPP_