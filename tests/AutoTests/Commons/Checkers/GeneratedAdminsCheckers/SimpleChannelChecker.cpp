
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include "SimpleChannelChecker.hpp"
          
#include <tests/AutoTests/Commons/Stats/ORMStats.hpp>

namespace AutoTest
{
  bool  
  SimpleChannelChecker::check(bool throw_error)  
    /*throw(CheckFailed, eh::Exception)*/  
  {
    AdminExistCheck remote_exists =
      !expected_.has_status() ||
         expected_.status() == "A" ||
           expected_.status() == "W"?
        exists_: AEC_NOT_EXISTS;

    AdminsArray<SimpleChannelAdmin, CT_ALL> central_admins;

    central_admins.initialize(
      test_,
      CTE_CENTRAL,
      STE_CAMPAIGN_SERVER,
      id_);

    AdminsArray<SimpleChannelAdmin, CT_ALL> remote_admins;

    remote_admins.initialize(
      test_,
      CTE_ALL_REMOTE,
      STE_CAMPAIGN_SERVER,
      id_);

    if (central_admins.empty() && remote_admins.empty())
    {
      throw CheckFailed("Admin containers are empty!");
    }

    central_admins.log(Logger::thlog());
    remote_admins.log(Logger::thlog());

    return
      (central_admins.empty() ||
        admin_checker(
          central_admins,
          expected_,
          exists_).
            check(throw_error)) &&
      (remote_admins.empty() ||
        admin_checker(
          remote_admins,
          remote_exists == AEC_EXISTS? expected_: Expected(),
          remote_exists).
            check(throw_error));
        
  }  
}
