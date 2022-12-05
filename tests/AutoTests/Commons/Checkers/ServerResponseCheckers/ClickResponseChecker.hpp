#ifndef __AUTOTESTS_COMMONS_CHECKERS_CLICKRESPONSECHECKER_HPP
#define __AUTOTESTS_COMMONS_CHECKERS_CLICKRESPONSECHECKER_HPP

#include <tests/AutoTests/Commons/Checkers/Checker.hpp>
#include <tests/AutoTests/Commons/AdClient.hpp>
#include <String/RegEx.hpp>

namespace AutoTest
{
  /**
   * @class ClickResponseChecker
   * @brief Check click frontend response.
   */
  class ClickResponseChecker: public AutoTest::Checker
  {
  public:

    static const String::RegEx EMPTY_PUB_PRECLICK;
    static const String::RegEx EMPTY_CRCLICK;
    
    /**
     * @brief Constructor.
     *
     * @param client (user).
     * @param expected PUBPRECLICK token.
     * @param expected CRCLICK token.
     */
    ClickResponseChecker(
      AdClient& client,
      const std::string& exp_pub_preclick,
      const std::string& exp_crclick);

    /**
     * @brief Constructor.
     *
     * @param client (user).
     * @param expected PUBPRECLICK regex.
     * @param expected CRCLICK regex.
     */
    ClickResponseChecker(
      AdClient& client,
      const String::RegEx& exp_pub_preclick_regex,
      const String::RegEx& exp_crclick_regex);

    /**
     * @brief Destructor.
     */
    virtual ~ClickResponseChecker() noexcept;

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool
    check(
      bool throw_error = true)
      /*throw(eh::Exception)*/;

  private:
    AdClient client_; // client(user)
    String::RegEx exp_pub_preclick_regex_; // PUBPRECLICK regex
    String::RegEx exp_crclick_regex_; // CRCLICK regex
  };
} //namespace AutoTest

#endif  // __AUTOTESTS_COMMONS_CHECKERS_CLICKRESPONSECHECKER_HPP
