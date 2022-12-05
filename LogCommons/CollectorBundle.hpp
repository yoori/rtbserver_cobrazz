#ifndef AD_SERVER_LOG_PROCESSING_COLLECTOR_BUNDLE_HPP
#define AD_SERVER_LOG_PROCESSING_COLLECTOR_BUNDLE_HPP


#include <cstddef>
#include <numeric>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Sync/SyncPolicy.hpp>

#include <LogCommons/CollectorBundleTypeDefs.hpp>
#include "LogSaverBaseImpl.hpp"

namespace AdServer {
namespace LogProcessing {

namespace CollectorBundleImplDefs_ {

template <class COLLECTOR_TYPE_, bool COLLECTOR_IS_NESTED_>
struct SizeCalculator;

template <class COLLECTOR_TYPE_>
struct SizeCalculator<COLLECTOR_TYPE_, false>
{
  static std::size_t calc_size(const COLLECTOR_TYPE_& collector)
  {
    return collector.size();
  }
};

template <class COLLECTOR_TYPE_>
struct SizeCalculator<COLLECTOR_TYPE_, true>
{
  static std::size_t calc_size(const COLLECTOR_TYPE_& collector)
  {
    return std::accumulate(collector.begin(),
      collector.end(), 0, add_size_);
  }

private:
  typedef typename COLLECTOR_TYPE_::ValueT ValueT;

  static std::size_t
  add_size_(std::size_t result, const ValueT& value) noexcept
  {
    return result + value.second.size();
  }
};

} // namespace CollectorBundleImplDefs_

  /**
   * Interface to LogGeneralizer data dumping.
   * Implementation have access to context object needs to perform independent
   * write task into separate thread.
   */
  template <typename BundleType>
  class ThreadLogSaver : public ReferenceCounting::AtomicImpl
  {
  public:
    typedef ReferenceCounting::SmartPtr<ThreadLogSaver> SmartPtr;
    typedef SpilloverImpl<typename BundleType::CollectorT>
      SpilloverType;
    typedef typename SpilloverImpl<typename BundleType::CollectorT>::
      SmartPtr Spillover_var;

    /**
     * Async save the data
     */
    virtual void
    dump_portion(const Spillover_var& data) noexcept = 0;

    /**
     * Implementation initiate periodical task that flush
     * expired data of bundle
     */
    virtual void
    dump_start(const Spillover_var& data, BundleType* spillovers_owner = 0) noexcept = 0;
  protected:
    virtual
    ~ThreadLogSaver() noexcept
    {}
  };


template <class LogTraits>
class CollectorBundle:
  private CollectorBundleParams,
  public ReferenceCounting::AtomicImpl
{
  typedef SpilloverImpl<typename LogTraits::CollectorType> SpilloverType;
  typedef typename SpilloverType::SmartPtr Spillover_var;
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef typename LogTraits::CollectorType CollectorT;
  typedef typename ThreadLogSaver<CollectorBundle<LogTraits> >::SmartPtr
    ThreadLogSaver_var;

  CollectorBundle(
    const CollectorBundleParams& params,
    const ThreadLogSaver_var& saver)
    noexcept;

  void
  merge(
    CollectorT& file_collector,
    const CollectorBundleFileGuard_var& file_handle)
    /*throw(eh::Exception)*/;

  /**
   * Take away the data, call by periodic tasks. Return data that should be
   * saved by caller (possible, synchronous)
   */
  void
  take_data(Spillover_var& data) /*throw(eh::Exception)*/
  {
    WriteGuard_ guard(lock_);
    if (data_->collector_size)
    {
      data = data_;
      data_ = Spillover_var(new SpilloverType);
    }
  }

  bool
  file_list_is_empty() const noexcept;

  /**
   * @return Generics::Time::ZERO if have not files into bundle.
   *   Other values means the minimum timestamp of stored files at the now.
   */
  Generics::Time
  get_min_file_timestamp() const /*throw(InvalidLogFileNameFormat)*/;

private:
  virtual
  ~CollectorBundle() noexcept
  {}

  friend
  class CollectorBundleImplDefs_::SizeCalculator<
    CollectorT, LogTraits::is_nested
  >;

  typedef CollectorBundleImplDefs_::SizeCalculator<
    CollectorT, LogTraits::is_nested
  > SizeCalculator_;

  typedef Sync::PosixRWLock RWLock_;
  typedef Sync::PosixRGuard ReadGuard_;
  typedef Sync::PosixWGuard WriteGuard_;

  typename SpilloverType::SmartPtr data_;
  ThreadLogSaver_var saver_;

  mutable RWLock_ lock_;
};

template <typename LogTraits>
inline
CollectorBundle<LogTraits>::CollectorBundle(
  const CollectorBundleParams& params,
  const ThreadLogSaver_var& saver)
  noexcept
:
  CollectorBundleParams(params),
  data_(new SpilloverType),
  saver_(saver)
{
  saver_->dump_start(new SpilloverType, this);
}

template <typename LogTraits>
inline
void
CollectorBundle<LogTraits>::merge(
  CollectorT& file_collector,
  const CollectorBundleFileGuard_var& file_handle)
  /*throw(eh::Exception)*/
{
  Spillover_var ready;
  {
    WriteGuard_ guard(lock_);

    if (data_->collector.empty())
    {
      data_->collector.swap(file_collector);
    }
    else
    {
      data_->collector.merge(file_collector);
    }
    data_->push_back(file_handle);
    data_->collector_size = SizeCalculator_::calc_size(data_->collector);
    if (data_->collector_size >= MAX_SIZE)
    {
      ready = data_;
      data_ = Spillover_var(new SpilloverType);
    }
  }
  if (ready)
  {
    saver_->dump_portion(ready);
  }
}

template <typename LogTraits>
inline
bool
CollectorBundle<LogTraits>::file_list_is_empty() const noexcept
{
  ReadGuard_ guard(lock_);
  return data_->files().empty();
}

template <typename LogTraits>
inline
Generics::Time
CollectorBundle<LogTraits>::get_min_file_timestamp() const
  /*throw(InvalidLogFileNameFormat)*/
{
  ReadGuard_ guard(lock_);
  return data_->get_min_file_timestamp();
}

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_COLLECTOR_BUNDLE_HPP */
