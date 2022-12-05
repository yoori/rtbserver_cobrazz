#ifndef AD_SERVER_LOG_PROCESSING_GENERIC_LOG_IO_IMPL_HPP
#define AD_SERVER_LOG_PROCESSING_GENERIC_LOG_IO_IMPL_HPP


#include <iosfwd>
#include <vector>
#include <string>
#include <fstream>
#include <tr1/unordered_map>

#include <Generics/GnuHashTable.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Generics/ArrayAutoPtr.hpp>
#include <LogCommons/LogCommons.ipp>
#include "CollectorBundle.hpp"

#include <LogCommons/LogSaverBaseImpl.hpp>

namespace AdServer {
namespace LogProcessing {

template <class LOG_TYPE_TRAITS_>
class GenericLogHeader
{
public:
  GenericLogHeader(): version_() {}

  explicit
  GenericLogHeader(const std::string &version): version_(version) {}

  const std::string& version() const
  {
    return version_;
  }

  template <class TRAITS_>
  friend
  std::istream& operator>>(std::istream &is,
    GenericLogHeader<TRAITS_> &header);

  template <class TRAITS_>
  friend
  std::ostream& operator<<(std::ostream &os,
    const GenericLogHeader<TRAITS_> &header);

private:
  std::string version_;
};

template <class LOG_TYPE_TRAITS_>
std::istream&
operator>>(std::istream &is, GenericLogHeader<LOG_TYPE_TRAITS_> &header)
{
  std::string signature;
  is >> signature;
  if (signature != LOG_TYPE_TRAITS_::signature())
  {
    is.setstate(std::ios::failbit);
  }
  else
  {
    read_tab(is);
    is >> header.version_;
  }
  read_eol(is);
  return is;
}

template <class LOG_TYPE_TRAITS_>
std::ostream&
operator<<(std::ostream &os, const GenericLogHeader<LOG_TYPE_TRAITS_> &header)
{
  if (header.version_.empty())
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Log header version cannot be empty!";
    throw InvalidArgValue(es);
  }
  os << LOG_TYPE_TRAITS_::signature() << '\t' << header.version_ << '\n';
  return os;
}

namespace Detail
{
  template <typename CollectorType, typename LoadAction>
  struct CollectorLoad
  {
    CollectorLoad(CollectorType& collector) noexcept
      : collector_(collector)
    {}

    void
    load_impl(std::istream& is, const CollectorBundleFileGuard_var&)
      /*throw(eh::Exception)*/
    {
      LoadAction::load_impl(is, collector_);
    }

    CollectorType& collector_;
  };

  template <typename LogTraits, typename LoadAction>
  class BundleLoad
  {
  protected:
    BundleLoad(const typename LogTraits::CollectorBundlePtrType& bundle)
      : bundle_(bundle)
    {}

    void
    load_impl(std::istream& is, const CollectorBundleFileGuard_var& file_handle)
      /*throw(eh::Exception)*/
    {
      LoadAction::load_impl(is, bundle_, file_handle);
    }

    typename LogTraits::CollectorBundlePtrType bundle_;
  };

  //
  // Current version load files elementary strategies
  //

  // Not nested, not thread_safe
  struct SimpleLoad
  {
    template <typename Collector>
    static
    void
    load_impl(std::istream& is, Collector& collector) /*throw(eh::Exception)*/
    {
      is >> collector;
    }
  };

  // Nested, not thread_safe
  struct NestedLoad
  {
    template <typename Collector>
    static
    void
    load_impl(std::istream& is, Collector& collector) /*throw(eh::Exception)*/
    {
      typename Collector::KeyT key;
      typename Collector::DataT data;
      is >> key;
      read_eol(is);
      is >> data;
      if (is.eof())
      {
        collector.add(key, data);
      }
    }
  };

  // Not nested, but thread safe way
  struct SimpleSafeLoad
  {
    template <typename CollectorBundlePtrType>
    static
    void
    load_impl(
      std::istream& is,
      const CollectorBundlePtrType& bundle,
      const CollectorBundleFileGuard_var& file_handle
    )
      /*throw(eh::Exception)*/
    {
      typename CollectorBundlePtrType::Type::CollectorT collector;
      is >> collector;
      bundle->merge(collector, file_handle);
    }
  };

  // nested, thread safe
  struct NestedSafeLoad
  {
    template <typename CollectorBundlePtrType>
    static
    void
    load_impl(
      std::istream& is,
      const CollectorBundlePtrType& bundle,
      const CollectorBundleFileGuard_var& file_handle
    )
      /*throw(eh::Exception)*/
    {
      typedef typename CollectorBundlePtrType::Type::CollectorT CollectorT;
      typename CollectorT::KeyT key;
      typename CollectorT::DataT data;
      is >> key;
      read_eol(is);
      is >> data;
      if (is.eof())
      {
        CollectorT collector;
        collector.add(key, data);
        bundle->merge(collector, file_handle);
      }
    }
  };

  struct NestedPackedLoad
  {
    template <typename Collector>
    static
    void
    load_impl(std::istream& is, Collector& collector) /*throw(eh::Exception)*/
    {
      typename Collector::KeyT key;
      while (1)
      {
        typename Collector::DataT data;
        is >> key;
        read_eol(is);
        is >> data;
        if (is.eof())
        {
          collector.move_data(key, data);
          break;
        }
        if (is.peek() == '\n')
        {
          collector.move_data(key, data);
          read_eol(is);
        }
      }
    }
  };

  // nested, packed, thread safe
  struct NestedPackedSafeLoad
  {
    template <typename CollectorBundlePtrType>
    static
    void
    load_impl(
      std::istream& is,
      const CollectorBundlePtrType& bundle,
      const CollectorBundleFileGuard_var& file_handle
    )
      /*throw(eh::Exception)*/
    {
      typedef typename CollectorBundlePtrType::Type::CollectorT CollectorT;
      CollectorT collector;
      typename CollectorT::KeyT key;
      while (1)
      {
        typename CollectorT::DataT data;
        is >> key;
        read_eol(is);
        is >> data;
        if (is.eof())
        {
          collector.add(key, data);
          bundle->merge(collector, file_handle);
          break;
        }
        if (is.peek() == '\n')
        {
          collector.add(key, data);
          read_eol(is);
        }
      }
    }
  };

  //
  // Old versions load policies
  //

  template <typename OldCollector>
  struct OldSimpleLoad
  {
    template <typename Collector>
    static
    void
    load_impl(std::istream& is, Collector& collector)
      /*throw(eh::Exception)*/
    {
      OldCollector old_collector;
      is >> old_collector;
      if (is.eof())
      {
        collector.insert(collector.end(),
          old_collector.begin(), old_collector.end());
      }
    }
  };

  template <typename OldCollector>
  struct OldNestedLoad
  {
    template <typename Collector>
    static
    void
    load_impl(std::istream& is, Collector& collector)
      /*throw(eh::Exception)*/
    {
      typename Collector::DataT data;
      typename OldCollector::KeyT old_key;
      typename OldCollector::DataT old_data;
      is >> old_key;
      read_eol(is);
      is >> old_data;
      if (is.eof())
      {
        data.insert(data.end(), old_data.begin(), old_data.end());
        collector.add(old_key, data);
      }
    }
  };

  template <typename OldCollector>
  struct OldNestedPackedLoad
  {
    template <typename Collector>
    static
    void
    load_impl(std::istream& is, Collector& collector)
      /*throw(eh::Exception)*/
    {
      while (1)
      {
        typename OldCollector::KeyT old_key;
        is >> old_key;
        read_eol(is);
        typename OldCollector::DataT old_data;
        is >> old_data;
        typename Collector::DataT data;
        if (is.eof())
        {
          data.insert(data.end(), old_data.begin(), old_data.end());
          collector.move_data(old_key, data);
          break;
        }
        if (is.peek() == '\n')
        {
          data.insert(data.end(), old_data.begin(), old_data.end());
          collector.move_data(old_key, data);
          read_eol(is);
        }
      }
    }
  };

  template <typename OldCollector>
  struct OldSummableLoad
  {
    template <typename Collector>
    static
    void
    load_impl(std::istream& is, Collector& collector)
      /*throw(eh::Exception)*/
    {
      OldCollector old_collector;
      is >> old_collector;
      if (is.eof())
      {
        collector.insert(old_collector.begin(), old_collector.end());
      }
    }
  };

  template <typename OldCollector>
  struct OldNestedSummableLoad
  {
    template <typename Collector>
    static
    void
    load_impl(std::istream& is, Collector& collector)
      /*throw(eh::Exception)*/
    {
      typename OldCollector::KeyT old_key;
      is >> old_key;
      read_eol(is);
      typename OldCollector::DataT old_data;
      is >> old_data;
      if (is.eof())
      {
        typename Collector::DataT data;
        data.insert(old_data.begin(), old_data.end());
        collector.add(old_key, data);
      }
    }
  };

  template <typename OldCollector>
  struct OldNestedPackedSummableLoad
  {
    template <typename Collector>
    static
    void
    load_impl(std::istream& is, Collector& collector)
      /*throw(eh::Exception)*/
    {
      while (1)
      {
        typename OldCollector::KeyT old_key;
        is >> old_key;
        read_eol(is);
        typename OldCollector::DataT old_data;
        is >> old_data;
        typename Collector::DataT data;
        if (is.eof())
        {
          data.insert(old_data.begin(), old_data.end());
          collector.add(old_key, data);
          break;
        }
        if (is.peek() == '\n')
        {
          data.insert(old_data.begin(), old_data.end());
          collector.add(old_key, data);
          read_eol(is);
        }
      }
    }
  };

  template <typename OldCollector>
  struct OldSimpleSafeLoad
  {
    template <typename CollectorBundlePtrType>
    static
    void
    load_impl(
      std::istream& is,
      const CollectorBundlePtrType& bundle,
      const CollectorBundleFileGuard_var& file_handle)
      /*throw(eh::Exception)*/
    {
      typedef typename CollectorBundlePtrType::Type::CollectorT CollectorT;
      OldCollector old_collector;
      is >> old_collector;
      CollectorT collector;
      if (is.eof())
      {
        collector.insert(collector.end(),
          old_collector.begin(), old_collector.end());
      }
      bundle->merge(collector, file_handle);
    }
  };

  template <typename CurrentTraits, typename OldCollector, bool is_old_nested>
  struct OldLoad
  {
    static
    void
    load_impl(std::istream& is, typename CurrentTraits::CollectorType& collector)
      /*throw(eh::Exception)*/
    {
      typedef typename LoadPolicySelector<OldCollector, is_old_nested>::LoadType LoadType;

      OldCollector old_collector;
      LoadType old_loader(old_collector);
      old_loader.load_impl(is, CollectorBundleFileGuard_var());

      collector = CurrentTraits::convert_collector(old_collector);
    }
  };

  template <typename CurrentTraits, typename OldCollector, bool is_old_nested>
  struct OldSafeLoad
  {
    static
    void
    load_impl(
      std::istream& is,
      const typename CurrentTraits::CollectorBundlePtrType& bundle,
      const CollectorBundleFileGuard_var& file_handle)
      /*throw(eh::Exception)*/
    {
      typedef typename CurrentTraits::CollectorType CollectorT;

      CollectorT collector;
      OldLoad<CurrentTraits, OldCollector, is_old_nested>::load_impl(is, collector);

      bundle->merge(collector, file_handle);
    }
  };

  template <typename OldCollector>
  struct OldNestedSafeLoad
  {
    template <typename CollectorBundlePtrType>
    static
    void
    load_impl(
      std::istream& is,
      const CollectorBundlePtrType& bundle,
      const CollectorBundleFileGuard_var& file_handle)
      /*throw(eh::Exception)*/
    {
      typedef typename CollectorBundlePtrType::Type::CollectorT CollectorT;
      typename OldCollector::KeyT old_key;
      is >> old_key;
      read_eol(is);
      typename OldCollector::DataT old_data;
      is >> old_data;
      CollectorT collector;
      if (is.eof())
      {
        typename CollectorT::DataT data;
        data.insert(data.end(), old_data.begin(), old_data.end());
        collector.add(old_key, data);
      }
      bundle->merge(collector, file_handle);
    }
  };

  template <typename OldCollector>
  struct OldSummableSafeLoad
  {
    template <typename CollectorBundlePtrType>
    static
    void
    load_impl(
      std::istream& is,
      const CollectorBundlePtrType& bundle,
      const CollectorBundleFileGuard_var& file_handle)
      /*throw(eh::Exception)*/
    {
      typedef typename CollectorBundlePtrType::Type::CollectorT CollectorT;
      OldCollector old_collector;
      is >> old_collector;
      CollectorT collector;
      if (is.eof())
      {
        collector.insert(old_collector.begin(), old_collector.end());
      }
      bundle->merge(collector, file_handle);
    }
  };

  template <typename OldCollector>
  struct OldNestedSummableSafeLoad
  {
    template <typename CollectorBundlePtrType>
    static
    void
    load_impl(
      std::istream& is,
      const CollectorBundlePtrType& bundle,
      const CollectorBundleFileGuard_var& file_handle)
      /*throw(eh::Exception)*/
    {
      typedef typename CollectorBundlePtrType::Type::CollectorT CollectorT;
      typename OldCollector::KeyT old_key;
      is >> old_key;
      read_eol(is);
      typename OldCollector::DataT old_data;
      is >> old_data;
      CollectorT collector;
      if (is.eof())
      {
        typename CollectorT::DataT data;
        data.insert(old_data.begin(), old_data.end());
        collector.add(old_key, data);
      }
      bundle->merge(collector, file_handle);
    }
  };

  template <typename OldCollector>
  struct OldNestedPackedSummableSafeLoad
  {
    template <typename CollectorBundlePtrType>
    static
    void
    load_impl(
      std::istream& is,
      const CollectorBundlePtrType& bundle,
      const CollectorBundleFileGuard_var& file_handle
    )
      /*throw(eh::Exception)*/
    {
      typedef typename CollectorBundlePtrType::Type::CollectorT CollectorT;
      CollectorT collector;
      typename CollectorT::KeyT key;
      while (1)
      {
        typename OldCollector::KeyT old_key;
        is >> old_key;
        read_eol(is);
        typename OldCollector::DataT old_data;
        is >> old_data;
        if (is.eof())
        {
          typename CollectorT::DataT data;
          data.insert(old_data.begin(), old_data.end());
          collector.add(old_key, data);
          bundle->merge(collector, file_handle);
          break;
        }
        if (is.peek() == '\n')
        {
          typename CollectorT::DataT data;
          data.insert(old_data.begin(), old_data.end());
          collector.add(old_key, data);
          read_eol(is);
        }
      }
    }
  };

  template <
    typename OldCollector,
    bool is_nested,
    bool is_summable,
    bool use_packed_loader = false>
  struct OldLoadActionSelector;

  template <typename OldCollector>
  struct OldLoadActionSelector<OldCollector, false, false>
  {
    typedef OldSimpleLoad<OldCollector> LoadType;
    typedef OldSimpleSafeLoad<OldCollector> SafeLoadType;
  };

  template <typename OldCollector>
  struct OldLoadActionSelector<OldCollector, false, true>
  {
    typedef OldSummableLoad<OldCollector> LoadType;
    typedef OldSummableSafeLoad<OldCollector> SafeLoadType;
  };

  template <typename OldCollector>
  struct OldLoadActionSelector<OldCollector, true, false>
  {
    typedef OldNestedLoad<OldCollector> LoadType;
    typedef OldNestedSafeLoad<OldCollector> SafeLoadType;
  };

  template <typename OldCollector>
  struct OldLoadActionSelector<OldCollector, true, false, true>
  {
    typedef OldNestedPackedLoad<OldCollector> LoadType;
  };

  template <typename OldCollector>
  struct OldLoadActionSelector<OldCollector, true, true>
  {
    typedef OldNestedSummableLoad<OldCollector> LoadType;
    typedef OldNestedSummableSafeLoad<OldCollector> SafeLoadType;
  };

  template <typename OldCollector>
  struct OldLoadActionSelector<OldCollector, true, true, true>
  {
    typedef OldNestedPackedSummableLoad<OldCollector> LoadType;
    typedef OldNestedPackedSummableSafeLoad<OldCollector> SafeLoadType;
  };
} // namespace Detail

template <typename LogTraits, typename LoadPolicy>
class GenericLogLoaderImpl: protected LoadPolicy, public LogLoader
{
public:
  template <typename CollectorOrBundle>
  GenericLogLoaderImpl(CollectorOrBundle& collector) :
    LoadPolicy(collector)
  {}

  virtual
  void
  load(std::istream& is, const CollectorBundleFileGuard_var& file_handle =
    CollectorBundleFileGuard_var())
    /*throw(Exception)*/
  {
    try
    {
      is.unsetf(std::ios::skipws);
      LoadPolicy::load_impl(is, file_handle);
    }
    catch (const Exception&)
    {
      throw;
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. : " << ex.what();
      throw Exception(es);
    }
    if (!is.eof())
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ <<
        ": Error: Malformed log file (extra data at the end of file)";
      throw Exception(es);
    }
  }

private:
  virtual
  ~GenericLogLoaderImpl() noexcept
  {}
};

template <typename LogTraits>
class HitsFilteringLogLoaderImpl: public LogLoader
{
public:
  HitsFilteringLogLoaderImpl(
    const typename LogTraits::CollectorBundlePtrType& bundle,
    HitsFilter_var& hits_filter
  )
  :
    bundle_(bundle),
    hits_filter_(hits_filter)
  {
  }

  virtual
  void
  load(std::istream& is, const CollectorBundleFileGuard_var& file_handle =
    CollectorBundleFileGuard_var())
    /*throw(Exception)*/
  {
    try
    {
      is.unsetf(std::ios::skipws);
      return load_i_(is, file_handle);
    }
    catch (const Exception&)
    {
      throw;
    }
    catch (const eh::Exception &ex)
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Caught eh::Exception. "
         << ": " << ex.what();
      throw Exception(es);
    }
    if (!is.eof())
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Error: Malformed log file "
         << "(extra data at the end of file)";
      throw Exception(es);
    }
  }

private:
  void
  load_i_(std::istream& is, const CollectorBundleFileGuard_var& file_handle)
    /*throw(eh::Exception)*/
  {
    typedef typename LogTraits::CollectorBundlePtrType::Type::CollectorT
      CollectorT;
    typename CollectorT::KeyT key;
    typename CollectorT::DataT data;
    is >> key;
    read_eol(is);
    is >> data;
    if (is.eof())
    {
      {
        HitsFilter::Locker_var locker = hits_filter_->create_locker();
        for (typename CollectorT::DataT::iterator it = data.begin();
          it != data.end(); )
        {
          unsigned long hits = hits_filter_->check(key.sdate(),
            it->first.distrib_hash(), it->second.hits());
          if (hits)
          {
            (it++)->second.set_hits(hits);
          }
          else
          {
            data.erase(it++);
          }
        }
      }
      CollectorT collector;
      if (!data.empty())
      {
        collector.add(key, data);
      }
      bundle_->merge(collector, file_handle);
    }
  }

  HitsFilteringLogLoaderImpl(const HitsFilteringLogLoaderImpl&);
  HitsFilteringLogLoaderImpl& operator=(const HitsFilteringLogLoaderImpl&);

  virtual
  ~HitsFilteringLogLoaderImpl() noexcept {}

  typename LogTraits::CollectorBundlePtrType bundle_;
  HitsFilter_var hits_filter_;
};

class LoadVersionsFacility
{
  typedef Generics::GnuHashTable<Generics::StringHashAdapter, LogLoader_var>
    LogLoaderMap;

protected:
  DECLARE_EXCEPTION(UnsupportedLogVersion, eh::DescriptiveException);

  void
  load_impl_(
    const std::string& version,
    std::istream& is,
    const CollectorBundleFileGuard_var& file_handle
  )
    const
    /*throw(eh::Exception)*/
  {
    LogLoaderMap::const_iterator it = loader_map_.find(version);
    if (it == loader_map_.end())
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Unsupported version: " << version;
      throw UnsupportedLogVersion(es);
    }
    it->second->load(is, file_handle);
  }

  void
  add_loader_(const std::string& version, const LogLoader_var& loader)
    /*throw(eh::Exception)*/
  {
    loader_map_[version] = loader;
  }

private:
  LogLoaderMap loader_map_;
};

template <class Traits>
class GenericLogIoHelperImpl: public LogIoHelper, private LoadVersionsFacility
{
  typedef GenericLogIoHelperImpl<Traits> ThisType;

public:
  typedef typename Traits::CollectorType CollectorT;

  explicit
  GenericLogIoHelperImpl(CollectorT& collector)
  :
    collector_(collector)
  {
    add_loader_(Traits::current_version(),
      new typename Traits::LoaderType(collector_));
    PreviousVersionSupportAdder_ prev_ver_adder(this);
    Traits::for_each_old(prev_ver_adder);
  }

  virtual
  ~GenericLogIoHelperImpl() noexcept {}

  virtual
  void
  load(std::istream& is);

  virtual
  void
  save(const std::string& path);

private:

  struct PreviousVersionSupportAdder_
  {
    PreviousVersionSupportAdder_(ThisType* ptr) noexcept
      : owner_(ptr)
    {}

    template <class OLD_COLLECTOR_, bool use_packed_loader>
    void
    operator ()(const char* version) /*throw(eh::Exception)*/
    {
      typedef GenericLogLoaderImpl<Traits,
        typename Detail::CollectorLoad<typename Traits::CollectorType,
          typename Detail::OldLoadActionSelector<OLD_COLLECTOR_,
            Traits::is_nested, Traits::is_summable, use_packed_loader
          >::LoadType>
        > PreviousVersionLoaderT;
      owner_->add_loader_(version,
        new PreviousVersionLoaderT(owner_->collector_));
    }

    template <class OLD_COLLECTOR_>
    void
    operator ()(const char* version) /*throw(eh::Exception)*/
    {
      operator()<OLD_COLLECTOR_, false>(version);
    }

    template <class OLD_COLLECTOR_, bool is_old_nested>
    void
    support(const char* version) /*throw(eh::Exception)*/
    {
      typedef GenericLogLoaderImpl<Traits,
        typename Detail::CollectorLoad<typename Traits::CollectorType,
        Detail::OldLoad<Traits, OLD_COLLECTOR_, is_old_nested> >
        > PreviousVersionLoaderT;
      owner_->add_loader_(version,
        new PreviousVersionLoaderT(owner_->collector_));
    }

  private:
    ThisType* owner_;
  };

  CollectorT& collector_;
};

template <class Traits>
void
GenericLogIoHelperImpl<Traits>::load(std::istream &is)
{
  try
  {
    typename Traits::HeaderType log_header;
    is.unsetf(std::ios::skipws);
    if (!(is >> log_header))
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Read error or invalid log header!";
      throw Exception(es);
    }
    return load_impl_(log_header.version(), is, CollectorBundleFileGuard_var());
  }
  catch (const Exception &ex)
  {
    throw;
  }
  catch (const eh::Exception &ex)
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Exception: " << ex.what();
    throw Exception(es);
  }
  catch (...)
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Unknown exception!!!";
    throw Exception(es);
  }
}

template <class Traits>
void
GenericLogIoHelperImpl<Traits>::save(const std::string& path)
{
  try
  {
    LogIoProxy<Traits>::save(collector_, path);
  }
  catch (const eh::Exception &ex)
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Exception: " << ex.what();
    throw Exception(es);
  }
}

template <class LOG_TYPE_TRAITS_, class SaveStrategy>
class GenericLogSaverImplBase: public virtual LogSaver
{
protected:
  typedef typename LOG_TYPE_TRAITS_::CollectorType CollectorT;

  GenericLogSaverImplBase(const std::string& path, unsigned long distrib_count = 0)
    : path_(path), distrib_count_(distrib_count) {}

  /*
   * This method saves collector using SaveStrategy-strategy in single output file.
   * Method uses members distrib_count_/path_ and in-param distrib_index for output filename.
   * @param collector[in] - collector for saving; It can be as part of large collector.
   * @param distrib_index[in] - index is used for output filename;
   * @param rename[in] - if true, method renames temporary output file to regular output file;
   *   if false - it doesn't rename; both filenames are returned in result;
   * @return result.first - regular file name, result.second - temporary file name.
   */
  StringPair
  save_file(const CollectorT& collector, unsigned long distrib_index = 0, bool rename = true) const;

  virtual
  ~GenericLogSaverImplBase() noexcept {}

  const std::string path_;
  const unsigned long distrib_count_;
};

template <class LOG_TYPE_TRAITS_, class SaveStrategy>
class GenericLogSaverImpl<LOG_TYPE_TRAITS_, SaveStrategy, false>:
  public GenericLogSaverImplBase<LOG_TYPE_TRAITS_, SaveStrategy>
{
private:
  typedef GenericLogSaverImplBase<LOG_TYPE_TRAITS_, SaveStrategy> Base;

public:
  GenericLogSaverImpl(
    typename Base::CollectorT& collector,
    const std::string& path)
    : Base(path),
      collector_(collector)
  {}

  void
  save();

private:
  virtual
  ~GenericLogSaverImpl() noexcept {}

  typename Base::CollectorT& collector_;
};

template <class LogTraits, class SaveStrategy>
class GenericLogSaverImpl<LogTraits, SaveStrategy, true>:
  public LogSaverBaseImpl<typename LogTraits::CollectorBundleType>,
  public GenericLogSaverImplBase<LogTraits, SaveStrategy>
{
  typedef LogSaverBaseImpl<typename LogTraits::CollectorBundleType> Base1;
  typedef GenericLogSaverImplBase<LogTraits, SaveStrategy> Base2;

public:
  typedef typename LogTraits::CollectorBundleType CollectorBundleType;
  typedef typename LogTraits::CollectorFilterType CollectorFilterT;

  GenericLogSaverImpl(
    const std::string& path,
    CollectorFilterT* collector_filter)
    : Base1(collector_filter),
      Base2(path)
  {}

  virtual void
  save(const typename Base1::Spillover_var& data);

private:
  virtual
  ~GenericLogSaverImpl() noexcept {}
};

template <class LOG_TYPE_TRAITS_, class SaveStrategy, class DistributeStrategy>
class DistribLogSaverImpl<LOG_TYPE_TRAITS_, SaveStrategy, DistributeStrategy, false>:
  public GenericLogSaverImplBase<LOG_TYPE_TRAITS_, SaveStrategy>
{
private:
  typedef GenericLogSaverImplBase<LOG_TYPE_TRAITS_, SaveStrategy> Base;

public:
  DistribLogSaverImpl(
    typename Base::CollectorT& collector,
    const std::string& path,
    unsigned long distrib_count)
    : Base(path, distrib_count),
      collector_(collector)
  {}

  void
  save();

private:
  virtual
  ~DistribLogSaverImpl() noexcept {}

  typename Base::CollectorT& collector_;
};

template <class LOG_TYPE_TRAITS_, class SaveStrategy, class DistributeStrategy>
class DistribLogSaverImpl<LOG_TYPE_TRAITS_, SaveStrategy, DistributeStrategy, true>:
  public LogSaverBaseImpl<typename LOG_TYPE_TRAITS_::CollectorBundleType>,
  public GenericLogSaverImplBase<LOG_TYPE_TRAITS_, SaveStrategy>
{
  typedef LogSaverBaseImpl<typename LOG_TYPE_TRAITS_::CollectorBundleType> Base1;
  typedef GenericLogSaverImplBase<LOG_TYPE_TRAITS_, SaveStrategy> Base2;

public:
  typedef typename LOG_TYPE_TRAITS_::CollectorBundleType CollectorBundleType;
  typedef typename LOG_TYPE_TRAITS_::CollectorFilterType CollectorFilterT;

DistribLogSaverImpl(
  const std::string& path,
  CollectorFilterT* collector_filter,
  unsigned long distrib_count)
  : Base1(collector_filter),
    Base2(path, distrib_count)
  {}

  virtual void
  save(const typename Base1::Spillover_var& data);

private:
  virtual
  ~DistribLogSaverImpl() noexcept {}
};

} // namespace LogProcessing
} // namespace AdServer

#include "GenericLogIoImpl.tpp"

#endif /* AD_SERVER_LOG_PROCESSING_GENERIC_LOG_IO_IMPL_HPP */
