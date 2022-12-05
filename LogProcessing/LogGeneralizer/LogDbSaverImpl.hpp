#ifndef AD_SERVER_LOG_PROCESSING_LOG_DB_SAVER_IMPL_HPP
#define AD_SERVER_LOG_PROCESSING_LOG_DB_SAVER_IMPL_HPP


#include <Logger/Logger.hpp>
#include <Commons/Postgres/Environment.hpp>
#include <Commons/Postgres/Connection.hpp>

#include "LogTypeExtTraits.hpp"
#include "LogTypeCsvTraits.hpp"
#include "DbSaverDeclMacro.hpp"
#include "DbConnectionFactory.hpp"
#include "LogGeneralizerStatDef.hpp"

namespace AdServer {
namespace LogProcessing {

DECLARE_NESTED_LOG_DB_SAVER_EXT_2(CreativeStat, CustomCreativeStat,
  LogGeneralizerStatMapBundle_var);

DECLARE_NESTED_LOG_DB_SAVER_EXT_2_USING_PREFIX(
  CreativeStatTraits,
  LogGeneralizerStatMapBundle_var,
  DeferredCreativeStat
);

DECLARE_NESTED_LOG_PG_CSV_SAVER(CreativeStat, CustomCreativeStat,
  LogGeneralizerStatMapBundle_var);

DECLARE_NESTED_LOG_PG_CSV_SAVER_USING_PREFIX(
  CreativeStatTraits,
  CreativeStatCsvTraits,
  LogGeneralizerStatMapBundle_var,
  DeferredCreativeStat
);

DECLARE_NESTED_LOG_DB_SAVER_EXT_2(CmpStat, CustomCmpStat,
  LogGeneralizerStatMapBundle_var);

DECLARE_NESTED_LOG_DB_SAVER_EXT_2_USING_PREFIX(
  CmpStatTraits,
  LogGeneralizerStatMapBundle_var,
  DeferredCmpStat
);

DECLARE_NESTED_LOG_PG_CSV_SAVER(CmpStat, CustomCmpStat,
  LogGeneralizerStatMapBundle_var);

DECLARE_NESTED_LOG_PG_CSV_SAVER_USING_PREFIX(
  CmpStatTraits,
  CmpStatCsvTraits,
  LogGeneralizerStatMapBundle_var,
  DeferredCmpStat
);

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_LOG_DB_SAVER_IMPL_HPP */

