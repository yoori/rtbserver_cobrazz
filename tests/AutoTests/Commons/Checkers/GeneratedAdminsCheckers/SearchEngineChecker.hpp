
#ifndef __AUTOTESTS_COMMONS_ADMINS_SEARCHENGINECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_SEARCHENGINECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/SearchEngineAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType = SearchEngineAdmin::Expected>  
  class SearchEngineChecker_:  
    public AutoTest::Checker  
  {  
    public:    

      typedef ExpectedType Expected;      

      SearchEngineChecker_(      
        BaseUnit* test,      
        unsigned long id,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        id_(id),      
        expected_(expected),      
        exists_(exists)      
        {}      

      virtual ~SearchEngineChecker_() noexcept {}      

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:    

      BaseUnit* test_;      
      unsigned long id_;      
      Expected expected_;      
      AdminExistCheck exists_;      

  };  

  typedef SearchEngineChecker_<SearchEngineAdmin::Expected> SearchEngineChecker;  
  typedef SearchEngineChecker_<std::string> SearchEngineCheckerSimple;  

}

#include "SearchEngineChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_SEARCHENGINECHECKER_HPP

