
#ifndef __AUTOTESTS_COMMONS_ADMINS_SITECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_SITECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/SiteAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  class SiteChecker:  
    public AutoTest::Checker  
  {  
    public:    
      typedef SiteAdmin::Expected Expected;      
            
      SiteChecker(      
        BaseUnit* test,      
        unsigned long site,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        site_(site),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~SiteChecker() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      unsigned long site_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_SITECHECKER_HPP

