#ifndef __AUTOTESTS_COMMONS_CHECKER_HPP
#define __AUTOTESTS_COMMONS_CHECKER_HPP

#include <eh/Exception.hpp>

namespace AutoTest
{
  DECLARE_EXCEPTION(CheckFailed, eh::DescriptiveException);
  /**
   * @class Checker
   * @brief Checker interface.
   *
   * Abstract class. Interface for classes, that check some AdServer entities.
   */
  class Checker
  {
  public:
    virtual bool
    check(bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/ = 0;

    virtual ~Checker() noexcept 
    {}
  };
} //namespace AutoTest

#endif  // __AUTOTESTS_COMMONS_CHECKER_HPP
