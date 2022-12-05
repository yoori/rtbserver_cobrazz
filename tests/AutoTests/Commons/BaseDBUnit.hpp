#ifndef __AUTOTESTS_COMMONS_BASEDBUNIT_HPP
#define __AUTOTESTS_COMMONS_BASEDBUNIT_HPP

#include <tuple>
#include <Commons/PathManip.hpp>
#include <tests/AutoTests/Commons/BaseUnit.hpp>
#include <tests/AutoTests/Commons/ORM/ORM.hpp>

/**
 * @class BaseDBUnit
 * @brief Test class with db object
 */
class BaseDBUnit:
  public BaseUnit
{
public:
  /**
   * @brief Constructor.
   * @param test statistics.
   * @param test name.
   * @param test config.
   */
  BaseDBUnit(
    UnitStat& stat_var,
    const char* task_name,
    XsdParams params_var);

  /**
   * @brief Destructor.
   */
  virtual ~BaseDBUnit() noexcept;

  /**
   * @brief Test setup.
   * Use to prepare test data, check environment, etc.
   */  
  virtual void set_up();

  /**
   * @brief Test tear down.
   * Use to restore DB entities, remove statistics, etc.
   */  
  virtual void tear_down() = 0;

  /**
   * @brief Test pre condition.
   * Use to check initial state.
   */  
  virtual void pre_condition();

  /**
   * @brief Run test cases.
   */  
  virtual bool run() = 0;

  /**
   * @brief Test pre condition.
   * Use to check result state.
   */  
  virtual void post_condition();

  /**
   * @brief Run test.
   */  
  bool run_test();

  /**
   * @brief Create DB entity.
   * @param entity ID.
   * @return restorer entity wrapper
   */  
  template <typename Entity>
  AutoTest::ORM::ORMRestorer<Entity>*
  create(unsigned long id) /*throw(eh::Exception)*/;

  /**
   * @brief Create DB entity.
   * @param entity.
   * @return restorer entity wrapper
   */  
  template <typename Entity>
  AutoTest::ORM::ORMRestorer<Entity>*
  create(const Entity& e) /*throw(eh::Exception)*/;

  /**
   * @brief Create DB entity.
   * @return restorer entity wrapper
   */  
  template <typename Entity>
  AutoTest::ORM::ORMRestorer<Entity>*
  create() /*throw(eh::Exception)*/;

protected:
  DECLARE_EXCEPTION(DataBaseError, eh::DescriptiveException);

  AutoTest::DBC::Conn pq_conn_; // Postgres connection
  AutoTest::ORM::Restorers restorers_; // ORM restorers storage

private:
  
  void safe_tear_down();

  AutoTest::DBC::Conn&
  get_conn(
    AutoTest::ORM::postgres_connection conn_type);
};

#include "BaseDBUnit.ipp"

#endif  // __AUTOTESTS_COMMONS_BASEDBUNIT_HPP
