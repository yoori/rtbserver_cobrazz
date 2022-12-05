#include <Generics/AppUtils.hpp>
#include <Commons/UserInfoManip.hpp>
#include <RequestInfoSvcs/RequestInfoManager/UserTagRequestMergeContainer.hpp>

using namespace AdServer::RequestInfoSvcs;
using AdServer::Commons::UserId;

namespace
{
  const char DEFAULT_ROOT_PATH[] = "./";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char TEST_FOLDER[] = "/UserTagRequestMergeContainerTestDir/";

  const Generics::Time MERGE_TIME_BOUND(2);

  const char USAGE[] =
    "UserTagRequestMergeContainerTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";

  const Generics::Time TIME_MERGE_BOUND(2);
}

class TestTagGroupProcessor:
  public virtual TagRequestGroupProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
public:
  typedef std::list<TagRequestGroupInfo> TagRequestGroupInfoList;

public:
  virtual void process_tag_request_group(
    const TagRequestGroupInfo& tag_group_info)
    /*throw(Exception)*/
  {
    groups_.push_back(tag_group_info);
  };

  const TagRequestGroupInfoList& result() const noexcept
  {
    return groups_;
  }

protected:
  virtual ~TestTagGroupProcessor() noexcept
  {}

private:
  TagRequestGroupInfoList groups_;
};

typedef ReferenceCounting::SmartPtr<TestTagGroupProcessor>
  TestTagGroupProcessor_var;

struct TestBase: public ReferenceCounting::AtomicImpl
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  int run() noexcept
  {
    try
    {
      Logging::Logger_var logger(new Logging::Null::Logger);

      TestTagGroupProcessor_var processor(
        new TestTagGroupProcessor());

      system(("rm -r " + *root_path + TEST_FOLDER +
        " 2>/dev/null ; mkdir -p " + *root_path + TEST_FOLDER).c_str());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserTagRequestMergeContainer_var container(
        new UserTagRequestMergeContainer(
          logger,
          processor,
          MERGE_TIME_BOUND,
          (*root_path + TEST_FOLDER).c_str(),
          "TagGroups",
          cache));

      run_impl(container);

      TagRequestGroupProcessor::TagRequestGroupInfoList etalon;

      fill_etalon(etalon);

      const TagRequestGroupProcessor::TagRequestGroupInfoList& result =
        processor->result();

      TagRequestGroupProcessor::TagRequestGroupInfoList::
        const_iterator et_it = etalon.begin();
      TagRequestGroupProcessor::TagRequestGroupInfoList::
        const_iterator res_it = result.begin();

      int i = 0;

      for (; et_it != etalon.end() && res_it != result.end();
          ++et_it, ++res_it, ++i)
      {
        if (!(*et_it == *res_it))
        {
          Stream::Error ostr;
          ostr << "result non equal standard - element #" << i << ": " <<
            std::endl << ">>> standard >>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
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
          for (TagRequestGroupProcessor::TagRequestGroupInfoList::
                 const_iterator e_it = result.begin();
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
          for(TagRequestGroupProcessor::TagRequestGroupInfoList::
                const_iterator e_it = etalon.begin();
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

  virtual void
  run_impl(UserTagRequestMergeContainer* cont) = 0;

  virtual void
  fill_etalon(TagRequestGroupProcessor::TagRequestGroupInfoList& etalon) = 0;
};

typedef ReferenceCounting::SmartPtr<TestBase> TestBase_var;
typedef std::list<TestBase_var> TestList;

struct TestPageIdAppearAndChange: public TestBase
{
  const char*
  name()
  {
    return "TestPageIdAppearAndChange";
  }

  void
  run_impl(UserTagRequestMergeContainer* cont)
  {
    const UserId uid = UserId("EEAAAAAAAAAAAAAAAAAAAA..");

    {
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 1;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "";
      tag_request_info.page_load_id = 1;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }

    {
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 2;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "";
      tag_request_info.page_load_id = 2;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }

    {
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 3;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "";
      tag_request_info.page_load_id = 1;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }
  }

  void
  fill_etalon(TagRequestGroupProcessor::TagRequestGroupInfoList& groups_list)
  {
    TagRequestGroupProcessor::TagRequestGroupInfo group_info1;

    {
      group_info1.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      group_info1.colo_id = 1;
      group_info1.site_id = 1;
      group_info1.country = "ru";
      group_info1.tags.insert(1);
      group_info1.ad_shown = false;
      group_info1.rollback = false;
      groups_list.push_back(group_info1);
    }

    {
      TagRequestGroupProcessor::TagRequestGroupInfo group_info;
      group_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      group_info.colo_id = 1;
      group_info.site_id = 1;
      group_info.country = "ru";
      group_info.tags.insert(2);
      group_info.ad_shown = false;
      group_info.rollback = false;
      groups_list.push_back(group_info);
    }

    {
      TagRequestGroupProcessor::TagRequestGroupInfo group_info3(group_info1);
      group_info3.rollback = true;
      groups_list.push_back(group_info3);
      group_info3.rollback = false;
      group_info3.tags.insert(3);
      groups_list.push_back(group_info3);
    }
  }
};

struct TestPageIdAndRefererHashMerge: public TestBase
{
  const char*
  name()
  {
    return "TestPageIdAndRefererHashMerge";
  }

  void
  run_impl(UserTagRequestMergeContainer* cont)
  {
    const UserId uid = UserId("EEAAAAAAAAAAAAAAAAAAAA..");

    {
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 1;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "test.com";
      tag_request_info.page_load_id = 1;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }

    {
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 2;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "test.com";
      tag_request_info.page_load_id = 0;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }
  }

  void
  fill_etalon(TagRequestGroupProcessor::TagRequestGroupInfoList& groups_list)
  {
    TagRequestGroupProcessor::TagRequestGroupInfo group_info1;

    {
      group_info1.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      group_info1.colo_id = 1;
      group_info1.site_id = 1;
      group_info1.country = "ru";
      group_info1.tags.insert(1);
      group_info1.ad_shown = false;
      group_info1.rollback = false;
      groups_list.push_back(group_info1);
    }

    {
      TagRequestGroupProcessor::TagRequestGroupInfo group_info1_rollback(group_info1);
      group_info1_rollback.rollback = true;
      groups_list.push_back(group_info1_rollback);

      TagRequestGroupProcessor::TagRequestGroupInfo group_info;
      group_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      group_info.colo_id = 1;
      group_info.site_id = 1;
      group_info.country = "ru";
      group_info.tags.insert(1);
      group_info.tags.insert(2);
      group_info.ad_shown = false;
      group_info.rollback = false;
      groups_list.push_back(group_info);
    }
  }
};

struct TestRefererAppearAndChange: public TestBase
{
  TestRefererAppearAndChange(bool noise_page_load_id)
    : noise_page_load_id_(noise_page_load_id),
      name_(std::string("TestRefererAppearAndChange") +
        (noise_page_load_id ? "(noised page_load_id)" : ""))
  {}

  virtual ~TestRefererAppearAndChange() noexcept {}

  const char*
  name()
  {
    return name_.c_str();
  }

  void
  run_impl(UserTagRequestMergeContainer* cont)
  {
    const UserId uid = UserId("EEAAAAAAAAAAAAAAAAAAAA..");

    {
      // produce #0
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 1;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "http://test.com";
      tag_request_info.page_load_id = noise_page_load_id_ ? 1 : 0;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }

    {
      // produce #1
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 2;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "http://test2.com";
      tag_request_info.page_load_id = 0;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }

    {
      // produce #2, #3
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 3;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "http://test.com";
      tag_request_info.page_load_id = 0;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }

    {
      // produce #4, #5
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") + TIME_MERGE_BOUND;
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") + TIME_MERGE_BOUND;
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 4;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "http://test.com";
      tag_request_info.page_load_id = 0;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }

    {
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") + TIME_MERGE_BOUND*2 + 1;
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") + TIME_MERGE_BOUND*2 + 1;
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 5;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "http://test.com";
      tag_request_info.page_load_id = noise_page_load_id_ ? 6 : 0;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }
  }

  void
  fill_etalon(TagRequestGroupProcessor::TagRequestGroupInfoList& groups_list)
  {
    TagRequestGroupProcessor::TagRequestGroupInfo group_info1;

    {
      group_info1.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      group_info1.colo_id = 1;
      group_info1.site_id = 1;
      group_info1.country = "ru";
      group_info1.tags.insert(1);
      group_info1.ad_shown = false;
      group_info1.rollback = false;
      // #0
      groups_list.push_back(group_info1);
    }

    {
      TagRequestGroupProcessor::TagRequestGroupInfo group_info;
      group_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      group_info.colo_id = 1;
      group_info.site_id = 1;
      group_info.country = "ru";
      group_info.tags.insert(2);
      group_info.ad_shown = false;
      group_info.rollback = false;
      // #1
      groups_list.push_back(group_info);
    }

    TagRequestGroupProcessor::TagRequestGroupInfo group_info3(group_info1);

    {
      group_info3.rollback = true;
      // #2
      groups_list.push_back(group_info3);
      group_info3.rollback = false;
      group_info3.tags.insert(3);
      // #3
      groups_list.push_back(group_info3);
    }

    {
      group_info3.rollback = true;
      // #4
      groups_list.push_back(group_info3);
      group_info3.rollback = false;
      group_info3.tags.insert(4);
      // #5
      groups_list.push_back(group_info3);
    }

    {
      // # 6
      TagRequestGroupProcessor::TagRequestGroupInfo group_info;
      group_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") +
          TIME_MERGE_BOUND*2 + 1;
      group_info.colo_id = 1;
      group_info.site_id = 1;
      group_info.country = "ru";
      group_info.tags.insert(5);
      group_info.ad_shown = false;
      group_info.rollback = false;
      groups_list.push_back(group_info);
    }
  }

protected:
  bool noise_page_load_id_;
  std::string name_;
};

struct TestAdShownChange: public TestBase
{
  // regression for ADSC-5679
  const char*
  name()
  {
    return "TestAdShownChange";
  }

  void
  run_impl(UserTagRequestMergeContainer* cont)
  {
    const UserId uid = UserId("EEAAAAAAAAAAAAAAAAAAAA..");

    {
      // produce #0
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 1;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "http://test.com";
      tag_request_info.page_load_id = 0;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }

    {
      // produce #1, #2
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 3;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "http://test.com";
      tag_request_info.page_load_id = 0;
      tag_request_info.ad_shown = true;

      cont->process_tag_request(tag_request_info);
    }

    {
      // produce #3, #4
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 4;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "http://test.com";
      tag_request_info.page_load_id = 0;
      tag_request_info.ad_shown = false; // check that ad_shown will be inherited

      cont->process_tag_request(tag_request_info);
    }
  }

  void
  fill_etalon(TagRequestGroupProcessor::TagRequestGroupInfoList& groups_list)
  {
    TagRequestGroupProcessor::TagRequestGroupInfo group_info1;

    {
      group_info1.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      group_info1.colo_id = 1;
      group_info1.site_id = 1;
      group_info1.country = "ru";
      group_info1.tags.insert(1);
      group_info1.ad_shown = false;
      group_info1.rollback = false;
      // #0
      groups_list.push_back(group_info1);
    }

    TagRequestGroupProcessor::TagRequestGroupInfo group_info3(group_info1);

    {
      group_info3.rollback = true;
      // #1
      groups_list.push_back(group_info3);
      group_info3.rollback = false;
      group_info3.ad_shown = true;
      group_info3.tags.insert(3);
      // #2
      groups_list.push_back(group_info3);
    }

    TagRequestGroupProcessor::TagRequestGroupInfo group_info4(group_info3);

    {
      group_info3.rollback = true;
      // #1
      groups_list.push_back(group_info3);
      group_info3.rollback = false;
      group_info3.ad_shown = true;
      group_info3.tags.insert(4);
      // #2
      groups_list.push_back(group_info3);
    }
  }
};

struct TestMergeWithDiffRefererIfDefinedOnePageLoadId: public TestBase
{
  const char*
  name()
  {
    return "TestMergeWithDiffRefererIfDefinedOnePageLoadId";
  }

  void
  run_impl(UserTagRequestMergeContainer* cont)
  {
    const UserId uid = UserId("EEAAAAAAAAAAAAAAAAAAAA..");

    {
      // produce #0
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 1;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "http://test.com";
      tag_request_info.page_load_id = 1;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }

    {
      // produce #1
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") +
          MERGE_TIME_BOUND + 1;
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") +
          MERGE_TIME_BOUND + 1;
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 2;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "http://test2.com";
      tag_request_info.page_load_id = 0;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }

    {
      // produce #2
      TagRequestInfo tag_request_info;
      tag_request_info.user_id = uid;
      tag_request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") +
          (MERGE_TIME_BOUND + 1) * 2;
      tag_request_info.isp_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") +
          (MERGE_TIME_BOUND + 1) * 2;
      tag_request_info.country = "ru";
      tag_request_info.colo_id = 1;
      tag_request_info.site_id = 1;
      tag_request_info.tag_id = 1;
      tag_request_info.user_status = 'I';
      tag_request_info.referer = "http://test.com";
      tag_request_info.page_load_id = 3;
      tag_request_info.ad_shown = false;

      cont->process_tag_request(tag_request_info);
    }
  }

  void
  fill_etalon(TagRequestGroupProcessor::TagRequestGroupInfoList& groups_list)
  {
    {
      TagRequestGroupProcessor::TagRequestGroupInfo group_info;
      group_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      group_info.colo_id = 1;
      group_info.site_id = 1;
      group_info.country = "ru";
      group_info.tags.insert(1);
      group_info.ad_shown = false;
      group_info.rollback = false;
      // #0
      groups_list.push_back(group_info);
    }

    TagRequestGroupProcessor::TagRequestGroupInfo group_info1;

    {
      group_info1.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") +
          MERGE_TIME_BOUND + 1;
      group_info1.colo_id = 1;
      group_info1.site_id = 1;
      group_info1.country = "ru";
      group_info1.tags.insert(2);
      group_info1.ad_shown = false;
      group_info1.rollback = false;
      groups_list.push_back(group_info1);
    }

    {
      TagRequestGroupProcessor::TagRequestGroupInfo group_info;
      group_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") +
          (MERGE_TIME_BOUND + 1) * 2;
      group_info.colo_id = 1;
      group_info.site_id = 1;
      group_info.country = "ru";
      group_info.tags.insert(1);
      group_info.ad_shown = false;
      group_info.rollback = false;
      groups_list.push_back(group_info);
    }
  }
};

int main(int argc, char* argv[]) noexcept
{
  try
  {
    using namespace Generics::AppUtils;
    Args args;
    CheckOption opt_help;

    args.add(equal_name("path") || short_name("p"), root_path);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if(opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    TestList tests;
    tests.push_back(new TestPageIdAppearAndChange());
    tests.push_back(new TestPageIdAndRefererHashMerge());
    tests.push_back(new TestRefererAppearAndChange(false));
    tests.push_back(new TestRefererAppearAndChange(true));
    tests.push_back(new TestAdShownChange());
    tests.push_back(new TestMergeWithDiffRefererIfDefinedOnePageLoadId());

    int result = 0;
    for(TestList::iterator it = tests.begin(); it != tests.end(); ++it)
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
