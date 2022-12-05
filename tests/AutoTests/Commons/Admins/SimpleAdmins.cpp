
#include "SimpleAdmins.hpp"
#include "Command.hpp"
#include "AdminsContainer.hpp"
#include <tests/AutoTests/Commons/GlobalSettings.hpp>

namespace AutoTest
{

  // class ChannelMatchLog

  void
  ChannelMatchLog::make_cmd (
    const char* address,
    const char* trigger,
    ChannelSrv service)
  {
    AdminParams params;
    if (trigger)
    {
      params.push_back(AdminParamPair("-p", trigger));
      params.push_back(AdminParamPair("-e", trigger));
      params.push_back(AdminParamPair("-u", trigger));
    }
    make_admin_cmd(*this, "match", address, params, static_cast<size_t>(service));
    add_cmd_i("-d");
    add_cmd_i("-f");
    add_cmd_i("-l");
  }

  ChannelMatchLog::ChannelMatchLog(
    const char* address,
    const char* trigger,
    ChannelSrv service)
  {
    make_cmd(address, trigger, service);
  }

  ChannelMatchLog::ChannelMatchLog(
    const std::string& address,
    const char* trigger,
    ChannelSrv service)
  {
    make_cmd(address.c_str(), trigger, service);
  }

  ChannelMatchLog::ChannelMatchLog(
    const char* address,
    const std::string& trigger,
    ChannelSrv service)
  {
    make_cmd(address, trigger.c_str(), service);
  }

  ChannelMatchLog::ChannelMatchLog(
    const std::string& address,
    const std::string& trigger,
    ChannelSrv service)
  {
    make_cmd(address.c_str(), trigger.c_str(), service);
  }

  // class UserInfoAdminLog

  void
  UserInfoAdminLog::make_cmd(
    const char* address,
    const char* uuid,
    UserInfoSrv service,
    bool temp)
  {
    AdminParams params;
    if (uuid)
    {
      if (temp)
      {
        params.push_back(AdminParamPair("--tuid=", uuid));
      }
      else
      {
        params.push_back(AdminParamPair("--uid=", uuid));
      }
    }
    make_admin_cmd(*this, "print", address, params, static_cast<size_t>(service));
  }

  UserInfoAdminLog::UserInfoAdminLog(
    const char* address,
    const char* uuid,
    UserInfoSrv service,
    bool temp)
  {
    make_cmd(address, uuid, service, temp);
  }

  // class DeleteOldUserProfiles

  void
  DeleteOldUserProfiles::make_cmd(
    const char* address)
  {

    AdminParams params;

    make_admin_cmd(
      *this,
      "delete-old-profiles",
      address,
      params,
      static_cast<size_t>(UserInfoManagerController));

    add_cmd_i("--sync");
  }

  void
  DeleteOldUserProfiles::execute(BaseUnit* test)
    /*throw(eh::Exception)*/
  {
    AdminsArray<DeleteOldUserProfiles> admins;
    admins.initialize(test, CTE_ALL, STE_USER_INFO_MANAGER_CONTROLLER);
    admins.exec();
  }

  DeleteOldUserProfiles::DeleteOldUserProfiles(
    const char* address)
  {
    make_cmd(address);
  }


  // class ClearExpiredProfiles

  void
  ClearExpiredProfiles::make_cmd(
    const char* address)
  {
    AdminParams params;

    make_admin_cmd(
      *this,
      "clear-expired",
      address,
      params,
      static_cast<size_t>(RequestInfoManager));

    add_cmd_i("--sync");
  }

  void
  ClearExpiredProfiles::execute(BaseUnit* test)
    /*throw(eh::Exception)*/
  {
    AdminsArray<ClearExpiredProfiles> admins;
    admins.initialize(test, CTE_ALL, STE_REQUEST_INFO_MANAGER);
    admins.exec();
  }

  ClearExpiredProfiles::ClearExpiredProfiles(
    const char* address)
  {
    make_cmd(address);
  }

  // class UpdateStats

  void
  UpdateStats::make_cmd(
    const char* address)
  {
    AdminParams params;

    make_admin_cmd(
      *this,
      "update_stat",
      address,
      params,
      static_cast<size_t>(CampaignServer));
  }

  void
  UpdateStats::execute(BaseUnit* test)
    /*throw(eh::Exception)*/
  {
    AdminsArray<UpdateStats> admins;
    admins.initialize(test, CTE_ALL, STE_CAMPAIGN_SERVER);
    admins.exec();
  }

  UpdateStats::UpdateStats(
    const char* address)
  {
    make_cmd(address);
  }

  // class DailyProcess

  void
  DailyProcess::execute(BaseUnit* test)
    /*throw(eh::Exception)*/
  {
    AdminsArray<DailyProcess> admins;
    admins.initialize(test, CTE_ALL, STE_EXPRESSION_MATCHER);
    admins.exec();
  }

  void
  DailyProcess::make_cmd(
    const char* address)
  {
    AdminParams params;
    params.push_back(AdminParamPair("-s", std::string()));

    make_admin_cmd(
      *this,
      "daily-process",
      address,
      params,
      static_cast<size_t>(ExpressionMatcher));
  }

  DailyProcess::DailyProcess(
    const char* address)
  {
    make_cmd(address);
  }

  // class CopyCmd

  const char* CopyCmd::DEFAULT_EXECUTOR_ = "/usr/bin/rsync";

  CopyCmd::CopyCmd(
    const std::string& from,
    const std::string& to)
  {
    std::string cmd =
      GlobalSettings::instance().initialized() &&
      GlobalSettings::instance().config().Copy()/*->present()*/
        ? GlobalSettings::instance().config().Copy()->command()
        : DEFAULT_EXECUTOR_;

    String::StringManip::SplitSpace tokenizer(cmd);

    String::SubString arg;
    while(tokenizer.get_token(arg))
    {
      add_cmd_i(arg.str());
    }
    add_cmd_i(from);
    add_cmd_i(to);
  }

  void
  CopyCmd::exec()
  {
    ShellCmd::exec();
  }

  // class MoveCmd

  const char* MoveCmd::DEFAULT_EXECUTOR_ = "/usr/bin/rsync --remove-source-files";

  MoveCmd::MoveCmd(
    const std::string& from,
    const std::string& to)
  {
    std::string cmd =
      GlobalSettings::instance().initialized() &&
      GlobalSettings::instance().config().Move()/*->present()*/
        ? GlobalSettings::instance().config().Move()->command()
        : DEFAULT_EXECUTOR_;

    String::StringManip::SplitSpace tokenizer(cmd);

    String::SubString arg;
    while(tokenizer.get_token(arg))
    {
      add_cmd_i(arg.str());
    }
    add_cmd_i(from);
    add_cmd_i(to);
  }

  void
  MoveCmd::exec()
  {
    ShellCmd::exec();
  }
}


