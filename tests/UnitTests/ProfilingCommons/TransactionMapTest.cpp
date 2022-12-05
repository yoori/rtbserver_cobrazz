/**
 * @file TransactionMapTest.cpp
 * ------------------
 * Simple test application must fill std::map with predefined values
 * set (0 : 0, 1 : 0, .. , 10 : 0)
 * create TaskRunner and run tasks (in few threads), 100 tasks for each
 * key value from (1 .. 10).
 * each task:
 *   t = get_transaction(task_key)
 *   val = t->get();
 *   sleep(0.1); // nanosleep
 *   t->set(++val)
 *
 * After all task will be executed: map must contains:
 *   (0 : 100, .. , 10 : 100)
 */

#include <TestCommons/MTTester.hpp>

#include <Commons/LockMap.hpp>
#include <ProfilingCommons/ProfileMap/TransactionMap.hpp>

using namespace AdServer::ProfilingCommons;

namespace
{
  const std::size_t TASKS_FOR_KEY = 1000;
  const long KEYS_AMOUNT = 10;
}

struct TestResult
{
  /// fill std::map with predefined values
  ///   set (0 : 0, 1 : 0, .. , 10 : 0)
  TestResult() noexcept
  {
    for (long i = 0; i < KEYS_AMOUNT; ++i)
    {
      test_filled_results[i] = 0;
    }
  }

  /**
   * Check filled results by test pass
   */
  bool
  check() /*throw(eh::Exception)*/
  {
    SharedResults standard;
    for (long i = 0; i < KEYS_AMOUNT; ++i)
    {
      standard[i] = TASKS_FOR_KEY;
    }
    bool result = test_filled_results == standard;
    if (!result)
    {
      for (std::size_t i = 0; i < test_filled_results.size(); ++i)
      {
        std::cerr << test_filled_results[i] << std::endl;
      }
      std::cerr << "Filled " << test_filled_results.size()
        << " rows" << std::endl;
    }
    return result;
  }

  // Here object that we fill through transactions mechanism by some threads
  // Key -> TestResult4Key
  typedef std::map<long, long> SharedResults;
  SharedResults test_filled_results;
};

  /**
   * class TransactionImpl: TransactionBase
   * { unsigned long get() // simple find and return value from std::map
   * save(unsigned long) // simple std::map::[]= };
   */

  class SimpleTransaction : public TransactionBase
  {
  public:
    // Transaction on this memory region
    static TestResult shared_memory;

    struct ArgType
    {};

    /**
     * @param holder Holds transaction mutex
     * @param key The access key to locked memory region by transaction
     */
    SimpleTransaction(
      TransactionBase::TransactionHolderBase* holder,
      const Generics::NumericHashAdapter<long>& key)
      noexcept
      : TransactionBase(holder),
        key_(key)
    {
    }

    /**
     * simple find and return value from safe shared by transaction std::map
     */
    long
    get() noexcept
    {
      return shared_memory.test_filled_results[key_.value()];
    }

    /**
     * simple set value to safe shared by transaction std::map
     */
    void
    set(long value) /*throw(eh::Exception)*/
    {
      shared_memory.test_filled_results[key_.value()] = value;
    }
  protected:
    virtual
    ~SimpleTransaction() noexcept
    {
    }
  private:
    /// The key for this transaction
    Generics::NumericHashAdapter<long> key_;
  };

  /// Create and fill std::map
  TestResult SimpleTransaction::shared_memory;


  template <typename KeyType>
  class SimpleTransactionsMap :
    public TransactionMap<KeyType, SimpleTransaction>
  {
    typedef TransactionMap<KeyType, SimpleTransaction> BaseType;
  public:
    typedef typename BaseType::Transaction_var Transaction_var;

    SimpleTransactionsMap() noexcept
      : TransactionMap<KeyType, SimpleTransaction>()
    {}

    virtual
    ~SimpleTransactionsMap() noexcept
    {
    }
  private:
    typedef typename BaseType::TransactionHolder TransactionHolder;

    virtual Transaction_var
    create_transaction_impl_(
      TransactionHolder* holder,
      const KeyType& key,
      const SimpleTransaction::ArgType& /* arg */)
      /*throw(eh::Exception)*/
    {
      return new SimpleTransaction(holder, key);
    }
  };


/// Multi-thread executable functor
struct Tester
{
  Tester() noexcept;

  void
  operator ()() /*throw(eh::Exception)*/;

private:
  typedef Sync::PosixMutex Mutex;
  typedef Sync::PosixGuard Guard;
  typedef SimpleTransactionsMap<Generics::NumericHashAdapter<long> >
    Transactions;

  /// need to pop up task from shared queue of tasks
  Mutex mutex_;
  Transactions transactions;
  /// Each Key mean one task for this Key.
  typedef std::vector<long> TasksWithKeys;
  TasksWithKeys tasks_;
};

//
// Tester implementation
//
Tester::Tester() noexcept
{
  tasks_.reserve(KEYS_AMOUNT * TASKS_FOR_KEY);
  for (long i = 0; i < KEYS_AMOUNT; ++i)
  {
    for (std::size_t j = 0; j < TASKS_FOR_KEY; ++j)
    {
      tasks_.push_back(i);
    }
  }
  std::random_shuffle(tasks_.begin(), tasks_.end());
}

void
Tester::operator ()() /*throw(eh::Exception)*/
{
  long key;
  { // Choice key for transaction
    Guard lock(mutex_);
    key = tasks_.back();
    tasks_.pop_back();
  }

  {
    Transactions::Transaction_var transaction =
      transactions.get_transaction(key);
    long content = transaction->get();

  //  sleep(.1);
    sched_yield();

    transaction->set(++content);
  }
}

int
main(int /*argc*/, char** /*argv*/) noexcept
{
  typedef AdServer::Commons::LockMap<Generics::StringHashAdapter> StringLockMap;
  StringLockMap lm;
  StringLockMap::WriteGuard lock = lm.write_lock("test");
  
  try
  {
    std::cout << "TransactionMap multithreading test started.." << std::endl;

    TestResult shared_memory;
    Tester test;
    TestCommons::MTTester<Tester&> mt_tester(
      test, 15);

    mt_tester.run(TASKS_FOR_KEY * KEYS_AMOUNT,
      0,
      TASKS_FOR_KEY * KEYS_AMOUNT);

    if (SimpleTransaction::shared_memory.check())
    {
      std::cout << "SUCCESS" << std::endl;
    }
    else
    {
      std::cerr << "FAIL" << std::endl;
    }
    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception" << std::endl;
  }

  return -1;
}
