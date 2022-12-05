
#ifndef __AUTOTESTS_COMMONS_ADMINS_USERTRIGGERMATCHPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_USERTRIGGERMATCHPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/UserTriggerMatchProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<class ExpectedType, CheckType ch = CT_ONE>
  class UserTriggerMatchProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    
            
      typedef ExpectedType Expected;

      UserTriggerMatchProfileChecker_(      
        BaseUnit* test,      
        const std::string& uid,      
        bool temp,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        uid_(uid),      
        temp_(temp),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~UserTriggerMatchProfileChecker_() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      std::string uid_;      
      bool temp_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };

  typedef UserTriggerMatchProfileChecker_<UserTriggerMatchProfileAdmin::Expected> UserTriggerMatchProfileChecker;
  typedef UserTriggerMatchProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> UserTriggerMatchProfileEmptyChecker;
}

#include "UserTriggerMatchProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_USERTRIGGERMATCHPROFILECHECKER_HPP

