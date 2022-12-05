
#ifndef __AUTOTESTS_COMMONS_ADMINS_CHANNELSEARCHCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CHANNELSEARCHCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/ChannelSearchAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  template<typename ExpectedType = ChannelSearchAdmin::Expected>  
  class ChannelSearchChecker_:  
    public AutoTest::Checker  
  {  
    public:    

      typedef ExpectedType Expected;      

      ChannelSearchChecker_(      
        BaseUnit* test,      
        const std::string& phrase,      
        ChannelSrv service,      
        const Expected& expected,      
        AdminExistCheck exists = AEC_EXISTS) :      
        test_(test),      
        phrase_(phrase),      
        service_(service),      
        expected_(expected),      
        exists_(exists)      
        {}      

      virtual ~ChannelSearchChecker_() noexcept {}      

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:    

      BaseUnit* test_;      
      std::string phrase_;      
      ChannelSrv service_;      
      Expected expected_;      
      AdminExistCheck exists_;      

  };  

  typedef ChannelSearchChecker_<ChannelSearchAdmin::Expected> ChannelSearchChecker;  
  typedef ChannelSearchChecker_<std::string> ChannelSearchCheckerSimple;  

}

#include "ChannelSearchChecker.tpp"

#endif  // __AUTOTESTS_COMMONS_ADMINS_CHANNELSEARCHCHECKER_HPP

