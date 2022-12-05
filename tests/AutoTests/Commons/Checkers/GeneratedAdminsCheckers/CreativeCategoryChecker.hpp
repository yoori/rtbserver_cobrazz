
#ifndef __AUTOTESTS_COMMONS_ADMINS_CREATIVECATEGORYCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CREATIVECATEGORYCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/CreativeCategoryAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType = CreativeCategoryAdmin::Expected>  
  class CreativeCategoryChecker_:  
    public AutoTest::Checker  
  {  
    public:    

      typedef ExpectedType Expected;      

      CreativeCategoryChecker_(      
        BaseUnit* test,      
        unsigned long category,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        category_(category),      
        expected_(expected),      
        exists_(exists)      
        {}      

      virtual ~CreativeCategoryChecker_() noexcept {}      

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:    

      BaseUnit* test_;      
      unsigned long category_;      
      Expected expected_;      
      AdminExistCheck exists_;      

  };  

  typedef CreativeCategoryChecker_<CreativeCategoryAdmin::Expected> CreativeCategoryChecker;  
  typedef CreativeCategoryChecker_<std::string> CreativeCategoryCheckerSimple;  

}

#include "CreativeCategoryChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_CREATIVECATEGORYCHECKER_HPP

