
#ifndef __AUTOTESTS_COMMONS_ADMINS_CREATIVECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CREATIVECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/CreativeAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  class CreativeChecker:  
    public AutoTest::Checker  
  {  
    public:    
      typedef CreativeAdmin::Expected Expected;      
            
      CreativeChecker(      
        BaseUnit* test,      
        unsigned long ccid,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        ccid_(ccid),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~CreativeChecker() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      unsigned long ccid_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_CREATIVECHECKER_HPP

