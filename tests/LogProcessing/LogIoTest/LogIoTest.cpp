
#include <getopt.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

#include <String/SubString.hpp>
#include <Logger/Logger.hpp>

#include <LogCommons/ActionRequest.hpp>
#include <LogCommons/ActionStat.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <LogCommons/CampaignUserStat.hpp>
#include <LogCommons/CcUserStat.hpp>
#include <LogCommons/CcgKeywordStat.hpp>
#include <LogCommons/CcgSelectionFailureStat.hpp>
#include <LogCommons/CcgStat.hpp>
#include <LogCommons/CcgUserStat.hpp>
#include <LogCommons/ChannelCountStat.hpp>
#include <LogCommons/ChannelHitStat.hpp>
#include <LogCommons/ChannelImpInventory.hpp>
#include <LogCommons/ChannelInventory.hpp>
#include <LogCommons/ChannelInventoryEstimationStat.hpp>
#include <LogCommons/ChannelOverlapUserStat.hpp>
#include <LogCommons/ChannelPerformance.hpp>
#include <LogCommons/ChannelPriceRange.hpp>
#include <LogCommons/ChannelTriggerImpStat.hpp>
#include <LogCommons/ChannelTriggerStat.hpp>
#include <LogCommons/CmpStat.hpp>
#include <LogCommons/ColoUpdateStat.hpp>
#include <LogCommons/ColoUserStat.hpp>
#include <LogCommons/ColoUsers.hpp>
#include <LogCommons/CreativeStat.hpp>
#include <LogCommons/DeviceChannelCountStat.hpp>
#include <LogCommons/ExpressionPerformance.hpp>
#include <LogCommons/HistoryMatch.hpp>
#include <LogCommons/ImpNotify.hpp>
#include <LogCommons/PassbackStat.hpp>
#include <LogCommons/Request.hpp>
#include <LogCommons/TagAuctionStat.hpp>
#include <LogCommons/TagPositionStat.hpp>
#include <LogCommons/TagRequest.hpp>
#include <LogCommons/RequestBasicChannels.hpp>
#include <LogCommons/SearchEngineStat.hpp>
#include <LogCommons/SearchTermStat.hpp>
#include <LogCommons/SiteReferrerStat.hpp>
#include <LogCommons/UserAgentStat.hpp>
#include <LogCommons/UserProperties.hpp>
#include <LogCommons/WebStat.hpp>
#include <LogCommons/SiteStat.hpp>
#include <LogCommons/SiteUserStat.hpp>
#include <LogCommons/PageLoadsDailyStat.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>
#include <LogProcessing/LogGeneralizer/LogTypeCsvTraits.hpp>
#include <LogCommons/GenericLogCsvSaverImpl.hpp>
#include <LogCommons/LogCommons.ipp>

#include <CampaignSvcs/CampaignCommons/CampaignTypes.hpp>

#define LOGIOTEST_PRODUCE_LARGER_FILES 0

using namespace AdServer::LogProcessing;

namespace
{
  const Generics::Time TEST_TIME(Generics::Time::get_time_of_day());
  const UserId TEST_USER_ID("PPPPPPPPPPPPPPPPPPPPPA..");
  template <typename ArrayType, size_t Count>
  const ArrayType*
  endof(const ArrayType (&array)[Count])
  {
    return &array[0] + Count;
  }
  const unsigned long NUMBERS_ARRAY[] =
  {
    111, 222, 333, 444, 555,
  };
  const unsigned long NUMBERS_MIXED_ARRAY[] =
  {
    444, 333, 111, 222, 555,
  };
  const NumberList SEARCH_CHANNELS(&NUMBERS_ARRAY[0], endof(NUMBERS_ARRAY));
  const NumberList PAGE_CHANNELS(
    std::reverse_iterator<const unsigned long*>(endof(NUMBERS_ARRAY)),
    std::reverse_iterator<const unsigned long*>(&NUMBERS_ARRAY[0]));
  const NumberList URL_CHANNELS(
    &NUMBERS_MIXED_ARRAY[0], endof(NUMBERS_MIXED_ARRAY));
  bool file_generator_mode = false;
}

#define HANDLE_EXCEPTIONS(VAR, VAL) \
  catch (const eh::Exception &ex) \
  { \
    std::cerr << "eh::Exception caught. : " \
              << ex.what() << std::endl; \
    VAR = VAL; \
  } \
  catch (...) \
  { \
    std::cerr << "Unknown exception caught." << std::endl; \
    VAR = VAL; \
  }

template <
  class TRAITS_,
  bool nested = TRAITS_::is_nested,
  bool summable = TRAITS_::is_summable
>
struct EqualityTester
{
  bool
  operator()(
    const typename TRAITS_::CollectorType &collector1,
    const typename TRAITS_::CollectorType &collector2
  );
};

template <class TRAITS_>
struct EqualityTester<TRAITS_, false, false>
{
  bool
  operator()(
    const typename TRAITS_::CollectorType &collector1,
    const typename TRAITS_::CollectorType &collector2
  )
  {
    typedef typename TRAITS_::CollectorType CollectorT;
    if (collector1.size() != collector2.size())
    {
      return false;
    }
    for (typename CollectorT::const_iterator it = collector1.begin();
      it != collector1.end(); ++it)
    {
      typename CollectorT::const_iterator find_it =
        std::find(collector2.begin(), collector2.end(), *it);
      if (find_it == collector2.end())
      {
        return false;
      }
    }
    return true;
  }
};

template <class TRAITS_>
struct EqualityTester<TRAITS_, false, true>
{
  bool
  operator()(
    const typename TRAITS_::CollectorType &collector1,
    const typename TRAITS_::CollectorType &collector2
  )
  {
    typedef typename TRAITS_::CollectorType CollectorT;
    if (collector1.size() != collector2.size())
    {
      return false;
    }
    for (typename CollectorT::const_iterator it = collector1.begin();
      it != collector1.end(); ++it)
    {
      typename CollectorT::const_iterator find_it =
        std::find(collector2.begin(), collector2.end(), *it);
      if (find_it == collector2.end() || !(find_it->second == it->second))
      {
        return false;
      }
    }
    return true;
  }
};

template <class TRAITS_>
struct EqualityTester<TRAITS_, true, false>
{
  bool
  operator()(
    const typename TRAITS_::CollectorType &collector1,
    const typename TRAITS_::CollectorType &collector2
  )
  {
    typedef typename TRAITS_::CollectorType CollectorT;
    if (collector1.size() != collector2.size())
    {
      return false;
    }
    for (typename CollectorT::const_iterator it = collector1.begin();
      it != collector1.end(); ++it)
    {
      typename CollectorT::const_iterator fit = collector2.find(it->first);
      if (fit == collector2.end())
      {
        return false;
      }
      if (it->second.size() != fit->second.size())
      {
        return false;
      }
      for (typename CollectorT::DataT::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      {
        typename CollectorT::DataT::const_iterator fiit =
          std::find(fit->second.begin(), fit->second.end(), *iit);
        if (fiit == fit->second.end())
        {
          return false;
        }
      }
    }
    return true;
  }
};

template <class TRAITS_>
struct EqualityTester<TRAITS_, true, true>
{
  bool
  operator()(
    const typename TRAITS_::CollectorType &collector1,
    const typename TRAITS_::CollectorType &collector2
  )
  {
    typedef typename TRAITS_::CollectorType CollectorT;
    if (collector1.size() != collector2.size())
    {
      return false;
    }
    for (typename CollectorT::const_iterator it = collector1.begin();
      it != collector1.end(); ++it)
    {
      typename CollectorT::const_iterator fit = collector2.find(it->first);
      if (fit == collector2.end())
      {
        return false;
      }
      if (it->second.size() != fit->second.size())
      {
        return false;
      }
      for (typename CollectorT::DataT::const_iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      {
        typename CollectorT::DataT::const_iterator fiit =
          fit->second.find(iit->first);
        if (fiit == fit->second.end() || !(fiit->second == iit->second))
        {
          return false;
        }
      }
    }
    return true;
  }
};

template <class LOG_TYPE_TRAITS_> class LogIoTester
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef typename LOG_TYPE_TRAITS_::CollectorType CollectorT;

  LogIoTester(bool dump_on_fail = false): dump_on_fail_(dump_on_fail) {}

  virtual ~LogIoTester() {}

  void test(CollectorT &collector) /*throw(Exception, eh::Exception)*/
  {
    if (file_generator_mode)
    {
      std::cout << "Generating " << std::setw(40) << std::setfill('.') <<
        std::left << Traits::log_base_name() << std::endl;
    }
    else
    {
      std::cout << "Testing " << std::setw(40) << std::setfill('.') <<
        std::left << Traits::log_base_name();
    }

    const std::string dir_name = std::string(".") + Traits::log_base_name();
    const std::string rm_cmd = std::string("rm -rf ") + dir_name;
    const std::string mkdir_cmd = std::string("mkdir -p ") + dir_name;

    system(rm_cmd.c_str());
    system(mkdir_cmd.c_str());

    CollectorT tmp_collector;
    collector.deep_copy(tmp_collector);
    LogIoProxy<Traits>::save(tmp_collector, dir_name);

    if (file_generator_mode)
    {
      return;
    }

    LogSortingMap log_sorting_map;
    Logging::Logger_var logger(new Logging::Null::Logger);
    search_for_files(dir_name, Traits::log_base_name(),
      log_sorting_map, -1, logger);

    CollectorT restored_collector;

    for (LogSortingMap::const_iterator it = log_sorting_map.begin();
      it != log_sorting_map.end(); ++it)
    {
      std::ifstream ifs(it->second.c_str());
      typename Traits::HeaderType log_header;
      if (!(ifs >> log_header))
      {
        Stream::Error es;
        es << "LogIoTester<" << Traits::log_base_name()
           << ">: Failed to read log header";
        throw Exception(es);
      }
      if (log_header.version() != Traits::current_version())
      {
        Stream::Error es;
        es << "LogIoTester<" << Traits::log_base_name()
           << ">: Invalid log header version: "
           << log_header.version();
        throw Exception(es);
      }
      LogIoProxy<Traits>::load(restored_collector, ifs);
    }

    if (EqualityTester<LOG_TYPE_TRAITS_>()(collector, restored_collector))
    {
      system(rm_cmd.c_str());
      std::cout << "SUCCEEDED" << std::endl;
      return;
    }
    else
    {
      if (dump_on_fail_)
      {
        const std::string orig_dir_name =
          std::string(Traits::log_base_name()) + "_Original";
        system((std::string("mkdir -p ") + orig_dir_name).c_str());
        std::ostringstream mv_orig_dir_cmd_oss;
        mv_orig_dir_cmd_oss << "mv " << dir_name << "/* " <<
          orig_dir_name << "; rmdir " << dir_name << std::endl;
        system(mv_orig_dir_cmd_oss.str().c_str());

        const std::string dump_dir_name =
          std::string(Traits::log_base_name()) + "_Restored";
        system((std::string("mkdir -p ") + dump_dir_name).c_str());

        LogIoProxy<Traits>::save(restored_collector, dump_dir_name);
      }
      else
      {
        system(rm_cmd.c_str());
      }

      std::cout << "FAILED" << std::endl;

      Stream::Error es;
      es << "LogIoTester<" << Traits::log_base_name()
         << ">: Restored collector is not equal "
         << "to the original collector";
      throw Exception(es);
    }
  }

private:
  typedef LOG_TYPE_TRAITS_ Traits;

  const bool dump_on_fail_;
};

const char usage_info[] = \
  "LogIoTest - A test application for log input/output code\n\n"
  "Usage: LogIoTest [-d|-g|-h]\n"
  "Options:\n"
  "    -d  Any failed test will dump collector contents to disk\n"
  "    -g  Generate test log files and exit\n"
  "    -h  Print this information\n";

int main(int argc, char **argv)
{
  int exitcode = 0;
  bool dump_on_fail = false;

  const char opts[] = "dgh";

  while (1)
  {
    int opt = getopt(argc, argv, opts);
    if (opt == -1)
    {
      break;
    }
    switch (opt)
    {
      case 'd':
        dump_on_fail = true;
        break;
      case 'g':
        file_generator_mode = true;
        break;
      case 'h':
        std::cout << usage_info << std::endl;
        return 0;
    }
  }

#if LOGIOTEST_PRODUCE_LARGER_FILES
  const unsigned max_iterations = 100000;
#endif

  try
  {
    CcgKeywordStatCollector collector;
    CcgKeywordStatCollector::KeyT key1(TEST_TIME, 123);
    CcgKeywordStatCollector::KeyT key2(TEST_TIME, 321);
    CcgKeywordStatCollector::DataT data;
    typedef CcgKeywordStatCollector::DataT::DataT::FixedNum FixedNum;
    CcgKeywordStatCollector::DataT::DataT
      inner_data(111, 222, FixedNum("200.98765"),
        FixedNum("300.98765"), FixedNum("400.98765"));
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef CcgKeywordStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, max_iterations - i - 1, i + 2), inner_data);
    }
#else
    CcgKeywordStatCollector::DataT::KeyT inner_key(127, 721, 217);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<CcgKeywordStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    CcgSelectionFailureStatCollector collector;
    CcgSelectionFailureStatCollector::KeyT key1(TEST_TIME);
    CcgSelectionFailureStatCollector::KeyT key2(TEST_TIME);
    CcgSelectionFailureStatCollector::DataT data;
    CcgSelectionFailureStatCollector::DataT::DataT inner_data(555);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef CcgSelectionFailureStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, i + 2), inner_data);
    }
#else
    CcgSelectionFailureStatCollector::DataT::KeyT inner_key(789, 987);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<CcgSelectionFailureStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    CcgStatCollector collector;
    CcgStatCollector::KeyT key1(TEST_TIME, 21);
    CcgStatCollector::KeyT key2(TEST_TIME, 22);
    CcgStatCollector::DataT data;
    CcgStatCollector::DataT::DataT inner_data(123);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef CcgStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1), inner_data);
    }
#else
    CcgStatCollector::DataT::KeyT inner_key(789);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<CcgStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    CcStatCollector collector;
    CcStatCollector::KeyT key1(TEST_TIME, 21);
    CcStatCollector::KeyT key2(TEST_TIME, 22);
    CcStatCollector::DataT data;
    CcStatCollector::DataT::DataT inner_data(2011);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef CcStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1), inner_data);
    }
#else
    CcStatCollector::DataT::KeyT inner_key(789);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<CcStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    CampaignUserStatCollector collector;
    CampaignUserStatCollector::KeyT key1(TEST_TIME, 21);
    CampaignUserStatCollector::KeyT key2(TEST_TIME, 22);
    CampaignUserStatCollector::DataT data;
    CampaignUserStatCollector::DataT::DataT inner_data(1999);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef CampaignUserStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, TEST_TIME), inner_data);
    }
#else
    CampaignUserStatCollector::DataT::KeyT inner_key(789, TEST_TIME);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<CampaignUserStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    CcgUserStatCollector collector;
    CcgUserStatCollector::KeyT key1(TEST_TIME, 21);
    CcgUserStatCollector::KeyT key2(TEST_TIME, 22);
    CcgUserStatCollector::DataT data;
    CcgUserStatCollector::DataT::DataT inner_data(1999);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef CcgUserStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, TEST_TIME), inner_data);
    }
#else
    CcgUserStatCollector::DataT::KeyT inner_key(789, TEST_TIME);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<CcgUserStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    CcUserStatCollector collector;
    CcUserStatCollector::KeyT key1(TEST_TIME, 21);
    CcUserStatCollector::KeyT key2(TEST_TIME, 22);
    CcUserStatCollector::DataT data;
    CcUserStatCollector::DataT::DataT inner_data(1999);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef CcUserStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, TEST_TIME), inner_data);
    }
#else
    CcUserStatCollector::DataT::KeyT inner_key(789, TEST_TIME);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<CcUserStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    AdvertiserUserStatCollector collector;
    AdvertiserUserStatCollector::KeyT key1(TEST_TIME);
    AdvertiserUserStatCollector::KeyT key2(TEST_TIME);
    AdvertiserUserStatCollector::DataT data;
    AdvertiserUserStatCollector::DataT::DataT inner_data(111, 222, 333);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef AdvertiserUserStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, TEST_TIME), inner_data);
    }
#else
    AdvertiserUserStatCollector::DataT::KeyT inner_key(789, TEST_TIME);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<AdvertiserUserStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ChannelInventoryCollector collector;
    ChannelInventoryCollector::KeyT key1(TEST_TIME, 21);
    ChannelInventoryCollector::KeyT key2(TEST_TIME, 22);
    ChannelInventoryCollector::DataT data;
    typedef ChannelInventoryCollector::DataT::DataT::FixedNum FixedNum;
    ChannelInventoryCollector::DataT::DataT
      inner_data(FixedNum("120.8901"), FixedNum("99"), FixedNum("43.252"));
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ChannelInventoryCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(max_iterations - i - 1), inner_data);
    }
#else
    ChannelInventoryCollector::DataT::KeyT inner_key(999);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ChannelInventoryTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ChannelImpInventoryCollector collector;
    ChannelImpInventoryCollector::KeyT key1(TEST_TIME, 21);
    ChannelImpInventoryCollector::KeyT key2(TEST_TIME, 22);
    ChannelImpInventoryCollector::DataT data;
    ChannelImpInventoryCollector::DataT::DataT::Counter
      counter1, counter2, counter3;
    typedef ChannelImpInventoryCollector::DataT::DataT::FixedNum FixedNum;
    counter1.imps = FixedNum("100.00001");
    counter1.user_count = FixedNum("100.00011");
    counter1.value = FixedNum("100.00111");
    counter2.imps = FixedNum("200.00002");
    counter2.user_count = FixedNum("200.00022");
    counter2.value = FixedNum("200.00222");
    counter3.imps = FixedNum("300.00003");
    counter3.user_count = FixedNum("300.00033");
    counter3.value = FixedNum("300.00333");
    ChannelImpInventoryCollector::DataT::DataT
      inner_data(99, 999, FixedNum("120.89017612"), FixedNum("43.252"),
        counter1, counter2, counter3);
    typedef ChannelImpInventoryInnerKey::CCGType CCGType;
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ChannelImpInventoryCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(max_iterations - i - 1, CCGType::TEXT), inner_data);
    }
#else
    ChannelImpInventoryCollector::DataT::KeyT inner_key(999, CCGType::TEXT);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ChannelImpInventoryTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ChannelPerformanceCollector collector;
    ChannelPerformanceCollector::KeyT key1(TEST_TIME, 21);
    ChannelPerformanceCollector::KeyT key2(TEST_TIME, 22);
    ChannelPerformanceCollector::DataT data;
    typedef ChannelPerformanceCollector::DataT::DataT::FixedNum FixedNum;
    ChannelPerformanceCollector::DataT::DataT
      inner_data(111, 99, 88, 77, FixedNum("66"));
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ChannelPerformanceCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, i + 10, "100x10"), inner_data);
    }
#else
    ChannelPerformanceCollector::DataT::KeyT inner_key1(999, 888,
      AdServer::Commons::ImmutableString("100x10 "));
    ChannelPerformanceCollector::DataT::KeyT inner_key2(999, 888,
      AdServer::Commons::ImmutableString());
    data.add(inner_key1, inner_data);
    data.add(inner_key2, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ChannelPerformanceTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ChannelPriceRangeCollector collector;
    ChannelPriceRangeCollector::KeyT key1(TEST_TIME);
    ChannelPriceRangeCollector::KeyT key2(TEST_TIME);
    ChannelPriceRangeCollector::DataT data;
    ChannelPriceRangeCollector::DataT::DataT inner_data(120.8901, 1234);
    typedef ChannelPriceRangeCollector::DataT::KeyT KeyType;
#if LOGIOTEST_PRODUCE_LARGER_FILES
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType("popup", "ru", i + 1, KeyType::EcpmT(8.342), 999),
        inner_data);
    }
#else
    ChannelPriceRangeCollector::DataT::KeyT
      inner_key(AdServer::Commons::ImmutableString("popup"),
        AdServer::Commons::ImmutableString("ru"),
        121, KeyType::EcpmT(8.342), 999);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ChannelPriceRangeTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ChannelTriggerImpStatCollector collector;
    ChannelTriggerImpStatCollector::KeyT key1(TEST_TIME, 123);
    ChannelTriggerImpStatCollector::KeyT key2(TEST_TIME, 321);
    ChannelTriggerImpStatCollector::DataT data;
    typedef ChannelTriggerImpStatCollector::DataT::DataT::FixedNum FixedNum;
    ChannelTriggerImpStatCollector::DataT::DataT
      inner_data(FixedNum("777"), FixedNum("888"));
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ChannelTriggerImpStatCollector::DataT::KeyT KeyType;
    char types[] = { 'P', 'S', 'U' };
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, i + 100, types[i % sizeof(types)]), inner_data);
    }
#else
    ChannelTriggerImpStatCollector::DataT::KeyT inner_key(133, 331, 'P');
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ChannelTriggerImpStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ChannelTriggerStatCollector collector;
    ChannelTriggerStatCollector::KeyT key1(TEST_TIME, 123);
    ChannelTriggerStatCollector::KeyT key2(TEST_TIME, 321);
    ChannelTriggerStatCollector::DataT data;
    ChannelTriggerStatCollector::DataT::DataT inner_data(777);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ChannelTriggerStatCollector::DataT::KeyT KeyType;
    char types[] = { 'P', 'S', 'U' };
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i, i + 100, types[i % sizeof(types)]), inner_data);
    }
#else
    ChannelTriggerStatCollector::DataT::KeyT inner_key(311, 131, 'P');
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ChannelTriggerStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ColoUserStatCollector collector;
    ColoUserStatCollector::KeyT key1(TEST_TIME, 21);
    ColoUserStatCollector::KeyT key2(TEST_TIME, 22);
    ColoUserStatCollector::DataT data;
    ColoUserStatCollector::DataT::DataT inner_data(1999, 9111, 1919, 9191);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ColoUserStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, TEST_TIME), inner_data);
    }
#else
    ColoUserStatCollector::DataT::KeyT inner_key(TEST_TIME, TEST_TIME);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ColoUserStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    GlobalColoUserStatCollector collector;
    GlobalColoUserStatCollector::KeyT key1(TEST_TIME, 21);
    GlobalColoUserStatCollector::KeyT key2(TEST_TIME, 22);
    GlobalColoUserStatCollector::DataT data;
    GlobalColoUserStatCollector::DataT::DataT
      inner_data(1999, 9111, 9191, 1919);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef GlobalColoUserStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, TEST_TIME), inner_data);
    }
#else
    GlobalColoUserStatCollector::DataT::KeyT inner_key(TEST_TIME, TEST_TIME);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<GlobalColoUserStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ColoUsersCollector collector;
    ColoUsersData data(111, 222, 333, 444, 555);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ColoUsersKey KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      collector.add(KeyType(
        TEST_TIME, OptionalDayTimestamp(), i + 1, max_iterations - i - 1),
        data);
    }
#else
    ColoUsersKey key(TEST_TIME, OptionalDayTimestamp(), 77, 88);
    collector.add(key, data);
#endif
    LogIoTester<ColoUsersTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    CreativeStatCollector collector;
    CreativeStatCollector::KeyT key1(TEST_TIME, TEST_TIME);
    CreativeStatCollector::KeyT key2(TEST_TIME - Generics::Time::ONE_HOUR,
      TEST_TIME - Generics::Time::ONE_HOUR);
    CreativeStatCollector::DataT data;
    typedef CreativeStatCollector::DataT::DataT::FixedNum FixedNum;
    typedef CreativeStatCollector::DataT::KeyT KeyType;
    typedef KeyType::DeliveryThresholdT DeliveryThresholdT;
    typedef KeyType::GeoChannelIdOptional GeoChannelIdOptional;
    CreativeStatCollector::DataT::DataT
      inner_data(10, 20, 30, 40, FixedNum("1"), FixedNum("2"),
        FixedNum("3"), FixedNum("4"), FixedNum("5"), FixedNum("6"),
          FixedNum("7"), FixedNum("8"));
#if LOGIOTEST_PRODUCE_LARGER_FILES
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, i + 10, 2, 3, "", i + 10, i + 20, i + 30, 1, 2,
        3, 4, 5, DeliveryThresholdT("0.99999"), 1, 1, false, false, false, 'U',
        GeoChannelIdOptional(), DeviceChannelIdOptional()), 111, false, 66
          inner_data);
    }
#else
    KeyType inner_key(1, 10, 2, 3, "", 10, 20, 30,
      1, 2, 3, 4, 5, DeliveryThresholdT("0.19999"), 1, 1,
        false, false, false, 'U', GeoChannelIdOptional(), 777, 111, true, 66);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<CreativeStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);


  try
  {
    ExpressionPerformanceCollector collector;
    ExpressionPerformanceCollector::KeyT key1(TEST_TIME, 123);
    ExpressionPerformanceCollector::KeyT key2(TEST_TIME, 321);
    ExpressionPerformanceCollector::DataT data;
    ExpressionPerformanceCollector::DataT::DataT inner_data(99, 88, 77);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ExpressionPerformanceCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, "x&y"), inner_data);
    }
#else
    ExpressionPerformanceCollector::DataT::KeyT inner_key(177, "x&y");
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ExpressionPerformanceTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    RequestBasicChannelsCollector collector;
    RequestBasicChannelsCollector::KeyT
      key1(
        Generics::Time(String::SubString("2007-11-28 12:13:14"),
          "%Y-%m-%d %H:%M:%S"),
        Generics::Time(String::SubString("2007-11-28 11:13:14"),
          "%Y-%m-%d %H:%M:%S"),
        21
      );
    RequestBasicChannelsCollector::KeyT
      key2(
        Generics::Time(String::SubString("2007-11-29 14:13:12"),
          "%Y-%m-%d %H:%M:%S"),
        Generics::Time(String::SubString("2007-11-29 13:13:12"),
          "%Y-%m-%d %H:%M:%S"),
        22
      );
    RequestBasicChannelsCollector::KeyT
      key3(
        Generics::Time(String::SubString("2006-11-29 14:13:12"),
          "%Y-%m-%d %H:%M:%S"),
        Generics::Time(String::SubString("2006-11-29 13:13:12"),
          "%Y-%m-%d %H:%M:%S"),
        22
      );
    RequestBasicChannelsCollector::DataT data;
    NumberArray history_channels;
    history_channels.push_back(100);
    history_channels.push_back(101);
    history_channels.push_back(102);
    NumberList impression_channels;
    impression_channels.push_back(300);
    impression_channels.push_back(301);
    impression_channels.push_back(302);

    RequestBasicChannelsCollector::DataT::DataT::TriggerMatchList
      page_trigger_channels, search_trigger_channels, url_trigger_channels,
      url_keyword_trigger_channels;

    RequestBasicChannelsCollector::DataT::DataT::TriggerMatch tr_match;
    tr_match.channel_id = 111;
    tr_match.channel_trigger_id = 112;
    page_trigger_channels.push_back(tr_match);
    tr_match.channel_id = 121;
    tr_match.channel_trigger_id = 122;
    page_trigger_channels.push_back(tr_match);
    tr_match.channel_id = 211;
    tr_match.channel_trigger_id = 212;
    search_trigger_channels.push_back(tr_match);
    tr_match.channel_id = 221;
    tr_match.channel_trigger_id = 222;
    search_trigger_channels.push_back(tr_match);
    tr_match.channel_id = 311;
    tr_match.channel_trigger_id = 312;
    url_trigger_channels.push_back(tr_match);
    tr_match.channel_id = 321;
    tr_match.channel_trigger_id = 322;
    url_trigger_channels.push_back(tr_match);
    tr_match.channel_id = 411;
    tr_match.channel_trigger_id = 412;
    url_keyword_trigger_channels.push_back(tr_match);
    tr_match.channel_id = 421;
    tr_match.channel_trigger_id = 422;
    url_keyword_trigger_channels.push_back(tr_match);

    RequestBasicChannelsCollector::DataT::DataT::AdBidSlotImpressionList
      text_ad_shown;

    typedef RequestBasicChannelsCollector::DataT::DataT::FixedNum FixedNum;

    RequestBasicChannelsCollector::DataT::DataT::AdSlotImpression
      ad_imp(FixedNum("100"), impression_channels);

    RequestBasicChannelsCollector::DataT::DataT::AdSlotImpressionOptional
      ad_imp_opt(ad_imp);

    RequestBasicChannelsCollector::DataT::DataT::AdBidSlotImpression
      ad_bid_imp(FixedNum("100"), FixedNum("200"), impression_channels);

    text_ad_shown.push_back(ad_bid_imp);
    text_ad_shown.push_back(ad_bid_imp);
    text_ad_shown.push_back(ad_bid_imp);

    std::list<unsigned long> full_freq_caps;
    full_freq_caps.push_back(1);
    full_freq_caps.push_back(2);
    full_freq_caps.push_back(3);

    RequestBasicChannelsCollector::DataT::DataT::AdSelectProps
    // tag_id, size, format, test_request, profiling_available, full_freq_caps
      ad_select(888, "SIZE 0", "format !@#$%,^$&*:.", 1, 21, full_freq_caps);

    StringList sizes = { "SIZE 1", "SIZE 2", "SIZE 3" };

    RequestBasicChannelsCollector::DataT::DataT::AdRequestProps
      ad_request(sizes, "RUS", 20, FixedNum("30"),
        ad_imp_opt, text_ad_shown, ad_select, AdServer::CampaignSvcs::AT_MAX_ECPM);

    RequestBasicChannelsCollector::DataT::DataT::AdRequestPropsOptional
      ad_request_opt;

    ad_request_opt = ad_request;

    RequestBasicChannelsCollector::DataT::DataT::Match match_request(
        history_channels,
        page_trigger_channels,
        search_trigger_channels,
        url_trigger_channels,
        url_keyword_trigger_channels
    );

    RequestBasicChannelsCollector::DataT::DataT
      inner_data1(
        'P',
        UserId("hSUsEk05T-m8PafRng8v6w.."),
//        UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
        UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
        RequestBasicChannelsCollector::DataT::DataT::MatchOptional(),
        RequestBasicChannelsCollector::DataT::DataT::AdRequestPropsOptional(ad_request_opt)
      );

    RequestBasicChannelsCollector::DataT::DataT
      inner_data2(
        'H',
//        UserId("hSUsEk05T-m8PafRng8v6w.."),
        UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
        UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
        match_request,
        RequestBasicChannelsCollector::DataT::DataT::AdRequestPropsOptional()
      );

    RequestBasicChannelsCollector::DataT::DataT
      inner_data3(
        'A',
        UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
        UserId("hSUsEk05T-m8PafRng8v6w.."),
        match_request,
        RequestBasicChannelsCollector::DataT::DataT::AdRequestPropsOptional(ad_request_opt)
      );

#if LOGIOTEST_PRODUCE_LARGER_FILES
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(inner_data1);
      data.add(inner_data2);
      data.add(inner_data3);
    }
#else
    data.add(inner_data1);
    data.add(inner_data2);
    data.add(inner_data3);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    collector.add(key3, data);
    LogIoTester<RequestBasicChannelsTraits>(dump_on_fail).test(collector);
#if 0
    RequestBasicChannelsCollector collector1 = collector,
      collector2 = collector;
    typedef LogIoProxy<RequestBasicChannelsTraits> LogIoProxyT;
    LogIoProxyT::save(collector, ".", 2);
    LogIoProxyT::save(collector1, ".", 5);
    LogIoProxyT::save(collector2, ".", 4);
#endif
#if 0
    typedef RequestBasicChannelsTraits::IoHelperType LogIoHelperT;
    RequestBasicChannelsCollector collector1;
    typedef GenericLogSaverImpl<RequestBasicChannelsTraits> SaverT;
    LogSortingMap lsm;
    search_for_files(".", RequestBasicChannelsTraits::log_base_name(), lsm);
    system("rm -rf ./OUT");
    system("mkdir -p ./OUT");
    if (lsm.empty())
    {
      std::cerr << "$$ lsm is empty!" << std::endl;
    }
    else
    {
      LogIoHelperT io_helper(collector1);
      for (LogSortingMap::const_iterator it = lsm.begin();
        it != lsm.end(); ++it)
      {
        std::cerr << "$$ it->second = " << it->second << std::endl;
        std::cerr << "$$ " << __LINE__ << std::endl;
        std::ifstream ifs(it->second.c_str());
        io_helper.load(ifs);
        std::cerr << "$$ " << __LINE__ << std::endl;
      }
      io_helper.save("./OUT/");
      std::cerr << "$$ " << __LINE__ << std::endl;
    }
#endif
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    SiteReferrerStatCollector collector;
    SiteReferrerStatCollector::KeyT key1(TEST_TIME, 21);
    SiteReferrerStatCollector::KeyT key2(TEST_TIME, 22);
    SiteReferrerStatCollector::DataT data;
    SiteReferrerStatCollector::DataT::DataT
      inner_data(333, 444, 555, 666, 777, 888, 999,
        FixedNumber("1.1"), FixedNumber("2.2"), FixedNumber("3.3"),
        FixedNumber("4.4"), FixedNumber("5.5"), FixedNumber("6.6"));
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef SiteReferrerStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType('U', i + 1, String::SubString("EXT_TAG_ID 1"),
        String::SubString("www.test.com")), inner_data);
    }
#else
    SiteReferrerStatCollector::DataT::KeyT
      inner_key('U', 567, String::SubString("EXT_TAG_ID 1"),
        String::SubString("www.testhost.com"));
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<SiteReferrerStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    UserPropertiesCollector collector;
    UserPropertiesData data(111, 222, 333, 444, 555, 666);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef UserPropertiesKey KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      collector.add(KeyType(TEST_TIME, TEST_TIME, i + 1,
        'U', "prop_name", ""), data);
    }
#else
    UserPropertiesKey
      key(TEST_TIME, TEST_TIME, 999, 'I', "prop_name", "");
    collector.add(key, data);
#endif
    LogIoTester<UserPropertiesTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    RequestCollector collector;
    RequestData::Revenue adv_revenue, pub_revenue, isp_revenue,
      adv_comm_revenue, adv_payable_comm_amount, pub_comm_revenue;

    typedef RequestData::Revenue::FixedNum RevenueFixedNum;
    adv_revenue.rate_id = 111;
    adv_revenue.request_revenue = RevenueFixedNum("1111");
    adv_revenue.imp_revenue = RevenueFixedNum("1122");
    adv_revenue.click_revenue = RevenueFixedNum("1133");
    adv_revenue.action_revenue = RevenueFixedNum("1144");
    pub_revenue.rate_id = 222;
    pub_revenue.request_revenue = RevenueFixedNum("2211");
    pub_revenue.imp_revenue = RevenueFixedNum("2222");
    pub_revenue.click_revenue = RevenueFixedNum("2233");
    pub_revenue.action_revenue = RevenueFixedNum("2244");
    isp_revenue.rate_id = 333;
    isp_revenue.request_revenue = RevenueFixedNum("3311");
    isp_revenue.imp_revenue = RevenueFixedNum("3322");
    isp_revenue.click_revenue = RevenueFixedNum("3333");
    isp_revenue.action_revenue = RevenueFixedNum("3344");
//    isp_revenue.action_revenue = -0.000001;
    adv_comm_revenue.rate_id = 666;
    adv_comm_revenue.request_revenue = RevenueFixedNum("6611");
    adv_comm_revenue.imp_revenue = RevenueFixedNum("6622");
    adv_comm_revenue.click_revenue = RevenueFixedNum("6633");
    adv_comm_revenue.action_revenue = RevenueFixedNum("6644");
    adv_payable_comm_amount.rate_id = 777;
    adv_payable_comm_amount.request_revenue = RevenueFixedNum("7711");
    adv_payable_comm_amount.imp_revenue = RevenueFixedNum("7722");
    adv_payable_comm_amount.click_revenue = RevenueFixedNum("7733");
    adv_payable_comm_amount.action_revenue = RevenueFixedNum("7744");
    pub_comm_revenue.rate_id = 888;
    pub_comm_revenue.request_revenue = RevenueFixedNum("8811");
    pub_comm_revenue.imp_revenue = RevenueFixedNum("8822");
    pub_comm_revenue.click_revenue = RevenueFixedNum("8833.98765432");
    pub_comm_revenue.action_revenue = RevenueFixedNum("8844.12345678");

    NumberArray channel_list;
    channel_list.push_back(111);
    channel_list.push_back(222);
    channel_list.push_back(333);
    channel_list.push_back(444);
    channel_list.push_back(555);

    NumberArray history_channel_list;
    history_channel_list.push_back(101);
    history_channel_list.push_back(202);
    history_channel_list.push_back(303);
    history_channel_list.push_back(404);
    history_channel_list.push_back(505);

    UserPropertyList user_properties;
    user_properties.push_back(UserProperty("Property Name 1",
        "Property Value 1"));
    user_properties.push_back(UserProperty("Property Name 2",
        "Property Value 2"));
    user_properties.push_back(UserProperty("Property Name 3",
        "Property Value 3"));
    user_properties.push_back(UserProperty("Property Name 4",
        "Property Value 4"));
    user_properties.push_back(UserProperty("Property Name 5",
        "Property Value 5"));
    // Added in ADSC-9944. Check read write empty string
    //   (StringIoTest: empty_separated_string() replace)
    user_properties.push_back(UserProperty("OsVersion", ""));
    user_properties.push_back(UserProperty("BrowserVersion", "\t"));

    RequestData::CmpChannelList cmp_channel_list;
    RequestData::CmpChannel cmp_channel;
    typedef RequestData::CmpChannel::FixedNum CmpChannelFixedNum;
    cmp_channel.channel_id = 101;
    cmp_channel.channel_rate_id = 102;
    cmp_channel.imp_revenue = CmpChannelFixedNum("103.12345678");
    cmp_channel.imp_sys_revenue = CmpChannelFixedNum("103.87654321");
    cmp_channel.adv_imp_revenue = CmpChannelFixedNum("113.12345678");
    cmp_channel.click_revenue = CmpChannelFixedNum("123.12345678");
    cmp_channel.click_sys_revenue = CmpChannelFixedNum("123.87654321");
    cmp_channel.adv_click_revenue = CmpChannelFixedNum("133.12345678");
    cmp_channel_list.push_back(cmp_channel);
    cmp_channel.channel_id = 201;
    cmp_channel.channel_rate_id = 202;
    cmp_channel.imp_revenue = CmpChannelFixedNum("203.12345678");
    cmp_channel.imp_sys_revenue = CmpChannelFixedNum("203.87654321");
    cmp_channel.adv_imp_revenue = CmpChannelFixedNum("213.12345678");
    cmp_channel.click_revenue = CmpChannelFixedNum("223.12345678");
    cmp_channel.click_sys_revenue = CmpChannelFixedNum("223.87654321");
    cmp_channel.adv_click_revenue = CmpChannelFixedNum("233.12345678");
    cmp_channel_list.push_back(cmp_channel);
    cmp_channel.channel_id = 301;
    cmp_channel.channel_rate_id = 302;
    cmp_channel.imp_revenue = CmpChannelFixedNum("303.12345678");
    cmp_channel.imp_sys_revenue = CmpChannelFixedNum("303.87654321");
    cmp_channel.adv_imp_revenue = CmpChannelFixedNum("313.12345678");
    cmp_channel.click_revenue = CmpChannelFixedNum("323.12345678");
    cmp_channel.click_sys_revenue = CmpChannelFixedNum("323.87654321");
    cmp_channel.adv_click_revenue = CmpChannelFixedNum("333.12345678");
    cmp_channel_list.push_back(cmp_channel);

    NumberArray lost_auction_ccgs;
    lost_auction_ccgs.push_back(101);
    lost_auction_ccgs.push_back(202);
    lost_auction_ccgs.push_back(303);
    lost_auction_ccgs.push_back(404);
    lost_auction_ccgs.push_back(505);

    NumberArray geo_channels;
    geo_channels.push_back(1001);
    geo_channels.push_back(2002);
    geo_channels.push_back(3003);
    geo_channels.push_back(4004);
    geo_channels.push_back(5005);

    const time_t ONE_HOUR = 3600;

    AdServer::LogProcessing::FixedNumberList model_ctrs1;
    model_ctrs1.push_back(AdServer::LogProcessing::FixedNumber(false, 1, 23000000));
    model_ctrs1.push_back(AdServer::LogProcessing::FixedNumber(false, 4, 56000000));
    model_ctrs1.push_back(AdServer::LogProcessing::FixedNumber(false, 7, 89000000));

    AdServer::LogProcessing::FixedNumberList model_ctrs2;
    model_ctrs2.push_back(AdServer::LogProcessing::FixedNumber(false, 12, 34000000));
    model_ctrs2.push_back(AdServer::LogProcessing::FixedNumber(false, 56, 78000000));

    RequestCollector::DataT data1(
      Generics::Time::get_time_of_day(),
      Generics::Time::get_time_of_day() - ONE_HOUR,
      Generics::Time::get_time_of_day() - 2 * ONE_HOUR,
      Generics::Time::get_time_of_day() - 3 * ONE_HOUR,
      RequestId("PPPPPPPPPPPPPPPPPPPPPA.."),
      RequestId("PPPPPPPPPPPPPPPPPPPPPA.."),
      UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
      UserId(),
      true, // test_request
      11111, // colo_id
      11112, // site_id
      22222, // tag_id
      "",
      77777,
      "ru",
      "IP ADDR",
      88888,
      99999,
      33333,
      55555,
      66666,
      RequestCollector::DataT::DeliveryThresholdT::ZERO,
      false,
      44444,
      user_properties,
      adv_revenue,
      pub_revenue,
      isp_revenue,
      adv_comm_revenue,
      adv_payable_comm_amount,
      pub_comm_revenue,
      channel_list,
      history_channel_list,
      "1 & 2 | 2 & 3",
      cmp_channel_list,
      55555,
      333,
      false,
      true,
      6,
      100, // position
      false,
      true,
      false,
      true,
      false,
      'D',
      'U',
      lost_auction_ccgs,
      geo_channels,
      OptionalValue<unsigned long>(),
      "64 x 64",
      77, // size_id
      false,
      OptionalValue<unsigned long>(),
      2,
      OptionalValue<unsigned long>(),
      111,
      321, // campaign_freq
      "",
      RequestData::FixedNum("1111"),
      RequestData::FixedNum("2222"),
      RequestData::FixedNum("3333"),
      RequestData::FixedNum("4444"),
      RequestData::FixedNum("5555"),
      RequestData::FixedNum("6666"),
      RequestData::FixedNum("7777"),
      "", // ctr_algorithm_id
      RequestData::FixedNum("11.11"), // ctr
      1010, // full_referer_hash
      AdServer::CampaignSvcs::AT_PROPORTIONAL_PROBABILITY,
      "", // conv_rate_algorithm_id
      RevenueFixedNum::ZERO,
      100, // tag_predicted_viewability
      model_ctrs1,
      RequestData::FixedNum("0.1"), // self_service_commission
      RequestData::FixedNum("0.1"), // adv_commission
      RequestData::FixedNum("0.13"), // pub_cost_coef
      0 // flags
    );

    RequestCollector::DataT data2(
      Generics::Time::get_time_of_day(),
      Generics::Time::get_time_of_day() - ONE_HOUR,
      Generics::Time::get_time_of_day() - 2 * ONE_HOUR,
      Generics::Time::get_time_of_day() - 3 * ONE_HOUR,
      RequestId("PPPPPPPPPPPPPPPPPPPPPA.."),
      RequestId(),
      UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
      UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
      false,
      11111,
      11112, // site_id
      22222,
      "EXT TAG ID",
      77777,
      "ru",
      "",
      88888,
      99999,
      33333,
      55555,
      66666,
      RequestCollector::DataT::DeliveryThresholdT("0.9999"),
      false,
      44444,
      user_properties,
      adv_revenue,
      pub_revenue,
      isp_revenue,
      adv_comm_revenue,
      adv_payable_comm_amount,
      pub_comm_revenue,
      channel_list,
      history_channel_list,
      "1 & 2 | 2 & 3",
      cmp_channel_list,
      0,
      999,
      true,
      false,
      1,
      7, // position
      true,
      false,
      true,
      false,
      true,
      'T',
      'O',
      lost_auction_ccgs,
      NumberArray(), // geo_channels
      3131,
      "32x32",
      OptionalUlong(), // size_id
      true,
      3,
      OptionalValue<unsigned long>(),
      1,
      111,
      123, // campaign_freq
      "test referer",
      RequestData::FixedNum("1111"),
      RequestData::FixedNum("0"),
      RequestData::FixedNum("3333"),
      RequestData::FixedNum("0"),
      RequestData::FixedNum("5555"),
      RequestData::FixedNum("6666"),
      RequestData::FixedNum("7777"),
      "ctr algorithm id", // ctr_algorithm_id
      RequestData::FixedNum("10.111"), // ctr
      2020, // full_referer_hash
      AdServer::CampaignSvcs::AT_PROPORTIONAL_PROBABILITY,
      "", // conv_rate_algorithm_id
      RevenueFixedNum::ZERO,
      -1, // tag_predicted_viewability
      model_ctrs2,
      RequestData::FixedNum("0.1"), // self_service_commission
      RequestData::FixedNum("0.1"), // adv_commission
      RequestData::FixedNum("0.13"), // pub_cost_coef
      0 // flags
    );

#if LOGIOTEST_PRODUCE_LARGER_FILES
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      collector.add(data1);
      collector.add(data2);
    }
#else
    collector.add(data1);
    collector.add(data2);
#endif
    LogIoTester<RequestTraits>(dump_on_fail).test(collector);
#if 0
    RequestCollector collector1 = collector, collector2 = collector;
    typedef LogIoProxy<RequestTraits> LogIoProxyT;
    LogIoProxyT::save(collector, ".", 2);
    LogIoProxyT::save(collector1, ".", 5);
    LogIoProxyT::save(collector2, ".", 4);
#endif
#if 0
    typedef RequestTraits::IoHelperType LogIoHelperT;
    RequestCollector collector1;
    typedef GenericLogSaverImpl<RequestTraits> SaverT;
    LogSortingMap lsm;
    search_for_files(".", RequestTraits::log_base_name(), lsm);
    system("rm -rf ./OUT");
    system("mkdir -p ./OUT");
    if (lsm.empty())
    {
      std::cerr << "$$ lsm is empty!" << std::endl;
    }
    else
    {
      LogIoHelperT io_helper(collector1);
      for (LogSortingMap::const_iterator it = lsm.begin();
        it != lsm.end(); ++it)
      {
        std::cerr << "$$ it->second = " << it->second << std::endl;
        std::cerr << "$$ " << __LINE__ << std::endl;
        std::ifstream ifs(it->second.c_str());
        io_helper.load(ifs);
        std::cerr << "$$ " << __LINE__ << std::endl;
      }
      io_helper.save("./OUT/");
      std::cerr << "$$ " << __LINE__ << std::endl;
    }
#endif
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    TagAuctionStatCollector collector;
    TagAuctionStatCollector::KeyT key1(TEST_TIME, 34);
    TagAuctionStatCollector::KeyT key2(TEST_TIME, 56);
    TagAuctionStatCollector::DataT data;
    TagAuctionStatCollector::DataT::DataT inner_data(757);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef TagAuctionStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, 988),
        inner_data);
    }
#else
    TagAuctionStatCollector::DataT::KeyT
      inner_key(123, 987);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<TagAuctionStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    TagRequestCollector collector;

    TagRequestCollector::DataT::OptInSection
      opt_in_sect(
        100,
        UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
        200,
        false,
        true,
        new AdServer::Commons::StringHolder("Test User Agent")
      );

    TagRequestCollector::DataT::OptInSectionOptional opt_in_sect_opt;

    opt_in_sect_opt = opt_in_sect;

    TagRequestCollector::DataT data1(
      Generics::Time::get_time_of_day(),
      Generics::Time::get_time_of_day(), // isp_time
      false, // test_request
      1,
      2,
      11,
      "",
      "www.referer1.com",
      777,
      'I',
      "RUS",
      RequestId("PPPPPPPPPPPPPPPPPPPPPA.."),
      FixedNumber("123.123"),
      StringList { "URL_1", "URL_2" },
      TagRequestCollector::DataT::OptInSectionOptional()
    );

    TagRequestCollector::DataT data2(
      Generics::Time::get_time_of_day(),
      Generics::Time::get_time_of_day(), // isp_time
      true, // test_request
      1,
      2,
      OptionalUlong(),
      "EXT TAG ID #1",
      "www.referer2.com",
      987,
      'O',
      "RUS",
      RequestId("PPPPPPPPPPPPPPPPPPPPPA.."),
      FixedNumber("0"),
      {},
      opt_in_sect_opt
    );

    TagRequestCollector::DataT data3(
      Generics::Time::get_time_of_day(),
      Generics::Time::get_time_of_day(), // isp_time
      false, // test_request
      1,
      2,
      22,
      "EXT_TAG_ID_#2",
      "www.referer3.com",
      111,
      'U',
      "RUS",
      RequestId("PPPPPPPPPPPPPPPPPPPPPA.."),
      FixedNumber("123.321"),
      StringList { "URL_1", "URL_2", "URL_3" },
      opt_in_sect_opt
    );

#if LOGIOTEST_PRODUCE_LARGER_FILES
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      collector.add(data1);
      collector.add(data2);
      collector.add(data3);
    }
#else
    collector.add(data1);
    collector.add(data2);
    collector.add(data3);
#endif
    LogIoTester<TagRequestTraits>(dump_on_fail).test(collector);
#if 0
    TagRequestCollector collector1 = collector, collector2 = collector;
    typedef LogIoProxy<TagRequestTraits> LogIoProxyT;
    LogIoProxyT::save(collector, ".", 2);
    LogIoProxyT::save(collector1, ".", 5);
    LogIoProxyT::save(collector2, ".", 4);
#endif
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ChannelHitStatCollector collector;
    ChannelHitStatCollector::KeyT key1(TEST_TIME, 123);
    ChannelHitStatCollector::KeyT key2(TEST_TIME, 321);
    ChannelHitStatCollector::DataT data;
    ChannelHitStatCollector::DataT::DataT inner_data(99, 88, 77, 66, 55);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ChannelHitStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1), inner_data);
    }
#else
    ChannelHitStatCollector::DataT::KeyT inner_key(999);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ChannelHitStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ActionStatCollector collector;
    ActionStatCollector::KeyT key1(TEST_TIME, 123);
    ActionStatCollector::KeyT key2(TEST_TIME, 321);
    ActionStatCollector::DataT data;
    ActionStatCollector::DataT::DataT
      inner_data1(88, 77, "", "", "  www.google.com/\t", TEST_TIME,
        FixedNumber("11.11"), 10001);
    ActionStatCollector::DataT::DataT
      inner_data2(88, 77, "ORDER_ID 1", "", "  www.google.com/\t", TEST_TIME,
        TEST_TIME, FixedNumber("22.22"), 10002);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ActionStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(RequestId("1OOydKMMTs2X0nelpQ2eHg.."),
        RequestId("PPPPPPPPPPPPPPPPPPPPPA.."), i + 1), inner_data1);
      data.add(KeyType(RequestId("1OOydKMMTs2X0nelpQ2eHg.."),
        RequestId("PPPPPPPPPPPPPPPPPPPPPA.."), i + 1), inner_data2);
    }
    collector.add(key1, data);
    collector.add(key2, data);
#else
    ActionStatCollector::DataT::KeyT
      inner_key(RequestId("1OOydKMMTs2X0nelpQ2eHg.."),
        RequestId("PPPPPPPPPPPPPPPPPPPPPA.."), 99);
    data.add(inner_key, inner_data1);
    collector.add(key1, data);
    data.add(inner_key, inner_data2);
    collector.add(key2, data);
#endif
    LogIoTester<ActionStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ActionRequestCollector collector;
    ActionRequestCollector::KeyT key1(TEST_TIME, 123);
    ActionRequestCollector::KeyT key2(TEST_TIME, 321);
    ActionRequestCollector::DataT data;
    ActionRequestCollector::DataT::DataT
      inner_data(999, FixedNumber("123.321"));
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ActionRequestCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, "", "  www.google.com/\t", 'I'), inner_data);
    }
#else
    ActionRequestCollector::DataT::KeyT
      inner_key(
        567,
        "",
        "  www.google.com/\t",
        'I'
      );
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ActionRequestTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ColoUpdateStatCollector collector;
    Generics::Time time1(String::SubString("2009-03-17"), "%Y-%d-%d");
    ColoUpdateStatData data1(time1, time1,
      OptionalString("1.0"));
    ColoUpdateStatData data2(Generics::Time::get_time_of_day(),
      time1, OptionalString("1.1"));
    ColoUpdateStatData data3(time1,
      Generics::Time::get_time_of_day(),
        OptionalString("1.0"));
#if LOGIOTEST_PRODUCE_LARGER_FILES
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      ColoUpdateStatKey key1(i + 1);
      ColoUpdateStatKey key2(max_iterations - i - 1);
      collector.add(key1, data1);
      collector.add(key1, data2);
      collector.add(key1, data3);
      collector.add(key2, data1);
      collector.add(key2, data2);
    }
#else
    ColoUpdateStatKey key1(123);
    ColoUpdateStatKey key2(321);
    collector.add(key1, data1);
    collector.add(key1, data2);
    collector.add(key1, data3);
    collector.add(key2, data1);
    collector.add(key2, data2);
#endif
    LogIoTester<ColoUpdateStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    PassbackStatCollector collector;
    PassbackStatCollector::KeyT key1(TEST_TIME, 22);
    PassbackStatCollector::KeyT key2(TEST_TIME, 21);
    PassbackStatCollector::DataT data;
    PassbackStatCollector::DataT::DataT inner_data(555);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef PassbackStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType('U', "RUS", i + 1, i + 2), inner_data);
    }
#else
    PassbackStatCollector::DataT::KeyT inner_key('U', "RUS", 789, 987);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<PassbackStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ChannelInventoryEstimationStatCollector collector;
    ChannelInventoryEstimationStatCollector::KeyT key1(TEST_TIME, 21);
    ChannelInventoryEstimationStatCollector::KeyT key2(TEST_TIME, 22);
    ChannelInventoryEstimationStatCollector::DataT data;
    typedef ChannelInventoryEstimationStatCollector::DataT::KeyT::LevelT
      LevelT;
    typedef ChannelInventoryEstimationStatCollector::DataT::DataT::FixedNum
      FixedNum;
    ChannelInventoryEstimationStatCollector::DataT::DataT
      inner_data(FixedNum("111.123456"), FixedNum("222.654321"));
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ChannelInventoryEstimationStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, LevelT(1.39)), inner_data);
    }
#else
    ChannelInventoryEstimationStatCollector::DataT::KeyT
      inner_key(149180, LevelT(1.39));
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ChannelInventoryEstimationStatTraits>(dump_on_fail).test(
      collector
    );
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    AdvertiserActionCollector collector;
    AdvertiserActionData data(
      Generics::Time::get_time_of_day(),
      UserId()/*UserId("A3kph0f4RPu5A9tmlArhSg..")*/,
      RequestId("2Sfj7L31T-Wrzc37sdWzgA.."),
      789,
      987,
      RequestId("1OOydKMMTs2X0nelpQ2eHg.."),
      {111, 222, 333, 444, 555},
      "  www.google.com/\t",
      "ORDER ID 1",
      "IP ADDR",
      FixedNumber(1234.1020304));

    collector.add(data);

    LogIoTester<AdvertiserActionTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ActionOpportunityCollector collector;
    ActionOpportunityData data;
    data.time = Generics::Time::get_time_of_day();
    data.user_id = UserId("1OOydKMMTs2X0nelpQ2eHg..");
    data.cid = 987654321;
    data.request_id = RequestId("PPPPPPPPPPPPPPPPPPPPPA..");
    collector.add(data);
    LogIoTester<ActionOpportunityTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    PassbackOpportunityCollector collector;
    PassbackOpportunityData data(TEST_TIME, 321, 123, TEST_USER_ID, 'U',
      TEST_USER_ID);
    collector.add(data);
    LogIoTester<PassbackOpportunityTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ClickCollector collector;
    ClickData data(TEST_TIME, TEST_USER_ID, "Referrer 1",
      AdServer::Commons::Optional<unsigned long>());
    collector.add(data);
    LogIoTester<ClickTraits>(dump_on_fail).test(collector);
#if 0
    ClickCollector collector1 = collector, collector2 = collector;
    typedef LogIoProxy<ClickTraits> LogIoProxyT;
    LogIoProxyT::save(collector, ".", 2);
    LogIoProxyT::save(collector1, ".", 5);
    LogIoProxyT::save(collector2, ".", 4);
#endif
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ImpressionCollector collector;
    ImpressionData
      data(
        TEST_TIME,
        TEST_USER_ID,
        TEST_USER_ID,
        "Referrer 1",
        OptionalFixedNumber("123.456789"),
        OptionalFixedNumber("987.654321"),
        'P',
        'N',
        66,
        String::SubString(),
        AdServer::Commons::Optional<unsigned long>()
      );
    collector.add(data);
    LogIoTester<ImpressionTraits>(dump_on_fail).test(collector);
#if 0
    typedef GenericLogIoHelperImpl<ImpressionTraits> LogIoHelperT;
    ImpressionCollector collector1;
    typedef GenericLogSaverImpl<ImpressionTraits> SaverT;
    LogSortingMap lsm;
    search_for_files(".", ImpressionTraits::log_base_name(), lsm);
    system("rm -rf ./OUT");
    system("mkdir -p ./OUT");
    if (lsm.empty())
    {
      std::cerr << "$$ lsm is empty!" << std::endl;
    }
    else
    {
      LogIoHelperT io_helper(collector1);
      for (LogSortingMap::const_iterator it = lsm.begin();
        it != lsm.end(); ++it)
      {
        std::cerr << "$$ it->second = " << it->second << std::endl;
        std::cerr << "$$ " << __LINE__ << std::endl;
        std::ifstream ifs(it->second.c_str());
        io_helper.load(ifs);
        std::cerr << "$$ " << __LINE__ << std::endl;
      }
      io_helper.save("./OUT/");
      std::cerr << "$$ " << __LINE__ << std::endl;
    }
#endif
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    PassbackImpressionCollector collector;
    PassbackImpressionData data(TEST_TIME, TEST_USER_ID,
      AdServer::Commons::Optional<unsigned long>());
    collector.add(data);
    LogIoTester<PassbackImpressionTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    HistoryMatchCollector collector;
    NumberList search_channels, page_channels, url_channels;
    search_channels.push_back(111);
    search_channels.push_back(222);
    search_channels.push_back(333);
    search_channels.push_back(444);
    search_channels.push_back(555);
    page_channels.push_back(111);
    page_channels.push_back(222);
    page_channels.push_back(333);
    page_channels.push_back(444);
    page_channels.push_back(555);
    url_channels.push_back(444);
    url_channels.push_back(333);
    url_channels.push_back(111);
    url_channels.push_back(222);
    url_channels.push_back(555);
    HistoryMatchData data(
      Generics::Time::get_time_of_day(),
      UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
      0,
      321,
      654,
      987,
      search_channels,
      page_channels,
      url_channels);
    collector.add(data);
    LogIoTester<HistoryMatchTraits>(dump_on_fail).test(collector);
#if 0
    HistoryMatchCollector collector1 = collector, collector2 = collector;
    typedef LogIoProxy<HistoryMatchTraits> LogIoProxyT;
    LogIoProxyT::save(collector, ".", 2);
    LogIoProxyT::save(collector1, ".", 5);
    LogIoProxyT::save(collector2, ".", 4);
#endif
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    CmpStatCollector collector;
    CmpStatCollector::KeyT key1(TEST_TIME, TEST_TIME, 123);
    CmpStatCollector::KeyT key2(TEST_TIME, TEST_TIME, 321);
    CmpStatCollector::DataT data;
    typedef CmpStatCollector::DataT::DataT::FixedNum FixedNum;
    typedef CmpStatCollector::DataT::KeyT::DeliveryThresholdT
      DeliveryThresholdT;
    CmpStatCollector::DataT::DataT
      inner_data(111, 222, FixedNum("400.12345"),
        FixedNum("200.98765"), FixedNum("1.0912"));
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef CmpStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 100, i + 10, i + 20, "ru", i + 1,
        DeliveryThresholdT("-1.12"), i + 20, i + 30, i + 40,
          max_iterations - i - 1, i, i, false, true), inner_data);
    }
#else
    CmpStatCollector::DataT::KeyT inner_key1(111, 999, 99, "", 127,
      DeliveryThresholdT("0.19999"), 888, 777, 666,
        721, 172, 712, false, true);
    CmpStatCollector::DataT::KeyT inner_key2(111, 999, OptionalUlong(), "Ru",
      127, DeliveryThresholdT("0.19999"), 888, 777, 666,
        721, 172, 712, true, false);
    data.add(inner_key1, inner_data);
    data.add(inner_key2, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<CmpStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);


  try
  {
    SiteStatCollector collector;
    SiteStatCollector::KeyT key1(TEST_TIME, 123);
    SiteStatCollector::KeyT key2(TEST_TIME, 321);
    SiteStatCollector::DataT data;
    SiteStatCollector::DataT::DataT inner_data(111, 222);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef SiteStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1), inner_data);
    }
#else
    SiteStatCollector::DataT::KeyT inner_key1(127);
    SiteStatCollector::DataT::KeyT inner_key2(721);
    data.add(inner_key1, inner_data);
    data.add(inner_key2, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<SiteStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    SiteUserStatCollector collector;
    SiteUserStatCollector::KeyT key1(TEST_TIME, 123);
    SiteUserStatCollector::KeyT key2(TEST_TIME, 321);
    SiteUserStatCollector::DataT data;
    SiteUserStatCollector::DataT::DataT inner_data(111);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef SiteUserStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1), TEST_TIME, inner_data);
    }
#else
    SiteUserStatCollector::DataT::KeyT inner_key1(127, TEST_TIME);
    SiteUserStatCollector::DataT::KeyT inner_key2(721, TEST_TIME);
    data.add(inner_key1, inner_data);
    data.add(inner_key2, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<SiteUserStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    PageLoadsDailyStatCollector collector;
    PageLoadsDailyStatCollector::KeyT key1(TEST_TIME, 21);
    PageLoadsDailyStatCollector::KeyT key2(TEST_TIME, 22);
    PageLoadsDailyStatCollector::DataT data;
    PageLoadsDailyStatCollector::DataT::DataT inner_data(111, 222);
    NumberList tags;
    tags.push_back(1);
    tags.push_back(2);
    tags.push_back(3);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef PageLoadsDailyStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, "RU", tags), inner_data);
    }
#else
    PageLoadsDailyStatCollector::DataT::KeyT
      inner_key1(127, "RU", tags.begin(), tags.end());
    PageLoadsDailyStatCollector::DataT::KeyT
      inner_key2(721, "", tags.begin(), tags.end());
    data.add(inner_key1, inner_data);
    data.add(inner_key2, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<PageLoadsDailyStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ChannelCountStatCollector collector;
    ChannelCountStatCollector::KeyT key1(TEST_TIME, 22);
    ChannelCountStatCollector::KeyT key2(TEST_TIME, 21);
    ChannelCountStatCollector::DataT data;
    ChannelCountStatCollector::DataT::DataT inner_data(555);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ChannelCountStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType('A', i + 1), inner_data);
    }
#else
    ChannelCountStatCollector::DataT::KeyT inner_key('A', 789);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ChannelCountStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    WebStatCollector collector;
    WebStatCollector::KeyT key1(TEST_TIME, 21);
    WebStatCollector::KeyT key2(TEST_TIME, 22);
    WebStatCollector::DataT data;
    WebStatCollector::DataT::DataT inner_data(333);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef WebStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      KeyType
        inner_key(
          "ct",
          "curct",
          "browser",
          "os",
          "source",
          i + 1,
          'S',
          'I',
          false,
          OptionalValue<unsigned long>(),
          123
        );
      data.add(inner_key, inner_data);
    }
#else
    WebStatCollector::DataT::KeyT
      inner_key1(
        "ct",
        "curct",
        "",
        "os",
        "",
        12345,
        'S',
        'U',
        false,
        OptionalValue<unsigned long>(),
        123
      );
    WebStatCollector::DataT::KeyT
      inner_key2(
        "ct",
        "curct",
        "browser",
        "os",
        "source",
        54321,
        'F',
        'U',
        false,
        321,
        OptionalValue<unsigned long>()
      );
    data.add(inner_key1, inner_data);
    data.add(inner_key2, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<WebStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ChannelOverlapUserStatCollector collector;
    ChannelOverlapUserStatCollector::KeyT key1(TEST_TIME);
    ChannelOverlapUserStatCollector::KeyT key2(TEST_TIME);
    ChannelOverlapUserStatCollector::DataT data;
    ChannelOverlapUserStatCollector::DataT::DataT inner_data(555);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef ChannelOverlapUserStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, i + 2), inner_data);
    }
#else
    ChannelOverlapUserStatCollector::DataT::KeyT inner_key(789, 987);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<ChannelOverlapUserStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    UserAgentStatCollector collector;
    UserAgentStatCollector::KeyT key1(TEST_TIME);
    UserAgentStatCollector::KeyT
      key2(Generics::Time(String::SubString("2012-06-04"), "%Y-%m-%d"));
    UserAgentStatCollector::DataT data;
    NumberList channels;
    channels.push_back(101);
    channels.push_back(102);
    channels.push_back(103);
    NumberList platforms;
    UserAgentStatCollector::DataT::DataT
      inner_data1(1, channels.begin(), channels.end(),
        platforms.begin(), platforms.end());
    platforms.push_back(201);
    platforms.push_back(202);
    platforms.push_back(203);
    UserAgentStatCollector::DataT::DataT
      inner_data2(111, channels.begin(), channels.end(),
        platforms.begin(), platforms.end());
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef UserAgentStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      std::ostringstream oss;
      oss << "User Agent #" << (i + 1);
      data.add(KeyType(new AdServer::Commons::StringHolder(oss.str())),
        inner_data1);
    }
#else
    UserAgentStatCollector::DataT::KeyT
      inner_key1(new AdServer::Commons::StringHolder("User Agent #1"));
    std::string long_ua(2000, 'A');
    UserAgentStatCollector::DataT::KeyT
      inner_key2(new AdServer::Commons::StringHolder(long_ua));
    std::string too_long_ua(2001, 'B');
    UserAgentStatCollector::DataT::KeyT
      inner_key3(new AdServer::Commons::StringHolder(too_long_ua));
    data.add(inner_key1, inner_data1);
    data.add(inner_key2, inner_data2);
    data.add(inner_key3, inner_data2);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<UserAgentStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    DeviceChannelCountStatCollector collector;
    DeviceChannelCountStatCollector::KeyT key1(TEST_TIME, 22);
    DeviceChannelCountStatCollector::KeyT key2(TEST_TIME, 21);
    DeviceChannelCountStatCollector::DataT data;
    DeviceChannelCountStatCollector::DataT::DataT inner_data(555);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef DeviceChannelCountStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 100, 'A', i + 1), inner_data);
    }
#else
    DeviceChannelCountStatCollector::DataT::KeyT inner_key(987, 'A', 789);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<DeviceChannelCountStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    SearchTermStatCollector collector;
    SearchTermStatCollector::KeyT key1(TEST_TIME, 21);
    SearchTermStatCollector::KeyT key2(TEST_TIME, 22);
    SearchTermStatCollector::DataT data;
    SearchTermStatCollector::DataT::DataT inner_data(333);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef SearchTermStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      std::ostringstream oss;
      oss << "search term " << (i + 1);
      data.add(KeyType(oss.str()), inner_data);
    }
#else
    SearchTermStatCollector::DataT::KeyT inner_key1("search term 1");
    SearchTermStatCollector::DataT::KeyT inner_key2("search term 2");
    data.add(inner_key1, inner_data);
    data.add(inner_key2, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
//    {
//      LogSaver_var d_saver =
//        new SearchTermStatTraits::DistribSaverType(collector, ".", 3);
//      d_saver->save();
//    }
    LogIoTester<SearchTermStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    SearchEngineStatCollector collector;
    SearchEngineStatCollector::KeyT key1(TEST_TIME, 21);
    SearchEngineStatCollector::KeyT key2(TEST_TIME, 22);
    SearchEngineStatCollector::DataT data;
    SearchEngineStatCollector::DataT::DataT inner_data(111, 222);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef SearchEngineStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, "host name"), inner_data);
    }
#else
    SearchEngineStatCollector::DataT::KeyT inner_key(123, "host name");
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<SearchEngineStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    TagPositionStatCollector collector;
    TagPositionStatCollector::KeyT key1(TEST_TIME, 123);
    TagPositionStatCollector::KeyT key2(TEST_TIME, 321);
    TagPositionStatCollector::DataT data;
    TagPositionStatCollector::DataT::DataT inner_data(99, 88, 77);
#if LOGIOTEST_PRODUCE_LARGER_FILES
    typedef TagPositionStatCollector::DataT::KeyT KeyType;
    for (unsigned i = 0; i < max_iterations; ++i)
    {
      data.add(KeyType(i + 1, i + 2, i + 3, i + 4, false), inner_data);
    }
#else
    TagPositionStatCollector::DataT::KeyT
      inner_key(999, 888, OptionalValue<unsigned long>(), 666, false);
    data.add(inner_key, inner_data);
#endif
    collector.add(key1, data);
    collector.add(key2, data);
    LogIoTester<TagPositionStatTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

  try
  {
    ImpNotifyCollector collector;
    ImpNotifyData data;
    data.time = Generics::Time::get_time_of_day();
    data.user_id = UserId("PPPPPPPPPPPPPPPPPPPPPA..");
    collector.add(data);
    data.channels.push_back(111);
    data.channels.push_back(222);
    data.channels.push_back(333);
    data.channels.push_back(444);
    data.channels.push_back(555);
    data.revenue = FixedNumber("1234.1020304");
    collector.add(data);
    LogIoTester<ImpNotifyTraits>(dump_on_fail).test(collector);
  }
  HANDLE_EXCEPTIONS(exitcode, 1);

#if 0
  try
  {
    const unsigned max_num = 100;
    ChannelTriggerStatCollector::KeyT key1(TEST_TIME, 123);
    ChannelTriggerStatCollector::KeyT key2(TEST_TIME, 321);
    ChannelTriggerStatCollector::DataT data;
    ChannelTriggerStatCollector::DataT::DataT inner_data(777);
    AdServer::Commons::StringHolder_var
      tr(new AdServer::Commons::StringHolder("A, \"BB\" CCC"));
    typedef ChannelTriggerStatCollector::DataT::KeyT KeyType;
    char types[] = { 'P', 'C', 'S', 'U' };
    for (unsigned i = 0; i < max_num; ++i)
    {
      data.add(KeyType(i + 1, types[i % sizeof(types)], tr), inner_data);
    }
    typedef ChannelTriggerStatExtTraits Traits;
    Traits::CollectorPtrType collector = new Traits::CollectorType;
    collector->add(key1, data);
    collector->add(key2, data);
    Traits::CollectorBundlePtrType bundle = new Traits::CollectorBundleType;
    bundle->initialize();
    bundle->merge_collector(collector);
    const std::string rmcmd = "rm -rf ";
    const std::string mkdircmd = "mkdir ";
    const char csvdir[] = ".CSVLoadingTest/";
    const char tmpdir[] = ".CSVLoadingTest/tmp/";
    system((rmcmd + csvdir).c_str());
    system((mkdircmd + csvdir).c_str());
    Traits::CsvSaverType csv_saver(bundle, csvdir);
    csv_saver.save();
    // select sdate,colo_id,channel_id,trigger_type,trigger_word,hits from
    // channeltriggerstats where sdate='2011-06-06 00:00:00';
    // psql -h test-nbmaster -d stat -U adserver -t -P format=unaligned -P
    // fieldsep="    " -o ~/tmp/psql.2
    const std::string shell = "bash";
    const std::string outfilename = std::string(tmpdir) + ".psql.out";
/*
    const char shcmd[] = "psql -h test-nbmaster -d stat -U adserver -t "
                          "  -P format=unaligned -P fieldsep=\\\"\t\\\" "
                          "  -o " CTS_CSV_TEST_OUTPUT_FILENAME " <<EOF\n"
//                          "select count(*) from channeltriggerstats;\n"
//                          "select sdate,colo_id,channel_id,trigger_type,"
                          "select to_char(sdate, 'YYYY-MM-DD'), colo_id,"
                          "   channel_id, trigger_type, trigger_word, hits"
                          " from channeltriggerstats"
                          "   where sdate='2011-06-06 00:00:00';\n"
                          "EOF\n";
*/
    std::ostringstream oss;
    oss << "psql -h test-nbmaster -d stat -U adserver -t -P format=unaligned "
           "-P fieldsep=\\\"\t\\\" -o " << outfilename << " <<EOF\n"
           "select count(*) from channeltriggerstats "
           "where sdate='" << TEST_TIME.gm_f() << "';\n"
           "select to_char(sdate, 'YYYY-MM-DD'), colo_id,"
           "  channel_id, trigger_type, trigger_word, hits "
           "from channeltriggerstats where sdate='" <<
           TEST_TIME.gm_f() << "';\n"
           "EOF\n";
    const std::string shcmd = oss.str();

    system((mkdircmd + tmpdir).c_str());
    Traits::CollectorPtrType collector0 = new Traits::CollectorType;
    {
      system((shell + " -c \"" + shcmd + "\"").c_str());
      std::ifstream ifs(outfilename.c_str());
      size_t numlines = 0;
      ifs >> numlines;
      while (numlines--)
      {
        DayTimestamp sdate;
        unsigned colo_id, channel_id, hits;
        char trigger_type;
        std::string trigger_word;
        ifs >> sdate;
        read_tab(ifs);
        ifs >> colo_id;
        read_tab(ifs);
        ifs >> channel_id;
        read_tab(ifs);
        ifs >> trigger_type;
        read_tab(ifs);
        read_until_tab(ifs, trigger_word);
        read_tab(ifs);
        ifs >> hits;
        read_eol(ifs);
        ChannelTriggerStatKey key(sdate, colo_id);
        ChannelTriggerStatData data;
        AdServer::Commons::StringHolder_var
          tr(new AdServer::Commons::StringHolder(trigger_word));
        ChannelTriggerStatInnerKey ikey(channel_id, trigger_type, tr);
        ChannelTriggerStatInnerData idata(hits);
        data.add(ikey, idata);
        collector0->add(key, data);
      }
    }
    const std::string rsync = "rsync";
    const char rsyncdest[] =
      "rsync://test-nbmaster:10873/csvlistener";
    system((rsync + " " + csvdir + "/*.csv " + rsyncdest).c_str());
    unsigned nsleepsecs = 30;
    std::cout << "Sleeping for " << nsleepsecs << " secs..." << std::endl;
    sleep(nsleepsecs);
    std::cout << "Resuming..." << std::endl;
    Traits::CollectorPtrType collector1 = new Traits::CollectorType;
    {
      system((shell + " -c \"" + shcmd + "\"").c_str());
      std::ifstream ifs(outfilename.c_str());
      size_t numlines = 0;
      ifs >> numlines;
      while (numlines--)
      {
        DayTimestamp sdate;
        unsigned colo_id, channel_id, hits;
        char trigger_type;
        std::string trigger_word;
        ifs >> sdate;
        read_tab(ifs);
        ifs >> colo_id;
        read_tab(ifs);
        ifs >> channel_id;
        read_tab(ifs);
        ifs >> trigger_type;
        read_tab(ifs);
        read_until_tab(ifs, trigger_word);
        read_tab(ifs);
        ifs >> hits;
        read_eol(ifs);
        ChannelTriggerStatKey key(sdate, colo_id);
        ChannelTriggerStatData data;
        AdServer::Commons::StringHolder_var
          tr(new AdServer::Commons::StringHolder(trigger_word));
        ChannelTriggerStatInnerKey ikey(channel_id, trigger_type, tr);
        ChannelTriggerStatInnerData idata(hits);
        data.add(ikey, idata);
        collector1->add(key, data);
      }
    }
    for (Traits::CollectorType::iterator it = collector1->begin();
      it != collector1->end(); ++it)
    {
      for (Traits::CollectorType::DataT::iterator iit = it->second.begin();
        iit != it->second.end(); ++iit)
      {
        unsigned hits = iit->second.hits() -
          collector0->find(it->first)->second.find(iit->first)->second.hits();
        iit->second = Traits::CollectorType::DataT::DataT(hits);
      }
    }
    if (EqualityTester<Traits>()(*collector, *collector1))
    {
      std::cout << "CSV loading test: PASSED" << std::endl;
      system((rmcmd + csvdir).c_str());
    }
    else
    {
      std::cout << "CSV loading test: FAILED" << std::endl;
//      Traits::CollectorBundlePtrType bundle = new Traits::CollectorBundleType;
//      bundle->initialize();
//      bundle->merge_collector(collector1);
//      const std::string rmcmd = "rm -rf ";
//      const std::string mkdircmd = "mkdir -p";
//      const char csvdir[] = ".CSVLoadingTest/FAIL";
//      system((rmcmd + csvdir).c_str());
//      system((mkdircmd + csvdir).c_str());
//      Traits::CsvSaverType csv_saver(bundle, csvdir);
//      csv_saver.save();
    }
  }
  HANDLE_EXCEPTIONS(exitcode, 1);
#endif

#if 0
  typedef OptionalValue<Aux_::StringIoWrapper,
    Aux_::ClearableOptionalValueTraits<Aux_::StringIoWrapper> >
      OptionalStringT;

  OptionalStringT s1, s2;

  s1 = "^";

  {
    std::ofstream ofs("opt_str_test.txt");
    ofs << s1;
  }
  {
    std::ifstream ifs("opt_str_test.txt");
    ifs >> s2;
  }
  std::cout << "s1 = " << s1 << std::endl;
  std::cout << "s2 = " << s2 << std::endl;
  std::cout << "s1.present() = " << s1.present() << std::endl;
  std::cout << "s2.present() = " << s2.present() << std::endl;
  std::cout << "s1.get() = " << s1.get() << std::endl;
  std::cout << "s2.get() = " << s2.get() << std::endl;
  std::cout << "static_cast<const std::string&>(s1.get()) = " <<
    static_cast<const std::string&>(s1.get()) << std::endl;
  std::cout << "static_cast<const std::string&>(s2.get()) = " <<
    static_cast<const std::string&>(s2.get()) << std::endl;
  std::cout << "(s1 == s2) = " << (s1 == s2) << std::endl;
#endif

#if 0
  LogFileNameInfo info(CreativeStatTraits::log_base_name());
  std::string file_name;

  LogFileNameInfo parse_res;

#define PARSE_LOG_FILE_NAME(file, info) \
try \
{ \
  parse_log_file_name(file, info); \
} \
catch (const InvalidLogFileNameFormat&) \
{ \
  std::cerr << "File '" << file << "' has invalid log file name format" \
            << std::endl; \
}

#define PRINT_LOGFILENAMEINFO(info) \
std::cout << "info.format = " << info.str_format() << std::endl; \
std::cout << "info.base_name = " << info.base_name << std::endl; \
std::cout << "info.timestamp = " <<\
  info.timestamp.get_gm_time().format("%Y%m%d.%H%M%S.%q") << std::endl; \
std::cout << "info.distrib_count = " << info.distrib_count << std::endl; \
std::cout << "info.distrib_index = " << info.distrib_index << std::endl;

  std::string out_dir_name;
  info.distrib_index = 123;
  info.distrib_count = 321;
  file_name = make_log_file_name(info, out_dir_name);
  std::cout << "##\tCURRENT\t\tfile_name: " << file_name << std::endl;
  PARSE_LOG_FILE_NAME(file_name, parse_res);
  PRINT_LOGFILENAMEINFO(parse_res);

  info.format = LFNF_CSV;
  file_name = make_log_file_name(info, out_dir_name);
  std::cout << "##\tCSV\t\tfile_name: " << file_name << std::endl;
  PARSE_LOG_FILE_NAME(file_name, parse_res);
  PRINT_LOGFILENAMEINFO(parse_res);

  info.format = LFNF_BASIC;
  file_name = make_log_file_name(info, out_dir_name);
//  file_name[file_name.find_last_of('.')] = 'X';
//  file_name[file_name.find_last_of('.') + 1] = 'X';
//  *(file_name.end() - 1) = 'X';
  std::cout << "##\tBASIC\t\tfile_name: " << file_name << std::endl;
  PARSE_LOG_FILE_NAME(file_name, parse_res);
  PRINT_LOGFILENAMEINFO(parse_res);

  info.format = LFNF_EXTENDED;
  info.distrib_index = 87;
  file_name = make_log_file_name(info, out_dir_name);
  *(file_name.end() - 12) = 'X';
  std::cout << "##\tEXTENDED\tfile_name: " << file_name << std::endl;
  PARSE_LOG_FILE_NAME(file_name, parse_res);
  PRINT_LOGFILENAMEINFO(parse_res);
  parse_res.format = LOG_FILE_NAME_FORMAT(39);
  std::cout << "##@@ parse_res.format = " << parse_res.format << std::endl;
  PRINT_LOGFILENAMEINFO(parse_res);
  parse_res.format = LOG_FILE_NAME_FORMAT(LFNF_INVALID - 1);
  std::cout << "##@@ parse_res.format = " << parse_res.format << std::endl;
  PRINT_LOGFILENAMEINFO(parse_res);
  parse_res.format = LOG_FILE_NAME_FORMAT(LFNF_CURRENT + 1);
  std::cout << "##@@ parse_res.format = " << parse_res.format << std::endl;
  PRINT_LOGFILENAMEINFO(parse_res);

  file_name = "CreativeStat.";
  std::cout << "##\tERR\tfile_name: " << file_name << std::endl;
  PARSE_LOG_FILE_NAME(file_name, parse_res);
  PRINT_LOGFILENAMEINFO(parse_res);
  std::cout << "##\n";

  file_name = "CreativeStat.1";
  std::cout << "##\tERR\tfile_name: " << file_name << std::endl;
  PARSE_LOG_FILE_NAME(file_name, parse_res);
  PRINT_LOGFILENAMEINFO(parse_res);
  std::cout << "##\n";

  file_name = "CreativeStat.12";
  std::cout << "##\tERR\tfile_name: " << file_name << std::endl;
  PARSE_LOG_FILE_NAME(file_name, parse_res);
  PRINT_LOGFILENAMEINFO(parse_res);
  std::cout << "##\n";

  file_name = "CreativeStat.123";
  std::cout << "##\tERR\tfile_name: " << file_name << std::endl;
  PARSE_LOG_FILE_NAME(file_name, parse_res);
  PRINT_LOGFILENAMEINFO(parse_res);
  std::cout << "##\n";
#endif

  return exitcode;
}

