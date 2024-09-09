#ifndef AD_SERVER_LOG_PROCESSING_GENERIC_LOG_CSV_SAVER_IMPL_HPP
#define AD_SERVER_LOG_PROCESSING_GENERIC_LOG_CSV_SAVER_IMPL_HPP


#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <utility>

#include <LogCommons/LogSaverBaseImpl.hpp>

namespace AdServer {
namespace LogProcessing {

template <
  typename LogExtTraits,
  bool is_nested = LogExtTraits::is_nested,
  bool is_summable = LogExtTraits::is_summable
>
struct CsvWrite;

template <typename LogExtTraits>
struct CsvWrite<LogExtTraits, false, false>
{
  static
  void
  impl(
    std::ostream& ostream,
    typename LogExtTraits::CollectorType::const_iterator it
  )
    /*throw(eh::Exception)*/
  {
    LogExtTraits::write_data_as_csv(ostream, *it) << '\n';
  }
};

template <typename LogExtTraits>
struct CsvWrite<LogExtTraits, false, true>
{
  static
  void
  impl(
    std::ostream& ostream,
    typename LogExtTraits::CollectorType::const_iterator it
  )
    /*throw(eh::Exception)*/
  {
    LogExtTraits::write_as_csv(ostream, it->first, it->second) << '\n';
  }
};

template <typename LogExtTraits>
struct CsvWrite<LogExtTraits, true, true>
{
  static
  void
  impl(
    std::ostream& ostream,
    typename LogExtTraits::CollectorType::const_iterator it
  )
    /*throw(eh::Exception)*/
  {
    for (auto iit = it->second.begin(); iit != it->second.end(); ++iit)
    {
      LogExtTraits::write_as_csv(ostream, it->first, iit->first, iit->second) << '\n';
    }
  }
};

template <class LogExtTraits>
class SimpleLogCsvSaverImpl: public SimpleLogSaver
{
public:
  DECLARE_EXCEPTION(CsvException, Exception);

  typedef typename LogExtTraits::CollectorType CollectorT;

  SimpleLogCsvSaverImpl(
    CollectorT& collector,
    const std::string& path,
    const std::optional<ArchiveParams>& archive_params
  )
  :
    collector_(collector),
    path_(path),
    archive_params_(archive_params)
  {
  }

  void save() /*throw(CsvException)*/;

private:
  SimpleLogCsvSaverImpl(const SimpleLogCsvSaverImpl&);
  SimpleLogCsvSaverImpl& operator=(const SimpleLogCsvSaverImpl&);

  virtual
  ~SimpleLogCsvSaverImpl() noexcept {}

  CollectorT& collector_;
  const std::string path_;
  const std::optional<ArchiveParams> archive_params_;
};

template <class LogExtTraits>
void
SimpleLogCsvSaverImpl<LogExtTraits>::save()
  /*throw(CsvException)*/
{
  if (collector_.empty())
  {
    return;
  }
  StringPair filenames;
  try
  {
    LogFileNameInfo name_info(LogExtTraits::csv_base_name());
    name_info.format = LogFileNameInfo::LFNF_CSV;
    filenames = make_log_file_name_pair(name_info, path_);

    {
      std::unique_ptr<std::ostream> ostream;
      if (archive_params_)
      {
        ostream = std::make_unique<ArchiveOfstream>(
          filenames.second,
          *archive_params_);
      }
      else
      {
        ostream = std::make_unique<std::ofstream>(
          filenames.second.c_str());
      }
      auto& ref_ostream = *ostream;

      if (!ref_ostream)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ << ": Error: "
           << "Failed to open file '" << filenames.second << '\'';
        throw CsvException(es);
      }
      ref_ostream << LogExtTraits::csv_header() << '\n';
      if (!ref_ostream)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ <<
           ": Error: Failed to write log header to file '" <<
           filenames.second << '\'';
        throw CsvException(es);
      }
      for (auto it = collector_.begin(); it != collector_.end(); ++it)
      {
        CsvWrite<LogExtTraits>::impl(ref_ostream, it);
      }
      ref_ostream.flush();
      if (!ref_ostream)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ <<
           ": Error: Failed to write log data to file '" <<
           filenames.second << '\'';
        throw CsvException(es);
      }
    }

    if (std::rename(filenames.second.c_str(), filenames.first.c_str()))
    {
      eh::throw_errno_exception<CsvException>(__PRETTY_FUNCTION__,
        ": failed to rename file '", filenames.second,
        "' to '", filenames.first, "'");
    }
  }
  catch (const CsvException&)
  {
    unlink(filenames.second.c_str());
    throw;
  }
  catch (const eh::Exception& ex)
  {
    unlink(filenames.second.c_str());
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. : " << ex.what();
    throw CsvException(es);
  }
  catch (...)
  {
    unlink(filenames.second.c_str());
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught unknown exception.";
    throw CsvException(es);
  }
}

template <typename LogExtTraits>
class GenericLogCsvSaverImpl:
  virtual public LogSaverBaseImpl<typename LogExtTraits::CollectorBundleType>
{
  typedef LogSaverBaseImpl<typename LogExtTraits::CollectorBundleType>
    BaseType;

public:
  typedef typename LogExtTraits::CollectorBundleType CollectorBundleType;
  typedef typename BaseType::Spillover_var Spillover_var;
  typedef typename LogExtTraits::CollectorFilterType CollectorFilterT;

  typedef typename BaseType::Exception Exception;
  DECLARE_EXCEPTION(CsvException, Exception);

  GenericLogCsvSaverImpl(
    const std::string& path,
    const std::optional<ArchiveParams>& archive_params,
    CollectorFilterT* collector_filter
  )
    /*throw(Exception)*/
  :
    BaseType(collector_filter),
    path_(path),
    archive_params_(archive_params)
  {
  }

  void
  save(const Spillover_var& data);

protected:
  typedef typename BaseType::CollectorT CollectorT;

  virtual
  ~GenericLogCsvSaverImpl() noexcept {}

  void
  save_i_(const CollectorT& collector) /*throw(CsvException)*/;

private:
  const std::string path_;
  const std::optional<ArchiveParams> archive_params_;
};

template <class LogExtTraits>
void
GenericLogCsvSaverImpl<LogExtTraits>::save(const Spillover_var& data)
{
  BaseType::collector_filter_->filter(data->collector);
  if (!data->collector.empty())
  {
    save_i_(data->collector);
  }
}

template <class LogExtTraits>
inline
void
GenericLogCsvSaverImpl<LogExtTraits>::save_i_(const CollectorT& collector)
  /*throw(CsvException)*/
{
  StringPair filenames;
  try
  {
    LogFileNameInfo name_info(LogExtTraits::csv_base_name());
    name_info.format = LogFileNameInfo::LFNF_CSV;
    filenames = make_log_file_name_pair(name_info, path_);

    {
      std::unique_ptr<std::ostream> ostream;
      if (archive_params_)
      {
        ostream = std::make_unique<ArchiveOfstream>(
          filenames.second,
          *archive_params_);
      }
      else
      {
        ostream = std::make_unique<std::ofstream>(
          filenames.second.c_str());
      }
      auto& ref_ostream = *ostream;

      if (!ref_ostream)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ << ": Error: "
           << "Failed to open file '" << filenames.second << '\'';
        throw CsvException(es);
      }
      ref_ostream << LogExtTraits::csv_header() << '\n';
      if (!ref_ostream)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ <<
           ": Error: Failed to write log header to file '" <<
           filenames.second << '\'';
        throw CsvException(es);
      }
      for (auto it = collector.begin(); it != collector.end(); ++it)
      {
        CsvWrite<LogExtTraits>::impl(ref_ostream, it);
      }
      ref_ostream.flush();
      if (!ref_ostream)
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__ <<
           ": Error: Failed to write log data to file '" <<
           filenames.second << '\'';
        throw CsvException(es);
      }
    }

    if (std::rename(filenames.second.c_str(), filenames.first.c_str()))
    {
      eh::throw_errno_exception<CsvException>(__PRETTY_FUNCTION__,
        ": failed to rename file '", filenames.second,
        "' to '", filenames.first, "'");
    }
  }
  catch (const CsvException&)
  {
    unlink(filenames.second.c_str());
    throw;
  }
  catch (const eh::Exception& ex)
  {
    unlink(filenames.second.c_str());
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. : " << ex.what();
    throw CsvException(es);
  }
  catch (...)
  {
    unlink(filenames.second.c_str());
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Caught unknown exception.";
    throw CsvException(es);
  }
}

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_GENERIC_LOG_CSV_SAVER_IMPL_HPP */

