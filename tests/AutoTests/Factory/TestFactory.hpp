#ifndef __TESTFACTORY_HPP
#define __TESTFACTORY_HPP

#include <tests/AutoTests/Commons/Common.hpp>

namespace TestFactory
{
  typedef std::set<std::string> StringList;
  typedef std::set<AutoTestSpeedGroup> GroupList;
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
      const StringList& exclude_tests,
      const StringList& exclude_categories,
      const StringList& tests,
      const GroupList& groups,
      const StringList& categories,
      int select_serialized)
      noexcept;

    void
    filter(
      const StringList& tests)
      noexcept;

    const UnitsList&
    units()
      noexcept;

  private:
    UnitsList units_;
  };

}; //namespace
  

#endif  // __TESTFACTORY_HPP
