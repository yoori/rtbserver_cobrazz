#ifndef LOGCOMMONS_LOGHOLDER_HPP
#define LOGCOMMONS_LOGHOLDER_HPP

#include <string>
#include <list>
#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Stream/MemoryStream.hpp>
#include <Generics/Time.hpp>
#include <Generics/LastPtr.hpp>
#include <Generics/TaskRunner.hpp>
#include <Sync/SyncPolicy.hpp>
#include <LogCommons/ArchiveOfstream.hpp>
#include <LogCommons/GenericLogIoImpl.hpp>
#include <LogCommons/GenericLogCsvSaverImpl.hpp>

namespace AdServer
{
  namespace LogProcessing
  {
    class LogHolder: public virtual ReferenceCounting::Interface
    {
    public:
      virtual Generics::Time flush_if_required(
        const Generics::Time& now) /*throw(eh::Exception)*/ = 0;

    protected:
      virtual
      ~LogHolder() noexcept = default;
    };

    typedef ReferenceCounting::SmartPtr<LogHolder> LogHolder_var;

    struct LogFlushTraits
    {
      LogFlushTraits() {}

      LogFlushTraits(
        const Generics::Time& period_val,
        const char* out_dir_val,
        const std::optional<ArchiveParams>& archive_params_val)
        : period(period_val),
          out_dir(out_dir_val),
          archive_params(archive_params_val)
      {}

      Generics::Time period;
      std::string out_dir;
      std::optional<ArchiveParams> archive_params;
    };

    template<typename LogTraitsType>
    struct DefaultSavePolicy
    {
      typedef
        AdServer::LogProcessing::GenericLogIoHelperImpl<LogTraitsType>
        IoHelperT;

      void save(
        typename LogTraitsType::CollectorType& collector,
        const char* out_dir,
        const std::optional<ArchiveParams>& archive_params)
      {
        IoHelperT(collector).save(out_dir, archive_params);
      }
    };

    template<typename LogTraitsType>
    class DistributionSavePolicy
    {
    public:
      DistributionSavePolicy(unsigned long distrib_count = 24)
        : distrib_count_(distrib_count)
      {}

      void save(
        typename LogTraitsType::CollectorType& collector,
        const char* out_dir,
        const std::optional<ArchiveParams>& /*archive_params*/)
      {
        LogIoProxy<LogTraitsType>::save(collector, out_dir, archive_params, distrib_count_);
      }

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
        const char* out_dir,
        const std::optional<ArchiveParams>& archive_params)
      {
        SimpleLogSaver_var(new CsvSaverT(collector, out_dir, archive_params))->save();
      }
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

      void add_record(
        typename LogHolderImpl<LogTraitsType, SavePolicy>::
          CollectorT::DataT data)
        /*throw(eh::Exception)*/;

      Generics::Time flush_if_required(
        const Generics::Time& now) /*throw(eh::Exception)*/;

      ~LogHolderLimitedDataAdd() noexcept;
    protected:
      void add_record_i_(
        typename LogHolderImpl<LogTraitsType, SavePolicy>::
          CollectorT::DataT&& data)
        /*throw(eh::Exception)*/;
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
      ~CompositeLogHolder() noexcept = default;

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
        PoolObjectBase(const ContainerHolder_var& container_holder)
          : container_holder_(container_holder)
        {
          container_holder_->get_object(holders_);
        }

        virtual
        ~PoolObjectBase() noexcept
        {
          try
          {
            container_holder_->release_object(holders_);
          }
          catch (const eh::Exception& e)
          {
            std::cerr << "PoolObjectBase::~PoolObjectBase<'" <<
              LogTraitsType::log_base_name() << "'>(): " <<
              "eh::Exception caught: " << e.what() << std::endl;
          }
        }

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
        template<typename Mediator>
        void
        add_record(
          typename CollectorT::KeyT key,
          Mediator data)
        {
          (*this->holders_.begin())->collector.add(
            std::move(key),
            std::move(data));
        }

      protected:
        friend PoolObject_var LogHolderPool::get_object();

        PoolObject(const typename Base::ContainerHolder_var& container_holder):
          Base::PoolObjectBase(container_holder) {}
        virtual
        ~PoolObject() noexcept = default;
      };

      LogHolderPool(
        const LogFlushTraits& flush_traits,
        const SavePolicy& save_policy = SavePolicy())
        /*throw(eh::Exception)*/:
          LogHolderPoolBase<LogTraitsType, SavePolicy>(flush_traits, save_policy) {}

      template<typename Mediator>
      void
      add_record(
        typename CollectorT::KeyT key,
        Mediator data)
      {
        PoolObject_var pool_object = get_object();
        pool_object->add_record(std::move(key), std::move(data));
      }

      PoolObject_var
      get_object()
      {
        return new PoolObject(this->get_container_holder_());
      }

    protected:
      virtual ~LogHolderPool() noexcept {}
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
        /*throw(eh::Exception)*/
        : flush_traits_(flush_traits),
          save_policy_(save_policy),
          task_runner_(ReferenceCounting::add_ref(task_runner)),
          portions_num_(portions_num)
      {
        init_portions_(portions_);
      }

      template<typename Mediator>
      void
      add_record(
        typename CollectorT::KeyT key,
        Mediator data)
      {
        // now implemented only for two level collectors (key -> (inner_key -> val)

        for(auto inner_it = data.begin(); inner_it != data.end(); ++inner_it)
        {
          typename CollectorT::DataT inner_data;
          inner_data.add(inner_it->first, inner_it->second);
          unsigned long portion_i = inner_it->first.hash() % portions_num_;
          //std::cout << "EVAL portion for hash = " << inner_it->first.hash() << ", portions_num_ = " << portions_num_ << ", portion_i = " << portion_i << std::endl;

          SyncPolicy::WriteGuard guard(portions_[portion_i]->lock);
          portions_[portion_i]->collector.add(key, std::move(inner_data));
        }
      }

      Generics::Time
      flush_if_required(
        const Generics::Time& now) /*throw(eh::Exception)*/;

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
      virtual ~LogHolderPortioned() noexcept {}

      void
      init_portions_(PortionArray& portions) noexcept
      {
        portions.resize(portions_num_);
        for(auto it = portions.begin();  it != portions.end(); ++it)
        {
          *it = new Portion();
        }
      }

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
        void
        add_record(typename CollectorT::DataT data)
        {
          (*this->holders_.begin())->collector.add(std::move(data));
        }

      protected:
        friend PoolObject_var LogHolderPoolData::get_object();

        PoolObject(const typename Base::ContainerHolder_var& container_holder):
          Base::PoolObjectBase(container_holder) {}
        virtual
        ~PoolObject() noexcept = default;
      };

      LogHolderPoolData(
        const LogFlushTraits& flush_traits,
        const SavePolicy& save_policy = SavePolicy())
        /*throw(eh::Exception)*/:
          LogHolderPoolBase<LogTraitsType, SavePolicy>(flush_traits, save_policy) {}

      void
      add_record(typename CollectorT::DataT data)
      {
        PoolObject_var pool_object = get_object();
        pool_object->add_record(std::move(data));
      }

      PoolObject_var
      get_object()
      {
        return new PoolObject(this->get_container_holder_());
      }

    protected:
      virtual
      ~LogHolderPoolData() noexcept = default;
    };
  }
}

#include "LogHolder.tpp"

#endif /*LOGCOMMONS_LOGHOLDER_HPP*/
