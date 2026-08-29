#pragma once

#include <iostream>

namespace AdServer::LogProcessing
{
  inline
  LogHolder::~LogHolder() noexcept = default;

  inline
  LogFlushTraits::LogFlushTraits()
  {}

  inline
  LogFlushTraits::LogFlushTraits(const Generics::Time& period_val, const char* out_dir_val)
    : period(period_val),
      out_dir(out_dir_val)
  {}

  template<typename SavePolicy, typename Collector>
  void
  save_log(const LogFlushTraits& flush_traits, SavePolicy& save_policy, Collector& collector)
    /*throw(eh::Exception)*/
  {
    if (flush_traits.primary_dump && flush_traits.primary_dump->available())
    {
      try
      {
        const std::string primary_output_dir =
          flush_traits.primary_dump->output_dir(flush_traits.out_dir);
        save_policy.save(collector, primary_output_dir.c_str());
        return;
      }
      catch (const eh::Exception&)
      {
        flush_traits.primary_dump->disable();
      }
    }

    save_policy.save(collector, flush_traits.out_dir.c_str());
  }

  template<typename LogTraitsType>
  void
  DefaultSavePolicy<LogTraitsType>::save(
    typename LogTraitsType::CollectorType& collector,
    const char* out_dir)
    /*throw(eh::Exception)*/
  {
    IoHelperT(collector).save(out_dir);
  }

  template<typename LogTraitsType>
  DistributionSavePolicy<LogTraitsType>::DistributionSavePolicy(unsigned long distrib_count)
    : distrib_count_(distrib_count)
  {}

  template<typename LogTraitsType>
  void
  DistributionSavePolicy<LogTraitsType>::save(
    typename LogTraitsType::CollectorType& collector,
    const char* out_dir)
    /*throw(eh::Exception)*/
  {
    LogIoProxy<LogTraitsType>::save(collector, out_dir, distrib_count_);
  }

  template<typename LogTraitsType>
  void
  SimpleCsvSavePolicy<LogTraitsType>::save(
    typename LogTraitsType::CollectorType& collector,
    const char* out_dir)
    /*throw(eh::Exception)*/
  {
    SimpleLogSaver_var(new CsvSaverT(collector, out_dir))->save();
  }

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
    if (!collector_.empty())
    {
      try
      {
        save_log(flush_traits_, save_policy_, collector_);
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
  LogHolderImpl<LogTraitsType, SavePolicy>::mutex_() noexcept
  {
    return lock_;
  }

  template<typename LogTraitsType, typename SavePolicy>
  Generics::Time
  LogHolderImpl<LogTraitsType, SavePolicy>::
  flush_if_required(const Generics::Time& now) /*throw(eh::Exception)*/
  {
    try
    {
      CollectorT tmp_collector;
      bool flush;

      {
        SyncPolicy::WriteGuard guard(this->mutex_());
        if ((flush = (flush_time_ + flush_traits_.period < now && !collector_.empty())))
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
        save_log(flush_traits_, save_policy_, tmp_collector);
      }

      return now + flush_traits_.period;
    }
    catch (const AdServer::LogProcessing::LogSaver::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderImpl::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "AdServer::LogProcessing::LogSaver::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderImpl::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " << "eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  /* LogHolderPortioned */
  template<typename LogTraitsType, typename SavePolicy>
  LogHolderPortioned<LogTraitsType, SavePolicy>::LogHolderPortioned(
    const LogFlushTraits& flush_traits,
    const SavePolicy& save_policy,
    Generics::TaskRunner* task_runner,
    unsigned long portions_num)
    /*throw(eh::Exception)*/
    : flush_traits_(flush_traits),
      save_policy_(save_policy),
      task_runner_(ReferenceCounting::add_ref(task_runner)),
      portions_num_(portions_num)
  {
    init_portions_(portions_);
  }

  template<typename LogTraitsType, typename SavePolicy>
  template<typename Mediator>
  void
  LogHolderPortioned<LogTraitsType, SavePolicy>::add_record(
    typename CollectorT::KeyT key,
    Mediator data)
  {
    // Implemented only for two-level collectors: key -> (inner_key -> value).
    for (auto inner_it = data.begin(); inner_it != data.end(); ++inner_it)
    {
      typename CollectorT::DataT inner_data;
      inner_data.add(inner_it->first, inner_it->second);
      const unsigned long portion_i = inner_it->first.hash() % portions_num_;

      SyncPolicy::WriteGuard guard(portions_[portion_i]->lock);
      portions_[portion_i]->collector.add(key, std::move(inner_data));
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  LogHolderPortioned<LogTraitsType, SavePolicy>::~LogHolderPortioned() noexcept = default;

  template<typename LogTraitsType, typename SavePolicy>
  void
  LogHolderPortioned<LogTraitsType, SavePolicy>::init_portions_(PortionArray& portions) noexcept
  {
    portions.resize(portions_num_);
    for (auto it = portions.begin(); it != portions.end(); ++it)
    {
      *it = new Portion();
    }
  }

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
    if (portion_)
    {
      sema_.release();
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  void
  LogHolderPortioned<LogTraitsType, SavePolicy>::DumpTask::execute() noexcept
  {
    if (!portion_->collector.empty())
    {
      save_log(log_holder_->flush_traits_, log_holder_->save_policy_, portion_->collector);
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

          for (unsigned long portion_i = 0; portion_i < portions_num_; ++portion_i)
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

      if (task_runner_)
      {
        int dump_portions_num = 0;
        Sync::Semaphore sema(0);

        for (auto portion_it = dump_portions.begin(); portion_it != dump_portions.end(); ++portion_it)
        {
          if (!(*portion_it)->collector.empty())
          {
            ++dump_portions_num;
            task_runner_->enqueue_task(Generics::Task_var(new DumpTask(this, *portion_it, sema)));
          }
        }

        // wait all dump tasks (task runner can't be deactivated
        for (int i = 0; i < dump_portions_num; ++i)
        {
          sema.acquire();
        }
      }
      else
      {
        for (auto portion_it = dump_portions.begin(); portion_it != dump_portions.end(); ++portion_it)
        {
          if (!(*portion_it)->collector.empty())
          {
            save_log(flush_traits_, save_policy_, (*portion_it)->collector);
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
        "AdServer::LogProcessing::LogSaver::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderPortioned::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " << "eh::Exception caught: " << ex.what();
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
    : LogHolderImpl<LogTraitsType, SavePolicy>(flush_traits, save_policy),
      logger_(ReferenceCounting::add_ref(logger)),
      limit_(limit),
      count_(0),
      skipped_(0)
  {}

  template<typename LogTraitsType, typename SavePolicy>
  template<typename... Args>
  void
  LogHolderLimitedDataAdd<LogTraitsType, SavePolicy>::add_record(Args&&... args)
    /*throw(eh::Exception)*/
  {
    typename LogHolderImpl<LogTraitsType, SavePolicy>::SyncPolicy::WriteGuard lock(this->mutex_());
    if (count_ < limit_)
    {
      this->collector_.add(std::forward<Args>(args)...);
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
    if (skipped_ && logger_ && logger_->log_level() >= Logging::Logger::WARNING)
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
        save_log(Base::flush_traits_, Base::save_policy_, tmp_collector);

        if (skipped_val && logger_ && logger_->log_level() >= Logging::Logger::WARNING)
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
        "AdServer::LogProcessing::LogSaver::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderLimitedDataAdd::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " << "eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }

  /* CompositeLogHolder */
  inline
  CompositeLogHolder::~CompositeLogHolder() noexcept = default;

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
  CompositeLogHolder::flush_if_required(const Generics::Time& now) /*throw(eh::Exception)*/
  {
    Generics::Time next_flush_time;
    SyncPolicy::ReadGuard guard(lock_);
    for (LogHolderList::iterator it = child_log_holders_.begin();
      it != child_log_holders_.end(); ++it)
    {
      Generics::Time local_next_flush_time = (*it)->flush_if_required(now);
      if (local_next_flush_time != Generics::Time::ZERO)
      {
        next_flush_time = (next_flush_time == Generics::Time::ZERO ?
          local_next_flush_time :
          std::min(next_flush_time, local_next_flush_time));
      }
    }

    return next_flush_time;
  }

  template<typename LogTraitsType, typename SavePolicy>
  LogHolderSharded<LogTraitsType, SavePolicy>::LogHolderSharded(
    const LogFlushTraits& flush_traits,
    const SavePolicy& save_policy,
    unsigned long shards_count)
    /*throw(eh::Exception)*/
    : flush_traits_(flush_traits),
      save_policy_(save_policy)
  {
    const unsigned long actual_shards_count = shards_count ? shards_count : 1;
    shards_.reserve(actual_shards_count);
    for (unsigned long i = 0; i < actual_shards_count; ++i)
    {
      shards_.emplace_back(std::make_shared<Shard>());
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  template<typename... Args>
  void
  LogHolderSharded<LogTraitsType, SavePolicy>::add_record(Args&&... args)
  {
    Shard_var shard = get_shard_();
    std::lock_guard<std::mutex> guard(shard->lock);
    shard->collector.add(std::forward<Args>(args)...);
  }

  template<typename LogTraitsType, typename SavePolicy>
  template<typename Function>
  void
  LogHolderSharded<LogTraitsType, SavePolicy>::add_records(Function&& function)
  {
    Shard_var shard = get_shard_();
    std::lock_guard<std::mutex> guard(shard->lock);
    std::forward<Function>(function)(shard->collector);
  }

  template<typename LogTraitsType, typename SavePolicy>
  typename LogHolderSharded<LogTraitsType, SavePolicy>::Shard_var
  LogHolderSharded<LogTraitsType, SavePolicy>::get_shard_() const noexcept
  {
    const std::size_t shard_index =
      next_shard_.fetch_add(1, std::memory_order_relaxed) % shards_.size();
    return shards_[shard_index];
  }

  template<typename LogTraitsType, typename SavePolicy>
  LogHolderSharded<LogTraitsType, SavePolicy>::~LogHolderSharded() noexcept
  {
    try
    {
      CollectorT collector;
      bool has_data = false;

      for (auto& shard : shards_)
      {
        {
          CollectorT local_collector;
          {
            std::lock_guard<std::mutex> guard(shard->lock);
            if (shard->collector.empty())
            {
              continue;
            }
            local_collector.swap(shard->collector);
          }

          if (!has_data)
          {
            collector.swap(local_collector);
            has_data = true;
          }
          else
          {
            collector.merge(std::move(local_collector));
          }
        }
      }

      if (has_data)
      {
        save_log(flush_traits_, save_policy_, collector);
      }
    }
    catch (const eh::Exception& ex)
    {
      std::cerr << "LogHolderSharded::~LogHolderSharded<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "eh::Exception caught: " << ex.what() << std::endl;
    }
  }

  template<typename LogTraitsType, typename SavePolicy>
  Generics::Time
  LogHolderSharded<LogTraitsType, SavePolicy>::flush_if_required(const Generics::Time& now)
    /*throw(eh::Exception)*/
  {
    try
    {
      {
        std::lock_guard<std::mutex> guard(lock_);

        if (flush_time_ + flush_traits_.period > now)
        {
          return flush_time_ + flush_traits_.period;
        }

        flush_time_ = now;
      }

      CollectorT collector;
      bool has_data = false;

      for (auto& shard : shards_)
      {
        CollectorT local_collector;
        {
          std::lock_guard<std::mutex> guard(shard->lock);
          if (shard->collector.empty())
          {
            continue;
          }
          local_collector.swap(shard->collector);
        }

        if (!has_data)
        {
          collector.swap(local_collector);
          has_data = true;
        }
        else
        {
          collector.merge(std::move(local_collector));
        }
      }

      if (!has_data)
      {
        return now + flush_traits_.period;
      }

      if (flush_traits_.out_dir.empty())
      {
        return Generics::Time::ZERO;
      }

      save_log(flush_traits_, save_policy_, collector);

      return now + flush_traits_.period;
    }
    catch (const AdServer::LogProcessing::LogSaver::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderSharded::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " <<
        "AdServer::LogProcessing::LogSaver::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "LogHolderSharded::flush_if_required<'" <<
        LogTraitsType::log_base_name() << "'>(): " << "eh::Exception caught: " << ex.what();
      throw Exception(ostr);
    }
  }
}
