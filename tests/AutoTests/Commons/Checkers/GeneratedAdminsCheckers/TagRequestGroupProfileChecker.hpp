
#ifndef __AUTOTESTS_COMMONS_ADMINS_TAGREQUESTGROUPPROFILECHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_TAGREQUESTGROUPPROFILECHECKER_HPP

#include <tests/AutoTests/Commons/Admins/TagRequestGroupProfileAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType, CheckType ch = CT_ONE>
  class TagRequestGroupProfileChecker_:  
    public AutoTest::Checker  
  {  
    public:    
            
      typedef ExpectedType Expected;

      TagRequestGroupProfileChecker_(      
        BaseUnit* test,      
        const std::string& uuid,      
        RequestInfoSrv service,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        uuid_(uuid),      
        service_(service),      
        expected_(expected),      
        exists_(exists)      
        {}      
            
      virtual ~TagRequestGroupProfileChecker_() noexcept {}      
            
      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      
            
    private:    
            
      BaseUnit* test_;      
      std::string uuid_;      
      RequestInfoSrv service_;      
      Expected expected_;      
      AdminExistCheck exists_;      
            
  };  

  typedef TagRequestGroupProfileChecker_<TagRequestGroupProfileAdmin::Expected> TagRequestGroupProfileChecker;
  typedef TagRequestGroupProfileChecker_<std::string, CT_ONE_NOT_EXPECTED> TagRequestGroupProfileEmptyChecker;

}

#include "TagRequestGroupProfileChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_TAGREQUESTGROUPPROFILECHECKER_HPP

