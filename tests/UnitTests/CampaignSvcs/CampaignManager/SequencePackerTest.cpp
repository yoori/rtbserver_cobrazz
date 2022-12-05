// @file CampaignManager/SequencePackerTest.cpp

#include <list>
#include <iostream>
#include <ReferenceCounting/DefaultImpl.hpp>
#include <Generics/Time.hpp>
#include <CampaignSvcs/CampaignManager/SequencePacker.hpp>

using namespace AdServer::CampaignSvcs;

struct TestObject : public ReferenceCounting::DefaultImpl<>
{
  TestObject(int i) : i_(i) {}

  TestObject(const TestObject& other) noexcept
    : ReferenceCounting::Interface(),
      ReferenceCounting::DefaultImpl<>(),
      i_(other.i_)
  {
  }

  virtual
  ~TestObject() noexcept {}
  
  bool
  operator ==(const TestObject& right) const
  {
    return i_ == right.i_;
  }

  bool
  operator <(const TestObject& right) const
  {
    return i_ == right.i_;
  }

  unsigned long
  hash() const
  {
    return i_;
  }
  
  int i_;
};

typedef ReferenceCounting::SmartPtr<const TestObject> TestObject_var;

struct TestObjectList:
  public ReferenceCounting::DefaultImpl<>,
  public ElementSeqBase,
  std::list<TestObject_var>
{
  TestObjectList() noexcept
  {}

  TestObjectList(const TestObjectList& other) /*throw(eh::Exception)*/
    : ReferenceCounting::Interface(),
      ReferenceCounting::DefaultImpl<>(),
      ElementSeqBase(other), std::list<TestObject_var>(other)
  {
  }

  virtual
  ~TestObjectList() noexcept
  {
    unkeep_(this);
  }

  void
  insert(const TestObject* obj)
  {
    std::list<TestObject_var>::iterator it = begin();
    
    for (; it != end(); ++it)
    {
      if (*obj < *(*it))
      {
        break;
      }
    }

    std::list<TestObject_var>::insert(
      it, ReferenceCounting::add_ref(obj));
  }
};

typedef ReferenceCounting::SmartPtr<const TestObjectList> TestObjectList_var;

typedef
  ReferenceCounting::SmartPtr<SequencePacker<TestObject, TestObjectList> >
  SequencePacker_var;

int perf_test()
{
  int result = 0;
  
  try
  {
    Generics::Timer timer;
    
    SequencePacker_var sequence_packer(
      new SequencePacker<TestObject, TestObjectList>());

    TestObjectList_var prev_seq;

    timer.start();
    
    for(unsigned long i = 1; i < 1000; ++i)
    {
      prev_seq = sequence_packer->get(
        prev_seq.in(), TestObject_var(new TestObject(i)));
    }

    timer.stop();
    std::cout << "Insert of 1000 sequences time: " << timer.elapsed_time() <<
      ", hashes count = " << 0 << std::endl;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    result = 1;
  }

  return result;
}

int perf_with_keep_test()
{
  int result = 0;
  
  try
  {
    Generics::Timer timer;
    
    SequencePacker_var sequence_packer(
      new SequencePacker<TestObject, TestObjectList>());

    std::list<TestObjectList_var> seqs;
    
    {
      TestObjectList_var prev_seq;

      timer.start();
    
      for(unsigned long i = 1; i < 1000; ++i)
      {
        prev_seq = sequence_packer->get(
          prev_seq.in(), TestObject_var(new TestObject(i)));
        seqs.push_back(prev_seq);
      }

      timer.stop();
    }

    unsigned long hashes_count = 0; // sequence_packer->hashes_count_();

    Generics::Timer destroy_timer;
    destroy_timer.start();
    seqs.clear();
    destroy_timer.stop();
    
    std::cout << "Insert of 1000 sequences with keeping: time = " <<
      (timer.elapsed_time() + destroy_timer.elapsed_time()) <<
      ", without destroy time = " << timer.elapsed_time() <<
      ", hashes count = " << hashes_count << std::endl;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    result = 1;
  }

  return result;
}

int
main() noexcept
{
  int result = 0;
  try
  {
    SequencePacker_var sequence_packer(
      new SequencePacker<TestObject, TestObjectList>());
  
    TestObjectList_var first_seq;
      
    {
      first_seq =
        sequence_packer->get(0, TestObject_var(new TestObject(1)));
      
      TestObjectList_var second_seq =
        sequence_packer->get(first_seq, TestObject_var(new TestObject(2)));
      TestObjectList_var third_seq =
        sequence_packer->get(second_seq, TestObject_var(new TestObject(3)));
  
      {
        TestObjectList_var first_seq_2 =
          sequence_packer->get(0, TestObject_var(new TestObject(1)));
        TestObjectList_var second_seq_2 =
          sequence_packer->get(first_seq_2,
            TestObject_var(new TestObject(2)));
        TestObjectList_var third_seq_2 =
          sequence_packer->get(second_seq_2,
            TestObject_var(new TestObject(3)));
  
        if (first_seq.in() != first_seq_2.in() ||
          second_seq.in() != second_seq_2.in())
        {
          std::cerr << "sequence pointers isn't equal." << std::endl;
          result = 1;
        }

        // erase one sequence and check only leftover 2
        second_seq.reset();
        second_seq_2.reset();
  
        if (sequence_packer->ptrs_size_() != 2 ||
          sequence_packer->size_() != 2)
        {
          std::cerr
            << "sequence packer size isn't equal 2: " << std::endl
            << "  sequence_packer->ptrs_size_() = "
            << sequence_packer->ptrs_size_() << std::endl
            << "  sequence_packer->size_() = "
            << sequence_packer->size_() << std::endl;
          result = 1;
        }
      }
    }
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    result = 1;
  }

  result += perf_test();
  result += perf_with_keep_test();
  
  return result;
}
