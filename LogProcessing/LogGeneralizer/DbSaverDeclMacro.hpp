#ifndef AD_SERVER_LOG_PROCESSING_DB_SAVER_DECL_MACRO_HPP
#define AD_SERVER_LOG_PROCESSING_DB_SAVER_DECL_MACRO_HPP


#define DECLARE_NESTED_LOG_DB_SAVER_EXT_2(LOG_TYPE, CUSTOM_TYPE, OUT_TYPE) \
class LOG_TYPE##DbSaver: \
  public LogSaverBaseImpl<CUSTOM_TYPE##ExtTraits::CollectorBundleType> \
{ \
  typedef LogSaverBaseImpl<CUSTOM_TYPE##ExtTraits::CollectorBundleType> \
    BaseType; \
public: \
  typedef CUSTOM_TYPE##ExtTraits Traits; \
  typedef Traits::CollectorBundleType CollectorBundleType; \
  typedef ReferenceCounting::SmartPtr<Traits::DbSaverType> SmartPtr; \
  typedef BaseType::Spillover_var Spillover_var; \
  typedef Traits::CollectorFilterType CollectorFilterT; \
  \
  LOG_TYPE##DbSaver( \
    const PostgresConnectionFactoryImpl_var& pg_conn_factory, \
    const OUT_TYPE& output, \
    CollectorFilterT* collector_filter \
  ) \
  : \
    BaseType(collector_filter), \
    pg_conn_factory_(pg_conn_factory), \
    output_(output) \
  {} \
  \
  void save(const Spillover_var& data) \
  { \
    collector_filter_->filter(data->collector); \
    \
    if (!data->collector.empty()) \
    { \
      save_i_(data->collector, data->save_time); \
    } \
  } \
  \
protected: \
  ~LOG_TYPE##DbSaver() noexcept {} \
  \
private: \
  typedef Traits::CollectorType CollectorT; \
  typedef Traits::CollectorBundleType CollectorBundleT; \
  \
  void \
  save_i_( \
    CollectorT &collector, \
    Generics::Time &outtime \
  ); \
  \
  bool \
  do_save_( \
    Commons::Postgres::Connection_var &connection, \
    const CollectorT::KeyT &outer_key, \
    const CollectorT::DataT &collector \
  ); \
  \
  PostgresConnectionFactoryImpl_var pg_conn_factory_; \
  OUT_TYPE output_; \
}

#define DECLARE_NESTED_LOG_DB_SAVER_EXT_2_USING_PREFIX( \
          LOG_TRAITS, \
          OUT_TYPE, \
          PREFIX \
        ) \
class PREFIX##DbSaver: \
  public LogSaverBaseImpl<PREFIX##ExtTraits::CollectorBundleType> \
{ \
  typedef LogSaverBaseImpl<PREFIX##ExtTraits::CollectorBundleType> BaseType; \
public: \
  typedef PREFIX##ExtTraits Traits; \
  typedef Traits::CollectorBundleType CollectorBundleType; \
  typedef ReferenceCounting::SmartPtr<Traits::DbSaverType> SmartPtr; \
  typedef BaseType::Spillover_var Spillover_var; \
  typedef Traits::CollectorFilterType CollectorFilterT; \
  \
  PREFIX##DbSaver( \
    const PostgresConnectionFactoryImpl_var& pg_conn_factory, \
    const OUT_TYPE& output, \
    CollectorFilterT* collector_filter) \
  : \
    BaseType(collector_filter), \
    pg_conn_factory_(pg_conn_factory), \
    output_(output) \
  {} \
  \
  void save(const Spillover_var& data) \
  { \
    collector_filter_->filter(data->collector); \
    \
    if (!data->collector.empty()) \
    { \
      save_i_(data->collector, \
        data->get_min_file_timestamp(), data->save_time); \
    } \
  } \
  \
protected: \
  ~PREFIX##DbSaver() noexcept {} \
  \
private: \
  typedef Traits::CollectorType CollectorT; \
  typedef Traits::CollectorBundleType CollectorBundleT; \
  \
  void \
  save_i_( \
    CollectorT &collector, \
    const Generics::Time &intime, \
    Generics::Time &outtime \
  ); \
  \
  bool \
  do_save_( \
    Commons::Postgres::Connection_var &connection, \
    const CollectorT::KeyT &outer_key, \
    const CollectorT::DataT &collector \
  ); \
  \
  PostgresConnectionFactoryImpl_var pg_conn_factory_; \
  OUT_TYPE output_; \
}

#define DECLARE_NESTED_LOG_PG_CSV_SAVER(LOG_TYPE, CUSTOM_TYPE, OUT_TYPE) \
class LOG_TYPE##PgCsvSaver: \
  public LogSaverBaseImpl<CUSTOM_TYPE##ExtTraits::CollectorBundleType> \
{ \
  typedef LogSaverBaseImpl<CUSTOM_TYPE##ExtTraits::CollectorBundleType> \
    BaseType; \
public: \
  DECLARE_EXCEPTION(PgException, LogSaver::Exception); \
  DECLARE_EXCEPTION(CsvException, LogSaver::Exception); \
  \
  typedef CUSTOM_TYPE##ExtTraits Traits; \
  typedef Traits::CollectorBundleType CollectorBundleType; \
  typedef ReferenceCounting::SmartPtr<Traits::PgCsvSaverType> SmartPtr; \
  typedef BaseType::Spillover_var Spillover_var; \
  typedef Traits::CollectorFilterType CollectorFilterT; \
  \
  LOG_TYPE##PgCsvSaver( \
    const std::string& path, \
    const std::optional<AdServer::LogProcessing::ArchiveParams>& archive_params, \
    const PostgresConnectionFactoryImpl_var& pg_conn_factory, \
    const OUT_TYPE& output, \
    CollectorFilterT* collector_filter \
  ) \
  : \
    BaseType(collector_filter), \
    path_(path), \
    archive_params_(archive_params), \
    pg_conn_factory_(pg_conn_factory), \
    output_(output) \
  {} \
  \
  void save(const Spillover_var& data) \
  { \
    collector_filter_->filter(data->collector); \
    \
    if (!data->collector.empty()) \
    { \
      pg_save_i_(data->collector, data->save_time); \
      csv_save_i_(data->collector); \
    } \
  } \
  \
protected: \
  ~LOG_TYPE##PgCsvSaver() noexcept {} \
  \
private: \
  typedef Traits::CollectorType CollectorT; \
  typedef Traits::CollectorBundleType CollectorBundleT; \
  \
  void \
  pg_save_i_( \
    CollectorT &collector, \
    Generics::Time &outtime \
  ); \
  \
  void \
  csv_save_i_(CollectorT &collector) \
  { \
    typedef Traits CsvTraits; \
    StringPair filenames; \
    try \
    { \
      LogFileNameInfo name_info(CsvTraits::csv_base_name()); \
      name_info.format = LogFileNameInfo::LFNF_CSV; \
      filenames = make_log_file_name_pair(name_info, path_); \
      std::unique_ptr<std::ostream> ostream; \
      if (archive_params_) \
      { \
        auto archive_ofstream = std::make_unique<ArchiveOfstream>( \
          filenames.second, \
          *archive_params_); \
        const auto& file_extension = archive_ofstream->file_extension(); \
        filenames.first += file_extension; \
        filenames.second += file_extension; \
        ostream = std::move(archive_ofstream); \
      } \
      else \
      { \
        ostream = std::make_unique<std::ofstream>( \
          filenames.second.c_str()); \
      } \
      auto& ref_ostream = *ostream; \
      if (!ref_ostream) \
      { \
        Stream::Error es; \
        es << __PRETTY_FUNCTION__ << ": Error: " \
           << "Failed to open file '" << filenames.second << '\''; \
        throw CsvException(es); \
      } \
      ref_ostream << CsvTraits::csv_header() << '\n'; \
      if (!ref_ostream) \
      { \
        Stream::Error es; \
        es << __PRETTY_FUNCTION__ << ": Error: Failed to write " \
          "log header to file '" << filenames.second << '\''; \
        throw CsvException(es); \
      } \
      for (CollectorT::const_iterator it = collector.begin(); \
        it != collector.end(); ++it) \
      { \
        for (CollectorT::DataT::const_iterator iit = it->second.begin(); \
          iit != it->second.end(); ++iit) \
        { \
          CsvTraits::write_as_csv(ref_ostream, it->first, iit->first, iit->second) << '\n'; \
        } \
      } \
      ref_ostream.flush(); \
      if (!ref_ostream) \
      { \
        Stream::Error es; \
        es << __PRETTY_FUNCTION__ << ": Error: Failed to write log data " \
          "to file '" << filenames.second << '\''; \
        throw CsvException(es); \
      } \
      ostream.reset(); \
      if (std::rename(filenames.second.c_str(), filenames.first.c_str())) \
      { \
        Stream::Error es; \
        es << __PRETTY_FUNCTION__ << ": Error: " \
           << "Failed to rename temporary file '" << filenames.second \
           << "to file '" << filenames.first << '\''; \
        throw CsvException(es); \
      } \
    } \
    catch (const CsvException&) \
    { \
      unlink(filenames.second.c_str()); \
      throw; \
    } \
    catch (const eh::Exception &ex) \
    { \
      unlink(filenames.second.c_str()); \
      Stream::Error es; \
      es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. " \
        ": " << ex.what(); \
      throw CsvException(es); \
    } \
    catch (...) \
    { \
      unlink(filenames.second.c_str()); \
      throw; \
    } \
  } \
  \
  bool \
  do_save_( \
    Commons::Postgres::Connection_var &connection, \
    const CollectorT::KeyT &outer_key, \
    const CollectorT::DataT &collector \
  ); \
  \
  const std::string path_; \
  const std::optional<AdServer::LogProcessing::ArchiveParams> archive_params_; \
  PostgresConnectionFactoryImpl_var pg_conn_factory_; \
  OUT_TYPE output_; \
}

#define DECLARE_NESTED_LOG_PG_CSV_SAVER_USING_PREFIX( \
          LOG_TRAITS, \
          LOG_CSV_TRAITS, \
          OUT_TYPE, \
          PREFIX \
        ) \
class PREFIX##PgCsvSaver: \
  public LogSaverBaseImpl<PREFIX##ExtTraits::CollectorBundleType> \
{ \
  typedef LogSaverBaseImpl<PREFIX##ExtTraits::CollectorBundleType> BaseType; \
public: \
  DECLARE_EXCEPTION(PgException, LogSaver::Exception); \
  DECLARE_EXCEPTION(CsvException, LogSaver::Exception); \
  \
  typedef PREFIX##ExtTraits Traits; \
  typedef Traits::CollectorBundleType CollectorBundleType; \
  typedef ReferenceCounting::SmartPtr<Traits::PgCsvSaverType> SmartPtr; \
  typedef BaseType::Spillover_var Spillover_var; \
  typedef Traits::CollectorFilterType CollectorFilterT; \
  \
  PREFIX##PgCsvSaver( \
    const std::string& path, \
    const std::optional<AdServer::LogProcessing::ArchiveParams>& archive_params, \
    const PostgresConnectionFactoryImpl_var& pg_conn_factory, \
    const OUT_TYPE& output, \
    CollectorFilterT* collector_filter \
  ) \
  : \
    BaseType(collector_filter), \
    path_(path), \
    archive_params_(archive_params), \
    pg_conn_factory_(pg_conn_factory), \
    output_(output) \
  {} \
  \
  void save(const Spillover_var& data) \
  { \
    collector_filter_->filter(data->collector); \
    \
    if (!data->collector.empty()) \
    { \
      pg_save_i_(data->collector, \
        data->get_min_file_timestamp(), data->save_time); \
      csv_save_i_(data->collector); \
    } \
  } \
  \
protected: \
  ~PREFIX##PgCsvSaver() noexcept {} \
  \
private: \
  typedef Traits::CollectorType CollectorT; \
  typedef Traits::CollectorBundleType CollectorBundleT; \
  \
  void \
  pg_save_i_( \
    CollectorT &collector, \
    const Generics::Time &intime, \
    Generics::Time &outtime \
  ); \
  \
  void \
  csv_save_i_(CollectorT &collector) \
  { \
    typedef LOG_CSV_TRAITS CsvTraits; \
    StringPair filenames; \
    try \
    { \
      LogFileNameInfo name_info(CsvTraits::csv_base_name()); \
      name_info.format = LogFileNameInfo::LFNF_CSV; \
      filenames = make_log_file_name_pair(name_info, path_); \
      std::unique_ptr<std::ostream> ostream; \
      if (archive_params_) \
      { \
        auto archive_ofstream = std::make_unique<ArchiveOfstream>( \
          filenames.second, \
          *archive_params_); \
        const auto& file_extension = archive_ofstream->file_extension(); \
        filenames.first += file_extension; \
        filenames.second += file_extension; \
        ostream = std::move(archive_ofstream); \
      } \
      else \
      { \
        ostream = std::make_unique<std::ofstream>( \
          filenames.second.c_str()); \
      } \
      auto& ref_ostream = *ostream; \
      if (!ref_ostream) \
      { \
        Stream::Error es; \
        es << __PRETTY_FUNCTION__ << ": Error: " \
           << "Failed to open file '" << filenames.second << '\''; \
        throw CsvException(es); \
      } \
      ref_ostream << CsvTraits::csv_header() << '\n'; \
      if (!ref_ostream) \
      { \
        Stream::Error es; \
        es << __PRETTY_FUNCTION__ << ": Error: Failed to write " \
          "log header to file '" << filenames.second << '\''; \
        throw CsvException(es); \
      } \
      for (CollectorT::const_iterator it = collector.begin(); \
        it != collector.end(); ++it) \
      { \
        for (CollectorT::DataT::const_iterator iit = it->second.begin(); \
          iit != it->second.end(); ++iit) \
        { \
          CsvTraits::write_as_csv(ref_ostream, it->first, iit->first, iit->second) << '\n'; \
        } \
      } \
      ref_ostream.flush(); \
      if (!ref_ostream) \
      { \
        Stream::Error es; \
        es << __PRETTY_FUNCTION__ << ": Error: Failed to write log data " \
           << "to file '" << filenames.second << '\''; \
        throw CsvException(es); \
      } \
      ostream.reset(); \
      if (std::rename(filenames.second.c_str(), filenames.first.c_str())) \
      { \
        Stream::Error es; \
        es << __PRETTY_FUNCTION__ << ": Error: " \
           << "Failed to rename temporary file '" << filenames.second \
           << "to file '" << filenames.first << '\''; \
        throw CsvException(es); \
      } \
    } \
    catch (const CsvException&) \
    { \
      unlink(filenames.second.c_str()); \
      throw; \
    } \
    catch (const eh::Exception &ex) \
    { \
      unlink(filenames.second.c_str()); \
      Stream::Error es; \
      es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. " \
        ": " << ex.what(); \
      throw CsvException(es); \
    } \
    catch (...) \
    { \
      unlink(filenames.second.c_str()); \
      throw; \
    } \
  } \
  \
  bool \
  do_save_( \
    Commons::Postgres::Connection_var &connection, \
    const CollectorT::KeyT &outer_key, \
    const CollectorT::DataT &collector \
  ); \
  \
  const std::string path_; \
  const std::optional<ArchiveParams> archive_params_; \
  PostgresConnectionFactoryImpl_var pg_conn_factory_; \
  OUT_TYPE output_; \
}

#endif /* AD_SERVER_LOG_PROCESSING_DB_SAVER_DECL_MACRO_HPP */

