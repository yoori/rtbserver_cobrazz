#include <limits.h>

#include <CORBACommons/CorbaAdapters.hpp>
#include <Commons/ProcessControlVarsImpl.hpp>
#include <Generics/Values.hpp>
#include <Generics/Function.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <SNMPAgent/SNMPAgentX.hpp>
#include <Logger/Logger.hpp>
#include <Stream/MemoryStream.hpp>
#include "StatsCollectorAgent.hpp"

namespace
{
  enum
  {
    KEY_MAX = 0,
    KEY_MIN,
    KEY_TOTAL,
    KEY_AVERAGE,
    KEY_COUNT
  };

  const char* KEYS[] =
  {
    "-Max",
    "-Min",
    "-Total",
    "-Average",
    "-Count"
  };

}

namespace AdServer
{
  namespace Controlling
  {
    const char* const StatsCollectorValues::BREAK = "-";

    StatsCollectorAgent::StatsCollectorAgentJob::StatsCollectorAgentJob(
      Logging::Logger* logger,
      const char* profile,
      const char* root,
      const char* directory,
      const char* agentx_socket)
      /*throw(eh::Exception, Exception)*/
      : SNMPJob(logger, profile, root, directory, agentx_socket)
    {
    }

    StatsCollectorAgent::StatsCollectorAgentJob::~StatsCollectorAgentJob()
      noexcept
    {
    }

    bool
    StatsCollectorAgent::StatsCollectorAgentJob::process_variable_(
      void* variable, const VariableInfo& info,
      unsigned /*size*/, const unsigned* ids)
      /*throw(eh::Exception, Exception)*/
    {
      StatsCollectorValues* values;
      {
        Sync::PosixRGuard guard (lock_);
        ValuesMap::iterator it = values_.find(*ids);
        if(it == values_.end())
        {
          return false;
        }
        values = it->second;
      }
      return set_variable_from_values(variable, info, *values);
    }

    CORBACommons::StatsValueSeq*
    StatsCollectorAgent::StatsValues::get_values()
      /*throw(eh::Exception)*/
    {
      CORBACommons::StatsValueSeq_var ret = new CORBACommons::StatsValueSeq;
      Sync::PosixRGuard guard (lock_);
      for(ValuesMap::const_iterator it = values_.begin();
          it != values_.end(); ++it)
      {
        unsigned id = it->first;
        CORBACommons::StatsValueSeq_var temp =
          CORBACommons::ValuesConverter::get_stats(*it->second);
        size_t old_length = ret->length();
        ret->length(old_length + temp->length());
        for(size_t i = 0; i<temp->length(); i++)
        {
          Stream::Error key;
          key << (*temp)[i].key << '.' << id;
          (*ret)[old_length + i].key << key.str();
          (*ret)[old_length + i].value = (*temp)[i].value;
        }
      }
      return ret._retn();
    }

    template <typename T>
    void
    add_series(
      T& result,
      const std::string& variable,
      StatsCollectorAgent::ValuesMap& map)
    {
      result = T();
      T temp_val = T();
      if(!variable.empty())
      {
        for(StatsCollectorAgent::ValuesMap::const_iterator it = map.begin();
             it != map.end(); ++it)
        {
          if(it->first)
          {
            StatsCollectorValues* value = it->second;
            if(value->get(variable, temp_val))
            {
              result += temp_val;
            }
          }
        }
      }
    }

    template <typename T>
    void
    StatsCollectorAgent::StatsValues::collect_and_update_(
      const std::string& key,
      const StatsCollectorAgent::RuleInfo& info)
      /*throw(eh::Exception)*/
    {
      T result_value = T();
      T temp_val = T();
      std::size_t counter = 0;
      bool ret_value = false;
      if(info.type == StatsCollectorAgent::RT_COUNTABLE)
      {
        Sync::PosixRGuard guard (lock_);
        for(StatsCollectorAgent::ValuesMap::const_iterator it = values_.begin();
           it != values_.end(); ++it)
        {
          if(it->first)
          {
            StatsCollectorValues* value = it->second;
            if(value->get(key, temp_val))
            {
              result_value += temp_val;
              ret_value = true;
            }
          }
        }
        if(ret_value)
        {
          values_[0]->set(info.name, result_value);
        }
      }
      else if(info.type == StatsCollectorAgent::RT_AVERAGE)
      {
        Sync::PosixRGuard guard (lock_);
        add_series(result_value, info.variable[0], values_);
        add_series(counter, info.variable[1], values_);
        if (counter)
        {
          values_[0]->set(info.name, static_cast<double>(result_value)/counter);
        }
      }
    }

    void StatsCollectorAgent::StatsValues::count_all(
      const ValuesDispetcher& disp)
      /*throw(eh::Exception)*/
    {
      for(ValuesDispetcher::const_iterator disp_it = disp.begin();
          disp_it != disp.end(); ++disp_it)
      {
        const std::string& key = disp_it->first;
        const RuleInfo& info = disp_it->second;
        if(info.type == StatsCollectorAgent::RT_IGNORE)
        {
          continue;
        }
        switch(info.arg_type)
        {
          case StatsCollectorAgent::AT_ULONG:
            collect_and_update_<unsigned long>(key, info);
            break;
          case StatsCollectorAgent::AT_LONG:
            collect_and_update_<long>(key, info);
            break;
          case StatsCollectorAgent::AT_DOUBLE:
            collect_and_update_<double>(key, info);
            break;
          default:
            break;
        }
      }
    }

    StatsCollectorAgent::StatsCollectorAgent(
      Logging::Logger* logger,
      const char* profile,
      const char* root,
      const char* directory,
      const char* agentx_socket)
      /*throw(eh::Exception, Exception)*/
      : SNMPAgentX::SNMPAgentAsync(SNMPJob_var(new StatsCollectorAgentJob(
          logger, profile, root, directory, agentx_socket))),
        job_(static_cast<StatsCollectorAgentJob&>(agent_))
    {
    }

    StatsCollectorAgent::~StatsCollectorAgent() noexcept
    {
    }

    void StatsCollectorValues::update_stat_values(
      const std::string& key,
      const MeasurableInfo& info)
      /*throw(Exception)*/
    {
      static const char FN[] = "StatsCollectorValues::update_stat_values";
      try
      {
        const Key KEYMAX(key + KEYS[KEY_MAX]);
        const Key KEYMIN(key + KEYS[KEY_MIN]);
        const Key COUNT(key + KEYS[KEY_COUNT]);
        const Key TOTAL(key + KEYS[KEY_TOTAL]);
        const Key AVERAGE(key + KEYS[KEY_AVERAGE]);

        typedef const Floating& (*FloatFunction)(const Floating&,
          const Floating&);

        {
          Sync::PosixGuard guard(mutex_);

          func_or_set_<FloatFunction>(KEYMAX, info.max, std::max);

          func_or_set_<FloatFunction>(KEYMIN, info.min, std::min);

          UnsignedInt all_count =
            func_or_set_(COUNT, info.count, std::plus<UnsignedInt>());

          Floating total_value =
            func_or_set_(TOTAL, info.total, std::plus<Floating>());

          set_(AVERAGE, all_count ? total_value / all_count : total_value);
        }
      }
      catch(const eh::Exception& e)
      {
        Stream::Error ostr;
        ostr << FN << ": eh::Exception: " << e.what();
        throw Exception(ostr);
      }
    }

    ValuesHandler::ValuesHandler(
      const StatsCollectorAgent::ValuesDispetcher& disp,
      StatsCollectorAgent::StatsValues* stats_values,
      size_t index,
      size_t own_index)
      /*throw(eh::Exception)*/
      : stats_values_(stats_values),
        values_ptr_(stats_values->get_values_ptr(index, own_index)),
        dispetcher_(disp)
    {
    }

    void ValuesHandler::process(const char* key, long value)
      /*throw(Exception)*/
    {
      const char* FN = "ValuesHandler::process(long)";
      try
      {
        StatsCollectorAgent::ValuesDispetcher::const_iterator fnd =
          dispetcher_.find(key);
        if(fnd == dispetcher_.end())
        {
          values_ptr_->add_or_set(key, value);
        }
        else
        {
          for(;fnd != dispetcher_.end() && fnd->first == key; ++fnd)
          {
            if(fnd->second.arg_type != StatsCollectorAgent::AT_LONG &&
               fnd->second.arg_type != StatsCollectorAgent::AT_ANY)
            {
              continue;
            }
            if(fnd->second.type == StatsCollectorAgent::RT_SETABLE)
            {
              try
              {
                values_ptr_->set(key, value);
              }
              catch(const eh::Exception& e)
              {
                Stream::Error err;
                err << FN << ": eh::Exception: " << e.what();
                throw Exception(err);
              }
            }
          }
        }
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << FN << ": eh::Exception: " << e.what();
        throw Exception(err);
      }
    }

    void ValuesHandler::process(const char* key, unsigned long value)
      /*throw(Exception)*/
    {
      const char* FN = "ValuesHandler::process(unsigned long)";
      StatsCollectorAgent::ValuesDispetcher::const_iterator fnd =
        dispetcher_.find(key);
      if(fnd == dispetcher_.end())
      {
        try
        {
          values_ptr_->set(key, value);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error err;
          err << FN << ": eh::Exception: " << e.what();
          throw Exception(err);
        }
      }
      else
      {
          for(;fnd != dispetcher_.end() && fnd->first == key; ++fnd)
          {
            const StatsCollectorAgent::RuleInfo& info = fnd->second;
            if(info.arg_type != StatsCollectorAgent::AT_ULONG &&
               info.arg_type != StatsCollectorAgent::AT_ANY)
            {
              continue;
            }
            switch(info.type)
            {
              case StatsCollectorAgent::RT_IGNORE:
                continue;
                break;
              case StatsCollectorAgent::RT_SAVE:
                switch(info.func_name)
                {
                  case StatsCollectorAgent::FN_COUNT:
                    saved_values[info.name].count_count(value);
                    break;
                  default:
                    {
                      Stream::Error err;
                      err << "Wrong function (" << info.func_name <<
                        ") for type ULONG and key '" << key << "'";
                      throw Exception(err);
                    }
                    break;
                }
                break;
              case StatsCollectorAgent::RT_COUNTABLE:
                if(info.func_name == StatsCollectorAgent::FN_NO)
                {
                  try
                  {
                    values_ptr_->set(key, value);
                  }
                  catch(const eh::Exception& e)
                  {
                    Stream::Error err;
                    err << FN << ": eh::Exception: " << e.what();
                    throw Exception(err);
                  }
                }
                else
                {
                  try
                  {
                    values_ptr_->add_or_set(key, value);
                  }
                  catch(const eh::Exception& e)
                  {
                    Stream::Error err;
                    err << FN << ": eh::Exception: " << e.what();
                    throw Exception(err);
                  }
                }
                break;
              case StatsCollectorAgent::RT_SETABLE:
                try
                {
                  values_ptr_->set(key, value);
                }
                catch(const eh::Exception& e)
                {
                  Stream::Error err;
                  err << FN << ": eh::Exception: " << e.what();
                  throw Exception(err);
                }
                break;
              case StatsCollectorAgent::RT_AVERAGE:
                break;
            }
        }
      }
    }

    void ValuesHandler::process(const char* key, double value)
      /*throw(Exception)*/
    {
      const char* FN = "ValuesHandler::process(double)";
      StatsCollectorAgent::ValuesDispetcher::const_iterator fnd =
        dispetcher_.find(key);
      if(fnd == dispetcher_.end())
      {
        try
        {
          values_ptr_->set(key, value);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error err;
          err << FN << ": eh::Exception: " << e.what();
          throw Exception(err);
        }
      }
      else
      {
        for(;fnd != dispetcher_.end() && fnd->first == key; ++fnd)
        {
          const StatsCollectorAgent::RuleInfo& info = fnd->second;
          if(info.arg_type != StatsCollectorAgent::AT_DOUBLE &&
             info.arg_type != StatsCollectorAgent::AT_TIME &&
             info.arg_type != StatsCollectorAgent::AT_ANY)
          {
            continue;
          }
          switch(info.type)
          {
            case StatsCollectorAgent::RT_IGNORE:
              break;
            case StatsCollectorAgent::RT_SAVE:
              switch(info.func_name)
              {
                case StatsCollectorAgent::FN_MIN:
                  saved_values[fnd->second.name].count_min(value);
                  break;
                case StatsCollectorAgent::FN_MAX:
                  saved_values[fnd->second.name].count_max(value);
                  break;
                case StatsCollectorAgent::FN_TOTAL:
                  saved_values[fnd->second.name].count_total(value);
                  break;
                default:
                  {
                    Stream::Error err;
                    err << "Wrong function (" << info.func_name <<
                      ") for type ULONG and key '" << key << "'";
                    throw Exception(err);
                  }
                  break;
              }
              break;
            case StatsCollectorAgent::RT_AVERAGE:
            case StatsCollectorAgent::RT_COUNTABLE:
              {
                Stream::Error err;
                err << "COUNTABLE and AVERAGE rules don't support for DOUBLE";
                throw Exception(err);
              }
              break;
            case StatsCollectorAgent::RT_SETABLE:
              try
              {
                values_ptr_->set(key, value);
              }
              catch(const eh::Exception& e)
              {
                Stream::Error err;
                err << FN << ": eh::Exception: " << e.what();
                throw Exception(err);
              }
              break;
          }
        }
      }
    }

    double ValuesHandler::string_to_double_(const char* str) noexcept
    {
      double ret;
      Stream::Parser istr(str);
      istr >> ret;
      return ret;
    }

    Generics::Time ValuesHandler::double_to_time_(double val) noexcept
    {
      return Generics::Time(
        static_cast<long>(val),
        static_cast<unsigned long>(
          (val - static_cast<long>(val)) * Generics::Time::USEC_MAX));
    }

    void ValuesHandler::process(const char* key, const char* value)
      /*throw(Exception)*/
    {
      const char* FN = "ValuesHandler::process(const char*)";

      StatsCollectorAgent::ValuesDispetcher::const_iterator fnd =
        dispetcher_.find(key);
      if(fnd != dispetcher_.end())
      {
        for(;fnd != dispetcher_.end() && fnd->first == key; ++fnd)
        {
          const StatsCollectorAgent::RuleInfo& info = fnd->second;
          if(info.arg_type != StatsCollectorAgent::AT_STRING &&
             info.arg_type != StatsCollectorAgent::AT_TIME &&
             info.arg_type != StatsCollectorAgent::AT_ANY)
          {
            continue;
          }
          switch(info.type)
          {
            case StatsCollectorAgent::RT_IGNORE:
              break;
            case StatsCollectorAgent::RT_SAVE:
              switch(info.func_name)
              {
                case StatsCollectorAgent::FN_MIN:
                  saved_values[fnd->second.name].count_min(
                    string_to_double_(value));
                  break;
                case StatsCollectorAgent::FN_MAX:
                  saved_values[fnd->second.name].count_max(
                    string_to_double_(value));
                  break;
                case StatsCollectorAgent::FN_TOTAL:
                  saved_values[fnd->second.name].count_total(
                    string_to_double_(value));
                  break;
                default:
                  {
                    Stream::Error err;
                    err << "Wrong function (" << info.func_name <<
                      ") for type ULONG and key '" << key << "'";
                    throw Exception(err);
                  }
                  break;
              }
              break;
            case StatsCollectorAgent::RT_AVERAGE:
            case StatsCollectorAgent::RT_COUNTABLE:
              {
                Stream::Error err;
                err << "COUNTABLE and AVERAGE rules don't support for STRING";
                throw Exception(err);
              }
              break;
            case StatsCollectorAgent::RT_SETABLE:
              try
              {
                values_ptr_->set(key, value);
              }
              catch(const eh::Exception& e)
              {
                Stream::Error err;
                err << FN << ": eh::Exception: " << e.what();
                throw Exception(err);
              }
              break;
          }
        }
      }
      else
      {
        try
        {
          values_ptr_->set(key, value);
        }
        catch(const eh::Exception& e)
        {
          Stream::Error err;
          err << FN << ": eh::Exception: " << e.what();
          throw Exception(err);
        }
      }
    }

    void ValuesHandler::finish(std::ostream& err) noexcept
    {
      for(SavedMap::iterator it = saved_values.begin();
          it != saved_values.end();
          saved_values.erase(it), it = saved_values.begin())
      {
        if(it->second.ready_measurable())
        {
          try
          {
            values_ptr_->update_stat_values(
              it->first,
              it->second.measurable_info());
          }
          catch(const eh::Exception& e)
          {
            err << "caught eh::Exception. : "
              << e.what();
          }
        }
        else
        {
          err << "not all values was set for '" << it->first << '\'';
        }
      }
      try
      {
        stats_values_->count_all(dispetcher_);
      }
      catch(const eh::Exception& e)
      {
          err << "exception on counting: " << e.what();
      }
    }

  }
}

