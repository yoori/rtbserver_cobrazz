// @file RequestInfoSvcs/RequestInfoMapTest.cpp

#include <Generics/AppUtils.hpp>
#include <Generics/DirSelector.hpp>
#include <RequestInfoSvcs/RequestInfoManager/RequestInfoContainer.hpp>

using namespace AdServer;
using namespace AdServer::RequestInfoSvcs;

/**
 * 1. simple profile saving and reading test
 * 2. expire profile clearing test
 */
namespace
{
  const char REQUEST_ID1[] = "1AAAAAAAAAAAAAAAAAAAAA..";
  const char REQUEST_ID2[] = "2AAAAAAAAAAAAAAAAAAAAA..";

  const char DEFAULT_ROOT_PATH[] = "./";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char TEST_FOLDER_SIMPLE[] = "/RequestInfoMapTestDir/Simple/";
  const char TEST_FOLDER_EXPIRE[] = "/RequestInfoMapTestDir/Expire/";

  const char USAGE[] =
    "RequestInfoMapTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";
}

class NewFileChecker
{
  class FileCollector
  {
  public:
    FileCollector(std::set<std::string>& res_val)
      : res(res_val)
    {}
    
    bool
    operator ()(const char* full_path, const struct stat&) noexcept
    {
      res.insert(full_path);
      return true;
    }
    
    std::set<std::string>& res;
  };

public:
  NewFileChecker(const char* path)
    : target_path_(path)
  {
    FileCollector coll(first_state_);
    Generics::DirSelect::directory_selector(
      target_path_.c_str(),
      coll,
      "*",
      Generics::DirSelect::DSF_RECURSIVE);
  };

  void
  new_files(std::vector<std::string>& files)
  {
    std::set<std::string> current_state;
    FileCollector coll(current_state);
    
    Generics::DirSelect::directory_selector(
      target_path_.c_str(),
      coll,
      "*",
      Generics::DirSelect::DSF_RECURSIVE);

    for (std::set<std::string>::const_iterator it =
          current_state.begin();
        it != current_state.end(); ++it)
    {
      if (first_state_.find(*it) == first_state_.end())
      {
        files.push_back(*it);
      }
    }
  }

  std::string target_path_;
  std::set<std::string> first_state_;
};
  
int
user_info_map_test()
{
  {
    /* simple test saving of profile (check that created one file for it) */
    system(("rm -r " + *root_path + TEST_FOLDER_SIMPLE +
      " 2>/dev/null ; mkdir -p " +
      *root_path + TEST_FOLDER_SIMPLE).c_str());

    NewFileChecker file_checker(
      (*root_path + TEST_FOLDER_SIMPLE).c_str());

    RequestInfoMap_var request_info_map = ProfilingCommons::ProfileMapFactory::
      open_transaction_packed_expire_map<
        ProfilingCommons::RequestIdPackHashAdapter,
        ProfilingCommons::RequestIdAccessor>(
        (*root_path + TEST_FOLDER_SIMPLE).c_str(),
        "Request",
        Generics::Time(60));

    {
      Generics::ConstSmartMemBuf_var buf =
        request_info_map->get_profile("AAAAAAAAAAAAAAAAAAAAAA..");
      if (buf.in())
      {
        return 1;
      }

      Generics::SmartMemBuf_var new_buf = new Generics::SmartMemBuf();
      Generics::MemBuf arr(5);
      strcpy(arr.get<char>(), "TEST");
      new_buf->membuf().swap(arr);
      request_info_map->save_profile(REQUEST_ID1, Generics::transfer_membuf(new_buf));
    }

    std::vector<std::string> new_files;
    file_checker.new_files(new_files);
    if (new_files.size() != 1)
    {
      std::cerr << "one file must appear. Appear:" << std::endl;
      for (std::vector<std::string>::const_iterator it =
            new_files.begin();
          it != new_files.end(); ++it)
      {
        std::cerr << "  " << *it << std::endl;
      }
    }
  }

  /*
  {
    RequestInfoMap request_info_map("Test/", "Request", 60);

    {
      Generics::SmartMemBuf_var buf = request_info_map.get_profile("test");
      if (!buf.in())
      {
        return 1;
      }

      if (::strcmp((const char*)buf->get(), "TEST") != 0)
      {
        return 1;
      }
    }
  }
  */  
  {
    /* create record in past & test expired clearing functionality */
    system(("rm -r " + *root_path + TEST_FOLDER_EXPIRE +
      " 2>/dev/null ; mkdir -p " +
      *root_path + TEST_FOLDER_EXPIRE).c_str());

    RequestInfoMap_var request_info_map = ProfilingCommons::ProfileMapFactory::
      open_transaction_packed_expire_map<
        ProfilingCommons::RequestIdPackHashAdapter,
        ProfilingCommons::RequestIdAccessor>(
        (*root_path + TEST_FOLDER_EXPIRE).c_str(),
        "Request",
        Generics::Time(60));

    NewFileChecker file_checker(
      (*root_path + TEST_FOLDER_EXPIRE).c_str());
    
    {
      Generics::SmartMemBuf_var buf(new Generics::SmartMemBuf);
      Generics::MemBuf arr(5);
      strcpy(arr.get<char>(), "TEST");
      buf->membuf().swap(arr);

      Generics::Time save_time(Generics::Time::get_time_of_day() - 60*60);
      request_info_map->save_profile("BAAAAAAAAAAAAAAAAAAAAA..",
        Generics::transfer_membuf(buf), save_time);
      request_info_map->clear_expired(save_time + 61);

      Generics::ConstSmartMemBuf_var rbuf =
        request_info_map->get_profile("BAAAAAAAAAAAAAAAAAAAAA..");
      if (rbuf.in())
      {
        std::cerr << "expire profile didn't removed." << std::endl;
      }
    }

    std::vector<std::string> new_files;
    file_checker.new_files(new_files);
    if (!new_files.empty())
    {
      std::cerr << "appear excess file: " << *new_files.begin()
        << std::endl;
    }
  }
  
  return 0;
}

bool
init(int& argc, char**& argv) /*throw(eh::Exception)*/
{
  using namespace Generics::AppUtils; 
  Args args;
  CheckOption opt_help;

  args.add(
    equal_name("path") ||
    short_name("p"),
    root_path);
  args.add(
    equal_name("help") ||
    short_name("h"),
    opt_help);

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
    return user_info_map_test();
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  return -1;
}
