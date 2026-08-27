#pragma once

#include <tests/AutoTests/Commons/Common.hpp>

namespace TestFactory
{
  typedef std::set<std::string> StringArray;
  typedef std::set<AutoTestSpeedGroup> GroupArray;
  typedef std::list<UnitDescriptor*> UnitsList;
  typedef std::vector<UnitDescriptor*> UnitsSeq;
  typedef const xsd::tests::AutoTests::UnitLocalDataType& Locals;

  DECLARE_EXCEPTION(Exception, eh::Exception);
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  class TestFactory
  {

  public:
    TestFactory() noexcept;

    ~TestFactory() noexcept;

    void
    filter(
      const StringArray& exclude_tests,
      const StringArray& exclude_categories,
      const StringArray& tests,
      const GroupArray& groups,
      const StringArray& categories,
      int select_serialized)
      noexcept;

    void
    filter(const StringArray& tests)
      noexcept;

    const UnitsList&
    units()
      noexcept;

  private:
    UnitsList units_;
  };

}; //namespace
