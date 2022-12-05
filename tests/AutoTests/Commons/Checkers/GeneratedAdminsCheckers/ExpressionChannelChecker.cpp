
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include "ExpressionChannelChecker.hpp"

#include <tests/AutoTests/Commons/Stats/ORMStats.hpp>

namespace AutoTest
{

  const ExpressionChannelChecker::Expected ExpressionChannelChecker::ANY_EXPECTED_;

  ExpressionChannelChecker::ExpressionChannelChecker(      
    BaseUnit* test,      
    unsigned long channel_id,      
    const Expected& expected,      
    AdminExistCheck exists)
    : test_(test),      
      channel_id_(channel_id),      
      expected_(expected),      
      exists_(exists)      
  { }

  ExpressionChannelChecker::~ExpressionChannelChecker() noexcept
  { }
  
  bool  
  ExpressionChannelChecker::check(bool throw_error)  
    /*throw(CheckFailed, eh::Exception)*/  
  {
    
    if (expected_.has_status())
    {
      // To calc new account display status
      AutoTest::ORM::update_display_status(
        test_, "CHANNEL", static_cast<int>(channel_id_));
    }
    
    AdminExistCheck remote_exists =
      !expected_.has_status() ||
         expected_.status() == "A" ||
           expected_.status() == "W"?
        exists_: AEC_NOT_EXISTS;

    AdminsArray<ExpressionChannelAdmin, CT_ALL> central_admins;

    central_admins.initialize(test_,
      CTE_CENTRAL, STE_CAMPAIGN_SERVER,
      channel_id_);
    
    AdminsArray<ExpressionChannelAdmin, CT_ALL> remote_admins;

    remote_admins.initialize(test_,
      CTE_ALL_REMOTE, STE_CAMPAIGN_SERVER,
      channel_id_);

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
          remote_exists == AEC_EXISTS? expected_: ANY_EXPECTED_,
          remote_exists).
            check(throw_error));
  }
}
