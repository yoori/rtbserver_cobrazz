#ifndef AD_SERVER_LOG_PROCESSING_COLLECTOR_BUNDLE_TYPE_DEFS_HPP
#define AD_SERVER_LOG_PROCESSING_COLLECTOR_BUNDLE_TYPE_DEFS_HPP

#include <list>
#include <LogCommons/FileReceiver.hpp>

namespace AdServer {
namespace LogProcessing {

  typedef FileReceiver::FileGuard_var CollectorBundleFileGuard_var;
  typedef std::list<CollectorBundleFileGuard_var> CollectorBundleFileList;

  struct CollectorBundleParams
  {
    const std::size_t MAX_SIZE;
  };

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_COLLECTOR_BUNDLE_TYPE_DEFS_HPP */

