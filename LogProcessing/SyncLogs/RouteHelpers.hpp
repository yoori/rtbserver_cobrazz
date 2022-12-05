#ifndef SYNC_LOGS_ROUTE_HELPERS_HPP
#define SYNC_LOGS_ROUTE_HELPERS_HPP

#include <string>
#include <vector>
#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>
#include <String/RegEx.hpp>

#include <Commons/HostDistribution.hpp>
#include "Utils.hpp"

namespace AdServer
{
namespace LogProcessing
{
  /** Information about host availability */
  struct DestHost
  {
    DestHost()
      : available(true),
        last_check_time(0)
    {}

    mutable bool available;
    mutable Generics::Time last_check_time;
  };

  enum SchedType
  {
    ST_BY_NUMBER,
    ST_ROUND_ROBIN,
    ST_HASH,
    ST_DEFINITEHASH,
    ST_FROM_FILE_NAME
  };

  class RouteBasicHelper: public virtual ReferenceCounting::Interface
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(NotAvailable, Exception);
    DECLARE_EXCEPTION(NotReady, Exception);

    RouteBasicHelper(SchedType feed_type)
      noexcept;

    virtual std::string
    get_dest_host(const char* src_file)
      /*throw(NotAvailable, NotReady)*/ = 0;

    virtual void bad_host(const std::string& host) noexcept;

    SchedType feed_type() const noexcept;

  protected:
    virtual ~RouteBasicHelper() noexcept {}

  protected:
    const SchedType feed_type_;
  };

  typedef ReferenceCounting::QualPtr<RouteBasicHelper>
    RouteBasicHelper_var;
  typedef ReferenceCounting::FixedPtr<RouteBasicHelper>
    FixedRouteBasicHelper_var;

  class RouteRoundRobinHelper:
    public RouteBasicHelper,
    public ReferenceCounting::AtomicImpl
  {
  public:
    RouteRoundRobinHelper(
      SchedType feed_type,
      const StringList& dst_hosts,
      unsigned long host_check_period)
      /*throw(Exception)*/;

    virtual std::string
    get_dest_host(const char* src_file)
      /*throw(NotAvailable)*/;

    virtual void bad_host(const std::string& host) noexcept;

  protected:
    virtual ~RouteRoundRobinHelper() noexcept {};

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;
    typedef std::map<std::string, DestHost> DestMap;

  private:
    void fill_dst_hosts_(const StringList& dst_hosts)
      /*throw(Exception)*/;

  private:
    mutable SyncPolicy::Mutex lock_;
    DestMap dst_map_;
    DestMap::const_iterator dst_it_;
    const unsigned long host_check_period_;
  };

  class RouteByNumberHelper:
    public RouteBasicHelper,
    public ReferenceCounting::AtomicImpl
  {
  public:
    RouteByNumberHelper(
      SchedType feed_type,
      const StringList& dst_hosts)
      /*throw(Exception)*/;

    virtual std::string
    get_dest_host(const char* src_file) /*throw(NotAvailable)*/;

  protected:
    typedef std::vector<std::string> DestHosts;

  protected:
    virtual ~RouteByNumberHelper() noexcept {};

  protected:
    DestHosts dst_hosts_;
  };

  class FileHashDeterminer
  {
  public:
    FileHashDeterminer(const char* file_name_regexp)
      /*throw(eh::Exception)*/;

    bool
    get_hash(unsigned long& hash, const char* src_file) const
      /*throw(RouteBasicHelper::Exception)*/;

  protected:
    static
    std::string
    init_hash_regexp_(const char* hash_pattern)
      /*throw(RouteBasicHelper::Exception)*/;

  private:
    const String::RegEx src_file_name_regexp_;
  };

  class RouteHashHelper:
    public RouteByNumberHelper,
    public FileHashDeterminer
  {
  public:
    RouteHashHelper(
      SchedType feed_type,
      const StringList& dst_hosts,
      const char* src_file_name_regexp)
      /*throw(Exception)*/;

    virtual std::string
    get_dest_host(const char* src_file) /*throw(NotAvailable)*/;

  protected:
    virtual ~RouteHashHelper() noexcept {};

  private:
    const String::RegEx src_file_name_regexp_;
  };

  class RouteDefiniteHashHelper:
    public RouteBasicHelper,
    public FileHashDeterminer,
    public ReferenceCounting::AtomicImpl
  {
  public:
    RouteDefiniteHashHelper(
      SchedType feed_type,
      const char* config_file,
      const char* config_file_schema,
      const char* src_file_name_regexp,
      const Generics::Time& distr_reload_period)
      noexcept;

    virtual std::string
    get_dest_host(const char* src_file) /*throw(NotReady)*/;

  protected:
    virtual ~RouteDefiniteHashHelper() noexcept {};

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;

  private:
    const std::string src_file_name_pattern_;
    const std::string config_file_;
    const std::string config_file_schema_;
    const Generics::Time distr_reload_period_;

    SyncPolicy::Mutex host_distr_lock_;
    Generics::Time host_distr_load_time_;
    Commons::HostDistributionFile_var host_distr_;
  };

  class RouteHostFromFileNameHelper:
    public RouteBasicHelper,
    public ReferenceCounting::AtomicImpl
  {
  public:
    RouteHostFromFileNameHelper(
      SchedType feed_type,
      const char* src_file_name_regexp) noexcept;

    virtual std::string
    get_dest_host(const char* src_file) noexcept;

  protected:
    virtual ~RouteHostFromFileNameHelper() noexcept {};

  private:
    const String::RegEx src_file_name_regexp_;
  };
}
}

namespace AdServer
{
namespace LogProcessing
{
  inline
  RouteBasicHelper::RouteBasicHelper(SchedType feed_type)
    noexcept
    : feed_type_(feed_type)
  {
  }

  inline
  SchedType RouteBasicHelper::feed_type() const noexcept
  {
    return feed_type_;
  }

}
}
#endif

