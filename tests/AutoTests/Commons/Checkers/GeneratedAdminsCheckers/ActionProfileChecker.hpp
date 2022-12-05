
#ifndef __AUTOTESTS_COMMONS_ADMINS_ACTIONPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_ACTIONPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/ActionProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType, CheckType ch = CT_ONE>
  class ActionProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    
            
      typedef ExpectedType Expected;

      ActionProfileChecker_(      
        BaseUnit* test,      
        const std::string& uuid,      
        RequestInfoSrv service,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        uuid_(uuid),      
        service_(service),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~ActionProfileChecker_() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      std::string uuid_;      
      RequestInfoSrv service_;
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  

  typedef ActionProfileChecker_<ActionProfileAdmin::Expected> ActionProfileChecker;
  typedef ActionProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> ActionProfileEmptyChecker;

}

#include "ActionProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_ACTIONPROFILECHECKER_HPP

