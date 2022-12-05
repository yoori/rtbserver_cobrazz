#ifndef LEVELPROFILEMAP_HPP
#define LEVELPROFILEMAP_HPP

#include <vector>
#include <Sync/SyncPolicy.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/CompositeActiveObject.hpp>

#include <ProfilingCommons/ProfileMap/ProfileMap.hpp>
#include "BaseLevel.hpp"
#include "RWMemLevel.hpp"
#include "LoadingProgressCallbackBase.hpp"
#include "FileController.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  struct LevelMapTraits
  {
    enum Mode
    {
      BLOCK_RUNTIME = 0,
      NONBLOCK_RUNTIME,
    };

    LevelMapTraits(
      Mode mode_val,
      unsigned long rw_buffer_size_val,
      uint64_t rwlevel_max_size_val,
      uint64_t max_undumped_size_val,
      unsigned long max_levels0_val,
      const Generics::Time& expire_time_val,
      FileController* file_controller_val = nullptr)
      noexcept
      : mode(mode_val),
        rw_buffer_size(rw_buffer_size_val),
        rwlevel_max_size(rwlevel_max_size_val),
        max_undumped_size(max_undumped_size_val),
        max_levels0(max_levels0_val),
        expire_time(expire_time_val),
        file_controller(ReferenceCounting::add_ref(file_controller_val))
    {}

    Mode mode;
    unsigned long rw_buffer_size;
    uint64_t rwlevel_max_size;
    uint64_t max_undumped_size;
    unsigned long max_levels0;
    Generics::Time expire_time;
    FileController_var file_controller;
  };

  template<typename KeyType, typename KeySerializerType>
  class LevelProfileMap:
    public virtual ProfileMap<KeyType>,
    public virtual ReferenceCounting::AtomicImpl,
    public Generics::CompositeActiveObject
  {
  public:
    DECLARE_EXCEPTION(Exception, typename ProfileMap<KeyType>::Exception);
    DECLARE_EXCEPTION(Blocked, Exception);
    DECLARE_EXCEPTION(NotActive, Exception);

    typedef KeySerializerType KeySerializerT;

  public:
    LevelProfileMap(
      Generics::ActiveObjectCallback* callback,
      const char* directory,
      const char* file_prefix,
      const LevelMapTraits& traits,
      LoadingProgressCallbackBase* progress_checker_parent = nullptr)
      /*throw(eh::Exception)*/;

    virtual void
    wait_preconditions(const KeyType&, OperationPriority) const
      /*throw(NotActive, Exception)*/;

    virtual bool
    check_profile(const KeyType& key) const /*throw(Exception)*/;

    virtual Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0)
      /*throw(Exception)*/;

    virtual void
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME)
      /*throw(NotActive, Blocked, Exception)*/;

    virtual bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME)
      /*throw(NotActive, Blocked, Exception)*/;

    virtual void
    copy_keys(typename ProfileMap<KeyType>::KeyList& keys) /*throw(Exception)*/;

    virtual void
    clear_expired(const Generics::Time& /*expire_time*/)
      /*throw(Exception)*/
    {}

    virtual unsigned long
    size() const noexcept;

    virtual unsigned long
    area_size() const noexcept;

    bool
    empty() const noexcept;

    void
    dump() noexcept;

    // ActiveObject interface
    virtual void
    activate_object() /*throw(ActiveObject::Exception)*/;

    void
    deactivate_object() noexcept;

    void
    wait_object() noexcept;

  public:
    class LevelAssigner;

    typedef Sync::Policy::PosixThread MapHolderChangeSyncPolicy;
    typedef Sync::Policy::PosixThread MergeTaskCountSyncPolicy;
    typedef Sync::Policy::PosixThread OpSyncPolicy;
    typedef Sync::Policy::PosixThreadRW SyncPolicy;

    struct LevelHolder: public ReferenceCounting::AtomicCopyImpl
    {
      LevelHolder() noexcept;

      unsigned long index;
      unsigned long sub_index;
      unsigned long uniq_index;
      ReferenceCounting::SmartPtr<ReadBaseLevel<KeyType> > read_level;
      std::string full_index_file_name;
      std::string full_body_file_name;
      bool to_remove;
      bool backup;

    protected:
      virtual ~LevelHolder() noexcept;
    };

    struct LevelHolderPtrLess;
    
    typedef ReferenceCounting::SmartPtr<LevelHolder> LevelHolder_var;
    typedef std::vector<LevelHolder_var> LevelHolderArray;

    struct MapHolder: public ReferenceCounting::AtomicCopyImpl
    {
      ReferenceCounting::SmartPtr<
        RWMemLevel<KeyType, KeySerializerType> > rw_level;
      LevelHolderArray levels;

    protected:
      virtual ~MapHolder() noexcept
      {}
    };

    typedef ReferenceCounting::SmartPtr<MapHolder> MapHolder_var;
    typedef ReferenceCounting::ConstPtr<MapHolder> ConstMapHolder_var;
    typedef LevelProfileMap<KeyType, KeySerializerType> ThisMap;
    typedef ReferenceCounting::SmartPtr<ThisMap> ThisMap_var;

    class DumpRWLevelTask;
    class DumpMemLevelTask;
    class MergeLevelTask;
    class ClearExpiredTask;

    struct FileNames
    {
      std::string regular;
      std::string dump_temporary;
    };

  protected:
    virtual
    ~LevelProfileMap() noexcept;

    // task methods
    void
    exchange_rw_level_i_()
      /*throw(Exception)*/;

    void
    dump_mem_level_(
      unsigned long index,
      unsigned long sub_index)
      /*throw(eh::Exception)*/;

    void
    merge_levels_()
      noexcept;

    void
    add_level_(
      MapHolder* map_holder,
      LoadingProgressCallbackBase_var progress_checker_parent,
      const char* directory,
      const char* file_name)
      /*throw(Exception)*/;

    typename ReadBaseLevel<KeyType>::Iterator_var
    create_filter_iterator_(
      typename ReadBaseLevel<KeyType>::Iterator* it)
      noexcept;

    ConstMapHolder_var
    get_map_holder_() const noexcept;

    template<typename IteratorType, typename MapHolderType>
    IteratorType
    find_level_(
      MapHolderType* map_holder,
      unsigned long index,
      unsigned long sub_index)
      noexcept;

    bool
    rw_level_dump_check_by_size_i_(
      const MapHolder* map_holder)
      const noexcept;

    bool
    rw_level_dump_check_by_time_i_(
      const Generics::Time& now)
      const noexcept;

    static
    bool
    merge_check_(const MapHolder* map_holder)
      noexcept;

    FileNames
    index_file_name_(
      unsigned long index,
      unsigned long sub_index,
      unsigned long uniq_index)
      const noexcept;

    FileNames
    body_file_name_(
      unsigned long index,
      unsigned long sub_index,
      unsigned long uniq_index)
      const noexcept;

    std::string
    index_merge_file_name_()
      const noexcept;

    std::string
    body_merge_file_name_()
      const noexcept;

    static void
    print_levels_(std::ostream& ostr, const MapHolder* map_holder)
      noexcept;

    bool
    signal_by_undumped_size_(
      unsigned long prev_undumped_size,
      unsigned long new_undumped_size)
      const noexcept;

    bool
    signal_by_levels_change_(
      unsigned long prev_levels0,
      unsigned long new_levels0)
      const noexcept;

  private:
    const std::string file_directory_;
    const std::string file_prefix_;

    const LevelMapTraits::Mode mode_;
    const unsigned long rw_buffer_size_;
    const uint64_t rwlevel_max_size_;
    const Generics::Time max_dump_period_;
    const uint64_t max_undumped_size_;
    const unsigned long max_levels0_;
    const uint64_t max_background_undumped_size_;
    const unsigned long max_background_levels0_;
    const Generics::Time expire_time_;
    const FileController_var file_controller_;

    Generics::ActiveObjectCallback_var callback_;
    Generics::Planner_var planner_;
    Generics::TaskRunner_var task_runner_;

    mutable MergeTaskCountSyncPolicy::Mutex merge_tasks_count_lock_;
    mutable Sync::Conditional merge_tasks_count_change_;
    unsigned long merge_tasks_count_;
    volatile sig_atomic_t stop_merge_;

    mutable MapHolderChangeSyncPolicy::Mutex map_holder_change_lock_;

    mutable OpSyncPolicy::Mutex rw_level_lock_;
    mutable Sync::Conditional undumped_size_change_;
    bool active_;
    uint64_t undumped_size_;
    unsigned long levels0_;
    Generics::Time last_rw_level_dump_time_;

    mutable SyncPolicy::Mutex map_holder_lock_;
    MapHolder_var map_holder_;
  };
}
}

#include "LevelProfileMap.tpp"

#endif /*LEVELPROFILEMAP_HPP*/
