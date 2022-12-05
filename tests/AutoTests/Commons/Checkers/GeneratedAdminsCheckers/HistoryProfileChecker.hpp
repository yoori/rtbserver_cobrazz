
#ifndef __AUTOTESTS_COMMONS_ADMINS_HISTORYPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_HISTORYPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/HistoryProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType = HistoryProfileAdmin::Expected, CheckType ch = CT_ONE>   
  class HistoryProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    

      typedef ExpectedType Expected;      

      HistoryProfileChecker_(      
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

      virtual ~HistoryProfileChecker_() noexcept {}      

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:    

      BaseUnit* test_;      
      std::string uid_;      
      bool temp_;      
      UserInfoSrv service_;      
      Expected expected_;      
      AdminExistCheck exists_;      

  };  

  typedef HistoryProfileChecker_<HistoryProfileAdmin::Expected> HistoryProfileChecker;  
  typedef HistoryProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> HistoryProfileEmptyChecker;

}

#include "HistoryProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_HISTORYPROFILECHECKER_HPP

