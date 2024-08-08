#ifndef BIDCOSTPREDICTOR_UTILS_HPP
#define BIDCOSTPREDICTOR_UTILS_HPP

// STD
#include <cstdio>
#include <functional>
#include <list>
#include <memory>
#include <random>
#include <string>

// UNIXCOMMONS
#include <eh/Exception.hpp>

// THIS
#include <LogCommons/LogCommons.hpp>

template<class T>
concept ConceptMemberPtr = std::is_member_pointer_v<T>;

namespace AdServer::LogProcessing
{

inline bool operator>(const DayTimestamp& date1, const DayTimestamp& date2)
{
  return date1.time() > date2.time();
}

} // namespace AdServer::LogProcessing

namespace PredictorSvcs::BidCostPredictor::Utils
{

namespace LogProcessing = AdServer::LogProcessing;

DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

using Path = std::string;
using Files = std::list<Path>;
using TempFilePath = Path;
using ResultFilePath = Path;
using GeneratedPath = std::pair<TempFilePath, ResultFilePath>;
using VirtualMemory = double;
using RamMemory = double;
using Memory = std::pair<VirtualMemory, RamMemory>;

enum class DirInfo
{
  RegularFile,
  Directory
};

class CallOnDestroy final
{
public:
  using Callback = std::function<void()>;

public:
  CallOnDestroy(const Callback& callback)
    : callback_(callback)
  {
  }

  ~CallOnDestroy()
  {
    try
    {
      callback_();
    }
    catch (...)
    {
    }
  }

private:
  Callback callback_;
};

Files get_directory_files(
  const Path& path_dir,
  const std::string& prefix = std::string(),
  const DirInfo dir_info = DirInfo::RegularFile);

GeneratedPath generate_file_path(
  const Path& output_dir,
  const std::string& prefix,
  const LogProcessing::DayTimestamp& date);

Memory memory_process_usage();

} // namespace PredictorSvcs::BidCostPredictor::Utils

#endif //BIDCOSTPREDICTOR_UTILS_HPP
