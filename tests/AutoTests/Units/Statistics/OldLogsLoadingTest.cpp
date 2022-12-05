#include "OldLogsLoadingTest.hpp"
 
REFLECT_UNIT(OldLogsLoadingTest) (
  "Statistics",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::ORM::HourlyStats HourlyStats;
  namespace ORM = ::AutoTest::ORM;
}

bool 
OldLogsLoadingTest::run_test()
{
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      get_global_params().LogGeneralizerIn()),
    "need LogGeneralizerIn item of type PathType in config file");

  std::string login = get_global_params().LogGeneralizerIn()->path();

  add_descr_phrase("Initializing DB connection");
  AutoTest::DBC::Conn conn(open_pq());

  add_descr_phrase("CreativeStat v 3.0");
  case_CreativeStat(conn, login, 1);

  return true;
}

void
OldLogsLoadingTest::case_CreativeStat(
  AutoTest::DBC::Conn& conn, 
  const std::string& login,
  int num)
{
  std::string n = strof(num);
  std::string dst = login + "CreativeStat/";
  
  std::string file = fetch_string("CreativeStatFile" + n);
  std::string commit_file = fetch_string("CreativeStatCommitFile" + n);
  std::string date = fetch_string("CreativeStatDate" + n);
  const unsigned long cc_id = fetch_int("CreativeStatCC" + n);
  const unsigned long numshown = fetch_int("CreativeStatNumShown" + n);
  const unsigned long imps = fetch_int("CreativeStatImps" + n);
  
  HourlyStats stats;
  stats.key().cc_id(cc_id).num_shown(numshown).stimestamp(date);
  stats.select(conn);
  AutoTest::CopyCmd(file, dst).exec();
  AutoTest::CopyCmd(commit_file, dst).exec();

  HourlyStats::Diffs diff;
  diff.imps(imps);
  FAIL_CONTEXT(
    AutoTest::wait_checker(
      AutoTest::stats_diff_checker(
        conn, diff, stats)).check(),
    "must got expected values for case_CreativeStat");
}

