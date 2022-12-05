
#ifndef __AUTOTESTS_COMMONS_ADMINS_FRAUDPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_FRAUDPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/FraudProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType, CheckType ch = CT_ONE>
  class FraudProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    
            
      typedef ExpectedType Expected;

      FraudProfileChecker_(      
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
            
      virtual ~FraudProfileChecker_() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      std::string uuid_;      
      RequestInfoSrv service_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  

  typedef FraudProfileChecker_<FraudProfileAdmin::Expected> FraudProfileChecker;
  typedef FraudProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> FraudProfileEmptyChecker;

}

#include "FraudProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_FRAUDPROFILECHECKER_HPP

