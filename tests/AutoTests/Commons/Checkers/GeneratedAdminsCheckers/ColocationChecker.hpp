
#ifndef __AUTOTESTS_COMMONS_ADMINS_COLOCATIONCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_COLOCATIONCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/ColocationAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  class ColocationChecker:
    public AutoTest::Checker
  {
    public:
      typedef ColocationAdmin::Expected Expected;

      ColocationChecker(
        BaseUnit* test,     
        unsigned long colo,
        const Expected& expected,
        AdminExistCheck exists = AEC_EXISTS) :
        test_(test),
        colo_(colo),
        expected_(expected),
        exists_(exists)
        {}

      virtual ~ColocationChecker() noexcept {}

      bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;

    private:

      BaseUnit* test_;
      unsigned long colo_;
      Expected expected_;     
      AdminExistCheck exists_;

  };
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_COLOCATIONCHECKER_HPP

