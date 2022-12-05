// @file ProfilingCommons/ExpireProfileMapThreadsTest.cpp
#include <Generics/AppUtils.hpp>
#include <TestCommons/MTTester.hpp>
#include "ProfileMapUtils.hpp"

using namespace AdServer::ProfilingCommons;

namespace
{
  const char THREAD_FRAGMENT_STORM_PATH[] =
    "/ExpireProfileMapTestCont/ThreadFragmentStorm/";
  const char THREAD_ONE_STORM_PATH[] =
    "/ExpireProfileMapTestCont/ThreadOneStorm/";
  const char THREAD_ONE_MOVE_STORM_PATH[] =
    "/ExpireProfileMapTestCont/ThreadOneMoveStorm/";
  const char THREAD_STORM_CHECK_PATH[] =
    "/ExpireProfileMapTestCont/ThreadStorm/";
  const char THREAD_TIMED_STORM_ONE_PATH[] =
    "/ExpireProfileMapTestCont/ThreadTimedStormOne/";
  const char THREAD_TIMED_STORM_FEW_PATH[] =
    "/ExpireProfileMapTestCont/ThreadTimedStormFew/";

  const char USAGE[] =
    "ExpireProfileMapThreadsTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -h, --help : show this message.\n";
}

typedef AdServer::ProfilingCommons::ExpireProfileMap<
  Generics::StringHashAdapter,
  PlainStorage::StringSerializer>
  StringMap;

template<typename MapType>
struct WriteReadProfileFunctor
{
  WriteReadProfileFunctor(
    MapType& map_val,
    unsigned long max_profile_size_val,
    long profiles_count_val = -1)
    : map(map_val),
      max_profile_size(max_profile_size_val),
      id("test"),
      profiles_count(profiles_count_val)
  {}

  void operator()()
  {
    Generics::Time time(0);

    std::ostringstream id_str;
    id_str << id;

    if(profiles_count != -1)
    {
      id_str << (::rand() % profiles_count);
    }

    Generics::SmartMemBuf_var read_buf(map.get_profile(id_str.str().c_str()));
    unsigned long old_size = read_buf.in() ? read_buf->membuf().size() : 0;
    assert(old_size % 100 == 0);

    unsigned long new_size = 0;
    if(old_size + 100 < max_profile_size)
    {
      new_size = old_size + 100;
    }

    Generics::SmartMemBuf_var new_buf(new Generics::SmartMemBuf(new_size));
    map.save_profile(id_str.str().c_str(), Generics::transfer_membuf(new_buf), time);

//  std::cerr << "saved" << std::endl;
  }

  MapType& map;
  unsigned long max_profile_size;
  std::string id;
  long profiles_count;
};

template<typename MapType>
struct TimedWriteReadProfileFunctor
{
  TimedWriteReadProfileFunctor(
    MapType& map_val,
    unsigned long min_profile_size_val,
    unsigned long max_profile_size_val,
    unsigned long profile_size_step_val,
    long profiles_count_val = -1)
    : map(map_val),
      min_profile_size(min_profile_size_val),
      max_profile_size(max_profile_size_val),
      profile_size_step(profile_size_step_val),
      id("test"),
      profiles_count(profiles_count_val)
  {}

  void operator()()
  {
    Generics::Time time(::rand() % 100);

    std::ostringstream id_str;
    id_str << id;

    if(profiles_count != -1)
    {
      id_str << (::rand() % profiles_count);
    }

    Generics::SmartMemBuf_var read_buf(map.get_profile(id_str.str().c_str()));
    unsigned long old_size = 0;

    if(read_buf.in())
    {
      old_size = read_buf->membuf().size();
      if(old_size < min_profile_size || old_size > max_profile_size)
      {
        assert(0);
      }
    }

    unsigned long new_size = min_profile_size;
    if(old_size + profile_size_step <= max_profile_size)
    {
      new_size = old_size + profile_size_step;
    }

    Generics::SmartMemBuf_var new_buf(new Generics::SmartMemBuf(new_size));
    map.save_profile(id_str.str().c_str(), Generics::transfer_membuf(new_buf), time);
  }

  MapType& map;
  unsigned long min_profile_size;
  unsigned long max_profile_size;
  unsigned long profile_size_step;
  std::string id;
  long profiles_count;
};

template<typename MapType>
struct WriteRemoveProfileFunctor
{
  WriteRemoveProfileFunctor(
    MapType& map_val,
    unsigned long /*profile_size*/,
    long profiles_count_val = -1)
    : map(map_val),
      id("test"),
      profiles_count(profiles_count_val)
  {}

  void operator()()
  {
    try
    {
      std::ostringstream id_str;
      id_str << id;

      if(profiles_count != -1)
      {
        id_str << (::rand() % profiles_count);
      }

      Generics::SmartMemBuf_var new_buf(new Generics::SmartMemBuf(1000));

      /*
      bool small = (::rand() % 1 == 0);
      Generics::SmartMemBuf_var new_buf(new Generics::SmartMemBuf(
        small ? (::rand() % 300) : (::rand() % 30000)));
      */
      map.save_profile(id_str.str().c_str(), Generics::transfer_membuf(new_buf), Generics::Time(0));
      if(::rand() % 2 == 0)
      {
        map.remove_profile(id_str.str().c_str());
      }
//    }
    }
    catch(const eh::Exception& ex)
    {
      std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
      throw;
    }
  }

  MapType& map;
  unsigned long max_profile_size;
  std::string id;
  long profiles_count;
};

template<typename MapType>
struct WriteRemoveRareProfileFunctor
{
  WriteRemoveRareProfileFunctor(
    MapType& map_val,
    unsigned long /*profile_size*/,
    long profiles_count_val = -1)
    : map(map_val),
      id("test"),
      profiles_count(profiles_count_val)
  {}

  void operator()()
  {
    try
    {
      std::ostringstream id_str;
      id_str << id;

      if(profiles_count != -1)
      {
        id_str << (::rand() % profiles_count);
      }

      Generics::SmartMemBuf_var new_buf(new Generics::SmartMemBuf(1000));

      /*
      bool small = (::rand() % 1 == 0);
      Generics::SmartMemBuf_var new_buf(new Generics::SmartMemBuf(
        small ? (::rand() % 300) : (::rand() % 30000)));
      */
      map.save_profile(id_str.str().c_str(), Generics::transfer_membuf(new_buf), Generics::Time(0));

      if(::rand() % 64 == 0)
      {
        map.remove_profile(id_str.str().c_str());
      }
//    }
    }
    catch(const eh::Exception& ex)
    {
      std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
      throw;
    }
  }

  MapType& map;
  unsigned long max_profile_size;
  std::string id;
  long profiles_count;
};

void remove_and_create(const char* folder_name)
{
  system((std::string("rm -r ") + folder_name + " 2>/dev/null ; "
    "mkdir -p " + folder_name).c_str());
}

template<typename MapType>
int threads_one_profile_storm_test(
  const char* root, unsigned long time)
{
  static const char* FUN = "threads_one_profile_storm_test()";

  typedef ProfileMap<typename MapType::KeyTypeT> UseMapType;
  typedef ReferenceCounting::SmartPtr<UseMapType> UseMap_var;

  try
  {
    UseMap_var test = init_map<MapType>(root, "", Generics::Time(100));

    WriteReadProfileFunctor<UseMapType> fun(*test, 200);
    TestCommons::MTTester<WriteReadProfileFunctor<UseMapType>&> mt_tester(fun, 10);
    mt_tester.run(1000, time, 10);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": eh::Exception: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}

template<typename MapType>
int threads_one_profile_move_storm_test(
  const char* root, unsigned long time)
{
  static const char* FUN = "threads_one_profile_move_storm_test()";

  typedef ProfileMap<typename MapType::KeyTypeT> UseMapType;
  typedef ReferenceCounting::SmartPtr<UseMapType> UseMap_var;

  try
  {
    UseMap_var test = init_map<MapType>(root, "", Generics::Time::ONE_SECOND);

    TimedWriteReadProfileFunctor<UseMapType> fun(*test, 1, 3000, 10);
    TestCommons::MTTester<TimedWriteReadProfileFunctor<UseMapType>&> mt_tester(fun, 20);
    mt_tester.run(1000, time, 10);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": eh::Exception: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}

template<typename MapType, template <typename> class FunType>
int threads_profile_storm_test(
  const char* root, long profiles_count, unsigned long /*time*/)
{
  static const char* FUN = "threads_profile_storm_test()";

  typedef ProfileMap<typename MapType::KeyTypeT> UseMapType;
  typedef ReferenceCounting::SmartPtr<UseMapType> UseMap_var;

  try
  {
    UseMap_var test = init_map<MapType>(root, "", Generics::Time(100));

    FunType<UseMapType> fun(*test, 2000, profiles_count);
    TestCommons::MTTester<FunType<UseMapType>&> mt_tester(fun, 100);
    mt_tester.run(1000, 10, -1);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": eh::Exception: " << ex.what() << std::endl;
    return 1;
  }

  remove_and_create(root);

  return 0;
}

template<typename MapType>
int threads_profile_storm_timed_test(
  const char* root, long profiles_count, unsigned long time)
{
  typedef ProfileMap<typename MapType::KeyTypeT> UseMapType;
  typedef ReferenceCounting::SmartPtr<UseMapType> UseMap_var;

  static const char* FUN = "threads_profile_storm_timed_test()";

  try
  {
    UseMap_var test = init_map<MapType>(root, "", Generics::Time(25));

    TimedWriteReadProfileFunctor<UseMapType> fun(*test, 0, 2000, 100, profiles_count);
    TestCommons::MTTester<TimedWriteReadProfileFunctor<UseMapType>&> mt_tester(fun, 30);
    mt_tester.run(10000, time, 100);
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << FUN << ": eh::Exception: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}

int
main(int argc, char* argv[]) noexcept
{
  using namespace Generics::AppUtils;
  Args args;
  CheckOption opt_help;
  Option<unsigned long> opt_time(100);
  Option<std::string> opt_root_dir("./");

  args.add(equal_name("help") || short_name("h"), opt_help);
  args.add(equal_name("time") || short_name("t"), opt_time);
  args.add(equal_name("dir") || short_name("p"), opt_root_dir);

  args.parse(argc - 1, argv + 1);

  if (opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return 0;
  }

  int result = 0;

  try
  {
    {
      std::cout << "==== fragmented allocation testing ====" << std::endl;

      std::string root = *opt_root_dir + THREAD_FRAGMENT_STORM_PATH;

      remove_and_create(root.c_str());

      /*
      ReferenceCounting::SmartPtr<ProfileMap<std::string> > test =
        init_map<StringMap>(root.c_str(), "", Generics::Time(100));

      
      for(int i = 0; i < 1000; ++i)
      {
        std::ostringstream id_str;
        id_str << "f" << i;

        Generics::SmartMemBuf_var new_buf(new Generics::SmartMemBuf(1000));
        test->save_profile(id_str.str().c_str(), new_buf->membuf(), Generics::Time(0));
      }

      for(int i = 0; i < 1000 / 64; ++i)
      {
        std::ostringstream id_str;
        id_str << "f" << (i * 64);
        test->remove_profile(id_str.str().c_str());
      }

      test = 0;

      std::cout << "fragmented file created." << std::endl;
*/
      /*
      for(int i = 0; i < 1000; ++i)
      {
      */
      for(int i = 0; i < 10; ++i)
      {
        std::cout << "test #" << i << std::endl;
        result += threads_profile_storm_test<StringMap, WriteRemoveProfileFunctor>(
          root.c_str(), 50000, *opt_time);
      }
    }

    /*
    {
      std::cout << "==== small fragment testing ====" << std::endl;

      remove_and_create(
        (*opt_root_dir + THREAD_FRAGMENT_STORM_PATH).c_str());

      result += threads_profile_storm_test<StringMap, WriteRemoveRareProfileFunctor>(
        (*opt_root_dir + THREAD_FRAGMENT_STORM_PATH).c_str(),
        50000, *opt_time);

      std::cout << "==== small fragment testing after reopen ====" << std::endl;

      result += threads_profile_storm_test<StringMap, WriteRemoveProfileFunctor>(
        (*opt_root_dir + THREAD_FRAGMENT_STORM_PATH).c_str(),
        50000, *opt_time);

      // repeat with reopen
      result += threads_profile_storm_test<StringMap, WriteRemoveProfileFunctor>(
        (*opt_root_dir + THREAD_FRAGMENT_STORM_PATH).c_str(),
        50, *opt_time);
    }
    */

    /*
    {
      std::cout << "==== one profile testing ====" << std::endl;

      const std::string test_root = *opt_root_dir + THREAD_ONE_STORM_PATH;

      remove_and_create(test_root.c_str());

      result += threads_one_profile_storm_test<StringMap>(
        test_root.c_str(), *opt_time);

      // repeat with reopen
      result += threads_one_profile_storm_test<StringMap>(
        test_root.c_str(), *opt_time);

      if(!result)
      {
        remove_and_create(test_root.c_str());
      }
    }

    {
      std::cout << "==== one profile move testing ====" << std::endl;

      const std::string test_root = *opt_root_dir + THREAD_ONE_MOVE_STORM_PATH;

      remove_and_create(test_root.c_str());

      result += threads_one_profile_move_storm_test<StringMap>(
        test_root.c_str(), *opt_time);

      // repeat with reopen
      result += threads_one_profile_move_storm_test<StringMap>(
        test_root.c_str(), *opt_time);

      /// *
      if(!result)
      {
        remove_and_create(test_root.c_str());
      }
    }

    {
      std::cout << "==== few profiles testing ====" << std::endl;

      remove_and_create(
        (*opt_root_dir + THREAD_STORM_CHECK_PATH).c_str());

      result += threads_profile_storm_test<StringMap, WriteReadProfileFunctor>(
        (*opt_root_dir + THREAD_STORM_CHECK_PATH).c_str(), 100, *opt_time);

      // repeat with reopen
      result += threads_profile_storm_test<StringMap, WriteReadProfileFunctor>(
        (*opt_root_dir + THREAD_STORM_CHECK_PATH).c_str(), 100, *opt_time);
    }

    {
      std::cout << "==== one profile testing (few files) ====" << std::endl;

      remove_and_create(
        (*opt_root_dir + THREAD_TIMED_STORM_ONE_PATH).c_str());

      result += threads_profile_storm_timed_test<StringMap>(
        (*opt_root_dir + THREAD_TIMED_STORM_ONE_PATH).c_str(),
        1, *opt_time);

      // repeat with reopen
      result += threads_profile_storm_timed_test<StringMap>(
        (*opt_root_dir + THREAD_TIMED_STORM_ONE_PATH).c_str(),
        1, *opt_time);
    }

    {
      std::cout << "==== few profiles testing (few files) ====" << std::endl;

      remove_and_create(
        (*opt_root_dir + THREAD_TIMED_STORM_FEW_PATH).c_str());

      result += threads_profile_storm_timed_test<StringMap>(
        (*opt_root_dir + THREAD_TIMED_STORM_FEW_PATH).c_str(),
        1000, *opt_time);

      // repeat with reopen
      result += threads_profile_storm_timed_test<StringMap>(
        (*opt_root_dir + THREAD_TIMED_STORM_FEW_PATH).c_str(),
        1000, *opt_time);
    }
    */

    return result;
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
