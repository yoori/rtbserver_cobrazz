
#ifndef __AUTOTESTS_COMMONS_ADMINS_CHANNELCATEGORYCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_CHANNELCATEGORYCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/ChannelCategoryAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  class ChannelCategoryChecker:
    public AutoTest::Checker
  {
    public:
      typedef ChannelCategoryAdmin::Expected Expected;
    
      ChannelCategoryChecker(
        BaseUnit* test,
        unsigned long category,    
        const Expected& expected,
        AdminExistCheck exists = AEC_EXISTS) :
        test_(test),
        category_(category),
        expected_(expected),
        exists_(exists)
        {}

      virtual ~ChannelCategoryChecker() noexcept {}

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;      

    private:
    
      BaseUnit* test_;
      unsigned long category_;
      Expected expected_;
      AdminExistCheck exists_;

  };
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_CHANNELCATEGORYCHECKER_HPP

