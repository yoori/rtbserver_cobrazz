#include <map>
#include <list>

#include <Generics/AppUtils.hpp>

#include <RequestInfoSvcs/ExpressionMatcher/UserTriggerMatchContainer.hpp>

using namespace AdServer;
using namespace AdServer::RequestInfoSvcs;

namespace
{
  const char DEFAULT_ROOT_PATH[] = "./";

  const char TEST_FOLDER[] = "/TriggerMatch/";

  const char USAGE[] =
    "UserTriggerMatchContainer [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n"
    "  -m, --model : run modeling tests.\n";

  const AdServer::Commons::UserId TEST_UID("BAAAAAAAAAAAAAAAAAAAAA..");
  const AdServer::Commons::UserId TEST_TEMP_UID("TTTTTTTTTTTTTTTTTTTTTA..");

  const RevenueDecimal ONE_IMP_COUNTER("2.8");
  const RevenueDecimal ONE_NEGATE_IMP_COUNTER("-2.7");
  /*
  const RevenueDecimal ONE_IMP_COUNTER("3.5");
  const RevenueDecimal ONE_NEGATE_IMP_COUNTER("-3.75");
  */
  const RevenueDecimal ONE_IMP_SUM(1);
  const RevenueDecimal ONE_IMP_AND_NEGATE_COUNTER =
    ONE_IMP_COUNTER + ONE_NEGATE_IMP_COUNTER;
  const unsigned long TRIGGERS_IN_CHANNEL = 15;

  const unsigned long POSITIVE_TRIGGERS_GROUP_SIZE = 10;
  const unsigned long NEGATIVE_TRIGGERS_GROUP_SIZE = 10;
  const unsigned long MAX_TRIGGER_VISITS = 10;
}

struct CollTriggerActionProcessor:
  public virtual AdServer::RequestInfoSvcs::TriggerActionProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  virtual void process_triggers_impression(
    const TriggersMatchInfo& match_info)
    /*throw(TriggerActionProcessor::Exception)*/
  {
    page_imps_.add(match_info.page_matches);
    search_imps_.add(match_info.search_matches);
    url_imps_.add(match_info.url_matches);
  }

  virtual void process_triggers_click(
    const TriggersMatchInfo& match_info)
    /*throw(TriggerActionProcessor::Exception)*/
  {
    page_clicks_.add(match_info.page_matches);
    search_clicks_.add(match_info.search_matches);
    url_clicks_.add(match_info.url_matches);
  }

  const MatchCountMap& page_imps() const
  {
    return page_imps_;
  }

  const MatchCountMap& search_imps() const
  {
    return search_imps_;
  }

  const MatchCountMap& url_imps() const
  {
    return url_imps_;
  }

  const MatchCountMap& page_clicks() const
  {
    return page_clicks_;
  }

  const MatchCountMap& search_clicks() const
  {
    return search_clicks_;
  }

  const MatchCountMap& url_clicks() const
  {
    return url_clicks_;
  }

  void print(std::ostream& out, const char* offset) const
  {
    std::string sub_offset(offset);
    sub_offset += "  ";

    out << offset << "page impressions:" << std::endl;
    page_imps_.print(out, sub_offset.c_str());
    out << offset << "page clicks:" << std::endl;
    page_clicks_.print(out, sub_offset.c_str());

    out << offset << "search impressions:" << std::endl;
    search_imps_.print(out, sub_offset.c_str());
    out << offset << "search clicks:" << std::endl;
    search_clicks_.print(out, sub_offset.c_str());

    out << offset << "url impressions:" << std::endl;
    url_imps_.print(out, sub_offset.c_str());
    out << offset << "url clicks:" << std::endl;
    url_clicks_.print(out, sub_offset.c_str());
  }

  bool check_page_trigger(
    const char* prefix,
    unsigned long channel_trigger_id,
    unsigned long channel_id,
    const std::list<RevenueDecimal>& expected)
  {
    bool zero_expected = std::find(
      expected.begin(), expected.end(), RevenueDecimal::ZERO) != expected.end();

    MatchCountMap::const_iterator mit = page_imps_.find(
      MatchCounterKey(channel_trigger_id, channel_id));

    if(mit == page_imps_.end())
    {
      if(!zero_expected)
      {
        std::cerr << prefix <<
          ": unexpected null match counter for channel_trigger_id = " <<
          channel_trigger_id << std::endl;
        return false;
      }

      return true;
    }

    bool res = false;
    for(std::list<RevenueDecimal>::const_iterator eit = expected.begin();
        eit != expected.end(); ++eit)
    {
      if(mit->second == *eit)
      {
        res = true;
        break;
      }
    }

    if(!res)
    {
      std::cerr << prefix <<
        ": unexpected match counter for channel_trigger_id = " <<
        channel_trigger_id << ": " << mit->second <<
        " instead one of (";
      Algs::print(std::cerr, expected.begin(), expected.end());
      std::cerr << "), full result:" << std::endl;
      print(std::cerr, "  ");
      return false;
    }

    return true;
  }

  bool check_page_trigger(
    const char* prefix,
    unsigned long channel_trigger_id,
    unsigned long channel_id,
    const RevenueDecimal& first_expected_counter,
    const RevenueDecimal& second_expected_counter)
  {
    std::list<RevenueDecimal> check;
    check.push_back(first_expected_counter);
    check.push_back(second_expected_counter);
    return check_page_trigger(prefix, channel_trigger_id, channel_id, check);
  }

  bool check_imp_sum(
    const char* prefix,
    const RevenueDecimal& expected_counter)
  {
    RevenueDecimal sum = get_sum_(page_imps_);
    sum += get_sum_(search_imps_);
    sum += get_sum_(url_imps_);
    if(sum != expected_counter)
    {
      std::cerr << prefix << ": unexpected match counter sum = " << sum <<
        " instead " << expected_counter << ", full result:" << std::endl;
      print(std::cerr, "  ");
      return false;
    }

    return true;
  }

  bool check_page_sum(
    const char* prefix,
    const RevenueDecimal& expected_counter)
  {
    RevenueDecimal page_sum = get_sum_(page_imps_);
    if(page_sum != expected_counter)
    {
      std::cerr << prefix << ": unexpected match counter page sum = " << page_sum <<
        " instead " << expected_counter << ", full result:" << std::endl;
      print(std::cerr, "  ");
      return false;
    }

    return true;
  }

protected:
  virtual
  ~CollTriggerActionProcessor() noexcept = default;

  static RevenueDecimal get_sum_(const MatchCountMap& matches)
  {
    RevenueDecimal res_sum = RevenueDecimal::ZERO;
    for(MatchCountMap::const_iterator mit = matches.begin();
        mit != matches.end(); ++mit)
    {
      res_sum += mit->second;
    }
    return res_sum;
  }

private:
  MatchCountMap page_imps_;
  MatchCountMap search_imps_;
  MatchCountMap url_imps_;

  MatchCountMap page_clicks_;
  MatchCountMap search_clicks_;
  MatchCountMap url_clicks_;
};

class UserTriggerMatchProfileProviderTestImpl:
  public UserTriggerMatchProfileProvider,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  UserTriggerMatchProfileProviderTestImpl(
    UserTriggerMatchContainer* container)
    : container_(ReferenceCounting::add_ref(container))
  {}

  virtual
  Generics::ConstSmartMemBuf_var
  get_user_profile(const AdServer::Commons::UserId& user_id)
    /*throw(Exception)*/
  {
    try
    {
      return container_->get_user_profile(user_id);
    }
    catch(const UserTriggerMatchContainer::Exception& ex)
    {
      throw UserTriggerMatchProfileProvider::Exception(ex.what());
    }
  }

protected:
  virtual
  ~UserTriggerMatchProfileProviderTestImpl() noexcept = default;

private:
  UserTriggerMatchContainer_var container_;
};

typedef ReferenceCounting::SmartPtr<CollTriggerActionProcessor>
  CollTriggerActionProcessor_var;

class TestBase: public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef UserTriggerMatchContainer::Config Config;
  typedef UserTriggerMatchContainer::Config_var Config_var;
  typedef Config::ChannelInfo_var ChannelInfo_var;
  typedef Config::ChannelInfo ChannelInfo;

  typedef UserTriggerMatchContainer::RequestInfo RequestInfo;
  typedef UserTriggerMatchContainer::ImpressionInfo ImpressionInfo;

  typedef TriggerActionProcessor::TriggersMatchInfo TriggersMatchInfo;

  int run(const char* root_path) noexcept
  {
    try
    {
      Logging::Logger_var logger(new Logging::Null::Logger);

      system((std::string("rm -r ") + root_path + TEST_FOLDER +
        " 2>/dev/null ; mkdir -p " + root_path + TEST_FOLDER +
        " ; mkdir -p " + root_path + TEST_FOLDER + "Chunk_0_4" +
        " ; mkdir -p " + root_path + TEST_FOLDER + "Chunk_1_4" +
        " ; mkdir -p " + root_path + TEST_FOLDER + "Chunk_2_4" +
        " ; mkdir -p " + root_path + TEST_FOLDER + "Chunk_3_4").c_str());

      AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_folders;
      AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
        chunk_folders,
        (std::string(root_path) + TEST_FOLDER).c_str());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(1));

      CollTriggerActionProcessor_var processor =
        new CollTriggerActionProcessor();

      UserTriggerMatchContainer_var temp_container(
        new UserTriggerMatchContainer(
          logger,
          processor,
          0,
          chunk_folders.size(), // chunks_number
          chunk_folders,
          "TempUserTriggerMatch",
          (std::string(root_path) + TEST_FOLDER).c_str(),
          0,
          POSITIVE_TRIGGERS_GROUP_SIZE,
          NEGATIVE_TRIGGERS_GROUP_SIZE,
          MAX_TRIGGER_VISITS,
          cache,
          AdServer::ProfilingCommons::LevelMapTraits(
            AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
            10*1024*1024,
            10*1024*1024,
            20*1024*1024,
            20,
            Generics::Time::ZERO),
          AdServer::ProfilingCommons::LevelMapTraits(
            AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
            10*1024*1024,
            10*1024*1024,
            20*1024*1024,
            20,
            Generics::Time::ZERO)
          ));

      temp_container->activate_object();

      UserTriggerMatchProfileProvider_var profile_provider =
        new UserTriggerMatchProfileProviderTestImpl(temp_container);

      UserTriggerMatchContainer_var container(
        new UserTriggerMatchContainer(
          logger,
          processor,
          profile_provider,
          chunk_folders.size(), // chunks_number
          chunk_folders,
          "UserTriggerMatch",
          (std::string(root_path) + TEST_FOLDER).c_str(),
          "RequestTriggerMatch",
          POSITIVE_TRIGGERS_GROUP_SIZE,
          NEGATIVE_TRIGGERS_GROUP_SIZE,
          MAX_TRIGGER_VISITS,
          cache,
          AdServer::ProfilingCommons::LevelMapTraits(
            AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
            10*1024*1024,
            10*1024*1024,
            20*1024*1024,
            20,
            Generics::Time::ZERO),
          AdServer::ProfilingCommons::LevelMapTraits(
            AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
            10*1024*1024,
            10*1024*1024,
            20*1024*1024,
            20,
            Generics::Time::ZERO)
          ));

      container->activate_object();

      Config_var config = new Config();
      init_config(*config);

      container->config(config);
      temp_container->config(config);

      bool res = run_impl(container, temp_container, processor);

      container->deactivate_object();
      temp_container->deactivate_object();
      container->wait_object();
      temp_container->wait_object();

      if(res)
      {
        std::cout << "Test '" << name() << "': success" << std::endl;
      }
      else
      {
        std::cerr << "Test '" << name() << "': failed" << std::endl;
      }
      return res;
    }
    catch (const eh::Exception& ex)
    {
      std::cerr << "Test '" << name() << "': " << ex.what() << std::endl;
    }

    return 1;
  }

  virtual const char* name() const = 0;

  virtual void init_config(Config& config) = 0;

  virtual bool run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* temp_container,
    CollTriggerActionProcessor* processor) = 0;

protected:
  virtual
  ~TestBase() noexcept = default;
};

typedef ReferenceCounting::SmartPtr<TestBase> TestBase_var;
typedef std::list<TestBase_var> TestList;

class OneMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "OneMatchTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    channel->page_time_to = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    bool res = processor->check_page_trigger(
      name(), 2, 1, ONE_IMP_COUNTER, ONE_IMP_AND_NEGATE_COUNTER);
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */
    return res;
  }

protected:
  virtual
  ~OneMatchTest() noexcept = default;
};

class OneSearchMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "OneSearchMatchTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->search_min_visits = 1;
    channel->search_time_to = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->search_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* /*processor*/)
  {
    const Generics::Time base_time(String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.search_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    /*
    bool res = processor->check_page_trigger(
      name(), 2, ONE_IMP_COUNTER, ONE_IMP_AND_NEGATE_COUNTER);
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);
    */
    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */
    return true;
  }

protected:
  virtual
  ~OneSearchMatchTest() noexcept = default;
};

class FewTriggersMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "FewTriggersMatchTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    channel->page_time_to = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);
      request_info.page_matches[1].push_back(3);
      request_info.page_matches[1].push_back(4);
      request_info.page_matches[1].push_back(5);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    /*
    RevenueDecimal divided_imp_counter = RevenueDecimal::div(
      ONE_IMP_COUNTER, RevenueDecimal(4));
    RevenueDecimal divided_imp_and_negate_counter = RevenueDecimal::div(
      ONE_IMP_AND_NEGATE_COUNTER, RevenueDecimal(4));
    bool res = processor->check_page_trigger(
      name(), 2, ONE_IMP_COUNTER, ONE_IMP_AND_NEGATE_COUNTER);
    */
    bool res = processor->check_page_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */

    return res;
  }

protected:
  virtual
  ~FewTriggersMatchTest() noexcept = default;
};

class LastMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "LastMatchTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    channel->page_time_to = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    { // #1
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time + 1;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(3);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time + 1;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    bool res = processor->check_page_trigger(
      name(), 3, 1, ONE_IMP_COUNTER, ONE_IMP_AND_NEGATE_COUNTER);
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */
    return res;
  }

protected:
  virtual
  ~LastMatchTest() noexcept = default;
};

class ZeroMinVisitsMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "ZeroMinVisitsMatchTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 0;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    bool res = processor->check_page_trigger(
      name(), 2, 1, RevenueDecimal::ZERO, RevenueDecimal::ZERO);
    res &= processor->check_page_sum(name(), RevenueDecimal::ZERO);

    return res;
  }

protected:
  virtual
  ~ZeroMinVisitsMatchTest() noexcept = default;
};

class ZeroMinVisitsImpressionTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "ZeroMinVisitsImpressionTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    {
      // change config: set page min_visits = 0
      Config_var config(new Config());
      ChannelInfo_var channel(new ChannelInfo());
      channel->page_min_visits = 0;
      channel->search_min_visits = 1;
      config->channels[1] = channel;
      cont->config(config);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    bool res = processor->check_page_trigger(
      name(), 2, 1, RevenueDecimal::ZERO, RevenueDecimal::ZERO);
    res &= processor->check_page_sum(name(), RevenueDecimal::ZERO);

    return res;
  }

protected:
  virtual
  ~ZeroMinVisitsImpressionTest() noexcept = default;
};

class PastMatchAfterImpressionTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "PastMatchAfterImpressionTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    bool res = processor->check_page_trigger(
      name(), 2, 1, ONE_IMP_COUNTER, ONE_IMP_AND_NEGATE_COUNTER);
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */
    return res;
  }

protected:
  virtual
  ~PastMatchAfterImpressionTest() noexcept = default;
};

class PastMatchAfterMatchAndImpressionTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "PastMatchAfterMatchAndImpressionTest(impression revert)";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(3);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time + 1;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time + 1;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    bool res = processor->check_page_trigger(
      name(), 2, 1, ONE_IMP_COUNTER, ONE_IMP_AND_NEGATE_COUNTER);
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */
    return res;
  }

protected:
  virtual
  ~PastMatchAfterMatchAndImpressionTest() noexcept = default;
};

class PastImpressionAfterClickTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "PastImpressionAfterClickTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_click(imp_info.request_id, base_time);
      cont->process_impression(imp_info);
    }

    bool res = processor->check_page_trigger(
      name(), 2, 1, ONE_IMP_COUNTER, ONE_IMP_AND_NEGATE_COUNTER);
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */
    return res;
  }

protected:
  virtual
  ~PastImpressionAfterClickTest() noexcept = default;
};

class TimeToExpireTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "TimeToExpireTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 2;
    channel->page_time_to = Generics::Time::ONE_DAY.tv_sec;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    { // #1
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time + Generics::Time::ONE_DAY + 1;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(3);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time + Generics::Time::ONE_DAY + 1;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
    }

    // if first page match successfuly removed: impression will not be divided
    //   between triggers 2,3
    bool res = processor->check_page_trigger(
      name(), 3, 1, ONE_IMP_COUNTER, ONE_IMP_AND_NEGATE_COUNTER);
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */
    return res;
  }

protected:
  virtual
  ~TimeToExpireTest() noexcept = default;
};

class ImpressionDivideTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "ImpressionDivideTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 2;
    channel->page_time_to = Generics::Time::ONE_DAY.tv_sec;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    { // #1
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time + Generics::Time::ONE_DAY - 2;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(3);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time + Generics::Time::ONE_DAY - 1;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
    }

    RevenueDecimal divided_imp_counter =
      RevenueDecimal::div(ONE_IMP_COUNTER, RevenueDecimal(2));
    RevenueDecimal divided_negate_imp_counter =
      RevenueDecimal::div(ONE_NEGATE_IMP_COUNTER, RevenueDecimal(2));
    std::list<RevenueDecimal> exp;

    exp.push_back(divided_imp_counter); // no noise
    exp.push_back(divided_imp_counter + divided_negate_imp_counter);
    exp.push_back(divided_imp_counter +
      divided_negate_imp_counter + divided_negate_imp_counter);

    exp.push_back(divided_imp_counter + divided_imp_counter);
    exp.push_back(divided_imp_counter + divided_imp_counter +
      divided_negate_imp_counter);
    exp.push_back(divided_imp_counter + divided_imp_counter +
      divided_negate_imp_counter + divided_negate_imp_counter);

    bool res = processor->check_page_trigger(
      name(),
      2,
      1,
      exp);
    res &= processor->check_page_trigger(
      name(),
      3,
      1,
      exp);
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    return res;
  }

protected:
  virtual
  ~ImpressionDivideTest() noexcept = default;
};

class OneTriggerInChannelMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "OneTriggerInChannelMatchTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    channel->page_time_to = 1;
    channel->page_triggers.push_back(2);
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    bool res = processor->check_page_trigger(
      name(), 2, 1, RevenueDecimal(1), RevenueDecimal(1));
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */

    return res;
  }

protected:
  virtual
  ~OneTriggerInChannelMatchTest() noexcept = default;
};

class OneTriggerInChannelMatchUnexistingTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "OneTriggerInChannelMatchUnexistingTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    channel->page_time_to = 1;
    channel->page_triggers.push_back(2);
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(3);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    bool res = processor->check_page_trigger(
      name(), 3, 1, RevenueDecimal(1), RevenueDecimal(1));
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */

    return res;
  }

protected:
  virtual
  ~OneTriggerInChannelMatchUnexistingTest() noexcept = default;
};

class SmallChannelMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "SmallChannelMatchTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    channel->page_time_to = 1;
    for(int i = 0; i < 7; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    bool res = processor->check_page_trigger(
      name(), 2, 1, RevenueDecimal(1), RevenueDecimal(1));
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */

    return res;
  }

protected:
  virtual
  ~SmallChannelMatchTest() noexcept = default;
};

class LastTriggerDeactivatedMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "LastTriggerDeactivatedMatchTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    channel->page_time_to = 1;
    // no triggers
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);
      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    bool res = processor->check_page_trigger(
      name(), 2, 1, RevenueDecimal(1), RevenueDecimal(1));
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */

    return res;
  }

protected:
  virtual
  ~LastTriggerDeactivatedMatchTest() noexcept = default;
};

class ChannelDeactivatedMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "ChannelDeactivatedMatchTest";
  }

  virtual void init_config(Config& /*config*/)
  {}

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);
      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    bool res = processor->check_page_trigger(
      name(), 2, 1, RevenueDecimal(1), RevenueDecimal(1));
    res &= processor->check_page_sum(name(), ONE_IMP_SUM);

    return res;
  }

protected:
  virtual
  ~ChannelDeactivatedMatchTest() noexcept = default;
};

class FewKeywordsOneUrlChannelMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "FewKeywordsOneUrlChannelMatchTest"; // ADSC-5891
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    channel->page_time_to = 1;
    channel->url_min_visits = 1;
    channel->url_time_to = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    channel->url_triggers.push_back(TRIGGERS_IN_CHANNEL);
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);
      request_info.url_matches[1].push_back(TRIGGERS_IN_CHANNEL);
      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    bool res = processor->check_page_trigger(
      name(),
      2,
      1,
      RevenueDecimal::div(ONE_IMP_COUNTER, RevenueDecimal(2)),
      RevenueDecimal::div(ONE_IMP_AND_NEGATE_COUNTER, RevenueDecimal(2)));

    res &= processor->check_imp_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */

    return res;
  }

protected:
  virtual
  ~FewKeywordsOneUrlChannelMatchTest() noexcept = default;
};

class MergeChannelMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "MergeChannelMatchTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 2;
    channel->page_time_to = 1;
    channel->url_min_visits = 1;
    channel->url_time_to = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* temp_container,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    // #0
    {
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_TEMP_UID;
      request_info.page_matches[1].push_back(2);
      temp_container->process_request(request_info);
    }

    {
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.merge_user_id = TEST_TEMP_UID;
      request_info.page_matches[1].push_back(1);
      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    RevenueDecimal divided_imp_counter =
      RevenueDecimal::div(ONE_IMP_COUNTER, RevenueDecimal(2));
    RevenueDecimal divided_negate_imp_counter =
      RevenueDecimal::div(ONE_NEGATE_IMP_COUNTER, RevenueDecimal(2));
    std::list<RevenueDecimal> exp;

    exp.push_back(divided_imp_counter); // no noise
    exp.push_back(divided_imp_counter + divided_negate_imp_counter);
    exp.push_back(divided_imp_counter +
      divided_negate_imp_counter + divided_negate_imp_counter);
    exp.push_back(divided_imp_counter + divided_imp_counter);
    exp.push_back(divided_imp_counter + divided_imp_counter +
      divided_negate_imp_counter);
    exp.push_back(divided_imp_counter + divided_imp_counter +
      divided_negate_imp_counter + divided_negate_imp_counter);

    bool res = processor->check_page_trigger(
      name(),
      1,
      1,
      exp);

    res &= processor->check_page_trigger(
      name(),
      2,
      1,
      exp);

    res &= processor->check_imp_sum(name(), ONE_IMP_SUM);

    /*
    std::cout << name() << ", result:" << std::endl;
    processor->print(std::cout, "  ");
    */

    return res;
  }

protected:
  virtual
  ~MergeChannelMatchTest() noexcept = default;
};

class MergeFewChannelsMatchTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "MergeFewChannelsMatchTest";
  }

  virtual void init_config(Config& config)
  {
    for(unsigned long channel_id = 1; channel_id <= 4; ++channel_id)
    {
      ChannelInfo_var channel(new ChannelInfo());
      channel->page_min_visits = 1;
      channel->page_time_to = 1;
      channel->url_min_visits = 1;
      channel->url_time_to = 1;
      for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
      {
        channel->page_triggers.push_back(channel_id * TRIGGERS_IN_CHANNEL + i);
      }
      config.channels[channel_id] = channel;
    }
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* temp_container,
    CollTriggerActionProcessor* processor)
  {
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    // #0
    {
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_TEMP_UID;
      request_info.page_matches[1].push_back(TRIGGERS_IN_CHANNEL + 1);
      temp_container->process_request(request_info);
    }

    {
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_TEMP_UID;
      request_info.page_matches[3].push_back(TRIGGERS_IN_CHANNEL * 3 + 1);
      temp_container->process_request(request_info);
    }

    {
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.merge_user_id = TEST_TEMP_UID;
      request_info.page_matches[2].push_back(TRIGGERS_IN_CHANNEL * 2 + 1);
      cont->process_request(request_info);
    }

    {
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.page_matches[4].push_back(TRIGGERS_IN_CHANNEL * 4 + 1);
      cont->process_request(request_info);
    }

    {
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time;
      request_info.user_id = TEST_UID;
      request_info.merge_user_id = TEST_TEMP_UID;
      cont->process_request(request_info);
    }

    {
      ImpressionInfo imp_info;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.user_id = TEST_UID;
      imp_info.time = base_time;
      imp_info.channels.insert(1);
      imp_info.channels.insert(2);
      imp_info.channels.insert(3);
      imp_info.channels.insert(4);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time);
    }

    bool res = true;
    for(unsigned long i = 1; i <= 4; ++i)
    {
      res &= processor->check_page_trigger(
        name(),
        TRIGGERS_IN_CHANNEL * i + 1,
        i,
        ONE_IMP_COUNTER, ONE_IMP_AND_NEGATE_COUNTER);
    }

    res &= processor->check_imp_sum(
      name(),
      RevenueDecimal::mul(ONE_IMP_SUM, RevenueDecimal(false, 4, 0),
        Generics::DMR_FLOOR));

    return res;
  }

protected:
  virtual
  ~MergeFewChannelsMatchTest() noexcept = default;
};

// Modeling tests
class ModelTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "ModelTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    static const unsigned long COUNT = 10000;
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    Generics::Timer timer;
    Generics::CPUTimer cpu_timer;
    timer.start();
    cpu_timer.start();

    // always match & click trigger #2
    for(int i = 0; i < 10000; ++i)
    { // #0
      UserTriggerMatchContainer::RequestInfo request_info;
      request_info.time = base_time + i;
      request_info.user_id = TEST_UID;
      request_info.page_matches[1].push_back(2);

      cont->process_request(request_info);

      ImpressionInfo imp_info;
      imp_info.user_id = TEST_UID;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.time = base_time + i;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time + i);

      /*
      if(i % 100 == 0)
      {
        std::cout << i << " requests done, size = " <<
          cont->get_profile(TEST_UID)->membuf().size() << std::endl;
      }
      */
    }

    cpu_timer.stop();
    timer.stop();

    std::cout << name() << ", abs time = " << timer.elapsed_time() <<
      ", cpu time = " << cpu_timer.elapsed_time() <<
      ", result:" << std::endl;

    processor->print(std::cout, "  ");

    return processor->check_page_sum(
      name(),
      RevenueDecimal::mul(ONE_IMP_SUM, RevenueDecimal(false, COUNT, 0),
        Generics::DMR_FLOOR));
  }

protected:
  virtual
  ~ModelTest() noexcept = default;
};

class DivideImpModelTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "DivideImpModelTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 2;
    channel->page_time_to = 2;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const unsigned long COUNT = 10000;
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    Generics::Timer timer;
    Generics::CPUTimer cpu_timer;
    timer.start();
    cpu_timer.start();

    // always match & click trigger #2
    for(unsigned long i = 0; i < COUNT; ++i)
    { // #0

      {
        UserTriggerMatchContainer::RequestInfo request_info;
        request_info.time = base_time + 2*i;
        request_info.user_id = TEST_UID;
        request_info.page_matches[1].push_back(2);

        cont->process_request(request_info);
      }

      {
        UserTriggerMatchContainer::RequestInfo request_info;
        request_info.time = base_time + 2*i + 1;
        request_info.user_id = TEST_UID;
        request_info.page_matches[1].push_back(3);
        cont->process_request(request_info);
      }

      ImpressionInfo imp_info;
      imp_info.user_id = TEST_UID;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.time = base_time + 2*i + 1;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time + i);

      /*
      if(i % 100 == 0)
      {
        std::cout << i << " requests done, size = " <<
          cont->get_profile(TEST_UID)->membuf().size() << std::endl;
      }
      */
    }

    cpu_timer.stop();
    timer.stop();

    std::cout << name() << ", abs time = " << timer.elapsed_time() <<
      ", cpu time = " << cpu_timer.elapsed_time() <<
      ", result:" << std::endl;

    processor->print(std::cout, "  ");

    return processor->check_page_sum(
      name(),
      RevenueDecimal::mul(ONE_IMP_SUM, RevenueDecimal(false, COUNT, 0),
        Generics::DMR_FLOOR));
  }

protected:
  virtual
  ~DivideImpModelTest() noexcept = default;
};

class FewTriggersImpModelTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "FewTriggersImpModelTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 2;
    channel->page_time_to = 2;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      channel->page_triggers.push_back(i);
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const unsigned long COUNT = 10000;
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    Generics::Timer timer;
    Generics::CPUTimer cpu_timer;
    timer.start();
    cpu_timer.start();

    // always match & click triggers #2,#3,#4,#5
    for(unsigned long i = 0; i < COUNT; ++i)
    { // #0

      {
        UserTriggerMatchContainer::RequestInfo request_info;
        request_info.time = base_time + 2*i + 1;
        request_info.user_id = TEST_UID;
        request_info.page_matches[1].push_back(2);
        request_info.page_matches[1].push_back(3);
        request_info.page_matches[1].push_back(4);
        request_info.page_matches[1].push_back(5);

        cont->process_request(request_info);
      }

      ImpressionInfo imp_info;
      imp_info.user_id = TEST_UID;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.time = base_time + 2*i + 1;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time + i);

      /*
      if(i % 100 == 0)
      {
        std::cout << i << " requests done, size = " <<
          cont->get_profile(TEST_UID)->membuf().size() << std::endl;
      }
      */
    }

    cpu_timer.stop();
    timer.stop();

    std::cout << name() << ", abs time = " << timer.elapsed_time() <<
      ", cpu time = " << cpu_timer.elapsed_time() <<
      ", result:" << std::endl;

    processor->print(std::cout, "  ");

    return processor->check_page_sum(
      name(),
      RevenueDecimal::mul(ONE_IMP_SUM, RevenueDecimal(false, COUNT, 0),
        Generics::DMR_FLOOR));
  }

protected:
  virtual
  ~FewTriggersImpModelTest() noexcept = default;
};

class DeactivatedTriggerImpModelTest: public TestBase
{
public:
  virtual const char* name() const
  {
    return "DeactivatedTriggerImpModelTest";
  }

  virtual void init_config(Config& config)
  {
    ChannelInfo_var channel(new ChannelInfo());
    channel->page_min_visits = 1;
    channel->page_time_to = 1;
    for(unsigned long i = 0; i < TRIGGERS_IN_CHANNEL; ++i)
    {
      if(i != 2)
      {
        channel->page_triggers.push_back(i);
      }
    }
    config.channels[1] = channel;
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* processor)
  {
    const unsigned long COUNT = 10000;
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    Generics::Timer timer;
    Generics::CPUTimer cpu_timer;
    timer.start();
    cpu_timer.start();

    for(unsigned long i = 0; i < COUNT; ++i)
    { // #0

      {
        UserTriggerMatchContainer::RequestInfo request_info;
        request_info.time = base_time + 2*i + 1;
        request_info.user_id = TEST_UID;
        request_info.page_matches[1].push_back(2);

        cont->process_request(request_info);
      }

      ImpressionInfo imp_info;
      imp_info.user_id = TEST_UID;
      imp_info.request_id = Commons::RequestId::create_random_based();
      imp_info.time = base_time + 2*i + 1;
      imp_info.channels.insert(1);
      cont->process_impression(imp_info);
      cont->process_click(imp_info.request_id, base_time + i);

      /*
      if(i % 100 == 0)
      {
        std::cout << i << " requests done, size = " <<
          cont->get_profile(TEST_UID)->membuf().size() << std::endl;
      }
      */
    }

    cpu_timer.stop();
    timer.stop();

    std::cout << name() << ", abs time = " << timer.elapsed_time() <<
      ", cpu time = " << cpu_timer.elapsed_time() <<
      ", result:" << std::endl;

    processor->print(std::cout, "  ");

    return processor->check_page_sum(
      name(),
      RevenueDecimal::mul(ONE_IMP_SUM, RevenueDecimal(false, COUNT, 0),
        Generics::DMR_FLOOR));
  }

protected:
  virtual
  ~DeactivatedTriggerImpModelTest() noexcept = default;
};

class PerfTest: public TestBase
{
public:
  static const unsigned long CHANNEL_COUNT = 20;

  virtual const char* name() const
  {
    return "PerfTest";
  }

  virtual void init_config(Config& config)
  {
    for(unsigned long channel_i = 1; channel_i < CHANNEL_COUNT + 1; ++channel_i)
    {
      ChannelInfo_var channel(new ChannelInfo());
      channel->page_min_visits = 5;
      channel->page_time_to = 30000;
      for(unsigned long i = channel_i * TRIGGERS_IN_CHANNEL;
          i < (channel_i + 1) * TRIGGERS_IN_CHANNEL; ++i)
      {
        channel->page_triggers.push_back(i);
      }
      config.channels[channel_i] = channel;
    }
  }

  virtual bool
  run_impl(
    UserTriggerMatchContainer* cont,
    UserTriggerMatchContainer* /*temp_container*/,
    CollTriggerActionProcessor* /*processor*/)
  {
    const unsigned long USER_COUNT = 1000;//10000;
    const unsigned long REQUESTS_PER_USER = 1000;//100;
    const Generics::Time base_time(
      String::SubString("2011-11-29 19:40:28"), "%Y-%m-%d %H:%M:%S");

    for(unsigned long global_i = 0; global_i < 1; ++global_i)
    {
      Generics::Timer timer;
      Generics::CPUTimer cpu_timer;
      timer.start();
      cpu_timer.start();

      for(unsigned long user_i = 0; user_i < USER_COUNT; ++user_i)
      {
        AdServer::Commons::UserId uid = AdServer::Commons::UserId::create_random_based();

        for(unsigned long i = 0; i < REQUESTS_PER_USER; ++i)
        {
          UserTriggerMatchContainer::RequestInfo request_info;
          request_info.time = base_time + /*2*i +*/ 1;
          request_info.user_id = uid;
          for(unsigned long channel_i = 1; channel_i < CHANNEL_COUNT + 1; ++channel_i)
          {
            request_info.page_matches[channel_i].push_back(
              channel_i * TRIGGERS_IN_CHANNEL + 1);
          }
          cont->process_request(request_info);
        }
      }

      cpu_timer.stop();
      timer.stop();

      std::cout << name() << "(#" << global_i << "): abs time = " << timer.elapsed_time() <<
        ", cpu time = " << cpu_timer.elapsed_time() <<
        ", requests = " << (REQUESTS_PER_USER * USER_COUNT) <<
        ", users = " << USER_COUNT << std::endl;
    }

    return true;
  }

protected:
  virtual
  ~PerfTest() noexcept = default;
};

int
main(int argc, char** argv) noexcept
{
  ::srand(time(0));

  try
  {
    using namespace Generics::AppUtils;
    Option<std::string> root_path(DEFAULT_ROOT_PATH);
    CheckOption opt_help;
    CheckOption opt_run_modeling_tests;
    CheckOption opt_run_perf_tests;

    Args args;
    args.add(equal_name("path") || short_name("p"), root_path);
    args.add(equal_name("help") || short_name("h"), opt_help);
    args.add(equal_name("model") || short_name("m"), opt_run_modeling_tests);
    args.add(equal_name("perf"), opt_run_perf_tests);
    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    TestList tests;
    tests.push_back(TestBase_var(new OneMatchTest()));
    tests.push_back(TestBase_var(new OneSearchMatchTest()));
    tests.push_back(TestBase_var(new FewTriggersMatchTest()));
    tests.push_back(TestBase_var(new LastMatchTest()));
    tests.push_back(TestBase_var(new ZeroMinVisitsMatchTest()));
    tests.push_back(TestBase_var(new ZeroMinVisitsImpressionTest()));
    tests.push_back(TestBase_var(new PastMatchAfterImpressionTest()));
    tests.push_back(TestBase_var(new PastImpressionAfterClickTest()));
    tests.push_back(TestBase_var(new PastMatchAfterMatchAndImpressionTest()));
    tests.push_back(TestBase_var(new TimeToExpireTest()));
    tests.push_back(TestBase_var(new ImpressionDivideTest()));
    tests.push_back(TestBase_var(new FewKeywordsOneUrlChannelMatchTest()));
    tests.push_back(TestBase_var(new OneTriggerInChannelMatchTest()));
    tests.push_back(TestBase_var(new OneTriggerInChannelMatchUnexistingTest()));
    tests.push_back(TestBase_var(new SmallChannelMatchTest()));
    tests.push_back(TestBase_var(new LastTriggerDeactivatedMatchTest()));
    tests.push_back(TestBase_var(new ChannelDeactivatedMatchTest()));
    tests.push_back(TestBase_var(new MergeChannelMatchTest()));
    tests.push_back(TestBase_var(new MergeFewChannelsMatchTest()));

    if(opt_run_modeling_tests.enabled())
    {
      tests.push_back(TestBase_var(new ModelTest()));
      tests.push_back(TestBase_var(new DivideImpModelTest()));
      tests.push_back(TestBase_var(new FewTriggersImpModelTest()));
      tests.push_back(TestBase_var(new DeactivatedTriggerImpModelTest()));
    }

    if(opt_run_perf_tests.enabled())
    {
      tests.push_back(TestBase_var(new PerfTest()));
    }

    int result = 0;
    for(TestList::iterator test_it = tests.begin();
        test_it != tests.end(); ++test_it)
    {
      result += (*test_it)->run(root_path->c_str());
    }

    return result;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return -1;
}
