#include "SiteReferrerStatCsvSaver.hpp"

#include <cstdio>
#include <fstream>

#include <eh/Errno.hpp>


namespace AdServer::LogProcessing
{
  SiteReferrerStatCsvSaver::SiteReferrerStatCsvSaver(
    const std::string& path,
    CollectorFilterT* collector_filter)
    : BaseType(collector_filter),
      path_(path)
  {}

  void
  SiteReferrerStatCsvSaver::save(const Spillover_var& data)
  {
    BaseType::collector_filter_->filter(data->collector);
    if (!data->collector.empty())
    {
      save_i_(data->collector);
    }
  }

  template <typename CsvTraits>
  StringPair
  SiteReferrerStatCsvSaver::write_temp_(const CollectorT& collector)
  {
    LogFileNameInfo name_info(CsvTraits::csv_base_name());
    name_info.format = LogFileNameInfo::LFNF_CSV;
    StringPair filenames = make_log_file_name_pair(name_info, path_);

    try
    {
      std::ofstream ofs(filenames.second.c_str());
      if (!ofs)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ << ": Failed to open file '"
          << filenames.second << '\'';
        throw CsvException(es);
      }

      ofs << CsvTraits::csv_header() << '\n';
      for (auto it = collector.begin(); it != collector.end(); ++it)
      {
        CsvWrite<CsvTraits>::impl(ofs, it);
      }
      ofs.flush();
      if (!ofs)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ << ": Failed to write file '"
          << filenames.second << '\'';
        throw CsvException(es);
      }
    }
    catch (...)
    {
      unlink(filenames.second.c_str());
      throw;
    }

    return filenames;
  }

  void
  SiteReferrerStatCsvSaver::save_i_(const CollectorT& collector)
  {
    StringPair legacy;
    StringPair version_2;
    try
    {
      legacy = write_temp_<SiteReferrerStatCsvTraits>(collector);
      version_2 = write_temp_<SiteReferrerStat2CsvTraits>(collector);

      if (std::rename(legacy.second.c_str(), legacy.first.c_str()))
      {
        eh::throw_errno_exception<CsvException>(
          __PRETTY_FUNCTION__, ": failed to rename file '", legacy.second,
          "' to '", legacy.first, "'");
      }
      if (std::rename(version_2.second.c_str(), version_2.first.c_str()))
      {
        eh::throw_errno_exception<CsvException>(
          __PRETTY_FUNCTION__, ": failed to rename file '", version_2.second,
          "' to '", version_2.first, "'");
      }
    }
    catch (const eh::Exception& ex)
    {
      unlink(legacy.second.c_str());
      unlink(version_2.second.c_str());
      unlink(legacy.first.c_str());
      unlink(version_2.first.c_str());
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": " << ex.what();
      throw CsvException(es);
    }
    catch (...)
    {
      unlink(legacy.second.c_str());
      unlink(version_2.second.c_str());
      unlink(legacy.first.c_str());
      unlink(version_2.first.c_str());
      throw;
    }
  }
}
