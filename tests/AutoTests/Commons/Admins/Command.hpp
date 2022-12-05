#ifndef _AUTOTESTS_COMMONS_ADMINS_COMMAND_HPP
#define _AUTOTESTS_COMMONS_ADMINS_COMMAND_HPP

#include <vector>
#include <string>
#include <eh/Exception.hpp>
#include <tests/AutoTests/Commons/AutoTestsXsd.hpp>

namespace AutoTest
{
  class ShellCmd;

  DECLARE_EXCEPTION(InvalidService, eh::DescriptiveException);

  typedef std::pair<std::string, std::string> AdminParamPair;
  typedef std::vector <AdminParamPair> AdminParams;

  enum CampaignSrv
  {
    CampaignManager = 0,
    CampaignServer = 1
  };

  enum UserInfoSrv
  {
    UserInfoManager = 2,
    UserInfoManagerController = 3
  };

  enum ChannelSrv
  {
    ChannelManagerController = 4,
    ChannelServer = 5,
    ChannelUpdate = 6,
    ChannelSearch = 7
  };

  enum RequestInfoSrv
  {
    RequestInfoManager = 8,
  };

  enum ExpressionMatcherSrv
  {
    ExpressionMatcher = 9
  };

  /** srv_type_by_index
   * @brief Get service type by index
   * Receive command's arguments
   * @param service index.
   * @return service type.
   */
  ServiceTypeEnum
  srv_type_by_index(
    size_t srv_id) /*throw(InvalidService)*/;

  /** make_admin_cmd
   * @brief Make call string for admin
   * Receive command's arguments
   * and consruct call string using it.
   * @param cmd ShellCmd object for which this function creates command string.
   * @param aspect aspect parameter of CampaigAdmin command.
   * @param service address.
   * @param parameters vector.
   * @param service index.
   */
  void
  make_admin_cmd(
    ShellCmd& cmd,
    const char* aspect,
    const char* address,
    const AdminParams& params,
    size_t srv_id) /*throw(InvalidService, eh::Exception)*/;
}

#endif  //  _AUTOTESTS_COMMONS_ADMINS_COMMAND_HPP
