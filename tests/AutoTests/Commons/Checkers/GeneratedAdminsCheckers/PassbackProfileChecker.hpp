
#ifndef __AUTOTESTS_COMMONS_ADMINS_PASSBACKPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_PASSBACKPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/PassbackProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType, CheckType ch = CT_ONE>
  class PassbackProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    
            
      typedef ExpectedType Expected;

      PassbackProfileChecker_(      
        BaseUnit* test,      
        const std::string& requestid,      
        RequestInfoSrv service,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        requestid_(requestid),      
        service_(service),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~PassbackProfileChecker_() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      std::string requestid_;      
      RequestInfoSrv service_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  

  typedef PassbackProfileChecker_<PassbackProfileAdmin::Expected> PassbackProfileChecker;
  typedef PassbackProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> PassbackProfileEmptyChecker;

}

#include "PassbackProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_PASSBACKPROFILECHECKER_HPP

