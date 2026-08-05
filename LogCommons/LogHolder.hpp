#pragma once

#include <string>
#include <list>
#include <utility>
#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Stream/MemoryStream.hpp>
#include <Generics/Time.hpp>
#include <Generics/LastPtr.hpp>
#include <Generics/TaskRunner.hpp>
#include <Sync/SyncPolicy.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>
#include <LogCommons/GenericLogCsvSaverImpl.hpp>
#include <LogCommons/PrimaryDump.hpp>

namespace AdServer::LogProcessing
{
  class LogHolder: public virtual ReferenceCounting::Interface
  {
  public:
    virtual Generics::Time flush_if_required(
      const Generics::Time& now) /*throw(eh::Exception)*/ = 0;

  protected:
    virtual
    ~LogHolder() noexcept;
  };

  typedef ReferenceCounting::SmartPtr<LogHolder> LogHolder_var;

  struct LogFlushTraits
  {
    LogFlushTraits();

    LogFlushTraits(
      const Generics::Time& period_val,
      const char* out_dir_val);

    Generics::Time period;
    std::string out_dir;
    PrimaryDumpPtr primary_dump;
  };

  template<typename SavePolicy, typename Collector>
  void
  save_log(
    const LogFlushTraits& flush_traits,
    SavePolicy& save_policy,
    Collector& collector)
    /*throw(eh::Exception)*/;

  template<typename LogTraitsType>
  struct DefaultSavePolicy
  {
    using IoHelperT = AdServer::LogProcessing::GenericLogIoHelperImpl<LogTraitsType>;

    void save(
      typename LogTraitsType::CollectorType& collector,
      const char* out_dir)
      /*throw(eh::Exception)*/;
  };

  template<typename LogTraitsType>
  class DistributionSavePolicy
  {
  public:
    DistributionSavePolicy(unsigned long distrib_count = 24);

    void save(
      typename LogTraitsType::CollectorType& collector,
      const char* out_dir)
      /*throw(eh::Exception)*/;

  private:
    unsigned long distrib_count_;
  };

  template<typename LogTraitsType>
  struct SimpleCsvSavePolicy
  {
    typedef
      AdServer::LogProcessing::SimpleLogCsvSaverImpl<LogTraitsType>
      CsvSaverT;

    void save(
      typename LogTraitsType::CollectorType& collector,
      const char* out_dir)
      /*throw(eh::Exception)*/;
  };

  template<
    typename LogTraitsType,
    typename SavePolicy = DefaultSavePolicy<LogTraitsType> >
  class LogHolderImpl:
    public virtual LogHolder,
    public virtual ReferenceCounting::AtomicImpl
  {
  private:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:
    typedef typename LogTraitsType::CollectorType CollectorT;
    typedef Sync::Policy::PosixThread SyncPolicy;

  public:
    LogHolderImpl(
      const LogFlushTraits& flush_traits,
      const SavePolicy& save_policy = SavePolicy())
      /*throw(eh::Exception)*/;

    Generics::Time flush_if_required(
      const Generics::Time& now) /*throw(eh::Exception)*/;

  protected:
    virtual
    ~LogHolderImpl() noexcept;

    SyncPolicy::Mutex& mutex_() noexcept;

  private:
    SyncPolicy::Mutex lock_;

  protected:
    LogFlushTraits flush_traits_;
    SavePolicy save_policy_;
    Generics::Time flush_time_;
    CollectorT collector_;
  };

  /** LogHolderLimitedDataAdd
   *    Adapter for loggers with add(DataT).
   *    Store not more than limit records.
   *    On flush write number of skipped records to Logging::Logger
   */
  template <
    typename LogTraitsType,
    typename SavePolicy = DefaultSavePolicy<LogTraitsType>
  >
  class LogHolderLimitedDataAdd:
    public LogHolderImpl<LogTraitsType, SavePolicy>
  {
  private:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    Logging::Logger_var logger_;
    const size_t limit_;
    size_t count_;
    size_t skipped_;

  public:
    LogHolderLimitedDataAdd(
      Logging::Logger* logger,
      size_t limit,
      const LogFlushTraits& flush_traits,
      const SavePolicy& save_policy = SavePolicy())
      /*throw(eh::Exception)*/;

    template<typename... Args>
    void add_record(Args&&... args)
      /*throw(eh::Exception)*/;

    Generics::Time flush_if_required(
      const Generics::Time& now) /*throw(eh::Exception)*/;

    ~LogHolderLimitedDataAdd() noexcept;

  };

  class CompositeLogHolder:
    public virtual LogHolder,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    void add_child_log_holder(LogHolder* log_holder) /*throw(eh::Exception)*/;

    virtual Generics::Time flush_if_required(
      const Generics::Time& now = Generics::Time::get_time_of_day()) /*throw(eh::Exception)*/;

  protected:
    virtual
    ~CompositeLogHolder() noexcept;

  private:
    typedef Sync::Policy::PosixThread SyncPolicy;
    typedef std::list<LogHolder_var> LogHolderList;

    SyncPolicy::Mutex lock_;
    LogHolderList child_log_holders_;
  };

  template <
    typename LogTraitsType,
    typename SavePolicy = DefaultSavePolicy<LogTraitsType> >
  class LogHolderPoolBase:
    public virtual LogHolder,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    virtual Generics::Time flush_if_required(
      const Generics::Time& now) /*throw(eh::Exception)*/;

  protected:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    class ContainerHolder;
    struct LogHolderPoolObject;

    typedef ReferenceCounting::SmartPtr<ContainerHolder> ContainerHolder_var;
    typedef Generics::LastPtr<ContainerHolder> LastContainerHolder_var;
    typedef Sync::Policy::PosixThreadRW SyncPolicy;
    typedef ReferenceCounting::SmartPtr<LogHolderPoolObject> LogHolderPoolObject_var;
    typedef std::list<LogHolderPoolObject_var> LogHolderList;

    class PoolObjectBase: public ReferenceCounting::AtomicImpl
    {
    protected:
      PoolObjectBase(const ContainerHolder_var& container_holder);

      virtual
      ~PoolObjectBase() noexcept;

      ContainerHolder_var container_holder_;
      LogHolderList holders_;
    };

  protected:
    LogHolderPoolBase(
      const LogFlushTraits& flush_traits,
      const SavePolicy& save_policy = SavePolicy())
      /*throw(eh::Exception)*/;

    virtual
    ~LogHolderPoolBase() noexcept;

    ContainerHolder_var
    get_container_holder_() const /*throw(eh::Exception)*/;

  protected:
    const LogFlushTraits flush_traits_;
    SavePolicy save_policy_;
    mutable SyncPolicy::Mutex lock_;
    ContainerHolder_var container_holder_;
    Generics::Time flush_time_;
  };

  template <
    typename LogTraitsType,
    typename SavePolicy = DefaultSavePolicy<LogTraitsType> >
  class LogHolderPool:
    public LogHolderPoolBase<LogTraitsType, SavePolicy>
  {
  public:
    class PoolObject;

    typedef typename LogTraitsType::CollectorType CollectorT;
    typedef LogHolderPoolBase<LogTraitsType, SavePolicy> Base;
    typedef ReferenceCounting::SmartPtr<PoolObject> PoolObject_var;

    class PoolObject: public Base::PoolObjectBase
    {
    public:
      template<typename... Args>
      void
      add_record(Args&&... args);

    protected:
      friend PoolObject_var LogHolderPool::get_object();

      PoolObject(const typename Base::ContainerHolder_var& container_holder);

      virtual
      ~PoolObject() noexcept;
    };

    LogHolderPool(
      const LogFlushTraits& flush_traits,
      const SavePolicy& save_policy = SavePolicy())
      /*throw(eh::Exception)*/;

    template<typename... Args>
    void
    add_record(Args&&... args);

    PoolObject_var
    get_object();

  protected:
    virtual ~LogHolderPool() noexcept;
  };

  // LogHolderPortioned
  template <
    typename LogTraitsType,
    typename SavePolicy = DefaultSavePolicy<LogTraitsType> >
  class LogHolderPortioned:
    public virtual LogHolder,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    class CollectorHolder;

    typedef typename LogTraitsType::CollectorType CollectorT;

    LogHolderPortioned(
      const LogFlushTraits& flush_traits,
      const SavePolicy& save_policy = SavePolicy(),
      Generics::TaskRunner* task_runner = 0,
      unsigned long portions_num = 16)
      /*throw(eh::Exception)*/;

    template<typename Mediator>
    void
    add_record(typename CollectorT::KeyT key, Mediator data);

    Generics::Time
    flush_if_required(const Generics::Time& now) /*throw(eh::Exception)*/;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

    struct Portion: public ReferenceCounting::AtomicImpl
    {
      mutable SyncPolicy::Mutex lock;
      CollectorT collector;
    };

    typedef ReferenceCounting::SmartPtr<Portion> Portion_var;
    typedef std::vector<Portion_var> PortionArray;

    class DumpTask :
      public Generics::Task,
      public ReferenceCounting::AtomicImpl
    {
    public:
      DumpTask(
        LogHolderPortioned<LogTraitsType, SavePolicy>* log_holder,
        Portion_var portion,
        Sync::Semaphore& sema);

      virtual void
      execute() noexcept;

    protected:
      virtual ~DumpTask() noexcept;

    protected:
      LogHolderPortioned<LogTraitsType, SavePolicy>* log_holder_;
      Portion_var portion_;
      Sync::Semaphore& sema_;
    };

  protected:
    virtual ~LogHolderPortioned() noexcept;

    void
    init_portions_(PortionArray& portions) noexcept;

  protected:
    LogFlushTraits flush_traits_;
    SavePolicy save_policy_;
    Generics::TaskRunner_var task_runner_;
    unsigned long portions_num_;

    SyncPolicy::Mutex lock_; // flush_time_ & dump lock
    Generics::Time flush_time_;
    PortionArray portions_;
  };

  // LogHolderPoolData
  template <
    typename LogTraitsType,
    typename SavePolicy = DefaultSavePolicy<LogTraitsType> >
  class LogHolderPoolData:
    public LogHolderPoolBase<LogTraitsType, SavePolicy>
  {
  public:
    class PoolObject;

    typedef typename LogTraitsType::CollectorType CollectorT;
    typedef LogHolderPoolBase<LogTraitsType, SavePolicy> Base;
    typedef ReferenceCounting::SmartPtr<PoolObject> PoolObject_var;

    class PoolObject: public Base::PoolObjectBase
    {
    public:
      template<typename... Args>
      void
      add_record(Args&&... args);

    protected:
      friend PoolObject_var LogHolderPoolData::get_object();

      PoolObject(const typename Base::ContainerHolder_var& container_holder);

      virtual
      ~PoolObject() noexcept;
    };

    LogHolderPoolData(
      const LogFlushTraits& flush_traits,
      const SavePolicy& save_policy = SavePolicy())
      /*throw(eh::Exception)*/;

    template<typename... Args>
    void
    add_record(Args&&... args);

    PoolObject_var
    get_object();

  protected:
    virtual
    ~LogHolderPoolData() noexcept;
  };
}

#include "LogHolder.tpp"
