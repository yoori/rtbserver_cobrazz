
#ifndef __AUTOTESTS_COMMONS_ADMINS_ACCOUNTCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_ACCOUNTCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/AccountAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType = AccountAdmin::Expected>  
  class AccountChecker_:  
    public AutoTest::Checker  
  {  
    public:    

      typedef ExpectedType Expected;      

      AccountChecker_(      
        BaseUnit* test,      
        unsigned long account,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        account_(account),      
        expected_(expected),      
        exists_(exists)      
        {}      

      virtual ~AccountChecker_() noexcept {}      

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:    

      BaseUnit* test_;      
      unsigned long account_;      
      Expected expected_;      
      AdminExistCheck exists_;      

  };  

  typedef AccountChecker_<AccountAdmin::Expected> AccountChecker;  
  typedef AccountChecker_<std::string> AccountCheckerSimple;  

}

#include "AccountChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_ACCOUNTCHECKER_HPP

