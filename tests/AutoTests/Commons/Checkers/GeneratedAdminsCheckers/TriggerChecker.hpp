
#ifndef __AUTOTESTS_COMMONS_ADMINS_TRIGGERCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_TRIGGERCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/TriggerAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType = TriggerAdmin::Expected>  
  class TriggerChecker_:  
    public AutoTest::Checker  
  {  
    public:    

      typedef ExpectedType Expected;      

      TriggerChecker_(      
        BaseUnit* test,      
        const std::string& trigger,      
        ChannelSrv service,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        trigger_(trigger),      
        service_(service),      
        expected_(expected),      
        exists_(exists)      
        {}      

      virtual ~TriggerChecker_() noexcept {}      

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:    

      BaseUnit* test_;      
      std::string trigger_;      
      ChannelSrv service_;      
      Expected expected_;      
      AdminExistCheck exists_;      

  };  

  typedef TriggerChecker_<TriggerAdmin::Expected> TriggerChecker;  
  typedef TriggerChecker_<std::string> TriggerCheckerSimple;  

}

#include "TriggerChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_TRIGGERCHECKER_HPP

