#ifndef LOGCOMMONS_LOGHOLDER_TPP
#define LOGCOMMONS_LOGHOLDER_TPP

#include <iostream>

namespace AdServer
{
namespace LogProcessing
{
  template<typename LogTraitsType, typename SavePolicy>
  LogHolderImpl<LogTraitsType, SavePolicy>::LogHolderImpl(
    const LogFlushTraits& flush_traits,
    const SavePolicy& save_policy)
    /*throw(eh::Exception)*/
    : flush_traits_(flush_traits),
      save_policy_(save_policy)
  {}

  template<typename LogTraitsType, typename SavePolicy>
  LogHolderImpl<LogTraitsType, SavePolicy>::~LogHolderImpl() noexcept
  {
    if(!collector_.empty())
    {
      try
      {
        save_policy_.save(collector_, flush_traits_.out_dir.c_str());
      }
      catch (const eh::Exception& ex)
      {
        std::cerr << "LogHolderImpl::~LogHolderImpl<'" <<
          LogTraitsType::log_base_name() << "'>(): " <<
          "eh::Exception caught: " << ex.what() << std::endl;
      }
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  LogHolderImpl<LogTraitsType, SavePolicy>::SyncPolicy::Mutex&
  LogHolderImpl<LogTraitsType, SavePolicy>::mutex_()
    noexcept
  {
    return lock_;
  }

  template<typename LogTraitsType, typename SavePolicy>
  Generics::Time
  LogHolderImpl<LogTraitsType, SavePolicy>::
  flush_if_required(const Generics::Time& now)
    /*throw(eh::Exception)*/
  {
    try
    {
      CollectorT tmp_collector;
      bool flush;

      {
        SyncPolicy::WriteGuard guard(this->mutex_());
        if ((flush = (flush_time_ + flush_traits_.period < now &&
          !collector_.empty())))
        {
          collector_.swap(tmp_collector);
          flush_time_ = now;
          if (flush_traits_.out_dir.empty())
          {
            return Generics::Time::ZERO;
          }
        }
      }

      if (flush)
      {
        save_policy_.save(tmp_collector, flush_traits_.out_dir.c_str());
      }

      return now + flush_traits_.period;
    }
    catch (const AdServer::LogProcessing::LogSaver::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderImpl::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "AdServer::LogProcessing::LogSaver::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderImpl::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  /* LogHolderPortioned */
  template<typename LogTraitsType, typename SavePolicy>
  LogHolderPortioned<LogTraitsType, SavePolicy>::DumpTask::DumpTask(
    LogHolderPortioned<LogTraitsType, SavePolicy>* log_holder,
    Portion_var portion,
    Sync::Semaphore& sema)
      : log_holder_(log_holder),
        portion_(std::move(portion)),
        sema_(sema)
  {}

  template<typename LogTraitsType, typename SavePolicy>
  LogHolderPortioned<LogTraitsType, SavePolicy>::DumpTask::~DumpTask() noexcept
  {
    if(portion_)
    {
      sema_.release();
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  void
  LogHolderPortioned<LogTraitsType, SavePolicy>::DumpTask::execute() noexcept
  {
    if(!portion_->collector.empty())
    {
      log_holder_->save_policy_.save(portion_->collector, log_holder_->flush_traits_.out_dir.c_str());
    }

    portion_ = Portion_var();
    sema_.release();
  }

  template<typename LogTraitsType, typename SavePolicy>
  Generics::Time
  LogHolderPortioned<LogTraitsType, SavePolicy>::
  flush_if_required(const Generics::Time& now)
    /*throw(eh::Exception)*/
  {
    try
    {
      PortionArray dump_portions;
      init_portions_(dump_portions);

      {
        SyncPolicy::WriteGuard guard(lock_);

        if (flush_time_ + flush_traits_.period < now)
        {
          flush_time_ = now;

          for(unsigned long portion_i = 0; portion_i < portions_num_; ++portion_i)
          {
            SyncPolicy::WriteGuard guard(portions_[portion_i]->lock);
            portions_[portion_i]->collector.swap(dump_portions[portion_i]->collector);
          }
        }
      }

      if (flush_traits_.out_dir.empty())
      {
        // drop collections
        return Generics::Time::ZERO; // ???
      }

      if(task_runner_)
      {
        int dump_portions_num = 0;
        Sync::Semaphore sema(0);

        for (auto portion_it = dump_portions.begin(); portion_it != dump_portions.end(); ++portion_it)
        {
          if(!(*portion_it)->collector.empty())
          {
            ++dump_portions_num;
            task_runner_->enqueue_task(
              Generics::Task_var(new DumpTask(this, *portion_it, sema)));
          }
        }

        // wait all dump tasks (task runner can't be deactivated
        for(int i = 0; i < dump_portions_num; ++i)
        {
          sema.acquire();
        }
      }
      else
      {
        for (auto portion_it = dump_portions.begin(); portion_it != dump_portions.end(); ++portion_it)
        {
          if(!(*portion_it)->collector.empty())
          {
            save_policy_.save((*portion_it)->collector, flush_traits_.out_dir.c_str());
          }
        }        
      }

      return now + flush_traits_.period;
    }
    catch (const AdServer::LogProcessing::LogSaver::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderPortioned::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "AdServer::LogProcessing::LogSaver::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderPortioned::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  /* LogHolderLimitedDataAdd */
  template<typename LogTraitsType, typename SavePolicy>
  LogHolderLimitedDataAdd<LogTraitsType, SavePolicy>::LogHolderLimitedDataAdd(
    Logging::Logger* logger,
    size_t limit,
    const LogFlushTraits& flush_traits,
    const SavePolicy& save_policy)
    /*throw(eh::Exception)*/
    : LogHolderImpl<LogTraitsType, SavePolicy>(
        flush_traits, save_policy),
      logger_(ReferenceCounting::add_ref(logger)),
      limit_(limit),
      count_(0),
      skipped_(0)
  {}

  template<typename LogTraitsType, typename SavePolicy>
  void
  LogHolderLimitedDataAdd<LogTraitsType, SavePolicy>::add_record_i_(
    typename LogHolderImpl<LogTraitsType, SavePolicy>::
      CollectorT::DataT&& data)
    /*throw(eh::Exception)*/
  {
    this->collector_.add(std::move(data));
  }

  template<typename LogTraitsType, typename SavePolicy>
  void
  LogHolderLimitedDataAdd<LogTraitsType, SavePolicy>::add_record(
    typename LogHolderImpl<LogTraitsType, SavePolicy>::
      CollectorT::DataT data)
    /*throw(eh::Exception)*/
  {
    typename LogHolderImpl<LogTraitsType, SavePolicy>::
      SyncPolicy::WriteGuard lock(this->mutex_());
    if (count_ < limit_)
    {
      add_record_i_(std::move(data));
      ++count_;
    }
    else
    {
      ++skipped_;
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  LogHolderLimitedDataAdd<LogTraitsType, SavePolicy>::~LogHolderLimitedDataAdd()
    noexcept
  {
    static const char* FUN = "LogHolderLimitedDataAdd::~LogHolderLimitedDataAdd():";

    // No catch mutex. Only message before shutdown. Saving will be done in ~LogHolderImpl()
    if (skipped_ &&
        logger_ && logger_->log_level() >= Logging::Logger::WARNING)
    {
      logger_->stream(Logging::Logger::WARNING, "LogHolderLimitedDataAddAspect", "ADS-IMPL-187")
        << FUN << " skipped \'" << skipped_ << "\' data records";
    }

    if (logger_ && logger_->log_level() >= Logging::Logger::TRACE)
    {
      logger_->stream(Logging::Logger::TRACE, "LogHolderLimitedDataAddAspect")
        << FUN << " saved \'" << count_ << "\' data records";
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  inline
  Generics::Time
  LogHolderLimitedDataAdd<LogTraitsType, SavePolicy>::flush_if_required(
    const Generics::Time& now) /*throw(eh::Exception)*/
  {
    static const char* FUN = "LogHolderLimitedDataAdd::flush_if_required():";

    typedef LogHolderImpl<LogTraitsType, SavePolicy> Base;
    try
    {
      typename LogHolderImpl<LogTraitsType, SavePolicy>::CollectorT tmp_collector;
      bool flush;
      size_t skipped_val = 0;
      size_t saved_val = 0;

      {
        typename LogHolderImpl<LogTraitsType, SavePolicy>::
          SyncPolicy::WriteGuard guard(this->mutex_());

        if ((flush = (Base::flush_time_ + Base::flush_traits_.period < now &&
          !Base::collector_.empty())))
        {
          skipped_val = skipped_;
          saved_val = count_;

          Base::collector_.swap(tmp_collector);
          Base::flush_time_ = now;
          if (Base::flush_traits_.out_dir.empty())
          {
            return Generics::Time::ZERO;
          }
          count_ = 0;
          skipped_ = 0;
        }
      }

      if (flush)
      {
        Base::save_policy_.save(tmp_collector, Base::flush_traits_.out_dir.c_str());

        if (skipped_val &&
            logger_ && logger_->log_level() >= Logging::Logger::WARNING)
        {
          logger_->stream(Logging::Logger::WARNING, "LogHolderLimitedDataAddAspect", "ADS-IMPL-187")
            << FUN << " skipped \'" << skipped_val << "\' data records";
        }

        if (logger_ && logger_->log_level() >= Logging::Logger::TRACE)
        {
          logger_->stream(Logging::Logger::TRACE, "LogHolderLimitedDataAddAspect")
            << FUN << " saved \'" << saved_val << "\' data records";
        }
      }

      return now + Base::flush_traits_.period;
    }
    catch (const AdServer::LogProcessing::LogSaver::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderLimitedDataAdd::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "AdServer::LogProcessing::LogSaver::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderLimitedDataAdd::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  /* CompositeLogHolder */
  inline
  void
  CompositeLogHolder::add_child_log_holder(LogHolder* log_holder)
    /*throw(eh::Exception)*/
  {
    SyncPolicy::WriteGuard guard(lock_);
    child_log_holders_.push_back(ReferenceCounting::add_ref(log_holder));
  }

  inline
  Generics::Time
  CompositeLogHolder::flush_if_required(
    const Generics::Time& now) /*throw(eh::Exception)*/
  {
    Generics::Time next_flush_time;
    SyncPolicy::ReadGuard guard(lock_);
    for(LogHolderList::iterator it = child_log_holders_.begin();
        it != child_log_holders_.end();
        ++it)
    {
      Generics::Time local_next_flush_time = (*it)->flush_if_required(now);
      if(local_next_flush_time != Generics::Time::ZERO)
      {
        next_flush_time = (next_flush_time == Generics::Time::ZERO ?
          local_next_flush_time :
          std::min(next_flush_time, local_next_flush_time));
      }
    }

    return next_flush_time;
  }

  template<typename LogTraitsType, typename SavePolicy>
  class LogHolderPoolBase<LogTraitsType, SavePolicy>::ContainerHolder:
    public Generics::Last<ReferenceCounting::AtomicImpl>
  {
  public:
    ContainerHolder(): index_(0) {}

    void
    get_object(LogHolderList& holders);

    void
    release_object(LogHolderList& holders);

    bool
    empty() const
    {
      return holders_.empty();
    }

    LogHolderPoolObject_var
    collect();

  protected:
    virtual ~ContainerHolder() noexcept {}

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

    _Atomic_word index_;
    SyncPolicy::Mutex lock_;
    LogHolderList holders_;
  };

  template<typename LogTraitsType, typename SavePolicy>
  struct LogHolderPoolBase<LogTraitsType, SavePolicy>::LogHolderPoolObject:
    public ReferenceCounting::AtomicImpl
  {
  public:
    typedef typename LogTraitsType::CollectorType CollectorT;

    LogHolderPoolObject(int index_): index(index_) {}

    const int index;
    CollectorT collector;

  protected:
    virtual ~LogHolderPoolObject() noexcept {}
  };

  template<typename LogTraitsType, typename SavePolicy>
  inline
  void
  LogHolderPoolBase<LogTraitsType, SavePolicy>::ContainerHolder::get_object(
    typename LogHolderPoolBase<LogTraitsType, SavePolicy>::LogHolderList& holders)
  {
    {
      SyncPolicy::WriteGuard guard(lock_);
      if (!holders_.empty())
      {
        holders.splice(holders.begin(), holders_, holders_.begin());
      }
    }

    if (holders.empty())
    {
      int index = __gnu_cxx::__exchange_and_add(&index_, 1);
      holders.push_back(new LogHolderPoolObject(index));
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  inline
  void
  LogHolderPoolBase<LogTraitsType, SavePolicy>::ContainerHolder::release_object(
    typename LogHolderPoolBase<LogTraitsType, SavePolicy>::LogHolderList& holders)
  {
    int index = (*holders.begin())->index;

    {
      SyncPolicy::WriteGuard guard(lock_);

      auto it = holders_.begin();
      auto it_end = holders_.end();

      while (it != it_end && (*it)->index < index)
      {
        ++it;
      }

      holders_.splice(it, holders);
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  inline
  typename LogHolderPoolBase<LogTraitsType, SavePolicy>::LogHolderPoolObject_var
  LogHolderPoolBase<LogTraitsType, SavePolicy>::ContainerHolder::collect()
  {
    typename LogHolderList::iterator it = holders_.begin();
    typename LogHolderPoolBase<LogTraitsType, SavePolicy>::LogHolderPoolObject_var& collector = *it;

    while (++it != holders_.end())
    {
      collector->collector.merge((*it)->collector);
      (*it)->collector.clear();
    }

    return collector;
  }

  template<typename LogTraitsType, typename SavePolicy>
  LogHolderPoolBase<LogTraitsType, SavePolicy>::LogHolderPoolBase(
    const LogFlushTraits& flush_traits,
    const SavePolicy& save_policy)
    /*throw(eh::Exception)*/
    : flush_traits_(flush_traits),
      save_policy_(save_policy),
      container_holder_(new ContainerHolder())
  {
  }

  template<typename LogTraitsType, typename SavePolicy>
  LogHolderPoolBase<LogTraitsType, SavePolicy>::~LogHolderPoolBase() noexcept
  {
    try
    {
      if (!container_holder_->empty())
      {
        LogHolderPoolObject_var collector = container_holder_->collect();
        save_policy_.save(collector->collector, flush_traits_.out_dir.c_str());
      }
    }
    catch (const eh::Exception& ex)
    {
      std::cerr << "LogHolderPoolBase::~LogHolderPoolBase<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "eh::Exception caught: " << ex.what() << std::endl;
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  Generics::Time
  LogHolderPoolBase<LogTraitsType, SavePolicy>::flush_if_required(
    const Generics::Time& now) /*throw(eh::Exception)*/
  {
    try
    {
      ContainerHolder_var container_holder(new ContainerHolder());

      {
        SyncPolicy::WriteGuard guard(lock_);

        if (flush_time_ + flush_traits_.period > now)
        {
          return flush_time_ + flush_traits_.period;
        }

        container_holder_.swap(container_holder);
        flush_time_ = now;
      }

      LastContainerHolder_var last_container_holder(container_holder.retn());
      if (last_container_holder->empty())
      {
        return now + flush_traits_.period;
      }

      LogHolderPoolObject_var collector = last_container_holder->collect();

      if (flush_traits_.out_dir.empty())
      {
        return Generics::Time::ZERO;
      }

      save_policy_.save(collector->collector, flush_traits_.out_dir.c_str());

      return now + flush_traits_.period;
    }
    catch (const AdServer::LogProcessing::LogSaver::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderPoolBase::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "AdServer::LogProcessing::LogSaver::Exception caught: " <<
        ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderPoolBase::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  inline
  typename LogHolderPoolBase<LogTraitsType, SavePolicy>::ContainerHolder_var
  LogHolderPoolBase<LogTraitsType, SavePolicy>::get_container_holder_() const /*throw(eh::Exception)*/
  {
    typename SyncPolicy::ReadGuard guard(this->lock_);
    return this->container_holder_;
  }
}
}

#endif /*LOGCOMMONS_LOGHOLDER_TPP*/
