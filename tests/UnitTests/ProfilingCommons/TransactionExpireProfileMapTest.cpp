/**
 * @file TransactionExpireProfileMapTest.cpp
 */

#include <TestCommons/MTTester.hpp>
#include <Generics/AppUtils.hpp>
#include <Generics/Rand.hpp>

#include <Commons/Algs.hpp>
#include <ProfilingCommons/ProfileMap/TransactionProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>

using namespace AdServer::ProfilingCommons;

namespace
{
  const char USAGE[] =
    "TransactionExpireProfileMapTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n";

  const char DEFAULT_ROOT_PATH[] = "./";
  const char MAP_PATH[] = "/TransactionExpireProfileMap";
  const std::size_t ITERATIONS_IN_THREAD = 1000;

  const std::size_t TASKS_FOR_KEY = 1000;
  const std::size_t KEYS_AMOUNT = 10;
}

class LongIndexAccessor :
  public PlainStorage::NumberSerializer<uint32_t>
{
public:
  typedef uint32_t NumberType;
  typedef Generics::NumericHashAdapter<NumberType>
    NumericHashAdapter;

  unsigned int
  size(const NumericHashAdapter& key)
    /*throw(eh::Exception)*/
  {
    return sizeof(key.value());
  }

  PlainStorage::SizeType
  load(const void* buf, PlainStorage::SizeType size, NumericHashAdapter& key)
    /*throw(eh::Exception)*/
  {
    key = NumericHashAdapter(*static_cast<const NumberType*>(buf));
    return size;
  }

  void
  save(const NumericHashAdapter& key, void* buf, unsigned int size)
    /*throw(eh::Exception)*/
  {
    ::memcpy(buf, (void*)&key.value(), size);
  }
};

/// Multi-thread executable functor
template <typename TransMapType>
struct Tester
{
  Tester(TransMapType& map) noexcept;

  void
  operator ()() /*throw(eh::Exception)*/;

  bool
  check() /*throw(eh::Exception)*/;

private:
  typedef Sync::PosixMutex Mutex;
  typedef Sync::PosixGuard Guard;
  typedef std::vector<long> TasksWithKeys;

  /// need to pop up task from shared queue of tasks
  Mutex mutex_;
  TransMapType& transactions;
  /// Each Key mean one task for this Key.
  TasksWithKeys tasks_;
};

//
// Tester implementation
//
template <typename Transactions>
Tester<Transactions>::Tester(Transactions& map) noexcept
  : transactions(map)
{
  tasks_.reserve(KEYS_AMOUNT * TASKS_FOR_KEY);
  for (std::size_t i = 0; i < KEYS_AMOUNT; ++i)
  {
    for (std::size_t j = 0; j < TASKS_FOR_KEY; ++j)
    {
      tasks_.push_back(i);
    }
  }
  std::random_shuffle(tasks_.begin(), tasks_.end());
}

template <typename Transactions>
void
Tester<Transactions>::operator ()() /*throw(eh::Exception)*/
{
  long key;

  {
    // Choice key for transaction
    Guard lock(mutex_);
    key = tasks_.back();
    tasks_.pop_back();
  }

  {
    typename Transactions::Transaction_var transaction =
      transactions.get_transaction(key);
    Generics::SmartMemBuf_var content = Algs::copy_membuf(transaction->get_profile());

    if (!content.in())
    {
      // Grow container
      Generics::SmartMemBuf_var new_buf(
        new Generics::SmartMemBuf(4));
      *new_buf->membuf().get<uint32_t>() = 0;
      content = Algs::copy_membuf(new_buf);
      transaction->save_profile(Generics::transfer_membuf(new_buf));
    }

    sched_yield();
    ++*content->membuf().get<uint32_t>();

    transaction->save_profile(Generics::transfer_membuf(content));
  }
}

template <typename Transactions>
bool
Tester<Transactions>::check() /*throw(eh::Exception)*/
{
  typedef std::map<std::size_t, uint32_t> Results;

  Results test_filled_results;
  bool result = true;
  for (std::size_t i = 0; i < KEYS_AMOUNT; ++i)
  {
    Generics::ConstSmartMemBuf_var content = transactions.get_profile(i);
    uint32_t value = *content->membuf().get<uint32_t>();
    test_filled_results[i] = value;
    if (value != static_cast<uint32_t>(TASKS_FOR_KEY))
    {
      result = false;
    }
  }

  if (!result)
  {
    for (std::size_t i = 0; i < test_filled_results.size(); ++i)
    {
      std::cerr << test_filled_results[i] << std::endl;
    }
    std::cerr << "Filled " << test_filled_results.size() <<
      " rows, typeid=" << typeid(Transactions).name() << std::endl;
  }
  return result;
}

template <typename MapType>
int
do_check(MapType& map) /*throw(eh::Exception)*/
{
  Tester<MapType> test(map);
  TestCommons::MTTester<Tester<MapType>&> mt_tester(test, 15);

  mt_tester.run(TASKS_FOR_KEY * KEYS_AMOUNT,
    0,
    TASKS_FOR_KEY * KEYS_AMOUNT);

  if (test.check())
  {
    std::cout << "SUCCESS" << std::endl;
    return 0;
  }

  std::cerr << "FAIL" << std::endl;
  return 1;
}

int
main(int argc, char* argv[]) noexcept
{
  try
  {
    using namespace Generics::AppUtils;
    Args args;
    StringOption root_path(DEFAULT_ROOT_PATH);
    CheckOption opt_help;

    args.add(equal_name("path") || short_name("p"), root_path);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    std::cout << "TransactionExpiryProfileMap multithreading test"
      " started.." << std::endl;

    int res = 0;

    std::string cleanup("mkdir -p ");
    cleanup += root_path->c_str();
    cleanup += " ; rm -f ";
    cleanup += root_path->c_str();
    cleanup += "/*ExpireMap.*";
    ::system(cleanup.c_str());

    {
      ReferenceCounting::SmartPtr<TransactionProfileMap<Generics::NumericHashAdapter<uint32_t> > > map =
        ProfileMapFactory::open_transaction_expire_map<
          Generics::NumericHashAdapter<uint32_t>,
          PlainStorage::NumberSerializer<uint32_t> >(
          root_path->c_str(),
          "ExpireMap",
          Generics::Time(100));

      res += do_check(*map);
    }

    {
      ReferenceCounting::SmartPtr<
      TransactionProfileMap<Generics::NumericHashAdapter<uint32_t> > > map =
        ProfileMapFactory::open_transaction_packed_expire_map<
          Generics::NumericHashAdapter<uint32_t>,
          LongIndexAccessor>(
          root_path->c_str(),
          "PackedExpireMap",
          Generics::Time(100));

      res += do_check(*map);
    }

    return res;
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
