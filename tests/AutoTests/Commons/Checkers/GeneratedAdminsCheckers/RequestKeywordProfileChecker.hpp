
#ifndef __AUTOTESTS_COMMONS_ADMINS_REQUESTKEYWORDPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_REQUESTKEYWORDPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/RequestKeywordProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType, CheckType ch = CT_ONE>
  class RequestKeywordProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    
            
      typedef ExpectedType Expected;

      RequestKeywordProfileChecker_(      
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
            
      virtual ~RequestKeywordProfileChecker_() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      std::string requestid_;      
      RequestInfoSrv service_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  

  typedef RequestKeywordProfileChecker_<RequestKeywordProfileAdmin::Expected> RequestKeywordProfileChecker;
  typedef RequestKeywordProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> RequestKeywordProfileEmptyChecker;

}

#include "RequestKeywordProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_REQUESTKEYWORDPROFILECHECKER_HPP

