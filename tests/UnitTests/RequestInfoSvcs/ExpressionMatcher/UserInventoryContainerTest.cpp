// @file ExpressionMatcher/UserInventoryContainerTest.cpp
#include <map>
#include <list>

#include <Generics/AppUtils.hpp>
#include <Generics/DirSelector.hpp>
#include <Generics/Uuid.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>

#include <RequestInfoSvcs/ExpressionMatcher/UserInventoryContainer.hpp>

using namespace AdServer;
using namespace AdServer::RequestInfoSvcs;

namespace
{
  const char DEFAULT_ROOT_PATH[] = "./";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char TEST_FOLDER[] = "/Inv/";

  const char USAGE[] =
    "UserInventoryContainerTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";

  const AdServer::Commons::UserId TEST_UID("BAAAAAAAAAAAAAAAAAAAAA..");

  void prepare_test_folders(
    AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap& chunk_folders)
  {
    system(("rm -r " + *root_path + TEST_FOLDER +
      " 2>/dev/null ; mkdir -p " + *root_path + TEST_FOLDER +
      " ; mkdir -p " + *root_path + TEST_FOLDER + "Chunk_0_4" +
      " ; mkdir -p " + *root_path + TEST_FOLDER + "Chunk_1_4" +
      " ; mkdir -p " + *root_path + TEST_FOLDER + "Chunk_2_4" +
      " ; mkdir -p " + *root_path + TEST_FOLDER + "Chunk_3_4").c_str());

    AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
      chunk_folders,
      (*root_path + TEST_FOLDER).c_str());
  }
}

typedef std::list<InventoryActionProcessor::InventoryInfo>
  InventoryInfoList;

struct NullInvProcessor:
  public virtual InventoryActionProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
  void
  process_request(const InventoryInfo& /*inv_info*/)
    /*throw(InventoryActionProcessor::Exception)*/
  {}

  void
  process_user(const InventoryUserInfo& /*inv_info*/)
    /*throw(InventoryActionProcessor::Exception)*/
  {}

protected:
  virtual
  ~NullInvProcessor() noexcept = default;
};

class CollInvTestProcessor:
  public virtual InventoryActionProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  void
  process_request(const InventoryInfo& inv_info)
    /*throw(InventoryActionProcessor::Exception)*/
  {
    result_.push_back(inv_info);
  }

  void
  process_user(const InventoryUserInfo& inv_info)
    /*throw(InventoryActionProcessor::Exception)*/
  {
    result_.push_back(inv_info);
  }

  const InventoryInfoList&
  result() const
  {
    return result_;
  }

protected:
  virtual
  ~CollInvTestProcessor() noexcept = default;

private:
  InventoryInfoList result_;
};

typedef ReferenceCounting::SmartPtr<CollInvTestProcessor>
  CollInvTestProcessor_var;

class TestBase : public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  static const unsigned long CPM_FACTOR = 1000 * 100;

  int
  run() noexcept
  {
    try
    {
      Logging::Logger_var logger(new Logging::Null::Logger);

      CollInvTestProcessor_var inv_processor(
        new CollInvTestProcessor());

      AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_folders;
      prepare_test_folders(chunk_folders);

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(1));

      UserInventoryInfoContainer_var inv_container(
        new UserInventoryInfoContainer(
          logger,
          Generics::Time::ONE_DAY * 3,
          inv_processor,
          0,
          false, // adrequest_anonymize
          chunk_folders.size(), // chunks_number
          chunk_folders,
          "Inv",
          cache,
          AdServer::ProfilingCommons::LevelMapTraits(
            AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
            10*1024*1024,
            10*1024*1024,
            20*1024*1024,
            20,
            Generics::Time::ZERO)
          ));

      inv_container->activate_object();

      run_impl(inv_container);

      inv_container->deactivate_object();
      inv_container->wait_object();

      InventoryInfoList etalon;
      bool check_inventory_etalon = fill_inventory_etalon(etalon);

      if(check_inventory_etalon)
      {
        // inventory check
        const InventoryInfoList& result = inv_processor->result();

        InventoryInfoList::const_iterator et_it = etalon.begin();
        InventoryInfoList::const_iterator res_it = result.begin();

        int i = 0;

        for(; et_it != etalon.end() && res_it != result.end();
          ++et_it, ++res_it, ++i)
        {
          if(!(*et_it == *res_it))
          {
            Stream::Error ostr;
            ostr << "inventory result non equal to standard - element #" << i <<
              ": " << std::endl << ">>> standard >>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
            et_it->print(ostr, "  ");
            ostr << "=== result =======================" << std::endl;
            res_it->print(ostr, "  ");
            ostr << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
            throw Exception(ostr);
          }
        }

        if(!(et_it == etalon.end() && res_it == result.end()))
        {
          Stream::Error ostr;
          ostr << "inventory result size = " << result.size() <<
            " non equal to standard = " << etalon.size() << ". Result: " << std::endl;
          int i = 0;
          for(InventoryInfoList::const_iterator e_it = result.begin();
            e_it != result.end();
            ++e_it, ++i)
          {
            ostr << "===== #" << i << " =======" << std::endl;
            e_it->print(ostr, "  ");
          }
          throw Exception(ostr);
        }
      }

      std::cout << "Test '" << name() << "': OK" << std::endl;

      return 0;
    }
    catch (const eh::Exception& ex)
    {
      std::cerr << "Test '" << name() << "': " << ex.what() << std::endl;
    }

    return 1;
  }

  virtual const char*
  name() = 0;

  virtual void
  run_impl(UserInventoryInfoContainer* cont) = 0;

  virtual bool
  fill_inventory_etalon(InventoryInfoList& /*etalon*/)
  {
    return false;
  }

  typedef InventoryActionProcessor::InventoryInfo::ChannelImpCounter ChannelImpCounter;

  static
  MatchRequestProcessor::MatchInfo
  base_match_info(const Generics::Time& date) noexcept
  {
    MatchRequestProcessor::MatchInfo match_info;
    match_info.user_id = TEST_UID;
    match_info.time = date;
    match_info.isp_time = date;
    match_info.placement_colo_time = date;
    match_info.cost_threshold = RevenueDecimal::ZERO;
    match_info.max_text_ads = 0;
    match_info.colo_id = 1;
    match_info.triggered_expression_channels.fill();
    return match_info;
  }

  static
  MatchRequestProcessor::MatchInfo
  simple_match_info(const Generics::Time& date) noexcept
  {
    MatchRequestProcessor::MatchInfo match_info = base_match_info(date);
    match_info.country_code = "RU";
    return match_info;
  }

  static
  MatchRequestProcessor::MatchInfo
  simple_match_info(const char* date) noexcept
  {
    return simple_match_info(
      Generics::Time(String::SubString(date), "%Y-%m-%d"));
  }

  static
  InventoryActionProcessor::InventoryInfo
  simple_etalon(
    const Generics::Time& date,
    const char* country_code = "RU") noexcept
  {
    InventoryActionProcessor::InventoryInfo inv_info;
    inv_info.time = date;
    inv_info.colo_id = 1;
    inv_info.country_code = country_code;
    inv_info.impop_ecpm = 0;
    return inv_info;
  }

  static
  InventoryActionProcessor::InventoryInfo
  simple_etalon(
    const char* date, const char* country_code = "RU") noexcept
  {
    return simple_etalon(
      Generics::Time(String::SubString(date), "%Y-%m-%d"),
      country_code);
  }

  static
  void daily_process(
    UserInventoryInfoContainer* cont,
    const UserInventoryInfoContainer::InventoryDailyMatchInfo& inv_daily_info)
  {
    UserIdList all_users;
    cont->all_users(all_users);

    for (UserIdList::const_iterator user_it = all_users.begin();
      user_it != all_users.end();
      ++user_it)
    {
      Generics::Time last_daily_processing_time;

      if (!cont->get_last_daily_processing_time(*user_it,
        last_daily_processing_time))
      {
        Stream::Error ostr;
        ostr << "Daily process user must exists.";
        throw Exception(ostr);
      }
      else
      {
        UserInventoryInfoContainer::InventoryDailyMatchInfo
          inv_daily_info_copy(inv_daily_info);
        inv_daily_info_copy.user_id = *user_it;
        cont->process_user(inv_daily_info_copy);
      }
    }
  }

protected:
  virtual
  ~TestBase() noexcept = default;
};

typedef ReferenceCounting::SmartPtr<TestBase> TestBase_var;
typedef std::list<TestBase_var> TestList;

class TestChannelByCPM: public TestBase
{
public:
  const char* name()
  {
    return "ChannelByCPM";
  }

  void
  run_impl(UserInventoryInfoContainer* cont)
  {
    { // #0
      MatchRequestProcessor::MatchInfo match_info = base_match_info(
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
      match_info.tag_size = "test";
      match_info.sizes.insert(match_info.tag_size);
      match_info.sizes.insert(AdServer::Commons::ImmutableString("test2"));
      match_info.country_code = "RU";
      match_info.cost_threshold = RevenueDecimal(false, 20, 0);
      match_info.triggered_expression_channels->push_back(1);
      match_info.triggered_cpm_expression_channels.insert(1);

      cont->process_match_request(match_info);
    }

    { // #1
      MatchRequestProcessor::MatchInfo match_info = base_match_info(
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
      match_info.tag_size = "test";
      match_info.sizes.insert(match_info.tag_size);
      match_info.sizes.insert(AdServer::Commons::ImmutableString("test2"));
      match_info.country_code = "RU";
      match_info.cost_threshold = RevenueDecimal(false, 10, 0);

      match_info.triggered_expression_channels->push_back(1);
      match_info.triggered_expression_channels->push_back(2);
      match_info.triggered_cpm_expression_channels.insert(1);
      match_info.triggered_cpm_expression_channels.insert(2);

      cont->process_match_request(match_info);
    }

    { // #2
      MatchRequestProcessor::MatchInfo match_info = base_match_info(
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
      match_info.country_code = "RU";

      match_info.triggered_expression_channels->push_back(1);
      match_info.triggered_expression_channels->push_back(2);
      match_info.triggered_expression_channels->push_back(3);
      match_info.triggered_cpm_expression_channels.insert(1);
      match_info.triggered_cpm_expression_channels.insert(2);
      match_info.triggered_cpm_expression_channels.insert(3);

      cont->process_match_request(match_info);
    }

    { // #3
      MatchRequestProcessor::MatchInfo match_info = base_match_info(
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
      match_info.tag_size = "test";
      match_info.sizes.insert(match_info.tag_size);
      match_info.sizes.insert(AdServer::Commons::ImmutableString("test2"));
      match_info.country_code = "RU";
      MatchRequestProcessor::MatchInfo::AdSlot display_ad;
      display_ad.avg_revenue = RevenueDecimal(false, 10, 0);
      display_ad.imp_channels.insert(2);
      match_info.display_ad = display_ad;

      match_info.triggered_expression_channels->push_back(1);
      match_info.triggered_expression_channels->push_back(2);
      match_info.triggered_expression_channels->push_back(3);
      match_info.triggered_cpm_expression_channels.insert(1);
      match_info.triggered_cpm_expression_channels.insert(2);
      match_info.triggered_cpm_expression_channels.insert(3);

      cont->process_match_request(match_info);
    }
  }

  bool
  fill_inventory_etalon(InventoryInfoList& inv_info_list)
  {
    { // #0
      InventoryActionProcessor::InventoryInfo inv_info;
      inv_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      inv_info.colo_id = 1;
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test"));
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test2"));
      inv_info.country_code = "RU";
      inv_info.impop_ecpm = 20 * CPM_FACTOR;

      inv_info.total_appear_channels.push_back(1);
      inv_info.active_appear_channels.push_back(1);
      inv_info.display_appears.impop_appear_channels.push_back(1);
      inv_info.display_appears.impop_no_imp_appear_channels.push_back(1);

      inv_info.display_imps.impop_no_imp_channels.emplace_back(1, ChannelImpCounter(
        1, RevenueDecimal(false, 20, 0)));

      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test"), 1));
      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test2"), 1));

      inv_info_list.push_back(inv_info);
    }

    { // #1
      InventoryActionProcessor::InventoryInfo inv_info;
      inv_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      inv_info.colo_id = 1;
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test"));
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test2"));
      inv_info.country_code = "RU";
      inv_info.impop_ecpm = 10 * CPM_FACTOR;

      inv_info.total_appear_channels.push_back(2);
      inv_info.active_appear_channels.push_back(2);

      inv_info.display_appears.impop_appear_channels.push_back(2);
      inv_info.display_appears.impop_no_imp_appear_channels.push_back(2);

      inv_info.display_imps.impop_no_imp_channels.emplace_back(1,
        ChannelImpCounter(1, RevenueDecimal(false, 10, 0)));
      inv_info.display_imps.impop_no_imp_channels.emplace_back(2,
        ChannelImpCounter(1, RevenueDecimal(false, 10, 0)));

      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test"), 1));
      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test"), 2));
      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test2"), 1));
      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test2"), 2));

      inv_info.disappear_channel_ecpms.push_back(
        ChannelECPM(AdServer::Commons::ImmutableString("test"),
          1, 20 * CPM_FACTOR));
      inv_info.disappear_channel_ecpms.push_back(
        ChannelECPM(AdServer::Commons::ImmutableString("test2"),
          1, 20 * CPM_FACTOR));

      inv_info_list.push_back(inv_info);
    }

    { // #2
      InventoryActionProcessor::InventoryInfo inv_info;
      inv_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      inv_info.colo_id = 1;
      inv_info.country_code = "RU";
      inv_info.impop_ecpm = 0;

      inv_info.total_appear_channels.push_back(3);
      inv_info.active_appear_channels.push_back(3);

      inv_info_list.push_back(inv_info);
    }

    { // #3
      InventoryActionProcessor::InventoryInfo inv_info;
      inv_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      inv_info.colo_id = 1;
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test"));
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test2"));
      inv_info.country_code = "RU";
      inv_info.impop_ecpm = 10 * CPM_FACTOR;

      inv_info.display_appears.impop_appear_channels.push_back(3);
      inv_info.display_appears.imp_other_appear_channels.push_back(1);
      inv_info.display_appears.imp_other_appear_channels.push_back(3);
      inv_info.display_appears.imp_appear_channels.push_back(2);

      inv_info.display_imps.imp_other_channels.emplace_back(1, ChannelImpCounter(
        1, RevenueDecimal(false, 10, 0)));
      inv_info.display_imps.imp_other_channels.emplace_back(3, ChannelImpCounter(
        1, RevenueDecimal(false, 10, 0)));
      inv_info.display_imps.imp_channels.emplace_back(2, RevenueDecimal(false, 10, 0));

      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test"), 3));
      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test2"), 3));

      inv_info_list.push_back(inv_info);
    }

    return true;
  }

protected:
  virtual
  ~TestChannelByCPM() noexcept = default;
};

class Test_ADSC_1756 : public TestBase
{
public:
  const char*
  name()
  {
    return "Test-ADSC-1756";
  }

  void
  run_impl(UserInventoryInfoContainer* cont)
  {
    {
      MatchRequestProcessor::MatchInfo match_info = base_match_info(
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
      match_info.tag_size = "test";
      match_info.sizes.insert(match_info.tag_size);
      match_info.country_code = "RU";
      match_info.cost_threshold = RevenueDecimal(false, 20, 0);
      match_info.triggered_expression_channels->push_back(1);
      match_info.triggered_cpm_expression_channels.insert(1);

      cont->process_match_request(match_info);
    }

    {
      MatchRequestProcessor::MatchInfo match_info = base_match_info(
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
      match_info.tag_size = "test";
      match_info.sizes.insert(match_info.tag_size);
      match_info.country_code = "RU";
      match_info.cost_threshold = RevenueDecimal(false, 10, 0);

      match_info.triggered_expression_channels->push_back(1);
      match_info.triggered_expression_channels->push_back(2);
      match_info.triggered_cpm_expression_channels.insert(1);
      match_info.triggered_cpm_expression_channels.insert(2);

      cont->process_match_request(match_info);
    }
  }

  bool
  fill_inventory_etalon(InventoryInfoList& inv_info_list)
  {
    {
      InventoryActionProcessor::InventoryInfo inv_info;
      inv_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      inv_info.colo_id = 1;
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test"));
      inv_info.country_code = "RU";
      inv_info.impop_ecpm = 20 * CPM_FACTOR;

      inv_info.total_appear_channels.push_back(1);
      inv_info.active_appear_channels.push_back(1);
      inv_info.display_appears.impop_appear_channels.push_back(1);
      inv_info.display_appears.impop_no_imp_appear_channels.push_back(1);

      inv_info.display_imps.impop_no_imp_channels.emplace_back(1, ChannelImpCounter(
        1, RevenueDecimal(false, 20, 0)));

      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test"), 1));

      inv_info_list.push_back(inv_info);
    }

    {
      InventoryActionProcessor::InventoryInfo inv_info;
      inv_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      inv_info.colo_id = 1;
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test"));
      inv_info.country_code = "RU";
      inv_info.impop_ecpm = 10 * CPM_FACTOR;

      inv_info.total_appear_channels.push_back(2);
      inv_info.active_appear_channels.push_back(2);
      inv_info.display_appears.impop_appear_channels.push_back(2);
      inv_info.display_appears.impop_no_imp_appear_channels.push_back(2);

      inv_info.display_imps.impop_no_imp_channels.emplace_back(1, ChannelImpCounter(
        1, RevenueDecimal(false, 10, 0)));
      inv_info.display_imps.impop_no_imp_channels.emplace_back(2, ChannelImpCounter(
        1, RevenueDecimal(false, 10, 0)));

      inv_info.disappear_channel_ecpms.push_back(
        ChannelECPM(AdServer::Commons::ImmutableString("test"),
          1, 20 * CPM_FACTOR));

      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test"), 1));
      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test"), 2));

      inv_info_list.push_back(inv_info);
    }

    return true;
  }

protected:
  virtual
  ~Test_ADSC_1756() noexcept = default;
};

class TestImpMerge : public TestBase
{
public:
  const char* name()
  {
    return "TestImpMerge";
  }

  void
  run_impl(UserInventoryInfoContainer* cont)
  {
    { // #0
      MatchRequestProcessor::MatchInfo match_info = base_match_info(
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
      match_info.tag_size = "test";
      match_info.sizes.insert(match_info.tag_size);
      match_info.country_code = "RU";
      match_info.cost_threshold = RevenueDecimal(false, 10, 0);
      MatchRequestProcessor::MatchInfo::AdSlot display_ad;
      display_ad.avg_revenue = RevenueDecimal(false, 10, 0);
      display_ad.imp_channels.insert(1);
      match_info.display_ad = display_ad;

      match_info.triggered_expression_channels->push_back(1);
      match_info.triggered_cpm_expression_channels.insert(1);

      cont->process_match_request(match_info);
    }

    { // #1
      MatchRequestProcessor::MatchInfo match_info = base_match_info(
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
      match_info.tag_size = "test";
      match_info.sizes.insert(match_info.tag_size);
      match_info.country_code = "RU";
      match_info.cost_threshold = RevenueDecimal(false, 20, 0);
      MatchRequestProcessor::MatchInfo::AdSlot display_ad;
      display_ad.avg_revenue = RevenueDecimal(false, 20, 0);
      display_ad.imp_channels.insert(2);
      match_info.display_ad = display_ad;

      match_info.triggered_expression_channels->push_back(1);
      match_info.triggered_expression_channels->push_back(2);
      match_info.triggered_cpm_expression_channels.insert(1);
      match_info.triggered_cpm_expression_channels.insert(2);

      cont->process_match_request(match_info);
    }

    { // #2
      MatchRequestProcessor::MatchInfo match_info = base_match_info(
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
      match_info.tag_size = "test";
      match_info.sizes.insert(match_info.tag_size);
      match_info.country_code = "RU";
      match_info.cost_threshold = RevenueDecimal(false, 10, 0);
      MatchRequestProcessor::MatchInfo::AdSlot display_ad;
      display_ad.avg_revenue = RevenueDecimal(false, 15, 0);
      display_ad.imp_channels.insert(2);
      match_info.display_ad = display_ad;

      match_info.triggered_expression_channels->push_back(1);
      match_info.triggered_expression_channels->push_back(2);
      match_info.triggered_cpm_expression_channels.insert(1);
      match_info.triggered_cpm_expression_channels.insert(2);

      cont->process_match_request(match_info);
    }
  }

  bool
  fill_inventory_etalon(InventoryInfoList& inv_info_list)
  {
    { // #0
      InventoryActionProcessor::InventoryInfo inv_info;
      inv_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      inv_info.colo_id = 1;
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test"));
      inv_info.country_code = "RU";
      inv_info.impop_ecpm = 10 * CPM_FACTOR;

      inv_info.total_appear_channels.push_back(1);
      inv_info.active_appear_channels.push_back(1);

      inv_info.display_appears.impop_appear_channels.push_back(1);
      inv_info.display_appears.imp_appear_channels.push_back(1);

      inv_info.display_imps.imp_channels.emplace_back(1, RevenueDecimal(false, 10, 0));

      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test"), 1));

      inv_info_list.push_back(inv_info);
    }

    { // #1
      InventoryActionProcessor::InventoryInfo inv_info;
      inv_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      inv_info.colo_id = 1;
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test"));
      inv_info.country_code = "RU";
      inv_info.impop_ecpm = 20 * CPM_FACTOR;

      inv_info.total_appear_channels.push_back(2);
      inv_info.active_appear_channels.push_back(2);
      inv_info.display_appears.impop_appear_channels.push_back(2);
      inv_info.display_appears.imp_appear_channels.push_back(2);
      inv_info.display_appears.imp_other_appear_channels.push_back(1);

      inv_info.display_imps.imp_channels.emplace_back(2, RevenueDecimal(false, 20, 0));
      inv_info.display_imps.imp_other_channels.emplace_back(1, ChannelImpCounter(
        1, RevenueDecimal(false, 20, 0)));

      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test"), 2));

      inv_info_list.push_back(inv_info);
    }

    { // #2
      InventoryActionProcessor::InventoryInfo inv_info;
      inv_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      inv_info.colo_id = 1;
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test"));
      inv_info.country_code = "RU";
      inv_info.impop_ecpm = 15 * CPM_FACTOR;

      inv_info.display_imps.imp_channels.emplace_back(2, RevenueDecimal(false, 15, 0));
      inv_info.display_imps.imp_other_channels.emplace_back(1, ChannelImpCounter(
        1, RevenueDecimal(false, 15, 0)));

      inv_info.disappear_channel_ecpms.push_back(
        ChannelECPM(AdServer::Commons::ImmutableString("test"),
          2, 20 * CPM_FACTOR));

      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test"), 2));

      inv_info_list.push_back(inv_info);
    }

    return true;
  }

protected:
  virtual
  ~TestImpMerge() noexcept = default;
};

class TestSimpleDailyProcessing : public TestBase
{
public:
  const char* name()
  {
    return "SimpleDailyProcessing";
  }

  void
  run_impl(UserInventoryInfoContainer* cont)
  {
    {
      MatchRequestProcessor::MatchInfo match_info = base_match_info(
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
      match_info.tag_size = "test";
      match_info.sizes.insert(match_info.tag_size);
      match_info.country_code = "RU";
      MatchRequestProcessor::MatchInfo::AdSlot display_ad;
      display_ad.avg_revenue = RevenueDecimal(false, 20, 0);
      display_ad.imp_channels.insert(1);
      match_info.display_ad = display_ad;
      match_info.triggered_expression_channels->push_back(1);
      match_info.triggered_cpm_expression_channels.insert(1);

      cont->process_match_request(match_info);
    }

    UserIdList all_users;
    cont->all_users(all_users);

    for (UserIdList::const_iterator user_it = all_users.begin();
      user_it != all_users.end();
      ++user_it)
    {
      Generics::Time last_daily_processing_time;

      if (!cont->get_last_daily_processing_time(*user_it,
        last_daily_processing_time))
      {
        Stream::Error ostr;
        ostr << "Daily process user must exists.";
        throw Exception(ostr);
      }
      else
      {
        UserInventoryInfoContainer::InventoryDailyMatchInfo inv_daily_info;
        inv_daily_info.user_id = *user_it;
        inv_daily_info.time = last_daily_processing_time +
          Generics::Time::ONE_DAY;
        inv_daily_info.colo_id = 1;
        inv_daily_info.triggered_expression_channels.push_back(1);
        inv_daily_info.triggered_expression_channels.push_back(3);
        cont->process_user(inv_daily_info);
      }
    }
  }

  bool
  fill_inventory_etalon(InventoryInfoList& inv_info_list)
  {
    {
      InventoryActionProcessor::InventoryInfo inv_info;
      inv_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      inv_info.colo_id = 1;
      inv_info.sizes.insert(AdServer::Commons::ImmutableString("test"));
      inv_info.country_code = "RU";
      inv_info.impop_ecpm = 20 * CPM_FACTOR;

      inv_info.total_appear_channels.push_back(1);
      inv_info.active_appear_channels.push_back(1);
      inv_info.display_appears.impop_appear_channels.push_back(1);
      inv_info.display_appears.imp_appear_channels.push_back(1);
      inv_info.display_imps.imp_channels.emplace_back(1, RevenueDecimal(false, 20, 0));

      inv_info.appear_channel_ecpms.push_back(
        SizeChannel(AdServer::Commons::ImmutableString("test"), 1));

      inv_info_list.push_back(inv_info);
    }

    {
      InventoryActionProcessor::InventoryInfo inv_info;
      inv_info.time =
        Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d");
      inv_info.colo_id = 1;
      inv_info.impop_ecpm = 0;

      inv_info.total_appear_channels.push_back(1);
      inv_info.total_appear_channels.push_back(3);


      inv_info_list.push_back(inv_info);
    }

    return true;
  }

protected:
  virtual
  ~TestSimpleDailyProcessing() noexcept = default;
};

class TestExtendedDailyProcessing: public TestBase
{
public:
  /**
   * impression (day) -> logged
   * daily processing(day + 1) -> logged
   * daily processing(day + 1)
   * impression (day) -> logged
   * impression (day + 3) -> logged
   * daily processing(day + 2)
   * daily processing(day + 3)
   * daily processing(day + 4) -> logged
   */
  const char* name()
  {
    return "TestExtendedDailyProcessing";
  }

  void
  run_impl(UserInventoryInfoContainer* cont)
  {
    {
      MatchRequestProcessor::MatchInfo match_info(
        simple_match_info("2008-01-05"));
      match_info.triggered_expression_channels->push_back(1);
      cont->process_match_request(match_info);
    }

    for(int i = 0; i < 2; ++i)
    {
      UserInventoryInfoContainer::InventoryDailyMatchInfo inv_daily_info;
      inv_daily_info.time =
        Generics::Time(String::SubString("2008-01-06"), "%Y-%m-%d");
      inv_daily_info.colo_id = 1;
      inv_daily_info.triggered_expression_channels.push_back(1);

      daily_process(cont, inv_daily_info);
    }

    {
      MatchRequestProcessor::MatchInfo match_info(
        simple_match_info("2008-01-05"));
      match_info.triggered_expression_channels->push_back(1);
      cont->process_match_request(match_info);
    }

    {
      MatchRequestProcessor::MatchInfo match_info(
        simple_match_info("2008-01-08"));
      match_info.triggered_expression_channels->push_back(1);
      cont->process_match_request(match_info);
    }

    {
      UserInventoryInfoContainer::InventoryDailyMatchInfo inv_daily_info;
      inv_daily_info.time =
        Generics::Time(String::SubString("2008-01-07"), "%Y-%m-%d");
      inv_daily_info.colo_id = 1;
      inv_daily_info.triggered_expression_channels.push_back(1);
      daily_process(cont, inv_daily_info);
    }

    {
      UserInventoryInfoContainer::InventoryDailyMatchInfo inv_daily_info;
      inv_daily_info.time =
        Generics::Time(String::SubString("2008-01-08"), "%Y-%m-%d");
      inv_daily_info.colo_id = 1;
      inv_daily_info.triggered_expression_channels.push_back(1);
      daily_process(cont, inv_daily_info);
    }

    {
      UserInventoryInfoContainer::InventoryDailyMatchInfo inv_daily_info;
      inv_daily_info.time =
        Generics::Time(String::SubString("2008-01-09"), "%Y-%m-%d");
      inv_daily_info.colo_id = 1;
      inv_daily_info.triggered_expression_channels.push_back(1);
      daily_process(cont, inv_daily_info);
    }
  }

  bool
  fill_inventory_etalon(InventoryInfoList& inv_info_list)
  {
    {
      InventoryActionProcessor::InventoryInfo inv_info(
        simple_etalon("2008-01-05"));
      inv_info.total_appear_channels.push_back(1);
      inv_info.active_appear_channels.push_back(1);
      inv_info_list.push_back(inv_info);
    }

    {
      InventoryActionProcessor::InventoryInfo inv_info(
        simple_etalon("2008-01-06", ""));
      inv_info.total_appear_channels.push_back(1);
      inv_info_list.push_back(inv_info);
    }

    inv_info_list.push_back(simple_etalon("2008-01-05"));

    {
      InventoryActionProcessor::InventoryInfo inv_info(
        simple_etalon("2008-01-08"));
      inv_info.active_appear_channels.push_back(1);
      inv_info.total_appear_channels.push_back(1);
      inv_info_list.push_back(inv_info);
    }

    {
      InventoryActionProcessor::InventoryInfo inv_info(
        simple_etalon("2008-01-09", ""));
      inv_info.total_appear_channels.push_back(1);
      inv_info_list.push_back(inv_info);
    }

    return true;
  }

protected:
  virtual
  ~TestExtendedDailyProcessing() noexcept = default;
};

class TestExpireClear : public TestBase
{
public:
  const char* name()
  {
    return "TestExpireClear";
  }

  void
  run_impl(UserInventoryInfoContainer* cont)
  {
    Generics::Time date(String::SubString("2008-01-01"), "%Y-%m-%d");

    for(int i = 0; i < 4; ++i)
    {
      MatchRequestProcessor::MatchInfo match_info(
        simple_match_info(date));
      match_info.triggered_expression_channels->push_back(1);
      MatchRequestProcessor::MatchInfo::AdSlot display_ad;
      display_ad.avg_revenue = RevenueDecimal(false, 15, 0);
      display_ad.imp_channels.insert(1);
      match_info.display_ad = display_ad;
      cont->process_match_request(match_info);
      date += Generics::Time::ONE_DAY;
    }

    MatchRequestProcessor::MatchInfo match_info(
      simple_match_info("2008-01-01"));
    match_info.triggered_expression_channels->push_back(2);
    MatchRequestProcessor::MatchInfo::AdSlot display_ad;
    display_ad.avg_revenue = RevenueDecimal(false, 15, 0);
    display_ad.imp_channels.insert(2);
    match_info.display_ad = display_ad;
    cont->process_match_request(match_info);

    Generics::ConstSmartMemBuf_var mem_buf = cont->get_profile(TEST_UID);
    UserChannelInventoryProfileReader profile_reader(
      mem_buf->membuf().data(),
      mem_buf->membuf().size());
    if(profile_reader.days().size() > 3)
    {
      Stream::Error ostr;
      ostr << "Profile contains more then 3 days";
      throw Exception(ostr);
    }
  }

  bool
  fill_inventory_etalon(InventoryInfoList& inv_info_list)
  {
    Generics::Time date(String::SubString("2008-01-01"), "%Y-%m-%d");

    for(int i = 0; i < 4; ++i)
    {
      InventoryActionProcessor::InventoryInfo inv_info(simple_etalon(date));
      inv_info.total_appear_channels.push_back(1);
      inv_info.active_appear_channels.push_back(1);
      inv_info.auction_type = CampaignSvcs::AT_MAX_ECPM;
      inv_info_list.push_back(inv_info);
      date += Generics::Time::ONE_DAY;
    }

    return true;
  }

protected:
  virtual
  ~TestExpireClear() noexcept = default;
};

class TestExpire : public TestBase
{
public:
  const char* name()
  {
    return "TestExpire";
  }

  void
  run_impl(UserInventoryInfoContainer* cont)
  {
    Generics::Time base_date(
      Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));

    MatchRequestProcessor::MatchInfo match_info(
      simple_match_info(base_date));

    {
      match_info.triggered_expression_channels->push_back(1);
      MatchRequestProcessor::MatchInfo::AdSlot display_ad;
      display_ad.avg_revenue = RevenueDecimal(false, 15, 0);
      display_ad.imp_channels.insert(1);
      match_info.display_ad = display_ad;
      cont->process_match_request(match_info);
    }

    {
      match_info.time = base_date - Generics::Time::ONE_DAY * 31;
      match_info.isp_time = match_info.time;
      match_info.placement_colo_time = match_info.time;
      cont->process_match_request(match_info);
    }
  }

  bool
  fill_inventory_etalon(InventoryInfoList& inv_info_list)
  {
    InventoryActionProcessor::InventoryInfo inv_info(simple_etalon(
      Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d")));
    inv_info.total_appear_channels.push_back(1);
    inv_info.active_appear_channels.push_back(1);
    inv_info.auction_type = CampaignSvcs::AT_MAX_ECPM;
    inv_info_list.push_back(inv_info);
    return true;
  }

protected:
  virtual
  ~TestExpire() noexcept = default;
};

int
perf_test()
{
  try
  {
    Logging::Logger_var logger(new Logging::Null::Logger);

    InventoryActionProcessor_var inv_processor(
      new NullInvProcessor());

    AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_folders;
    prepare_test_folders(chunk_folders);

    AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
      new AdServer::ProfilingCommons::ProfileMapFactory::Cache(1));

    UserInventoryInfoContainer_var inv_container(
      new UserInventoryInfoContainer(
        logger,
        Generics::Time::ONE_DAY * 3,
        inv_processor,
        0,
        false, // adrequest_anonymize
        chunk_folders.size(), // chunks_number
        chunk_folders,
        "Inv",
        cache,
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          10*1024*1024,
          10*1024*1024,
          20*1024*1024,
          20,
          Generics::Time::ZERO)
        ));

    inv_container->activate_object();

    MatchRequestProcessor::MatchInfo match_info = TestBase::base_match_info(
      Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
    match_info.tag_size = "test";
    match_info.sizes.insert(match_info.tag_size);
    match_info.country_code = "RU";
    match_info.cost_threshold = RevenueDecimal(false, 20, 0);
    for(unsigned long channel_id = 1; channel_id < 10000; ++channel_id)
    {
      match_info.triggered_expression_channels->push_back(channel_id);
      match_info.triggered_cpm_expression_channels.insert(channel_id);
    }

    Generics::CPUTimer timer;
    timer.start();

    for(int i = 0; i < 10000; ++i)
    {
      inv_container->process_match_request(match_info);
    }

    timer.stop();

    inv_container->deactivate_object();
    inv_container->wait_object();

    std::cout << "Test 'Performance': execution time = " <<
      timer.elapsed_time() << std::endl;

    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "Test 'Performance': " << ex.what() << std::endl;
  }

  return 1;
}

int
perf_test2()
{
  try
  {
    Logging::Logger_var logger(new Logging::Null::Logger);

    InventoryActionProcessor_var inv_processor(
      new NullInvProcessor());

    AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_folders;
    prepare_test_folders(chunk_folders);

    AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
      new AdServer::ProfilingCommons::ProfileMapFactory::Cache(1));

    UserInventoryInfoContainer_var inv_container(
      new UserInventoryInfoContainer(
        logger,
        Generics::Time::ONE_DAY * 3,
        inv_processor,
        0,
        false, // adrequest_anonymize
        chunk_folders.size(), // chunks_number
        chunk_folders,
        "Inv",
        cache,
        AdServer::ProfilingCommons::LevelMapTraits(
          AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
          10*1024*1024,
          10*1024*1024,
          20*1024*1024,
          20,
          Generics::Time::ZERO)
        ));

    inv_container->activate_object();

    const unsigned long MATCH_INFO_ARRAY_SIZE = 100;
    MatchRequestProcessor::MatchInfo match_info_array[MATCH_INFO_ARRAY_SIZE];

    for(unsigned long i = 0; i < MATCH_INFO_ARRAY_SIZE; ++i)
    {
      MatchRequestProcessor::MatchInfo& match_info = match_info_array[i];
      match_info = TestBase::base_match_info(
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
      match_info.tag_size = "test";
      match_info.sizes.insert(match_info.tag_size);
      match_info.country_code = "RU";
      match_info.cost_threshold = RevenueDecimal(false, 20, 0);
      for(unsigned long channel_id = i * 100;
          channel_id < i * 100 + 100; ++channel_id)
      {
        match_info.triggered_expression_channels->push_back(channel_id);
        match_info.triggered_cpm_expression_channels.insert(channel_id);
      }
    }

    Generics::CPUTimer timer;
    timer.start();

    for(int i = 0; i < 10000; ++i)
    {
      inv_container->process_match_request(match_info_array[i % MATCH_INFO_ARRAY_SIZE]);
    }

    timer.stop();

    inv_container->deactivate_object();
    inv_container->wait_object();

    std::cout << "Test 'Performance2': execution time = " <<
      timer.elapsed_time() << std::endl;

    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "Test 'Performance': " << ex.what() << std::endl;
  }

  return 1;
}

int
main(int argc, char** argv) noexcept
{
  try
  {
      Logging::Logger_var logger(new Logging::Null::Logger);

      CollInvTestProcessor_var inv_processor(
        new CollInvTestProcessor());

      AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_folders;
      prepare_test_folders(chunk_folders);

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(1));

      UserInventoryInfoContainer_var inv_container(
        new UserInventoryInfoContainer(
          logger,
          Generics::Time::ONE_DAY * 3,
          inv_processor,
          0,
          false, // adrequest_anonymize
          chunk_folders.size(), // chunks_number
          chunk_folders,
          "Inv",
          cache,
          AdServer::ProfilingCommons::LevelMapTraits(
            AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
            10*1024*1024,
            10*1024*1024,
            20*1024*1024,
            20,
            Generics::Time::ZERO)
          ));

      inv_container->activate_object();

      for(int i = 0; i < 100000; ++i)
      {
        MatchRequestProcessor::MatchInfo match_info = TestBase::base_match_info(
          Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"));
        match_info.user_id = Commons::RequestId::create_random_based();
        match_info.tag_size = "test";
        match_info.sizes.insert(match_info.tag_size);
        match_info.sizes.insert(AdServer::Commons::ImmutableString("test2"));
        match_info.country_code = "RU";
        match_info.cost_threshold = RevenueDecimal(false, 20, 0);
        match_info.triggered_expression_channels->push_back(1);
        match_info.triggered_cpm_expression_channels.insert(1);

        inv_container->process_match_request(match_info);
      }

      inv_container->deactivate_object();
      inv_container->wait_object();
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return 0;

  try
  {
    using namespace Generics::AppUtils;
    Args args;
    CheckOption opt_help;

    args.add(equal_name("path") || short_name("p"), root_path);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    int result = 0;
    TestList tests;
    tests.push_back(new Test_ADSC_1756());
    tests.push_back(new TestChannelByCPM());
    tests.push_back(new TestImpMerge());
    tests.push_back(new TestExpireClear());
    tests.push_back(new TestSimpleDailyProcessing());
    tests.push_back(new TestExtendedDailyProcessing());
    tests.push_back(new TestExpire());
    for (TestList::iterator it = tests.begin();
      it != tests.end(); ++it)
    {
      result += (*it)->run();
    }

    /*
    // performance testing
    {
      result += perf_test();
      result += perf_test2();
    }
    */

    return result;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return -1;
}
