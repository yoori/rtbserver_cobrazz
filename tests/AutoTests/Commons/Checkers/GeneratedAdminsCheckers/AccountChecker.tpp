
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include <tests/AutoTests/Commons/Checkers/CompositeCheckers.hpp>
#include <tests/AutoTests/Commons/Stats/ORMStats.hpp>

namespace AutoTest
{

  template<typename ExpectedType>  
  bool  
  AccountChecker_<ExpectedType>::check(bool throw_error)  
    /*throw (CheckFailed, eh::Exception)*/  
  {  
    if (expected_.has_status())
    {
      // To calc new account display status
      AutoTest::ORM::update_display_status(
        test_, "ACCOUNT", static_cast<int>(account_));
    }

    AdminExistCheck remote_exists =
      expected_.force_remote_present() ||
        ((!expected_.has_status() || expected_.status() == "A") &&
          (!expected_.has_eval_status() || expected_.eval_status() == "A"))?
            exists_: AEC_NOT_EXISTS;

    AdminsArray<AccountAdmin, CT_ALL> central_admins;

    central_admins.initialize(
      test_,
      CTE_CENTRAL,
      STE_CAMPAIGN_MANAGER,
      account_);
    
    AdminsArray<AccountAdmin, CT_ALL> remote_admins;

    remote_admins.initialize(
      test_,
      CTE_ALL_REMOTE,
      STE_CAMPAIGN_MANAGER,
      account_);

    if (central_admins.empty() && remote_admins.empty())
    {
      throw CheckFailed("Admin containers are empty!");
    }

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
          remote_exists == AEC_EXISTS? expected_: ExpectedType(),
          remote_exists).
            check(throw_error));
  }
}
