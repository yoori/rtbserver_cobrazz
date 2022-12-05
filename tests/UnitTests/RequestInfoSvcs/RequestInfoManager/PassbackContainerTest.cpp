// @file RequestInfoSvcs/PassbackContainerTest.cpp

#include <map>

#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/Rand.hpp>

#include "PassbackContainerMTTest.hpp"

using namespace AdServer::RequestInfoSvcs;
using AdServer::Commons::UserId;
using AdServer::Commons::RequestId;

namespace
{
  const char USAGE[] =
    "PassbackContainerTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";

  const char DEFAULT_ROOT_PATH[] = "./";
}

RequestId
create_request_id()
{
  return RequestId::create_random_based();
}

TagRequestInfo
create_simple_tag_request(const Generics::Time& time)
{
  TagRequestInfo tag_request_info;
  tag_request_info.colo_id = 1;
  tag_request_info.user_status = 'I';
  tag_request_info.time = time;
  tag_request_info.site_id = 1;
  tag_request_info.tag_id = 2;
  tag_request_info.request_id = create_request_id();
  return tag_request_info;
}

PassbackProcessor::PassbackInfo
create_simple_passback_info(const Generics::Time& time)
{
  PassbackProcessor::PassbackInfo passback_info;
  passback_info.colo_id = 1;
  passback_info.user_status = 'I';
  passback_info.time = time;
  passback_info.tag_id = 2;
  return passback_info;
}

struct TestBase: public virtual ReferenceCounting::DefaultImpl<>
{
  typedef TestPassbackProcessorImpl::PassbackInfoList PassbackInfoList;

  virtual const char* name() = 0;

  virtual void run(PassbackContainer* passback_container) /*throw(eh::Exception)*/ = 0;

  virtual void fill_etalon(
    PassbackInfoList& passback_info_list) noexcept = 0;

  static void print_passback_list(
    std::ostream& out,
    const PassbackInfoList& lst,
    const char* offset) noexcept
  {
    unsigned long i = 0;
    for(PassbackInfoList::const_iterator pit = lst.begin();
        pit != lst.end(); ++pit, ++i)
    {
      out << offset << "#" << i << std::endl;
      pit->print(out, (std::string(offset) + "  ").c_str());
    }
  }
};

typedef ReferenceCounting::SmartPtr<TestBase> TestBase_var;
typedef std::list<TestBase_var> TestBaseList;

struct VerificationOfNonExistsPassbackTest: public TestBase
{
  const char* name()
  {
    return "VerificationOfNonExistsPassbackTest";
  }

  virtual void run(PassbackContainer* passback_container) /*throw(eh::Exception)*/
  {
    TagRequestInfo tag_request_info =
      create_simple_tag_request(Generics::Time(10));

    passback_container->process_tag_request(tag_request_info);
  }

  virtual void fill_etalon(PassbackInfoList& /*etalon*/) noexcept
  {}
};

struct UnverifiedPassbackTest: public TestBase
{
  const char* name()
  {
    return "UnverifiedPassbackTest";
  }

  virtual void run(PassbackContainer* passback_container) /*throw(eh::Exception)*/
  {
    RequestId req_id = create_request_id();

    passback_container->process_passback_request(
      req_id, Generics::Time(10));
  }

  virtual void fill_etalon(PassbackInfoList& /*etalon*/) noexcept
  {}
};

struct VerifiedPassbackTest: public TestBase
{
  const char* name()
  {
    return "VerifiedPassbackTest";
  }

  virtual void run(PassbackContainer* passback_container) /*throw(eh::Exception)*/
  {
    TagRequestInfo tag_request_info =
      create_simple_tag_request(Generics::Time(10));

    passback_container->process_tag_request(tag_request_info);

    passback_container->process_passback_request(
      tag_request_info.request_id,
      Generics::Time(10));
  }

  virtual void fill_etalon(PassbackInfoList& etalon) noexcept
  {
    etalon.push_back(create_simple_passback_info(Generics::Time(10)));
  }
};

struct VerifiedPassbackReverseOrderTest: public TestBase
{
  const char* name()
  {
    return "VerifiedPassbackReverseOrderTest";
  }

  virtual void run(PassbackContainer* passback_container) /*throw(eh::Exception)*/
  {
    TagRequestInfo tag_request_info =
      create_simple_tag_request(Generics::Time(9));

    passback_container->process_passback_request(
      tag_request_info.request_id,
      Generics::Time(10));

    passback_container->process_tag_request(tag_request_info);
  }

  virtual void fill_etalon(PassbackInfoList& etalon) noexcept
  {
    etalon.push_back(create_simple_passback_info(Generics::Time(10)));
  }
};

struct DoublePassbackVerificationTest: public TestBase
{
  const char* name()
  {
    return "DoublePassbackVerificationTest";
  }

  virtual void run(PassbackContainer* passback_container) /*throw(eh::Exception)*/
  {
    TagRequestInfo tag_request_info =
      create_simple_tag_request(Generics::Time(9));

    passback_container->process_tag_request(tag_request_info);

    passback_container->process_passback_request(
      tag_request_info.request_id,
      Generics::Time(10));

    passback_container->process_passback_request(
      tag_request_info.request_id,
      Generics::Time(20));
  }

  virtual void fill_etalon(PassbackInfoList& etalon) noexcept
  {
    etalon.push_back(create_simple_passback_info(Generics::Time(10)));
  }
};

struct DoublePassbackTest: public TestBase
{
  const char* name()
  {
    return "DoublePassbackTest";
  }

  virtual void run(PassbackContainer* passback_container) /*throw(eh::Exception)*/
  {
    TagRequestInfo tag_request_info =
      create_simple_tag_request(Generics::Time(9));

    passback_container->process_tag_request(tag_request_info);

    passback_container->process_tag_request(tag_request_info);

    passback_container->process_passback_request(
      tag_request_info.request_id,
      Generics::Time(10));

    passback_container->process_passback_request(
      tag_request_info.request_id,
      Generics::Time(20));
  }

  virtual void fill_etalon(PassbackInfoList& etalon) noexcept
  {
    etalon.push_back(create_simple_passback_info(Generics::Time(10)));
  }
};

int
passback_container_test(const char* test_folder) noexcept
{
  int result = 0; // number of failed scenarious

  try
  {
    system((std::string("rm -r ") + test_folder +
      " 2>/dev/null ; mkdir -p " + test_folder).c_str());

    Logging::Logger_var logger(new Logging::Null::Logger);

    TestPassbackProcessorImpl_var passback_processor(
      new TestPassbackProcessorImpl());

    AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
      new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

    PassbackContainer_var passback_container(
      new PassbackContainer(
        logger,
        passback_processor,
        test_folder,
        "Passback",
        cache,
        Generics::Time(10), // expire time (sec)
        Generics::Time(2)));

    TestBaseList tests;
    tests.push_back(TestBase_var(new VerificationOfNonExistsPassbackTest()));
    tests.push_back(TestBase_var(new UnverifiedPassbackTest()));
    tests.push_back(TestBase_var(new VerifiedPassbackTest()));
    tests.push_back(TestBase_var(new VerifiedPassbackReverseOrderTest()));
    tests.push_back(TestBase_var(new DoublePassbackVerificationTest()));

    for(TestBaseList::iterator test_it = tests.begin();
        test_it != tests.end(); ++test_it)
    {
      try
      {
        passback_processor->clear();

        (*test_it)->run(passback_container);

        TestBase::PassbackInfoList etalon;
        (*test_it)->fill_etalon(etalon);

        if(passback_processor->result().size() !=
             etalon.size() ||
           !std::equal(
             etalon.begin(),
             etalon.end(),
             passback_processor->result().begin()))
        {
          std::cerr << (*test_it)->name() << ": unexpected result" <<
            (passback_processor->result().size() != etalon.size() ?
              "(incorrect passback number)" : "") << ": " << std::endl <<
            "  result:" << std::endl;
          TestBase::print_passback_list(std::cerr, passback_processor->result(), "    ");
          std::cerr << "  etalon: " << std::endl;
          TestBase::print_passback_list(std::cerr, etalon, "    ");

          ++result;
        }
      }
      catch(const eh::Exception& ex)
      {
        std::cerr << (*test_it)->name() << ": caught eh::Exception: " <<
          ex.what() << std::endl;
        ++result;
      }
    }

    // MT tests
    TestIt test_it = {passback_container, passback_processor};
    multi_thread_test(&test_it);
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "Caught eh::Exception: " << ex.what() << std::endl;
    ++result;
  }

  return result;
}

int
main(int argc, char* argv[]) noexcept
{
  try
  {
    using namespace Generics::AppUtils;

    Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);
    CheckOption opt_help;

    Args args;
    args.add(equal_name("path") || short_name("p"), root_path);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return false;
    }

    return passback_container_test(
      (*root_path + "/PassbackContainerTestDir/").c_str());
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  return -1;
}
