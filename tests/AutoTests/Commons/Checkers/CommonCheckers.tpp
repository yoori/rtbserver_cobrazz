#ifndef AUTOTESTS_COMMONS_COMMONCHECKERS_IPP
#define AUTOTESTS_COMMONS_COMMONCHECKERS_IPP

#include <tests/AutoTests/Commons/Sequence.hpp>

namespace AutoTest
{
  // EqualChecker
  template<typename ArgType1,typename ArgType2>
  EqualChecker<ArgType1, ArgType2>::EqualChecker(
    const ArgType1& exp_value,
    const ArgType2& got_value,
    CheckerType check_type)
    : exp_value_(exp_value),
      got_value_(got_value),
      check_type_(check_type)
  {}

  template<typename ArgType1,typename ArgType2>
  EqualChecker<ArgType1, ArgType2>::~EqualChecker()
    noexcept
  { }
  
  template<typename ArgType1,typename ArgType2>
  bool
  EqualChecker<ArgType1, ArgType2>::check(bool throw_error)
    /*throw (CheckFailed)*/
  {
    
    bool result = equal(exp_value_, got_value_);
    if (check_type_ == CT_NOT_EQUAL)
    {
      result = !result;
    }
    if(throw_error && !result)
    {
      Stream::Error ostr;
      ostr << "Error: 'equal' constraint failed " <<
        to_string(exp_value_) << " != " <<
        to_string(got_value_) << " (expected != got)";
      throw CheckFailed(ostr);
    }
    return result;
  }

  template<typename ArgType1, typename ArgType2>
  EqualChecker<ArgType1,ArgType2>
  equal_checker(
    const ArgType1& exp_value,
    const ArgType2& got_value,
    CheckerType check_type)
  {
    return EqualChecker<ArgType1,ArgType2>(
      exp_value,
      got_value,
      check_type);
  }

  template<typename ArgType>
  EqualChecker<std::string, ArgType>
  equal_checker(
    const char* exp_value,
    const ArgType& got_value,
    CheckerType check_type)
  {
    return EqualChecker<std::string,ArgType>(
      exp_value,
      got_value,
      check_type);
  }

  // SequenceChecker
  template<typename FirstSequenceType, typename SecondSequenceType>
  SequenceChecker<FirstSequenceType, SecondSequenceType>::SequenceChecker(
    const FirstSequenceType& expected,
    const SecondSequenceType& got,
    SequenceCheckerEnum check_type)
    : expected_(expected),
      got_(got),
      check_type_(check_type)
  { }

  template<typename FirstSequenceType, typename SecondSequenceType>
  SequenceChecker<FirstSequenceType, SecondSequenceType>::~SequenceChecker()
    noexcept
  { }

  template<typename FirstSequenceType, typename SecondSequenceType>
  bool
  SequenceChecker<FirstSequenceType, SecondSequenceType>::check_seq_(
    std::string& dsc) const
  {
    switch (check_type_)
    {
    case SCE_COMPARE:
        dsc = " != ";
        return (countof(expected_) == countof(got_) &&
          std::equal(
            beginof(expected_),
            endof(expected_),
            beginof(got_)));
    case SCE_ENTRY:
        dsc = " not entry in ";
        return AutoTest::entry_in_seq(
          expected_, got_);
    case SCE_NOT_ENTRY:
        dsc = " entry in ";
        return AutoTest::not_entry_in_seq(
          expected_, got_);
    }
    return false;
  }

  template<typename FirstSequenceType, typename SecondSequenceType>
  bool
  SequenceChecker<FirstSequenceType, SecondSequenceType>::check(
    bool throw_error) /*throw (CheckFailed)*/
  {

    std::string description;

    bool result = check_seq_(description);
    
    if(throw_error && !result)
    {
      Stream::Error ostr;
      ostr << AutoTest::seq_to_str(expected_) <<
        description <<  AutoTest::seq_to_str(got_);
      throw CheckFailed(ostr);  
    }

    return result;
  }

  template<typename FirstSequenceType, typename SecondSequenceType>
  SequenceChecker<FirstSequenceType, SecondSequenceType>
  sequence_checker(
    const FirstSequenceType& expected,
    const SecondSequenceType& got,
    SequenceCheckerEnum check_type)
  {
    return SequenceChecker<FirstSequenceType, SecondSequenceType>(
      expected,
      got,
      check_type);
  }

  template<typename Arg, size_t Count, typename SequenceType>
  SequenceChecker<std::vector<Arg>, SequenceType>
  sequence_checker(
    const Arg (&expected)[Count],
    const SequenceType& got,
    SequenceCheckerEnum check_type)
  {
    std::vector<Arg> exp(expected, expected + Count);

    return SequenceChecker<std::vector<Arg>, SequenceType>(
      exp,
      got,
      check_type);
  }

  template<typename Arg, typename SequenceType>
  SequenceChecker<std::vector<Arg>, SequenceType>
  sequence_checker(
    std::initializer_list<Arg> expected,
    const SequenceType& got,
    SequenceCheckerEnum check_type)
  {
    std::vector<Arg> exp(expected);
    
    return SequenceChecker<std::vector<Arg>, SequenceType>(
      expected,
      got,
      check_type);
  }

  template<typename Arg, typename SequenceType>
  SequenceChecker<std::vector<Arg>, SequenceType>
  entry_checker(
    const Arg& expected,
    const SequenceType& got,
    SequenceCheckerEnum check_type)
  {
    std::vector<Arg> exp(1, expected);

    return SequenceChecker<std::vector<Arg>, SequenceType>(
      exp,
      got,
      check_type);
  }

  template<typename DBFetcher>
  DBRecordChecker<DBFetcher>::DBRecordChecker(
    DBFetcher& table,
    bool exists)
    : table_(table),
      exists_(exists)
  {}

  template<typename DBFetcher>
  DBRecordChecker<DBFetcher>::~DBRecordChecker() noexcept
  {}

  template<typename DBFetcher>
  bool
  DBRecordChecker<DBFetcher>::check(
    bool throw_error)
    /*throw(CheckFailed, eh::Exception)*/
  {
    if(table_.select() != exists_ )
    {
      if (throw_error)
      {
        Stream::Error ostr;
        ostr << (exists_ ? "can't get expected record" :
          "got unexpected record");
        throw CheckFailed(ostr);
      }
      return false;
    }
    return true;
  }

  template<typename DBFetcher>
  DBRecordChecker<DBFetcher>
  db_record_checker(DBFetcher& table, bool exists)
  {
    return DBRecordChecker<DBFetcher>(table, exists);
  }
}

#endif /*AUTOTESTS_COMMONS_COMMONCHECKERS_IPP*/
