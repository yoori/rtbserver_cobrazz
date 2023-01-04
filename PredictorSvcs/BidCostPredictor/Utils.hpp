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

inline bool operator>(const DayTimestamp& date1, const DayTimestamp& date2)
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

bool ExistDirectory(
        const std::string& path_directory) noexcept;

Files GetDirectoryFiles(
        const Path& path_dir,
        const std::string& prefix = std::string(),
        const DirInfo dir_info = DirInfo::RegularFile);

GeneratedPath GenerateFilePath(
        const Path& output_dir,
        const std::string& prefix,
        const LogProcessing::DayTimestamp& date);

} // namespace Utils
} // namespace BidCostPredictor
} // namespace PredictorSvcs

#endif //BIDCOSTPREDICTOR_UTILS_HPP
