#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <eh/Exception.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Time.hpp>
#include <String/RegEx.hpp>

#include <Commons/HostDistribution.hpp>
#include "Utils.hpp"

namespace AdServer::LogProcessing
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

    struct Destination
    {
      explicit Destination(std::string host_val = std::string())
        : host(std::move(host_val)),
          recovery_generation(0),
          probe(false)
      {}

      std::string host;
      std::uint64_t recovery_generation;
      bool probe;
    };

    RouteBasicHelper(
      SchedType feed_type,
      unsigned long host_check_period)
      noexcept;

    Destination
    get_destination(const char* src_file)
      /*throw(NotAvailable, NotReady)*/;

    virtual std::string
    get_dest_host(const char* src_file)
      /*throw(NotAvailable, NotReady)*/ = 0;

    virtual void bad_host(const Destination& destination) noexcept;

    void good_host(const Destination& destination) noexcept;

    SchedType feed_type() const noexcept;

    unsigned long host_check_period() const noexcept;

  protected:
    virtual ~RouteBasicHelper() noexcept {}

  private:
    struct HostState
    {
      HostState()
        : unavailable(false),
          probing(false),
          recovery_generation(0),
          retry_at(0)
      {}

      bool unavailable;
      bool probing;
      std::uint64_t recovery_generation;
      Generics::Time retry_at;
    };

    using HostStateMap = std::map<std::string, HostState>;

  protected:
    const SchedType feed_type_;

  private:
    const unsigned long host_check_period_;
    std::mutex host_states_lock_;
    HostStateMap host_states_;
    std::atomic<std::size_t> unavailable_host_count_;
    std::atomic<std::uint64_t> recovery_generation_;
  };

  using RouteBasicHelper_var = ReferenceCounting::QualPtr<RouteBasicHelper>;
  using FixedRouteBasicHelper_var = ReferenceCounting::FixedPtr<RouteBasicHelper>;

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

    virtual void bad_host(const Destination& destination) noexcept;

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
  };

  class RouteByNumberHelper:
    public RouteBasicHelper,
    public ReferenceCounting::AtomicImpl
  {
  public:
    RouteByNumberHelper(
      SchedType feed_type,
      const StringList& dst_hosts,
      unsigned long host_check_period)
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
      const char* src_file_name_regexp,
      unsigned long host_check_period)
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
      const Generics::Time& distr_reload_period,
      unsigned long host_check_period)
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
      const char* src_file_name_regexp,
      unsigned long host_check_period)
      noexcept;

    virtual std::string
    get_dest_host(const char* src_file) noexcept;

  protected:
    virtual ~RouteHostFromFileNameHelper() noexcept {};

  private:
    const String::RegEx src_file_name_regexp_;
  };
}

namespace AdServer::LogProcessing
{
  inline
  RouteBasicHelper::RouteBasicHelper(
    SchedType feed_type,
    unsigned long host_check_period)
    noexcept
    : feed_type_(feed_type),
      host_check_period_(host_check_period),
      unavailable_host_count_(0),
      recovery_generation_(0)
  {
  }

  inline
  SchedType RouteBasicHelper::feed_type() const noexcept
  {
    return feed_type_;
  }

  inline
  unsigned long RouteBasicHelper::host_check_period() const noexcept
  {
    return host_check_period_;
  }

}
