#ifndef STATS_COLLECTOR_IMPL_HPP_
#define STATS_COLLECTOR_IMPL_HPP_

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/ActiveObject.hpp>
#include <CORBACommons/ServantImpl.hpp>
#include <CORBACommons/ProcessControlImpl.hpp>
#include <Stream/MemoryStream.hpp>
#include <Controlling/StatsCollector/StatsCollector.hpp>
#include <Controlling/StatsCollector/StatsCollector_s.hpp>
#include <xsd/Controlling/StatsCollectorConfig.hpp>
#include <Controlling/StatsCollector/StatsCollectorAgent.hpp>

namespace AdServer
{
  namespace Controlling
  {
    typedef CORBACommons::ReferenceCounting::ServantImpl
      <POA_AdServer::Controlling::StatsCollector> StatsServant;
    class StatsCollectorImpl:
      public virtual Generics::ActiveObject,
      public virtual StatsServant
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      StatsCollectorImpl(
        Logging::Logger* logger,
        xsd::AdServer::Configuration::StatsCollectorConfigType* config)
        /*throw(Exception)*/;

      /** Generics::ActiveObject interface */
      virtual void activate_object()
        /*throw(ActiveObject::AlreadyActive,
          ActiveObject::Exception, eh::Exception)*/;

      virtual void deactivate_object()
        /*throw(ActiveObject::Exception, eh::Exception)*/;

      virtual void wait_object()
        /*throw(ActiveObject::Exception, eh::Exception)*/;

      virtual bool active() /*throw(eh::Exception)*/;

      bool ready() noexcept;

      char* comment() /*throw(CORBACommons::OutOfMemory)*/;

      virtual
        void add_stats(
          const char* host_name,
          const AdServer::Controlling::StatsValueSeq& data)
        /*throw(CORBACommons::ProcessStatsControl::ImplementationException)*/;

      /**
       * Returns a sequence of all stored values
       * @return CORBA sequence of stored values
       */
      virtual
        StatsValueSeq* get_stats()
          /*throw(CORBA::Exception,
              CORBACommons::ProcessStatsControl::ImplementationException)*/;

      void update_stat_values(
        const char* key,
        Generics::Time& min,
        Generics::Time& max,
        Generics::Time& total,
        unsigned long count) noexcept;

      static uint32_t address_(const sockaddr_in* address) noexcept;
    protected:

      virtual ~StatsCollectorImpl() noexcept{};

      unsigned resolve_index_(const char* hostname)
        /*throw(Exception)*/;

    protected:
      StatsCollectorAgent::ValuesDispetcher disp_;
      unsigned own_index_;
      StatsCollectorAgent_var agent_;
      std::unique_ptr<StatsCollectorAgent::StatsValues> values_;
      /*use when there isn't SNMP agent*/

    private:
      Sync::PosixMutex mutex_;
      Logging::Logger_var logger_;
      bool ready_;
      static const char* ASPECT;
    };

    typedef ReferenceCounting::SmartPtr<StatsCollectorImpl>
      StatCollectorImpl_var;
  }
}

namespace AdServer
{
  namespace Controlling
  {
    inline
    bool StatsCollectorImpl::ready() noexcept
    {
      return ready_;
    }

    inline
    bool StatsCollectorImpl::active() /*throw(eh::Exception)*/
    {
      return ready_;
    }

    inline
    uint32_t StatsCollectorImpl::address_(const sockaddr_in* address) noexcept
    {
      return address->sin_addr.s_addr;
    }
  }
}
#endif
