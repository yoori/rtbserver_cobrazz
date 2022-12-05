#include "RequestInfoManagerStats.hpp"

namespace AdServer {
namespace RequestInfoSvcs {

  const Generics::StringHashAdapter StatNames::LOAD_ABS_TIME(
    "loadAbsTime");
  const Generics::StringHashAdapter StatNames::PROCESS_ABS_TIME(
    "processAbsTime");
  const Generics::StringHashAdapter StatNames::PROCESS_COUNT(
    "processCount");
  const Generics::StringHashAdapter StatNames::FILE_COUNT(
    "fileCount");
  const Generics::StringHashAdapter StatNames::LAST_PROCESSED_TIMESTAMP(
    "lastProcessedFileTimestamp");
  const char* StatNames::LOG_CORBA_NAMES[StatNames::LOG_COUNT] =
  {
    "advertiserAction.",
    "click.",
    "impression.",
    "passbackImpression.",
    "request.",
    "tagRequest.",
  };
  
  const LogStatValues::Key StatNames::WEB_INDEX_REQUESTS("webIndexRequestCounter");

} // namespace RequestInfoSvcs
} // namespace AdServer
