#include <algorithm> 
#include <functional>
#include "TestFactory.hpp"

namespace TestFactory
{
  // Utils

  // Check group predicate
  struct IsGroup : public std::binary_function<AutoTestSpeedGroup, int, bool>
  {
    bool operator() (
      AutoTestSpeedGroup g1,
      int g2) const
    {
      return g2 & g1;
    }
  };
    
  struct IsTrue : public std::unary_function<bool,bool>
  {
    bool operator() (
      bool v) const
    {
      return v;
    }
  };

  typedef std::logical_not<bool> IsFalse;

  // Serialize filter
  struct SerializedPred : public std::unary_function<UnitDescriptor* ,bool>
  {
    SerializedPred(int serialized_mode) :
      serialized_mode_(serialized_mode)
    { }
    
    bool operator() (
      const UnitDescriptor* u) const
    {
      return !(serialized_mode_ && u->serialize);
    }

  private:
    int serialized_mode_;
  };

  // Common filter
  template<class MemberType, class UnaryPredicate>
  class IsPred
  {
    typedef typename std::equal_to<MemberType> EqualTo;
    typedef typename std::set<MemberType> List;
    
  public:
    IsPred(
      const List& list,
      MemberType UnitDescriptor::* member,
      UnaryPredicate pred) :
      list_(list),
      member_(member),
      pred_(pred)
    { }

    bool operator() (
      const UnitDescriptor* u) const
    {
      return
        !list_.empty() &&
          pred_(
            std::count_if(
              list_.begin(),
              list_.end(),
              std::bind1st(
                EqualTo(),
                u->*(member_))) != 0);
    }

  private:
    const List& list_;
    MemberType UnitDescriptor::* member_;
    UnaryPredicate pred_;
  };

  // Group filter
  class GroupUnaryPred
  {
  public:
    GroupUnaryPred(
      const GroupList& list,
      bool mode) :
      list_(list),
      mode_(mode)
    { }
      
    bool operator() (
      const UnitDescriptor* u) const
    {
      bool check =
        std::count_if(
          list_.begin(),
          list_.end(),
          std::bind2nd(
            IsGroup(),
            u->group));

      if (check)
      {
        return false;
      }

      bool is_dual =
        (u->group & AUTO_TEST_SLOW) &&
        (u->group & AUTO_TEST_FAST);

      return !list_.empty() && !(is_dual && mode_);
    }
  private:
    const GroupList& list_;
    bool mode_;
  };


  // Check entry to run list
  template<typename MemberType>
  IsPred<MemberType, IsTrue>
  is_entry(
    const typename std::set<MemberType>& list,
    MemberType UnitDescriptor::* member)
  {
    return IsPred<MemberType, IsTrue>(list, member, IsTrue());
  }

  // Check not entry to run list
  template<typename MemberType>
  IsPred<MemberType, IsFalse>
  is_not_entry(
    const typename std::set<MemberType>& list,
    MemberType UnitDescriptor::* member)
  {
    return IsPred<MemberType, IsFalse>(list, member, IsFalse());
  }

  // Tests filter predicate
  class FilterPredicate
  {
  public:
    FilterPredicate(
      const StringList& exclude_tests,
      const StringList& exclude_categories,
      const StringList& tests,
      const GroupList& groups,
      const StringList& categories,
      int select_serialized) :
      exclude_tests_(exclude_tests),
      exclude_categories_(exclude_categories),
      tests_(tests),
      groups_(groups),
      categories_(categories),
      select_serialized_(select_serialized)
    {  }

    bool operator() (
      const UnitDescriptor* u) const
    {
      if (is_entry(tests_, &UnitDescriptor::name)(u))
      {
        return false;
      }

      if (!SerializedPred(select_serialized_)(u))
      {
        return false;
      }
      
      return
        (GroupUnaryPred(
           groups_,
           (groups_.size() == 1 &&
             *groups_.begin() == AUTO_TEST_SLOW))(u) ||
          is_not_entry(categories_, &UnitDescriptor::category)(u) ||
          is_entry(exclude_tests_, &UnitDescriptor::name)(u) ||
          is_entry(exclude_categories_, &UnitDescriptor::category)(u));
    }

  private:
    const StringList& exclude_tests_;
    const StringList& exclude_categories_;
    const StringList& tests_;
    const GroupList& groups_;
    const StringList& categories_;
    int select_serialized_;
  };

  // class TestFactory

  TestFactory::TestFactory() noexcept
  {
    for (UnitDescriptor* i = base_unit_descriptor; i; i = i->next)
    {
      units_.push_back(i);
    }
  }
  
  TestFactory::~TestFactory() noexcept
  { }
  
  void
  TestFactory::filter(
    const StringList& exclude_tests,
    const StringList& exclude_categories,
    const StringList& tests,
    const GroupList& groups,
    const StringList& categories,
    int select_serialized)
    noexcept
  {
    units_.remove_if(
      FilterPredicate(
        exclude_tests,
        exclude_categories,
        tests,
        groups,
        categories,
        select_serialized));
  }

  void
  TestFactory::filter(
    const StringList& tests)
    noexcept
  {
    units_.remove_if(
      is_not_entry(tests, &UnitDescriptor::name));
  }

  const UnitsList&
  TestFactory::units()
    noexcept
  {
    return units_;
  }
 
}
