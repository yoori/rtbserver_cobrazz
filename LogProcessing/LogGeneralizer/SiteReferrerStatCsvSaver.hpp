#pragma once


#include <LogCommons/LogSaverBaseImpl.hpp>

#include "LogTypeCsvTraits.hpp"

namespace AdServer::LogProcessing
{
  class SiteReferrerStatCsvSaver:
    virtual public LogSaverBaseImpl<SiteReferrerStatCsvTraits::CollectorBundleType>
  {
    using BaseType =
      LogSaverBaseImpl<SiteReferrerStatCsvTraits::CollectorBundleType>;

  public:
    using CollectorBundleType = SiteReferrerStatCsvTraits::CollectorBundleType;
    using Spillover_var = typename BaseType::Spillover_var;
    using CollectorFilterT = SiteReferrerStatCsvTraits::CollectorFilterType;
    using Exception = typename BaseType::Exception;
    DECLARE_EXCEPTION(CsvException, Exception);

    SiteReferrerStatCsvSaver(
      const std::string& path,
      CollectorFilterT* collector_filter);

    void save(const Spillover_var& data) override;

  protected:
    ~SiteReferrerStatCsvSaver() noexcept override = default;

  private:
    using CollectorT = typename BaseType::CollectorT;

    template <typename CsvTraits>
    StringPair write_temp_(const CollectorT& collector);

    void save_i_(const CollectorT& collector);

    const std::string path_;
  };
}
