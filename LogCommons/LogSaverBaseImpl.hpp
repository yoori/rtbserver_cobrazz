#ifndef LOGSAVERBASEIMPL_HPP_INCLUDED
#define LOGSAVERBASEIMPL_HPP_INCLUDED

#include <LogCommons/LogCommons.hpp>

namespace AdServer {
namespace LogProcessing {

  template <typename CollectorType>
  class SpilloverImpl : public ReferenceCounting::AtomicImpl
  {
  public:
    typedef ReferenceCounting::SmartPtr<SpilloverImpl<CollectorType> >
      SmartPtr;

    SpilloverImpl() noexcept
      : collector_size(0)
    {}

    void
    clear() noexcept
    {
      collector.clear();
      files_.clear();
      collector_size = 0;
      save_time = Generics::Time::ZERO;
      min_file_timestamp_.clear();
    }

    void
    push_back(const CollectorBundleFileGuard_var& file_handle)
      /*throw(eh::Exception)*/
    {
      files_.push_back(file_handle);
      const std::string& new_file_name = file_handle->file_name();
      if (!min_file_timestamp_.empty())
      {
        if (new_file_name < min_file_timestamp_)
        {
          min_file_timestamp_ = new_file_name;
        }
      }
      else
      {
        min_file_timestamp_ = new_file_name;
      }
    }

    const CollectorBundleFileList&
    files() const noexcept
    {
      return files_;
    }

    Generics::Time
    get_min_file_timestamp() const /*throw(InvalidLogFileNameFormat)*/
    {
      if (min_file_timestamp_.empty())
      {
        return Generics::Time::ZERO;
      }
      LogFileNameInfo info;
      parse_log_file_name(min_file_timestamp_, info);
      return info.timestamp;
    }

    CollectorType collector;
    std::size_t collector_size;
    Generics::Time save_time;
  private:
    virtual
    ~SpilloverImpl() noexcept
    {}

    CollectorBundleFileList files_;
    std::string min_file_timestamp_;
  };

  template <typename CollectorBundleType>
  class LogSaverBaseImpl : public virtual LogSaver
  {
  public:
    typedef ReferenceCounting::SmartPtr<LogSaverBaseImpl<CollectorBundleType> >
      SmartPtr;
    typedef typename SpilloverImpl<
      typename CollectorBundleType::CollectorT>::SmartPtr Spillover_var;
    typedef CollectorFilter<typename CollectorBundleType::CollectorT>
      CollectorFilterType;

    LogSaverBaseImpl(
      CollectorFilterType* collector_filter)
      : collector_filter_(ReferenceCounting::add_ref(collector_filter))
    {}

    virtual
    void
    save(const Spillover_var& data) = 0;

  protected:
    typename CollectorFilterType::SmartPtr collector_filter_;
    typedef typename CollectorBundleType::CollectorT CollectorT;

    virtual
    ~LogSaverBaseImpl() noexcept
    {}

  };

} // namespace LogProcessing
} // namespace AdServer

#endif // LOGSAVERBASEIMPL_HPP_INCLUDED
