#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include <unistd.h>

#include <LogCommons/LogHolder.hpp>

namespace
{
  DECLARE_EXCEPTION(SaveError, eh::DescriptiveException);

  struct SavePolicy
  {
    bool fail_primary = false;
    std::vector<std::string> paths;

    template<typename Collector>
    void
    save(Collector&, const char* path)
    {
      paths.emplace_back(path);
      if (fail_primary && paths.back().ends_with(".ram"))
      {
        throw SaveError("primary save failed");
      }
    }
  };

  bool
  expect(bool condition, const char* message)
  {
    if (!condition)
    {
      std::cerr << "PrimaryDumpTest: " << message << std::endl;
    }
    return condition;
  }
}

int
main()
{
  char root_template[] = "/tmp/PrimaryDumpTest.XXXXXX";
  const char* const root = ::mkdtemp(root_template);
  if (!expect(root != nullptr, "mkdtemp failed"))
  {
    return 1;
  }

  int result = 0;
  int collector = 0;
  AdServer::LogProcessing::LogFlushTraits flush_traits;
  flush_traits.out_dir = std::string(root) + "/Request";
  flush_traits.primary_dump =
    std::make_shared<AdServer::LogProcessing::PrimaryDump>(
      root,
      0,
      ".ram",
      Generics::Time(10));

  SavePolicy failing_policy;
  failing_policy.fail_primary = true;
  AdServer::LogProcessing::save_log(
    flush_traits,
    failing_policy,
    collector);
  AdServer::LogProcessing::save_log(
    flush_traits,
    failing_policy,
    collector);

  result |= !expect(
    failing_policy.paths == std::vector<std::string>{
      flush_traits.out_dir + ".ram",
      flush_traits.out_dir,
      flush_traits.out_dir},
    "primary failure did not activate secondary fallback");

  AdServer::LogProcessing::LogFlushTraits successful_traits;
  successful_traits.out_dir = std::string(root) + "/WebStat";
  successful_traits.primary_dump =
    std::make_shared<AdServer::LogProcessing::PrimaryDump>(root, 0, ".ram");

  SavePolicy successful_policy;
  AdServer::LogProcessing::save_log(
    successful_traits,
    successful_policy,
    collector);
  result |= !expect(
    successful_policy.paths == std::vector<std::string>{
      successful_traits.out_dir + ".ram"},
    "available primary was not selected");

  AdServer::LogProcessing::PrimaryDump full_primary(
    root,
    std::numeric_limits<std::uint64_t>::max(),
    ".ram");
  result |= !expect(
    !full_primary.available(),
    "min_free_space limit was ignored");

  result |= !expect(::rmdir(root) == 0, "temporary directory cleanup failed");
  return result;
}
