
#ifndef __AUTOTESTS_COMMONS_ADMINS_SITEREACHPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_SITEREACHPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/SiteReachProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType, CheckType ch = CT_ONE>
  class SiteReachProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    
            
      typedef ExpectedType Expected;

      SiteReachProfileChecker_(      
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
            
      virtual ~SiteReachProfileChecker_() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      std::string uuid_;      
      RequestInfoSrv service_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };

  typedef SiteReachProfileChecker_<SiteReachProfileAdmin::Expected> SiteReachProfileChecker;  
  typedef SiteReachProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> SiteReachProfileEmptyChecker;
}

#include "SiteReachProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_SITEREACHPROFILECHECKER_HPP

