#ifndef LOGCOMMONS_FILERECEIVERFACADE_HPP_
#define LOGCOMMONS_FILERECEIVERFACADE_HPP_

#include <list>
#include <string>

#include <Generics/ActiveObject.hpp>
#include <Sync/SyncPolicy.hpp>

#include "FileReceiver.hpp"
#include "LogCommons.hpp"

namespace AdServer
{
  namespace LogProcessing
  {
    namespace
    {
      class Timeout
      {
      public:
        Timeout(unsigned long timeout) noexcept
        {
          if (!timeout)
          {
            end_time = Generics::Time(std::numeric_limits<time_t>::max());
          }
          else
          {
            end_time = Generics::Time::get_time_of_day() + timeout;
          }
        }

        bool
        time_is_up() const noexcept
        {
          return (Generics::Time::get_time_of_day() >= end_time);
        }

      public:
        Generics::Time end_time;
      };
    }

    template<typename Type>
    class DefaultOrderStrategy
    {
    public:
      typedef Type LogType;
      typedef Generics::Time Key;

      static
      Key
      key(
        LogType,
        const std::string* log_file_name = nullptr)
        noexcept
      {
        if (!log_file_name)
        {
          return Generics::Time();
        }

        LogFileNameInfo name_info;
        parse_log_file_name(*log_file_name, name_info);
        return name_info.timestamp;
      }
    };

    /**
     * FileReceivers MUST used for get_eldest inside FileReceiverFacade only.
     */
    template<typename OrderStrategy>
    class FileReceiverFacade : public Generics::SimpleActiveObject,
      public ReferenceCounting::AtomicImpl
    {
    public:
      typedef typename OrderStrategy::LogType LogType;
      typedef typename OrderStrategy::Key Key;
      typedef typename FileReceiver::Interrupted Interrupted;

      struct FileEntity
      {
        LogType type;
        FileReceiver::FileGuard_var file_guard;
      };

      typedef std::list<std::pair<LogType, FileReceiver_var> >
        FileReceiversInitializer;

    public:
      FileReceiverFacade(
        const FileReceiversInitializer& file_receivers,
        const OrderStrategy& strategy = OrderStrategy())
        noexcept;

      /**
       * Blocking call
       */
      FileEntity
      get_eldest(unsigned long timeout = 0)
        /*throw(eh::Exception, Interrupted)*/;

    private:
      typedef Sync::Policy::PosixThread SyncPolicy;

      class FileReceiverHolder : public ReferenceCounting::AtomicImpl,
        public FileReceiver::Callback
      {
      public:
        const LogType type;
        FileReceiver_var file_receiver;
        mutable SyncPolicy::Mutex lock;
        Key key;
        FileReceiverFacade& facade;

      public:
        FileReceiverHolder(
          LogType t,
          FileReceiver* fr,
          FileReceiverFacade& f)
          noexcept;

        virtual void
        on_top_changed(const std::string& new_top) noexcept;

      protected:
        virtual
        ~FileReceiverHolder() noexcept
        {}
      };

      typedef ReferenceCounting::SmartPtr<FileReceiverHolder, ReferenceCounting::PolicyAssert>
        FileReceiverHolder_var;

      typedef std::list<FileReceiverHolder_var> FileReceivers;

    private:
      FileReceivers file_receivers_;
      FileReceivers empty_file_receivers_;
      OrderStrategy strategy_;

    protected:
      virtual
      ~FileReceiverFacade() noexcept
      {}

      typename FileReceivers::iterator
      find_holder_i_(
        FileReceivers& file_receivers,
        LogType log_type)
        noexcept;

      friend bool operator< (
        const FileReceiverHolder_var& arg1,
        const FileReceiverHolder_var& arg2)
      {
        return (arg1->key < arg2->key);
      }

      static bool
      compare_file_receiver_holder_var(
        const FileReceiverHolder_var& arg1,
        const FileReceiverHolder_var& arg2)
      {
        return (arg1->key < arg2->key);
      }
    };
  }
}

namespace AdServer
{
  namespace LogProcessing
  {
    /// FileReceiverFacade::FileReceiverHolder
    template<typename OrderStrategy>
    FileReceiverFacade<OrderStrategy>::FileReceiverHolder::FileReceiverHolder(
      LogType t,
      FileReceiver* fr,
      FileReceiverFacade& f)
      noexcept
      : type(t), file_receiver(ReferenceCounting::add_ref(fr)), facade(f)
    {
      file_receiver->add_callback(this);
    }

    template<typename OrderStrategy>
    void
    FileReceiverFacade<OrderStrategy>::FileReceiverHolder::on_top_changed(
      const std::string& new_top)
      noexcept
    {
      const Key new_key = facade.strategy_.key(type, &new_top);
      SyncPolicy::WriteGuard guard(lock);

      Sync::ConditionalGuard cond_guard(facade.cond_);
      auto it = facade.find_holder_i_(facade.empty_file_receivers_, type);

      if (it != facade.empty_file_receivers_.end())
      {
        facade.file_receivers_.push_back(*it);
        facade.empty_file_receivers_.erase(it);
        key = new_key;
      }
      else if (new_key < key)
      {
        key = new_key;
      }

      facade.file_receivers_.sort(
        FileReceiverFacade<OrderStrategy>::compare_file_receiver_holder_var);
      facade.cond_.broadcast();
    }

    /// FileReceiverFacade
    template<typename OrderStrategy>
    FileReceiverFacade<OrderStrategy>::FileReceiverFacade(
      const FileReceiversInitializer& file_receivers,
      const OrderStrategy& strategy)
      noexcept
      : strategy_(strategy)
    {
      for (auto it = file_receivers.begin();
           it != file_receivers.end(); ++it)
      {
        file_receivers_.emplace_back(
          FileReceiverHolder_var(new FileReceiverHolder(
            it->first, it->second, *this)));
        file_receivers_.back()->key = strategy_.key(it->first);
      }
    }

    template<typename OrderStrategy>
    typename FileReceiverFacade<OrderStrategy>::FileReceivers::iterator
    FileReceiverFacade<OrderStrategy>::find_holder_i_(
      FileReceivers& file_receivers,
      LogType log_type)
      noexcept
    {
      auto it = file_receivers.begin();

      while (it != file_receivers.end() &&
             (*it)->type != log_type)
      {
        ++it;
      }

      return it;
    }

    template<typename OrderStrategy>
    typename FileReceiverFacade<OrderStrategy>::FileEntity
    FileReceiverFacade<OrderStrategy>::get_eldest(
      unsigned long timeout)
      /*throw(eh::Exception, Interrupted)*/
    {
      FileEntity res;
      const Timeout interrupt_time(timeout);

      while (active() && !res.file_guard && !interrupt_time.time_is_up())
      {
        FileReceiverHolder_var holder;

        {
          Sync::ConditionalGuard lock(cond_);

          while (active() && file_receivers_.empty() && !interrupt_time.time_is_up())
          {
            lock.timed_wait(timeout ? &interrupt_time.end_time : nullptr);
          }

          if (!file_receivers_.empty())
          {
            holder = *file_receivers_.begin();
          }
        }

        if (holder)
        {
          std::string new_top;
          SyncPolicy::WriteGuard guard(holder->lock);
          res.type = holder->type;
          res.file_guard = holder->file_receiver->get_eldest(new_top);
          Sync::ConditionalGuard cond_guard(cond_);

          if (new_top.empty() || !res.file_guard)
          {
            auto it = find_holder_i_(file_receivers_, holder->type);

            if (it != file_receivers_.end())
            {
              empty_file_receivers_.push_back(*it);
              file_receivers_.erase(it);
            }
          }
          else
          {
            holder->key = strategy_.key(holder->type, &new_top);
            file_receivers_.sort(
              FileReceiverFacade<OrderStrategy>::compare_file_receiver_holder_var);
          }
        }
      }

      return res;
    }
  }
}

#endif /* LOGCOMMONS_FILERECEIVERFACADE_HPP_ */
