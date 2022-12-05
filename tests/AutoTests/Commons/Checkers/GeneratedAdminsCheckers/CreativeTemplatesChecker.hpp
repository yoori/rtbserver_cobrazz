
#ifndef __AUTOTESTS_COMMONS_ADMINS_CREATIVETEMPLATESCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CREATIVETEMPLATESCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/CreativeTemplatesAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  class CreativeTemplatesChecker:  
    public AutoTest::Checker  
  {  
    public:    
      typedef CreativeTemplatesAdmin::Expected Expected;      
            
      CreativeTemplatesChecker(      
        BaseUnit* test,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~CreativeTemplatesChecker() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_CREATIVETEMPLATESCHECKER_HPP

