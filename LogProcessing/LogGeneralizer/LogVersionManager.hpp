#ifndef AD_SERVER_LOG_PROCESSING_LOG_VERSION_MANAGER_HPP
#define AD_SERVER_LOG_PROCESSING_LOG_VERSION_MANAGER_HPP


#include <map>
#include <set>
#include <numeric>
#include <algorithm>
#include <functional>
#include <Generics/Singleton.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Generics/HashTableAdapters.hpp>

#include "ThreadLogSaverImpl.hpp"

#include <LogCommons/LogCommons.hpp>
#include <LogCommons/CollectorBundle.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>
#include "LogTypeExtTraits.hpp"
#include "LogDbSaverImpl.hpp"
#include "DbConnectionFactory.hpp"
#include "LogGeneralizerStatDef.hpp"

namespace AdServer {
namespace LogProcessing {

class CollectorBundleProxy: public ReferenceCounting::DefaultImpl<>
{
public:

  virtual bool
  file_list_is_empty() const noexcept = 0;

  virtual Generics::Time
  get_min_file_timestamp() const /*throw(InvalidLogFileNameFormat)*/ = 0;

protected:
  virtual ~CollectorBundleProxy() noexcept {}
};

typedef ReferenceCounting::SmartPtr<CollectorBundleProxy>
  CollectorBundleProxy_var;

template <class BUNDLE_PTR_TYPE_>
class CollectorBundleProxyImpl: public CollectorBundleProxy
{
public:
  CollectorBundleProxyImpl(const BUNDLE_PTR_TYPE_ &bundle) :
    bundle_(bundle)
  {}

  virtual bool
  file_list_is_empty() const noexcept
  {
    return bundle_->file_list_is_empty();
  }

  virtual Generics::Time
  get_min_file_timestamp() const /*throw(InvalidLogFileNameFormat)*/
  {
    return bundle_->get_min_file_timestamp();
  }

private:
  virtual
  ~CollectorBundleProxyImpl() noexcept {}

  BUNDLE_PTR_TYPE_ bundle_;
};

class BundleProxyManager
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  void
  add_proxy(const std::string &version, const CollectorBundleProxy_var &proxy)
  {
    proxy_map_[version] = proxy;
    proxy_set_.insert(proxy);
  }

  bool
  file_list_is_empty() const noexcept
  {
    for (CollectorBundleProxySet_::const_iterator it = proxy_set_.begin();
      it != proxy_set_.end(); ++it)
    {
      if (!(*it)->file_list_is_empty())
      {
        return false;
      }
    }
    return true;
  }

  Generics::Time
  get_min_file_timestamp() const /*throw(InvalidLogFileNameFormat)*/
  {
    Generics::Time result;
    for (CollectorBundleProxySet_::const_iterator it = proxy_set_.begin();
      it != proxy_set_.end(); ++it)
    {
      const Generics::Time& ts = (*it)->get_min_file_timestamp();
      if (result == Generics::Time::ZERO)
      {
        result = ts;
      }
      else
      {
        if (ts != Generics::Time::ZERO)
        {
          result = std::min(result, ts);
        }
      }
    }
    return result;
  }

  const CollectorBundleProxy_var&
  get_proxy(const std::string &version) const
  {
    CollectorBundleProxyMap_::const_iterator it = proxy_map_.find(version);
    if (it == proxy_map_.end())
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Bad version: " << version;
      throw Exception(es);
    }
    return it->second;
  }

private:
  typedef Generics::GnuHashTable<
            Generics::StringHashAdapter, CollectorBundleProxy_var
          > CollectorBundleProxyMap_;

  typedef std::set<CollectorBundleProxy_var> CollectorBundleProxySet_;

  CollectorBundleProxyMap_ proxy_map_;
  CollectorBundleProxySet_ proxy_set_;
};


namespace Aux_ {

class ConversionFacility
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef std::pair<std::string, std::string> KeyT;

  struct ConvData
  {
    LogLoader_var loader;
    CollectorBundleProxy_var proxy;
    CollectorBundleProxy_var proxy2;
  };

  void register_conversion(const KeyT &key, const ConvData &conv_data)
  {
    conv_data_map_[key] = conv_data;
  }

  const ConvData& get_conversion(const KeyT &key) const
  {
    ConvDataMapT::const_iterator it = conv_data_map_.find(key);
    if (it == conv_data_map_.end())
    {
      std::ostringstream oss;
      oss << __PRETTY_FUNCTION__ << ": Unknown conversion requested: [ "
        << key.first << " -> " << key.second << " ]";
      throw Exception(oss.str());
    }
    return it->second;
  }

private:
  typedef std::map<KeyT, ConvData> ConvDataMapT;

  ConvDataMapT conv_data_map_;
};

typedef ::Generics::Singleton<ConversionFacility> ConversionFacilitySingleton;

template <typename FromLogTraits, typename ToLogTraits>
LOG_CONV_RESULT
ConvertAllBelow_2_6<FromLogTraits, ToLogTraits>::make_conversions(
  std::istream& is,
  const CollectorBundleFileGuard_var& file_handle,
  const std::string& version)
  /*throw(eh::Exception)*/
{
  if (version < "2.6")
  {
    ConversionFacility &conv_facility =
      ConversionFacilitySingleton::instance();

    ConversionFacility::KeyT
      conv_type(std::string(FromLogTraits::log_base_name()) + " V" + version,
        ToLogTraits::log_base_name());

    const ConversionFacility::ConvData &conv_data =
      conv_facility.get_conversion(conv_type);

    std::ios::pos_type pos = is.tellg();

    conv_data.loader->load(is, file_handle);

    is.clear();
    is.seekg(pos);

    return LCR_PARTIAL; // needs additional processing
  }
  return LCR_NIL;
}

template <typename FromLogTraits, typename ToLogTraits>
LOG_CONV_RESULT
ConversionToOtherType<FromLogTraits, ToLogTraits>::make_conversions(
  std::istream& is,
  const CollectorBundleFileGuard_var& file_handle,
  const std::string& version)
  /*throw(eh::Exception)*/
{
  ConversionFacility &conv_facility =
    ConversionFacilitySingleton::instance();

  ConversionFacility::KeyT
    conv_type(std::string(FromLogTraits::log_base_name()) + " V" + version,
      ToLogTraits::log_base_name());

  const ConversionFacility::ConvData &conv_data =
    conv_facility.get_conversion(conv_type);

  conv_data.loader->load(is, file_handle);

  return LCR_OK; // needs no additional processing
}

} // namespace Aux_

inline
LOG_CONV_RESULT
ColoUsersExtTraits::make_conversions(
  std::istream& is,
  const CollectorBundleFileGuard_var& file_handle,
  const std::string& /*version*/)
  /*throw(eh::Exception)*/
{
  Aux_::ConversionFacility &conv_facility =
    Aux_::ConversionFacilitySingleton::instance();

  Aux_::ConversionFacility::KeyT
    conv_type(log_base_name(),
      std::string(ColoUserStatTraits::log_base_name()) + "/" +
        GlobalColoUserStatTraits::log_base_name());

  const Aux_::ConversionFacility::ConvData &conv_data =
    conv_facility.get_conversion(conv_type);

  conv_data.loader->load(is, file_handle);
  // TODO: Add support for shared filenames?

  return LCR_OK; // needs no additional processing
}

template <typename LogVersionManager>
void
CampaignUserStatExtTraits::add_conversion_support(
  LogVersionManager* log_version_manager)
  /*throw(eh::Exception)*/
{
  // For old (V1.1) CampaignStat logs
  log_version_manager->template add_conversion<
    typename ConvTraits::CollectorType>(
      ConvTraits::current_version());
  // For old (V1.0) CampaignStat logs
  log_version_manager->template add_conversion<CampaignStatCollector_V_1_0>(
    "1.0");
}

template <typename LogVersionManager>
void
SiteUserStatExtTraits::add_conversion_support(
  LogVersionManager* log_version_manager)
  /*throw(eh::Exception)*/
{
  log_version_manager->template add_conversion<
    typename ConvTraits::CollectorType>(
      ConvTraits::current_version());
}

namespace Aux_
{

template <typename LogVersionManager>
class ConversionAdder
{
public:
  ConversionAdder(LogVersionManager* log_version_manager) noexcept
    : log_version_manager_(log_version_manager)
  {}

  template <typename OldCollector>
  void
  operator ()(const char* old_version) /*throw(eh::Exception)*/
  {
    log_version_manager_->template add_conversion<
      OldCollector>(old_version);
  }
private:
  LogVersionManager* log_version_manager_;
};

template <typename LogVersionManager>
void
CcgCcStatToCcgCcUserStatConversion::add_conversion_support(
  LogVersionManager* log_version_manager)
  /*throw(eh::Exception)*/
{
  // For old CCGStat logs
  ConversionAdder<LogVersionManager> convert_versions(
    log_version_manager);
  LogVersionManager::Traits::ConvTraits::for_each_old(
    convert_versions);
}

template <typename LogVersionManager>
void
ColoUsers_To_Colo_Or_GlobalColo_UserStatConvertion::add_conversion_support(
  LogVersionManager* log_version_manager)
  /*throw(eh::Exception)*/
{
  typedef typename LogVersionManager::Traits LogTraits;
  typedef typename LogTraits::ConvTraits ConvTraits;

  Aux_::ConversionFacility &conv_facility =
    Aux_::ConversionFacilitySingleton::instance();

  typedef ColoUsers_To_Colo_Or_GlobalColo_UserStatLoader ConvLoaderT;

  Aux_::ConversionFacility::KeyT
    conv_type(ConvTraits::log_base_name(),
      std::string(ColoUserStatTraits::log_base_name()) + "/" +
        GlobalColoUserStatTraits::log_base_name());
  try
  {
    const Aux_::ConversionFacility::ConvData &conv_data =
      conv_facility.get_conversion(conv_type);

    ConvLoaderT *conv_loader = 0;

    if ((conv_loader = dynamic_cast<ConvLoaderT*>(conv_data.loader.in())))
    {
      conv_loader->set_bundle(log_version_manager->get_bundle());
    }
    else
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Registered conversion log loader has "
        "invalid type (dynamic_cast<> failed)";
      throw typename LogVersionManager::Exception(es);
    }

    Aux_::ConversionFacility::ConvData upd_conv_data = conv_data;
    upd_conv_data.proxy2 =
      log_version_manager->get_proxy(LogTraits::current_version());
    conv_facility.register_conversion(conv_type, upd_conv_data);
  }
  catch (const Aux_::ConversionFacility::Exception&)
  {
    Aux_::ConversionFacility::ConvData conv_data;

    ConvLoaderT *conv_loader = new ConvLoaderT;

    conv_loader->set_bundle(log_version_manager->get_bundle());

    conv_data.loader = conv_loader;
    conv_data.proxy =
      log_version_manager->get_proxy(LogTraits::current_version());

    conv_facility.register_conversion(conv_type, conv_data);
  }
}

} // namespace Aux_

template <typename LogExtTraits>
class LogVersionManagerBaseImpl:
  protected LoadVersionsFacility,
  public BundleProxyManager
{
public:
  typedef typename BundleProxyManager::Exception Exception;
  typedef LogExtTraits Traits;

  void
  load(std::istream& is, const CollectorBundleFileGuard_var& file_handle)
  {
    LogExtTraits::other_logs_conversion(file_handle) ?
      load_<typename LogExtTraits::template
        ConvTraitsSelector<LogExtTraits>::Type>(is, file_handle) :
      load_<LogExtTraits>(is, file_handle);
  }

  const typename LogExtTraits::CollectorBundlePtrType&
  get_bundle() noexcept
  {
    return bundle_;
  }

  template <typename OldCollector>
  void
  add_conversion(const char* version) /*throw(eh::Exception)*/
  {
    using namespace Aux_;
    typedef typename LogExtTraits::ConvTraits SourceTraits;

    ConversionFacility::KeyT
      conv_type(std::string(SourceTraits::log_base_name()) + " V" + version,
        LogExtTraits::log_base_name());
    ConversionFacility::ConvData conv_data;
    conv_data.loader =
      LogExtTraits::template make_conversion_loader<OldCollector>(bundle_);
    conv_data.proxy = get_proxy(LogExtTraits::current_version());
    ConversionFacilitySingleton::instance().register_conversion(
      conv_type, conv_data);
  }

protected:
  template <typename LogTraits>
  void
  load_(std::istream& is, const CollectorBundleFileGuard_var& file_handle)
    /*throw(eh::Exception)*/
  {
    typename LogTraits::HeaderType log_header;
    if (!(is >> log_header))
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << ": Read error or invalid log header!";
      throw Exception(es);
    }
    const std::string& version = log_header.version();
    LOG_CONV_RESULT result =
      LogTraits::make_conversions(is, file_handle, version);
    switch (result)
    {
      case LCR_PARTIAL:
      case LCR_NIL: // LCR_NIL means that no conversion was performed
        load_impl_(version, is, file_handle);
      case LCR_OK:
        return;
      default:
        Stream::Error es;
        es << FNT << "Unknown conversion result code = " << result;
        throw Exception(es);
    }
  }

  // Simply interface to add_previous_version_support
  class PreviousVersionAdder
  {
    typedef LogVersionManagerBaseImpl<LogExtTraits> LvmBaseImplType;

  public:
    PreviousVersionAdder(LvmBaseImplType* manager) noexcept
    :
      log_version_manager_(manager)
    {
    }

    template <class OLD_COLLECTOR_, bool use_packed_loader>
    void
    operator()(const char* old_version) /*throw(eh::Exception)*/
    {
      typedef GenericLogLoaderImpl<
        LogExtTraits,
        typename Detail::BundleLoad<
          LogExtTraits,
          typename Detail::OldLoadActionSelector<
            OLD_COLLECTOR_, LogExtTraits::is_nested,
            LogExtTraits::is_summable, use_packed_loader
          >::SafeLoadType
        >
      > CompatLoaderDefault;

      log_version_manager_->add_previous_version_support<
        typename LogExtTraits::template CompatLoaderSelector<
          CompatLoaderDefault
        >::Type
      >(old_version);
    }

    template <class OLD_COLLECTOR_>
    void
    operator()(const char* version) /*throw(eh::Exception)*/
    {
      operator()<OLD_COLLECTOR_, false>(version);
    }

    template <class OLD_COLLECTOR_, bool is_old_nested>
    void
    support(const char* old_version) /*throw(eh::Exception)*/
    {
      typedef GenericLogLoaderImpl<
        LogExtTraits,
        typename Detail::BundleLoad<
          LogExtTraits,
          Detail::OldSafeLoad<LogExtTraits, OLD_COLLECTOR_, is_old_nested>
        >
      > CompatLoaderDefault;

      log_version_manager_->add_previous_version_support<
        typename LogExtTraits::template CompatLoaderSelector<
          CompatLoaderDefault
        >::Type
      >(old_version);
    }

  protected:
    LvmBaseImplType* log_version_manager_;
  };

  template <typename OtherLogExtTraits, typename SaverType>
  void
  add_version_support_(
    const LogProcThreadInfo_var& context,
    typename OtherLogExtTraits::CollectorBundlePtrType& bundle,
    const CollectorBundleParams& bundle_params,
    SaverType* saver)
    /*throw(eh::Exception)*/
  {
    typename ThreadLogSaverImpl<OtherLogExtTraits, SaverType>::SmartPtr
       thread_saver(
      new ThreadLogSaverImpl<OtherLogExtTraits, SaverType>(context, typename SaverType::SmartPtr(saver)));

    bundle = typename OtherLogExtTraits::CollectorBundlePtrType(
      new typename OtherLogExtTraits::CollectorBundleType(
        bundle_params, thread_saver));

    add_loader_(OtherLogExtTraits::current_version(),
      new typename OtherLogExtTraits::ThreadSafeLoaderType(bundle));

    typedef typename OtherLogExtTraits::CollectorBundlePtrType BundlePtrT;

    CollectorBundleProxy_var proxy =
      new CollectorBundleProxyImpl<BundlePtrT>(bundle);

    add_proxy(OtherLogExtTraits::current_version(), proxy);
  }

  template <typename LoaderType>
  void
  add_previous_version_support(const std::string& version)
    /*throw(eh::Exception)*/
  {
    add_loader_(version, new LoaderType(bundle_));

    add_proxy(version, get_proxy(LogExtTraits::current_version()));
  }

  typename LogExtTraits::CollectorBundlePtrType bundle_;
};

//No DB mode
template <class LOG_TYPE_EXT_TRAITS_>
class LogVersionManager:
  public LogVersionManagerBaseImpl<LOG_TYPE_EXT_TRAITS_>
{
  typedef LogVersionManagerBaseImpl<LOG_TYPE_EXT_TRAITS_> BaseType;

  void
  add_current_version_support(
    const LogProcThreadInfo_var& context,
    const CollectorBundleParams& bundle_params,
    const std::string& dest)
  {
    if (context->DISTRIB_COUNT > 1)
    {
      typedef typename LOG_TYPE_EXT_TRAITS_::DistribThreadSafeSaverType SaverType;

      SaverType* saver(new SaverType(dest,
        typename LOG_TYPE_EXT_TRAITS_::CollectorFilterType::SmartPtr(
          new typename LOG_TYPE_EXT_TRAITS_::CollectorFilterType),
        context->DISTRIB_COUNT));

      BaseType::template add_version_support_<LOG_TYPE_EXT_TRAITS_, SaverType>(
        context, BaseType::bundle_, bundle_params, saver);
    }
    else
    {
      typedef typename LOG_TYPE_EXT_TRAITS_::ThreadSafeSaverType SaverType;

      SaverType* saver(new SaverType(dest,
        typename LOG_TYPE_EXT_TRAITS_::CollectorFilterType::SmartPtr(
          new typename LOG_TYPE_EXT_TRAITS_::CollectorFilterType)));

      BaseType::template add_version_support_<LOG_TYPE_EXT_TRAITS_, SaverType>(
        context, BaseType::bundle_, bundle_params, saver);
    }
  }

public:
  typedef typename BaseType::Traits Traits;

  LogVersionManager(
    const LogProcThreadInfo_var& context,
    const std::string& dest,
    const CollectorBundleParams& bundle_params
  )
    /*throw(eh::Exception)*/
  {
    add_current_version_support(context, bundle_params, dest);
    typename BaseType::PreviousVersionAdder add_versions(this);
    Traits::for_each_old(add_versions);
    Traits::add_conversion_support(this);
  }
};

// For deferrable log types
template <class Traits>
class LogVersionManager2 : public LogVersionManagerBaseImpl<Traits>
{
  typedef LogVersionManagerBaseImpl<Traits> BaseType;

  typedef typename BaseType::PreviousVersionAdder PreviousVersionAdder;

  /// For deferred logs
  void
  add_current_version_support(
    const LogProcThreadInfo_var& context,
    const CollectorBundleParams &bundle_params,
    const LogGeneralizerStatMapBundle_var& lgsm_bundle,
    const PostgresConnectionFactoryImpl_var &pg_conn_factory
  )
  {
    typename Traits::DbSaverType::SmartPtr saver(
      new typename Traits::DbSaverType(pg_conn_factory, lgsm_bundle,
        typename Traits::CollectorFilterType::SmartPtr(
        new typename Traits::CollectorFilterType)));

    typename ThreadLogSaverImpl<Traits, typename Traits::DbSaverType>::SmartPtr
      thread_saver(new ThreadLogSaverImpl<Traits,
        typename Traits::DbSaverType>(context, saver));

    BaseType::bundle_ =
      new typename Traits::CollectorBundleType(bundle_params, thread_saver);

    BaseType::add_loader_(Traits::current_version(),
      new typename Traits::ThreadSafeLoaderType(BaseType::bundle_));

    typedef typename Traits::CollectorBundlePtrType BundlePtrT;

    CollectorBundleProxy_var proxy =
      new CollectorBundleProxyImpl<BundlePtrT>(BaseType::bundle_);

    BaseType::add_proxy(Traits::current_version(), proxy);
  }

public:
  LogVersionManager2(
    const LogProcThreadInfo_var& context,
    const std::string &/* dest */,
    const LogGeneralizerStatMapBundle_var& lgsm_bundle,
    const PostgresConnectionFactoryImpl_var &pg_conn_factory,
    const CollectorBundleParams &bundle_params
  )
  {
    add_current_version_support(
      context, bundle_params, lgsm_bundle, pg_conn_factory);

    PreviousVersionAdder add_versions(this);
    Traits::for_each_old(add_versions);
    Traits::add_conversion_support(this);
  }
};

// For deferrable log types - both Postres and CSV output
template <class Traits>
class LogVersionManager9 : public LogVersionManagerBaseImpl<Traits>
{
  typedef LogVersionManagerBaseImpl<Traits> BaseType;

  typedef typename BaseType::PreviousVersionAdder PreviousVersionAdder;

  /// For deferred logs
  void
  add_current_version_support(
    const LogProcThreadInfo_var& context,
    const CollectorBundleParams& bundle_params,
    const std::string& dest,
    const LogGeneralizerStatMapBundle_var& lgsm_bundle,
    const PostgresConnectionFactoryImpl_var& pg_conn_factory
  )
  {
    typename Traits::PgCsvSaverType::SmartPtr saver(
      new typename Traits::PgCsvSaverType(dest, pg_conn_factory,
        lgsm_bundle,
        typename Traits::CollectorFilterType::SmartPtr(
        new typename Traits::CollectorFilterType)));

    typename
      ThreadLogSaverImpl<Traits, typename Traits::PgCsvSaverType>::SmartPtr
        thread_saver(new ThreadLogSaverImpl<Traits,
          typename Traits::PgCsvSaverType>(context, saver));

    BaseType::bundle_ =
      new typename Traits::CollectorBundleType(bundle_params, thread_saver);

    BaseType::add_loader_(Traits::current_version(),
      new typename Traits::ThreadSafeLoaderType(BaseType::bundle_));

    typedef typename Traits::CollectorBundlePtrType BundlePtrT;

    CollectorBundleProxy_var proxy =
      new CollectorBundleProxyImpl<BundlePtrT>(BaseType::bundle_);

    BaseType::add_proxy(Traits::current_version(), proxy);
  }

public:

  LogVersionManager9(
    const LogProcThreadInfo_var& context,
    const std::string& dest,
    const LogGeneralizerStatMapBundle_var& lgsm_bundle,
    const PostgresConnectionFactoryImpl_var& pg_conn_factory,
    const CollectorBundleParams& bundle_params
  )
  {
    add_current_version_support(context, bundle_params, dest, lgsm_bundle,
      pg_conn_factory);

    PreviousVersionAdder add_versions(this);
    Traits::for_each_old(add_versions);
    Traits::add_conversion_support(this);
  }
};

// For CSV-only output
template <class Traits>
class LogVersionManager3 : public LogVersionManagerBaseImpl<Traits>
{
  typedef LogVersionManagerBaseImpl<Traits> BaseType;
  typedef typename BaseType::PreviousVersionAdder PreviousVersionAdder;

  struct CsvSaverTraits : public Traits
  {
    typedef typename Traits::CsvSaverType ThreadSafeSaverType;
  };

  void
  add_current_version_support(
    const LogProcThreadInfo_var& context,
    const CollectorBundleParams& bundle_params,
    const std::string& dest)
  {
    typedef typename CsvSaverTraits::ThreadSafeSaverType SaverType;

    SaverType* saver(new SaverType(dest,
      typename Traits::CollectorFilterType::SmartPtr(
        new typename Traits::CollectorFilterType)));

    BaseType::template add_version_support_<Traits, SaverType>(
      context, BaseType::bundle_, bundle_params, saver);
  }

public:
  LogVersionManager3(
    const LogProcThreadInfo_var& context,
    const std::string &dest,
    const CollectorBundleParams &bundle_params
  )
  {
    add_current_version_support(context, bundle_params, dest);

    PreviousVersionAdder add_versions(this);
    CsvSaverTraits::for_each_old(add_versions);
    CsvSaverTraits::add_conversion_support(this);
  }
};

// No DB - Use hits filtering
template <class Traits>
class LogVersionManager7: public LogVersionManagerBaseImpl<Traits>
{
  typedef LogVersionManagerBaseImpl<Traits> BaseType;
  typedef typename BaseType::PreviousVersionAdder PreviousVersionAdder;

  void
  add_current_version_support(
    const LogProcThreadInfo_var& context,
    const CollectorBundleParams& bundle_params,
    const std::string& dest,
    HitsFilter_var& hits_filter)
  {
    if (context->DISTRIB_COUNT > 1)
    {
      typedef typename Traits::DistribThreadSafeSaverType SaverType;

      SaverType* saver(new SaverType(dest,
        typename Traits::CollectorFilterType::SmartPtr(
          new typename Traits::CollectorFilterType(hits_filter)),
        context->DISTRIB_COUNT));

      BaseType::template add_version_support_<Traits, SaverType>(
        context, BaseType::bundle_, bundle_params, saver);
    }
    else
    {
      typedef typename Traits::ThreadSafeSaverType SaverType;

      SaverType* saver(new SaverType(dest,
        typename Traits::CollectorFilterType::SmartPtr(
          new typename Traits::CollectorFilterType(hits_filter))));

      BaseType::template add_version_support_<Traits, SaverType>(
        context, BaseType::bundle_, bundle_params, saver);
    }
  }

public:
  LogVersionManager7(
    const LogProcThreadInfo_var& context,
    const std::string& dest,
    const CollectorBundleParams& bundle_params,
    HitsFilter_var& hits_filter
  )
  {
    add_current_version_support(context, bundle_params, dest, hits_filter);

    PreviousVersionAdder add_versions(this);
    Traits::for_each_old(add_versions);
    Traits::add_conversion_support(this);
  }
};

// For CSV-only output (with hits filtering)
template <class Traits>
class LogVersionManager8: public LogVersionManagerBaseImpl<Traits>
{
  typedef LogVersionManagerBaseImpl<Traits> BaseType;
  typedef typename BaseType::PreviousVersionAdder PreviousVersionAdder;

  void
  add_current_version_support(
    const LogProcThreadInfo_var& context,
    const CollectorBundleParams& bundle_params,
    const std::string& dest,
    HitsFilter_var& hits_filter
  )
  {
    typedef typename Traits::CsvSaverType SaverType;
    typename SaverType::SmartPtr saver = new SaverType(
      dest, typename Traits::CollectorFilterType::SmartPtr(
        new typename Traits::CollectorFilterType(hits_filter)));

    typename ThreadLogSaverImpl<Traits, SaverType>::SmartPtr
       thread_saver(
      new ThreadLogSaverImpl<Traits, SaverType>(context, saver));

    BaseType::bundle_ =
      new typename Traits::CollectorBundleType(
        bundle_params,
        thread_saver);

    BaseType::add_loader_(Traits::current_version(),
      new typename Traits::ThreadSafeLoaderType(BaseType::bundle_));

    CollectorBundleProxy_var proxy =
      new CollectorBundleProxyImpl<
        typename Traits::CollectorBundlePtrType
      >(BaseType::bundle_);

    BaseType::add_proxy(Traits::current_version(), proxy);
  }

public:
  LogVersionManager8(
    const LogProcThreadInfo_var& context,
    const std::string& dest,
    const CollectorBundleParams& bundle_params,
    HitsFilter_var& hits_filter
  )
  {
    add_current_version_support(context, bundle_params, dest, hits_filter);

    PreviousVersionAdder add_versions(this);
    Traits::for_each_old(add_versions);
    Traits::add_conversion_support(this);
  }
};

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_LOG_VERSION_MANAGER_HPP */

