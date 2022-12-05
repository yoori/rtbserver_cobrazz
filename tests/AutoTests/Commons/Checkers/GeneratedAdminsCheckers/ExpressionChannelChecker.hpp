
#ifndef __AUTOTESTS_COMMONS_ADMINS_EXPRESSIONCHANNELCHECKER_HPP
#define __AUTOTESTS_COMMONS_ADMINS_EXPRESSIONCHANNELCHECKER_HPP

#include <tests/AutoTests/Commons/Admins/ExpressionChannelAdmin.hpp>
#include <tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>
#include <tests/AutoTests/Commons/Checkers/Checker.hpp>

namespace AutoTest
{
  class ExpressionChannelChecker:
    public AutoTest::Checker
  {
  public:    
    typedef ExpressionChannelAdmin::Expected Expected;
            
    ExpressionChannelChecker(
      BaseUnit* test,
      unsigned long channel_id,
      const Expected& expected,
      AdminExistCheck exists = AEC_EXISTS);
            
    virtual ~ExpressionChannelChecker() noexcept;

    bool check(bool throw_error = true) /*throw(CheckFailed, eh::Exception)*/;
    
  private:
    BaseUnit* test_;
    unsigned long channel_id_;
    Expected expected_; 
    AdminExistCheck exists_;

    static const Expected ANY_EXPECTED_;
  };  
}
#endif  // __AUTOTESTS_COMMONS_ADMINS_EXPRESSIONCHANNELCHECKER_HPP

