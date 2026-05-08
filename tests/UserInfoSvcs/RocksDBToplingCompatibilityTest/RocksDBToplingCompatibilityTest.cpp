#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <ftw.h>
#include <sys/stat.h>
#include <unistd.h>

#include <Generics/AppUtils.hpp>
#include <Generics/MemBuf.hpp>
#include <Generics/Time.hpp>

#include <ProfilingCommons/ProfileMap/RocksDBProfileMap.hpp>
#include <ProfilingCommons/ProfileMap/ToplingDBProfileMap.hpp>

namespace
{
  struct Record final
  {
    std::string key;
    std::string value;
  };

  Generics::ConstSmartMemBuf_var
  make_profile(const std::string& value)
  {
    return Generics::ConstSmartMemBuf_var(
      new Generics::ConstSmartMemBuf(value.data(), value.size()));
  }

  std::string
  read_profile(const Generics::ConstSmartMemBuf* profile)
  {
    if(!profile)
    {
      return {};
    }

    return std::string(
      static_cast<const char*>(profile->membuf().data()),
      profile->membuf().size());
  }

  int
  remove_path_callback(
    const char* path,
    const struct stat*,
    int,
    struct FTW*)
  {
    return ::remove(path);
  }

  void
  remove_all(const std::string& path)
  {
    if(::nftw(path.c_str(), remove_path_callback, 64, FTW_DEPTH | FTW_PHYS) != 0 &&
      errno != ENOENT)
    {
      const int error = errno;
      throw std::runtime_error(
        "can't remove '" + path + "': " + std::strerror(error));
    }
  }

  void
  print_usage()
  {
    std::cerr
      << "Usage: RocksDBToplingCompatibilityTest [OPTIONS]\n"
      << "Options:\n"
      << "  --path <path>    DB path (default: /tmp/RocksDBToplingCompatibilityTest.<pid>)\n"
      << "  --keep-db <0|1>  keep DB directory after test (default: 0)\n";
  }
}

int
main(int argc, char** argv)
{
  try
  {
    using namespace Generics::AppUtils;
    using RocksMap =
      AdServer::ProfilingCommons::RocksDBProfileMap<std::string>;
    using ToplingMap =
      AdServer::ProfilingCommons::ToplingDBProfileMap<std::string>;

    StringOption opt_path(
      std::string("/tmp/RocksDBToplingCompatibilityTest.") +
      std::to_string(::getpid()));
    Option<unsigned int> opt_keep_db(0);
    CheckOption opt_help;

    Args args(-1);
    args.add(equal_name("path") || short_name("p"), opt_path);
    args.add(equal_name("keep-db"), opt_keep_db);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if(opt_help.enabled())
    {
      print_usage();
      return 0;
    }

    const std::vector<Record> records = {
      {"compat/key/1", "rocks-value-1"},
      {"compat/key/2", "rocks-value-2-with-longer-body"},
      {"compat/key/3", std::string("\0binary-value\0tail", 18)}
    };

    remove_all(*opt_path);

    {
      RocksMap rocks_map(String::SubString(*opt_path), Generics::Time::ZERO);
      for(const auto& record : records)
      {
        Generics::ConstSmartMemBuf_var profile = make_profile(record.value);
        rocks_map.save_profile(
          record.key,
          profile.in(),
          Generics::Time::get_time_of_day());
      }
    }

    {
      ToplingMap topling_map(
        String::SubString(*opt_path),
        Generics::Time::ZERO,
        1,
        16);
      topling_map.activate_object();

      for(const auto& record : records)
      {
        Generics::ConstSmartMemBuf_var profile =
          topling_map.get_profile(record.key);
        const std::string actual = read_profile(profile.in());
        if(actual != record.value)
        {
          std::cerr << "mismatch for key '" << record.key << "': expected " <<
            record.value.size() << " bytes, got " << actual.size() <<
            " bytes" << std::endl;
          topling_map.deactivate_object();
          topling_map.wait_object();
          return 1;
        }
      }

      const auto missing = topling_map.get_profile("compat/missing");
      if(missing.in())
      {
        std::cerr << "unexpected profile for missing key" << std::endl;
        topling_map.deactivate_object();
        topling_map.wait_object();
        return 1;
      }

      Generics::ConstSmartMemBuf_var profile =
        make_profile("topling-written-value");
      topling_map.save_profile(
        "compat/topling-write",
        profile.in(),
        Generics::Time::get_time_of_day());

      topling_map.deactivate_object();
      topling_map.wait_object();
    }

    {
      ToplingMap topling_map(
        String::SubString(*opt_path),
        Generics::Time::ZERO,
        1,
        16);
      topling_map.activate_object();

      Generics::ConstSmartMemBuf_var profile =
        topling_map.get_profile("compat/topling-write");
      const std::string actual = read_profile(profile.in());
      if(actual != "topling-written-value")
      {
        std::cerr << "mismatch for Topling-written key: expected " <<
          "topling-written-value, got " << actual.size() << " bytes" <<
          std::endl;
        topling_map.deactivate_object();
        topling_map.wait_object();
        return 1;
      }

      topling_map.deactivate_object();
      topling_map.wait_object();
    }

    if(*opt_keep_db == 0)
    {
      remove_all(*opt_path);
    }

    std::cout << "RocksDB -> ToplingDB compatibility: OK, records=" <<
      records.size() << ", path=" << *opt_path << std::endl;
    return 0;
  }
  catch(const std::exception& ex)
  {
    std::cerr << "Caught exception: " << ex.what() << std::endl;
  }

  return 1;
}
