#pragma once

#include <list>
#include <LogCommons/FileReceiver.hpp>

namespace AdServer::LogProcessing
{

  typedef FileReceiver::FileGuard_var CollectorBundleFileGuard_var;
  typedef std::list<CollectorBundleFileGuard_var> CollectorBundleFileList;

  struct CollectorBundleParams
  {
    const std::size_t MAX_SIZE;
  };

} // namespace AdServer::LogProcessing
