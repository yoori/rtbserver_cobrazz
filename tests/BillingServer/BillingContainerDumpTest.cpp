#include <array>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>

#include <sys/resource.h>

#include <CampaignSvcs/BillingServer/BillingContainer.hpp>

namespace
{
  namespace fs = std::filesystem;
  using AdServer::CampaignSvcs::BillingContainer;
  using AdServer::CampaignSvcs::BillingContainer_var;
  using Snapshot = std::map<std::string, std::string>;

  struct Fixture
  {
    const char* name;
    const char* contents;
  };

  const std::array<Fixture, 5> FIXTURES{{
    {"Accounts", "1\t2026-10-08\t1\n1\t1970-01-01\t0"},
    {"Campaigns", "1\t2026-10-08\t1\t1\t0\n1\t1970-01-01\t0\t0\t0"},
    {"Ccgs", "1\t2026-10-08\t1\t1\t0\n1\t1970-01-01\t0\t0\t0"},
    {"CcgRateAmounts", "1\t1\t2026-10-08\t0:0.1:G:1:0:1"},
    {"CampaignRateOpts", "1\t2026-10-08\t0.1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0"}
  }};

  void require(bool condition, const std::string& message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  class TemporaryDirectory
  {
  public:
    TemporaryDirectory()
    {
      char path[] = "/tmp/BillingContainerDumpTest-XXXXXX";
      require(::mkdtemp(path) != nullptr, "mkdtemp failed");
      path_ = path;
    }

    ~TemporaryDirectory()
    {
      std::error_code error;
      fs::remove_all(path_, error);
    }

    fs::path path_;
  };

  class WriteFailure
  {
  public:
    WriteFailure()
    {
      require(::getrlimit(RLIMIT_FSIZE, &previous_limit_) == 0, "getrlimit failed");
      previous_handler_ = std::signal(SIGXFSZ, SIG_IGN);
      require(previous_handler_ != SIG_ERR, "signal failed");
      auto limit = previous_limit_;
      limit.rlim_cur = 1;
      if (::setrlimit(RLIMIT_FSIZE, &limit) != 0)
      {
        std::signal(SIGXFSZ, previous_handler_);
        throw std::runtime_error("setrlimit failed");
      }
    }

    ~WriteFailure()
    {
      ::setrlimit(RLIMIT_FSIZE, &previous_limit_);
      std::signal(SIGXFSZ, previous_handler_);
    }

  private:
    struct rlimit previous_limit_{};
    using SignalHandler = void (*)(int);
    SignalHandler previous_handler_ = SIG_DFL;
  };

  Snapshot read_snapshot(const fs::path& root)
  {
    Snapshot result;
    for (const auto& fixture : FIXTURES)
    {
      std::ifstream file(root / fixture.name, std::ios::binary);
      require(file.is_open(), std::string("missing snapshot file: ") + fixture.name);
      result.emplace(fixture.name,
        std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
      require(!file.bad(), std::string("can't read snapshot file: ") + fixture.name);
    }

    return result;
  }

  void test_write_failure(const fs::path& root, const Fixture& failed_file)
  {
    fs::create_directory(root);
    for (const auto& fixture : FIXTURES)
    {
      std::ofstream file(root / fixture.name);
      if (&fixture == &failed_file)
      {
        file << fixture.contents;
      }

      file.close();
      require(static_cast<bool>(file), "can't write fixture");
    }

    BillingContainer_var container = new BillingContainer(
      nullptr, root.string(), Generics::Time::ONE_DAY, 1);
    container->dump();
    const auto snapshot = read_snapshot(root);
    require(snapshot.at(failed_file.name).size() > 1, "fixture must exceed the write limit");

    std::string error;
    {
      WriteFailure failure;
      try
      {
        container->dump();
      }
      catch (const BillingContainer::Exception& ex)
      {
        error = ex.what();
      }
    }

    require(!error.empty(), "dump accepted an incomplete snapshot");
    const fs::path failed_path = fs::path(root.string() + "~") / failed_file.name;
    require(error.find(failed_path.string()) != std::string::npos, "error omits the failed path");
    require(fs::file_size(failed_path) == 1, "write failure did not truncate the expected file");
    require(read_snapshot(root) == snapshot, "failed dump replaced the previous snapshot");
    require(!fs::exists(root.string() + ".bak"), "failed dump moved the previous snapshot");

    container->dump();
    require(read_snapshot(root) == snapshot, "retry changed the snapshot contents");
    require(!fs::exists(root.string() + "~"), "successful retry left a temporary snapshot");

    BillingContainer_var reloaded = new BillingContainer(
      nullptr, root.string(), Generics::Time::ONE_DAY, 1);
    reloaded->dump();
    require(read_snapshot(root) == snapshot, "snapshot did not survive reload");
  }
}

int main()
{
  try
  {
    TemporaryDirectory directory;
    bool passed = true;
    for (const auto& fixture : FIXTURES)
    {
      try
      {
        test_write_failure(directory.path_ / fixture.name, fixture);
        std::cout << fixture.name << ": passed\n";
      }
      catch (const std::exception& ex)
      {
        passed = false;
        std::cerr << fixture.name << ": " << ex.what() << '\n';
      }
    }

    return passed ? 0 : 1;
  }
  catch (const std::exception& ex)
  {
    std::cerr << "BillingContainerDumpTest: " << ex.what() << '\n';
    return 1;
  }
}
