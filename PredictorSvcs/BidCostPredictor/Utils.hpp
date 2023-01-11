#ifndef BIDCOSTPREDICTOR_UTILS_HPP
#define BIDCOSTPREDICTOR_UTILS_HPP

// STD
#include <cstdio>
#include <list>
#include <memory>
#include <random>
#include <string>

// THIS
#include <eh/Exception.hpp>
#include <LogCommons/LogCommons.hpp>

namespace AdServer
{
namespace LogProcessing
{

inline bool operator>(
  const DayTimestamp& date1,
  const DayTimestamp& date2)
{
  return date1.time() > date2.time();
}

} // namespace LogProcessing
} // namespace AdServer

namespace PredictorSvcs
{
namespace BidCostPredictor
{
namespace Utils
{

namespace LogProcessing = AdServer::LogProcessing;

DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

using Path = std::string;
using Files = std::list<Path>;
using GeneratedPath = std::pair<Path, Path>;

enum class DirInfo
{
  RegularFile,
  Directory
};

bool exist_directory(
  const std::string& path_directory) noexcept;

Files get_directory_files(
  const Path& path_dir,
  const std::string& prefix = std::string(),
  const DirInfo dir_info = DirInfo::RegularFile);

GeneratedPath generate_file_path(
  const Path& output_dir,
  const std::string& prefix,
  const LogProcessing::DayTimestamp& date);

std::pair<double, double> memory_process_usage();

} // namespace Utils
} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_UTILS_HPP
