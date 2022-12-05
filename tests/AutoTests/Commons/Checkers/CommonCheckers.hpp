#ifndef __AUTOTESTS_COMMONS_COMMONCHECKERS_HPP
#define __AUTOTESTS_COMMONS_COMMONCHECKERS_HPP

#include "Checker.hpp"
#include <Generics/Time.hpp>
#include <initializer_list>
#include <vector>

namespace AutoTest
{
  /**
   * @enum CheckerType
   * @brief Determine possible results of comparison operations.
   */
  enum CheckerType {CT_EQUAL, CT_NOT_EQUAL};

  /**
   * @class EqualChecker
   * @brief Utils for comparing 2 values
   */
  template <typename ArgType1, typename ArgType2>
  class EqualChecker: public Checker
  {
  public:
    EqualChecker(
      const ArgType1& exp_value,
      const ArgType2& got_value,
      CheckerType check_type = CT_EQUAL);

    virtual ~EqualChecker() noexcept;
    
    /**
     * Test that expected value @a exp_value equal or not
     * (depend on @ check_type) with the got value @ got_value
     */
    virtual bool check(bool throw_error = true) /*throw(CheckFailed)*/;

  private:
    ArgType1 exp_value_;
    ArgType2 got_value_;
    CheckerType check_type_;
  };

  /**
   * @class SequenceChecker
   * @brief Sequence checker.
   */

  enum SequenceCheckerEnum
  {
    SCE_COMPARE,
    SCE_ENTRY,
    SCE_NOT_ENTRY
  };

  template<typename FirstSequenceType, typename SecondSequenceType>
  class SequenceChecker: public Checker
  {
  public:
    SequenceChecker(
      const FirstSequenceType& expected,
      const SecondSequenceType& got,
      SequenceCheckerEnum check_type = SCE_COMPARE);

    virtual ~SequenceChecker() noexcept;

    bool check(bool throw_error = true) /*throw(CheckFailed)*/;

  private:
    bool check_seq_(std::string& dsc) const;
    
    FirstSequenceType expected_;
    SecondSequenceType got_;
    SequenceCheckerEnum check_type_;
  };

  /**
   * @class TimeLessChecker
   */
  class TimeLessChecker: public Checker
  {
  public:
    DECLARE_EXCEPTION(TimeLessCheckFailed, CheckFailed);

    TimeLessChecker(
      const Generics::Time& now_less_then) noexcept;

    virtual ~TimeLessChecker() noexcept;

    bool check(bool throw_error = true) /*throw(TimeLessCheckFailed)*/;

  private:
    const Generics::Time now_less_then_;
  };

  /**
   * @class DBRecordChecker
   * @brief Check that expected DB-record exists or not
   */
  template<typename DBFetcher>
  class DBRecordChecker : public Checker
  {
  public:
    DBRecordChecker(
      DBFetcher& table,
      bool exists);

    virtual ~DBRecordChecker() noexcept;

    bool check(bool throw_error = true)
      /*throw(CheckFailed, eh::Exception)*/;

  protected:
    DBFetcher& table_;
    bool exists_;
  };

  // helper functions
  void predicate_checker(bool predicate) /*throw(CheckFailed)*/;

  template<typename ArgType1, typename ArgType2>
  EqualChecker<ArgType1,ArgType2>
  equal_checker(
    const ArgType1& exp_value,
    const ArgType2& got_value,
    CheckerType check_type = CT_EQUAL);

  template<typename ArgType>
  EqualChecker<std::string, ArgType>
  equal_checker(
    const char* exp_value,
    const ArgType& got_value,
    CheckerType check_type = CT_EQUAL);

  template<typename FirstSequenceType, typename SecondSequenceType>
  SequenceChecker<FirstSequenceType, SecondSequenceType>
  sequence_checker(
    const FirstSequenceType& expected,
    const SecondSequenceType& got,
    SequenceCheckerEnum check_type = SCE_COMPARE);

  template<typename Arg, size_t Count, typename SequenceType>
  SequenceChecker<std::vector<Arg>, SequenceType>
  sequence_checker(
    const Arg (&expected)[Count],
    const SequenceType& got,
    SequenceCheckerEnum check_type = SCE_COMPARE);

  template<typename Arg, typename SequenceType>
  SequenceChecker<std::vector<Arg>, SequenceType>
  sequence_checker(
    std::initializer_list<Arg> expected,
    const SequenceType& got,
    SequenceCheckerEnum check_type = SCE_COMPARE);

  template<typename Arg, typename SequenceType>
  SequenceChecker<std::vector<Arg>, SequenceType>
  entry_checker(
    const Arg& expected,
    const SequenceType& got,
    SequenceCheckerEnum check_type = SCE_ENTRY);

  template<typename DBFetcher>
  DBRecordChecker<DBFetcher>
  db_record_checker(DBFetcher& table, bool exists);
  
} //namespace AutoTest

#include "CommonCheckers.tpp"

#endif  // __AUTOTESTS_COMMONS_COMMONCHECKERS_HPP
