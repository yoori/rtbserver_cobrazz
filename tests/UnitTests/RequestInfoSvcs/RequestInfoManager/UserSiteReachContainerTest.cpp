/**
 * @file RequestInfoSvcs/UserSiteReachContainerTest.cpp
 */

#include <Generics/AppUtils.hpp>
#include <Commons/UserInfoManip.hpp>

#include <RequestInfoSvcs/RequestInfoManager/UserSiteReachContainer.hpp>

using namespace AdServer::RequestInfoSvcs;
using AdServer::Commons::UserId;

namespace
{
  const char DEFAULT_ROOT_PATH[] = "./";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char TEST_FOLDER[] = "/Reach/";

  const char USAGE[] =
    "UserSiteReachContainerTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";
}

typedef std::list<SiteReachProcessor::SiteReachInfo>
  SiteReachInfoList;

struct CollSiteReachTestProcessor:
  public SiteReachProcessor,
  public ReferenceCounting::AtomicImpl
{
public:
  virtual void process_site_reach(const SiteReachInfo& reach_info)
    /*throw(Exception)*/
  {
    reach_info_list_.push_back(reach_info);
  }

  const SiteReachInfoList& result() const
  {
    return reach_info_list_;
  }

protected:
  virtual ~CollSiteReachTestProcessor() noexcept
  {}

private:
  SiteReachInfoList reach_info_list_;
};

typedef ReferenceCounting::SmartPtr<CollSiteReachTestProcessor>
  CollSiteReachTestProcessor_var;

struct TestBase: public ReferenceCounting::AtomicImpl
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  int
  run() noexcept
  {
    try
    {
      Logging::Logger_var logger(new Logging::Null::Logger);

      CollSiteReachTestProcessor_var reach_processor(
        new CollSiteReachTestProcessor());

      system(("rm -r " + *root_path + TEST_FOLDER +
        " 2>/dev/null ; mkdir -p " + *root_path + TEST_FOLDER).c_str());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserSiteReachContainer_var reach_container(
        new UserSiteReachContainer(
          logger,
          reach_processor,
          (*root_path + TEST_FOLDER).c_str(),
          "Reach",
          cache));

      run_impl(reach_container);

      SiteReachInfoList etalon;

      fill_etalon(etalon);

      const SiteReachInfoList& result = reach_processor->result();

      SiteReachInfoList::const_iterator et_it = etalon.begin();
      SiteReachInfoList::const_iterator res_it = result.begin();

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
          for (SiteReachInfoList::const_iterator e_it = result.begin();
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
          for (SiteReachInfoList::const_iterator e_it = etalon.begin();
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

  virtual const char* name() = 0;

  virtual void run_impl(UserSiteReachContainer* cont) = 0;

  virtual void fill_etalon(SiteReachInfoList& etalon) = 0;

  TagRequestInfo create_tag_request_info(
    const UserId& user_id,
    const Generics::Time& isp_time,
    unsigned long site_id)
  {
    TagRequestInfo ret;
    ret.colo_id = 1;
    ret.tag_id = 2;
    ret.user_status = 'U';
    ret.isp_time = isp_time;
    ret.site_id = site_id;
    ret.page_load_id = 3;
    ret.ad_shown = false;
    ret.user_id = user_id;
    return ret;
  }
};

typedef ReferenceCounting::SmartPtr<TestBase> TestBase_var;
typedef std::list<TestBase_var> TestList;

struct TestSimpleDailyAppear : public TestBase
{
  const char*
  name()
  {
    return "SimpleDailyAppear";
  }

  void
  run_impl(UserSiteReachContainer* cont)
  {
    TagRequestInfo tag_request_info = create_tag_request_info(
      UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
      Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"),
      1);

    cont->process_tag_request(tag_request_info);
  }

  void
  fill_etalon(SiteReachInfoList& reach_info_list)
  {
    SiteReachProcessor::SiteReachInfo reach_info;
    reach_info.appearance_list.push_back(IdAppearance(
      1,
      Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"),
      Generics::Time::ZERO,
      1));
    reach_info_list.push_back(reach_info);
  }
};

typedef ReferenceCounting::SmartPtr<TestBase> TestBase_var;
typedef std::list<TestBase_var> TestList;

struct TestOrderedAppearance : public TestBase
{
  const char*
  name()
  {
    return "TestOrderedAppearance";
  }

  void
  run_impl(UserSiteReachContainer* cont)
  {
    {
      TagRequestInfo tag_request_info = create_tag_request_info(
        UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"),
        1);

      cont->process_tag_request(tag_request_info);
    }

    {
      TagRequestInfo tag_request_info = create_tag_request_info(
        UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"),
        1);

      cont->process_tag_request(tag_request_info);
    }

    {
      TagRequestInfo tag_request_info = create_tag_request_info(
        UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
        Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"),
        1);

      cont->process_tag_request(tag_request_info);
    }

    {
      TagRequestInfo tag_request_info = create_tag_request_info(
        UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
        Generics::Time(String::SubString("2008-01-10"), "%Y-%m-%d"),
        1);

      cont->process_tag_request(tag_request_info);
    }
  }

  void
  fill_etalon(SiteReachInfoList& reach_info_list)
  {
    {
      SiteReachProcessor::SiteReachInfo reach_info;
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"),
        Generics::Time::ZERO,
        1));
      reach_info_list.push_back(reach_info);
    }

    {
      SiteReachProcessor::SiteReachInfo reach_info;
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"),
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"),
        1));
      reach_info_list.push_back(reach_info);
    }

    {
      SiteReachProcessor::SiteReachInfo reach_info;
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-10"), "%Y-%m-%d"),
        Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"),
        1));
      reach_info_list.push_back(reach_info);
    }
  }
};

struct TestUnorderedAppearance : public TestBase
{
  const char*
  name()
  {
    return "TestUnorderedAppearance";
  }

  void
  run_impl(UserSiteReachContainer* cont)
  {
    // #0
    {
      TagRequestInfo tag_request_info = create_tag_request_info(
        UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
        Generics::Time(String::SubString("2008-01-10"), "%Y-%m-%d"),
        1);

      cont->process_tag_request(tag_request_info);
    }

    // #1
    {
      TagRequestInfo tag_request_info = create_tag_request_info(
        UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
        Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"),
        1);

      cont->process_tag_request(tag_request_info);
    }

    // #2
    {
      TagRequestInfo tag_request_info = create_tag_request_info(
        UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
        Generics::Time(String::SubString("2008-01-03"), "%Y-%m-%d"),
        1);

      cont->process_tag_request(tag_request_info);
    }

    // # 3
    {
      TagRequestInfo tag_request_info = create_tag_request_info(
        UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"),
        1);

      cont->process_tag_request(tag_request_info);
    }
  }

  void
  fill_etalon(SiteReachInfoList& reach_info_list)
  {
    // #0
    {
      SiteReachProcessor::SiteReachInfo reach_info;
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-10"), "%Y-%m-%d"),
        Generics::Time::ZERO,
        1));
      reach_info_list.push_back(reach_info);
    }

    // #1
    {
      // revert first changes
      SiteReachProcessor::SiteReachInfo reach_info;
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-10"), "%Y-%m-%d"),
        Generics::Time::ZERO,
        -1));
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"),
        Generics::Time::ZERO,
        1));
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-10"), "%Y-%m-%d"),
        Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"),
        1));
      reach_info_list.push_back(reach_info);
    }

    // #2
    {
      // 2008-01-02, 2008-01-10 + 2008-01-03
      // revert first changes
      SiteReachProcessor::SiteReachInfo reach_info;
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-10"), "%Y-%m-%d"),
        Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"),
        -1));
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-03"), "%Y-%m-%d"),
        Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"),
        1));
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-10"), "%Y-%m-%d"),
        Generics::Time(String::SubString("2008-01-03"), "%Y-%m-%d"),
        1));
      reach_info_list.push_back(reach_info);
    }

    // #3
    {
      // 2008-01-02, 2008-01-10 + 2008-01-03
      // revert first changes
      SiteReachProcessor::SiteReachInfo reach_info;
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"),
        Generics::Time::ZERO,
        -1));
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"),
        Generics::Time::ZERO,
        1));
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-02"), "%Y-%m-%d"),
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"),
        1));
      reach_info_list.push_back(reach_info);
    }
  }
};

struct TestReverseAppearance : public TestBase
{
  const char*
  name()
  {
    return "TestReverseAppearance";
  }

  void
  run_impl(UserSiteReachContainer* cont)
  {
    // #0
    {
      TagRequestInfo tag_request_info = create_tag_request_info(
        UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
        Generics::Time(String::SubString("2008-01-15"), "%Y-%m-%d"),
        1);

      cont->process_tag_request(tag_request_info);
    }

    // #1
    {
      TagRequestInfo tag_request_info = create_tag_request_info(
        UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
        Generics::Time(String::SubString("2008-01-14"), "%Y-%m-%d"),
        1);

      cont->process_tag_request(tag_request_info);
    }

    // #2
    {
      TagRequestInfo tag_request_info = create_tag_request_info(
        UserId("EEAAAAAAAAAAAAAAAAAAAA.."),
        Generics::Time(String::SubString("2008-01-10"), "%Y-%m-%d"),
        1);

      cont->process_tag_request(tag_request_info);
    }
  }

  void
  fill_etalon(SiteReachInfoList& reach_info_list)
  {
    // #0
    {
      SiteReachProcessor::SiteReachInfo reach_info;
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-15"), "%Y-%m-%d"),
        Generics::Time::ZERO,
        1));
      reach_info_list.push_back(reach_info);
    }

    // #1
    {
      // revert first changes
      SiteReachProcessor::SiteReachInfo reach_info;
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-15"), "%Y-%m-%d"),
        Generics::Time::ZERO,
        -1));
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-14"), "%Y-%m-%d"),
        Generics::Time::ZERO,
        1));
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-15"), "%Y-%m-%d"),
        Generics::Time(String::SubString("2008-01-14"), "%Y-%m-%d"),
        1));
      reach_info_list.push_back(reach_info);
    }

    // #2
    {
      SiteReachProcessor::SiteReachInfo reach_info;
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-14"), "%Y-%m-%d"),
        Generics::Time::ZERO,
        -1));
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-10"), "%Y-%m-%d"),
        Generics::Time::ZERO,
        1));
      reach_info.appearance_list.push_back(IdAppearance(
        1,
        Generics::Time(String::SubString("2008-01-14"), "%Y-%m-%d"),
        Generics::Time(String::SubString("2008-01-10"), "%Y-%m-%d"),
        1));
      reach_info_list.push_back(reach_info);
    }
  }
};

int
main(int argc, char* argv[]) noexcept
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

    tests.push_back(new TestOrderedAppearance());
    tests.push_back(new TestUnorderedAppearance());
    tests.push_back(new TestReverseAppearance());

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
