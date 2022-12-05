/* $Id$
* @file CompositeCheckers.hpp
* Checkers composition:
*   WaitChecker, help fun : wait_checker
*   AndChecker, help fun : and_checker(1, 2[, 3[, 4]])
*   OrChecker, help fun : or_checker(1, 2)
*   ThrowChecker, help fun : throw_checker
*/

#ifndef AUTOTESTS_COMMONS_COMPOSITECHECKERS_HPP
#define AUTOTESTS_COMMONS_COMPOSITECHECKERS_HPP

#include "Checker.hpp"
#include <tests/AutoTests/Commons/Shutdown.hpp>
#include <tests/AutoTests/Commons/Logger.hpp>

namespace AutoTest
{
  namespace Internals
  {
    /**
     * @class CheckerHolder
     * @brief Checker holder interface
     */
    class CheckerHolder:
      public Checker,
      public ReferenceCounting::AtomicCopyImpl
    {
    public:

      /**
       * @brief Clone held checker
       * @return copy of held checker
       */
      virtual
      ReferenceCounting::SmartPtr<CheckerHolder>
      clone() const noexcept = 0;

    protected:

      /**
       * @brief Destructor
       */
      virtual
      ~CheckerHolder() noexcept;
    };

    typedef ReferenceCounting::SmartPtr<CheckerHolder>
      CheckerHolder_var;
    
    /**
     * @class CheckerHolderImpl
     * @brief Checker holder implementation
     */
    template<typename CheckerType>
    struct CheckerHolderImpl: public CheckerHolder
    {
    public:

      /**
       * @brief Constructor
       * @param checker
       */
      CheckerHolderImpl(const CheckerType& init) noexcept;

      /**
       * @brief Clone held checker
       * @return copy of held checker
       */     
      virtual CheckerHolder_var
      clone() const noexcept;

      /**
       * @brief test checker
       * @param throw on error flag
       * @return true - OK, false - check FAIL
       */      
      bool
      check(
        bool throw_error = true)
        /*throw(CheckFailed, eh::Exception)*/;

    protected:
      /**
       * @brief Destructor
       */
      virtual
      ~CheckerHolderImpl() noexcept;

    private:
      CheckerType checker_;
    };

    /**
     * @class SubCheckersHolder
     * @brief Checkers container
     */
    class SubCheckersHolder
    {
    public:

      /**
       * @brief Default constructor
       */
      SubCheckersHolder() noexcept;

      /**
       * @brief Constructor
       * @param sub checker
       */
      SubCheckersHolder(const SubCheckersHolder& init)
        noexcept;

      /**
       * @brief Destructor
       */
      virtual ~SubCheckersHolder() noexcept;

      /**
       * @brief add sub checkers
       * @param first sub checker
       * @param other sub checkers
       */
      template<typename SubCheckerType, typename... SubCheckers>
      void
      add_sub_checker(
        const SubCheckerType& sub_checker,
        SubCheckers... sub_checkers)
        noexcept;

      /**
       * @brief add sub checker
       * @param checker
       */
      template<typename SubCheckerType>
      void
      add_sub_checker(const SubCheckerType& sub_checker)
        noexcept;

    protected:
      typedef std::list<CheckerHolder_var> CheckerHolderList;
      CheckerHolderList sub_checkers_;  // checkers list
    };

    // To release cyclic includes (BaseUnit <-> CompositeCheckers)
    int get_unit_timeout();
  }

  /**
   * @class WaitChecker
   * @brief Event wait checker
   */
  template<typename SubCheckerType>
  class WaitChecker: public Checker
  {
  public:

    /**
     * @brief Constructor
     * @param sub checker
     * @param wait timeout
     * @param interval between adjacent checks
     */
    WaitChecker(
      const SubCheckerType& sub_checker,
      unsigned long wait_time = 0,
      unsigned long sleep_time =
        AutoTest::DEFAULT_SLEEP_TIME);

    /**
     * @brief Destructor
     */
    virtual ~WaitChecker() noexcept;

    /**
     * @brief test checker
     * @param throw on error flag
     * @return true - OK, false - check FAIL
     */  
    bool check(bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/;

  private:
    SubCheckerType sub_checker_;
    unsigned long wait_time_;
    unsigned long sleep_time_;
  };

  /**
   * @class NotChecker
   * Negation predicate implementation
   */
  template<typename SubCheckerType>
  class NotChecker: public Checker
  {
  public:
    /**
     * @brief Constructor
     * @param sub checker
     */
    NotChecker(const SubCheckerType& sub_checker);

    /**
     * @brief Destructor
     */
    virtual ~NotChecker() noexcept;

    /**
     * @brief test checker
     * @param throw on error flag
     * @return true - OK, false - check FAIL
     */  
    bool check(bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/;

  private:
    SubCheckerType sub_checker_;
  };

  /**
   * @class OrChecker
   * @brief Disjunction predicate implementation
   */
  class OrChecker:
    protected Internals::SubCheckersHolder,
    public virtual Checker
  {
  public:

    /**
     * @class ICounter
     * @brief Counter for events
     * Usecase: count sub checker passed checks,
     *  when used in the loop.
     */
    class ICounter
    {
    public:

      /**
       * @brief Increment passed events for sub checker by index
       * @param sub checker index
       */
      virtual
      void incr(size_t idx) = 0;
    };

    /**
     * @brief Constructor
     * @param counter object
     */
    explicit OrChecker(ICounter* counter = 0) noexcept;

    /**
     * @brief Constructor
     * @param sub checkers
     */
    template<typename... SubCheckers>
    OrChecker(SubCheckers... sub_checkers);

    /**
     * @brief Destructor
     */
    virtual ~OrChecker() noexcept;

    /**
     * @brief add another sub checkers
     * @param sub checkers
     * @return checker
     */
    template<typename... SubCheckers>
    OrChecker&
    or_if(SubCheckers... sub_checkers);

    /**
     * @brief test checker
     * @param throw on error flag
     * @return true - OK, false - check FAIL
     */  
    bool check(bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/;

  private:
    ICounter* counter_;
  };

  class CountChecker :
    public AutoTest::Checker,
    public AutoTest::OrChecker::ICounter
  {

  /**
   * @class CountChecker
   * @brief Simple aggregate checker for OrChecker
   */
  public:

    /**
     * @brief Constructor
     * @param number of events
     * @param sample size
     */
    CountChecker(
      size_t events_size,
      size_t sample_size);

    /**
     * @brief Destructor
     */
    virtual
    ~CountChecker() noexcept;

    /**
     * @brief ICounter.incr implemantation
     */
    virtual
    void incr(size_t index);

    /**
     * @brief test checker
     * @param throw on error flag
     * @return true - OK, false - check FAIL
     */  
    virtual
    bool
    check(bool throw_error = true)
      /*throw(AutoTest::CheckFailed, eh::Exception)*/;

    /**
     * @brief Get counters
     * @return events array
     */  
    const std::vector<unsigned long>& counts() const;

  private:
    std::vector<size_t> counts_;
    size_t sample_size_;
  };

  
  /**
   * @class AndChecker
   * @brief Conjunction predicate implementation
   */
  class AndChecker:
    protected Internals::SubCheckersHolder,
    public virtual Checker
  {
  public:

    /**
     * @brief Constructor
     * @param sub checkers
     */
    template<typename... SubCheckers>
    AndChecker(
      SubCheckers... sub_checkers)
      noexcept;

    /**
     * @brief Destructor
     */
    virtual ~AndChecker() noexcept;
   
    /**
     * @brief add another sub checkers
     * @param sub checkers
     * @return checker
     */
    template<typename... SubCheckers>
    AndChecker&
    and_if(SubCheckers... sub_checkers) noexcept;

    /**
     * @brief test checker
     * @param throw on error flag
     * @return true - OK, false - check FAIL
     */  
    bool check(bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/;
  };

  /**
   * @class ThrowChecker
   *  wrapper around other checker
   *  allow to override throw_error parameter value
   *  usable for cases when required interrupt wait_checker
   */
  template<typename SubCheckerType>
  class ThrowChecker: public Checker
  {
  public:

    /**
     * @brief Constructor
     * @param sub checker
     * @param throw on error flag.
     */
    ThrowChecker(
      const SubCheckerType& sub_checker,
      bool throw_error);

    /**
     * @brief Destructor
     */
    virtual ~ThrowChecker() noexcept;

    /**
     * @brief test checker
     * @param throw on error flag
     * @return true - OK, false - check FAIL
     */  
    bool check(bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/;

  private:
    SubCheckerType sub_checker_;
    bool throw_error_;
  };


  /**
   * @class FailChecker
   * Allow to check slave when master fail
   */
  template<typename MasterChecker, typename SlaveChecker>
  class FailChecker: public Checker
  {
  public:

    /**
     * @brief Constructor
     * @param master checker
     * @param slave checker.
     */
    FailChecker(
      const MasterChecker& master,
      const SlaveChecker& slave);

    /**
     * @brief Destructor
     */
    virtual ~FailChecker() noexcept;

    /**
     * @brief test checker
     * @param throw on error flag
     * @return true - OK, false - check FAIL
     */  
    bool check(bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/;

  private:
    MasterChecker master_;
    SlaveChecker slave_;
  };

  // helpers

  /**
   * @brief WaitChecker helper
   * @param checker
   * @param wait timeout
   * @param interval between adjacent checks
   */  
  template<typename SubCheckerType>
  WaitChecker<SubCheckerType>
  wait_checker(
    const SubCheckerType& sub_checker,
    unsigned long wait_time = 0,
    unsigned long sleep_time =
      AutoTest::DEFAULT_SLEEP_TIME);

  /**
   * @brief ThrowChecker helper
   * @param checker
   * @param throw on error flag
   * @return throw checker
   */
  template<typename SubCheckerType>
  ThrowChecker<SubCheckerType>
  throw_checker(
    const SubCheckerType& sub_checker,
    bool throw_error = true);

  /**
   * @brief FailChecker helper
   * @param master checker
   * @param slave checker
   * @return fail checker
   */
  template<typename MasterChecker, typename SlaveChecker>
  FailChecker<MasterChecker, SlaveChecker>
  fail_checker(
    const MasterChecker& master,
    const SlaveChecker& slave);

  /**
   * @brief OrChecker helper
   * @param sub checkers
   * @return or checker
   */
  template<typename... SubCheckers>
  OrChecker
  or_checker(
    SubCheckers... sub_checkers);

  /**
   * @brief OrChecker with counter object helper
   * @param counter object
   * @param sub checkers
   * @return or checker
   */
  template<typename... SubCheckers>
  OrChecker
  or_count_checker(
    OrChecker::ICounter* counter,
    SubCheckers... sub_checkers);

  /**
   * @brief AndChecker helper
   * @param sub checkers
   * @return and checker
   */
  template<typename... SubCheckers>
  AndChecker
  and_checker(
    SubCheckers... sub_checkers);

  /**
   * @brief NotChecker helper
   * @param sub checker
   * @return not checker
   */
  template<typename SubCheckerType>
  NotChecker<SubCheckerType>
  not_checker(
    const SubCheckerType& checker);
  
}

#include "CompositeCheckers.tpp"

#endif /*AUTOTESTS_COMMONS_COMPOSITECHECKERS_HPP*/
