
#ifndef _AUTOTEST__SPENTBUDGETCHECKER_
#define _AUTOTEST__SPENTBUDGETCHECKER_
 
#include <tests/AutoTests/Commons/Common.hpp>

namespace AutoTest
{
  /*
   * @class SpentBudgetChecker
   * @brief Check CCG budget reaching.
   */
  class SpentBudgetChecker : public AutoTest::Checker
  {
  public:

    /**
     * @brief Constructor
     *
     * @param test.
     * @param check request.
     * @param CCG ID.
     * @param expected CC ID.
     */
    SpentBudgetChecker(
      BaseUnit* test,
      const NSLookupRequest& request,
      unsigned long ccgid,
      unsigned long ccid,
      const Generics::Time& deadline = Generics::Time::ZERO);

    /**
     * @brief Destructor
     */
    ~SpentBudgetChecker() noexcept;

    /**
     * @brief check
     */
    bool check(bool throw_error = true)
      /*throw(AutoTest::CheckFailed, eh::Exception)*/;

  private:
    BaseUnit* test_;          // test
    NSLookupRequest request_; // check 
    unsigned long ccgid_;     // CCG ID
    unsigned long  ccid_;        // expected CC ID
    Generics::Time deadline_;
  };
}; // namespace AutoTest

#endif //_AUTOTEST__SPENTBUDGETCHECKER_
