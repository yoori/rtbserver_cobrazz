
#ifndef _AUTOTEST_COMMONS_CHECKER_ADMINSCHECKER_HPP
#define _AUTOTEST_COMMONS_CHECKER_ADMINSCHECKER_HPP

#include "Checker.hpp"
#include <tests/AutoTests/Commons/Admins/Admins.hpp>
#include <tests/AutoTests/Commons/Admins/AdminsContainer.hpp>

namespace AutoTest
{
  enum AdminExistCheck
  {
    AEC_NOT_EXISTS = 0, // Check expected exists
    AEC_EXISTS = 1      // Check expected not exists
  };

  /**
   * @class AdminChecker
   * @brief Check admin expected
   *
   * Used for checked admin result.
   */
  template<class Admin, class Expected = typename Admin::Expected>
  class AdminChecker : public Checker
  {
  public:
    /**
     * @brief Constructor.
     *
     * @param admin.
     * @param expected.
     * @param exists flag.
     */
    AdminChecker(
      const Admin& admin,
      const Expected& expected,
      AdminExistCheck exists = AEC_EXISTS) :
      admin_(admin),
      expected_(expected),
      exists_(exists)
    { }

    /**
     * @brief Destructor.
     */
    virtual ~AdminChecker() noexcept
    { }

    /**
     * @brief Check.
     * @param throw on error flag.
     */
    bool check(bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/
    {
      if (!admin_.check(expected_, exists_ == AEC_EXISTS))
      {
        if (throw_error)
        {
          Stream::Error error;
          if(exists_)
          {
            error << "can't get expected (";
          }
          else
          {
            error << "got unexpected (";
          }
          admin_.print_idname(error);
          error << ')' << std::endl;
          admin_.dump_expects(error);
          error << std::endl << "got:" <<
            std::endl << std::endl;
          {
            admin_.dumpout(error);
          }
          throw CheckFailed(error);
        }
        return false;
      }
      return true;
    }

  private:
    Admin admin_;
    Expected expected_;
    AdminExistCheck exists_;
  };

  /**
   * @brief helper function for AdminChecker.
   * @param admin.
   * @param expected.
   * @param exists.
   * @return AdminChecker
   */
  template<typename Admin, typename Expected>
  AdminChecker<Admin, Expected>
  admin_checker(
    const Admin& admin,
    const Expected& expected,
    AdminExistCheck exists = AEC_EXISTS) /*throw(CheckFailed, eh::Exception)*/
  {
    return AdminChecker<Admin, Expected>(
      admin,
      expected,
      exists);
  }

  /**
   * @brief helper function for AdminChecker.
   * @param admins array.
   * @param expected.
   * @param exists.
   * @return AdminChecker
   */
  template<typename Admin, CheckType ch, typename Expected>
  AdminChecker<AdminsArray<Admin, ch>, Expected>
  admin_checker(
    const AdminsArray<Admin, ch>& admin,
    const Expected& expected,
    AdminExistCheck exists = AEC_EXISTS) /*throw(CheckFailed, eh::Exception)*/
  {
    if (admin.empty())
    {
      throw CheckFailed("Admin container is empty!");
    }
    return AdminChecker<AdminsArray<Admin, ch>, Expected>(
      admin,
      expected,
      exists);
  }
}

#endif  // _AUTOTEST_COMMONS_CHECKER_ADMINSCHECKER_HPP
