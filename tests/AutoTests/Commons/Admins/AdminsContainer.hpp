#ifndef __AUTOTEST_COMMONS_ADMINS_ADMINSCONTAINER_HPP
#define __AUTOTEST_COMMONS_ADMINS_ADMINSCONTAINER_HPP

#include <tests/AutoTests/Commons/Logger.hpp>
#include <tests/AutoTests/Commons/BaseUnit.hpp>

namespace AutoTest
{
  enum CheckType
  {
    CT_ONE,  // only one instance of service must contain entity
    CT_ANY,  // any of instances of service must contain entity
    CT_ALL,  // all instances of service must contain entity
    CT_ONE_NOT_EXPECTED // only one instances of service mustn't contain entity
  };

  /**
   * @class AdminsArray
   * @brief Represents array of admin commands
   *
   * Adserver can have more than one service of each type,
   * so some DB entities can be loaded to one or several of them.
   * This class provides functionality to check expected entity
   * is loaded to one/any/all services of indicated type.
   * Compatible with all adins checkers.
   */
  template<typename AdminType, CheckType ch = CT_ANY>
  class AdminsArray
  {
  protected:
    typedef std::vector<AdminType> ArrayType;
    typedef GlobalConfig::ServiceList ServiceList;
    typedef ServiceList::const_iterator Service_iterator;
    ArrayType admins_;

  public:

    /**
     * @brief Default constructor
     */
    AdminsArray () {};

    /**
     * @brief Copy constructor
     *
     * @param from copied object
     */
    AdminsArray(const AdminsArray& from);

    /**
     * @brief Base initializer
     *
     * Initializes AdminsArray with admin commands.
     * @param test pointer to test object
     * @param cluster defines services of which cluster will be checked
     * @param service defines type of checked services.
     */
    void
    initialize(
      BaseUnit* test,
      ClusterTypeEnum cluster,
      ServiceTypeEnum service);

    /**
     * @brief Initializer with one additional arg
     *
     * Like the previous one, but get one more argument
     * that will be passed to admin constructor
     */
    template<typename Arg>
    void
    initialize(
      BaseUnit* test,
      ClusterTypeEnum cluster,
      ServiceTypeEnum service,
      Arg arg);

    /**
     * @brief Initializer with two additional args
     *
     * Like the Base initializer, but get two more arguments
     * that will be passed to admin constructor
     */
    template<typename Arg1, typename Arg2>
    void
    initialize(
      BaseUnit* test,
      ClusterTypeEnum cluster,
      ServiceTypeEnum service,
      Arg1 arg1,
      Arg2 arg2);

    /**
     * @brief Initializer with three additional args
     *
     * Like the Base initializer, but get three more arguments
     * that will be passed to admin constructor
     */
    template<typename Arg1, typename Arg2, typename Arg3>
    void
    initialize(
      BaseUnit* test,
      ClusterTypeEnum cluster,
      ServiceTypeEnum service,
      Arg1 arg1,
      Arg2 arg2,
      Arg3 arg3);

    /**
     * @brief Add admin to array
     *
     * This func adds admin of defined type to admins array
     * @param admin admin command that will be added
     */
    void add(const AdminType& admin);

    /**
     * @brief Delete admin from array
     *
     * This func deletes the last admin from admins array
     */
    void del();

    /**
     * @brief Clear admins array
     *
     * Makes admins array empty.
     */
    void clear();

    /**
     * @brief Random acces operators
     *
     * Easy way to acces admin with indicated index
     */
    AdminType& operator [] (int i);
    const AdminType& operator [] (int i) const;

    /**
     * @brief Overload of 'check' BaseAdmin method
     */
    template<typename Expects>
    bool check(
      const Expects& expects,
      bool exists = true);

    /**
     * @brief Fetch values from output
     *
     * Fetches values from admin cmd output for every admin of array
     */
    bool fetch();

    /**
     * @brief Execute every admin cmd
     *
     * Calls exec method for every admin of array
     */
    void exec();

    /**
     * @brief Logs admins output
     *
     * Executes every admin cmd of array and logs output.
     */
    void log(Logger& logger,
             unsigned long log_level = Logging::Logger::DEBUG);

    /**
     * @Test whether container is empty.
     *
     * @return true if the container size is 0, false otherwise.
     */
    bool empty() const;

    /**
     * @brief Print command line of every admin of array
     */
    std::ostream& print_idname (std::ostream& out);

    /**
     * @brief Print expected values
     */
    std::ostream& dump_expects (std::ostream& out);

    /**
     * @brief Print comparison of expected and fetched values
     */
    std::ostream& dumpout (std::ostream& out);
  }; //class AdminsArray

}//namespace AutoTest

#include "AdminsContainer.tpp"

#endif //__AUTOTEST_COMMONS_ADMINS_ADMINSCONTAINER_HPP
