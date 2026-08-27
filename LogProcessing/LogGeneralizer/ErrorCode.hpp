#pragma once


#include <LogCommons/LogCommons.hpp>

namespace AdServer::LogProcessing
{

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

} // namespace AdServer::LogProcessing
