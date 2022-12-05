#include <sstream>
#include <iomanip>
#include <set>
#include <list>
#include <String/AsciiStringManip.hpp>
#include <String/Tokenizer.hpp>
#include <Generics/DirSelector.hpp>
#include "FileLevel.hpp"
#include "MergeIterator.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  namespace
  {
    const char DUMP_SUFFIX[] = ".dump";
    const Generics::Time DUMP_RETRY_PERIOD(10); // 10 seconds
  }

  template<typename KeyType, typename KeySerializerType>
  class LevelProfileMap<KeyType, KeySerializerType>::LevelAssigner
  {
  public:
    LevelAssigner(
      LevelProfileMap<KeyType, KeySerializerType>& level_profile_map,
      typename LevelProfileMap<KeyType, KeySerializerType>::MapHolder* map_holder,
      LoadingProgressCallbackBase_var progress_checker,
      const char* file_directory,
      const char* file_prefix)
      noexcept
      : level_profile_map_(level_profile_map),
        map_holder_(ReferenceCounting::add_ref(map_holder)),
        progress_checker_(progress_checker),
        file_directory_(file_directory),
        file_prefix_(file_prefix)
    {}

    bool
    operator()(const char* file_path, const struct stat& /*st*/)
      /*throw (eh::Exception)*/
    {
      if(::strncmp(file_path,
        file_prefix_.c_str(), file_prefix_.size()) == 0)
      {
        level_profile_map_.add_level_(
          map_holder_,
          progress_checker_,
          file_directory_.c_str(),
          file_path);
      }

      return true;
    }

  private:
    LevelProfileMap<KeyType, KeySerializerType>& level_profile_map_;
    typename LevelProfileMap<KeyType, KeySerializerType>::
      MapHolder_var map_holder_;
    LoadingProgressCallbackBase_var progress_checker_;
    const std::string file_directory_;
    const std::string file_prefix_;
  };

  template<typename KeyType, typename KeySerializerType>
  struct LevelProfileMap<KeyType, KeySerializerType>::LevelHolderPtrLess
  {
    bool
    operator()(const LevelHolder* left, const LevelHolder* right) const
    {
      return left->index < right->index ||
        (left->index == right->index &&
         left->sub_index > right->sub_index);
    }
  };

  template<typename KeyType, typename KeySerializerType>
  class LevelProfileMap<KeyType, KeySerializerType>::DumpRWLevelTask:
    public Generics::TaskGoal
  {
  public:
    DumpRWLevelTask(
      Generics::TaskRunner* task_runner,
      ThisMap* level_profile_map,
      bool periodic)
      noexcept;

    virtual void
    execute() noexcept;

  protected:
    virtual
    ~DumpRWLevelTask() noexcept
    {}

  protected:
    ThisMap_var level_profile_map_;
    const bool periodic_;
  };

  template<typename KeyType, typename KeySerializerType>
  class LevelProfileMap<KeyType, KeySerializerType>::DumpMemLevelTask:
    public Generics::TaskGoal
  {
  public:
    DumpMemLevelTask(
      Generics::TaskRunner* task_runner,
      ThisMap* level_profile_map,
      unsigned long index,
      unsigned long sub_index)
      noexcept;

    virtual void
    execute() noexcept;

  protected:
    virtual
    ~DumpMemLevelTask() noexcept
    {}

  protected:
    ThisMap_var level_profile_map_;
    const unsigned long index_;
    const unsigned long sub_index_;
  };

  template<typename KeyType, typename KeySerializerType>
  class LevelProfileMap<KeyType, KeySerializerType>::MergeLevelTask:
    public Generics::TaskGoal
  {
  public:
    MergeLevelTask(
      Generics::TaskRunner* task_runner,
      ThisMap* level_profile_map)
      noexcept;

    virtual void
    execute() noexcept;

  protected:
    virtual
    ~MergeLevelTask() noexcept
    {}

  protected:
    ThisMap_var level_profile_map_;
  };

  template<typename KeyType, typename KeySerializerType>
  class LevelProfileMap<KeyType, KeySerializerType>::ClearExpiredTask:
    public Generics::Task,
    public ReferenceCounting::AtomicImpl
  {
  public:
    ClearExpiredTask(ThisMap* level_profile_map)
      noexcept;

    virtual void
    execute() noexcept;

  protected:
    virtual
    ~ClearExpiredTask() noexcept
    {}

  protected:
    ThisMap_var level_profile_map_;
  };

  // DumpRWLevelTask impl
  template<typename KeyType, typename KeySerializerType>
  LevelProfileMap<KeyType, KeySerializerType>::
  DumpRWLevelTask::DumpRWLevelTask(
    Generics::TaskRunner* task_runner,
    LevelProfileMap<KeyType, KeySerializerType>* level_profile_map,
    bool periodic)
    noexcept
    : Generics::TaskGoal(task_runner),
      level_profile_map_(ReferenceCounting::add_ref(level_profile_map)),
      periodic_(periodic)
  {}

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::
  DumpRWLevelTask::execute()
    noexcept
  {
    Generics::Time now = Generics::Time::get_time_of_day();

    typename LevelProfileMap<KeyType, KeySerializerType>::
      OpSyncPolicy::WriteGuard rw_level_lock(
        level_profile_map_->rw_level_lock_);
    if(level_profile_map_->rw_level_dump_check_by_time_i_(now))
    {
      level_profile_map_->exchange_rw_level_i_();
    }
  }

  // DumpMemLevelTask impl
  template<typename KeyType, typename KeySerializerType>
  LevelProfileMap<KeyType, KeySerializerType>::
  DumpMemLevelTask::DumpMemLevelTask(
    Generics::TaskRunner* task_runner,
    LevelProfileMap<KeyType, KeySerializerType>* level_profile_map,
    unsigned long index,
    unsigned long sub_index)
    noexcept
    : Generics::TaskGoal(task_runner),
      level_profile_map_(ReferenceCounting::add_ref(level_profile_map)),
      index_(index),
      sub_index_(sub_index)
  {}

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::
  DumpMemLevelTask::execute()
    noexcept
  {
    try
    {
      //std::cerr << "PS:DumpMemLevelTask::execute()" << std::endl;
      level_profile_map_->dump_mem_level_(index_, sub_index_);
    }
    catch (const eh::Exception& ex)
    {
      // reschedule dump
      level_profile_map_->planner_->schedule(
        this,
        Generics::Time::get_time_of_day() + DUMP_RETRY_PERIOD);

      level_profile_map_->callback_->report_error(
        Generics::ActiveObjectCallback::CRITICAL_ERROR,
        String::SubString(ex.what()));
    }
  }

  // MergeLevelTask impl
  template<typename KeyType, typename KeySerializerType>
  LevelProfileMap<KeyType, KeySerializerType>::
  MergeLevelTask::MergeLevelTask(
    Generics::TaskRunner* task_runner,
    LevelProfileMap<KeyType, KeySerializerType>* level_profile_map)
    noexcept
    : Generics::TaskGoal(task_runner),
      level_profile_map_(ReferenceCounting::add_ref(level_profile_map))
  {}

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::
  MergeLevelTask::execute()
    noexcept
  {
    level_profile_map_->merge_levels_();
  }

  // ClearExpiredTask impl
  template<typename KeyType, typename KeySerializerType>
  LevelProfileMap<KeyType, KeySerializerType>::
  ClearExpiredTask::ClearExpiredTask(
    LevelProfileMap<KeyType, KeySerializerType>* level_profile_map)
    noexcept
    : level_profile_map_(ReferenceCounting::add_ref(level_profile_map))
  {}

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::
  ClearExpiredTask::execute()
    noexcept
  {
  }

  class IndexFileChecker
  {
  public:
    IndexFileChecker(
      unsigned int& index_number,
      const std::string& file_prefix)
      noexcept
      : index_number_(index_number),
        file_prefix_(file_prefix)
    {}

    bool
    operator()(const char* file_path, const struct stat& /*st*/)
      noexcept
    {
      if(::strncmp(file_path,
        file_prefix_.c_str(), file_prefix_.size()) == 0)
      {
        if(::strncmp(file_path + (::strlen(file_path) - ::strlen(".index")),
            ".index", ::strlen(".index")) == 0)
        {
          ++index_number_;
        }
      }

      return true;
    }

  private:
    unsigned int& index_number_;
    const std::string file_prefix_;
  };

  // LevelProfileMap::LevelHolder
  template<typename KeyType, typename KeySerializerType>
  LevelProfileMap<KeyType, KeySerializerType>::LevelHolder::LevelHolder()
    noexcept
    : index(0),
      sub_index(0),
      uniq_index(0),
      to_remove(false),
      backup(false)
  {}

  template<typename KeyType, typename KeySerializerType>
  LevelProfileMap<KeyType, KeySerializerType>::LevelHolder::~LevelHolder()
    noexcept
  {
    read_level = nullptr;

    if(backup)
    {
      const std::string new_index_file_name = full_index_file_name + ".bak";
      if(::rename(full_index_file_name.c_str(), new_index_file_name.c_str()) < 0)
      {
        std::cerr << "can't rename '" << full_index_file_name <<
          "' to '" << new_index_file_name << std::endl;
      }

      const std::string new_body_file_name = full_body_file_name + ".bak";
      if(::rename(full_body_file_name.c_str(), new_body_file_name.c_str()) < 0)
      {
        std::cerr << "can't rename '" << full_body_file_name <<
          "' to '" << new_body_file_name << std::endl;
      }
    }
    else if(to_remove)
    {
      ::unlink(full_index_file_name.c_str());
      ::unlink(full_body_file_name.c_str());
    }
  }

  // LevelProfileMap
  template<typename KeyType, typename KeySerializerType>
  LevelProfileMap<KeyType, KeySerializerType>::LevelProfileMap(
    Generics::ActiveObjectCallback* callback,
    const char* directory,
    const char* file_prefix,
    const LevelMapTraits& traits,
    LoadingProgressCallbackBase* progress_checker_parent)
    /*throw (eh::Exception)*/
    : file_directory_(directory),
      file_prefix_(file_prefix),
      mode_(traits.mode),
      rw_buffer_size_(traits.rw_buffer_size),
      rwlevel_max_size_(std::min(traits.rwlevel_max_size, traits.max_undumped_size)),
      max_undumped_size_(traits.max_undumped_size),
      max_levels0_(traits.max_levels0),
      max_background_undumped_size_(traits.max_undumped_size / 2),
      max_background_levels0_(traits.max_levels0 / 2),
      expire_time_(traits.expire_time),
      file_controller_(
        traits.file_controller ? ReferenceCounting::add_ref(traits.file_controller) :
        new PosixFileController()),
      callback_(ReferenceCounting::add_ref(callback)),
      planner_(new Generics::Planner(callback_)),
      task_runner_(new Generics::TaskRunner(callback_, traits.max_levels0)),
      merge_tasks_count_(0),
      stop_merge_(0),
      active_(false),
      undumped_size_(0),
      levels0_(0)
  {
    add_child_object(task_runner_);
    add_child_object(planner_);

    MapHolder_var map_holder = new MapHolder();

    LoadingProgressCallbackBase_var progress_checker;
    unsigned int index_number = 0;
    if (progress_checker_parent)
    {
      IndexFileChecker index_checker(index_number, file_prefix_ + ".");
      Generics::DirSelect::directory_selector(
        directory,
        Generics::DirSelect::wrap_functor(index_checker, true));
      progress_checker = new LoadingProgressCallback(
        progress_checker_parent,
        index_number);
    }
    else
    {
      progress_checker = new LoadingProgressCallbackBase();
    }

    LevelAssigner level_assigner(
      *this,
      map_holder,
      progress_checker,
      directory,
      (file_prefix_ + ".").c_str());

    Generics::DirSelect::directory_selector(
      directory,
      Generics::DirSelect::wrap_functor(level_assigner, true));

    map_holder->rw_level = new RWMemLevel<KeyType, KeySerializerType>();

    map_holder_.swap(map_holder);

    if(merge_check_(map_holder_))
    {
      ++merge_tasks_count_;
      Generics::Task_var merge_task(new MergeLevelTask(0, this));
      task_runner_->enqueue_task(merge_task);
    }

    progress_checker->loading_is_finished();
  }

  template<typename KeyType, typename KeySerializerType>
  LevelProfileMap<KeyType, KeySerializerType>::~LevelProfileMap() noexcept
  {}

  template<typename KeyType, typename KeySerializerType>
  typename LevelProfileMap<KeyType, KeySerializerType>::ConstMapHolder_var
  LevelProfileMap<KeyType, KeySerializerType>::get_map_holder_() const noexcept
  {
    SyncPolicy::ReadGuard lock(map_holder_lock_);
    return map_holder_;
  }

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::wait_preconditions(
    const KeyType&, OperationPriority priority) const
    /*throw(NotActive, Exception)*/
  {
    if(priority == OP_BACKGROUND)
    {
      Sync::ConditionalGuard cond_guard(
        undumped_size_change_,
        rw_level_lock_);

      while((undumped_size_ >= max_background_undumped_size_ ||
          levels0_ >= max_background_levels0_) &&
        active_)
      {
        cond_guard.wait();
      }

      if(!active_)
      {
        throw NotActive("");
      }
    }
  }

  template<typename KeyType, typename KeySerializerType>
  bool
  LevelProfileMap<KeyType, KeySerializerType>::check_profile(
    const KeyType& key)
    const /*throw(Exception)*/
  {
    ConstMapHolder_var map_holder = get_map_holder_();

    CheckProfileResult check_profile_result;

    {
      OpSyncPolicy::ReadGuard rw_level_lock(rw_level_lock_);

      check_profile_result =
        map_holder->rw_level->check_profile(key);
      if(check_profile_result.operation == PO_ERASE)
      {
        return false;
      }
      else if(check_profile_result.operation != PO_NOT_FOUND)
      {
        return true;
      }
    }

    for(typename LevelHolderArray::const_iterator
          level_it = map_holder->levels.begin();
        level_it != map_holder->levels.end(); ++level_it)
    {
      check_profile_result =
        (*level_it)->read_level->check_profile(key);
      if(check_profile_result.operation == PO_ERASE)
      {
        return false;
      }
      else if(check_profile_result.operation != PO_NOT_FOUND)
      {
        return true;
      }
    }

    return false;
  }

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::activate_object()
    /*throw (ActiveObject::Exception)*/
  {
    try
    {
      Generics::CompositeActiveObject::activate_object();
      OpSyncPolicy::WriteGuard rw_level_lock(rw_level_lock_);
      active_ = true;
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LevelProfileMap<KeyType, KeySerializerType>::activate_object(): got eh::Exception: " <<
        ex.what();
      throw ActiveObject::Exception(ostr);
    }
  }

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::deactivate_object()
    noexcept
  {
    // don't deactivate child objects,
    // because operations can enter after deactivation
    // but do dump for accelarate wait_object
    OpSyncPolicy::WriteGuard rw_level_lock(rw_level_lock_);
    exchange_rw_level_i_();
  }

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::wait_object()
    noexcept
  {
    // trust that all operations to storage finished
    // set active_ marker for interrupt unfinished operations
    // TODO: use SimpleActiveObject instead marker
    //   for do it real ActiveObject : deactivate wakeup wait
    {
      OpSyncPolicy::WriteGuard rw_level_lock(rw_level_lock_);
      active_ = false;
    }
    undumped_size_change_.broadcast();
    dump();
    Generics::CompositeActiveObject::deactivate_object();
    Generics::CompositeActiveObject::wait_object();
    Generics::CompositeActiveObject::clear();
  }

  template<typename KeyType, typename KeySerializerType>
  Generics::ConstSmartMemBuf_var
  LevelProfileMap<KeyType, KeySerializerType>::get_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
    /*throw(Exception)*/
  {
    ConstMapHolder_var map_holder = get_map_holder_();

    GetProfileResult get_profile_result;

    {
      OpSyncPolicy::ReadGuard rw_level_lock(rw_level_lock_);

      get_profile_result = map_holder->rw_level->get_profile(key);
      if(get_profile_result.operation == PO_ERASE)
      {
        return Generics::ConstSmartMemBuf_var();
      }
      else if(get_profile_result.mem_buf.in())
      {
        if(last_access_time)
        {
          *last_access_time = get_profile_result.access_time;
        }

        return get_profile_result.mem_buf;
      }
    }

    for(typename LevelHolderArray::const_iterator
          level_it = map_holder->levels.begin();
        level_it != map_holder->levels.end(); ++level_it)
    {
      get_profile_result = (*level_it)->read_level->get_profile(key);
      if(get_profile_result.operation == PO_ERASE)
      {
        return Generics::ConstSmartMemBuf_var();
      }
      else if(get_profile_result.mem_buf.in())
      {
        if(last_access_time)
        {
          *last_access_time = get_profile_result.access_time;
        }

        return get_profile_result.mem_buf;
      }
    }

    return Generics::ConstSmartMemBuf_var();
  }

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::save_profile(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now,
    OperationPriority op_priority)
    /*throw(NotActive, Blocked, Exception)*/
  {
    static const char* FUN = "LevelProfileMap<>::save_profile()";

    if (mem_buf->membuf().size() > std::numeric_limits<uint32_t>::max())
    {
      Stream::Error ostr;
      ostr << FUN << ": max size exceeded, size = " <<
        mem_buf->membuf().size();
      throw Exception(ostr);
    }

    bool signal_undumped_size_change = false;
    Generics::ConstSmartMemBuf_var destroy_mem_buf;

    {
      Sync::ConditionalGuard cond_guard(
        undumped_size_change_,
        rw_level_lock_);

      while(undumped_size_ >= max_undumped_size_ ||
        levels0_ >= max_levels0_)
      {
        if(!active_)
        {
          throw NotActive("");
        }

        if(op_priority == OP_RUNTIME && mode_ == LevelMapTraits::NONBLOCK_RUNTIME)
        {
          throw Blocked(
            levels0_ >= max_levels0_ ? 
              "max_levels0 lock" :
              "max_undumped_size lock");
        }

        /*
        std::cerr << "PS:blocked at save, "
          "undumped_size_ = " << undumped_size_ <<
          ", levels0_ = " << levels0_ <<
          ", max_undumped_size_ = " << max_undumped_size_ <<
          ", max_levels0_ = " << max_levels0_ <<
          std::endl;
        */
        cond_guard.wait();
      }

      /*
      std::cerr << "save_profile: undumped_size_ = " <<
        undumped_size_ << "(max_undumped_size_ = " <<
        max_undumped_size_ << ")" << std::endl;
      */

      ConstMapHolder_var map_holder = get_map_holder_();
      unsigned long prev_area_size = map_holder->rw_level->area_size();
      destroy_mem_buf = map_holder->rw_level->save_profile(
        key, mem_buf, PO_REWRITE, 0, now);
      uint64_t new_area_size = map_holder->rw_level->area_size();
      uint64_t prev_undumped_size = undumped_size_;
      undumped_size_ += new_area_size;
      undumped_size_ -= prev_area_size;

      signal_undumped_size_change = (
        prev_undumped_size >= max_undumped_size_ &&
        undumped_size_ < max_undumped_size_) ||
        (prev_undumped_size != 0 && undumped_size_ == 0);

      /*
      std::cerr << "PS: undumped_size_(1): " << prev_undumped_size <<
        "=>" << undumped_size_ <<
        "(max_undumped_size_ = " << max_undumped_size_ <<
        ", new_area_size = " << new_area_size <<
        ", prev_area_size = " << prev_area_size <<
        ", rw_level->area_size = " << map_holder->rw_level->area_size() <<
        ", rwlevel_max_size_ = " << rwlevel_max_size_ <<
        ", signal_undumped_size_change = " << signal_undumped_size_change <<
        ")" <<
        std::endl;
      */
      if(rw_level_dump_check_by_size_i_(map_holder))
      {
        exchange_rw_level_i_();
      }
    }

    if(signal_undumped_size_change)
    {
      undumped_size_change_.broadcast();
    }
  }
    
  template<typename KeyType, typename KeySerializerType>
  bool
  LevelProfileMap<KeyType, KeySerializerType>::remove_profile(
    const KeyType& key,
    OperationPriority op_priority)
    /*throw(NotActive, Blocked, Exception)*/
  {
    bool signal_undumped_size_change = false;

    {
      Sync::ConditionalGuard cond_guard(
        undumped_size_change_,
        rw_level_lock_);

      while(undumped_size_ >= max_undumped_size_ ||
        levels0_ >= max_levels0_)
      {
        if(!active_)
        {
          throw NotActive("");
        }

        if(op_priority == OP_RUNTIME && mode_ == LevelMapTraits::NONBLOCK_RUNTIME)
        {
          throw Blocked(
            levels0_ >= max_levels0_ ? 
              "max_levels0 lock" :
              "max_undumped_size lock");
        }

        cond_guard.wait();
      }

      ConstMapHolder_var map_holder = get_map_holder_();
      unsigned long prev_area_size = map_holder->rw_level->area_size();
      map_holder->rw_level->remove_profile(key, 0);
      unsigned long new_area_size = map_holder->rw_level->area_size();
      unsigned long prev_undumped_size = undumped_size_;
      undumped_size_ += new_area_size;
      undumped_size_ -= prev_area_size;

      signal_undumped_size_change =
        (prev_undumped_size >= max_undumped_size_ &&
         undumped_size_ < max_undumped_size_) ||
        (prev_undumped_size != 0 && undumped_size_ == 0);

      /*
      std::cerr << "undumped_size_(2): " << prev_undumped_size <<
        "=>" << undumped_size_ << std::endl;
      */
      if(rw_level_dump_check_by_size_i_(map_holder))
      {
        exchange_rw_level_i_();
      }
    }

    if(signal_undumped_size_change)
    {
      undumped_size_change_.broadcast();
    }

    return true;
  }

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::copy_keys(
    typename ProfileMap<KeyType>::KeyList& keys)
    /*throw(Exception)*/
  {
    ConstMapHolder_var map_holder = get_map_holder_();

    std::list<typename ReadBaseLevel<KeyType>::KeyIterator_var> its;

    {
      OpSyncPolicy::ReadGuard rw_level_lock(rw_level_lock_);
      its.push_back(map_holder->rw_level->get_key_iterator());
    }

    for(typename LevelHolderArray::const_iterator level_it =
          map_holder->levels.begin();
        level_it != map_holder->levels.end(); ++level_it)
    {
      its.push_back(
        (*level_it)->read_level->get_key_iterator());
    }

    typename ReadBaseLevel<KeyType>::KeyIterator_var merge_it =
      new KeyMergeIterator<KeyType>(its);

    typename ReadBaseLevel<KeyType>::KeyIterator_var it =
      new KeyOperationPackIterator<KeyType>(merge_it);

    KeyType cur_key;
    ProfileOperation cur_operation;
    Generics::Time cur_access_time;
    while(it->get_next(cur_key, cur_operation, cur_access_time))
    {
      keys.push_back(cur_key);
    }
  }

  template<typename KeyType, typename KeySerializerType>
  unsigned long
  LevelProfileMap<KeyType, KeySerializerType>::size()
    const noexcept
  {
    ConstMapHolder_var map_holder = get_map_holder_();

    unsigned long size;

    {
      OpSyncPolicy::ReadGuard rw_level_lock(rw_level_lock_);
      size = map_holder->rw_level->size();
    }

    for(typename LevelHolderArray::const_iterator
          level_it = map_holder->levels.begin();
        level_it != map_holder->levels.end(); ++level_it)
    {
      size += (*level_it)->read_level->size();
    }

    return size;
  }

  template<typename KeyType, typename KeySerializerType>
  unsigned long
  LevelProfileMap<KeyType, KeySerializerType>::area_size()
    const noexcept
  {
    ConstMapHolder_var map_holder = get_map_holder_();

    unsigned long size;

    {
      OpSyncPolicy::WriteGuard rw_level_lock(rw_level_lock_);
      size = map_holder->rw_level->area_size();
    }

    for(typename LevelHolderArray::const_iterator
          level_it = map_holder->levels.begin();
        level_it != map_holder->levels.end(); ++level_it)
    {
      size += (*level_it)->read_level->area_size();
    }

    return size;
  }

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::dump()
    noexcept
  {
    {
      OpSyncPolicy::WriteGuard rw_level_lock(rw_level_lock_);
      exchange_rw_level_i_();
    }

    {
      // wait when all records will be dumped
      Sync::ConditionalGuard cond_guard(
        undumped_size_change_,
        rw_level_lock_);
      while(undumped_size_ > 0)
      {
        cond_guard.wait();
      }
    }

    {
      // wait when all merge tasks will be finished
      Sync::ConditionalGuard cond_guard(
        merge_tasks_count_change_,
        merge_tasks_count_lock_);
      stop_merge_ = 1;
      while(merge_tasks_count_ > 0)
      {
        cond_guard.wait();
      }
      stop_merge_ = 0;
    }
  }

  template<typename KeyType, typename KeySerializerType>
  bool
  LevelProfileMap<KeyType, KeySerializerType>::signal_by_undumped_size_(
    unsigned long prev_undumped_size,
    unsigned long new_undumped_size)
    const noexcept
  {
    return (prev_undumped_size >= max_undumped_size_ &&
        new_undumped_size < max_undumped_size_) ||
      (prev_undumped_size >= max_background_undumped_size_ &&
        new_undumped_size < max_background_undumped_size_) ||
      (prev_undumped_size != 0 && new_undumped_size == 0);
  }

  template<typename KeyType, typename KeySerializerType>
  bool
  LevelProfileMap<KeyType, KeySerializerType>::signal_by_levels_change_(
    unsigned long prev_levels0,
    unsigned long new_levels0)
    const noexcept
  {
    return (new_levels0 < max_levels0_ && prev_levels0 >= max_levels0_) ||
      (new_levels0 < max_background_levels0_ &&
        prev_levels0 >= max_background_levels0_);
  }

  template<typename KeyType, typename KeySerializerType>
  template<typename IteratorType, typename MapHolderType>
  IteratorType
  LevelProfileMap<KeyType, KeySerializerType>::find_level_(
    MapHolderType* map_holder,
    unsigned long index,
    unsigned long sub_index)
    noexcept
  {
    for(IteratorType it = map_holder->levels.begin();
        it != map_holder->levels.end(); ++it)
    {
      if((*it)->index == index &&
         (*it)->sub_index == sub_index)
      {
        return it;
      }
    }

    return map_holder->levels.end();
  }

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::add_level_(
    MapHolder* map_holder,
    LoadingProgressCallbackBase_var progress_checker,
    const char* /*directory*/,
    const char* index_file_name)
    /*throw(Exception)*/
  {
    static const char* FUN = "LevelProfileMap<>::add_level_()";

    try
    {
      String::SubString prefix;
      String::SubString index_str;
      String::SubString sub_index_str;
      String::SubString uniq_index_str;
      String::SubString suffix;
      String::SubString add_suffix;
      unsigned long index;
      unsigned long sub_index;
      String::SubString index_file_name_substr(index_file_name);
      typename String::StringManip::Splitter<
        String::AsciiStringManip::SepPeriod> tokenizer(
          index_file_name_substr);

      if(tokenizer.get_token(prefix) &&
         tokenizer.get_token(index_str) &&
         tokenizer.get_token(sub_index_str) &&
         tokenizer.get_token(uniq_index_str) &&
         tokenizer.get_token(suffix) &&
         (uniq_index_str == "index" || suffix == "index") &&
         String::StringManip::str_to_int(index_str, index) &&
         String::StringManip::str_to_int(sub_index_str, sub_index) &&
         // skip files with additional suffix (.bak, ...)
         (!tokenizer.get_token(add_suffix) || add_suffix.empty())
         )
      {
        unsigned long uniq_index = 0;

        // open level
        std::string body_file_name;
        prefix.assign_to(body_file_name);
        body_file_name += ".";
        index_str.append_to(body_file_name);
        body_file_name += ".";
        sub_index_str.append_to(body_file_name);
        if(!uniq_index_str.empty() && uniq_index_str != "index")
        {
          String::StringManip::str_to_int(uniq_index_str, uniq_index);

          body_file_name += ".";
          uniq_index_str.append_to(body_file_name);
        }
        body_file_name += ".data";

        std::string full_index_file_name = file_directory_ + "/" + index_file_name;
        std::string full_body_file_name = file_directory_ + "/" + body_file_name;

        ReferenceCounting::SmartPtr<ReadFileLevel<KeyType, KeySerializerType> >
          read_level = new ReadFileLevel<KeyType, KeySerializerType>(
            full_index_file_name.c_str(),
            full_body_file_name.c_str(),
            rw_buffer_size_,
            true, // disable caching for all levels for case when we open file on init
            file_controller_,
            progress_checker);

        if(index == 0)
        {
          levels0_ += 1;
        }

        LevelHolder_var level_holder = new LevelHolder();
        level_holder->index = index;
        level_holder->sub_index = sub_index;
        level_holder->uniq_index = uniq_index;
        level_holder->read_level = read_level;
        level_holder->full_index_file_name.swap(full_index_file_name);
        level_holder->full_body_file_name.swap(full_body_file_name);
        typename LevelHolderArray::iterator ins_it = std::lower_bound(
          map_holder->levels.begin(),
          map_holder->levels.end(),
          level_holder,
          LevelHolderPtrLess());
        map_holder->levels.insert(ins_it, level_holder);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": on opening files by '" <<
        index_file_name << "', caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename KeyType, typename KeySerializerType>
  std::string
  LevelProfileMap<KeyType, KeySerializerType>::index_merge_file_name_()
    const noexcept
  {
    std::ostringstream ostr;
    ostr << this->file_directory_ << "/" << this->file_prefix_ << ".merge.index";
    return ostr.str();
  }

  template<typename KeyType, typename KeySerializerType>
  std::string
  LevelProfileMap<KeyType, KeySerializerType>::body_merge_file_name_()
    const noexcept
  {
    std::ostringstream ostr;
    ostr << this->file_directory_ << "/" << this->file_prefix_ << ".merge.data";
    return ostr.str();
  }

  template<typename KeyType, typename KeySerializerType>
  typename LevelProfileMap<KeyType, KeySerializerType>::FileNames
  LevelProfileMap<KeyType, KeySerializerType>::index_file_name_(
    unsigned long index,
    unsigned long sub_index,
    unsigned long uniq_index)
    const noexcept
  {
    std::ostringstream ostr;
    ostr << this->file_directory_ << "/" << this->file_prefix_ << "." <<
      std::setw(8) << std::setfill('0') << index << "." <<
      std::setw(8) << sub_index << "." <<
      std::setw(8) << uniq_index << ".index";

    FileNames names;
    names.regular = ostr.str();

    ostr << DUMP_SUFFIX;
    names.dump_temporary = ostr.str();

    return names;
  }

  template<typename KeyType, typename KeySerializerType>
  typename LevelProfileMap<KeyType, KeySerializerType>::FileNames
  LevelProfileMap<KeyType, KeySerializerType>::body_file_name_(
    unsigned long index,
    unsigned long sub_index,
    unsigned long uniq_index)
    const noexcept
  {
    std::ostringstream ostr;
    ostr << this->file_directory_ << "/" << this->file_prefix_ << "." <<
      std::setw(8) << std::setfill('0') << index << "." <<
      std::setw(8) << sub_index << "." <<
      std::setw(8) << uniq_index << ".data";

    FileNames names;
    names.regular = ostr.str();

    ostr << DUMP_SUFFIX;
    names.dump_temporary = ostr.str();

    return std::move(names);
  }

  template<typename KeyType, typename KeySerializerType>
  bool
  LevelProfileMap<KeyType, KeySerializerType>::merge_check_(
    const MapHolder* map_holder)
    noexcept
  {
    bool dumped_level0_found = false;

    for(typename LevelHolderArray::const_iterator it =
          map_holder->levels.begin();
        it != map_holder->levels.end() && (*it)->index == 0; ++it)
    {
      if(!(*it)->full_index_file_name.empty())
      {
        if(dumped_level0_found)
        {
          // two dumped levels 0 exists
          return true;
        }

        dumped_level0_found = true;
      }
    }

    return false;
  }

  template<typename KeyType, typename KeySerializerType>
  bool
  LevelProfileMap<KeyType, KeySerializerType>::rw_level_dump_check_by_size_i_(
    const MapHolder* map_holder)
    const noexcept
  {
    return map_holder->rw_level->area_size() >= rwlevel_max_size_;
  }

  template<typename KeyType, typename KeySerializerType>
  bool
  LevelProfileMap<KeyType, KeySerializerType>::rw_level_dump_check_by_time_i_(
    const Generics::Time& now)
    const noexcept
  {
    return last_rw_level_dump_time_ + max_dump_period_ <= now;
  }

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::exchange_rw_level_i_()
    /*throw(Exception)*/
  {
    //static const char* FUN = "LevelProfileMap<>::exchange_rw_level_i_()";

    //std::cerr << "PS:>exchange_rw_level_i_" << std::endl;

    //Generics::Time now = Generics::Time::get_time_of_day();
    unsigned long new_sub_index = 0;
    ReferenceCounting::SmartPtr<
      RWMemLevel<KeyType, KeySerializerType> > destroy_rw_level;
    bool dump_mem_level = false;

    {
      MapHolderChangeSyncPolicy::WriteGuard map_holder_change_lock(
        map_holder_change_lock_);

      ConstMapHolder_var map_holder = get_map_holder_();

      if(map_holder->rw_level->area_size() > 0)
      {
        dump_mem_level = true;

        MapHolder_var new_map_holder = new MapHolder(*map_holder);
        /*
        std::cerr << FUN << ": new_map_holder->levels.size() = " <<
          new_map_holder->levels.size() << std::endl;
        */
        typename LevelHolderArray::iterator level_holder_it =
          new_map_holder->levels.begin();

        if(level_holder_it != new_map_holder->levels.end())
        {
          new_sub_index = (*level_holder_it)->sub_index + 1;
        }

        LevelHolder_var new_level_holder = new LevelHolder();
        new_level_holder->index = 0;
        new_level_holder->sub_index = new_sub_index;
        new_level_holder->uniq_index = Generics::safe_rand() % 1000000;

        ReferenceCounting::SmartPtr<
          RWMemLevel<KeyType, KeySerializerType> > empty_rw_level =
            new RWMemLevel<KeyType, KeySerializerType>();

        new_level_holder->read_level =
          map_holder->rw_level->convert_to_read_mem_level();
        new_map_holder->rw_level.swap(empty_rw_level);
        destroy_rw_level.swap(empty_rw_level);
        new_map_holder->levels.insert(level_holder_it, new_level_holder);
        levels0_ += 1;

        SyncPolicy::WriteGuard map_holder_lock(map_holder_lock_);
        map_holder_.swap(new_map_holder);
      }
    }

    //std::cerr << "PS:<exchange_rw_level_i_(0, " << new_sub_index << ")" << std::endl;

    if(dump_mem_level)
    {
      // set dump Mem Level task
      //std::cerr << "PS:create DumpMemLevelTask(0, " << new_sub_index << ")" << std::endl;

      Generics::Task_var task(new DumpMemLevelTask(
        0, // task runner
        this,
        0,
        new_sub_index));
      task_runner_->enqueue_task(task);
    }
  }

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::dump_mem_level_(
    unsigned long index,
    unsigned long sub_index)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "LevelProfileMap<>::dump_mem_level_()";

    bool do_merge = false;
    unsigned long level_area_size = 0;

    {
      ConstMapHolder_var map_holder = get_map_holder_();

      /*
      {
        Stream::Error ostr;
        ostr << "PS:>dump_mem_level_(" << index << ", " << sub_index << ")" << std::endl;
        print_levels_(ostr, map_holder);
        std::cerr << ostr.str().str() << std::endl;
      }
      */

      typename LevelHolderArray::const_iterator level_it =
        this->find_level_<
          typename LevelHolderArray::const_iterator, const MapHolder>(
            map_holder, index, sub_index);

      if(level_it != map_holder->levels.end())
      {
        assert((*level_it)->full_index_file_name.empty());
        level_area_size = (*level_it)->read_level->area_size();

        /*
        std::cerr << "PS:>dump_mem_level_ into '" << index_file_name_(index, sub_index) <<
          "'" << std::endl;
        */
        const unsigned long res_uniq_index = Generics::safe_rand() % 1000000;
        FileNames full_index_file_name = index_file_name_(
          index, sub_index, res_uniq_index);
        FileNames full_body_file_name = body_file_name_(
          index, sub_index, res_uniq_index);

        ReferenceCounting::SmartPtr<ReadFileLevel<KeyType, KeySerializerType> >
          new_file_level;

        try
        {
          // allow caching on full fetch,
          // because we have big probability that it will be used on merge
          // in near time
          new_file_level = new ReadFileLevel<KeyType, KeySerializerType>(
            full_index_file_name.dump_temporary.c_str(),
            full_body_file_name.dump_temporary.c_str(),
            create_filter_iterator_(
              (*level_it)->read_level->get_iterator(0)),
            rw_buffer_size_,
            false, // allow caching
            0,
            file_controller_);

          new_file_level->rename_files(
            full_index_file_name.regular.c_str(),
            full_body_file_name.regular.c_str());
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": can't create file level, caught eh::Exception: " <<
            ex.what();
          throw Exception(ostr);
        }

        MapHolderChangeSyncPolicy::WriteGuard map_holder_change_lock(
          map_holder_change_lock_);

        MapHolder_var new_map_holder = new MapHolder(*get_map_holder_());
        typename LevelHolderArray::iterator modify_level_it =
          find_level_<typename LevelHolderArray::iterator, MapHolder>(
            new_map_holder, index, sub_index);

        assert(modify_level_it != new_map_holder->levels.end());

        LevelHolder_var new_level_holder = new LevelHolder(**modify_level_it);
        new_level_holder->uniq_index = res_uniq_index;
        new_level_holder->read_level = new_file_level;
        new_level_holder->full_index_file_name.swap(full_index_file_name.regular);
        new_level_holder->full_body_file_name.swap(full_body_file_name.regular);
        *modify_level_it = new_level_holder;

        do_merge |= merge_check_(new_map_holder);

        SyncPolicy::WriteGuard map_holder_lock(map_holder_lock_);
        map_holder_.swap(new_map_holder);

        /*
        {
          Stream::Error ostr;
          ostr << "<dump_mem_level_(" << index << ", " << sub_index << ")" << std::endl;
          print_levels_(ostr, map_holder_);
          std::cerr << ostr.str().str() << std::endl;
        }
        */
      }
    }

    //std::cerr << "PS:<dump_mem_level_(" << index << ", " << sub_index << ")" << std::endl;

    // change undumped_size
    bool signal_undumped_size_change = false;

    {
      OpSyncPolicy::WriteGuard rw_level_lock(rw_level_lock_);
      unsigned long prev_undumped_size = undumped_size_;
      undumped_size_ -= level_area_size;
      signal_undumped_size_change = signal_by_undumped_size_(
        prev_undumped_size,
        undumped_size_);

      /*
      std::cerr << "PS:undumped_size_(3): " << prev_undumped_size <<
        "=>" << undumped_size_ << ": " <<
        signal_undumped_size_change << std::endl;
      */
    }

    if(signal_undumped_size_change)
    {
      undumped_size_change_.broadcast();
    }

    // do merging if level 0 have two files (dumped levels)
    if(do_merge)
    {
      {
        // block concurrent merge
        MergeTaskCountSyncPolicy::WriteGuard merge_tasks_count_lock(
          merge_tasks_count_lock_);
        if(merge_tasks_count_ >= 1)
        {
          do_merge = false;
        }
        else
        {
          ++merge_tasks_count_;
        }
      }

      if(do_merge)
      {
        Generics::Task_var merge_task(new MergeLevelTask(0, this));
        task_runner_->enqueue_task(merge_task);
      }
    }
  }
  
  template<typename ContainerType>
  struct PresentInSet
  {
  public:
    PresentInSet(const ContainerType& cont)
      : cont_(cont)
    {}

    bool
    operator()(const typename ContainerType::value_type& val)
    {
      return cont_.find(val) != cont_.end();
    };

  private:
    const ContainerType& cont_;
  };

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::merge_levels_()
    noexcept
  {
    static const char* FUN = "LevelProfileMap<>::merge_levels_()";

    typedef std::set<LevelHolder_var, LevelHolderPtrLess> LevelHolderSet;

    bool do_merge = false;
    bool merge_failed = false;

    try
    {
      // find levels for merge - sequence of filled levels
      ConstMapHolder_var map_holder = get_map_holder_();

      // merged levels will be removed at destroy of this set (outside locks)
      LevelHolderSet merge_levels;
      LevelHolderSet corrupted_levels;
      unsigned long merge_levels0 = 0;
      std::list<typename ReadBaseLevel<KeyType>::Iterator_var> its;

      unsigned long last_index = 0;
      uint64_t sum_area_size = 0;
      for(typename LevelHolderArray::const_iterator it =
            map_holder->levels.begin();
          it != map_holder->levels.end(); ++it)
      {
        if((*it)->full_index_file_name.empty())
        {
          // level in RAM (undumped), actual only for 0 levels
          sum_area_size = 0;
          merge_levels.clear();
          merge_levels0 = 0;
        }
        else
        {
          if((*it)->index != last_index && (*it)->index != last_index + 1 &&
               // found empty level (last_index + 1)
             sum_area_size < rwlevel_max_size_ * (1 << ((*it)->index - 1))
             )
          {
            // merged level guaranteed can be placed in this cell or before
            break;
          }

          if((*it)->index == 0)
          {
            ++merge_levels0;
          }
          merge_levels.insert(*it);
          last_index = (*it)->index;
          sum_area_size += (*it)->read_level->area_size();
        }
      }

      if(merge_levels.size() > 1)
      {
        for(typename LevelHolderSet::const_iterator level_it =
              merge_levels.begin();
            level_it != merge_levels.end();)
        {
          try
          {
            its.push_back(
              (*level_it)->read_level->get_iterator(
                rw_buffer_size_ / merge_levels.size()));

            ++level_it;
          }
          catch(const eh::Exception& ex)
          {
            // one of source levels corrupted, removed or have incorrect permissions
            // log error and continue without it
            Stream::Error ostr;
            ostr << FUN << ": can't fetch level '" <<
              (*level_it)->full_index_file_name << "': " << ex.what();
            callback_->report_error(
              Generics::ActiveObjectCallback::CRITICAL_ERROR,
              ostr.str());

            corrupted_levels.insert(*level_it);
            merge_levels.erase(level_it++);
          }
        }

        // create new level that is result of merging
        std::string index_merge_file_name = index_merge_file_name_();
        std::string body_merge_file_name = body_merge_file_name_();
        uint64_t new_area_size = 0;

        typename ReadBaseLevel<KeyType>::Iterator_var merge_iterator =
          create_filter_iterator_(
            typename ReadBaseLevel<KeyType>::Iterator_var(
              new OperationPackIterator<KeyType>(
                typename ReadBaseLevel<KeyType>::Iterator_var(
                  new MergeIterator<KeyType>(its)))));

        ReferenceCounting::SmartPtr<ReadFileLevel<KeyType, KeySerializerType> >
          new_level = new ReadFileLevel<KeyType, KeySerializerType>(
            index_merge_file_name.c_str(),
            body_merge_file_name.c_str(),
            merge_iterator,
            rw_buffer_size_,
            true, // disable caching
            &stop_merge_,
            file_controller_);

        new_area_size = new_level->area_size();

        // determine new file index
        unsigned long res_index = 0;
        unsigned long res_sub_index = 0;
        uint64_t index_area_size = rwlevel_max_size_;
        while(index_area_size < new_area_size)
        {
          ++res_index;
          index_area_size *= 2;
        }

        std::cerr << "NEW INDEX=" << res_index << ", new_area_size = " << new_area_size <<
          ", rwlevel_max_size_ = " << rwlevel_max_size_ << std::endl;

        // new index must be placed inside removed indexes interval
        if(res_index <= (*merge_levels.begin())->index)
        {
          res_index = (*merge_levels.begin())->index;
          res_sub_index = (*merge_levels.begin())->sub_index;
        }

        unsigned long res_uniq_index = Generics::safe_rand() % 1000000;

        {
          MapHolderChangeSyncPolicy::WriteGuard map_holder_change_lock(
            map_holder_change_lock_);

          MapHolder_var new_map_holder = new MapHolder(*get_map_holder_());

          typename LevelHolderArray::iterator remove_it =
            std::remove_if(
              new_map_holder->levels.begin(),
              new_map_holder->levels.end(),
              PresentInSet<LevelHolderSet>(merge_levels));

          new_map_holder->levels.erase(remove_it, new_map_holder->levels.end());

          std::string full_index_file_name =
            index_file_name_(res_index, res_sub_index, res_uniq_index).regular;
          std::string full_body_file_name =
            body_file_name_(res_index, res_sub_index, res_uniq_index).regular;
          new_level->rename_files(
            full_index_file_name.c_str(),
            full_body_file_name.c_str());

          LevelHolder_var level_holder = new LevelHolder();
          level_holder->index = res_index;
          level_holder->sub_index = res_sub_index;
          level_holder->uniq_index = res_uniq_index;
          level_holder->read_level = new_level;
          level_holder->full_index_file_name.swap(full_index_file_name);
          level_holder->full_body_file_name.swap(full_body_file_name);

          typename LevelHolderArray::iterator ins_it = std::lower_bound(
            new_map_holder->levels.begin(),
            new_map_holder->levels.end(),
            level_holder,
            LevelHolderPtrLess());
          new_map_holder->levels.insert(ins_it, level_holder);

          do_merge = merge_check_(new_map_holder);

          SyncPolicy::WriteGuard lock(this->map_holder_lock_);
          map_holder_.swap(new_map_holder);
        }

        for(typename LevelHolderSet::iterator it = merge_levels.begin();
            it != merge_levels.end(); ++it)
        {
          (*it)->to_remove = true;
        }

        for(typename LevelHolderSet::iterator it = corrupted_levels.begin();
            it != corrupted_levels.end(); ++it)
        {
          (*it)->backup = true;
        }

        bool signal_undumped_size_change = false;

        {
          OpSyncPolicy::WriteGuard rw_level_lock(rw_level_lock_);
          unsigned long prev_levels0 = levels0_;
          if(res_index == 0)
          {
            levels0_ += 1;
          }
          levels0_ -= merge_levels0;
          signal_undumped_size_change |= signal_by_levels_change_(
            prev_levels0,
            levels0_);
        }

        if(signal_undumped_size_change)
        {
          undumped_size_change_.broadcast();
        }
      }
    }
    catch(const typename ReadFileLevel<KeyType, KeySerializerType>::Interrupted&)
    {}
    catch(const eh::Exception& ex)
    {
      merge_failed = true;

      callback_->report_error(
        Generics::ActiveObjectCallback::CRITICAL_ERROR,
        String::SubString(ex.what()));
    }

    bool signal_merge_tasks_count_change = false;

    {
      // do merge recheck inside merge_tasks_count_lock_,
      // because dump_mem_level_ can cancel merge enqueue inside this lock
      // if merge already in progress
      MergeTaskCountSyncPolicy::WriteGuard merge_tasks_count_lock(
        merge_tasks_count_lock_);

      if(stop_merge_)
      {
        do_merge = false;
      }
      else if(!do_merge)
      {
        MapHolder_var cur_map_holder;

        {
          SyncPolicy::ReadGuard map_holder_lock(map_holder_lock_);
          cur_map_holder = map_holder_;
        }

        do_merge = merge_check_(cur_map_holder);
      }

      if(!do_merge)
      {
        signal_merge_tasks_count_change = (
          --merge_tasks_count_ == 0);
      }
    }

    if(do_merge)
    {
      // rerun merge
      ReferenceCounting::SmartPtr<Generics::TaskGoal> merge_task(
        new MergeLevelTask(task_runner_, this));

      if(!merge_failed)
      {
        task_runner_->enqueue_task(merge_task);
      }
      else
      {
        // if merge failed do it only after pause
        planner_->schedule(
          merge_task,
          Generics::Time::get_time_of_day() + DUMP_RETRY_PERIOD);
      }
    }

    if(signal_merge_tasks_count_change)
    {
      merge_tasks_count_change_.broadcast();
    }
  }

  template<typename KeyType, typename KeySerializerType>
  typename ReadBaseLevel<KeyType>::Iterator_var
  LevelProfileMap<KeyType, KeySerializerType>::create_filter_iterator_(
    typename ReadBaseLevel<KeyType>::Iterator* it)
    noexcept
  {
    if(expire_time_ != Generics::Time::ZERO)
    {
      Generics::Time now = Generics::Time::get_time_of_day();
      return new AccessTimeFilterIterator<KeyType>(
        it,
        now - expire_time_);
    }

    return ReferenceCounting::add_ref(it);
  }

  template<typename KeyType, typename KeySerializerType>
  void
  LevelProfileMap<KeyType, KeySerializerType>::print_levels_(
    std::ostream& ostr, const MapHolder* map_holder)
    noexcept
  {
    for(typename LevelHolderArray::const_iterator
          level_it = map_holder->levels.begin();
        level_it != map_holder->levels.end(); ++level_it)
    {
      ostr << "  (" << (*level_it)->index << "," << (*level_it)->sub_index << ")=>'" <<
        (*level_it)->full_index_file_name << "'" << std::endl;
    }
  }
}
}
