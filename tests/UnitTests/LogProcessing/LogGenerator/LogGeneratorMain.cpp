
#include <iostream>
#include <fstream>
#include <optional>

#include <Generics/AppUtils.hpp>
#include <Generics/DirSelector.hpp>
#include <Generics/TaskRunner.hpp>
#include <String/Tokenizer.hpp>
#include <Logger/Logger.hpp>
#include <TestCommons/MTTester.hpp>

#include <LogCommons/ActionRequest.hpp>
#include <LogCommons/ActionStat.hpp>
#include <LogCommons/AdRequestLogger.hpp>
#include <LogCommons/ArchiveIfstream.hpp>
#include <LogCommons/CcgKeywordStat.hpp>
#include <LogCommons/CcgStat.hpp>
#include <LogCommons/ChannelCountStat.hpp>
#include <LogCommons/ChannelHitStat.hpp>
#include <LogCommons/ChannelImpInventory.hpp>
#include <LogCommons/ChannelInventory.hpp>
#include <LogCommons/ChannelInventoryEstimationStat.hpp>
#include <LogCommons/ChannelPerformance.hpp>
#include <LogCommons/ChannelPriceRange.hpp>
#include <LogCommons/ChannelTriggerImpStat.hpp>
#include <LogCommons/ChannelTriggerStat.hpp>
#include <LogCommons/CmpStat.hpp>
#include <LogCommons/ColoUpdateStat.hpp>
#include <LogCommons/ColoUsers.hpp>
#include <LogCommons/CreativeStat.hpp>
#include <LogCommons/ExpressionPerformance.hpp>
#include <LogCommons/HistoryMatch.hpp>
#include <LogCommons/PageLoadsDailyStat.hpp>
#include <LogCommons/PassbackStat.hpp>
#include <LogCommons/Request.hpp>
#include <LogCommons/RequestBasicChannels.hpp>
#include <LogCommons/SiteReferrerStat.hpp>
#include <LogCommons/SiteStat.hpp>
#include <LogCommons/TagAuctionStat.hpp>
#include <LogCommons/TagRequest.hpp>
#include <LogCommons/UserProperties.hpp>
#include <LogCommons/WebStat.hpp>
#include <LogCommons/LogCommons.ipp>

#include "LogGeneratorMain.hpp"

using namespace AdServer::LogProcessing;

namespace
{
  /// Most of logs use the same data, declare it here
  const Generics::Time TEST_TIME(Generics::Time::get_time_of_day());
  const UserId TEST_USER_ID("PPPPPPPPPPPPPPPPPPPPPA..");
  const RequestId TEST_REQUEST_ID(
    "PPPPPPPPPPPPPPPPPPPPPA..");

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

  /// Make type ClickCollector differ to
  /// ImpressionCollector and PassbackImpressionCollector. (enable override)
  struct ClickCollectorT : public ClickCollector
  {};

  struct ClickTraitsT : public ClickTraits
  {
    typedef ClickCollectorT CollectorType;
  };

  bool file_generator_mode = false;

  const char USAGE[] =
    "LogIoTest - unit test for log input/output code.\n\n"
    "Usage: LogIoTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path       The path to folder which hold generate files.\n"
    "  -d, --debug      Any failed test will dump collector contents to disk\n"
    "  -mt              Multithread generation of files\n"
    "  -g, --generate   Comma separated list of Log names to be generated,\n"
    "                   option with empty list \"\" will generate all\n"
    "                   possible Log files\n"
    "  -gd, --generate_dist The same as --generate, but will generate"
    " distributed logs with additional savings\n"
    "  -c, --content    Maximum count of records in one generated Log file\n"
    "  -r, --raw        Do not create commit files\n"
    "  -l, --log-number Number of files to generate (1 by default)\n"
    "  -t, --time       Print time meterings\n"
    "  -e, --examine    Search suitable files and load it in memory\n"
    "  -h, --help       Print this information\n";


  Generics::AppUtils::CheckOption debug;
  Generics::AppUtils::CheckOption print_time;
  Generics::AppUtils::CheckOption no_commit_files;

  Generics::AppUtils::StringOption generate_list;
  Generics::AppUtils::StringOption generate_distrib_list;

  const unsigned long DEFAULT_RECORDS_COUNT = 1;
  Generics::AppUtils::Option<unsigned long> records_count(
    DEFAULT_RECORDS_COUNT);
  Generics::AppUtils::Option<unsigned long> files_count(1);

  Generics::AppUtils::CheckOption opt_generate_req_ids;
  Generics::AppUtils::CheckOption check_load_files;
  Generics::AppUtils::CheckOption multi_thread_generation;

} // namespace

  const char* TimingResult::prefix = 0;
  Generics::Time TimingResult::total_time;
  Generics::Time TimingResult::total_time_abs;

/**
 * Functor (some in one) that fill containers, too many differences in filling
 * and need a lot of code
 */
struct Filler
{
  /// First value for modification while unique records generate
  /// Start from 1, not zero
  static const std::size_t BEGIN_INDEX = 123;

  Filler(bool generate_unique_request_ids = false)
    : generate_unique_request_ids_(generate_unique_request_ids)
  {}

  /// Additional savings
  template <typename CommonCollector>
  static void
  distribute_save(const CommonCollector&, Generics::Time&, Generics::Time&)
  {
  }

  void
  operator ()(ActionOpportunityCollector& collector)
  {
    ActionOpportunityData data;
    data.time = TEST_TIME;
    data.user_id = UserId("1OOydKMMTs2X0nelpQ2eHg..");
    data.cid = 987654321;
    data.request_id = TEST_REQUEST_ID;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      collector.add(data);
    }
  }

  void
  operator ()(ActionRequestCollector& collector)
  {
    ActionRequestCollector::KeyT key1(TEST_TIME, 123);
    ActionRequestCollector::KeyT key2(TEST_TIME, 321);
    ActionRequestCollector::DataT data;
    ActionRequestCollector::DataT::DataT
      inner_data(999, FixedNumber("123.321"));
    typedef ActionRequestCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i, "", "  www.google.com/\t", 'I'), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ActionStatCollector& collector)
  {
    ActionStatCollector::KeyT key1(TEST_TIME, 123);
    ActionStatCollector::KeyT key2(TEST_TIME, 321);
    ActionStatCollector::DataT data;
    ActionStatCollector::DataT::DataT
      inner_data1(88, 77, "", "", "  www.google.com/\t", TEST_TIME,
        FixedNumber("11.11"), 10001);
    ActionStatCollector::DataT::DataT
      inner_data2(88, 77, "ORDER_ID 1", "", "  www.google.com/\t", TEST_TIME,
        TEST_TIME, FixedNumber("22.22"), 10002);
    typedef ActionStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(RequestId("1OOydKMMTs2X0nelpQ2eHg.."), RequestId(), i),
        inner_data1);
      data.add(KeyType(RequestId("1OOydKMMTs2X0nelpQ2eHg.."), RequestId(), i),
        inner_data2);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(AdvertiserActionCollector& collector)
  {
    AdvertiserActionData data(
      TEST_TIME,
      TEST_USER_ID,
      RequestId(),
      OptionalUlong(),
      OptionalUlong(),
      RequestId(),
      NumberList(),
      "",
      "",
      "",
      FixedNumber(FixedNumber::ZERO));
      collector.add(data);

    AdvertiserActionData data2(
      TEST_TIME,
      TEST_USER_ID,
      RequestId(),
      OptionalUlong(),
      OptionalUlong(),
      RequestId("1OOydKMMTs2X0nelpQ2eHg.."),
      {111, 222, 333, 444, 555},
      "",
      "",
      "",
      FixedNumber(FixedNumber::ZERO));
      collector.add(data2);

    AdvertiserActionData data3(
      TEST_TIME,
      TEST_USER_ID,
      RequestId(),
      OptionalUlong(),
      OptionalUlong(),
      RequestId("1OOydKMMTs2X0nelpQ2eHg.."),
      {111, 222, 333, 444, 555},
      "  www.google.com/\t",
      "",
      "",
      FixedNumber(FixedNumber::ZERO));
      collector.add(data3);

    AdvertiserActionData data4(
      TEST_TIME,
      TEST_USER_ID,
      RequestId(),
      789,
      1,
      RequestId("1OOydKMMTs2X0nelpQ2eHg.."),
      {111, 222, 333, 444, 555},
      "  www.google.com/\t",
      "order_id",
      "ip addr",
      FixedNumber(FixedNumber::ZERO));
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      collector.add(data4);
    }
  }

  void
  operator ()(AdvertiserUserStatCollector& collector)
  {
    AdvertiserUserStatCollector::KeyT key1(TEST_TIME);
    AdvertiserUserStatCollector::KeyT key2(TEST_TIME);
    AdvertiserUserStatCollector::DataT data;
    AdvertiserUserStatCollector::DataT::DataT inner_data(111, 222, 333);
    typedef AdvertiserUserStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i + 1, TEST_TIME), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(CcgKeywordStatCollector& collector)
  {
    CcgKeywordStatCollector::KeyT key1(TEST_TIME, 123);
    CcgKeywordStatCollector::KeyT key2(TEST_TIME, 321);
    CcgKeywordStatCollector::DataT data;
    typedef CcgKeywordStatCollector::DataT::DataT::FixedNum FixedNum;
    CcgKeywordStatCollector::DataT::DataT
      inner_data(111, 222, FixedNum("200.98765"),
        FixedNum("300.98765"), FixedNum("400.98765"));
    typedef CcgKeywordStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i, *records_count - i, 0), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(CcgStatCollector& collector)
  {
    CcgStatCollector::KeyT key1(TEST_TIME, 21);
    CcgStatCollector::KeyT key2(TEST_TIME, 22);
    CcgStatCollector::DataT data;
    CcgStatCollector::DataT::DataT inner_data(123);
    typedef CcgStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(CcStatCollector& collector)
  {
    CcStatCollector::KeyT key1(TEST_TIME, 21);
    CcStatCollector::KeyT key2(TEST_TIME, 22);
    CcStatCollector::DataT data;
    CcStatCollector::DataT::DataT inner_data(2011);
    typedef CcStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ChannelCountStatCollector& collector)
  {
    ChannelCountStatCollector::KeyT key1(TEST_TIME, 22);
    ChannelCountStatCollector::KeyT key2(TEST_TIME, 21);
    ChannelCountStatCollector::DataT data;
    ChannelCountStatCollector::DataT::DataT inner_data(555);
    typedef ChannelCountStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType('A', i + 1), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ChannelHitStatCollector& collector)
  {
    ChannelHitStatCollector::KeyT key1(TEST_TIME, 123);
    ChannelHitStatCollector::KeyT key2(TEST_TIME, 321);
    ChannelHitStatCollector::DataT data;
    ChannelHitStatCollector::DataT::DataT inner_data(99, 88, 77, 66, 55);
    typedef ChannelHitStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ChannelImpInventoryCollector& collector)
  {
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
    typedef ChannelImpInventoryCollector::DataT::KeyT KeyType;
    typedef ChannelImpInventoryInnerKey::CCGType CCGType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(*records_count - i, CCGType::TEXT), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ChannelInventoryCollector& collector)
  {
    ChannelInventoryCollector::KeyT key1(TEST_TIME, 177);
    ChannelInventoryCollector::KeyT key2(TEST_TIME, 177);
    ChannelInventoryCollector::DataT data;
    typedef ChannelInventoryCollector::DataT::DataT::FixedNum FixedNum;
    ChannelInventoryCollector::DataT::DataT
      inner_data(FixedNum("120.8901"), FixedNum("99"), FixedNum("43.252"));
    typedef ChannelInventoryCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(*records_count - i), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ChannelInventoryEstimationStatCollector& collector)
  {
    ChannelInventoryEstimationStatCollector::KeyT key1(TEST_TIME, 21);
    ChannelInventoryEstimationStatCollector::KeyT key2(TEST_TIME, 22);
    ChannelInventoryEstimationStatCollector::DataT data;
    typedef ChannelInventoryEstimationStatCollector::DataT::KeyT::LevelT
      LevelT;
    typedef ChannelInventoryEstimationStatCollector::DataT::DataT::FixedNum
      FixedNum;
    ChannelInventoryEstimationStatCollector::DataT::DataT
      inner_data(FixedNum("111.123456"), FixedNum("222.654321"));
    typedef ChannelInventoryEstimationStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i, LevelT(1.39)), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ChannelPerformanceCollector& collector)
  {
    ChannelPerformanceCollector::KeyT key1(TEST_TIME, 123);
    ChannelPerformanceCollector::KeyT key2(TEST_TIME, 321);
    ChannelPerformanceCollector::DataT data;
    typedef ChannelPerformanceCollector::DataT::DataT::FixedNum FixedNum;
    ChannelPerformanceCollector::DataT::DataT
      inner_data(111, 99, 88, 77, FixedNum("66"));
    typedef ChannelPerformanceCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i, i, AdServer::Commons::ImmutableString("SIZE 1")),
        inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ChannelPriceRangeCollector& collector)
  {
    ChannelPriceRangeCollector::KeyT key1(TEST_TIME);
    ChannelPriceRangeCollector::KeyT key2(TEST_TIME);
    ChannelPriceRangeCollector::DataT data;
    ChannelPriceRangeCollector::DataT::DataT inner_data(120.8901, 1234);
    typedef ChannelPriceRangeCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(AdServer::Commons::ImmutableString("popup"),
        AdServer::Commons::ImmutableString("ru"),
        i, KeyType::EcpmT(8.342), 999), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ChannelTriggerImpStatCollector& collector)
  {
    ChannelTriggerImpStatCollector::KeyT key1(TEST_TIME, 123);
    ChannelTriggerImpStatCollector::KeyT key2(TEST_TIME, 321);
    ChannelTriggerImpStatCollector::DataT data;
    typedef ChannelTriggerImpStatCollector::DataT::DataT::FixedNum FixedNum;
    ChannelTriggerImpStatCollector::DataT::DataT
      inner_data(FixedNum("777"), FixedNum("888"));
    typedef ChannelTriggerImpStatCollector::DataT::KeyT KeyType;
    char types[] = { 'P', 'S', 'U' };
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i, i + 1, types[i % sizeof(types)]), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ChannelTriggerStatCollector& collector)
  {
    ChannelTriggerStatCollector::KeyT key1(TEST_TIME, 123);
    ChannelTriggerStatCollector::KeyT key2(TEST_TIME, 321);
    ChannelTriggerStatCollector::DataT data;
    ChannelTriggerStatCollector::DataT::DataT inner_data(777);
    typedef ChannelTriggerStatCollector::DataT::KeyT KeyType;
    char types[] = { 'P', 'S', 'U' };
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(
        KeyType(i, i + 100, types[i % sizeof(types)]),
        inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ClickCollectorT& collector)
  {
    ClickData data(TEST_TIME, TEST_REQUEST_ID, "",
      std::optional<unsigned long>());

    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      collector.add(data);
    }
  }

  static void
  distribute_save(ClickCollectorT& collector,
    Generics::Time& time, Generics::Time& time_abs)
  {
    distribute_save_def_<ClickTraits>(collector, time, time_abs);
  }

  void
  operator ()(CmpStatCollector& collector)
  {
    CmpStatCollector::KeyT key1(TEST_TIME, TEST_TIME, 123);
    CmpStatCollector::KeyT key2(TEST_TIME, TEST_TIME, 321);
    CmpStatCollector::DataT data;
    typedef CmpStatCollector::DataT::DataT::FixedNum FixedNum;
    CmpStatCollector::DataT::DataT
      inner_data(111, 222, FixedNum("400.12345"),
        FixedNum("300.98765"), FixedNum("12.34567890"));
    typedef CmpStatCollector::DataT::KeyT KeyType;
    typedef KeyType::DeliveryThresholdT DeliveryThresholdT;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i, i + 1, i + 10, "", i + 2, DeliveryThresholdT("1.0"),
        i, i, i, *records_count - i, i, i, i % 2, i % 4),
          inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ColoUpdateStatCollector& collector)
  {
    Generics::Time time1(String::SubString("2009-03-17"), "%Y-%d-%d");
    ColoUpdateStatData data1(time1, time1, OptionalString("1.0"));
    ColoUpdateStatData data2(TEST_TIME, time1, OptionalString("2.9.0.11"));
    ColoUpdateStatData data3(time1, TEST_TIME, OptionalString("2.9.0.9"));
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      ColoUpdateStatKey key1(i);
      ColoUpdateStatKey key2(*records_count - i);
      collector.add(key1, data1);
      collector.add(key1, data2);
      collector.add(key1, data3);
      collector.add(key2, data1);
      collector.add(key2, data2);
    }
  }

  void
  operator ()(ColoUsersCollector& collector)
  {
    ColoUsersData data(999, 888, 111, 222, 333);
    typedef ColoUsersKey KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      collector.add(KeyType(TEST_TIME, OptionalDayTimestamp(), i, 1), data);
    }
  }

  void
  operator ()(CreativeStatCollector& collector)
  {
    CreativeStatCollector::KeyT key1(TEST_TIME, TEST_TIME);
    CreativeStatCollector::KeyT key2(
      TEST_TIME - Generics::Time::ONE_HOUR,
      TEST_TIME - Generics::Time::ONE_HOUR);
    CreativeStatCollector::DataT data;
    typedef CreativeStatCollector::DataT::DataT::FixedNum FixedNum;
    CreativeStatCollector::DataT::DataT
      inner_data(10, 20, 30, 40, FixedNum("1"), FixedNum("2"),
        FixedNum("3"), FixedNum("4"), FixedNum("5"), FixedNum("6"),
          FixedNum("7"), FixedNum("8"));
    typedef CreativeStatCollector::DataT::KeyT KeyType;
    typedef KeyType::DeliveryThresholdT DeliveryThresholdT;
    typedef KeyType::GeoChannelIdOptional GeoChannelIdOptional;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i, i + 1, 2, 3, "", i, i + 1, i + 2, 1, 2, 3, 4,
        5, DeliveryThresholdT("0.99999"), 1, 1, false, false, false, 'U',
          GeoChannelIdOptional(), 1, 1, false, -1), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(ExpressionPerformanceCollector& collector)
  {
    ExpressionPerformanceCollector::KeyT key1(TEST_TIME, 123);
    ExpressionPerformanceCollector::KeyT key2(TEST_TIME, 321);
    ExpressionPerformanceCollector::DataT data;
    ExpressionPerformanceCollector::DataT::DataT inner_data(99, 88, 77);
    typedef ExpressionPerformanceCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i, "x&y"), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(HistoryMatchCollector& collector)
  {
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      collector.add(HistoryMatchData(
        TEST_TIME,
        TEST_USER_ID,
        0,
        i,
        654,
        987,
        SEARCH_CHANNELS,
        PAGE_CHANNELS,
        URL_CHANNELS));
    }
  }

  static void
  distribute_save(HistoryMatchCollector& collector,
    Generics::Time& time, Generics::Time& time_abs)
  {
    distribute_save_def_<HistoryMatchTraits>(collector, time, time_abs);
  }

  void
  operator ()(PageLoadsDailyStatCollector& collector)
  {
    PageLoadsDailyStatCollector::KeyT key1(TEST_TIME, 21);
    PageLoadsDailyStatCollector::KeyT key2(TEST_TIME, 22);
    PageLoadsDailyStatCollector::DataT data;
    PageLoadsDailyStatCollector::DataT::DataT inner_data(111, 222);
    NumberList tags;
    tags.push_back(1);
    tags.push_back(2);
    tags.push_back(3);
    typedef PageLoadsDailyStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i + 1, "RU", tags.begin(), tags.end()), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(PassbackOpportunityCollector& collector)
  {
    PassbackOpportunityData data(TEST_TIME, 321, 123, TEST_REQUEST_ID, 'U',
      TEST_USER_ID);
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      if (generate_unique_request_ids_)
      {
        data = PassbackOpportunityData(TEST_TIME, 321, 123,
          RequestId::create_random_based(), 'U', TEST_USER_ID);
      }
      collector.add(data);
      data = PassbackOpportunityData(
        data.time().time() - Generics::Time::ONE_HOUR, 321, 123,
          TEST_REQUEST_ID, 'U', TEST_USER_ID);
    }
  }

  void
  operator ()(PassbackStatCollector& collector)
  {
    PassbackStatCollector::KeyT key1(TEST_TIME, 22);
    PassbackStatCollector::KeyT key2(TEST_TIME, 21);
    PassbackStatCollector::DataT data;
    PassbackStatCollector::DataT::DataT inner_data(555);
    typedef PassbackStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType('U', "US", i, i + 1), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(RequestCollector& collector)
  {
    RequestData::Revenue adv_revenue, pub_revenue, isp_revenue,
      adv_comm_revenue, adv_payable_comm_amount, pub_comm_revenue;

    typedef RequestData::Revenue::FixedNum FixedNum;
    adv_revenue.rate_id = 111;
    adv_revenue.request_revenue = FixedNum(String::SubString("1111"));
    adv_revenue.imp_revenue = FixedNum(String::SubString("1122"));
    adv_revenue.click_revenue = FixedNum(String::SubString("1133"));
    adv_revenue.action_revenue = FixedNum(String::SubString("1144"));
    pub_revenue.rate_id = 222;
    pub_revenue.request_revenue = FixedNum(String::SubString("2211"));
    pub_revenue.imp_revenue = FixedNum(String::SubString("2222"));
    pub_revenue.click_revenue = FixedNum(String::SubString("2233"));
    pub_revenue.action_revenue = FixedNum(String::SubString("2244"));
    isp_revenue.rate_id = 333;
    isp_revenue.request_revenue = FixedNum(String::SubString("3311"));
    isp_revenue.imp_revenue = FixedNum(String::SubString("3322"));
    isp_revenue.click_revenue = FixedNum(String::SubString("3333"));
    isp_revenue.action_revenue = FixedNum(String::SubString("3344"));
//    isp_revenue.action_revenue = -0.000001;
    adv_comm_revenue.rate_id = 666;
    adv_comm_revenue.request_revenue = FixedNum(String::SubString("6611"));
    adv_comm_revenue.imp_revenue = FixedNum(String::SubString("6622"));
    adv_comm_revenue.click_revenue = FixedNum(String::SubString("6633"));
    adv_comm_revenue.action_revenue = FixedNum(String::SubString("6644"));
    adv_payable_comm_amount.rate_id = 777;
    adv_payable_comm_amount.request_revenue = FixedNum(String::SubString("7711"));
    adv_payable_comm_amount.imp_revenue = FixedNum(String::SubString("7722"));
    adv_payable_comm_amount.click_revenue = FixedNum(String::SubString("7733"));
    adv_payable_comm_amount.action_revenue = FixedNum(String::SubString("7744"));
    pub_comm_revenue.rate_id = 888;
    pub_comm_revenue.request_revenue = FixedNum(String::SubString("8811"));
    pub_comm_revenue.imp_revenue = FixedNum(String::SubString("8822"));
    pub_comm_revenue.click_revenue = FixedNum(String::SubString("8833"));
    pub_comm_revenue.action_revenue = FixedNum(String::SubString("8844"));

    NumberArray channel_list;
    channel_list.push_back(111);
    channel_list.push_back(222);
    channel_list.push_back(333);
    channel_list.push_back(444);
    channel_list.push_back(555);

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

    AdServer::LogProcessing::FixedNumberList model_ctrs1;
    model_ctrs1.push_back(AdServer::LogProcessing::FixedNumber(false, 1, 23000000));
    model_ctrs1.push_back(AdServer::LogProcessing::FixedNumber(false, 4, 56000000));
    model_ctrs1.push_back(AdServer::LogProcessing::FixedNumber(false, 7, 89000000));

    RequestCollector::DataT data1(
      TEST_TIME,
      TEST_TIME,
      TEST_TIME,
      TEST_TIME,
      TEST_REQUEST_ID,
      TEST_REQUEST_ID,
      TEST_USER_ID,
      TEST_USER_ID,
      true,
      11111,
      11112, // site_id
      22222,
      "",
      77777,
      "ru",
      "",
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
      NumberArray(),
      "1 & 2 | 2 & 3",
      RequestData::CmpChannelList(), // cmp_channel_list
      55555,
      333,
      false,
      true,
      6,
      100, // position
      true,
      true,
      false,
      false,
      false,
      'D',
      'U',
      lost_auction_ccgs,
      geo_channels,
      OptionalValue<unsigned long>(),
      "32x32",
      77, // size_id
      false,
      1,
      2,
      3,
      4,
      1, // campaign_freq
      "",
      FixedNum("1111"),
      FixedNum("2222"),
      FixedNum("3333"),
      FixedNum("4444"),
      FixedNum("5555"),
      FixedNum("6666"),
      FixedNum("7777"),
      "", // ctr_algorithm_id
      RequestData::FixedNum("11.11"), // ctr
      111, // full_referer_hash
      AdServer::CampaignSvcs::AT_PROPORTIONAL_PROBABILITY,
      "", // conv_rate_algorithm_id
      RequestData::FixedNum::ZERO, // conv_rate
      100, // tag_predicted_viewability
      model_ctrs1,
      FixedNum("0.1"), // self_service_commission
      FixedNum("0.1"), // adv_commission
      FixedNum("0.13"), // pub_cost_coef
      0 // flags
    );

    AdServer::LogProcessing::FixedNumberList model_ctrs2;
    model_ctrs2.push_back(AdServer::LogProcessing::FixedNumber(false, 12, 34000000));
    model_ctrs2.push_back(AdServer::LogProcessing::FixedNumber(false, 56, 78000000));

    RequestCollector::DataT data2(
      TEST_TIME,
      TEST_TIME,
      TEST_TIME,
      TEST_TIME,
      TEST_REQUEST_ID,
      TEST_REQUEST_ID,
      TEST_USER_ID,
      TEST_USER_ID,
      false,
      11111,
      11112, // site_id
      22222,
      "EXT_TAG_ID",
      77777,
      "ru",
      "",
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
      NumberArray(),
      "1 & 2 | 2 & 3",
      RequestData::CmpChannelList(), // cmp_channel_list
      0,
      999,
      true,
      false,
      1,
      7, // position
      false,
      false,
      true,
      false,
      false,
      'T',
      'U',
      lost_auction_ccgs,
      NumberArray(), // geo_channels
      OptionalValue<unsigned long>(),
      "32x32",
      OptionalUlong(), // size_id
      true,
      3,
      2,
      1,
      0,
      0, // campaign_freq
      "",
      FixedNum("1111"),
      FixedNum("2222"),
      FixedNum("3333"),
      FixedNum("4444"),
      FixedNum("5555"),
      FixedNum("6666"),
      FixedNum("7777"),
      "ctr algorithm id", // ctr_algorithm_id
      RequestData::FixedNum("10.111"), // ctr
      111, // full_referer_hash
      AdServer::CampaignSvcs::AT_PROPORTIONAL_PROBABILITY,
      "", // conv_rate_algorithm_id
      RequestData::FixedNum::ZERO, // conv_rate
      -1, // tag_predicted_viewability
      model_ctrs2,
      FixedNum("0.1"), // self_service_commission
      FixedNum("0.1"), // adv_commission
      FixedNum("0.13"), // pub_cost_coef
      0 // flags
      );

    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      collector.add(data1);
      collector.add(data2);
    }
  }

  static void
  distribute_save(RequestCollector& collector,
    Generics::Time& time, Generics::Time& time_abs)
  {
    distribute_save_def_<RequestTraits>(collector, time, time_abs);
  }

  void
  operator ()(RequestBasicChannelsCollector& collector)
  {
    RequestBasicChannelsCollector::KeyT key1(
      Generics::Time(String::SubString("2007-11-28 12:13:14"),
        "%Y-%m-%d %H:%M:%S"),
      Generics::Time(String::SubString("2007-11-28 11:13:14"),
        "%Y-%m-%d %H:%M:%S"), 21);
    RequestBasicChannelsCollector::KeyT key2(
      Generics::Time(String::SubString("2007-11-29 14:13:12"),
        "%Y-%m-%d %H:%M:%S"),
      Generics::Time(String::SubString("2007-11-29 13:13:12"),
        "%Y-%m-%d %H:%M:%S"), 22);

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
        UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
        UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
        RequestBasicChannelsCollector::DataT::DataT::MatchOptional(),
        RequestBasicChannelsCollector::DataT::DataT::AdRequestPropsOptional(ad_request_opt)
      );

    RequestBasicChannelsCollector::DataT::DataT
      inner_data2(
        'H',
        UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
        UserId("PPPPPPPPPPPPPPPPPPPPPA.."),
        match_request,
        RequestBasicChannelsCollector::DataT::DataT::AdRequestPropsOptional()
      );

    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(inner_data1);
    }
    collector.add(key1, data);
  }

  static void
  distribute_save(RequestBasicChannelsCollector& collector,
    Generics::Time& time, Generics::Time& time_abs)
  {
    distribute_save_def_<RequestBasicChannelsTraits>(collector, time, time_abs);
  }

  void
  operator ()(SiteReferrerStatCollector& collector)
  {
    SiteReferrerStatCollector::KeyT key1(TEST_TIME, 21);
    SiteReferrerStatCollector::KeyT key2(TEST_TIME, 22);
    SiteReferrerStatCollector::DataT data;
    SiteReferrerStatCollector::DataT::DataT
      inner_data(333, 444, 555, 666, 777, 888, 999,
        FixedNumber("1.1"), FixedNumber("2.2"), FixedNumber("3.3"),
        FixedNumber("4.4"), FixedNumber("5.5"), FixedNumber("6.6"));
    typedef SiteReferrerStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType('U', i, String::SubString("EXT_TAG_ID 1"),
        String::SubString("www.testhost.com")), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(SiteStatCollector& collector)
  {
    SiteStatCollector::KeyT key1(TEST_TIME, 123);
    SiteStatCollector::KeyT key2(TEST_TIME, 321);
    SiteStatCollector::DataT data;
    SiteStatCollector::DataT::DataT inner_data(111, 222);
    typedef SiteStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i), inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(TagAuctionStatCollector& collector)
  {
    TagAuctionStatCollector::KeyT key1(TEST_TIME, 34);
    TagAuctionStatCollector::KeyT key2(TEST_TIME, 56);
    TagAuctionStatCollector::DataT data;
    TagAuctionStatCollector::DataT::DataT inner_data(757);
    typedef TagAuctionStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      data.add(KeyType(i + 1, 988),
        inner_data);
    }
    collector.add(key1, data);
    collector.add(key2, data);
  }

  void
  operator ()(TagRequestCollector& collector)
  {
    TagRequestCollector::DataT::OptInSection
      opt_in_sect(
        100,
        TEST_USER_ID,
        200,
        false,
        false,
        new AdServer::Commons::StringHolder("Test User Agent")
      );

    TagRequestCollector::DataT::OptInSectionOptional opt_in_sect_opt;

    opt_in_sect_opt = opt_in_sect;

    TagRequestCollector::DataT data1(
      Generics::Time::get_time_of_day(),
      TEST_TIME,
      false, // test_request
      1,
      2,
      11,
      "",
      "www.referer1.com",
      777,
      'U',
      "US",
      TEST_REQUEST_ID,
      FixedNumber("123.321"),
      StringList { "URL_1", "URL_2" },
      TagRequestCollector::DataT::OptInSectionOptional()
    );

    TagRequestCollector::DataT data2(
      Generics::Time::get_time_of_day(),
      TEST_TIME,
      false, // test_request
      1,
      2,
      22,
      "EXT_TAG_ID",
      "www.referer1.com",
      987,
      'U',
      "US",
      TEST_REQUEST_ID,
      FixedNumber("123.321"),
      StringList { "URL_1", "URL_2" },
      opt_in_sect_opt
    );

    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      if(i % 2 == 0)
      {
        collector.add(data1);
      }
      else
      {
        collector.add(data2);
      }
    }
  }

  static void
  distribute_save(TagRequestCollector& collector,
    Generics::Time& time, Generics::Time& time_abs)
  {
    distribute_save_def_<TagRequestTraits>(collector, time, time_abs);
  }

  void
  operator ()(UserPropertiesCollector& collector)
  {
    const Generics::StringHashAdapter keys[5] = {
      Generics::StringHashAdapter("prop_name1"),
      Generics::StringHashAdapter("prop_name2"),
      Generics::StringHashAdapter("prop_name3"),
      Generics::StringHashAdapter("prop_name4"),
      Generics::StringHashAdapter("prop_name5")
      };

    UserPropertiesData data(111, 111, 111, 222, 333, 444);
    typedef UserPropertiesKey KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      collector.add(KeyType(TEST_TIME, TEST_TIME, i, 'U', keys[i % 5],
        "prop_value"), data);
    }
  }

  void
  operator ()(SeqCollector<GenericAdRequestData, true>& collector)
  {
    GenericAdRequestData data(TEST_TIME, TEST_REQUEST_ID,
      std::optional<unsigned long>());
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      if (generate_unique_request_ids_)
      {
        data = GenericAdRequestData(TEST_TIME,
          RequestId::create_random_based(),
          std::optional<unsigned long>());
      }
      collector.add(data);
      data = GenericAdRequestData(
        data.time().time() - Generics::Time::ONE_SECOND, TEST_REQUEST_ID,
        std::optional<unsigned long>());
    }
  }

  void
  operator ()(ImpressionCollector& collector)
  {
    ImpressionData data(TEST_TIME, TEST_REQUEST_ID, TEST_REQUEST_ID, "",
      OptionalFixedNumber(), OptionalFixedNumber(), 'A', 'T', -1,
      String::SubString(),
      std::optional<unsigned long>());
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      if (generate_unique_request_ids_)
      {
        data = ImpressionData(TEST_TIME, RequestId::create_random_based(),
          UserId::create_random_based(), "",
          OptionalFixedNumber(), OptionalFixedNumber(), 'A', 'T', -1,
          String::SubString(),
          std::optional<unsigned long>());
      }
      collector.add(data);
      data = ImpressionData(
        data.time().time() - Generics::Time::ONE_SECOND, TEST_REQUEST_ID,
        TEST_REQUEST_ID, "", OptionalFixedNumber(), OptionalFixedNumber(),
        'A', 'T',
        -1,
        String::SubString(),
        std::optional<unsigned long>());
    }
  }

  void
  operator ()(WebStatCollector& collector)
  {
    WebStatCollector::KeyT key1(TEST_TIME, 21);
    WebStatCollector::KeyT key2(TEST_TIME, 22);
    WebStatCollector::DataT data;
    WebStatCollector::DataT::DataT inner_data(333);
    typedef WebStatCollector::DataT::KeyT KeyType;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
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
    collector.add(key1, data);
    collector.add(key2, data);
  }

 /// Identical code for ImpressionCollector = use it
/*  void
  operator ()(PassbackImpressionCollector& collector)
  {
    PassbackImpressionData data;
    data.time = TEST_TIME;
    data.request_id = TEST_REQUEST_ID;
    for (unsigned i = BEGIN_INDEX; i < BEGIN_INDEX + *records_count; ++i)
    {
      collector.add(data);
    }
  }*/

private:
  template <typename LogTraits>
  static void
  distribute_save_def_(typename LogTraits::CollectorType& collector,
    Generics::Time& distrib_save_time, Generics::Time& distrib_save_time_abs)
  {
    // Later collector will be used to control result of distributed files loading
    typename LogTraits::CollectorType collector0, collector1, collector2;

    collector.deep_copy(collector0);
    collector.deep_copy(collector1);
    collector.deep_copy(collector2);

    collector += collector0;
    collector += collector1;
    collector += collector2;
    const std::string DIR_NAME = (root_path.installed() ? root_path :
      std::string("./") + LogTraits::log_base_name());

    Generics::ScopedTimer distrib_save_timer_abs(distrib_save_time_abs);
    Generics::ScopedCPUTimer distrib_save_timer(distrib_save_time);
    LogIoProxy<LogTraits>::save(collector0, DIR_NAME, 2);
    LogIoProxy<LogTraits>::save(collector1, DIR_NAME, 5);
    LogIoProxy<LogTraits>::save(collector2, DIR_NAME, 4);
  }

  bool generate_unique_request_ids_;
};

template <typename LogTraits>
class LogIoTester : public Prober
{
public:

  virtual void
  init()
  {
    dir_name_ = root_path.installed() ?
      root_path + '/' + name() :
      std::string("./") + name();
    Stream::Error clean_cmd;
    clean_cmd
       << "rm -f ./" << dir_name_ << "/* 2>/dev/null;"
       << "rm -f ./" << dir_name_ << "/Intermediate/* 2>/dev/null;"
       << "rm -f ./" << dir_name_ << "/Deferred/* 2>/dev/null;"
       << "rm -f ./" << dir_name_ << "/Deferred/Intermediate/* 2>/dev/null;"
       << "mkdir -p " << dir_name_ << "/Intermediate 2>/dev/null"
       << std::ends;
    system(clean_cmd.str().data());
  }

  virtual void
  load_log(const char* name) /*throw(eh::Exception)*/
  {
    using ArchiveIfstream = AdServer::LogProcessing::ArchiveIfstream;

    CollectorType restored_collector;

    std::unique_ptr<std::istream> istream;
    if (ArchiveIfstream::is_archive(name))
    {
      istream = std::make_unique<ArchiveIfstream>(name);
    }
    else
    {
      istream = std::make_unique<std::ifstream>(name);
    }
    auto& ref_istream = *istream;

     typename LogTraits::HeaderType log_header;
    if (!(ref_istream >> log_header))
    {
      Stream::Error ostr;
      ostr << "load_log<" << RequestTraits::log_base_name()
        << ">: Failed to read log header";
      throw Exception(ostr);
    }
    if (log_header.version() != LogTraits::current_version())
    {
      Stream::Error ostr;
      ostr << "load_log<" << RequestTraits::log_base_name()
        << ">: Invalid log header version: "
        << log_header.version();
      throw Exception(ostr);
    }
    LogIoProxy<LogTraits>::load(restored_collector, ref_istream);
  }

  virtual int
  probe_log(bool gen_req) noexcept
  {
    try
    {
      CollectorType collector;
      Filler filler(gen_req);
      if (print_time.enabled())
      {
        Generics::ScopedTimer fill_timer_abs(fill_time_abs);
        Generics::ScopedCPUTimer fill_timer(fill_time);
        filler(collector);
      }
      else
      {
        filler(collector);
      }

      test_(collector);
      return 0;
    }
    catch (const eh::Exception& ex)
    {
      std::cerr << ex.what() << std::endl;
    }
    catch (...)
    {
      std::cerr << "Unknown exception caught." << std::endl;
    }
    return 1;
  }

  virtual const char*
  name() const noexcept
  {
    return LogTraits::log_base_name();
  }

private:
  virtual
  ~LogIoTester() noexcept
  {}

  typedef typename LogTraits::CollectorType CollectorType;

  void
  test_(CollectorType& collector) /*throw(Exception, eh::Exception)*/;

  std::string dir_name_;
};

namespace
{

  const Prober_var LOGS[] =
  {
    new LogIoTester<ActionOpportunityTraits>,
    new LogIoTester<ActionRequestTraits>,
    new LogIoTester<ActionStatTraits>,
    new LogIoTester<AdvertiserActionTraits>,
    new LogIoTester<AdvertiserUserStatTraits>,
    new LogIoTester<CcgKeywordStatTraits>,
    new LogIoTester<CcgStatTraits>,
    new LogIoTester<CcStatTraits>,
    new LogIoTester<ChannelCountStatTraits>,
    new LogIoTester<ChannelHitStatTraits>,
    new LogIoTester<ChannelImpInventoryTraits>,
    new LogIoTester<ChannelInventoryTraits>,
    new LogIoTester<ChannelInventoryEstimationStatTraits>,
    new LogIoTester<ChannelPerformanceTraits>,
    new LogIoTester<ChannelPriceRangeTraits>,
    new LogIoTester<ChannelTriggerStatTraits>,
    new LogIoTester<ClickTraitsT>,
    new LogIoTester<CmpStatTraits>,
    new LogIoTester<ColoUpdateStatTraits>,
    new LogIoTester<ColoUsersTraits>,
    new LogIoTester<CreativeStatTraits>,
    new LogIoTester<ExpressionPerformanceTraits>,
    new LogIoTester<HistoryMatchTraits>,
    new LogIoTester<ImpressionTraits>,
    new LogIoTester<PageLoadsDailyStatTraits>,
    new LogIoTester<PassbackImpressionTraits>,
    new LogIoTester<PassbackOpportunityTraits>,
    new LogIoTester<PassbackStatTraits>,
    new LogIoTester<RequestTraits>,
    new LogIoTester<RequestBasicChannelsTraits>,
    new LogIoTester<SiteReferrerStatTraits>,
    new LogIoTester<SiteStatTraits>,
    new LogIoTester<TagAuctionStatTraits>,
    new LogIoTester<TagRequestTraits>,
    new LogIoTester<UserPropertiesTraits>,
    new LogIoTester<WebStatTraits>
  };

  const std::size_t LOGS_SIZE = (sizeof(LOGS) / sizeof(Prober_var));

  bool
  init(int& argc, char**& argv)
    /*throw(eh::Exception)*/
  {
    using namespace Generics::AppUtils;
    Args args;
    CheckOption opt_help;
    Generics::AppUtils::StringOption root_path_option;

    args.add(equal_name("path") || short_name("p"), root_path_option);
    args.add(equal_name("debug") || short_name("d"), debug);
    args.add(short_name("mt"), multi_thread_generation);
    args.add(equal_name("time") || short_name("t"), print_time);
    args.add(equal_name("generate") || short_name("g"), generate_list);
    args.add(equal_name("generate-dist") || short_name("gd"),
      generate_distrib_list);
    args.add(equal_name("content") || short_name("c"), records_count);
    args.add(equal_name("raw") || short_name("r"), no_commit_files);
    args.add(equal_name("log-number") || short_name("l"), files_count);
    args.add(equal_name("generate-rid"), opt_generate_req_ids);
    args.add(equal_name("examine") || short_name("e"), check_load_files);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);
    if (root_path_option.installed())
    {
      root_path.set(*root_path_option);
    }

    if (generate_list.installed() || generate_distrib_list.installed())
    {
      file_generator_mode = true;
      if (!generate_list->empty() || !generate_distrib_list->empty())
      {
        typedef std::map<std::string, std::size_t> Index;
        Index logs_indexes;
        for (std::size_t i = 0; i < LOGS_SIZE; ++i)
        {
          logs_indexes.insert(Index::value_type(LOGS[i]->name(), i));
        }
        std::string logs_list = *generate_list + "," + *generate_distrib_list;
        String::StringManip::SplitComma tokenizer(logs_list);
        String::SubString token;
        while (tokenizer.get_token(token))
        {
          String::StringManip::trim(token);
          Index::const_iterator cit = logs_indexes.find(token.str());
          if (cit != logs_indexes.end())
          {
            test_schedule.push_back(cit->second);
          }
          else
          {
            std::cerr << "Unknown log name: " << token << std::endl;
          }
        }
      }
    }
    if (test_schedule.empty())
    {
      for (std::size_t i = 0; i < LOGS_SIZE; ++i)
      {
         test_schedule.push_back(i);
      }
    }

    if (opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return false;
    }
    std::for_each(test_schedule.begin(), test_schedule.end(), Prober::s_init);
    return true;
  }

}

void
Prober::s_init(std::size_t i)
{
  LOGS[i]->init();
}

class FileGetter
{
public:

  void
  load_files(const std::string& dir_name)
  {
    Generics::DirSelect::directory_selector(
      dir_name.c_str(),
      *this,
      "[A-Z]*",
      Generics::DirSelect::DSF_DONT_RESOLVE_LINKS |
      Generics::DirSelect::DSF_RECURSIVE |
      Generics::DirSelect::DSF_EXCEPTION_ON_OPEN);
  }

  void
  operator ()(const char* full_path, const struct stat&)
    /*throw(eh::Exception)*/
  {
    const char* name = strrchr(full_path, '/');
    LogFileNameInfo name_info;
    parse_log_file_name(name, name_info);
    for (std::size_t i = 0; i < LOGS_SIZE; ++i)
    {
      if (LOGS[i]->name() == name_info.base_name)
      {
        try
        {
          LOGS[i]->load_log(full_path);
          std::cout << "File loaded: " << full_path << std::endl;
        }
        catch (const eh::Exception& ex)
        {
          std::cerr << "Cannot load file:" << full_path << std::endl
            << "Exception: " << ex.what() << std::endl;
        }
      }
    }
  }
};

void
load_folder()
{
  const std::string DIR_NAME = (root_path.installed() ? root_path :
    std::string("./In"));

  FileGetter loader;
  loader.load_files(DIR_NAME);
}

class SimpleWorker : public Generics::Task, public ReferenceCounting::DefaultImpl<>
{
public:
  SimpleWorker() noexcept
    : exitcode_(0)
  {}

  virtual void
  execute() /*throw(eh::Exception)*/
  {
    if (file_generator_mode)
    {
      std::cout << "Start logs generation  "
        << Generics::Time::get_time_of_day().get_gm_time() << std::endl;
    }
    Results fill_results, save_results, distrib_save_results, load_results;

    for (Schedule::const_iterator cit = test_schedule.begin();
      cit != test_schedule.end(); ++cit)
    {
      for (unsigned long fi = 0; fi < *files_count; ++fi)
      {
        exitcode_ += LOGS[*cit]->probe_log(opt_generate_req_ids.enabled());
      }
      if (print_time.enabled())
      {
        consider_result_(fill_results, LOGS[*cit]->name(),
          LOGS[*cit]->fill_time, LOGS[*cit]->fill_time_abs);
        consider_result_(save_results, LOGS[*cit]->name(),
          LOGS[*cit]->save_time, LOGS[*cit]->save_time_abs);
        consider_result_(distrib_save_results, LOGS[*cit]->name(),
          LOGS[*cit]->distrib_save_time, LOGS[*cit]->distrib_save_time_abs);
        consider_result_(load_results, LOGS[*cit]->name(),
          LOGS[*cit]->load_time, LOGS[*cit]->load_time_abs);
      }
    }
    if (file_generator_mode)
    {
      std::cout << "Finish logs generation "
        << Generics::Time::get_time_of_day().get_gm_time() << std::endl;
    }

    print_results_(fill_results, "Fill");
    print_results_(save_results, "Save");
    print_results_(distrib_save_results, "Distrib save");
    print_results_(load_results, "Load");
  }
  int
  get_exitcode() const noexcept
  {
    return exitcode_;
  }
protected:
  void
  consider_result_(Results& results,
    const char* log_name,
    const Generics::Time& time_val,
    const Generics::Time& time_val_abs)
  {
    if (time_val != Generics::Time::ZERO)
    {
      TimingResult resultator;
      resultator.log_name = log_name;
      resultator.time = time_val;
      resultator.time_abs = time_val_abs;
      results.insert(resultator);
    }
  }

  void
  print_results_(const Results& results, const char* name)
  {
    if (!results.empty())
    {
      std::string prefix(name);
      prefix += " time for '";
      TimingResult::prefix = prefix.c_str();
      std::for_each(results.begin(), results.end(), TimingResult());
      std::cout << "Total " << name << " CPU time: "
        << TimingResult::total_time << ", absolute time: "
        << TimingResult::total_time_abs << std::endl;
      TimingResult::total_time = Generics::Time::ZERO;
      TimingResult::total_time_abs = Generics::Time::ZERO;
    }
  }
private:
  int exitcode_;
};

/// Multi-thread executable functor
struct Tester
{
  Tester() noexcept : next_log_(0)
  {}

  void
  operator ()() /*throw(Exception)*/;
private:
  volatile _Atomic_word next_log_;
};

void
Tester::operator ()() /*throw(Exception)*/
{
  _Atomic_word old = __gnu_cxx::__exchange_and_add(&next_log_, 1);
  LOGS[test_schedule[old % test_schedule.size()]]->probe_log(true);
}

class MassiveGenerator : public SimpleWorker
{
public:
  MassiveGenerator(std::size_t threads = 4) noexcept
    : threads_(threads)
  {
    std::for_each(test_schedule.begin(), test_schedule.end(), Prober::s_init);
  }

  virtual void
  execute() /*throw(eh::Exception)*/
  {
    file_generator_mode = true;
    print_time = Generics::AppUtils::CheckOption(); // disable
    std::cout << "Perform write in " << threads_ << " thread" << std::endl;
    std::cout << "Start multiple logs generation" << std::endl;

    Generics::Time abs_time;
    {
      Generics::ScopedTimer dump_timer(abs_time);

      Tester test;
      TestCommons::MTTester<Tester&> mt_tester(test, threads_);

      mt_tester.run(*files_count * test_schedule.size(), 0,
        *files_count * test_schedule.size());
    }
    std::cout << "Finish logs generation, elapsed time: "
      << abs_time << std::endl << std::endl;
  }
private:
  std::size_t threads_;
};

int
main(int argc, char* argv[]) noexcept
{
  int exitcode = 0;

  try
  {
    if (!init(argc, argv))
    {
      return 0;
    }

    if (check_load_files.enabled())
    {
      load_folder();
      return 0;
    }
    Generics::Task_var worker;
    if (multi_thread_generation.enabled())
    {
/*      worker = new MassiveGenerator(1);
      worker->execute();
      worker = new MassiveGenerator(2);
      worker->execute();
      worker = new MassiveGenerator(4);
      worker->execute();*/
      worker = new MassiveGenerator(8);
/*      worker->execute();
      worker = new MassiveGenerator(16);
      worker->execute();
      worker = new MassiveGenerator(32);
      worker->execute();
      worker = new MassiveGenerator(49);
      worker->execute();
      worker = new MassiveGenerator(64);*/
    }
    else
    {
      worker = new SimpleWorker;
      exitcode = static_cast<SimpleWorker*>(worker.in())->get_exitcode();
    }
    worker->execute();

  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    ++exitcode;
  }

  return exitcode;
}

//
// Implementations
//
void
read_dir(
  const std::string& path,
  std::list<std::string>& entries)
  /*throw(std::exception)*/
{
  DIR *dir = opendir(path.c_str());

  if (!dir)
  {
    throw std::runtime_error("can't open dir " + path);
  }

  dirent* ent;

  while ((ent = readdir(dir)))
  {
    const std::string full_path = path + '/' + ent->d_name;
    struct stat st;

    if (ent->d_name[0] != '.' && ent->d_name[0] != '~' &&
        stat(full_path.c_str(), &st) == 0)
    {
      if (S_ISREG(st.st_mode))
      {
        entries.push_back(ent->d_name);
      }
    }
  }

  closedir (dir);
}

template <class LogTraits>
void
LogIoTester<LogTraits>::test_(CollectorType& collector)
  /*throw(Exception, eh::Exception)*/
{
  using ArchiveIfstream = AdServer::LogProcessing::ArchiveIfstream;

  if (!print_time.enabled() && (!file_generator_mode || debug.enabled()))
  {
    std::cout << "Testing " << std::setw(40) << std::setfill('.') <<
      std::left << name();
  }

  {
    CollectorType tmp_collector;
    collector.deep_copy(tmp_collector);
    if (generate_distrib_list.installed())
    {
      Filler::distribute_save(collector,
        distrib_save_time, distrib_save_time_abs);
    }
    else
    {
      Generics::ScopedTimer save_timer_abs(save_time_abs);
      Generics::ScopedCPUTimer save_timer(save_time);
      LogIoProxy<LogTraits>::save(tmp_collector, dir_name_);
    }
  }

  std::string rm_cmd("rm -rf ");
  rm_cmd += dir_name_;
  if (file_generator_mode)
  {
    if (print_time.enabled() && !debug.enabled())
    {
      system(rm_cmd.c_str());
    }
    else if (!no_commit_files.enabled())
    {
      typedef std::list<std::string> FileList;
      FileList files;

      read_dir(dir_name_ , files);
      for (FileList::const_iterator ci = files.begin();
           ci != files.end(); ++ci)
      {
        const std::string file_name(*ci + ".C");
        ::rename(ci->c_str(), file_name.c_str());
        std::ofstream ofs(
          std::string(dir_name_ + "/~" + *ci + ".commit").c_str());
      }
    }

    return;
  }

  LogSortingMap log_sorting_map;
  search_for_files(dir_name_, name(), log_sorting_map);

  CollectorType restored_collector;

  for (LogSortingMap::const_iterator it = log_sorting_map.begin();
    it != log_sorting_map.end(); ++it)
  {
    Generics::ScopedTimer load_timer_abs(load_time_abs);
    Generics::ScopedCPUTimer load_timer(load_time);

    const std::string& file_path = it->second;
    std::unique_ptr<std::istream> istream;
    if (ArchiveIfstream::is_archive(file_path))
    {
      istream = std::make_unique<ArchiveIfstream>(file_path);
    }
    else
    {
      istream = std::make_unique<std::ifstream>(file_path);
    }
    auto& ref_istream = *istream;

    typename LogTraits::HeaderType log_header;
    if (!(ref_istream >> log_header))
    {
      Stream::Error ostr;
      ostr << "LogIoTester<" << name()
        << ">: Failed to read log header";
      throw Exception(ostr);
    }
    if (log_header.version() != LogTraits::current_version())
    {
      Stream::Error ostr;
      ostr << "LogIoTester<" << name()
        << ">: Invalid log header version: "
        << log_header.version();
      throw Exception(ostr);
    }
    LogIoProxy<LogTraits>::load(restored_collector, ref_istream);
  }

  if (collector == restored_collector)
  {
    system(rm_cmd.c_str());
    if (!print_time.enabled())
    {
      std::cout << "SUCCESS" << std::endl;
    }
  }
  else
  {
    if (debug.enabled())
    {
      std::ostringstream mv_orig_dir_cmd_oss;
      mv_orig_dir_cmd_oss << "mv " << dir_name_ << ' ' <<
        name() << "_Original" << std::endl;
      system(mv_orig_dir_cmd_oss.str().c_str());

      const std::string DUMP_DIR_NAME =
        dir_name_ + "_Restored";
      system(("mkdir -p " + DUMP_DIR_NAME).c_str());

      LogIoProxy<LogTraits>::save(restored_collector, DUMP_DIR_NAME);
    }
    else
    {
      system(rm_cmd.c_str());
    }

    std::cout << "FAILED" << std::endl;
    Stream::Error ostr;
    ostr << "LogIoTester<" << name()
      << ">: Restored collector is not equal to the original collector";
    throw Exception(ostr);
  }
}
