#include <iostream> // TO REMOVE
#include <Stream/MemoryStream.hpp>
#include <String/TextTemplate.hpp>
#include <Generics/DirSelector.hpp>

#include "FileRouter.hpp"
#include "RouteHelpers.hpp"

namespace AdServer
{
namespace LogProcessing
{
  namespace
  {
    std::string EMPTY_STRING;
  }

  //
  // RouteBasicHelper
  //
  void RouteBasicHelper::bad_host(const std::string& /*host*/) noexcept
  {}

  //
  // RouteRoundRobinHelper
  //
  void
  RouteRoundRobinHelper::fill_dst_hosts_(const StringList& dest_hosts)
    /*throw(Exception)*/
  {
    if(dest_hosts.empty())
    {
      throw Exception("List of destination hosts is empty.");
    }
    for(StringList::const_iterator it = dest_hosts.begin();
        it != dest_hosts.end(); ++it)
    {
      dst_map_.insert(std::make_pair(*it, DestHost()));
    }
  }

  void
  RouteRoundRobinHelper::bad_host(const std::string& host) noexcept
  {
    DestMap::iterator fnd = dst_map_.find(host);
    if(fnd != dst_map_.end())
    {
      Generics::Time now = Generics::Time::get_time_of_day();

      SyncPolicy::WriteGuard guard(lock_);
      fnd->second.available = false;
      fnd->second.last_check_time = now;
    }
  }

  RouteRoundRobinHelper::RouteRoundRobinHelper(
    SchedType feed_type,
    const StringList& dst_hosts,
    unsigned long host_check_period)
    /*throw(Exception)*/
    : RouteBasicHelper(feed_type),
      host_check_period_(host_check_period)
  {
    fill_dst_hosts_(dst_hosts);
    dst_it_ = dst_map_.begin();
  }

  std::string
  RouteRoundRobinHelper::get_dest_host(
    const char* src_file)
    /*throw(NotAvailable)*/
  {
    Generics::Time cur_time;

    {
      SyncPolicy::WriteGuard guard(lock_);

      for(size_t i = 0; i < dst_map_.size(); i++)
      {
        if(!dst_it_->second.available)
        {
          if(i == 0)
          {
            cur_time = Generics::Time::get_time_of_day();
          }

          if(dst_it_->second.last_check_time + host_check_period_ <= cur_time)
          {
            dst_it_->second.available = true;
          }
        }

        DestMap::const_iterator res = dst_it_;
        if(++dst_it_ == dst_map_.end())
        {
          dst_it_ = dst_map_.begin();
        }

        if(res->second.available)
        {
          return res->first;
        }
      }
    }

    Stream::Error ostr;
    ostr << "no available hosts for moving file '" << src_file << "'.";
    throw NotAvailable(ostr);
  }

  //
  // RouteByNumberHelper
  //
  RouteByNumberHelper::RouteByNumberHelper(
    SchedType feed_type,
    const StringList& dst_hosts)
    /*throw(Exception)*/
    : RouteBasicHelper(feed_type)
  {
    std::copy(
      dst_hosts.begin(),
      dst_hosts.end(),
      std::back_insert_iterator<DestHosts>(dst_hosts_));
  }

  std::string
  RouteByNumberHelper::get_dest_host(const char* src_file)
    /*throw(NotAvailable)*/
  {
    try
    {
      size_t host_num = Utils::find_host_num(src_file, dst_hosts_.size());
      return dst_hosts_[host_num];
    }
    catch (eh::Exception& ex)
    {
      throw NotAvailable(ex.what());
    }
  }

  //
  // FileHashDeterminer
  //
  FileHashDeterminer::FileHashDeterminer(
    const char* file_name_regexp)
    /*throw(eh::Exception)*/
    : src_file_name_regexp_(init_hash_regexp_(file_name_regexp))
  {}

  bool
  FileHashDeterminer::get_hash(
    unsigned long& hash,
    const char* src_file) const
    /*throw(RouteBasicHelper::Exception)*/
  {
    static const char* FUN = "FileHashDeterminer::get_hash()";

    try
    {
      const char* file_name = Generics::DirSelect::file_name(src_file);

      String::RegEx::Result match_result;
      if(src_file_name_regexp_.search(match_result, String::SubString(file_name)) &&
        match_result.size() > 1)
      {
        return String::StringManip::str_to_int(match_result[1], hash);
      }

      return false;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw RouteBasicHelper::Exception(ostr);
    }
  }

  std::string
  FileHashDeterminer::init_hash_regexp_(const char* hash_pattern)
    /*throw(RouteBasicHelper::Exception)*/
  {
    static const char* FUN = "FileHashDeterminer::init_hash_regexp_()";

    try
    {
      String::TextTemplate::Args templ_args;
      templ_args[TemplateParams::HASH] = "(\\d*)";
      Stream::Parser istr(hash_pattern);
      String::TextTemplate::IStream templ(
        istr,
        TemplateParams::MARKER,
        TemplateParams::MARKER);

      return templ.instantiate(templ_args);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw RouteBasicHelper::Exception(ostr);
    }
  }

  //
  // RouteHashHelper
  //
  RouteHashHelper::RouteHashHelper(
    SchedType feed_type,
    const StringList& dst_hosts,
    const char* src_file_name_regexp)
    /*throw(Exception)*/
    : RouteByNumberHelper(feed_type, dst_hosts),
      FileHashDeterminer(src_file_name_regexp)
  {}

  std::string
  RouteHashHelper::get_dest_host(const char* src_file)
    /*throw(NotAvailable)*/
  {
    try
    {
      unsigned long hash = 0;
      if(get_hash(hash, src_file))
      {
        return dst_hosts_[hash % dst_hosts_.size()];
      }
      else
      {
        return EMPTY_STRING;
      }
    }
    catch (const eh::Exception& ex)
    {
      throw NotAvailable(ex.what());
    }
  }

  //
  // RouteDefiniteHashHelper
  //
  RouteDefiniteHashHelper::RouteDefiniteHashHelper(
    SchedType feed_type,
    const char* config_file,
    const char* config_file_schema,
    const char* src_file_name_regexp,
    const Generics::Time& distr_reload_period)
    noexcept
    : RouteBasicHelper(feed_type),
      FileHashDeterminer(src_file_name_regexp),
      config_file_(config_file),
      config_file_schema_(config_file_schema),
      distr_reload_period_(distr_reload_period)
  {}

  std::string
  RouteDefiniteHashHelper::get_dest_host(const char* src_file)
    /*throw(NotReady)*/
  {
    Generics::Time now = Generics::Time::get_time_of_day();
    Commons::HostDistributionFile_var host_distr;

    bool reload_host_distr = false;

    {
      SyncPolicy::WriteGuard lock(host_distr_lock_);
      if(now > host_distr_load_time_ + distr_reload_period_)
      {
        reload_host_distr = true;
        host_distr_load_time_ = now;
      }
      else
      {
        host_distr = host_distr_;
      }
    }

    if(reload_host_distr)
    {
      try
      {
        host_distr = new Commons::HostDistributionFile(
          config_file_.c_str(),
          config_file_schema_.c_str());
      }
      catch(...)
      {}

      if(host_distr.in())
      {
        SyncPolicy::WriteGuard lock(host_distr_lock_);
        host_distr_ = host_distr;
      }
    }

    if(host_distr.in())
    {
      unsigned long hash;
      if(get_hash(hash, src_file))
      {
        return host_distr->get_host_by_index(
          hash % host_distr->get_index_limit());
      }
      else
      {
        return EMPTY_STRING;
      }
    }

    throw NotReady("");
  }

  //
  // RouteHostFromFileNameHelper
  //
  RouteHostFromFileNameHelper::RouteHostFromFileNameHelper(
    SchedType feed_type,
    const char* src_file_name_regexp)
    noexcept
    : RouteBasicHelper(feed_type),
      src_file_name_regexp_(std::string(src_file_name_regexp))
  {}

  std::string
  RouteHostFromFileNameHelper::get_dest_host(
    const char* src_file)
    noexcept
  {
    try
    {
      const char* file_name = Generics::DirSelect::file_name(src_file);
      String::RegEx::Result match_result;

      if(src_file_name_regexp_.search(
          match_result, String::SubString(file_name)) &&
         match_result.size() > 1)
      {
        return std::string(match_result[1].data(), match_result[1].length());
      }
    }
    catch(const eh::Exception& ex)
    {}

    return EMPTY_STRING;
  }
}
}

