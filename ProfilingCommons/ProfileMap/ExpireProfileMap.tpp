/**
 * Expiry profiles mapping implementations
 * @file ProfilingCommons/ExpireProfileMap.tpp
 */

#ifndef PROFILING_COMMONS_EXPIRE_PROFILE_MAP_TPP
#define PROFILING_COMMONS_EXPIRE_PROFILE_MAP_TPP

#include <cassert>
#include <Generics/DirSelector.hpp>
#include <String/RegEx.hpp>

namespace
{
  const char LOG_RE_TIME_FORMAT[] = "\\d{8}\\.\\d{6}";
  const char LOG_TIME_FORMAT[] = "%Y%m%d.%H%M%S";
}

namespace AdServer
{
namespace ProfilingCommons
{
  /**
   * Functor extract max keep time from full patched file name and
   * fill map full_path -> maximum keeping time.
   * ChunkSelector - logic for find files.
   */
  struct ChunkSelector
  {
    struct ChunkFileDescription
    {
      ChunkFileDescription(
        const Generics::Time& max_keep_time)
        : max_time(max_keep_time)
      {}

      Generics::Time max_time;
    };

    /// In really used full_path -> max_keep_time map.
    typedef std::map<std::string, ChunkFileDescription>
      ChunkFileDescriptionMap;

    ChunkSelector(
      const char* prefix,
      ChunkFileDescriptionMap& chunk_files)
      : chunk_files_(chunk_files)
    {
      std::string re_str = prefix;
      re_str += ".";
      re_str += std::string("(") + LOG_RE_TIME_FORMAT + ")";
      reg_exp_.set_expression(re_str);
    }

    /**
     * Executes functor
     * @param full_path the full path of the file
     * @param () not used file information
     * @return required result
     */
    bool
    operator ()(const char* full_path, const struct stat&)
      noexcept
    {
      String::RegEx::Result sub_strs;

      String::SubString file_name(
        Generics::DirSelect::file_name(full_path));

      if (!reg_exp_.search(sub_strs, file_name) ||
        (assert(!sub_strs.empty()), sub_strs[0].length() != file_name.size()))
      {
        return false;
      }

      chunk_files_.insert(
        std::make_pair(
          full_path,
          ChunkFileDescription(
            Generics::Time(sub_strs[1], LOG_TIME_FORMAT))));

      return true;
    }

  private:
    /// Compiled regular expression matcher: "prefix.(\d{8}\\.\d{6})"
    String::RegEx reg_exp_;
    /// Reference to container that get result map of files
    ChunkFileDescriptionMap& chunk_files_;
  };

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  SubMap::SubMap(
    const char* file,
    const Generics::Time& min_time_val,
    const Generics::Time& max_time_val,
    PlainSubMapType* map_val)
    /*throw(eh::Exception)*/
    : filename(file),
      min_time(min_time_val),
      max_time(max_time_val),
      map(ReferenceCounting::add_ref(map_val))
  {}

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  SubMap::~SubMap() noexcept
  {
    try
    {
      if(unlink_sub_maps_holder.in())
      {
        map->close();
        ::unlink(filename.c_str());

        // no file now
        SyncPolicy::WriteGuard lock(unlink_sub_maps_holder->lock);
        unlink_sub_maps_holder->unlink_maps.erase(max_time);
      }
    }
    catch(...)
    {}
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  ExpireProfileMapBase() noexcept
  {}

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  ExpireProfileMapBase(
    const char* root_path,
    const char* prefix,
    const Generics::Time& extend_time_period,
    FileOpenStrategyType file_open_strategy)
    /*throw(Exception)*/
    : chunk_file_root_(root_path),
      chunk_file_prefix_(prefix),
      extend_time_period_(std::max(extend_time_period, Generics::Time(1))),
      file_open_strategy_(file_open_strategy),
      unlink_sub_maps_holder_(new UnlinkSubMapsHolder())
  {
    static const char* FUN = "ExpireProfileMapBase::ExpireProfileMapBase()";
    ChunkSelector::ChunkFileDescriptionMap chunk_files;
    ChunkSelector chunk_selector(chunk_file_prefix_.c_str(), chunk_files);

    Generics::DirSelect::directory_selector(
      chunk_file_root_.c_str(),
      chunk_selector,
      "*",
      Generics::DirSelect::DSF_RECURSIVE);

    sub_maps_holder_ = new SubMapListHolder;

    for(ChunkSelector::ChunkFileDescriptionMap::const_iterator
          it = chunk_files.begin();
        it != chunk_files.end();
        ++it)
    {
      try
      {
        // use latest configured extend_time for min_time calculate
        ReferenceCounting::SmartPtr<PlainSubMapType> new_sub_map(
          file_open_strategy_(it->first.c_str()));

        sub_maps_holder_->maps.push_front(new SubMap(
          it->first.c_str(),
          it->second.max_time - extend_time_period_,
          it->second.max_time,
          new_sub_map));
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << "Can't init SubMap for file '" << it->first << "': " << ex.what();
        throw Exception(ostr);
      }
    }
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  typename ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
    SubMapListHolder_var
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  sub_maps_() const noexcept
  {
    SyncPolicy::ReadGuard lock(sub_maps_lock_);
    return sub_maps_holder_;
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  bool
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  check_profile(const KeyType& key) const
    /*throw(Exception)*/
  {
    static const char* FUN = "ExpireProfileMapBase::check_profile()";

    try
    {
      // lock key for read
      SubMapListHolder_var sub_maps_holder = sub_maps_();

      for(typename SubMapList::iterator it = sub_maps_holder->maps.begin();
          it != sub_maps_holder->maps.end(); ++it)
      {
        if((*it)->map->check_profile(key))
        {
          return true;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    return false;
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  Generics::ConstSmartMemBuf_var
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  get_profile(const KeyType& key, Generics::Time* access_time)
    /*throw(Exception)*/
  {
    static const char* FUN = "ExpireProfileMapBase::get_profile()";

    SubMap_var map_ptr;

    try
    {
      // lock key for read
      SubMapListHolder_var sub_maps_holder = sub_maps_();

      for(typename SubMapList::iterator it = sub_maps_holder->maps.begin();
          it != sub_maps_holder->maps.end(); ++it)
      {
        map_ptr = *it;
        Generics::ConstSmartMemBuf_var profile = (*it)->map->get_profile(key, access_time);
        if(profile.in())
        {
          return profile;
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception at file '" <<
        (map_ptr.in() ? map_ptr->filename : "") << "': " << ex.what();
      throw Exception(ostr);
    }

    return Generics::ConstSmartMemBuf_var();
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  void
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  save_profile(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now,
    OperationPriority)
    /*throw(Exception)*/
  {
    static const char* FUN = "ExpireProfileMapBase::save_profile()";

    try
    {
      SubMap_var target_map_ptr = find_or_create_chunk_(now);

      if(!target_map_ptr->map->check_profile(key))
      {
        remove_profile_i_(key, target_map_ptr);
      }

      target_map_ptr->map->save_profile(key, mem_buf, now);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  bool
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  remove_profile_i_(const KeyType& key, SubMap* checked_sub_map)
    /*throw(Exception)*/
  {
    static const char* FUN = "ExpireProfileMapBase::remove_profile_i_()";

    try
    {
      SubMapListHolder_var sub_maps_holder = sub_maps_();
      for(typename SubMapList::iterator it = sub_maps_holder->maps.begin();
          it != sub_maps_holder->maps.end(); ++it)
      {
        if(it->in() != checked_sub_map &&
           (*it)->map->remove_profile(key))
        {
          return true;
        }
      }

      return false;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  bool
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  remove_profile(
    const KeyType& key,
    OperationPriority)
    /*throw(Exception)*/
  {
    return remove_profile_i_(key, 0);
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  typename ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::SubMapList::iterator
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  find_chunk_(
    typename ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
      SubMap_var& target_map,
    SubMapListHolder* sub_maps_holder,
    const Generics::Time& now)
    /*throw(Exception)*/
  {
    typename SubMapList::iterator it = sub_maps_holder->maps.end();
    typename SubMapList::iterator ins_it = sub_maps_holder->maps.end();

    for(typename SubMapList::iterator mit = sub_maps_holder->maps.begin();
          mit != sub_maps_holder->maps.end(); ++mit)
    {
      // requested timestamp dispose after all file intervals,
      // insert "fresh" chunk [now, now + extend_time) in head of chunks list
      if((*mit)->max_time <= now)
      {
        ins_it = mit;
        break;
      }

      // requested timestamp inside existing file, use it.
      if((*mit)->min_time <= now && now < (*mit)->max_time)
      {
        it = mit;
        break;
      }
    }

    target_map = (it != sub_maps_holder->maps.end() ? *it : nullptr);

    return ins_it;
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  typename ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::SubMap_var
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  find_or_create_chunk_(const Generics::Time& now)
    /*throw(Exception)*/
  {
    SubMapListHolder_var sub_maps_holder = sub_maps_();

    SubMap_var target_map;
    find_chunk_(target_map, sub_maps_holder, now);

    if(!target_map.in())
    {
      SyncPolicy::WriteGuard lock(extend_lock_);

      sub_maps_holder = sub_maps_(); // get fresh sub maps holder
      sub_maps_holder = new SubMapListHolder(*sub_maps_holder);

      typename SubMapList::iterator ins_it =
        find_chunk_(target_map, sub_maps_holder, now);

      if(!target_map.in()) // double check strategy
      {
        // extend maps
        Generics::Time new_map_content_min_time(
          extend_time_period_ * (
            now.tv_sec / extend_time_period_.tv_sec));

        Generics::Time new_map_content_max_time(
          new_map_content_min_time + extend_time_period_);

        {
          // offset content max time for avoid exists files clash
          // file name construction based on max time
          SyncPolicy::WriteGuard lock(unlink_sub_maps_holder_->lock);
          while(unlink_sub_maps_holder_->unlink_maps.find(
            new_map_content_max_time) !=
              unlink_sub_maps_holder_->unlink_maps.end())
          {
            new_map_content_max_time += Generics::Time::ONE_SECOND;
          }
        }

        // check previous max times, it can be less that new if extend time changed
        // after reopen map
        if(ins_it != sub_maps_holder->maps.begin())
        {
          --ins_it;
          while(ins_it != sub_maps_holder->maps.begin() &&
            (*ins_it)->max_time < new_map_content_max_time)
          {
            --ins_it;
          }

          assert((*ins_it)->max_time != new_map_content_max_time);
          if((*ins_it)->max_time > new_map_content_max_time)
          {
            ++ins_it;
          }
        }

        target_map = create_new_chunk_(
          new_map_content_min_time, new_map_content_max_time);

        /// Inserts given target_map into sub maps before specified ins_it.
        sub_maps_holder->maps.insert(ins_it, target_map);

        SyncPolicy::WriteGuard lock(sub_maps_lock_);
        sub_maps_holder_ = sub_maps_holder;
      }
    }

    return target_map;
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  typename ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::SubMap_var
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  create_new_chunk_(
    const Generics::Time& min_time,
    const Generics::Time& max_time)
    /*throw(Exception)*/
  {
    static const char* FUN = "ExpireProfileMapBase::create_new_chunk_()";

    try
    {
      std::ostringstream filename_s;
      filename_s << chunk_file_root_ << "/" << chunk_file_prefix_ << "." <<
        max_time.get_gm_time().format(LOG_TIME_FORMAT);

      ReferenceCounting::SmartPtr<PlainSubMapType> new_sub_map(
        file_open_strategy_(filename_s.str().c_str()));

      return new SubMap(
        filename_s.str().c_str(), min_time, max_time, new_sub_map);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  void
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  clear_expired(const Generics::Time& expire_time)
    noexcept
  {
    SubMapList erase_maps; // close and remove files outside locks

    {
      SyncPolicy::WriteGuard lock(extend_lock_);

      SubMapListHolder_var sub_maps_holder = sub_maps_();
      sub_maps_holder = new SubMapListHolder(*sub_maps_holder);

      for(typename SubMapList::iterator it = sub_maps_holder->maps.begin();
          it != sub_maps_holder->maps.end(); )
      {
        if((*it)->max_time <= expire_time)
        {
          erase_maps.push_back(*it);
          sub_maps_holder->maps.erase(it++);
        }
        else
        {
          ++it;
        }
      }

      SyncPolicy::WriteGuard sm_lock(sub_maps_lock_);
      sub_maps_holder_ = sub_maps_holder;
    }

    // serialize all unlink_sub_maps_holder changes inside SubMap's
    SyncPolicy::WriteGuard lock(unlink_marking_lock_);

    for(typename SubMapList::iterator it = erase_maps.begin();
        it != erase_maps.end(); ++it)
    {
      (*it)->unlink_sub_maps_holder = unlink_sub_maps_holder_;
    }
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  unsigned long
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  size() const
    noexcept
  {
    SubMapListHolder_var sub_maps_holder = sub_maps_();

    unsigned long ret = 0;

    for(typename SubMapList::const_iterator it =
          sub_maps_holder->maps.begin();
        it != sub_maps_holder->maps.end(); ++it)
    {
      ret += (*it)->map->size();
    }

    return ret;
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  unsigned long
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  area_size() const
    noexcept
  {
    SubMapListHolder_var sub_maps_holder = sub_maps_();

    unsigned long ret = 0;

    for(typename SubMapList::const_iterator it =
          sub_maps_holder->maps.begin();
        it != sub_maps_holder->maps.end(); ++it)
    {
      ret += (*it)->map->area_size();
    }

    return ret;
  }

  template<typename KeyType,
    typename PlainSubMapType, typename FileOpenStrategyType>
  unsigned long
  ExpireProfileMapBase<KeyType, PlainSubMapType, FileOpenStrategyType>::
  file_count_() const
    noexcept
  {
    SubMapListHolder_var sub_maps_holder = sub_maps_();
    return sub_maps_holder->maps.size();
  }
}
}

#endif  // PROFILING_COMMONS_EXPIRE_PROFILE_MAP_TPP
