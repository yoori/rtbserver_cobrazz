
#ifndef __AUTOTESTS_COMMONS_ADMINS_CCGKEYWORDCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CCGKEYWORDCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/CCGKeywordAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType = CCGKeywordAdmin::Expected>  
  class CCGKeywordChecker_:  
    public AutoTest::Checker  
  {  
    public:    

      typedef ExpectedType Expected;      

      CCGKeywordChecker_(      
        BaseUnit* test,      
        unsigned long ccg_keyword_id,      
        ChannelSrv service,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        ccg_keyword_id_(ccg_keyword_id),      
        service_(service),      
        expected_(expected),      
        exists_(exists)      
        {}      

      virtual ~CCGKeywordChecker_() noexcept {}      

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:    

      BaseUnit* test_;      
      unsigned long ccg_keyword_id_;      
      ChannelSrv service_;      
      Expected expected_;      
      AdminExistCheck exists_;      

  };  

  typedef CCGKeywordChecker_<CCGKeywordAdmin::Expected> CCGKeywordChecker;  
  typedef CCGKeywordChecker_<std::string> CCGKeywordCheckerSimple;  

}

#include "CCGKeywordChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_CCGKEYWORDCHECKER_HPP

