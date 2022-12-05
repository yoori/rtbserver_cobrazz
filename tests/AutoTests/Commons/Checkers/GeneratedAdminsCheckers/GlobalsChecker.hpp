
#ifndef __AUTOTESTS_COMMONS_ADMINS_GLOBALSCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_GLOBALSCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/GlobalsAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType = GlobalsAdmin::Expected>  
  class GlobalsChecker_:  
    public AutoTest::Checker  
  {  
    public:    

      typedef ExpectedType Expected;      

      GlobalsChecker_(      
        BaseUnit* test,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        expected_(expected),      
        exists_(exists)      
        {}      

      virtual ~GlobalsChecker_() noexcept {}      

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:    

      BaseUnit* test_;      
      Expected expected_;      
      AdminExistCheck exists_;      

  };  

  typedef GlobalsChecker_<GlobalsAdmin::Expected> GlobalsChecker;  
  typedef GlobalsChecker_<std::string> GlobalsCheckerSimple;  

}

#include "GlobalsChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_GLOBALSCHECKER_HPP

