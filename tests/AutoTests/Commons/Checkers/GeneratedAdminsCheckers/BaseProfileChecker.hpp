
#ifndef __AUTOTESTS_COMMONS_ADMINS_BASEPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_BASEPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/BaseProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType = BaseProfileAdmin::Expected, CheckType ch = CT_ONE>  
  class BaseProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    

      typedef ExpectedType Expected;      

      BaseProfileChecker_(      
        BaseUnit* test,      
        const std::string& uid,      
        bool temp,      
        UserInfoSrv service,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        uid_(uid),      
        temp_(temp),      
        service_(service),      
        expected_(expected),      
        exists_(exists)      
        {}      

      virtual ~BaseProfileChecker_() noexcept {}      

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:    

      BaseUnit* test_;      
      std::string uid_;      
      bool temp_;      
      UserInfoSrv service_;      
      Expected expected_;      
      AdminExistCheck exists_;      

  };  

  typedef BaseProfileChecker_<BaseProfileAdmin::Expected> BaseProfileChecker;  
  typedef BaseProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> BaseProfileEmptyChecker;  

}

#include "BaseProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_BASEPROFILECHECKER_HPP

