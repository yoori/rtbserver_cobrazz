#ifndef __AUTOTESTS_COMMONS_CHECKERS_REDIRECTCHECKER_HPP
#define __AUTOTESTS_COMMONS_CHECKERS_REDIRECTCHECKER_HPP

#include <tests/AutoTests/Commons/Checkers/Checker.hpp>
#include <tests/AutoTests/Commons/AdClient.hpp>
#include <String/RegEx.hpp>

namespace AutoTest
{
  /**
   * @class RedirectChecker
   * @brief Check client redirection.
   */
  class RedirectChecker: public AutoTest::Checker
  {
  public:
    /**
     * @brief Constructor.
     *
     * @param client (user).
     * @param expected location.
     */
    RedirectChecker(
      AdClient& client,
      const std::string& location);

    /**
     * @brief Constructor.
     *
     * @param client (user).
     * @param regex to check location.
     */
    RedirectChecker(
      AdClient& client,
      const String::RegEx& regex);

    /**
     * @brief Destructor.
     */
    virtual ~RedirectChecker() noexcept;

    /**
     * @brief Get location.
     */
    const std::string&
    location() const;

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
    String::RegEx regex_; // regex to check location
    std::string location_; // location
  };
} //namespace AutoTest

// inlines
namespace AutoTest
{
  inline
  const std::string&
  RedirectChecker::location() const
  {
    return location_;
  }
}

#endif /*AUTOTESTS_COMMONS_CHECKERS_REDIRECTCHECKER_HPP*/
