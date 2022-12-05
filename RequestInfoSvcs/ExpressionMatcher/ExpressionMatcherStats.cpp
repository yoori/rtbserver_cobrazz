/// @file ExpressionMatcherStats.cpp
#include "ExpressionMatcherStats.hpp"

namespace AdServer
{
  namespace RequestInfoSvcs
  {
    namespace
    {
      const ExpressionMatcherStatsImpl::Key OPTIN_USER(
        "processedMatches-OptInUser-Count");
      const ExpressionMatcherStatsImpl::Key TEMPORARY_USER(
        "processedMatches-temporaryUser-Count");
      const ExpressionMatcherStatsImpl::Key NOT_OPTIN_USER(
        "processedMatches-nonOptInUser-Count");
      const ExpressionMatcherStatsImpl::Key LAST_PROCESSED_TIMESTAMP(
        "processedRequestBasicChannelsTimestamp");
    }

    void
    ExpressionMatcherStatsImpl::fill_values(const Stats& stats)
      /*throw(eh::Exception)*/
    {
      Sync::PosixGuard guard(mutex_);
      set_<UnsignedInt>(OPTIN_USER, stats.processed_matches_optin_user);
      set_<UnsignedInt>(TEMPORARY_USER, stats.processed_matches_temporary_user);
      set_<UnsignedInt>(NOT_OPTIN_USER, stats.processed_matches_non_optin_user);
      set_(LAST_PROCESSED_TIMESTAMP, stats.last_processed_file_timestamp.as_double());
   }

  }
}
