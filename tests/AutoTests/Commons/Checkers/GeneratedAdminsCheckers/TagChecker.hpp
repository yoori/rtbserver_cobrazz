
#ifndef __AUTOTESTS_COMMONS_ADMINS_TAGCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_TAGCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/TagAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  class TagChecker:  
    public AutoTest::Checker  
  {  
    public:    
      typedef TagAdmin::Expected Expected;      
            
      TagChecker(      
        BaseUnit* test,      
        unsigned long tag,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        tag_(tag),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~TagChecker() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      unsigned long tag_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_TAGCHECKER_HPP

