/**
 * @file RequestInfoSvcs/UserCampaignReachContainerTest.cpp
 */

#include <Generics/AppUtils.hpp>
#include <Commons/UserInfoManip.hpp>

#include "UserCampaignReachContainerMTTest.hpp"

using namespace AdServer::RequestInfoSvcs;
using AdServer::Commons::UserId;

namespace
{
  const char DEFAULT_ROOT_PATH[] = "./";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char TEST_FOLDER[] = "/Reach/";

  const char USAGE[] =
    "UserCampaignReachContainerTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";
}

typedef ReferenceCounting::SmartPtr<CollReachTestProcessor>
  CollReachTestProcessor_var;

struct TestBase: public ReferenceCounting::AtomicImpl
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  int
  run() noexcept
  {
    try
    {
      Logging::Logger_var logger(new Logging::Null::Logger);

      CollReachTestProcessor_var reach_processor(
        new CollReachTestProcessor());

      system(("rm -r " + *root_path + TEST_FOLDER +
        " 2>/dev/null ; mkdir -p " + *root_path + TEST_FOLDER).c_str());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserCampaignReachContainer_var reach_container(
        new UserCampaignReachContainer(
          logger,
          reach_processor,
          (*root_path + TEST_FOLDER).c_str(),
          "Reach",
          cache));

      run_impl(reach_container);

      ReachInfoList etalon;

      fill_etalon(etalon);

      const ReachInfoList& result = reach_processor->result();

      ReachInfoList::const_iterator et_it = etalon.begin();
      ReachInfoList::const_iterator res_it = result.begin();

      int i = 0;

      for (; et_it != etalon.end() && res_it != result.end();
          ++et_it, ++res_it, ++i)
      {
        if (!(*et_it == *res_it))
        {
          Stream::Error ostr;
          ostr << "result non equal standard - element #" << i << ": "
            << std::endl
            << ">>> standard >>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
          et_it->print(ostr, "  ");
          ostr << "=== result =======================" << std::endl;
          res_it->print(ostr, "  ");
          ostr << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";

          throw Exception(ostr);
        }
      }

      if (!(et_it == etalon.end() && res_it == result.end()))
      {
        Stream::Error ostr;
        ostr << "result size non equal size = " << result.size() <<
          " of standard = " << etalon.size() << ". Result:" << std::endl;
        
        {
          int i = 0;
          for (ReachInfoList::const_iterator e_it = result.begin();
              e_it != result.end();
              ++e_it, ++i)
          {
            ostr << "===== #" << i << " =======" << std::endl;
            e_it->print(ostr, "  ");
          }
        }
        
        ostr << "Etalon:" << std::endl;
        
        {
          int i = 0;
          for (ReachInfoList::const_iterator e_it = etalon.begin();
              e_it != etalon.end();
              ++e_it, ++i)
          {
            ostr << "===== #" << i << " =======" << std::endl;
            e_it->print(ostr, "  ");
          }
        }

        throw Exception(ostr);
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

  RequestInfo create_request_info(const Generics::Time& time, bool text_campaign = false)
  {
    RequestInfo request_info;
    request_info.user_id = UserId("EAAAAAAAAAAAAAAAAAAAAA..");
    request_info.time = time;
    request_info.adv_time = time;
    request_info.colo_id = 1;
    request_info.campaign_id = 1;
    request_info.ccg_id = 2;
    request_info.cc_id = 3;
    request_info.advertiser_id = 4;
    request_info.text_campaign = text_campaign;
    return request_info;
  }

  virtual const char* name() = 0;

  virtual void run_impl(UserCampaignReachContainer* cont) = 0;

  virtual void fill_etalon(ReachInfoList& etalon) = 0;
};

typedef ReferenceCounting::SmartPtr<TestBase> TestBase_var;
typedef std::list<TestBase_var> TestList;

class CollReachSumTestProcessor:
  public AdServer::RequestInfoSvcs::CampaignReachProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  typedef AdServer::RequestInfoSvcs::CampaignReachProcessor::Exception
    Exception;

  struct Key
  {
    Key(
      unsigned long id_val,
      const Generics::Time& date_val,
      const Generics::Time& last_appearance_date_val)
      : id(id_val),
        date(date_val),
        last_appearance_date(last_appearance_date_val)
    {}

    bool
    operator<(const Key& right) const
    {
      return id < right.id ||
        (id == right.id && (
           date < right.date ||
           (date == right.date &&
             last_appearance_date < right.last_appearance_date)));
    }

    bool
    operator==(const Key& right) const
    {
      return id == right.id &&
        date == right.date &&
        last_appearance_date == right.last_appearance_date;
    }

    const unsigned long id;
    const Generics::Time date;
    const Generics::Time last_appearance_date;

  private:
    unsigned long hash_;
  };

  struct Counter
  {
    Counter(): value(0) {}
    Counter(long val): value(val) {}

    Counter&
    operator+=(const Counter& right)
    {
      value += right.value;
      return *this;
    }

    bool
    operator==(const Counter& right) const
    {
      return value == right.value;
    }

    long value;
  };

  typedef std::map<Key, Counter> ReachMap;

  void
  process_reach(
    const AdServer::RequestInfoSvcs::CampaignReachProcessor::ReachInfo&
      reach_info)
    /*throw(Exception)*/
  {
    for(IdAppearanceList::const_iterator cmp_it =
          reach_info.campaigns.begin();
        cmp_it != reach_info.campaigns.end(); ++cmp_it)
    {
      /*
      std::cout << "add for [" << cmp_it->id << ", " <<
        cmp_it->date.gm_f() << ", " << cmp_it->last_appearance_date.gm_f() <<
        "]: " << cmp_it->counter << std::endl;
      */

      Key key(cmp_it->id, cmp_it->date, cmp_it->last_appearance_date);
      if((result_[key] += Counter(cmp_it->counter)) == Counter(0))
      {
        result_.erase(key);
      }
    }
  }

  const ReachMap&
  result() const
  {
    return result_;
  }

  void
  clear()
  {
    result_.clear();
  }

protected:
  virtual ~CollReachSumTestProcessor() noexcept
  {}

private:
  ReachMap result_;
};

typedef ReferenceCounting::QualPtr<CollReachSumTestProcessor>
  CollReachSumTestProcessor_var;

struct TestSumBase: public ReferenceCounting::AtomicImpl
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef CollReachSumTestProcessor::Key Key;
  typedef CollReachSumTestProcessor::Counter Counter;
  typedef CollReachSumTestProcessor::ReachMap ReachMap;
  typedef std::list<RequestInfo> RequestInfoList;

  struct RequestInfoWrap: public RequestInfo
  {
    RequestInfoWrap(
      const RequestInfo& request_info,
      unsigned long i)
      : RequestInfo(request_info),
        index(i)
    {}

    bool
    operator<(const RequestInfoWrap& right) const
    {
      return index < right.index;
    }

    unsigned long index;
  };

  typedef std::vector<RequestInfoWrap> RequestInfoWrapArray;

  static void
  print_reach_map(std::ostream& out, const ReachMap& reach_map, const char* prefix)
  {
    for(ReachMap::const_iterator it = reach_map.begin();
        it != reach_map.end(); ++it)
    {
      out << prefix <<
        "[ " << it->first.id << ", " << it->first.date.gm_f() << ", " <<
          it->first.last_appearance_date.gm_f() <<
        " : " <<
        it->second.value << " ]" << std::endl;
    }
  }

  static void
  print_requests(
    std::ostream& out,
    const RequestInfoWrapArray& requests,
    const char* prefix)
  {
    for(RequestInfoWrapArray::const_iterator req_it =
          requests.begin();
        req_it != requests.end(); ++req_it)
    {
      out << prefix << "[ " << req_it->request_id << ", " <<
        req_it->time.gm_f() << " ]" << std::endl;
    }
  }

  int
  run() noexcept
  {
    try
    {
      Logging::Logger_var logger(new Logging::Null::Logger);

      CollReachSumTestProcessor_var reach_processor(
        new CollReachSumTestProcessor());

      RequestInfoList requests;
      ReachMap etalon;
      fill(requests, etalon);

      RequestInfoWrapArray requests_array;
      requests_array.reserve(requests.size());
      unsigned long cur_index = 0;
      for(RequestInfoList::const_iterator rit = requests.begin();
          rit != requests.end(); ++rit)
      {
        requests_array.push_back(RequestInfoWrap(*rit, cur_index++));
      }

      std::sort(requests_array.begin(), requests_array.end());
      while(std::next_permutation(requests_array.begin(), requests_array.end()))
      {
        system(("rm -r " + *root_path + TEST_FOLDER +
          " 2>/dev/null ; mkdir -p " + *root_path + TEST_FOLDER).c_str());

        UserCampaignReachContainer_var reach_container(
          new UserCampaignReachContainer(
            logger,
            reach_processor,
            (*root_path + TEST_FOLDER).c_str(),
            "Reach",
            0));

        reach_processor->clear();

        // run scenario
        for(RequestInfoWrapArray::const_iterator req_it =
              requests_array.begin();
            req_it != requests_array.end(); ++req_it)
        {
          reach_container->process_impression(
            *req_it,
            ImpressionInfo(),
            RequestActionProcessor::ProcessingState());
        }

        // check output
        const ReachMap& result = reach_processor->result();
        if(result.size() != etalon.size())
        {
          std::cerr << "Test '" << name() << "': incorrect result size: " <<
            result.size() << " instead " << etalon.size() << ", result: " << std::endl;
          print_reach_map(std::cerr, result, "    ");
          std::cerr << "  instead:" << std::endl;
          print_reach_map(std::cerr, etalon, "    ");
          std::cerr << "  for requests:" << std::endl;
          print_requests(std::cerr, requests_array, "    ");
            
          return 1;
        }
        else if(!std::equal(result.begin(), result.end(), etalon.begin()))
        {
          std::cerr << "Test '" << name() << "': incorrect result: " << std::endl;
          print_reach_map(std::cerr, result, "    ");
          std::cerr << "  instead:" << std::endl;
          print_reach_map(std::cerr, etalon, "    ");
          std::cerr << "  for requests:" << std::endl;
          print_requests(std::cerr, requests_array, "    ");

          return 1;
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

  RequestInfo
  create_request_info(const Generics::Time& time, bool text_campaign = false)
  {
    RequestInfo request_info;
    request_info.user_id = UserId("EAAAAAAAAAAAAAAAAAAAAA..");
    request_info.time = time;
    request_info.adv_time = time;
    request_info.colo_id = 1;
    request_info.campaign_id = 1;
    request_info.ccg_id = 2;
    request_info.cc_id = 3;
    request_info.advertiser_id = 4;
    request_info.text_campaign = text_campaign;
    return request_info;
  }

  virtual const char* name() = 0;

  virtual void
  fill(
    RequestInfoList& requests,
    ReachMap& etalon) = 0;
};

typedef ReferenceCounting::SmartPtr<TestSumBase> TestSumBase_var;
typedef std::list<TestSumBase_var> TestSumList;

struct TestSimpleDailyAppear : public TestBase
{
  const Generics::Time TIME;

  TestSimpleDailyAppear()
    : TIME(String::SubString("2008-01-01"), "%Y-%m-%d")
  {}

  const char* name()
  {
    return "TestSimpleDailyAppear";
  }

  void
  run_impl(UserCampaignReachContainer* cont)
  {
    RequestInfo request_info = create_request_info(TIME, false);
    cont->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());

    request_info.text_campaign = true;
    cont->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  void
  fill_etalon(ReachInfoList& reach_info_list)
  {
    {
      CampaignReachProcessor::ReachInfo reach_info;
      reach_info.campaigns.push_back(IdAppearance(1, TIME, Generics::Time::ZERO, 1));
      reach_info.ccgs.push_back(IdAppearance(2, TIME, Generics::Time::ZERO, 1));
      reach_info.creatives.push_back(IdAppearance(3, TIME, Generics::Time::ZERO, 1));
      reach_info.advertisers.push_back(IdAppearance(4, TIME, Generics::Time::ZERO, 1));
      reach_info.display_advertisers.push_back(IdAppearance(4, TIME, Generics::Time::ZERO, 1));

      reach_info_list.push_back(reach_info);
    }
    
    {
      CampaignReachProcessor::ReachInfo reach_info;
      reach_info.text_advertisers.push_back(IdAppearance(4, TIME, Generics::Time::ZERO, 1));
      reach_info_list.push_back(reach_info);
    }
  }
};

struct TestOrderedAppearance: public TestBase
{
  const Generics::Time TIME1;
  const Generics::Time TIME2;
  const Generics::Time TIME3;

  TestOrderedAppearance()
    : TIME1(String::SubString("2008-01-01"), "%Y-%m-%d"),
      TIME2(String::SubString("2008-01-02"), "%Y-%m-%d"),
      TIME3(String::SubString("2008-01-10"), "%Y-%m-%d")
  {}

  const char* name()
  {
    return "TestOrderedAppearance";
  }

  void
  run_impl(UserCampaignReachContainer* cont)
  {
    cont->process_impression(
      create_request_info(TIME1, false),
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
    cont->process_impression(
      create_request_info(TIME2, false),
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
    cont->process_impression(
      create_request_info(TIME3, false),
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  void
  fill_etalon(ReachInfoList& reach_info_list)
  {
    {
      CampaignReachProcessor::ReachInfo reach_info;
      reach_info.campaigns.push_back(IdAppearance(1, TIME1, Generics::Time::ZERO, 1));
      reach_info.ccgs.push_back(IdAppearance(2, TIME1, Generics::Time::ZERO, 1));
      reach_info.creatives.push_back(IdAppearance(3, TIME1, Generics::Time::ZERO, 1));
      reach_info.advertisers.push_back(IdAppearance(4, TIME1, Generics::Time::ZERO, 1));
      reach_info.display_advertisers.push_back(IdAppearance(4, TIME1, Generics::Time::ZERO, 1));

      reach_info_list.push_back(reach_info);
    }

    {
      CampaignReachProcessor::ReachInfo reach_info;
      reach_info.campaigns.push_back(IdAppearance(1, TIME2, TIME1, 1));
      reach_info.ccgs.push_back(IdAppearance(2, TIME2, TIME1, 1));
      reach_info.creatives.push_back(IdAppearance(3, TIME2, TIME1, 1));
      reach_info.advertisers.push_back(IdAppearance(4, TIME2, TIME1, 1));
      reach_info.display_advertisers.push_back(IdAppearance(4, TIME2, TIME1, 1));

      reach_info_list.push_back(reach_info);
    }

    {
      CampaignReachProcessor::ReachInfo reach_info;
      reach_info.campaigns.push_back(IdAppearance(1, TIME3, TIME2, 1));
      reach_info.ccgs.push_back(IdAppearance(2, TIME3, TIME2, 1));
      reach_info.creatives.push_back(IdAppearance(3, TIME3, TIME2, 1));
      reach_info.advertisers.push_back(IdAppearance(4, TIME3, TIME2, 1));
      reach_info.display_advertisers.push_back(IdAppearance(4, TIME3, TIME2, 1));

      reach_info_list.push_back(reach_info);
    }
  }
};

struct TestAdStorm: public TestBase
{
  static const unsigned long COUNT = 1000;
  static const unsigned long PRINT_STEP = 100;

  const Generics::Time TIME;

  TestAdStorm()
    : TIME(String::SubString("2008-01-01"), "%Y-%m-%d")
  {}

  const char* name()
  {
    return "AdStorm";
  }

  void
  run_impl(UserCampaignReachContainer* cont)
  {
    for (unsigned long i = 0; i < COUNT; ++i)
    {
      /*
      if (i % PRINT_STEP == 0)
      {
        std::cout << "processed " << i << " / " << COUNT << std::endl;
      }
      */
      RequestInfo request_info;
      request_info.user_id = UserId("CAAAAAAAAAAAAAAAAAAAAA..");
      request_info.time = TIME;
      request_info.adv_time = TIME;
      request_info.colo_id = 1;
      request_info.campaign_id = i / 4;
      request_info.ccg_id = i / 2;
      request_info.cc_id = i;
      request_info.advertiser_id = i / 8;
      request_info.text_campaign = false;

      cont->process_impression(
        request_info,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }
  }

  void
  fill_etalon(ReachInfoList& reach_info_list)
  {
    for (unsigned long i = 0; i < COUNT; ++i)
    {
      CampaignReachProcessor::ReachInfo reach_info;

      if (i && i % 8 == 0)
      {
        reach_info.advertisers.push_back(
          IdAppearance(i / 8, TIME, Generics::Time::ZERO, 1));
        reach_info.display_advertisers.push_back(
          IdAppearance(i / 8, TIME, Generics::Time::ZERO, 1));
      }

      if (i % 4 == 0)
      {
        reach_info.campaigns.push_back(
          IdAppearance(i / 4, TIME, Generics::Time::ZERO, 1));
      }

      if (i % 2 == 0)
      {
        reach_info.ccgs.push_back(
          IdAppearance(i / 2, TIME, Generics::Time::ZERO, 1));
      }

      reach_info.creatives.push_back(
        IdAppearance(i, TIME, Generics::Time::ZERO, 1));

      reach_info_list.push_back(reach_info);
    }
  }
};

struct TestReverseAppearance: public TestBase
{
  const Generics::Time TIME1;
  const Generics::Time TIME2;
  const Generics::Time TIME3;

  TestReverseAppearance()
    : TIME1(String::SubString("2008-01-15"), "%Y-%m-%d"),
      TIME2(String::SubString("2008-01-14"), "%Y-%m-%d"),
      TIME3(String::SubString("2008-01-10"), "%Y-%m-%d")
  {}

  const char* name()
  {
    return "TestReverseAppearance";
  }

  void
  run_impl(UserCampaignReachContainer* cont)
  {
    /* this scenario is possible at day change because no garantees that
     * logs appear in correct order by time
     */
    cont->process_impression(
      create_request_info(TIME1, true),
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
    cont->process_impression(
      create_request_info(TIME2, true),
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
    cont->process_impression(
      create_request_info(TIME3, true),
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  void
  fill_etalon(ReachInfoList& reach_info_list)
  {
    {
      CampaignReachProcessor::ReachInfo reach_info;
      reach_info.campaigns.push_back(IdAppearance(1, TIME1, Generics::Time::ZERO, 1));
      reach_info.ccgs.push_back(IdAppearance(2, TIME1, Generics::Time::ZERO, 1));
      reach_info.creatives.push_back(IdAppearance(3, TIME1, Generics::Time::ZERO, 1));
      reach_info.advertisers.push_back(IdAppearance(4, TIME1, Generics::Time::ZERO, 1));
      reach_info.text_advertisers.push_back(IdAppearance(4, TIME1, Generics::Time::ZERO, 1));

      reach_info_list.push_back(reach_info);
    }

    {
      CampaignReachProcessor::ReachInfo reach_info;

      // revert first changes
      reach_info.campaigns.push_back(IdAppearance(1, TIME1, Generics::Time::ZERO, -1));
      reach_info.ccgs.push_back(IdAppearance(2, TIME1, Generics::Time::ZERO, -1));
      reach_info.creatives.push_back(IdAppearance(3, TIME1, Generics::Time::ZERO, -1));
      reach_info.advertisers.push_back(IdAppearance(4, TIME1, Generics::Time::ZERO, -1));
      reach_info.text_advertisers.push_back(IdAppearance(4, TIME1, Generics::Time::ZERO, -1));

      // apply new changes
      reach_info.campaigns.push_back(IdAppearance(1, TIME2, Generics::Time::ZERO, 1));
      reach_info.ccgs.push_back(IdAppearance(2, TIME2, Generics::Time::ZERO, 1));
      reach_info.creatives.push_back(IdAppearance(3, TIME2, Generics::Time::ZERO, 1));
      reach_info.advertisers.push_back(IdAppearance(4, TIME2, Generics::Time::ZERO, 1));
      reach_info.text_advertisers.push_back(IdAppearance(4, TIME2, Generics::Time::ZERO, 1));

      reach_info.campaigns.push_back(IdAppearance(1, TIME1, TIME2, 1));
      reach_info.ccgs.push_back(IdAppearance(2, TIME1, TIME2, 1));
      reach_info.creatives.push_back(IdAppearance(3, TIME1, TIME2, 1));
      reach_info.advertisers.push_back(IdAppearance(4, TIME1, TIME2, 1));
      reach_info.text_advertisers.push_back(IdAppearance(4, TIME1, TIME2, 1));

      reach_info_list.push_back(reach_info);
    }

    {
      CampaignReachProcessor::ReachInfo reach_info;

      // revert TIME2 record
      reach_info.campaigns.push_back(IdAppearance(1, TIME2, Generics::Time::ZERO, -1));
      reach_info.ccgs.push_back(IdAppearance(2, TIME2, Generics::Time::ZERO, -1));
      reach_info.creatives.push_back(IdAppearance(3, TIME2, Generics::Time::ZERO, -1));
      reach_info.advertisers.push_back(IdAppearance(4, TIME2, Generics::Time::ZERO, -1));
      reach_info.text_advertisers.push_back(IdAppearance(4, TIME2, Generics::Time::ZERO, -1));

      // apply new changes
      reach_info.campaigns.push_back(IdAppearance(1, TIME3, Generics::Time::ZERO, 1));
      reach_info.ccgs.push_back(IdAppearance(2, TIME3, Generics::Time::ZERO, 1));
      reach_info.creatives.push_back(IdAppearance(3, TIME3, Generics::Time::ZERO, 1));
      reach_info.advertisers.push_back(IdAppearance(4, TIME3, Generics::Time::ZERO, 1));
      reach_info.text_advertisers.push_back(IdAppearance(4, TIME3, Generics::Time::ZERO, 1));

      reach_info.campaigns.push_back(IdAppearance(1, TIME2, TIME3, 1));
      reach_info.ccgs.push_back(IdAppearance(2, TIME2, TIME3, 1));
      reach_info.creatives.push_back(IdAppearance(3, TIME2, TIME3, 1));
      reach_info.advertisers.push_back(IdAppearance(4, TIME2, TIME3, 1));
      reach_info.text_advertisers.push_back(IdAppearance(4, TIME2, TIME3, 1));

      reach_info_list.push_back(reach_info);
    }
  }
};

struct TestSumAppearance: public TestSumBase
{
  const Generics::Time TIME1;
  const Generics::Time TIME2;
  const Generics::Time TIME3;
  const Generics::Time TIME4;

  TestSumAppearance()
    : TIME1(String::SubString("2013-01-21"), "%Y-%m-%d"),
      TIME2(String::SubString("2013-01-22"), "%Y-%m-%d"),
      TIME3(String::SubString("2013-01-31"), "%Y-%m-%d"),
      TIME4(String::SubString("2013-03-12"), "%Y-%m-%d")
  {}

  const char* name()
  {
    return "TestSumAppearance";
  }

  void
  fill(RequestInfoList& requests, ReachMap& etalon)
  {
    requests.push_back(create_request_info(TIME1, false));
    requests.push_back(create_request_info(TIME2, false));
    requests.push_back(create_request_info(TIME3, false));
    requests.push_back(create_request_info(TIME4, false));

    etalon[Key(1, TIME1, Generics::Time::ZERO)] = 1;
    etalon[Key(1, TIME2, TIME1)] = 1;
    etalon[Key(1, TIME3, TIME2)] = 1;
    etalon[Key(
      1,
      TIME4,
      Generics::Time(String::SubString("2013-02-09"), "%Y-%m-%d"))] = 1; // - 31 day
  }
};

bool
init(int& argc, char**& argv) /*throw(eh::Exception)*/
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
    return false;
  }
  return true;
}

int
main(int argc, char* argv[]) noexcept
{
  try
  {
    if (!init(argc, argv))
    {
      return 0;
    }

    int result = 0;
    TestList tests;

    tests.push_back(new TestSimpleDailyAppear());
    tests.push_back(new TestOrderedAppearance());
    tests.push_back(new TestReverseAppearance());
    tests.push_back(new TestAdStorm());

    for (TestList::iterator it = tests.begin();
      it != tests.end(); ++it)
    {
      result += (*it)->run();
    }

    std::cout << "to collection sum testing" << std::endl;

    TestSumList sum_tests;
    sum_tests.push_back(new TestSumAppearance());
    for (TestSumList::iterator it = sum_tests.begin();
      it != sum_tests.end(); ++it)
    {
      result += (*it)->run();
    }

    std::cout << "to multi thread testing" << std::endl;

    {
      Logging::Logger_var logger(new Logging::Null::Logger);

      CollReachTestProcessor_var reach_processor(
        new CollReachTestProcessor());

      system(("rm -r " + *root_path + TEST_FOLDER +
        " 2>/dev/null ; mkdir -p " + *root_path + TEST_FOLDER).c_str());

      UserCampaignReachContainer_var reach_container(
        new UserCampaignReachContainer(
          logger,
          reach_processor,
          (*root_path + TEST_FOLDER).c_str(),
          "Reach",
          0));

      TestIt test_it;
      test_it.reach_container = reach_container.in();
      test_it.reach_processor = reach_processor.in();
      result += multi_thread_test(&test_it);
    }

    return result;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  return -1;
}
