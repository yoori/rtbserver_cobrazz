#ifndef AD_SERVER_LOG_PROCESSING_CUSTOM_LOG_PROC_IMPL_HPP
#define AD_SERVER_LOG_PROCESSING_CUSTOM_LOG_PROC_IMPL_HPP


#include <iosfwd>
#include <string>
#include <fstream>

#include <Logger/Logger.hpp>
#include <LogCommons/ArchiveIfstream.hpp>
#include <LogCommons/LogCommons.hpp>
#include "LogVersionManager.hpp"
#include "ProcStatImpl.hpp"
#include "DbConnectionFactory.hpp"
#include "LogGeneralizerStatDef.hpp"

namespace AdServer {
namespace LogProcessing {

template <typename LogExtTraits,
  typename LogVersionManagerT = LogVersionManager2<LogExtTraits> >
class CustomLogProcessorImpl : public LogProcessorImplBase
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef LogExtTraits Traits;

  CustomLogProcessorImpl(
    const LogProcThreadInfo_var& context,
    const std::string &in_dir,
    const std::string &out_dir,
    const std::optional<ArchiveParams>& archive_params,
    const PostgresConnectionFactoryImpl_var &pg_conn_factory,
    const CollectorBundleParams &bundle_params,
    const ProcStatImpl_var& proc_stat_impl,
    const FileReceiverInterrupter_var& fr_interrupter,
    const LogGeneralizerStatMapBundle_var& lgsm_bundle
  )
  :
    LogProcessorImplBase(in_dir, context->logger),
    proc_stat_(proc_stat_impl),
    lgsm_bundle_(lgsm_bundle),
    log_version_manager_(new LogVersionManagerT(
      context, out_dir, archive_params, lgsm_bundle, pg_conn_factory, bundle_params))
  {
    create_file_receiver_(fr_interrupter);
  }

  virtual
  void
  check_and_load();

  virtual
  const FileReceiver_var&
  get_file_receiver()
  {
    return file_receiver_;
  }

protected:
  virtual
  ~CustomLogProcessorImpl() noexcept {}

private:
  typedef std::unique_ptr<LogVersionManagerT> LogVersionManagerPtrT;
  typedef ProcStatsValues<LogExtTraits> ProcStatT;

  static const char* error_dir_()
  {
    return DEFAULT_ERROR_DIR;
  }

  void
  create_file_receiver_(const FileReceiverInterrupter_var& fr_interrupter)
  {
    fr_interrupter_ = fr_interrupter;

    std::string fr_intermed_dir = in_dir_;

    (fr_intermed_dir += '/') += FR_INTERMED_DIR;
    file_receiver_ = new FileReceiver(fr_intermed_dir.c_str(),
      FR_MAX_FILES_TO_STORE, fr_interrupter, 0);
  }

  ProcStatT proc_stat_;
  const LogGeneralizerStatMapBundle_var lgsm_bundle_;
  LogVersionManagerPtrT log_version_manager_;
  FileReceiverInterrupter_var fr_interrupter_;
  FileReceiver_var file_receiver_;
};


template <typename LogExtTraits, typename LogVersionManagerT>
void
CustomLogProcessorImpl<LogExtTraits, LogVersionManagerT>::check_and_load()
{
  Generics::Time loaded_timestamp =
    log_version_manager_->get_min_file_timestamp();

  if (file_receiver_->empty())
  {
    LogExtTraits::search_files(in_dir_, file_receiver_);
  }

  LogFileNameInfo file_info;
  FileReceiver::FileGuard_var file;
  std::string tmp;
  while (fr_interrupter_->active() && (file = file_receiver_->get_eldest(tmp)))
  {
    if (logger_->log_level() >= Logging::Logger::TRACE)
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": processing file '"
         << file->full_path() << '\'';
      logger_->trace(es.str());
    }

    parse_log_file_name(file->full_path(), file_info);

    bool stop_loading_files = false;
    // if have loaded files
    if (!log_version_manager_->file_list_is_empty())
    {
      LogGeneralizerStatMapBundle::ReadGuardT lgsm_guard(lgsm_bundle_->lock);

      LogGeneralizerStatMap &lgs_map = lgsm_bundle_->map;
      for (LogGeneralizerStatMap::iterator lgsm_it = lgs_map.begin();
        lgsm_it != lgs_map.end(); ++lgsm_it)
      {
        Generics::Time ts;
        {
          LogGeneralizerStatValue::GuardT lgsv_guard(lgsm_it->second->lock);
          ts = lgsm_it->second->start_clear_timestamp;
          if (logger_->log_level() >= Logging::Logger::TRACE)
          {
            Stream::Error es;
            es << __PRETTY_FUNCTION__ << ": Performing timestamp check for "
               << "file '" << file->full_path() << '\''
               << " [ file timestamp = " << file_info.timestamp
               << " , start_clear_timestamp = " << ts
               << " , loaded_timestamp = " << loaded_timestamp << " ]";
            logger_->trace(es.str());
          }
          // Check under mutex, to prevent modification ts by others
          if ((ts > file_info.timestamp && ts < loaded_timestamp) ||
            (ts < file_info.timestamp && ts > loaded_timestamp))
          {
            stop_loading_files = true;

            if (logger_->log_level() >= Logging::Logger::TRACE)
            {
              Stream::Error es;
              es << __PRETTY_FUNCTION__ << ": Stopped loading at file '"
                 << file->full_path() << '\'' << " [ file timestamp = "
                 << file_info.timestamp << " , start_clear_timestamp = " << ts
                 << " , loaded_timestamp = " << loaded_timestamp << " ]";
              logger_->trace(es.str());
            }
            break;
          }
        }
      }
    }
    if (stop_loading_files)
    {
      file->revert();
      break;
    }

    const auto& file_path = file->full_path();
    std::unique_ptr<std::istream> istream;
    if (LogProcessing::ArchiveIfstream::is_archive(file_path))
    {
      istream = std::make_unique<ArchiveIfstream>(file_path);
    }
    else
    {
      istream = std::make_unique<std::ifstream>(file_path);
    }
    auto& ref_istream = *istream;

    if (!ref_istream)
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Failed to open file '"
         << file_path << "'. Skipping ...";
      logger_->error(es.str(), 0, LOG_GEN_IMPL_ERR_CODE_8);
      continue;
    }
    try
    {
      log_version_manager_->load(ref_istream, file);
      loaded_timestamp = file_info.timestamp;
    }
    catch (const eh::Exception &ex)
    {
      logger_->sstream(
        Logging::Logger::ERROR,
        0/*ASPECT*/,
        LOG_GEN_IMPL_ERR_CODE_5) << FNT << ": Exception: " << ex.what();

      if (logger_->log_level() >= Logging::Logger::TRACE)
      {
        logger_->sstream(Logging::Logger::TRACE)
          << FNT << ": Trying to save file '"
          << file->full_path() << "' in " << error_dir_() << "/";
      }
      try
      {
        FileStore(in_dir_, error_dir_()).store(file->full_path());
      }
      catch (const eh::Exception &ex)
      {
        logger_->sstream(
          Logging::Logger::ERROR,
          0/*ASPECT*/,
          LOG_GEN_IMPL_ERR_CODE_6) << FNT << ": Exception: " << ex.what();
      }
    }
  }
}

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_CUSTOM_LOG_PROC_IMPL_HPP */
