// @file ExpressionMatcher/UserColoReachContainerTest.cpp
/*
 * Most part of cases covered by UserCampaignReachContainerTest.
 *   But UserColoReachContainer and UserInventoryContainer have some specific:
 *   revert and resave on update (isp_)create_time.
 *   (both container have common logic for revert-resave,
 *   use function: Algs::collect_all_appearance(...))
 *   This unit test currently cover bugs: ADSC_8351, ADSC-8344.
 */
#include <map>
#include <list>

#include <Generics/AppUtils.hpp>
#include <Generics/DirSelector.hpp>
#include <Generics/Uuid.hpp>
#include <RequestInfoSvcs/RequestInfoCommons/UserChannelInventoryProfile.hpp>

#include <RequestInfoSvcs/ExpressionMatcher/UserColoReachContainer.hpp>
#include <Sync/PosixLock.hpp>

using namespace AdServer;
using namespace AdServer::RequestInfoSvcs;

namespace
{
  const char DEFAULT_ROOT_PATH[] = "./";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char TEST_FOLDER[] = "/ColoReach/";

  const char USAGE[] =
    "UserInventoryContainerTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";

  const AdServer::Commons::UserId TEST_UID("BAAAAAAAAAAAAAAAAAAAAA..");
  const Generics::Time delta_tz = Generics::Time::ZERO;
}

typedef ColoReachProcessor::ColoReachInfo ColoReachInfo;
typedef std::list<ColoReachInfo> ColoReachInfoList;

struct UserColoReachTestProcessor:
  public virtual ColoReachProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
  virtual void
  process_gmt_colo_reach(
    const ColoReachInfo& request_info)
    /*throw(ColoReachProcessor::Exception)*/
  {
    Sync::PosixGuard lock(mutex_gmt_);
    result_gmt_.push_back(request_info);
  }

  virtual void
  process_isp_colo_reach(
    const ColoReachInfo& request_info)
    /*throw(ColoReachProcessor::Exception)*/
  {
    Sync::PosixGuard lock(mutex_isp_);
    result_isp_.push_back(request_info);
  }

  const ColoReachInfoList&
  result_gmt() const
  {
    Sync::PosixGuard lock(mutex_gmt_);
    return result_gmt_;
  }

  const ColoReachInfoList&
  result_isp() const
  {
    Sync::PosixGuard lock(mutex_isp_);
    return result_isp_;
  }

protected:
  virtual ~UserColoReachTestProcessor() noexcept
  {}

private:
  // MT version 
  mutable Sync::PosixMutex mutex_gmt_;
  mutable Sync::PosixMutex mutex_isp_;

  ColoReachInfoList result_gmt_;
  ColoReachInfoList result_isp_;
};

typedef ReferenceCounting::SmartPtr<UserColoReachTestProcessor>
  UserColoReachTestProcessor_var;

struct TestBase : public ReferenceCounting::AtomicImpl
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  int
  run() noexcept
  {
    try
    {
      Logging::Logger_var logger(new Logging::Null::Logger);

      UserColoReachTestProcessor_var reach_processor(
        new UserColoReachTestProcessor());

      system(("rm -r " + *root_path + TEST_FOLDER +
        " 2>/dev/null ; mkdir -p " + *root_path + TEST_FOLDER +
        " ; mkdir -p " + *root_path + TEST_FOLDER + "Chunk_0_4" +
        " ; mkdir -p " + *root_path + TEST_FOLDER + "Chunk_1_4" +
        " ; mkdir -p " + *root_path + TEST_FOLDER + "Chunk_2_4" +
        " ; mkdir -p " + *root_path + TEST_FOLDER + "Chunk_3_4").c_str());

      AdServer::ProfilingCommons::ProfileMapFactory::ChunkPathMap chunk_folders;
      AdServer::ProfilingCommons::ProfileMapFactory::fetch_chunk_folders(
        chunk_folders,
        (*root_path + TEST_FOLDER).c_str());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var profile_cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(1));

      UserColoReachContainer_var reach_container(
        new UserColoReachContainer(
          logger,
          reach_processor, // ExpressionMatcherOutLogger
          true, // household
          chunk_folders.size(), // chunks_number
          chunk_folders,
          "ColoReach", // prefix
          profile_cache,
          AdServer::ProfilingCommons::LevelMapTraits(
            AdServer::ProfilingCommons::LevelMapTraits::BLOCK_RUNTIME,
            10*1024*1024,
            10*1024*1024,
            20*1024*1024,
            20,
            Generics::Time::ZERO)
          ));

      reach_container->activate_object();

      run_impl(reach_container);

      reach_container->deactivate_object();
      reach_container->wait_object();

      ColoReachInfoList isp_etalon, gmt_etalon;
      bool check_etalon_gmt = fill_gmt_etalon(gmt_etalon);
      bool check_etalon_isp = fill_isp_etalon(isp_etalon);

      if(check_etalon_gmt)
      {
        // gmt (GlobalColoUserStat) check 
        const ColoReachInfoList& gmt_result = reach_processor->result_gmt();
        compare_list(gmt_etalon, gmt_result, "colo reach GMT");
      }

      if(check_etalon_isp)
      {
        // isp (ColoUserStat) check
        const ColoReachInfoList& isp_result = reach_processor->result_isp();
        compare_list(isp_etalon, isp_result, "colo reach ISP");
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
  run_impl(UserColoReachContainer* cont) = 0;

  virtual bool
  fill_gmt_etalon(ColoReachInfoList& /*gmt_etalon*/)
  {
    return false;
  }

  virtual bool
  fill_isp_etalon(ColoReachInfoList& /*isp_etalon*/)
  {
    return false;
  }

  static
  UserColoReachContainer::RequestInfo
  create_request_info(
    const Generics::Time& time,
    const Generics::Time& delta_tz = Generics::Time::ZERO
    ) noexcept
  {
    UserColoReachContainer::RequestInfo request_info;

    request_info.user_id = TEST_UID;
    request_info.time = time;
    request_info.isp_time = (delta_tz == Generics::Time::ZERO) ? time : (time + delta_tz);
    request_info.colo_id = 1;

    return request_info;
  }

  protected:

  void
  compare_list(
    const ColoReachInfoList& etalon,
    const ColoReachInfoList& result,
    const char* result_name) /*throw(Exception)*/
  {
    ColoReachInfoList::const_iterator et_it = etalon.begin();
    ColoReachInfoList::const_iterator res_it = result.begin();

    int i = 0;

    for(; et_it != etalon.end() && res_it != result.end();
      ++et_it, ++res_it, ++i)
    {
      if(!(*et_it == *res_it))
      {
        Stream::Error ostr;
        ostr << result_name << " result non equal standard - element #" << i <<
          ": " << std::endl << ">>> standard >>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
        et_it->print(ostr, "  ");
        ostr << std::endl << "=== result =======================" << std::endl;
        res_it->print(ostr, "  ");
        ostr << std::endl << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<";
        throw Exception(ostr);
      }
    }

    if(!(et_it == etalon.end() && res_it == result.end()))
    {
      Stream::Error ostr;
      ostr << result_name << " result size non equal size = " << result.size() <<
        " of standard = " << etalon.size() << ". Result: " << std::endl;

      int i = 0;
      for(ColoReachInfoList::const_iterator e_it = result.begin();
        e_it != result.end();
        ++e_it, ++i)
      {
        ostr << "===== #" << i << " =======" << std::endl;
        e_it->print(ostr, "  ");
        ostr << std::endl;
      }
      throw Exception(ostr);
    }
  }
};

typedef ReferenceCounting::SmartPtr<TestBase> TestBase_var;
typedef std::list<TestBase_var> TestList;

// ADSC-8344
struct TestCreateDateISPTimeZoneLogging: public TestBase
{
  UserColoReachContainer::RequestInfo ri1;
  UserColoReachContainer::RequestInfo ri2;

  TestCreateDateISPTimeZoneLogging()
  {
    {
      ri1 = create_request_info(
        Generics::Time(
          String::SubString("2014-11-26 01:00:00"), "%Y-%m-%d %H:%M:%S"));
      ri1.isp_time = ri1.time - Generics::Time::ONE_HOUR*2;
    }

    {
      ri2 = create_request_info(
        Generics::Time(
          String::SubString("2014-11-26 01:10:00"), "%Y-%m-%d %H:%M:%S"));
      ri2.colo_id = 2;
    }
  }

  virtual
  const char* name()
  {
    return "CreateDateISPTimeZoneLogging (ADSC-8344)";
  }

  virtual
  void
  run_impl(UserColoReachContainer* cont)
  {
    cont->process_request(ri1);
    cont->process_request(ri2);
  }

  virtual
  bool
  fill_isp_etalon(ColoReachInfoList& l)
  {
    {
      ColoReachProcessor::ColoReachInfo reach_info;
      reach_info.create_time = Algs::round_to_day(ri1.isp_time);
      reach_info.colocations.push_back(
        IdAppearance(
          ri1.colo_id,
          Algs::round_to_day(ri1.isp_time),
          Generics::Time::ZERO,
          1));

      l.push_back(reach_info);
    }

    {
      ColoReachProcessor::ColoReachInfo reach_info;

      // Note: create_time logged in TZ for colo_id: 2
      reach_info.create_time = Algs::round_to_day(ri2.isp_time);
      reach_info.colocations.push_back(
        IdAppearance(
          ri2.colo_id,
          Algs::round_to_day(ri2.isp_time),
          Generics::Time::ZERO,
          1));
      l.push_back(reach_info);
    }

    return true;
  }

  virtual
  bool
  fill_gmt_etalon(ColoReachInfoList& /*l*/)
  {
    return false;
  }
};


// ADSC-8351
struct TestLastAppearanceDateBound: public TestBase
{
  const Generics::Time TODAY;
  const Generics::Time TODAY_32;
  const Generics::Time TODAY_33;

  const Generics::Time TIME1;
  const Generics::Time TIME2;
  const Generics::Time TIME3;

  TestLastAppearanceDateBound()
    : TODAY(String::SubString("2014-11-26 12:00:00"), "%Y-%m-%d %H:%M:%S"),
      TODAY_32(TODAY - Generics::Time::ONE_DAY*32),
      TODAY_33(TODAY - Generics::Time::ONE_DAY*33),
      TIME1(TODAY_32),
      TIME2(TODAY),
      TIME3(TODAY_33)
  {}

  virtual
  const char* name()
  {
    return "LastAppearanceDateBound (ADSC-8351)";
  }

  virtual
  void
  run_impl(UserColoReachContainer* cont)
  {
    cont->process_request(create_request_info(TIME1));
    cont->process_request(create_request_info(TIME2));
    cont->process_request(create_request_info(TIME3));
  }

  virtual
  bool
  fill_isp_etalon(ColoReachInfoList& l)
  {
    {
      // today - 32
      ColoReachProcessor::ColoReachInfo reach_info;
      reach_info.create_time = Algs::round_to_day(TIME1);
      reach_info.colocations.push_back(IdAppearance(1, Algs::round_to_day(TIME1), Generics::Time::ZERO, 1));

      l.push_back(reach_info);
    }

    {
      // today
      ColoReachProcessor::ColoReachInfo reach_info;
      reach_info.create_time = Algs::round_to_day(TIME1);
      // Note: last_appear limited by 31 day, not today - 32.
      reach_info.colocations.push_back(IdAppearance(1, Algs::round_to_day(TIME2), Algs::round_to_day(TIME2) - Generics::Time::ONE_DAY*31, 1));

      l.push_back(reach_info);
    }

    {
      // today - 33
      {
        // revert old records create date
        ColoReachProcessor::ColoReachInfo reach_info;
        reach_info.create_time = Algs::round_to_day(TIME1);
        // Note: last_appear limited by 31 day on revert
        reach_info.colocations.push_back(IdAppearance(1, Algs::round_to_day(TIME2), Algs::round_to_day(TIME2) - Generics::Time::ONE_DAY*31, -1));
        reach_info.colocations.push_back(IdAppearance(1, Algs::round_to_day(TIME1), Generics::Time::ZERO, -1));
        l.push_back(reach_info);
      }
      {
        // update old records with new create date
        ColoReachProcessor::ColoReachInfo reach_info;
        reach_info.create_time = Algs::round_to_day(TIME3);
        // Note: last_appear limited by 31 day on resave
        reach_info.colocations.push_back(IdAppearance(1, Algs::round_to_day(TIME2), Algs::round_to_day(TIME2) - Generics::Time::ONE_DAY*31, 1));
        reach_info.colocations.push_back(IdAppearance(1, Algs::round_to_day(TIME1), Generics::Time::ZERO, 1));
        l.push_back(reach_info);
      }
      {
        // Add new record for the current request.
        ColoReachProcessor::ColoReachInfo reach_info;
        reach_info.create_time = Algs::round_to_day(TIME3);
        reach_info.colocations.push_back(IdAppearance(1, Algs::round_to_day(TIME1), Generics::Time::ZERO, -1));
        reach_info.colocations.push_back(IdAppearance(1, Algs::round_to_day(TIME3), Generics::Time::ZERO, 1));
        reach_info.colocations.push_back(IdAppearance(1, Algs::round_to_day(TIME1), Algs::round_to_day(TIME3), 1));
        l.push_back(reach_info);
      }
    }

    return true;
  }

  virtual
  bool
  fill_gmt_etalon(ColoReachInfoList& /*l*/)
  {
    return false;
  }
};

int main(int argc, char** argv) noexcept
{
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
    tests.push_back(new TestLastAppearanceDateBound());
    tests.push_back(new TestCreateDateISPTimeZoneLogging());

    for (TestList::iterator it = tests.begin();
      it != tests.end(); ++it)
    {
      result += (*it)->run();
    }

    return result;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return -1;
}


