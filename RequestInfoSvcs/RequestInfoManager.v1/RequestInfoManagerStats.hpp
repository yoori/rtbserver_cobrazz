/// @file RequestInfoManagerStats.hpp

#ifndef REQUEST_INFO_MANAGER_STATS_INCLUDED
#define REQUEST_INFO_MANAGER_STATS_INCLUDED

#include <Generics/Values.hpp>
#include <Generics/Time.hpp>
#include <Sync/SyncPolicy.hpp>

#include <Commons/AtomicInt.hpp>

namespace AdServer
{
  namespace LogProcessing
  {
    class AdvertiserActionTraits;
    class ClickTraits;
    class ImpressionTraits;
    class PassbackImpressionTraits;
    class RequestTraits;
    class TagRequestTraits;
  }

  namespace RequestInfoSvcs
  {

    struct StatNames
    {
      static const Generics::StringHashAdapter LOAD_ABS_TIME;
      static const Generics::StringHashAdapter PROCESS_ABS_TIME;
      static const Generics::StringHashAdapter PROCESS_COUNT;
      static const Generics::StringHashAdapter FILE_COUNT;
      static const Generics::StringHashAdapter LAST_PROCESSED_TIMESTAMP;
      /// The number of columns in MIB table of all logs stats
      static const std::size_t LOG_VARS_COUNT = 5;
      /// The number of records in MIB table of all logs stats
      static const std::size_t LOG_COUNT = 6;
      static const char* LOG_CORBA_NAMES[LOG_COUNT];
      static const Generics::StringHashAdapter WEB_INDEX_REQUESTS;
    };

    template <typename T>
    struct Type2Index;

    template <> struct Type2Index<LogProcessing::AdvertiserActionTraits>
    {
      enum { result = 0 };
    };
    template <> struct Type2Index<LogProcessing::ClickTraits>
    {
      enum { result = 1 };
    };
    template <> struct Type2Index<LogProcessing::ImpressionTraits>
    {
      enum { result = 2 };
    };
    template <> struct Type2Index<LogProcessing::PassbackImpressionTraits>
    {
      enum { result = 3 };
    };
    template <> struct Type2Index<LogProcessing::RequestTraits>
    {
      enum { result = 4 };
    };
    template <> struct Type2Index<LogProcessing::TagRequestTraits>
    {
      enum { result = 5 };
    };


    struct LogStats
    {
      LogStats() noexcept
        : processed_records(0),
          file_counter(0)
      {}

      Generics::Time load_time;
      Generics::Time process_time;
      Generics::Time last_processed_file_timestamp;
      unsigned long processed_records;
      unsigned long file_counter;
    };

    struct Stats
    {
      Stats() noexcept;

      Algs::AtomicInt web_index_requests;
    };

    class StatsCounters : private Stats,
      public ReferenceCounting::AtomicImpl
    {
    public:
      void
      inc_web_index_request_sent() noexcept;

      const Stats&
      get_stats() const noexcept;
    private:
      virtual
      ~StatsCounters() noexcept = default;
    };
    typedef ReferenceCounting::SmartPtr<StatsCounters> StatsCounters_var;


    class LogStatValues : public Generics::Values
    {
    public:
      LogStatValues() /*throw(eh::Exception)*/ :
        Generics::Values(StatNames::LOG_VARS_COUNT)
      {}

      void
      fill_values(const LogStats& values) /*throw(eh::Exception)*/;

      void
      add_load_time(const Generics::Time& load_time) /*throw(eh::Exception)*/;
    private:
      virtual
      ~LogStatValues() noexcept {}
    };

    typedef ReferenceCounting::SmartPtr<LogStatValues>
      LogStatValues_var;


    /// This object is "replacement" for Generics::Values
    class RequestInfoManagerStatsImpl :
      public ReferenceCounting::AtomicImpl
    {
    public:
      RequestInfoManagerStatsImpl() /*throw(eh::Exception)*/
        : app_stats_(new Generics::Values)
      {
        for (std::size_t i = 0; i < StatNames::LOG_COUNT; ++i)
        {
          stats_[i] = new LogStatValues;
        }
      }

      void
      fill_values(unsigned log_index, const LogStats& values)
        /*throw(eh::Exception)*/;

      void
      fill_values(const Stats& values) /*throw(eh::Exception)*/;

      void
      add_load_time(unsigned log_index, const Generics::Time& load_time)
        /*throw(eh::Exception)*/;

      LogStatValues_var
      get_log_values(unsigned log_index) noexcept
      {
        return stats_[log_index];
      }

      const Generics::Values_var&
      get_app_stats() const noexcept
      {
        return app_stats_;
      }

      const LogStatValues_var
      get_log_values(unsigned log_index) const noexcept
      {
        return stats_[log_index];
      }

      /**
       * Calls functor for each value stored.
       * @param functor functor to call for each value
       */
      template <typename Functor>
      void
      enumerate_all(Functor& functor) const /*throw(eh::Exception)*/
      {
        for (std::size_t i = 0; i < StatNames::LOG_COUNT; ++i)
        {
          FunctorWrapper<Functor> f(functor, StatNames::LOG_CORBA_NAMES[i]);
          stats_[i]->enumerate_all(f);
        }
        app_stats_->enumerate_all(functor);
      }

    private:
      template <typename Functor>
      class FunctorWrapper
      {
      public:
        FunctorWrapper(Functor& f, const char* log_name) noexcept
          : functor_(f),
            log_name_(log_name)
        {}

        void
        operator ()(size_t size) /*throw(eh::Exception)*/
        {
          functor_(size * StatNames::LOG_COUNT);
        }

        template <typename Type>
        void
        operator ()(const Generics::Values::Key& key, const Type& value)
          /*throw(eh::Exception)*/
        {
          std::string name(log_name_);
          name += key.text();
          Generics::Values::Key new_key(name);
          functor_(new_key, value);
        }

      private:
        Functor& functor_;
        const char* log_name_;
      };

      virtual
      ~RequestInfoManagerStatsImpl() noexcept {}

      typedef LogStatValues_var StatArray[StatNames::LOG_COUNT];
      StatArray stats_;
      Generics::Values_var app_stats_;
    };

    typedef ReferenceCounting::SmartPtr<RequestInfoManagerStatsImpl>
      RequestInfoManagerStatsImpl_var;

  }
}

//
// Inlines impl's
//
namespace AdServer
{
  namespace RequestInfoSvcs
  {

    inline
    Stats::Stats() noexcept
      : web_index_requests(0)
    {}

    inline void
    StatsCounters::inc_web_index_request_sent() noexcept
    {
      web_index_requests += 1;
    }

    inline const Stats&
    StatsCounters::get_stats() const noexcept
    {
      return *this;
    }

    inline void
    RequestInfoManagerStatsImpl::fill_values(
      unsigned log_index, const LogStats& values)
      /*throw(eh::Exception)*/
    {
      stats_[log_index]->fill_values(values);
    }

    inline void
    RequestInfoManagerStatsImpl::fill_values(
      const Stats& values)
      /*throw(eh::Exception)*/
    {
      app_stats_->set<Generics::Values::UnsignedInt>(StatNames::WEB_INDEX_REQUESTS,
        values.web_index_requests);
    }

    inline void
    RequestInfoManagerStatsImpl::add_load_time(
      unsigned log_index, const Generics::Time& load_time)
      /*throw(eh::Exception)*/
    {
      stats_[log_index]->add_load_time(load_time);
    }

    inline void
    LogStatValues::fill_values(const LogStats& values)
      /*throw(eh::Exception)*/
    {
      Sync::PosixGuard guard(mutex_);

      func_or_set_(StatNames::LOAD_ABS_TIME, values.load_time.as_double(),
        std::plus<double>());
      func_or_set_(StatNames::PROCESS_ABS_TIME,
        values.process_time.as_double(), std::plus<double>());
      func_or_set_(StatNames::PROCESS_COUNT,
        values.processed_records, std::plus<UnsignedInt>());
      func_or_set_(StatNames::FILE_COUNT,
        values.file_counter, std::plus<UnsignedInt>());
      set_(StatNames::LAST_PROCESSED_TIMESTAMP,
        values.last_processed_file_timestamp.as_double());
    }

    inline void
    LogStatValues::add_load_time(const Generics::Time& load_time)
      /*throw(eh::Exception)*/
    {
      add_or_set(StatNames::LOAD_ABS_TIME, load_time.as_double());
    }

  }
}

#endif // REQUEST_INFO_MANAGER_STATS_INCLUDED
