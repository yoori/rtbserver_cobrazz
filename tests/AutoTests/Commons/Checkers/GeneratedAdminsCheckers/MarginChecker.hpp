
#ifndef __AUTOTESTS_COMMONS_ADMINS_MARGINCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_MARGINCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/MarginAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  class MarginChecker:  
    public AutoTest::Checker  
  {  
    public:    
      typedef MarginAdmin::Expected Expected;      
            
      MarginChecker(      
        BaseUnit* test,      
        unsigned long margin,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        margin_(margin),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~MarginChecker() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      unsigned long margin_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_MARGINCHECKER_HPP

