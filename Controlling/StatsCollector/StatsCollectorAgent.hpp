#ifndef STATS_COLLECTOR_AGENT_HPP
#define STATS_COLLECTOR_AGENT_HPP

#include<Generics/Values.hpp>
#include<SNMPAgent/SNMPAgentX.hpp>
#include<CORBACommons/StatsImpl.hpp>
#include <Logger/Logger.hpp>

namespace AdServer
{
  namespace Controlling
  {
    struct MeasurableInfo
    {
      MeasurableInfo() noexcept;

      enum
      {
        HAVE_MIN = 0x01,
        HAVE_MAX = 0x02,
        HAVE_TOTAL = 0x04,
        HAVE_COUNT = 0x08,
        HAVE_ALL = (HAVE_MIN | HAVE_MAX | HAVE_TOTAL | HAVE_COUNT)
      };

      double min;
      double max;
      double total;
      unsigned long count;
      unsigned long flags;
    };

    class StatsCollectorValues: public Generics::Values
    {
    public:
      static const char* const BREAK;

      void update_stat_values(
        const std::string& key,
        const MeasurableInfo& info)
        /*throw(Exception)*/;

      void change_countable_values(
        const std::string& name,
        unsigned long value)
        /*throw(Exception)*/;

      unsigned long update_countable_values(
        const std::string& name,
        unsigned long value,
        bool add)
        /*throw(Exception)*/;

    protected:
      virtual
      ~StatsCollectorValues() noexcept;
    };

    typedef ReferenceCounting::SmartPtr<StatsCollectorValues>
      StatsCollectorValues_var;

    class StatsCollectorAgent :
      private SNMPAgentX::SNMPAgentAsync,
      public ReferenceCounting::AtomicImpl
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      enum RuleType
      {
        RT_IGNORE,
        RT_SAVE,
        RT_COUNTABLE,
        RT_SETABLE,
        RT_AVERAGE
      };

      enum ArgsType
      {
        AT_LONG,
        AT_ULONG,
        AT_DOUBLE,
        AT_TIME,
        AT_STRING,
        AT_ANY
      };

      enum FuncName
      {
        FN_MAX = 0,
        FN_MIN,
        FN_TOTAL,
        FN_COUNT,
        FN_SUM,
        FN_AVG,
        FN_NO
      };

      struct RuleInfo
      {
        RuleType type;
        ArgsType arg_type;
        FuncName func_name;
        std::string name;
        std::string variable[2];
      };

      typedef std::map<unsigned long, StatsCollectorValues_var> ValuesMap;
      typedef std::multimap<std::string, RuleInfo> ValuesDispetcher;

      class StatsValues
      {
      public:
        virtual ~StatsValues() {};

        virtual
        StatsCollectorValues*
        get_values_ptr(unsigned id, unsigned oid_suffix) /*throw(eh::Exception)*/;

        CORBACommons::StatsValueSeq*
        get_values() /*throw(eh::Exception)*/;

        void count_all(const ValuesDispetcher& disp)
          /*throw(eh::Exception)*/;

      protected:
        template <typename T>
        void
        collect_and_update_(
          const std::string& key,
          const StatsCollectorAgent::RuleInfo& info)
          /*throw(eh::Exception)*/;

        Sync::PosixRWLock lock_;
        /// values_[0] store processed values: averages, total counters and so on.
        ValuesMap values_;
      };

      StatsValues*
      get_stats_values() noexcept;

      CORBACommons::StatsValueSeq*
      get_values() /*throw(eh::Exception)*/;

      StatsCollectorAgent(
        Logging::Logger* logger,
        const char* profile,
        const char* root,
        const char* directory = 0,
        const char* agentx_socket = 0)
        /*throw(eh::Exception, Exception)*/;

    protected:
      virtual
      ~StatsCollectorAgent() noexcept;

    private:
      class StatsCollectorAgentJob:
        public SNMPJob,
        public StatsValues
      {
      public:
        StatsCollectorAgentJob(Logging::Logger* logger,
          const char* profile, const char* root,
          const char* directory, const char* agentx_socket)
          /*throw(eh::Exception, Exception)*/;

        virtual
        StatsCollectorValues*
        get_values_ptr(unsigned id, unsigned oid_suffix) /*throw(eh::Exception)*/;

      protected:
        virtual
        ~StatsCollectorAgentJob() noexcept;

        virtual
        bool
        process_variable_(void* variable, const VariableInfo& info,
          unsigned size, const unsigned* ids)
          /*throw(eh::Exception, Exception)*/;
      };

      StatsCollectorAgentJob& job_;
    };

    typedef ReferenceCounting::SmartPtr<StatsCollectorAgent>
      StatsCollectorAgent_var;

    class ValuesHandler
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      struct SavedData
      {
        MeasurableInfo info;
      };

      class SavedInfo
      {
      public:
        void count_min(const double& value) noexcept;
        void count_max(const double& value) noexcept;
        void count_total(const double& value) noexcept;
        void count_count(unsigned long) noexcept;
        bool ready_measurable() const noexcept;
        const MeasurableInfo& measurable_info() const noexcept;
      private:
        SavedData data_;
      };

      typedef std::map<std::string, SavedInfo> SavedMap;

      ValuesHandler(
        const StatsCollectorAgent::ValuesDispetcher& disp,
        StatsCollectorAgent::StatsValues* stats_values,
        size_t index,
        size_t own_index)
        /*throw(eh::Exception)*/;

      void process(const char* key, long value)
        /*throw(Exception)*/;
      void process(const char* key, unsigned long value)
        /*throw(Exception)*/;
      void process(const char* key, double value)
        /*throw(Exception)*/;
      void process(const char* key, const char* value)
        /*throw(Exception)*/;

      void finish(std::ostream& err) noexcept;

    protected:
      static double string_to_double_(const char* str) noexcept;

      static Generics::Time double_to_time_(double str) noexcept;

    private:
      StatsCollectorAgent::StatsValues* stats_values_;
      StatsCollectorValues* values_ptr_;
      const StatsCollectorAgent::ValuesDispetcher& dispetcher_;
      SavedMap saved_values;
    };

  }
}

namespace AdServer
{
  namespace Controlling
  {
    inline
    StatsCollectorValues::~StatsCollectorValues() noexcept
    {
    }

    inline
    StatsCollectorAgent::StatsValues*
    StatsCollectorAgent::get_stats_values() noexcept
    {
      return &job_;
    }

    inline CORBACommons::StatsValueSeq*
    StatsCollectorAgent::get_values() /*throw(eh::Exception)*/
    {
      return job_.get_values();
    }

    inline StatsCollectorValues*
    StatsCollectorAgent::StatsValues::get_values_ptr(
      unsigned id, unsigned /*oid_suffix*/)
      /*throw(eh::Exception)*/
    {
      ValuesMap::iterator it;
      {
        Sync::PosixRGuard guard (lock_);
        it = values_.find(id);
        if(it != values_.end())
        {
          return it->second;
        }
      }
      {
        Sync::PosixWGuard guard(lock_);
        it = values_.find(id);
        if(it == values_.end())
        {
          StatsCollectorValues_var new_value = new StatsCollectorValues();
          values_[id] = new_value;
          return new_value;
        }
        return it->second;
      }
    }

    inline StatsCollectorValues*
    StatsCollectorAgent::StatsCollectorAgentJob::get_values_ptr(
      unsigned id, unsigned oid_suffix)
      /*throw(eh::Exception)*/
    {
      ValuesMap::iterator it;
      {
        Sync::PosixRGuard guard (lock_);
        it = values_.find(id);
        if(it != values_.end())
        {
          return it->second;
        }
      }
      {
        Sync::PosixWGuard guard(lock_);
        it = values_.find(id);
        if(it == values_.end())
        {
          StatsCollectorValues_var new_value = new StatsCollectorValues();
          values_[id] = new_value;
          unsigned ids[2] = {id, oid_suffix};
          if (const GenericSNMPAgent::RootInfo* root =
            get_rootinfo("statsCollectionTable.statsCollectionEntry"))
          {
            root->register_index(2, ids);
          }
          if (const GenericSNMPAgent::RootInfo* root =
            get_rootinfo("rtbTimeoutsTable.rtbTimeoutsEntry.timeoutsIndex"))
          {
            root->register_index(2, ids);
          }
          return new_value;
        }
        return it->second;
      }
    }

    inline MeasurableInfo::MeasurableInfo() noexcept
      : flags(0)
    {
    }

    inline void ValuesHandler::SavedInfo::count_min(
      const double& value)
      noexcept
    {
      data_.info.min = value;
      data_.info.flags |= MeasurableInfo::HAVE_MIN;
    }

    inline void ValuesHandler::SavedInfo::count_max(
      const double& value)
      noexcept
    {
      data_.info.max = value;
      data_.info.flags |= MeasurableInfo::HAVE_MAX;
    }

    inline void ValuesHandler::SavedInfo::count_total(
      const double& value)
      noexcept
    {
      data_.info.total = value;
      data_.info.flags |= MeasurableInfo::HAVE_TOTAL;
    }

    inline void ValuesHandler::SavedInfo::count_count(unsigned long value)
      noexcept
    {
      data_.info.count = value;
      data_.info.flags |= MeasurableInfo::HAVE_COUNT;
    }

    inline
    bool ValuesHandler::SavedInfo::ready_measurable() const noexcept
    {
      return (data_.info.flags & MeasurableInfo::HAVE_ALL) ? true : false;
    }

    inline
    const MeasurableInfo& ValuesHandler::SavedInfo::measurable_info() const
      noexcept
    {
      return data_.info;
    }

  }
}

#endif
