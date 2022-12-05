#include <limits.h>
#include <unistd.h>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/Values.hpp>
#include <Generics/Function.hpp>
#include <Generics/CRC.hpp>
#include <Logger/Logger.hpp>
#include <Stream/MemoryStream.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include "StatsCollectorImpl.hpp"
#include "StatsCollectorAgent.hpp"

namespace AdServer
{
  namespace Controlling
  {
    const char* StatsCollectorImpl::ASPECT = "StatsCollector";

    StatsCollectorImpl::StatsCollectorImpl(
      Logging::Logger* logger,
      xsd::AdServer::Configuration::StatsCollectorConfigType* config)
      /*throw(Exception)*/ :
        own_index_(0),
        logger_(new Logging::LoggerDefaultHolder(
          logger, 0, "ADS-IMPL-704")),
        ready_(false)
    {
      static const char FUN[] = "StatsCollectorImpl::StatsCollectorImpl";
      try
      {
        char name[2048];
        if (::gethostname(name, sizeof(name)) != 0)
        {
          eh::throw_errno_exception<Exception>(errno,
            FUN, "gethostname failed on host name '", name, "'");
        }
        values_.reset(new StatsCollectorAgent::StatsValues);
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << "Caught eh::Exception on initialization values: " << e.what();
        throw Exception(err);
      }
      if(config->Rules().present())
      {
        typedef xsd::AdServer::Configuration::RulesType RulesType;

        for(RulesType::Rule_sequence::const_iterator i =
            config->Rules().get().Rule().begin();
            i != config->Rules().get().Rule().end(); ++i)
        {
          StatsCollectorAgent::RuleInfo info;
          if(i->type() == "Measurable")
          {
            std::string key = i->prefix();
            {
              info.type = StatsCollectorAgent::RT_SAVE;
              info.arg_type = StatsCollectorAgent::AT_TIME;
              info.func_name = StatsCollectorAgent::FN_MIN;
              info.name = key;
              disp_.insert(
                std::make_pair(
                  key + StatsCollectorValues::BREAK + "Min",
                  info));
            }
            {
              info.type = StatsCollectorAgent::RT_SAVE;
              info.arg_type = StatsCollectorAgent::AT_TIME;
              info.func_name = StatsCollectorAgent::FN_MAX;
              info.name = key;
              disp_.insert(
                std::make_pair(
                  key + StatsCollectorValues::BREAK + "Max",
                  info));
            }
            {
              info.type = StatsCollectorAgent::RT_SAVE;
              info.arg_type = StatsCollectorAgent::AT_ULONG;
              info.func_name = StatsCollectorAgent::FN_COUNT;
              info.name = key;
              disp_.insert(
                std::make_pair(
                  key + StatsCollectorValues::BREAK + "Count",
                  info));
            }
            {
              info.type = StatsCollectorAgent::RT_SAVE;
              info.arg_type = StatsCollectorAgent::AT_TIME;
              info.func_name = StatsCollectorAgent::FN_TOTAL;
              info.name = key;
              disp_.insert(
                std::make_pair(
                  key + StatsCollectorValues::BREAK + "Total",
                  info));
            }
            info.type = StatsCollectorAgent::RT_IGNORE;
            info.name.clear();
            disp_.insert(
              std::make_pair(
                key + StatsCollectorValues::BREAK + "Average",
                info));
          }
          else if(i->type() == "Countable")
          {
            if(!i->name().present())
            {
              logger_->sstream(
                Logging::Logger::WARNING,
                ASPECT,
                "ADSC-IMPL-702") <<
                '\'' << i->type() << "' required name attribute "
                 " prefix '" << i->prefix() << "'";
              continue;
            }
            info.type = StatsCollectorAgent::RT_COUNTABLE;
            info.arg_type = StatsCollectorAgent::AT_ULONG;
            if(i->variable1().present() && i->variable1().get() == "Set")
            {
              info.func_name = StatsCollectorAgent::FN_NO;
            }
            else
            {
              info.func_name = StatsCollectorAgent::FN_SUM;
            }
            info.name = i->name().get();
            disp_.insert(std::make_pair(i->prefix(), info));
          }
          else if(i->type() == "Setable")
          {
            info.type = StatsCollectorAgent::RT_SETABLE;
            info.arg_type = StatsCollectorAgent::AT_ANY;
            info.func_name = StatsCollectorAgent::FN_NO;
            disp_.insert(std::make_pair(i->prefix(), info));
          }
          else if(i->type() == "Average")
          {
            if(!i->name().present())
            {
              logger_->sstream(
                Logging::Logger::WARNING,
                ASPECT,
                "ADSC-IMPL-702") <<
                '\'' << i->type() << "' required name attribute "
                 " prefix '" << i->prefix() << "'";
              continue;
            }
            if(!i->variable1().present())
            {
              logger_->sstream(
                Logging::Logger::WARNING,
                ASPECT,
                "ADSC-IMPL-702") <<
                '\'' << i->type() << "' required variable1 attribute "
                 " prefix '" << i->prefix() << "'";
              continue;
            }
            if(!i->variable2().present())
            {
              logger_->sstream(
                Logging::Logger::WARNING,
                ASPECT,
                "ADSC-IMPL-702") <<
                '\'' << i->type() << "' required variable2 attribute "
                 " prefix '" << i->prefix() << "'";
              continue;
            }
            info.type = StatsCollectorAgent::RT_AVERAGE;
            info.arg_type = StatsCollectorAgent::AT_ULONG;
            info.func_name = StatsCollectorAgent::FN_AVG;
            info.name = i->name().get();
            info.variable[0] = i->variable1().get();
            info.variable[1] = i->variable2().get();
            disp_.insert(std::make_pair(i->prefix(), info));

          }
          else
          {
            logger_->sstream(
              Logging::Logger::WARNING,
              ASPECT,
              "ADSC-IMPL-702") <<
              "Not correct value of type: '" <<
              i->type() << "' for '" << i->prefix() << "'";
          }
        }
      }
      if(config->SNMPConfig().present())
      {
        try
        {
          own_index_ = config->SNMPConfig().get().index().present() ?
            config->SNMPConfig().get().index().get() :
            getpid();

          agent_ = new StatsCollectorAgent(
            logger_,
            "",
            "StatsCollector-MIB:statsCollector",
            config->SNMPConfig().get().mib_dirs().c_str());
        }
        catch(const StatsCollectorAgent::Exception& e)
        {
          logger_->sstream(
            Logging::Logger::EMERGENCY,
            ASPECT,
            "ADSC-IMPL-703") <<
            "Caught exception on initialization snmp agent: " <<
            e.what();
        }
        catch(const eh::Exception& e)
        {
          logger_->sstream(
            Logging::Logger::EMERGENCY,
            ASPECT,
            "ADSC-IMPL-703") <<
            "Caught eh::Exception on initialization snmp agent: " <<
            e.what();
        }
      }
      // Create StatsValues for aggregating stats
      (agent_ ? agent_->get_stats_values() :
        values_.get())->get_values_ptr(0, own_index_);
    }

    void StatsCollectorImpl::activate_object()
      /*throw(
          ActiveObject::AlreadyActive, ActiveObject::Exception, eh::Exception)*/
    {
      ready_ = true;
    }

    void StatsCollectorImpl::deactivate_object()
      /*throw(ActiveObject::Exception, eh::Exception)*/
    {
      ready_ = false;
    }

    void StatsCollectorImpl::wait_object()
      /*throw(ActiveObject::Exception, eh::Exception)*/
    {
    }

    char* StatsCollectorImpl::comment() /*throw(CORBACommons::OutOfMemory)*/
    {
      try
      {
        return CORBA::string_dup("ok");
      }
      catch(const CORBA::Exception& e)
      {
        throw CORBACommons::OutOfMemory();
      }
      catch(const eh::Exception& e)
      {
        throw CORBACommons::OutOfMemory();
      }
    }

    unsigned StatsCollectorImpl::resolve_index_(const char* hostname)
      /*throw(Exception)*/
    {
      String::SubString name(hostname);
      return Generics::CRC::reversed(0, name.data(), name.length());
    }

    /**
     * Returns a sequence of all stored values
     * @return CORBA sequence of stored values
     */
    StatsValueSeq* StatsCollectorImpl::get_stats()
      /*throw(CORBA::Exception, CORBACommons::ProcessStatsControl::ImplementationException)*/
    {
      if(agent_)
      {
        return agent_->get_values();
      }
      else
      {
        return values_->get_values();
      }
    }

    void StatsCollectorImpl::add_stats(
      const char* host_name,
      const AdServer::Controlling::StatsValueSeq& data)
      /*throw(CORBACommons::ProcessStatsControl::ImplementationException)*/
    {
      std::ostringstream err;
      try
      {
        unsigned index = resolve_index_(host_name);
        ValuesHandler handler(
          disp_,
          (agent_ != 0 ? agent_->get_stats_values() : values_.get()),
          index, own_index_);

        for(size_t i = 0; i < data.length(); ++i)
        {
          const CORBACommons::StatsValue& value = data[i];
          if(value.value.type()->equal(CORBA::_tc_long))
          {
            CORBA::Long real_value;
            value.value >>= real_value;
            handler.process(value.key.in(), static_cast<long>(real_value));
          }
          else if(value.value.type()->equal(CORBA::_tc_ulong))
          {
            CORBA::ULong real_value;
            value.value >>= real_value;
            handler.process(
              value.key.in(),
              static_cast<unsigned long>(real_value));
          }
          else if(value.value.type()->equal(CORBA::_tc_ulonglong))
          {
            CORBA::ULongLong real_value;
            value.value >>= real_value;
            handler.process(
              value.key.in(),
              static_cast<unsigned long>(real_value));
          }
          else if(value.value.type()->equal(CORBA::_tc_longlong))
          {
            CORBA::LongLong real_value;
            value.value >>= real_value;
            handler.process(
              value.key.in(),
              static_cast<long>(real_value));
          }
          else if(value.value.type()->equal(CORBA::_tc_double))
          {
            CORBA::Double real_value;
            value.value >>= real_value;
            handler.process(
              value.key.in(),
              static_cast<double>(real_value));
          }
          else if(value.value.type()->equal(CORBA::_tc_string))
          {
            const char* real_value = 0;
            value.value >>= real_value;
            handler.process(value.key.in(), real_value);
          }
          else
          {
            Stream::Error ostr;
            ostr << "Wrong type on any for key: '" << value.key.in()
              << "', type kind: '" << value.value.type()->kind() << "'";
            CORBACommons::throw_desc<
              CORBACommons::ProcessStatsControl::ImplementationException>(
                ostr.str());
          }
        }
        handler.finish(err);
        if(err.str().size())
        {
          CORBACommons::throw_desc<
            CORBACommons::ProcessStatsControl::ImplementationException>(
              err.str());
        }
      }
      catch(const eh::Exception& eh)
      {
        Stream::Error ostr;
        ostr << "Caught eh::Exception. : " << eh.what();
        CORBACommons::throw_desc<
          CORBACommons::ProcessStatsControl::ImplementationException>(
            ostr.str());
      }
    }
  }
}

