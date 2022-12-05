#include "ProcessingContexts.hpp"
#include "LogProcessorDecl.hpp"

namespace AdServer
{
namespace LogProcessing
{
  time_t Configuration::db_dump_timeout;

  ProcessingContexts::Contexts ProcessingContexts::contexts_;
  LogProcThreadInfo_var ProcessingContexts::null_;
}
}
