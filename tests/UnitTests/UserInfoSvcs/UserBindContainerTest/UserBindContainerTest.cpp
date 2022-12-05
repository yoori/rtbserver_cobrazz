#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <list>
#include <sstream>
#include "sys/types.h"
#include "sys/sysinfo.h"

#include <Generics/AppUtils.hpp>
#include <TestCommons/MTTester.hpp>
#include <Commons/AtomicInt.hpp>
#include <ProfilingCommons/ProfileMap/ProfileMapFactory.hpp>
#include <UserInfoSvcs/UserBindServer/UserBindContainer.hpp>
#include <UserInfoSvcs/UserBindServer/FetchableHashTable.hpp>

using namespace AdServer::UserInfoSvcs;

namespace
{
  const std::string ROOT_PATH("./UserBind1/");
  const char TIME_FORMAT[] = "%d-%m-%Y:%H-%M-%S.%q";
}

struct TestUserInfo
{
  bool user_id_is_null;
  bool min_age_reached;
  bool user_id_generated;
  bool created;
};

bool
check_user_info(
  const TestUserInfo& test_user_info,
  const UserBindContainer::UserInfo& user_info)
{
  if (test_user_info.user_id_is_null == user_info.user_id.is_null() &&
      test_user_info.min_age_reached == user_info.min_age_reached &&
      test_user_info.user_id_generated == user_info.user_id_generated &&
      test_user_info.created == user_info.created)
  {
    return true;
  }

  std::cerr << "Unexpected result in UserInfo. Expected: "
   << "user_id: " << (test_user_info.user_id_is_null ? "is null" : "is not null")
   << ", min_age_reached: " << test_user_info.min_age_reached
   << ", user_id_generated: " << test_user_info.user_id_generated
   << ", created: " << test_user_info.created
   << ". But got: ";
   user_info.print(std::cerr);
 std::cerr << std::endl;

  return false;
}

void process_mem_usage(double& vm_usage, double& resident_set)
{
   using std::ios_base;
   using std::ifstream;
   using std::string;

   vm_usage     = 0.0;
   resident_set = 0.0;

   // 'file' stat seems to give the most reliable results
   //
   ifstream stat_stream("/proc/self/stat",ios_base::in);

   // dummy vars for leading entries in stat that we don't care about
   //
   string pid, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string O, itrealvalue, starttime;

   // the two fields we want
   //
   unsigned long vsize;
   long rss;

   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
   vm_usage     = vsize / 1024.0;
   resident_set = rss * page_size_kb;
}

UserBindContainer::ChunkPathMap
make_chunk_path_map(bool remove = true)
{
  if (remove)
  {
    system(("rm -fr " + ROOT_PATH).c_str());
    const std::size_t CHUNK_COUNT = 10;

    for (std::size_t i = 0; i < CHUNK_COUNT; ++i)
    {
      std::ostringstream oss;
      oss << ROOT_PATH << "Chunk_" << i << '_' << CHUNK_COUNT;
      system(("mkdir -p " + oss.str()).c_str());
    }
  }

  UserBindContainer::ChunkPathMap chunks;
  AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
    chunks, ROOT_PATH.c_str(), "Chunk");
  return chunks;
}

UserBindContainer_var
get_default_user_bind_container(bool remove_chunks_files)
{
  return new UserBindContainer(
    nullptr,                   //logger
    10,                        //common_chunks_number
    make_chunk_path_map(remove_chunks_files), //chunk_folders
    "UserSeen",                //file_prefix
    "UserBind",                //bound_file_prefix
    Generics::Time::ONE_DAY,   //extend_time_period = 4 days
    Generics::Time::ONE_DAY*2, //bound_extend_time_period = 8 days
    Generics::Time::ONE_DAY*4, //min_bind_age=4 days
    true,                      //bind_at_min_age
    100, // max_bad_event
    1024,                     //portions_number
    true, // load_slave
    0,
    1);
}

int
performance_test(unsigned long iterations)
{
  static const char* FUN = "performance_test()";

  UserBindContainer_var user_bind_container =
    new UserBindContainer(
      0,
      10,
      make_chunk_path_map(),
      "",
      "",
      Generics::Time::ONE_DAY,
      Generics::Time::ONE_DAY * 7,
      Generics::Time::ZERO,
      false,
      100, // max_bad_event
      1024,
      true,
      0,
      1);

  char buf[100];

  for(int global_i = 0; global_i < 2; ++global_i)
  {
    Generics::Time start_time = Generics::Time::get_time_of_day();

    for(unsigned long i = 0; i < iterations; ++i)
    {
      size_t size = String::StringManip::int_to_str(i, buf, sizeof(buf));

      user_bind_container->get_user_id(
        String::SubString(buf, size),
        AdServer::Commons::UserId(),
        start_time,
        false,
        Generics::Time::ZERO,
        false);
    }

    Generics::Time finish_time = Generics::Time::get_time_of_day();

    std::cout << FUN << ": execution time = " <<
      (finish_time - start_time) << " for " << iterations <<
      " operations(" <<
      (global_i ? "exists user" : "new user") <<
      ")" << std::endl;
  }

  return 0;
}

class GetUserIdTester
{
public:
  GetUserIdTester(
    UserBindContainer* user_bind_container,
    unsigned long iterations)
    noexcept
    : user_bind_container_(ReferenceCounting::add_ref(user_bind_container)),
      iterations_(iterations),
      counter_(0)
  {}

  void
  operator ()() noexcept
  {
    Generics::Time start_time = Generics::Time::get_time_of_day();

    char buf[100];

//  while(true)
    for(unsigned long i = 0; i < iterations_; ++i)
    {
      int res = counter_.exchange_and_add(1);
      size_t size = String::StringManip::int_to_str(res, buf, sizeof(buf));
      user_bind_container_->get_user_id(
        String::SubString(buf, size),
        AdServer::Commons::UserId(),
        start_time,
        false,
        Generics::Time::ZERO,
        false);
    }

    Generics::Time finish_time = Generics::Time::get_time_of_day();

    SyncPolicy::WriteGuard lock(lock_);
    sum_time_ += finish_time - start_time;
    max_time_ = std::max(max_time_, finish_time - start_time);
  }

  Generics::Time
  sum_time() const noexcept
  {
    SyncPolicy::ReadGuard lock(lock_);
    return sum_time_;
  }

  Generics::Time
  max_time() const noexcept
  {
    SyncPolicy::ReadGuard lock(lock_);
    return max_time_;
  }

protected:
  typedef Sync::Policy::PosixThread SyncPolicy;

protected:
  UserBindContainer_var user_bind_container_;
  const unsigned long iterations_;

  Algs::AtomicInt counter_;

  mutable SyncPolicy::Mutex lock_;
  Generics::Time sum_time_;
  Generics::Time max_time_;
};

int
mt_performance_test(unsigned long iterations, unsigned long threads)
{
  static const char* FUN = "mt_performance_test()";

  UserBindContainer_var user_bind_container =
    new UserBindContainer(
      0,
      10,
      UserBindContainer::ChunkPathMap(),
      "",
      "",
      Generics::Time::ONE_DAY,
      Generics::Time::ONE_DAY * 7,
      Generics::Time::ZERO,
      false,
      100, // max_bad_event
      1024,
      true,
      0,
      1);

  for(int global_i = 0; global_i < 2; ++global_i)
  {
    GetUserIdTester test_op(
      user_bind_container,
      iterations / threads
      //(global_i ? 1000000000 : iterations) / threads
      );

    TestCommons::MTTester<GetUserIdTester&> mt_tester(
      test_op, threads);
    mt_tester.run(threads, 0, threads);

    std::cout << FUN <<
      ": sum time = " << test_op.sum_time() <<
      ", max time = " << test_op.max_time() <<
      " for " << iterations <<
      " operations(" <<
      (global_i ? "exists user" : "new user") <<
      ") at " << threads << " threads" <<
      std::endl;
  }

  return 0;
}

int simple_test()
{
  static const char* FUN = "simple_test()";

  UserBindContainer_var user_bind_container =
    new UserBindContainer(
      0,
      10,
      UserBindContainer::ChunkPathMap(),
      "",
      "",
      Generics::Time::ONE_DAY,
      Generics::Time::ONE_DAY * 7,
      Generics::Time::ONE_HOUR,
      true, // optin when min age reached
      100, // max_bad_event
      1024,
      true,
      0,
      1);

  Generics::Time base_time(String::SubString("2008-01-01"), "%Y-%m-%d");
  std::string test_external_id("x");

  UserBindContainer::UserInfo user_info = user_bind_container->get_user_id(
    test_external_id,
    AdServer::Commons::UserId(),
    base_time,
    false,
    Generics::Time::ZERO,
    false);

  if(!user_info.user_id.is_null() || user_info.min_age_reached)
  {
    std::cerr << FUN << ": invalid result at first get" << std::endl;
    return 1;
  }

  user_info = user_bind_container->get_user_id(
    test_external_id,
    AdServer::Commons::UserId(),
    base_time + Generics::Time::ONE_HOUR - 2,
    false,
    Generics::Time::ZERO,
    false);

  if(!user_info.user_id.is_null() || user_info.min_age_reached)
  {
    std::cerr << FUN << ": invalid result at second get" << std::endl;
    return 1;
  }

  user_info = user_bind_container->get_user_id(
    test_external_id,
    AdServer::Commons::UserId(),
    base_time + Generics::Time::ONE_HOUR + 1,
    false,
    Generics::Time::ZERO,
    false);

  if(user_info.user_id.is_null() || !user_info.min_age_reached)
  {
    std::cerr << FUN << ": invalid result at third get: "
      "user_id = " << user_info.user_id <<
      ", min_age_reached = " << user_info.min_age_reached <<
      std::endl;
    return 1;
  }

  for(int i = 0; i < 10; ++i)
  {
    user_info = user_bind_container->get_user_id(
      test_external_id,
      AdServer::Commons::UserId(),
      base_time + Generics::Time::ONE_DAY * (2 + i) + 1,
      false,
      Generics::Time::ZERO,
      false);

    if(user_info.user_id.is_null() || !user_info.min_age_reached)
    {
      std::cerr << FUN << ": invalid result at get #" << (i + 4) << ": "
        "user_id = " << user_info.user_id <<
        ", min_age_reached = " << user_info.min_age_reached <<
        std::endl;
      return 1;
    }
  }

  return 0;
}

int expired_test()
{
  static const char* FUN = "expired_test()";

  UserBindContainer_var user_bind_container =
    new UserBindContainer(
      0,
      10,
      UserBindContainer::ChunkPathMap(),
      "",
      "",
      Generics::Time::ONE_DAY,
      Generics::Time::ONE_DAY * 7,
      Generics::Time::ONE_HOUR,
      true, // optin when min age reached
      100, // max_bad_event
      1024,
      true,
      0,
      1);

  Generics::Time base_time(String::SubString("2008-01-01"), "%Y-%m-%d");
  std::string test_external_id("x");

  UserBindContainer::UserInfo user_info = user_bind_container->get_user_id(
    test_external_id,
    AdServer::Commons::UserId(),
    base_time,
    false,
    Generics::Time::ZERO,
    false);

  if(!user_info.user_id.is_null() || user_info.min_age_reached)
  {
    std::cerr << FUN << ": invalid result at first get" << std::endl;
    return 1;
  }

  user_bind_container->clear_expired(
    Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"),
    Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"));

  user_info = user_bind_container->get_user_id(
    test_external_id,
    AdServer::Commons::UserId(),
    base_time + Generics::Time::ONE_DAY + 1,
    false,
    Generics::Time::ZERO,
    false);

  if(!user_info.user_id.is_null() || user_info.min_age_reached)
  {
    std::cerr << FUN << ": invalid result at second get"
      "(after clear expired)" << std::endl;
    return 1;
  }

  return 0;
}

int
mem_usage_test()
{
  UserBindContainer_var user_bind_container =
    new UserBindContainer(
      nullptr,                  //logger
      10,                       //common_chunks_number
      make_chunk_path_map(),    //chunk_folders
      "UserSeen",               //file_prefix
      "UserBind",               //bound_file_prefix
      Generics::Time::ONE_DAY,  //extend_time_period = 4 days
      Generics::Time::ONE_DAY,  //bound_extend_time_period = 4 days
      Generics::Time::ONE_HOUR, //min_bind_age
      true,                     //bind_at_min_age
      100, // max_bad_event
      1024,                    //portions_number
      true,
      0,
      1);

  Generics::Time base_date = Generics::Time::get_time_of_day();

  double virt_mem_before, rss_before;
  process_mem_usage(virt_mem_before, rss_before);
  int j = 1;

  for (int i = 0; i < 100000000; ++i)
  {
    std::ostringstream external_id;
    external_id << "external_id_" << i;
    user_bind_container->get_user_id(
      external_id.str(),
      AdServer::Commons::UserId(),
      base_date,
      false,
      Generics::Time::ZERO,
      false);

    if (i == j - 1)
    {
      double virt_mem_after, rss_after;
      process_mem_usage(virt_mem_after, rss_after);

      std::cout << "Users: " << j << ", Virtual memory size difference: " << (virt_mem_after - virt_mem_before)
        << " kB, Resident Set Size difference: " << (rss_after - rss_before) << " kB" << std::endl;

      j *= 10;
    }
  }

  return 0;
}

int
get_get_user_id_test()
{
  static const size_t COUNT = 9;
  static const TestUserInfo expected[COUNT] = {
    {true, false, false, true},
    {true, false, false, false},
    {true, false, false, false},
    {true, false, false, false},
    {true, false, false, false},
    {false, true, true, false},
    {false, true, false, false},
    {false, true, false, false},
    {false, true, false, false}
  };

  UserBindContainer_var user_bind_container =
    get_default_user_bind_container(true);

  const Generics::Time base_date(Generics::Time::get_time_of_day().get_gm_time().get_date());
  const std::string external_id = "external_id";

  UserBindContainer::UserInfo user_info = user_bind_container->get_user_id(
    external_id,
    AdServer::Commons::UserId(),
    base_date,
    false,
    Generics::Time::ZERO,
    false);

  if (!check_user_info(expected[0], user_info))
  {
    return 1;
  }

  for (size_t i = 1; i < COUNT; ++i)
  {
    user_info = user_bind_container->get_user_id(
      external_id,
      AdServer::Commons::UserId(),
      base_date + Generics::Time::ONE_DAY * (i - 1) + Generics::Time::ONE_SECOND,
      false,
      Generics::Time::ZERO,
      false);

    if (!check_user_info(expected[i], user_info))
    {
      return 1;
    }
  }

  return 0;
}

int
save_load_users_test()
{
  static const TestUserInfo expected[] = {
    {true, false, false, true},
    {true, false, false, false},
    {true, false, false, false},
    {false, true, true, false},
    {false, true, false, false} };

  const Generics::Time base_date(Generics::Time::get_time_of_day().get_gm_time().get_date());
  const std::string external_id = "external_id";
  size_t index = 0;

  {
    UserBindContainer_var user_bind_container =
      get_default_user_bind_container(true);

    UserBindContainer::UserInfo user_info = user_bind_container->get_user_id(
      external_id,
      AdServer::Commons::UserId(),
      base_date,
      false,
      Generics::Time::ZERO,
      false);

    if (!check_user_info(expected[index++], user_info))
    {
      return 1;
    }

    user_info = user_bind_container->get_user_id(
      external_id,
      AdServer::Commons::UserId(),
      base_date + Generics::Time::ONE_DAY + Generics::Time::ONE_SECOND,
      false,
      Generics::Time::ZERO,
      false);

    if (!check_user_info(expected[index++], user_info))
    {
      return 1;
    }

    user_bind_container->dump();
  }

  {
    UserBindContainer_var user_bind_container =
      get_default_user_bind_container(false);

    UserBindContainer::UserInfo user_info = user_bind_container->get_user_id(
      external_id,
      AdServer::Commons::UserId(),
      base_date + Generics::Time::ONE_DAY + Generics::Time::ONE_SECOND,
      false,
      Generics::Time::ZERO,
      false);

    if (!check_user_info(expected[index++], user_info))
    {
      return 1;
    }

    user_info = user_bind_container->get_user_id(
      external_id,
      AdServer::Commons::UserId(),
      base_date + Generics::Time::ONE_DAY * 4 + Generics::Time::ONE_SECOND,
      false,
      Generics::Time::ZERO,
      false);

    if (!check_user_info(expected[index++], user_info))
    {
      return 1;
    }

    user_bind_container->dump();
  }

  {
    UserBindContainer_var user_bind_container =
      get_default_user_bind_container(false);

    UserBindContainer::UserInfo user_info = user_bind_container->get_user_id(
      external_id,
      AdServer::Commons::UserId(),
      base_date + Generics::Time::ONE_DAY * 4 + Generics::Time::ONE_SECOND,
      false,
      Generics::Time::ZERO,
      false);

    if (!check_user_info(expected[index++], user_info))
    {
      return 1;
    }
  }

  return 0;
}

int
fetchable_hash_test()
{
  AdServer::UserInfoSvcs::bit_deque s;
  s.push_back(true);
  assert(s[0]);
  s.push_back(true);
  assert(s[0]);
  assert(s[1]);

  typedef AdServer::UserInfoSvcs::FetchableHashTable<
    Generics::NumericHashAdapter<int>,
    int,
    UnorderedSet> FetchableHashTableType;

  FetchableHashTableType f;

  f.set(1, 1);
  f.set(2, 2);
  int t = 0;
  bool ret = f.get(t, 1);
  assert(ret);
  assert(t == 1);
  f.erase(1);
  ret = f.get(t, 1);
  assert(!ret);
  ret = f.get(t, 2);
  assert(ret);
  assert(t == 2);

  ret = f.get(t, 0);
  assert(!ret);

  auto fetcher = f.fetcher();
  FetchableHashTableType::FetchArray fetch_array;
  while(true)
  {
    bool fin = !fetcher.get(fetch_array, 2);
    //std::cout << "fs = " << fetch_array.size() << std::endl;
    for(auto it = fetch_array.begin(); it != fetch_array.end(); ++it)
    {
      std::cout << it->first.value() << "=>" << it->second << std::endl;
    }

    if(fin)
    {
      break;
    }
  }
  return 0;
}

int main() noexcept
{
  const unsigned long ITERATIONS = 10000000;
  (void)ITERATIONS;
  
  int ret = 0;

  try
  {
    /*
    ret += simple_test();
    ret += expired_test();
    ret += mem_usage_test();
    */

    ret += fetchable_hash_test();
    /*
    ret += get_get_user_id_test();
    ret += save_load_users_test();
    ret += performance_test(ITERATIONS);
    */
    /*
    ret += mt_performance_test(ITERATIONS, 10);
    ret += mt_performance_test(ITERATIONS, 20);
    ret += mt_performance_test(ITERATIONS, 30);
    ret += mt_performance_test(ITERATIONS, 60);
    */
  }
  catch (eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    ret = 1;
  }

  return ret;
}
