#include <iostream>
#include <fstream>
#include <sstream>
#include <Generics/Time.hpp>
#include <PrivacyFilter/Filter.hpp>
#include <Generics/DirSelector.hpp>
#include <Generics/Rand.hpp>
#include <String/RegEx.hpp>

#include <Commons/Algs.hpp>
#include <Commons/PathManip.hpp>
#include <LogCommons/LogCommons.ipp>
#include <Commons/UserInfoManip.hpp>

#include "ChunkUtils.hpp"
#include "UserBindChunk.hpp"

namespace
{
  const char LOG_TIME_FORMAT[] = "%Y%m%d.%H%M%S";

  typedef const String::AsciiStringManip::Char1Category<'^'>
    EscapeCharCategory;
  EscapeCharCategory ESCAPE_CHAR_CATEGORY;
  const String::AsciiStringManip::SepNL NL_CHAR_CATEGORY;
  const String::AsciiStringManip::SepBar BAR_CHAR_CATEGORY;
}

namespace AdServer
{
namespace UserInfoSvcs
{
  struct UserBindChunk::PortionLoad
  {
    typedef std::map<
      Generics::Time, UserBindChunk::BoundUserHolderContainer::TimePeriodHolder_var>
      TimeHolderMap;

    struct SourceHolder
    {
      TimeHolderMap time_holders;

      // optimization : allow to keep current time holder on one file processing
      Generics::Time cur_time_holder_max_time;
      UserBindChunk::BoundUserHolderContainer::TimePeriodHolder_var cur_time_holder;
    };

    typedef Generics::GnuHashTable<
      Generics::StringHashAdapter, SourceHolder>
      SourceMap;

    SourceMap source_map;
  };

  // UserBindChunk::UserInfoHolder
  const UserBindChunk::UserInfoHolder::TimeOffset
  UserBindChunk::UserInfoHolder::TIME_OFFSET_NOT_INIT_ =
    std::numeric_limits<UserBindChunk::UserInfoHolder::TimeOffset>::min();

  const Generics::Time
  UserBindChunk::UserInfoHolder::TIME_OFFSET_MIN_ = Generics::Time(
    TIME_OFFSET_NOT_INIT_ + 1);

  const Generics::Time
  UserBindChunk::UserInfoHolder::TIME_OFFSET_MAX_ = Generics::Time(
    std::numeric_limits<UserBindChunk::UserInfoHolder::TimeOffset>::max());

  UserBindChunk::UserInfoHolder::UserInfoHolder()
    : first_seen_time_offset_(TIME_OFFSET_NOT_INIT_)
  {}

  void
  UserBindChunk::UserInfoHolder::save(
    std::ostream& out,
    const Generics::Time& base_time)
    const noexcept
  {
    out << get_time(base_time).get_gm_time().format("%Y%m%d.%H%M%S");
  }

  void
  UserBindChunk::UserInfoHolder::load(
    std::istream& input,
    const Generics::Time& base_time)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserBindChunk::UserInfoHolder::load()";

    std::string str;

    try
    {
      AdServer::LogProcessing::read_until_eol(input, str);
      set_time(Generics::Time(str, "%Y%m%d.%H%M%S"), base_time);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": invalid line '" << str << "': " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  Generics::Time
  UserBindChunk::UserInfoHolder::get_time(const Generics::Time& base_time) const
  {
    assert(first_seen_time_offset_ != TIME_OFFSET_NOT_INIT_);
    return Generics::Time(first_seen_time_offset_) + base_time;
  }

  void
  UserBindChunk::UserInfoHolder::set_time(
    const Generics::Time& time,
    const Generics::Time& base_time)
  {
    first_seen_time_offset_ = std::min(
      std::max(TIME_OFFSET_MIN_, time - base_time),
      TIME_OFFSET_MAX_).tv_sec;
  }

  void
  UserBindChunk::UserInfoHolder::adjust_time(
    const Generics::Time& old_base_time,
    const Generics::Time& new_base_time)
  {
    set_time(get_time(old_base_time), new_base_time);
  }

  bool
  UserBindChunk::UserInfoHolder::is_init() const
  {
    return first_seen_time_offset_ != TIME_OFFSET_NOT_INIT_;
  }

  // UserBindChunk::BoundUserInfoHolder
  UserBindChunk::BoundUserInfoHolder::BoundUserInfoHolder()
    noexcept
    : flags(0),
      bad_event_count(0),
      last_bad_event_day(0)
  {}

  bool
  UserBindChunk::BoundUserInfoHolder::need_save()
    const
  {
    return !user_id.is_null();
  }

  void
  UserBindChunk::BoundUserInfoHolder::save(
    std::ostream& out,
    const Generics::Time& /*base_time*/)
    const noexcept
  {
    out << user_id << "\t";
    out << String::StringManip::IntToStr(flags).str() << "\t";
    out << String::StringManip::IntToStr(bad_event_count).str() << "\t";
    out << String::StringManip::IntToStr(last_bad_event_day).str();
  }

  void
  UserBindChunk::BoundUserInfoHolder::load(
    std::istream& input,
    const Generics::Time& /*base_time*/)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserBindChunk::BoundUserInfoHolder::load()";

    std::string str;

    try
    {
      AdServer::LogProcessing::read_until_eol(input, str);
      String::StringManip::SplitTab tokenizer(str);

      String::SubString user_id_str;
      if(!tokenizer.get_token(user_id_str))
      {
        throw Exception("no user id");
      }

      user_id = Generics::Uuid(
        user_id_str,
        user_id_str.length() == 24 // padding
        );

      String::SubString flags_str;
      if(tokenizer.get_token(flags_str))
      {
        if(!String::StringManip::str_to_int(flags_str, flags))
        {
          Stream::Error ostr;
          ostr << "flags have incorrect value = '" << flags_str << "'";
          throw Exception(ostr);
        }

        String::SubString bad_event_count_str;
        if(!tokenizer.get_token(bad_event_count_str))
        {
          Stream::Error ostr;
          ostr << "unexpected end of line after flags value";
          throw Exception(ostr);
        }

        uint16_t read_bad_event_count;

        if(!String::StringManip::str_to_int(
             bad_event_count_str, read_bad_event_count))
        {
          Stream::Error ostr;
          ostr << "bad_event_count have incorrect value = '" <<
            bad_event_count_str << "'";
          throw Exception(ostr);
        }

        bad_event_count = std::min(read_bad_event_count, (uint16_t)255);

        String::SubString last_bad_event_day_str;
        if(!tokenizer.get_token(last_bad_event_day_str))
        {
          Stream::Error ostr;
          ostr << "unexpected end of line after bad_event_count value";
          throw Exception(ostr);
        }

        uint16_t last_bad_event_day_int;

        if(!String::StringManip::str_to_int(
             last_bad_event_day_str, last_bad_event_day_int))
        {
          Stream::Error ostr;
          ostr << "last_bad_event_day have incorrect value = '" <<
            last_bad_event_day_str << "'";
          throw Exception(ostr);
        }

        last_bad_event_day = last_bad_event_day_int;
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": invalid line '" << str << "': " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  // UserBindChunk::TimePeriodHolder
  template<
    typename HashAdapterType,
    typename UserInfoHolderType,
    template<typename, typename> class HashTableType>
  UserBindChunk::HolderContainer<
    HashAdapterType,
    UserInfoHolderType,
    HashTableType>::
  TimePeriodHolder::TimePeriodHolder(
    const Generics::Time& min_time_val,
    const Generics::Time& max_time_val)
    noexcept
    : min_time(min_time_val),
      max_time(max_time_val)
  {}

  // UserBindChunk::HolderContainerGuard
  template<typename HolderContainerType>
  UserBindChunk::HolderContainerGuard<HolderContainerType>::
  HolderContainerGuard() noexcept
    : holder_container_(new HolderContainerType())
  {}

  template<typename HolderContainerType>
  typename UserBindChunk::HolderContainerGuard<HolderContainerType>::
    HolderContainer_var
  UserBindChunk::HolderContainerGuard<HolderContainerType>::
  holder_container() const noexcept
  {
    SyncPolicy::ReadGuard lock(holder_container_lock_);
    return holder_container_;
  }

  template<typename HolderContainerType>
  void
  UserBindChunk::HolderContainerGuard<HolderContainerType>::
  swap_holder_container(HolderContainer_var& new_holder_container)
    noexcept
  {
    SyncPolicy::WriteGuard lock(holder_container_lock_);
    holder_container_.swap(new_holder_container);
  }

  // UserBindChunk
  UserBindChunk::UserBindChunk(
    Logging::Logger* logger,
    const char* file_root,
    const char* file_prefix,
    const char* bound_file_prefix,
    const Generics::Time& extend_time_period,
    const Generics::Time& bound_extend_time_period,
    const Generics::Time& min_age,
    bool bind_at_min_age,
    unsigned long max_bad_event,
    unsigned long portions_number,
    bool load_slave,
    unsigned long partition_index,
    unsigned long partitions_number,
    unsigned long local_chunks)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      file_root_(file_root),
      file_prefix_(file_prefix),
      bound_file_prefix_(bound_file_prefix),
      extend_time_period_(extend_time_period),
      bound_extend_time_period_(bound_extend_time_period),
      min_age_(min_age),
      bind_at_min_age_(bind_at_min_age),
      max_bad_event_(max_bad_event),
      load_slave_(load_slave),
      partition_index_(partition_index),
      partitions_number_(partitions_number)
  {
    portions_.resize(portions_number);

    for(PortionArray::iterator portion_it = portions_.begin();
        portion_it != portions_.end(); ++portion_it)
    {
      *portion_it = new Portion(portions_number * local_chunks);
    }

    if(file_root[0])
    {
      load_users_();
    }
  }

  UserBindChunk::~UserBindChunk() noexcept
  {}

  UserBindChunk::UserInfo
  UserBindChunk::get_user_id(
    const String::SubString& external_id,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    bool silent,
    const Generics::Time& create_time,
    bool for_set_cookie)
    noexcept
  {
    //static const char* FUN = "UserBindChunk::get_user_id()";

    // get source prefix
    String::SubString external_id_prefix;
    String::SubString external_id_suffix;
    div_external_id_(external_id_prefix, external_id_suffix, external_id);

    unsigned long portion_i = 0;

    // external_id_hash : hash by full external_id
    // use for portion, global users lockm user seen search
    StringDefHashAdapter external_id_hash(
      get_external_id_hash_(portion_i, external_id));

    // external_id_suffix_hash : hash by external_id suffix (without source at beginning)
    // use only for bound users search
    ExternalIdHashAdapter external_id_suffix_hash(
      get_bound_external_id_hash_(external_id_suffix));

    Portion_var portion = portions_[portion_i];

    UserLockMap::WriteGuard user_lock(
      portion->users_lock.write_lock(external_id_hash));

    Portion::BoundUserHolderContainerGuard_var bound_user_holder_container_guard =
      get_bound_user_holder_container_guard_(portion, external_id_prefix);

    // user can be bound here
    BoundUserInfoHolder bound_user_info_holder;

    bool insert_into_bound = false;
    bool inserted = false;
    ResultHolder result_holder(
      false, // invalid_operation
      false // user_found
      );
    Generics::Time base_time;

    if(get_user_id_(
      bound_user_info_holder,
      insert_into_bound,
      inserted,
      result_holder,
      base_time,
      *bound_user_holder_container_guard,
      external_id_suffix_hash, // for bound users holder use only suffix
      current_user_id,
      now,
      Generics::Time::ZERO, // create time not affect bound users
      false, // don't insert
      silent,
      for_set_cookie,
      bound_extend_time_period_))
    {
      return adapt_user_info_(bound_user_info_holder, false, result_holder);
    }

    UserInfoHolder user_info_holder;

    get_user_id_(
      user_info_holder,
      insert_into_bound,
      inserted,
      result_holder,
      base_time,
      portion->holder_container_guard,
      external_id_hash,
      current_user_id,
      now,
      create_time,
      true, // insert if not found
      silent,
      for_set_cookie,
      extend_time_period_);

    if(insert_into_bound)
    {
      bound_user_info_holder.user_id =
        AdServer::Commons::UserId::create_random_based();

      if (for_set_cookie)
      {
        bound_user_info_holder.flags |= BoundUserInfoHolder::BF_SETCOOKIE;
      }

      BoundUserHolderContainer::
        TimePeriodHolder_var time_holder = get_holder_(
          *bound_user_holder_container_guard,
          bound_user_holder_container_guard->holder_container().in(),
          now,
          bound_extend_time_period_);

      //SyncPolicy::WriteGuard lock(time_holder->lock);
      time_holder->users.set(
        external_id_suffix_hash, bound_user_info_holder);

      return adapt_user_info_(bound_user_info_holder, true, result_holder);
    }

    return adapt_user_info_(
      user_info_holder,
      inserted,
      result_holder,
      now,
      base_time);
  }

  UserBindChunk::UserInfo
  UserBindChunk::add_user_id(
    const String::SubString& external_id,
    const Commons::UserId& user_id,
    const Generics::Time& now,
    bool resave_if_exists,
    bool ignore_bad_event)
    noexcept
  {
    String::SubString external_id_prefix;
    String::SubString external_id_suffix;
    div_external_id_(external_id_prefix, external_id_suffix, external_id);

    unsigned long portion_i;

    // external_id_hash : hash by full external_id
    // use for portion, global users lockm user seen search
    StringDefHashAdapter external_id_hash(
      get_external_id_hash_(portion_i, external_id));

    // external_id_suffix_hash : hash by external_id suffix (without source at beginning)
    // use only for bound users search
    ExternalIdHashAdapter external_id_suffix_hash(
      get_bound_external_id_hash_(external_id_suffix));

    BoundUserInfoHolder found_user_info;
    bool found_user = false;

    Portion_var portion = portions_[portion_i];

    UserLockMap::WriteGuard user_lock(
      portion->users_lock.write_lock(external_id_hash));

    Portion::BoundUserHolderContainerGuard_var bound_user_holder_container_guard =
      get_bound_user_holder_container_guard_(portion, external_id_prefix);

    erase_user_id_(
      portion->holder_container_guard,
      external_id_hash);

    HolderContainerGuard<BoundUserHolderContainer>::HolderContainer_var
      bound_user_holder_container =
        bound_user_holder_container_guard->holder_container();

    // find user
    {
      for(BoundUserHolderContainer::TimePeriodHolderArray::
            const_iterator time_holder_it =
              bound_user_holder_container->time_holders.begin();
          time_holder_it != bound_user_holder_container->time_holders.end();
          ++time_holder_it)
      {
        BoundUserHolderContainer::TimePeriodHolder&
          time_period_holder = **time_holder_it;
        bool erase = false;

        {
          //SyncPolicy::ReadGuard lock(time_period_holder.lock);

          BoundUserInfoHolder prev_user_info;
          found_user = time_period_holder.users.get(
            prev_user_info, external_id_suffix_hash);

          if(found_user)
          {
            if(time_period_holder.max_time > now)
            {
              // modify user profile (its write locked by user_lock)
              ResultHolder result_holder(
                false, // invalid_operation
                true); // user_found
              UserInfo res_user_info = adapt_user_info_(
                prev_user_info,
                false,
                result_holder);

              if(resave_if_exists)
              {
                save_user_id_(
                  prev_user_info, res_user_info, user_id, ignore_bad_event, now);
                time_period_holder.users.set(
                  external_id_suffix_hash, prev_user_info);
              }

              return res_user_info;
            }

            found_user_info = prev_user_info;
            erase = true;
          }
        }

        if(erase)
        {
          //SyncPolicy::WriteGuard lock(time_period_holder.lock);
          time_period_holder.users.erase(external_id_suffix_hash);
          break;
        }
      }
    }

    BoundUserInfoHolder prev_user_info = found_user_info;
    ResultHolder result_holder(
      false, // invalid_operation
      found_user); // user_found
    UserInfo res_user_info = adapt_user_info_(prev_user_info, false, result_holder);

    if(resave_if_exists || !found_user)
    {
      save_user_id_(found_user_info, res_user_info, user_id, ignore_bad_event, now);
    }

    // insert into new portion
    BoundUserHolderContainer::TimePeriodHolder_var
      time_holder = get_holder_(
        *bound_user_holder_container_guard,
        bound_user_holder_container.in(),
        now,
        bound_extend_time_period_);

    //SyncPolicy::WriteGuard lock(time_holder->lock);
    time_holder->users.set(
      external_id_suffix_hash, found_user_info);

    return res_user_info;
  }

  void
  UserBindChunk::clear_expired(
    const Generics::Time& unbound_expire_time,
    const Generics::Time& bound_expire_time)
    noexcept
  {
    for(PortionArray::const_iterator portion_it =
          portions_.begin();
        portion_it != portions_.end(); ++portion_it)
    {
      clear_expired_((*portion_it)->holder_container_guard, unbound_expire_time);

      Portion::BoundUserHolderSourceMap source_map_copy;

      {
        Portion::SourceSyncPolicy::ReadGuard lock((*portion_it)->source_lock);
        source_map_copy = (*portion_it)->source_to_bound_user_holder_container_guards;
      }

      for(auto source_it = source_map_copy.begin(); source_it != source_map_copy.end(); ++source_it)
      {
        clear_expired_(*(source_it->second), bound_expire_time);
      }
    }
  }

  void
  UserBindChunk::dump() /*throw(Exception)*/
  {
    save_users_();
  }

  bool
  UserBindChunk::modify_at_get_(
    UserInfoHolder& user_info,
    const Commons::UserId& /*current_user_id*/,
    const Generics::Time& now,
    const Generics::Time& base_time,
    const Generics::Time* const old_base_time,
    const Generics::Time* const create_time,
    bool /*for_set_cookie*/,
    bool silent)
    const
    noexcept
  {
    if (silent)
    {
      return false;
    }

    if (old_base_time)
    {
      user_info.adjust_time(*old_base_time, base_time);
    }
    else if (!user_info.is_init() ||
      (create_time ? *create_time : now) < user_info.get_time(base_time))
    {
      user_info.set_time((create_time ? *create_time : now), base_time);
    }

    // return min age reached
    return bind_at_min_age_ &&
      user_info.get_time(base_time) + min_age_ < now;
  }

  bool
  UserBindChunk::modify_at_get_(
    BoundUserInfoHolder& user_info,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    const Generics::Time& /*base_time*/,
    const Generics::Time* const /*old_base_time*/,
    const Generics::Time* const /*creat_time*/,
    bool for_set_cookie,
    bool /*silent*/)
    const
    noexcept
  {
    if (for_set_cookie)
    {
      if (user_info.flags & BoundUserInfoHolder::BF_SETCOOKIE)
      {
        if(user_info.user_id != current_user_id)
        {
          rotate_bad_event_count_(user_info, now);
          ++user_info.bad_event_count;
        }
      }
      else
      {
        user_info.flags |= BoundUserInfoHolder::BF_SETCOOKIE;
      }
    }

    return false;
  }

  bool
  UserBindChunk::prepare_result_at_get_(
    UserInfoHolder&,
    ResultHolder&,
    bool)
    noexcept
  {
    return true;
  }

  bool
  UserBindChunk::prepare_result_at_get_(
    BoundUserInfoHolder& result,
    ResultHolder& result_holder,
    bool for_set_cookie)
    noexcept
  {
    // check max event number
    if(for_set_cookie &&
      (result.flags & BoundUserInfoHolder::BF_SETCOOKIE) &&
      result.bad_event_count > max_bad_event_)
    {
      result_holder.invalid_operation = true;
    }

    result_holder.user_found = true;

    return true;
  }

  UserBindChunk::UserInfo
  UserBindChunk::adapt_user_info_(
    UserInfoHolder& user_info,
    bool inserted,
    const ResultHolder& result_holder,
    const Generics::Time& now,
    const Generics::Time& base_time)
    const
    noexcept
  {
    UserInfo res_user_info;
    res_user_info.min_age_reached = (
      user_info.is_init() &&
      user_info.get_time(base_time) + min_age_ < now);
    res_user_info.created = inserted;
    res_user_info.invalid_operation = result_holder.invalid_operation;
    res_user_info.user_found = result_holder.user_found;
    return res_user_info;
  }

  UserBindChunk::UserInfo
  UserBindChunk::adapt_user_info_(
    BoundUserInfoHolder& user_info,
    bool user_id_generated,
    const ResultHolder& result_holder)
    const
    noexcept
  {
    UserInfo res_user_info;
    res_user_info.user_id = user_info.user_id;
    res_user_info.min_age_reached = true;
    res_user_info.user_id_generated = user_id_generated;
    res_user_info.invalid_operation = result_holder.invalid_operation;
    res_user_info.user_found = result_holder.user_found;
    return res_user_info;
  }

  void
  UserBindChunk::save_user_id_(
    BoundUserInfoHolder& user_info,
    UserInfo& res_user_info,
    const Commons::UserId& user_id,
    bool ignore_bad_event,
    const Generics::Time& now)
    const noexcept
  {
    if (user_info.flags & BoundUserInfoHolder::BF_SETCOOKIE)
    {
      if(user_id != user_info.user_id)
      {
        rotate_bad_event_count_(user_info, now);

        if(user_info.bad_event_count < max_bad_event_)
        {
          user_info.user_id = user_id;
        }
        else
        {
          // skip operation
          res_user_info.invalid_operation = true;
        }
        ++user_info.bad_event_count;
      }
    }
    else
    {
      user_info.flags |= BoundUserInfoHolder::BF_SETCOOKIE;
      user_info.user_id = user_id;
    }

    if(ignore_bad_event)
    {
      user_info.user_id = user_id;
    }
  }

  void
  UserBindChunk::rotate_bad_event_count_(
    BoundUserInfoHolder& user_info,
    const Generics::Time& now)
    const noexcept
  {
    unsigned long cur_day =
      Algs::round_to_day(now).tv_sec / Generics::Time::ONE_DAY.tv_sec;

    if (cur_day != user_info.last_bad_event_day)
    {
      user_info.last_bad_event_day = cur_day;
      user_info.bad_event_count = 0;
    }
  }

  void
  UserBindChunk::remove_chunk_(const char* path)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserBindChunk::remove_chunk_()";

    try
    {
      Generics::DirSelect::directory_selector(
        path,
        ChunkRemover(file_prefix_),
        "*",
        Generics::DirSelect::DSF_NON_RECURSIVE |
          Generics::DirSelect::DSF_NO_EXCEPTION_ON_OPEN);

      Generics::DirSelect::directory_selector(
        path,
        ChunkRemover(bound_file_prefix_),
        "*",
        Generics::DirSelect::DSF_NON_RECURSIVE |
          Generics::DirSelect::DSF_NO_EXCEPTION_ON_OPEN);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't remove chunk content: " << ex.what();
      throw Exception(ostr);
    }

    if(::rmdir(path) == -1 && errno != ENOENT)
    {
      if(errno == ENOTEMPTY)
      {
        // rename
        rename_to_prefixed_(path);
      }
      else
      {
        eh::throw_errno_exception<Exception>(
          FUN,
          ": failed to remove '",
          path,
          "'");
      }
    }
  }

  void
  UserBindChunk::rename_to_prefixed_(const char* path)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserBindChunk::rename_to_prefixed_()";

    while(true)
    {
      std::ostringstream tname_ostr;
      tname_ostr << path << "." <<
        Generics::Time::get_time_of_day().get_gm_time().format(
          LOG_TIME_FORMAT) << "." << Generics::safe_rand();
      const std::string& tname = tname_ostr.str();
      if(std::rename(path, tname.c_str()))
      {
        if(errno != EEXIST && errno != ENOTEMPTY)
        {
          eh::throw_errno_exception<Exception>(
            FUN,
            ": failed to rename '",
            path,
            "' to '",
            tname);
        }
      }
      else
      {
        break;
      }
    }
  }

  void
  UserBindChunk::save_users_() /*throw(Exception)*/
  {
    static const char* FUN = "UserBindChunk::save_users_()";

    if(!file_root_.empty())
    {
      FlushSyncPolicy::WriteGuard flush_lock(flush_lock_);

      // collect old files
      ChunkSelector::ChunkFileDescriptionMap old_chunk_files;

      {
        ChunkSelector chunk_selector(file_prefix_, old_chunk_files);

        Generics::DirSelect::directory_selector(
          file_root_.c_str(),
          chunk_selector,
          "*",
          Generics::DirSelect::DSF_NON_RECURSIVE);
      }

      {
        ChunkSelector chunk_selector(bound_file_prefix_, old_chunk_files);

        Generics::DirSelect::directory_selector(
          file_root_.c_str(),
          chunk_selector,
          "*",
          Generics::DirSelect::DSF_NON_RECURSIVE);
      }

      FileNameSet used_file_names;

      for(auto it = old_chunk_files.begin(); it != old_chunk_files.end(); ++it)
      {
        std::string file_name;
        PathManip::split_path(it->first.c_str(), 0, &file_name);
        used_file_names.insert(file_name);
      }

      // save holders into tmp files
      TempFilePathMap new_chunk_files;

      {
        SeenSaveTimePeriodHolderMap time_period_holders_by_time;

        for(PortionArray::const_iterator portion_it = portions_.begin();
          portion_it != portions_.end(); ++portion_it)
        {
          const HolderContainerGuard<SeenUserHolderContainer>& holder_container_guard =
            (*portion_it)->holder_container_guard;
          HolderContainerGuard<SeenUserHolderContainer>::HolderContainer_var
            holder_container = holder_container_guard.holder_container();
          for(auto time_holder_it = holder_container->time_holders.begin();
            time_holder_it != holder_container->time_holders.end();
            ++time_holder_it)
          {
            // use empty SaveTimePeriodHolder::key_prefix
            time_period_holders_by_time[(*time_holder_it)->max_time].push_back(
              SaveTimePeriodHolder<SeenUserHolderContainer::TimePeriodHolder>(
                String::SubString(), *time_holder_it));
          }
        }

        save_time_holders_(
          new_chunk_files,
          time_period_holders_by_time,
          file_prefix_.c_str(),
          used_file_names);
      }

      {
        BoundSaveTimePeriodHolderMap time_period_holders_by_time;

        for(PortionArray::const_iterator portion_it = portions_.begin();
          portion_it != portions_.end(); ++portion_it)
        {
          Portion::SourceSyncPolicy::ReadGuard lock((*portion_it)->source_lock);

          for(auto source_it = (*portion_it)->source_to_bound_user_holder_container_guards.begin();
            source_it != (*portion_it)->source_to_bound_user_holder_container_guards.end();
            ++source_it)
          {
            const HolderContainerGuard<BoundUserHolderContainer>& holder_container_guard =
              *(source_it->second);
            HolderContainerGuard<BoundUserHolderContainer>::HolderContainer_var
              holder_container = holder_container_guard.holder_container();
            for(auto time_holder_it = holder_container->time_holders.begin();
              time_holder_it != holder_container->time_holders.end();
              ++time_holder_it)
            {
              time_period_holders_by_time[(*time_holder_it)->max_time].push_back(
                SaveTimePeriodHolder<BoundUserHolderContainer::TimePeriodHolder>(
                  source_it->first.text() + "/", *time_holder_it));
            }
          }
        }

        save_time_holders_(
          new_chunk_files,
          time_period_holders_by_time,
          bound_file_prefix_.c_str(),
          used_file_names);
      }

      for(auto it = new_chunk_files.begin(); it != new_chunk_files.end(); ++it)
      {
        const std::string tmp_file = file_root_ + "/" + it->first;
        const std::string p_file = file_root_ + "/" + it->second;

        if(std::rename(tmp_file.c_str(), p_file.c_str()))
        {
          eh::throw_errno_exception<Exception>(
            FUN,
            ": failed to rename file '",
            it->first.c_str(),
            "' to '",
            it->second.c_str(),
            "'");
        }
      }

      // remove old files
      for(auto it = old_chunk_files.begin(); it != old_chunk_files.end(); ++it)
      {
        if(::unlink(it->first.c_str()) == -1)
        {
          eh::throw_errno_exception<BaseChunkSelector::Exception>(
            "can't remove file '",
            it->first.c_str(),
            "'");
        }
      }
    }
  }

  void
  UserBindChunk::load_users_() /*throw(Exception)*/
  {
    load_seen_users_(
      file_root_.c_str(),
      file_prefix_.c_str(),
      extend_time_period_);

    load_bound_users_(
      file_root_.c_str(),
      bound_file_prefix_.c_str(),
      bound_extend_time_period_);
  }

  template<typename HolderContainerType>
  void
  UserBindChunk::erase_user_id_(
    HolderContainerGuard<HolderContainerType>& holder_container_guard,
    const StringDefHashAdapter& external_id_hash)
    noexcept
  {
    typename HolderContainerGuard<HolderContainerType>::
      HolderContainer_var holder_container =
        holder_container_guard.holder_container();

    for(typename HolderContainerType::TimePeriodHolderArray::
          iterator time_holder_it = holder_container->time_holders.begin();
        time_holder_it != holder_container->time_holders.end();
        ++time_holder_it)
    {
      typename HolderContainerType::TimePeriodHolder&
        time_period_holder = **time_holder_it;

      {
        //SyncPolicy::ReadGuard lock(time_period_holder.lock);

        if(time_period_holder.users.erase(external_id_hash))
        {
          break;
        }
      }
    }
  }

  template<typename HolderContainerType, typename KeyType>
  bool
  UserBindChunk::get_user_id_(
    typename HolderContainerType::MappedType& res_user_info,
    bool& insert_into_bound,
    bool& inserted,
    ResultHolder& result_holder,
    Generics::Time& base_time,
    HolderContainerGuard<HolderContainerType>& holder_container_guard,
    const KeyType& external_id_hash,
    const Commons::UserId& current_user_id,
    const Generics::Time& now,
    const Generics::Time& create_time,
    bool insert_if_not_found,
    bool silent,
    bool for_set_cookie,
    const Generics::Time& extend_time_period)
    noexcept
  {
    //static const char* FUN = "UserBindChunk::get_user_id_()";

    insert_into_bound = false;

    typename HolderContainerType::MappedType found_user_info;

    typename HolderContainerGuard<HolderContainerType>::
      HolderContainer_var holder_container =
        holder_container_guard.holder_container();

    // find user
    bool found = false;
    bool erase = false;
    bool insert_after = insert_if_not_found;
    Generics::Time prev_base_time;

    for(typename HolderContainerType::TimePeriodHolderArray::
          iterator time_holder_it = holder_container->time_holders.begin();
        time_holder_it != holder_container->time_holders.end();
        ++time_holder_it)
    {
      typename HolderContainerType::TimePeriodHolder&
        time_period_holder = **time_holder_it;

      {
        //SyncPolicy::ReadGuard lock(time_period_holder.lock);

        bool user_found = time_period_holder.users.get(
          res_user_info, external_id_hash);

        if(user_found)
        {
          found = true;

          insert_into_bound = modify_at_get_(
            res_user_info,
            current_user_id,
            now,
            time_period_holder.min_time,
            nullptr, // old_base_time
            nullptr, // create_time
            for_set_cookie,
            silent);
          insert_after = !insert_into_bound;
          base_time = time_period_holder.min_time;

          if(silent || (!insert_into_bound && time_period_holder.max_time > now))
          {
            time_period_holder.users.set(external_id_hash, res_user_info);
            return prepare_result_at_get_(
              res_user_info, result_holder, for_set_cookie);
          }

          erase = true;
          prev_base_time = time_period_holder.min_time;
        }
      }

      if(erase)
      {
        //SyncPolicy::WriteGuard lock(time_period_holder.lock);
        time_period_holder.users.erase(external_id_hash);
        break;
      }
    }

    if(silent || !insert_after)
    {
      return false;
    }

    typename HolderContainerType::TimePeriodHolder_var
      time_holder = get_holder_(
        holder_container_guard,
        holder_container.in(),
        now,
        extend_time_period);

    base_time = time_holder->min_time;

    // if user appeared for 'min age reached' check can be used
    // alternate create_time, but for expiration continue to use now
    const Generics::Time* first_seen_time = (
      create_time == Generics::Time::ZERO ? nullptr : &create_time);

    if ((insert_if_not_found || found) && modify_at_get_(
      res_user_info,
      current_user_id,
      now,
      base_time,
      found ? &prev_base_time : nullptr,
      first_seen_time,
      for_set_cookie,
      silent))
    {
      insert_into_bound = true;
      return false;
    }

    if (!found)
    {
      inserted = true;
    }

    //SyncPolicy::WriteGuard lock(time_holder->lock);

    time_holder->users.set(external_id_hash, res_user_info);

    return prepare_result_at_get_(
      res_user_info, result_holder, for_set_cookie);
  }

  template<typename TimePeriodHolderType>
  void
  UserBindChunk::save_time_holders_(
    TempFilePathMap& result_files,
    const std::map<
      Generics::Time,
      std::list<UserBindChunk::SaveTimePeriodHolder<TimePeriodHolderType> > >&
      time_period_holders_by_time,
    const char* file_prefix,
    const FileNameSet& used_file_names)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserBindChunk::save_time_holders_()";

    for(auto max_time_it = time_period_holders_by_time.begin();
        max_time_it != time_period_holders_by_time.end();
        ++max_time_it)
    {
      bool user_saved = false;

      std::string new_persistent_file_name;
      std::string new_tmp_file_name;

      generate_file_name_(
        new_persistent_file_name,
        new_tmp_file_name,
        max_time_it->first,
        file_prefix,
        used_file_names);

      const std::string new_file_name = file_root_ + "/" +
        new_tmp_file_name;

      std::unique_ptr<std::fstream> file;

      for(auto time_period_holder_it = max_time_it->second.begin();
        time_period_holder_it != max_time_it->second.end();
        ++time_period_holder_it)
      {
        TimePeriodHolderType* time_period_holder = time_period_holder_it->time_holder.in();
        //SyncPolicy::ReadGuard lock(time_period_holder->lock);
        const Generics::Time& min_time = time_period_holder->min_time;
        auto& users = time_period_holder->users;

        auto fetcher = users.fetcher();
        while(true)
        {
          typename TimePeriodHolderType::UserIdMap::FetchArray users;
          bool fin = !fetcher.get(users, 100, 1000);

          for(auto user_it = users.begin(); user_it != users.end(); ++user_it)
          {
            if(!file.get())
            {
              file.reset(new std::fstream(
                new_file_name.c_str(),
                std::ios_base::out | std::ios_base::trunc));

              if(!file->is_open())
              {
                Stream::Error ostr;
                ostr << FUN << ": can't open file '" << new_file_name << "'";
                throw Exception(ostr);
              }

              result_files.insert(std::make_pair(
                new_tmp_file_name,
                new_persistent_file_name));
            }

            if(user_it->second.need_save())
            {
              if(user_saved)
              {
                *file << std::endl;
              }

              *file << AdServer::LogProcessing::SpacesString(time_period_holder_it->key_prefix);
              save_key_(*file, user_it->first);
              *file << "\t";
              user_it->second.save(*file, min_time);

              user_saved = true;
            }
          }
          
          if(fin)
          {
            break;
          }
        }
      }

      if(file.get())
      {
        file->close();

        if(file->bad())
        {
          Stream::Error ostr;
          ostr << FUN << ": can't save to file '" << new_file_name << "'";
          throw Exception(ostr);
        }
      }

      if(!user_saved && !new_tmp_file_name.empty())
      {
        // remove file from result_files
        result_files.erase(new_tmp_file_name);
        ::unlink(new_tmp_file_name.c_str());
      }
    }
  }

  void
  UserBindChunk::load_seen_users_(
    const char* file_root,
    const char* file_prefix,
    const Generics::Time& extend_time_period)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserBindChunk::load_seen_users_()";

    typedef std::vector<
      HolderContainerGuard<SeenUserHolderContainer>::HolderContainer_var>
      HolderContainerArray;

    ChunkSelector::ChunkFileDescriptionMap chunk_files;
    ChunkSelector chunk_selector(String::SubString(file_prefix), chunk_files);

    Generics::DirSelect::directory_selector(
      file_root,
      chunk_selector,
      "*",
      Generics::DirSelect::DSF_NON_RECURSIVE);

    HolderContainerArray holder_containers;
    holder_containers.resize(portions_.size());
    for(auto holder_container_it = holder_containers.begin();
        holder_container_it != holder_containers.end();
        ++holder_container_it)
    {
      *holder_container_it = new SeenUserHolderContainer();
    }

    SeenUserHolderContainer::TimePeriodHolderArray time_period_holders;
    Generics::Time cur_max_time;

    for(ChunkSelector::ChunkFileDescriptionMap::const_reverse_iterator
          it = chunk_files.rbegin();
        it != chunk_files.rend();
        ++it)
    {
      unsigned long line_i = 0;

      try
      {
        std::fstream file(it->first.c_str(), std::ios_base::in);

        if(file.is_open())
        {
          // correct max_time by currently configured extend_time_period
          Generics::Time it_max_time = extend_time_period * (
            it->second.max_time.tv_sec / extend_time_period.tv_sec);

          if(it_max_time < it->second.max_time)
          {
            it_max_time += extend_time_period;
          }

          if(time_period_holders.empty() ||
             it_max_time + extend_time_period <= cur_max_time)
          {
            // time period changed - merge fetched data (
            //   otherwise use previously fetched time holders)
            for(unsigned long portion_i = 0;
                portion_i < time_period_holders.size();
                ++portion_i)
            {
              // push value with less max_time to end
              holder_containers[portion_i]->time_holders.push_back(
                time_period_holders[portion_i]);
            }

            time_period_holders.clear();
            cur_max_time = it_max_time;
          }

          if(time_period_holders.empty())
          {
            time_period_holders.resize(portions_.size());

            // init time holders for each portion
            for(SeenUserHolderContainer::TimePeriodHolderArray::
                  iterator time_period_holder_it = time_period_holders.begin();
                time_period_holder_it != time_period_holders.end();
                ++time_period_holder_it)
            {
              *time_period_holder_it =
                new SeenUserHolderContainer::TimePeriodHolder(
                  it_max_time - extend_time_period,
                  it_max_time);
            }
          }

          while(!file.eof())
          {
            unsigned long portion;
            SeenUserHolderContainer::KeyType external_id;
            SeenUserHolderContainer::MappedType user_info;

            bool use_key = true;

            if(!file.fail())
            {
              use_key = load_key_(external_id, portion, file);
            }

            if(!file.fail())
            {
              AdServer::LogProcessing::read_tab(file);
            }

            if(!file.fail())
            {
              user_info.load(file, cur_max_time - extend_time_period);
            }

            if(file.fail())
            {
              Stream::Error ostr;
              ostr << FUN << ": incorrect line #" << line_i;
              throw Exception(ostr);
            }

            {
              char eol;
              file.get(eol);
              if(!file.eof() && (file.fail() || eol != '\n'))
              {
                Stream::Error ostr;
                ostr << FUN << ": incorrect line #" << line_i <<
                  ": line isn't closed when expected";
                throw Exception(ostr);
              }
            }

            ++line_i;

            if(use_key)
            {
              time_period_holders[portion]->users.set(external_id, user_info);
            }
          }
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't load '" << it->first <<
          "'(line #" << line_i << "), caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }

    for(unsigned long portion_i = 0;
        portion_i < time_period_holders.size();
        ++portion_i)
    {
      holder_containers[portion_i]->time_holders.push_back(
        time_period_holders[portion_i]);
    }

    for(unsigned long portion_i = 0;
        portion_i < portions_.size(); ++portion_i)
    {
      HolderContainerGuard<SeenUserHolderContainer>& holder_container_guard =
        portions_[portion_i]->holder_container_guard;
      holder_container_guard.swap_holder_container(holder_containers[portion_i]);
    }
  }

  void
  UserBindChunk::load_bound_users_(
    const char* file_root,
    const char* file_prefix,
    const Generics::Time& extend_time_period)
    /*throw(Exception)*/
  {
    static const char* FUN = "UserBindChunk::load_bound_users_()";

    typedef std::vector<PortionLoad>
      PortionLoadArray;

    ChunkSelector::ChunkFileDescriptionMap chunk_files;
    ChunkSelector chunk_selector(String::SubString(file_prefix), chunk_files);

    Generics::DirSelect::directory_selector(
      file_root,
      chunk_selector,
      "*",
      Generics::DirSelect::DSF_NON_RECURSIVE);

    PortionLoadArray portion_loads;
    portion_loads.resize(portions_.size());

    BoundUserHolderContainer::TimePeriodHolderArray time_period_holders;

    std::map<unsigned char, unsigned long> enc_stats;
    //std::map<std::string, std::map<unsigned char, unsigned long> > source_enc_stats;

    for(ChunkSelector::ChunkFileDescriptionMap::const_reverse_iterator
          it = chunk_files.rbegin();
        it != chunk_files.rend();
        ++it)
    {
      unsigned long line_i = 0;

      try
      {
        std::fstream file(it->first.c_str(), std::ios_base::in);

        if(file.is_open())
        {
          // correct max_time by currently configured extend_time_period
          Generics::Time it_max_time = extend_time_period * (
            it->second.max_time.tv_sec / extend_time_period.tv_sec);

          if(it_max_time < it->second.max_time)
          {
            it_max_time += extend_time_period;
          }

          // load file
          while(!file.eof())
          {
            unsigned long portion;
            StringDefHashAdapter external_id;
            BoundUserHolderContainer::MappedType user_info;

            bool use_key = true;

            if(!file.fail())
            {
              use_key = load_key_(external_id, portion, file);
            }

            if(!file.fail())
            {
              AdServer::LogProcessing::read_tab(file);
            }

            if(!file.fail())
            {
              user_info.load(file, it_max_time - extend_time_period);
            }

            if(file.fail())
            {
              Stream::Error ostr;
              ostr << FUN << ": incorrect line #" << line_i;
              throw Exception(ostr);
            }

            {
              char eol;
              file.get(eol);
              if(!file.eof() && (file.fail() || eol != '\n'))
              {
                Stream::Error ostr;
                ostr << FUN << ": incorrect line #" << line_i <<
                  ": line isn't closed when expected";
                throw Exception(ostr);
              }
            }

            ++line_i;

            std::string external_id_str = external_id.text();

            if(use_key && need_to_load_(external_id_str))
            {
              String::SubString external_id_prefix;
              String::SubString external_id_suffix;
              div_external_id_(external_id_prefix, external_id_suffix, external_id_str);
              ExternalIdHashAdapter external_id_suffix_hash =
                get_bound_external_id_hash_(external_id_suffix);

              {
                // fill enc stats
                auto enc_stat_it = enc_stats.find(external_id_suffix_hash.encoder_id());
                if(enc_stat_it != enc_stats.end())
                {
                  ++enc_stat_it->second;
                }
                else
                {
                  enc_stats[external_id_suffix_hash.encoder_id()] = 1;
                }

                /*
                auto& mm = source_enc_stats[external_id_prefix.str()];

                auto mm_enc_stat_it = mm.find(external_id_suffix_hash.encoder_id());
                if(mm_enc_stat_it != mm.end())
                {
                  ++mm_enc_stat_it->second;
                }
                else
                {
                  mm[external_id_suffix_hash.encoder_id()] = 1;
                }
                */
              }

              // correct cur_time_holder if required and insert record to it
              PortionLoad::SourceHolder& source_holder =
                portion_loads[portion].source_map[external_id_prefix];

              if(source_holder.cur_time_holder_max_time != it_max_time - extend_time_period ||
                !source_holder.cur_time_holder.in())
              {
                source_holder.cur_time_holder_max_time = it_max_time - extend_time_period;
                auto time_holder_it = source_holder.time_holders.find(
                  it_max_time - extend_time_period);

                if(time_holder_it != source_holder.time_holders.end())
                {
                  source_holder.cur_time_holder = time_holder_it->second;
                }
                else
                {
                  BoundUserHolderContainer::TimePeriodHolder_var new_time_holder =
                    new BoundUserHolderContainer::TimePeriodHolder(
                      it_max_time - extend_time_period,
                      it_max_time);

                  source_holder.time_holders.insert(
                    std::make_pair(it_max_time - extend_time_period, new_time_holder));
                  source_holder.cur_time_holder = new_time_holder;
                }
              }

              source_holder.cur_time_holder->users.set(
                external_id_suffix_hash, user_info);
            }
          }
        }
      }
      catch(const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": can't load '" << it->first <<
          "'(line #" << line_i << "), caught eh::Exception: " << ex.what();
        throw Exception(ostr);
      }
    }

    std::cerr << "enc stats:" << std::endl;
    for(auto it = enc_stats.begin(); it != enc_stats.end(); ++it)
    {
      std::cerr << static_cast<int>(it->first) << ": " << it->second << std::endl;
    }

    /*
    std::cerr << "source enc stats:" << std::endl;
    for(auto it = source_enc_stats.begin(); it != source_enc_stats.end(); ++it)
    {
      std::cerr << it->first << ":";
      for(auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
      {
        std::cerr << " " << static_cast<int>(it2->first) << "=>" << it2->second;
      }
      std::cerr << std::endl;
    }
    */

    // convert load container to main container
    unsigned long portion_i = 0;
    for(auto portition_load_it = portion_loads.begin(); portition_load_it != portion_loads.end();
      ++portition_load_it, ++portion_i)
    {
      auto& holder_container_guards =
        portions_[portion_i]->source_to_bound_user_holder_container_guards;

      for(auto source_it = portition_load_it->source_map.begin();
        source_it != portition_load_it->source_map.end(); ++source_it)
      {
        auto target_source_it = holder_container_guards.insert(
          std::make_pair(source_it->first, new Portion::BoundUserHolderContainerGuard())).first;

        Portion::BoundUserHolderContainerGuard& target_holder_container_guard =
          *(target_source_it->second);

        // create target BoundUserHolderContainer
        BoundUserHolderContainer_var bound_user_holder_container = new BoundUserHolderContainer();

        for(auto time_holder_it = source_it->second.time_holders.rbegin();
          time_holder_it != source_it->second.time_holders.rend(); ++time_holder_it)
        {
          bound_user_holder_container->time_holders.push_back(time_holder_it->second);
        }

        target_holder_container_guard.swap_holder_container(bound_user_holder_container);
      }
    }
  }

  template<typename HolderContainerType>
  void
  UserBindChunk::clear_expired_(
    HolderContainerGuard<HolderContainerType>& holder_container_guard,
    const Generics::Time& expire_time)
    noexcept
  {
    typename HolderContainerGuard<HolderContainerType>::HolderContainer_var
      new_holder_container = new HolderContainerType();

    ExtendSyncPolicy::WriteGuard extend_lock(holder_container_guard.extend_lock);

    typename HolderContainerGuard<HolderContainerType>::HolderContainer_var
      holder_container = holder_container_guard.holder_container();
    new_holder_container->time_holders.reserve(
      holder_container->time_holders.size());

    for(typename HolderContainerType::TimePeriodHolderArray::
          iterator time_holder_it = holder_container->time_holders.begin();
        time_holder_it != holder_container->time_holders.end();
        ++time_holder_it)
    {
      if((*time_holder_it)->max_time > expire_time)
      {
        new_holder_container->time_holders.push_back(*time_holder_it);
      }
    }

    holder_container_guard.swap_holder_container(new_holder_container);
  }

  template<typename HolderContainerType>
  void
  UserBindChunk::get_holder_i_(
    typename HolderContainerType::TimePeriodHolder_var& res_holder,
    const HolderContainerType* holder_container,
    const Generics::Time& time)
    noexcept
  {
    typename HolderContainerType::TimePeriodHolderArray::
      const_iterator it = holder_container->time_holders.begin();

    for(; it != holder_container->time_holders.end(); ++it)
    {
      if((*it)->max_time <= time)
      {
        return;
      }
      else if((*it)->min_time <= time && time < (*it)->max_time)
      {
        res_holder = *it;
        break;
      }
    }
  }

  template<typename HolderContainerType>
  typename HolderContainerType::TimePeriodHolderArray::iterator
  UserBindChunk::get_holder_i_(
    typename HolderContainerType::TimePeriodHolder_var& res_holder,
    HolderContainerType* holder_container,
    const Generics::Time& time)
    noexcept
  {
    typename HolderContainerType::TimePeriodHolderArray::
      iterator it = holder_container->time_holders.begin();

    for(; it != holder_container->time_holders.end(); ++it)
    {
      if((*it)->max_time <= time)
      {
        // insert new holder before it
        return it;
      }
      else if((*it)->min_time <= time && time < (*it)->max_time)
      {
        res_holder = *it;
        break;
      }
    }

    return holder_container->time_holders.end();
  }

  template<typename HolderContainerType>
  typename HolderContainerType::TimePeriodHolder_var
  UserBindChunk::get_holder_(
    HolderContainerGuard<HolderContainerType>& holder_container_guard,
    const HolderContainerType* holder_container,
    const Generics::Time& now,
    const Generics::Time& extend_time_period)
    noexcept
  {
    typename HolderContainerType::TimePeriodHolder_var res_holder;

    get_holder_i_(res_holder, holder_container, now);

    if(res_holder)
    {
      return res_holder;
    }

    // destroy old holder container outside lock
    typename HolderContainerGuard<HolderContainerType>::HolderContainer_var
      new_holder_container;

    Generics::Time min_time = extend_time_period * (
      now.tv_sec / extend_time_period.tv_sec);

    /*
    std::cerr << "create TimePeriodHolder(" << min_time.gm_ft() << ", " <<
      (min_time + extend_time_period_).gm_ft() << ")" << std::endl;
    */

    typename HolderContainerType::TimePeriodHolder_var new_time_holder =
      new typename HolderContainerType::TimePeriodHolder(
        min_time, min_time + extend_time_period);

    // serialize new holders creation
    ExtendSyncPolicy::WriteGuard extend_lock(holder_container_guard.extend_lock);

    typename HolderContainerGuard<HolderContainerType>::HolderContainer_var
      actual_holder_container = holder_container_guard.holder_container();

    // recheck : container can be extended by other thread
    new_holder_container = new HolderContainerType(
      *actual_holder_container);

    typename HolderContainerType::TimePeriodHolderArray::iterator ins_it =
      get_holder_i_(res_holder,
        new_holder_container.in(),
        new_time_holder->max_time - Generics::Time::ONE_SECOND);

    if(res_holder)
    {
      return res_holder;
    }

    new_holder_container->time_holders.insert(ins_it, new_time_holder);

    holder_container_guard.swap_holder_container(new_holder_container);

    return new_time_holder;
  }

  UserBindChunk::StringDefHashAdapter
  UserBindChunk::get_external_id_hash_(
    unsigned long& portion,
    const String::SubString& external_id) const noexcept
  {
    size_t full_hash = 0;

    {
      Generics::Murmur32v3Hash hash(full_hash);
      hash.add(external_id.data(), external_id.size());
    }

    portion = get_external_id_portion_(full_hash);
    return StringDefHashAdapter(external_id, full_hash);
  }

  ExternalIdHashAdapter
  UserBindChunk::get_bound_external_id_hash_(
    const String::SubString& external_id) const noexcept
  {
    size_t full_hash = 0;

    {
      Generics::Murmur32v3Hash hash(full_hash);
      hash.add(external_id.data(), external_id.size());
    }

    return ExternalIdHashAdapter(external_id, full_hash);
  }

  unsigned long
  UserBindChunk::get_external_id_portion_(
    unsigned long full_hash) const noexcept
  {
    return (
      (full_hash & 0xFFFF) ^ ((full_hash >> 16) & 0xFFFF)) %
      portions_.size();
  }

  void
  UserBindChunk::save_key_(
    std::ostream& out, const ExternalIdHashAdapter& key)
  {
    out << AdServer::LogProcessing::SpacesString(key.text());
  }

  bool
  UserBindChunk::load_key_(
    StringDefHashAdapter& res,
    unsigned long& portion,
    std::istream& in) const
    noexcept
  {
    AdServer::LogProcessing::SpacesString text;
    in >> text;
    res = get_external_id_hash_(portion, text);
    bool invalid_aes_id = (
      text.size() == 4 + 64 &&
      text[0] == 'b' && text[1] == 'l' && text[2] == '2' && text[3] == '/');
    return (text.size() <= 120) && !invalid_aes_id;
  }

  void
  UserBindChunk::save_key_(
    std::ostream& out, const HashHashAdapter& key)
  {
    out << key.hash();
  }

  bool
  UserBindChunk::load_key_(
    HashHashAdapter& res,
    unsigned long& portion,
    std::istream& in) const
      noexcept
  {
    AdServer::LogProcessing::SpacesString text;
    in >> text;
    if(text.find('/') != std::string::npos)
    {
      // old version always contains '/' between src and value
      res = get_external_id_hash_(portion, text);
    }
    else
    {
      size_t hash;
      if(String::StringManip::str_to_int(text, hash))
      {
        res = HashHashAdapter(hash);
        portion = get_external_id_portion_(hash);
      }
      else
      {
        in.setstate(std::ios_base::failbit);
      }
    }

    return true;
  }

  void
  UserBindChunk::generate_file_name_(
    std::string& persistent_file_name,
    std::string& tmp_file_name,
    const Generics::Time& max_time,
    const char* file_prefix,
    const std::set<std::string>& used_file_names)
    const
    noexcept
  {
    do
    {
      std::ostringstream fname_ostr;

      fname_ostr << file_prefix << "." <<
        max_time.get_gm_time().format(LOG_TIME_FORMAT) << "." <<
        std::setfill('0') << std::setw(4) <<
        (Generics::safe_rand() % 10000) <<
        std::setfill('0') << std::setw(4) <<
        (Generics::safe_rand() % 10000);

      persistent_file_name = fname_ostr.str();
    }
    while(used_file_names.find(persistent_file_name) != used_file_names.end());

    tmp_file_name = "~";
    tmp_file_name += persistent_file_name;
  }

  void
  UserBindChunk::div_external_id_(
    String::SubString& external_id_prefix,
    String::SubString& external_id_suffix,
    const String::SubString& external_id)
    noexcept
  {
    String::SubString::SizeType pos = external_id.find('/');
    if(pos != String::SubString::NPOS)
    {
      external_id_prefix = external_id.substr(0, pos);
      external_id_suffix = external_id.substr(pos + 1, external_id.size() - pos - 1);
    }
    else
    {
      external_id_prefix = String::SubString();
      external_id_suffix = external_id;
    }
  }

  UserBindChunk::Portion::BoundUserHolderContainerGuard_var
  UserBindChunk::get_bound_user_holder_container_guard_(
    Portion* portion,
    const String::SubString& external_id_prefix)
    noexcept
  {
    Generics::StringHashAdapter external_id_prefix_hash_adapter(external_id_prefix);
    Portion::BoundUserHolderContainerGuard_var bound_user_holder_container_guard;

    {
      Portion::SourceSyncPolicy::ReadGuard lock(portion->source_lock);
      auto source_it = portion->source_to_bound_user_holder_container_guards.find(
        external_id_prefix_hash_adapter);
      if(source_it != portion->source_to_bound_user_holder_container_guards.end())
      {
        bound_user_holder_container_guard = source_it->second;
      }
    }

    if(bound_user_holder_container_guard.in() == 0)
    {
      Portion::BoundUserHolderContainerGuard_var ins_bound_user_holder_container_guard =
        new Portion::BoundUserHolderContainerGuard();

      Portion::SourceSyncPolicy::WriteGuard lock(portion->source_lock);
      auto source_it = portion->source_to_bound_user_holder_container_guards.insert(
        std::make_pair(external_id_prefix_hash_adapter, ins_bound_user_holder_container_guard)).first;
      bound_user_holder_container_guard = source_it->second;
    }

    return bound_user_holder_container_guard;
  }

  bool
  UserBindChunk::need_to_load_(const String::SubString& id) const noexcept
  {
    if(load_slave_)
    {
      return true;
    }

    // code equal to distribution in UserBindOperationDistributor class
    return (AdServer::Commons::external_id_distribution_hash(
      id) >> 8) % partitions_number_ == partition_index_;
  }
} /* namespace UserInfoSvcs */
} /* namespace AdServer */
