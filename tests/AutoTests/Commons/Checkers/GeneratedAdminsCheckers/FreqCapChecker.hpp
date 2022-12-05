
#ifndef __AUTOTESTS_COMMONS_ADMINS_FREQCAPCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_FREQCAPCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/FreqCapAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType = FreqCapAdmin::Expected>  
  class FreqCapChecker_:  
    public AutoTest::Checker  
  {  
    public:    

      typedef ExpectedType Expected;      

      FreqCapChecker_(      
        BaseUnit* test,      
        unsigned long freqcap,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        freqcap_(freqcap),      
        expected_(expected),      
        exists_(exists)      
        {}      

      virtual ~FreqCapChecker_() noexcept {}      

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:    

      BaseUnit* test_;      
      unsigned long freqcap_;      
      Expected expected_;      
      AdminExistCheck exists_;      

  };  

  typedef FreqCapChecker_<FreqCapAdmin::Expected> FreqCapChecker;  
  typedef FreqCapChecker_<std::string> FreqCapCheckerSimple;  

}

#include "FreqCapChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_FREQCAPCHECKER_HPP

