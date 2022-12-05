#ifndef AD_SERVER_LOG_PROCESSING_ERROR_CODE_HPP
#define AD_SERVER_LOG_PROCESSING_ERROR_CODE_HPP


#include <LogCommons/LogCommons.hpp>

namespace AdServer {
namespace LogProcessing {

  struct SaveEx
  {
    DECLARE_EXCEPTION(PgException, LogSaver::Exception);
    DECLARE_EXCEPTION(CsvException, LogSaver::Exception);
  };

extern const char *LOG_GEN_DB_ERR_CODE_0;
extern const char *LOG_GEN_DB_ERR_CODE_1;
extern const char *LOG_GEN_DB_ERR_CODE_2;
extern const char *LOG_GEN_IMPL_ERR_CODE_0;
extern const char *LOG_GEN_IMPL_ERR_CODE_1;
extern const char *LOG_GEN_IMPL_ERR_CODE_2;
extern const char *LOG_GEN_IMPL_ERR_CODE_3;
extern const char *LOG_GEN_IMPL_ERR_CODE_4;
extern const char *LOG_GEN_IMPL_ERR_CODE_5;
extern const char *LOG_GEN_IMPL_ERR_CODE_6;
extern const char *LOG_GEN_IMPL_ERR_CODE_7;
extern const char *LOG_GEN_IMPL_ERR_CODE_8;
extern const char *LOG_GEN_IMPL_ERR_CODE_9;
extern const char *LOG_GEN_IMPL_ERR_CODE_10;
extern const char *LOG_GEN_IMPL_ERR_CODE_11;

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_ERROR_CODE_HPP */

