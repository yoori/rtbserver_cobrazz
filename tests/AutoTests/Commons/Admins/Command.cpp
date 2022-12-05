#include "Command.hpp"
#include "Admins.hpp"

namespace AutoTest
{

  struct ServiceDsc
  {
    const char* name;
    const char* admin;
    ServiceTypeEnum srv_type;
  };

  const ServiceDsc SERVICES[] =
  {
    { "CampaignManager", "CampaignAdmin", STE_CAMPAIGN_MANAGER },
    { "CampaignServer", "CampaignAdmin",  STE_CAMPAIGN_SERVER },
    { "UserInfoManager",  "UserInfoAdmin", STE_USER_INFO_MANAGER },
    { "UserInfoManagerController", "UserInfoAdmin", STE_USER_INFO_MANAGER_CONTROLLER },
    { "ChannelManagerController", "ChannelAdmin", STE_CHANNEL_CONTROLLER },
    { "ChannelServer", "ChannelAdmin", STE_CHANNEL_SERVER },
    { "ChannelUpdate", "ChannelAdmin", STE_CHANNEL_SERVER },
    { "ChannelSearch", "ChannelSearchAdmin", STE_CHANNEL_SEARCH_SERVER },
    { "RequestInfoManager", "RequestInfoAdmin", STE_REQUEST_INFO_MANAGER },
    { "ExpressionMatcher", "ExpressionMatcherAdmin", STE_EXPRESSION_MATCHER }
  };

  ServiceTypeEnum
  srv_type_by_index(
    size_t srv_id) /*throw(InvalidService)*/
  {
    if (srv_id >= sizeof(SERVICES)/sizeof(*SERVICES))
    {
      Stream::Error error;
      error << "Undefined service#" << srv_id;
      throw InvalidService(error);
    }
    return SERVICES[srv_id].srv_type;
  }


  void make_admin_cmd(
    ShellCmd& cmd,
    const char* aspect,
    const char* address,
    const AdminParams& params,
    size_t srv_id) /*throw(InvalidService, eh::Exception)*/
  {
    if (srv_id >= sizeof(SERVICES)/sizeof(*SERVICES))
    {
      Stream::Error error;
      error << "Undefined service#" << srv_id;
      throw InvalidService(error);
    }
    cmd.clear();
    cmd.add_cmd_i(SERVICES[srv_id].admin);
    cmd.add_cmd_i(aspect);
    for (AdminParams::const_iterator it=params.begin();
         it != params.end(); ++it)
    {
      if (it->first.empty() || *it->first.rbegin() != '=')
      {
        cmd.add_cmd_i(it->first);
        cmd.add_cmd_i(it->second);
      }
      else
      {
        cmd.add_cmd_i(it->first + it->second);
      }
    }
    cmd.add_cmd_i("-r");
    cmd.add_cmd_i(std::string("corbaloc::") + address + "/" +  SERVICES[srv_id].name);
  }
}
