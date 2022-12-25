// STD
#include <functional>
#include <list>
#include <memory>
#include <random>

// THIS
#include <LogCommons/BidCostStat.hpp>
#include "LogHelper.hpp"
#include "Utils.hpp"

// POSIX
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace PredictorSvcs
{
namespace BidCostPredictor
{
namespace Utils
{

namespace
{
class GeneratorNumber
{
public:
  static GeneratorNumber& Inctance()
  {
    static GeneratorNumber generator;
    return generator;
  }

  std::size_t generate()
  {
    return distribution_(rng_);
  }

private:
  GeneratorNumber()
                  : distribution_(0, 99999999)
  {
    std::array<int, std::mt19937::state_size> seed_data;
    std::generate(seed_data.begin(), seed_data.end(), std::ref(device_));
    std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
    rng_.seed(seq);
  }

private:
  std::random_device device_;

  std::mt19937 rng_;

  std::uniform_int_distribution<std::size_t> distribution_;
};
} // namespace

bool ExistDirectory(
        const std::string& path_directory) noexcept
{
  struct stat sb;
  return stat(path_directory.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode);
}

Files GetDirectoryFiles(
        const Path& path_dir,
        const std::string& prefix)
{
  Files files;

  auto deleter = [] (DIR* dir) {
    if (dir)
      closedir(dir);
  };

  std::unique_ptr<DIR, decltype(deleter)> dir(opendir(path_dir.c_str()),
                                              deleter);

  if (!dir)
  {
    Stream::Error stream;
    stream << __PRETTY_FUNCTION__ << ": Can't open directory" << path_dir;
    throw Exception(stream);
  }

  struct dirent* ent = nullptr;
  while ((ent = readdir(dir.get())))
  {
    const Path result_path = path_dir + '/' + ent->d_name;
    struct stat st;
    if (ent->d_name[0] != '.' && stat(result_path.c_str(), &st) == 0)
    {
      if (!S_ISREG(st.st_mode))
        continue;

      const std::string_view file_name(ent->d_name);
      if (file_name.size() < prefix.size())
        continue;

      if (file_name.substr(0, prefix.size()) == prefix)
        files.emplace_back(result_path);
    }
  }

  files.sort();
  return files;
}

GeneratedPath GenerateFilePath(
        const Path& output_dir,
        const std::string& prefix,
        const LogProcessing::DayTimestamp& date)
{
  std::stringstream stream;
  stream << prefix
         << "."
         << date
         << "."
         << GeneratorNumber::Inctance().generate();

  const Path temp_file_path = output_dir + "/~" + stream.str();
  const Path result_file_path = output_dir + "/" + stream.str();

  return {temp_file_path, result_file_path};
}

} // namespace Utils
} // namespace BidCostPredictor
} // namespace PredictorSvcs
