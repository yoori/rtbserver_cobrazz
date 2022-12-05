
#ifndef __AUTOTESTS_COMMONS_ADMINS_REQUESTTRIGGERMATCHPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_REQUESTTRIGGERMATCHPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/RequestTriggerMatchProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType, CheckType ch = CT_ONE>
  class RequestTriggerMatchProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    
            
      typedef ExpectedType Expected;

      RequestTriggerMatchProfileChecker_(      
        BaseUnit* test,      
        const std::string& uid,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        uid_(uid),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~RequestTriggerMatchProfileChecker_() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      std::string uid_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  

  typedef RequestTriggerMatchProfileChecker_<RequestTriggerMatchProfileAdmin::Expected> RequestTriggerMatchProfileChecker;
  typedef RequestTriggerMatchProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> RequestTriggerMatchProfileEmptyChecker;
}

#include "RequestTriggerMatchProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_REQUESTTRIGGERMATCHPROFILECHECKER_HPP

