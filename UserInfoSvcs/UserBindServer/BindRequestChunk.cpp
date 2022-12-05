#include <iostream>
#include <fstream>
#include <sstream>
#include <Generics/Time.hpp>
#include <PrivacyFilter/Filter.hpp>
#include <Generics/DirSelector.hpp>
#include <Generics/Rand.hpp>
#include <String/RegEx.hpp>
#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>

#include <Commons/Algs.hpp>
#include <Commons/PathManip.hpp>
#include <LogCommons/LogCommons.hpp>

#include "ChunkUtils.hpp"
#include "BindRequestChunk.hpp"

namespace
{
  const char LOG_TIME_FORMAT[] = "%Y%m%d.%H%M%S";
  const char TMP_SAVE_DIR_SUFFIX[] = ".BindRequest~";
}

namespace AdServer
{
namespace UserInfoSvcs
{
  namespace
  {
    const String::AsciiStringManip::CharCategory SAVE_ESCAPE_CHAR("^");
    const String::AsciiStringManip::CharCategory NL_CHAR_CATEGORY("\n");
    const String::AsciiStringManip::CharCategory BAR_CHAR_CATEGORY("|");

    void
    save_seq(std::ostream& out, const std::vector<std::string>& elements)
    { 
      for(auto it = elements.begin(); it != elements.end(); ++it)
      {
        // escape '|' '\n' ESCAPE_CHAR
        std::string to_rep;

        String::AsciiStringManip::flatten(
          to_rep,
          *it,
          String::SubString("^0"),
          SAVE_ESCAPE_CHAR);

        std::string tmp;

        String::AsciiStringManip::flatten(
          tmp,
          to_rep,
          String::SubString("^1"),
          NL_CHAR_CATEGORY);

        to_rep.swap(tmp);

        String::AsciiStringManip::flatten(
          tmp,
          to_rep,
          String::SubString("^2"),
          BAR_CHAR_CATEGORY);

        to_rep.swap(tmp);

        if(it != elements.begin())
        {
          out << '|';
        }

        out << to_rep;
      }
    }

    void
    parse_seq(std::vector<std::string>& res, const String::SubString& line)
    {
      String::StringManip::Splitter<String::AsciiStringManip::SepBar> tokenizer(line);      

      String::SubString token;
      while(tokenizer.get_token(token))
      {
        if(token.find('^') != String::SubString::NPOS)
        {
          std::string to_rep(token.str());
          std::string tmp;

          String::StringManip::replace(
            to_rep,
            tmp,
            String::SubString("^1"),
            String::SubString("\n"));

          to_rep.swap(tmp);

          String::StringManip::replace(
            to_rep,
            tmp,
            String::SubString("^2"),
            String::SubString("|"));

          to_rep.swap(tmp);

          String::StringManip::replace(
            to_rep,
            tmp,
            String::SubString("^0"),
            String::SubString("^"));

          to_rep.swap(tmp);

          res.push_back(to_rep);
        }
        else
        {
          res.push_back(token.str());
        }
      }
    }
  }

  // BindRequestChunk::BindRequestHolder
  BindRequestChunk::BindRequestHolder::BindRequestHolder()
  {}

  BindRequestChunk::BindRequestHolder::BindRequestHolder(
    const std::vector<std::string>& bind_user_ids)
    : bind_user_ids_(bind_user_ids)
  {}

  const std::vector<std::string>&
  BindRequestChunk::BindRequestHolder::bind_user_ids() const
  {
    return bind_user_ids_;
  }

  void
  BindRequestChunk::BindRequestHolder::save(std::ostream& out) const
    noexcept
  {
    save_seq(out, bind_user_ids_);
  }

  void
  BindRequestChunk::BindRequestHolder::load(std::istream& input)
    /*throw(Exception)*/
  {
    static const char* FUN = "BindRequestChunk::BindRequestHolder::load()";

    std::string str;

    try
    {
      AdServer::LogProcessing::read_until_eol(input, str);
      parse_seq(bind_user_ids_, str);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": invalid line '" << str << "': " <<
        ex.what();
      throw Exception(ostr);
    }
  }

  // BindRequestChunk::TimePeriodHolder
  template<typename HashAdapterType, typename UserInfoHolderType>
  BindRequestChunk::HolderContainer<HashAdapterType, UserInfoHolderType>::
  TimePeriodHolder::TimePeriodHolder(
    const Generics::Time& min_time_val,
    const Generics::Time& max_time_val)
    noexcept
    : min_time(min_time_val),
      max_time(max_time_val)
  {}

  // BindRequestChunk::HolderContainerGuard
  template<typename HolderContainerType>
  BindRequestChunk::HolderContainerGuard<HolderContainerType>::
  HolderContainerGuard() noexcept
    : holder_container_(new HolderContainerType())
  {}

  template<typename HolderContainerType>
  typename BindRequestChunk::HolderContainerGuard<HolderContainerType>::
    HolderContainer_var
  BindRequestChunk::HolderContainerGuard<HolderContainerType>::
  holder_container() const noexcept
  {
    SyncPolicy::ReadGuard lock(holder_container_lock_);
    return holder_container_;
  }

  template<typename HolderContainerType>
  void
  BindRequestChunk::HolderContainerGuard<HolderContainerType>::
  swap_holder_container(HolderContainer_var& new_holder_container)
    noexcept
  {
    SyncPolicy::WriteGuard lock(holder_container_lock_);
    holder_container_.swap(new_holder_container);
  }

  // BindRequestChunk
  BindRequestChunk::BindRequestChunk(
    Logging::Logger* logger,
    const char* file_root,
    const char* file_prefix,
    const Generics::Time& extend_time_period,
    unsigned long portions_number)
    /*throw(Exception)*/
    : logger_(ReferenceCounting::add_ref(logger)),
      file_root_(file_root),
      file_prefix_(file_prefix),
      extend_time_period_(extend_time_period)
  {
    portions_.resize(portions_number);

    for(PortionArray::iterator portion_it = portions_.begin();
        portion_it != portions_.end(); ++portion_it)
    {
      *portion_it = new Portion();
    }

    if(file_root[0])
    {
      load_holders_();
    }
  }

  BindRequestChunk::~BindRequestChunk() noexcept
  {}

  BindRequestProcessor::BindRequest
  BindRequestChunk::get_bind_request(
    const String::SubString& external_id,
    const Generics::Time&)
    noexcept
  {
    //static const char* FUN = "BindRequestChunk::get_user_id()";

    BindRequest res_bind_request;
    res_bind_request.bind_user_ids.reserve(20);

    unsigned long portion_i = 0;

    HashHashAdapter external_id_hash(
      get_external_id_hash_(portion_i, external_id));

    Portion_var portion = portions_[portion_i];

    UserLockMap::WriteGuard user_lock(
      portion->holders_lock.write_lock(external_id_hash));

    HolderContainerGuard<BindRequestHolderContainer>::
      HolderContainer_var holder_container =
        portion->holder_container_guard.holder_container();

    // find user
    for(auto time_holder_it = holder_container->time_holders.begin();
      time_holder_it != holder_container->time_holders.end();
      ++time_holder_it)
    {
      BindRequestHolderContainer::TimePeriodHolder&
        time_period_holder = **time_holder_it;

      SyncPolicy::ReadGuard lock(time_period_holder.lock);

      auto req_it = time_period_holder.holders.find(
        external_id_hash);

      if(req_it != time_period_holder.holders.end())
      {
        res_bind_request.bind_user_ids = req_it->second.bind_user_ids();
      }
    }

    return res_bind_request;
  }

  void
  BindRequestChunk::add_bind_request(
    const String::SubString& external_id,
    const BindRequest& bind_request,
    const Generics::Time& now)
    noexcept
  {
    BindRequestHolder bind_request_holder(bind_request.bind_user_ids);

    unsigned long portion_i;

    HashHashAdapter external_id_hash(
      get_external_id_hash_(portion_i, external_id));

    Portion_var portion = portions_[portion_i];

    UserLockMap::WriteGuard user_lock(
      portion->holders_lock.write_lock(external_id_hash));

    HolderContainerGuard<BindRequestHolderContainer>::HolderContainer_var
      holder_container =
        portion->holder_container_guard.holder_container();

    // insert into new portion
    BindRequestHolderContainer::TimePeriodHolder_var
      time_holder = get_holder_(
        portion->holder_container_guard,
        holder_container.in(),
        now,
        extend_time_period_);

    SyncPolicy::WriteGuard lock(time_holder->lock);
    time_holder->holders[external_id_hash] = bind_request_holder;
  }

  void
  BindRequestChunk::clear_expired(const Generics::Time& expire_time)
    noexcept
  {
    for(PortionArray::const_iterator portion_it =
          portions_.begin();
        portion_it != portions_.end(); ++portion_it)
    {
      clear_expired_((*portion_it)->holder_container_guard, expire_time);
    }
  }

  void
  BindRequestChunk::dump() /*throw(Exception)*/
  {
    save_holders_();
  }

  void
  BindRequestChunk::generate_file_name_(
    std::string& persistent_file_name,
    std::string& tmp_file_name,
    const Generics::Time& max_time,
    const std::set<std::string>& used_file_names)
    const
    noexcept
  {
    do
    {
      std::ostringstream fname_ostr;

      fname_ostr << file_prefix_ << "." <<
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
  BindRequestChunk::save_holders_() /*throw(Exception)*/
  {
    static const char* FUN = "BindRequestChunk::save_holders_()";

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

      FileNameSet used_file_names;

      for(auto it = old_chunk_files.begin(); it != old_chunk_files.end(); ++it)
      {
        std::string file_name;
        PathManip::split_path(it->first.c_str(), 0, &file_name);
        used_file_names.insert(file_name);
      }

      // save holders into tmp files
      TempFilePathMap new_chunk_files;

      save_holders_i_(
        new_chunk_files,
        &Portion::holder_container_guard,
        used_file_names);

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
  BindRequestChunk::load_holders_() /*throw(Exception)*/
  {
    load_holders_(
      &Portion::holder_container_guard,
      extend_time_period_);
  }

  template<typename HolderContainerType>
  void
  BindRequestChunk::save_holders_i_(
    TempFilePathMap& result_files,
    HolderContainerGuard<HolderContainerType> Portion::* holder_container_guard_field,
    const FileNameSet& used_file_names)
    /*throw(Exception)*/
  {
    static const char* FUN = "BindRequestChunk::save_holders_()";

    typedef std::list<
      typename HolderContainerType::TimePeriodHolder_var>
      TimePeriodHolderList;
    typedef std::map<Generics::Time, TimePeriodHolderList>
      MaxTimePeriodHolderMap;

    MaxTimePeriodHolderMap time_period_holders_by_time;

    for(PortionArray::const_iterator portion_it = portions_.begin();
        portion_it != portions_.end(); ++portion_it)
    {
      const HolderContainerGuard<HolderContainerType>& holder_container_guard =
        (**portion_it).*holder_container_guard_field;
      typename HolderContainerGuard<HolderContainerType>::HolderContainer_var
        holder_container = holder_container_guard.holder_container();
      for(typename HolderContainerType::TimePeriodHolderArray::
            const_iterator time_holder_it =
              holder_container->time_holders.begin();
          time_holder_it != holder_container->time_holders.end();
          ++time_holder_it)
      {
        time_period_holders_by_time[(*time_holder_it)->max_time].push_back(
          *time_holder_it);
      }
    }

    for(typename MaxTimePeriodHolderMap::const_iterator max_time_it =
          time_period_holders_by_time.begin();
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
        used_file_names);

      const std::string new_file_name = file_root_ + "/" +
        new_tmp_file_name;

      std::unique_ptr<std::fstream> file;

      for(typename TimePeriodHolderList::const_iterator time_period_holder_it =
            max_time_it->second.begin();
          time_period_holder_it != max_time_it->second.end();
          ++time_period_holder_it)
      {
        SyncPolicy::ReadGuard lock((*time_period_holder_it)->lock);
        const typename HolderContainerType::HolderMap&
          holders = (*time_period_holder_it)->holders;

        for(typename HolderContainerType::HolderMap::
              const_iterator user_it = holders.begin();
            user_it != holders.end(); ++user_it)
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

          if(user_saved)
          {
            *file << std::endl;
          }

          save_key_(*file, user_it->first);
          *file << "\t";
          user_it->second.save(*file);

          user_saved = true;
        }
      }

      if(file.get())
      {
        file->close();
      }
    }
  }

  template<typename HolderContainerType>
  void
  BindRequestChunk::load_holders_(
    HolderContainerGuard<HolderContainerType> Portion::* holder_container_guard_field,
    const Generics::Time& extend_time_period)
    /*throw(Exception)*/
  {
    static const char* FUN = "BindRequestChunk::load_holders_()";

    typedef std::vector<
      typename HolderContainerGuard<HolderContainerType>::HolderContainer_var>
      HolderContainerArray;

    ChunkSelector::ChunkFileDescriptionMap chunk_files;
    ChunkSelector chunk_selector(file_prefix_, chunk_files);

    Generics::DirSelect::directory_selector(
      file_root_.c_str(),
      chunk_selector,
      "*",
      Generics::DirSelect::DSF_NON_RECURSIVE);

    HolderContainerArray holder_containers;
    holder_containers.resize(portions_.size());
    for(typename HolderContainerArray::iterator holder_container_it =
          holder_containers.begin();
        holder_container_it != holder_containers.end();
        ++holder_container_it)
    {
      *holder_container_it = new HolderContainerType();
    }

    /*
    std::cerr << "Found chunks for '" << file_prefix << "':";
    for(ChunkSelector::ChunkFileDescriptionMap::const_iterator
          it = chunk_files.begin();
        it != chunk_files.end();
        ++it)
    {
      std::cerr << " " << it->first;
    }
    std::cerr << std::endl;
    */

    typename HolderContainerType::TimePeriodHolderArray time_period_holders;
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
            for(typename HolderContainerType::TimePeriodHolderArray::
                  iterator time_period_holder_it = time_period_holders.begin();
                time_period_holder_it != time_period_holders.end();
                ++time_period_holder_it)
            {
              *time_period_holder_it =
                new typename HolderContainerType::TimePeriodHolder(
                  it_max_time - extend_time_period,
                  it_max_time);
            }
          }
          
          while(!file.eof())
          {
            unsigned long portion;
            typename HolderContainerType::KeyType external_id;
            typename HolderContainerType::MappedType user_info;

            if(!file.fail())
            {
              load_key_(external_id, portion, file);
            }

            if(!file.fail())
            {
              AdServer::LogProcessing::read_tab(file);
            }

            if(!file.fail())
            {
              user_info.load(file);
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

            time_period_holders[portion]->holders.insert(
              std::make_pair(external_id, user_info));
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
      HolderContainerGuard<HolderContainerType>& holder_container_guard =
        portions_[portion_i]->*holder_container_guard_field;
      holder_container_guard.swap_holder_container(holder_containers[portion_i]);
    }
  }

  template<typename HolderContainerType>
  void
  BindRequestChunk::clear_expired_(
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
  BindRequestChunk::get_holder_i_(
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
  BindRequestChunk::get_holder_i_(
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
  BindRequestChunk::get_holder_(
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

  BindRequestChunk::HashHashAdapter
  BindRequestChunk::get_external_id_hash_(
    unsigned long& portion,
    const String::SubString& external_id) const noexcept
  {
    size_t full_hash = 0;

    {
      Generics::Murmur64Hash hash(full_hash);
      hash.add(external_id.data(), external_id.size());
    }

    portion = get_external_id_portion_(full_hash);
    return HashHashAdapter(full_hash);
  }

  unsigned long
  BindRequestChunk::get_external_id_portion_(
    unsigned long full_hash) const noexcept
  {
    return (
      (full_hash & 0xFFFF) ^ ((full_hash >> 16) & 0xFFFF)) %
      portions_.size();
  }

  void
  BindRequestChunk::save_key_(
    std::ostream& out, const HashHashAdapter& key)
  {
    out << key.hash();
  }

  void
  BindRequestChunk::load_key_(
    HashHashAdapter& res,
    unsigned long& portion,
    std::istream& in) const
    noexcept
  {
    AdServer::LogProcessing::SpacesString text;
    in >> text;

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
} /* namespace UserInfoSvcs */
} /* namespace AdServer */
