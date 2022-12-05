
#ifndef __AUTOTESTS_COMMONS_ADMINS_SIMPLECHANNELCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_SIMPLECHANNELCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/SimpleChannelAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  class SimpleChannelChecker:  
    public AutoTest::Checker  
  {  
    public:    
      typedef SimpleChannelAdmin::Expected Expected;      
            
      SimpleChannelChecker(      
        BaseUnit* test,      
        unsigned long id,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        id_(id),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~SimpleChannelChecker() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      unsigned long id_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_SIMPLECHANNELCHECKER_HPP

