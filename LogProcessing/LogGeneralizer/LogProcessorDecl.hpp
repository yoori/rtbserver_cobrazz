#ifndef AD_SERVER_LOG_PROCESSING_LOG_PROCESSOR_DECL_HPP
#define AD_SERVER_LOG_PROCESSING_LOG_PROCESSOR_DECL_HPP


#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Logger/Logger.hpp>
#include <Sync/SyncPolicy.hpp>
#include <LogCommons/FileReceiver.hpp>

namespace AdServer {
namespace LogProcessing {

class LogProcessor: public ReferenceCounting::AtomicImpl
{
public:
  virtual
  void
  check_and_load() = 0;

  virtual
  const FileReceiver_var&
  get_file_receiver() = 0;

protected:
  virtual ~LogProcessor() noexcept {}
};

typedef ReferenceCounting::SmartPtr<LogProcessor> LogProcessor_var;

class LogProcessorImplBase: public LogProcessor
{
public:
  LogProcessorImplBase(
    const std::string &in_dir,
    Logging::Logger *logger
  )
  :
    in_dir_(in_dir),
    logger_(ReferenceCounting::add_ref(logger))
  {
  }

protected:
  virtual ~LogProcessorImplBase() noexcept {}

  const std::string in_dir_;
  Logging::Logger_var logger_;
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_LOG_PROCESSOR_DECL_HPP */
