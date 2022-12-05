
#ifndef __AUTOTESTS_COMMONS_ADMINS_ACTIONCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_ACTIONCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/ActionAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  class ActionChecker:  
    public AutoTest::Checker  
  {  
    public:    
      typedef ActionAdmin::Expected Expected;      
            
      ActionChecker(      
        BaseUnit* test,      
        unsigned long action,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        action_(action),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~ActionChecker() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      unsigned long action_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_ACTIONCHECKER_HPP

