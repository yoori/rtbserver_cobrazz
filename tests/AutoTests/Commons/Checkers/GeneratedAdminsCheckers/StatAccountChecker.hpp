
#ifndef __AUTOTESTS_COMMONS_ADMINS_STATACCOUNTCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_STATACCOUNTCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/StatAccountAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType = StatAccountAdmin::Expected>  
  class StatAccountChecker_:  
    public AutoTest::Checker  
  {  
    public:    

      typedef ExpectedType Expected;      

      StatAccountChecker_(      
        BaseUnit* test,      
        unsigned long id,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        id_(id),      
        expected_(expected),      
        exists_(exists)      
        {}      

      virtual ~StatAccountChecker_() noexcept {}      

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:    

      BaseUnit* test_;      
      unsigned long id_;      
      Expected expected_;      
      AdminExistCheck exists_;      

  };  

  typedef StatAccountChecker_<StatAccountAdmin::Expected> StatAccountChecker;  
  typedef StatAccountChecker_<std::string> StatAccountCheckerSimple;  

}

#include "StatAccountChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_STATACCOUNTCHECKER_HPP

