
#ifndef __AUTOTESTS_COMMONS_ADMINS_REACHPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_REACHPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/ReachProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType, CheckType ch = CT_ONE>
  class ReachProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    
            
      typedef ExpectedType Expected;

      ReachProfileChecker_(      
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
            
      virtual ~ReachProfileChecker_() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      std::string uuid_;      
      RequestInfoSrv service_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  

  typedef ReachProfileChecker_<ReachProfileAdmin::Expected> ReachProfileChecker;
  typedef ReachProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> ReachProfileEmptyChecker;

}

#include "ReachProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_REACHPROFILECHECKER_HPP

