/* $Id: ProfilingServerStats.hpp 185978 2020-07-04 00:12:49Z jurij_kuznecov $
* @file ProfilingServerStats.hpp
* @author Artem V. Nikitin (artem_nikitin@ocslab.com)
* Profiling server statistics
*/
#pragma once

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Commons/AtomicInt.hpp>
#include <Frontends/ProfilingServer/DMPKafkaStreamer.hpp>

namespace AdServer
{
  namespace Profiling
  {
    class ProfilingServerStats : public ReferenceCounting::AtomicImpl
    {
    public:
      ProfilingServerStats(
        DMPKafkaStreamer* streamer) noexcept;
      
      void
      add_filtered_messages(unsigned long filtered_count)
        noexcept;

      void
      add_received_messages(
        unsigned long received_count,
        unsigned long received_size)
        noexcept;
      
      unsigned long
      filtered_messages() const noexcept;

      unsigned long
      kafka_errors() const noexcept;
      
      unsigned long
      sent_messages() const noexcept;

      unsigned long
      sent_messages_size() const noexcept;

      unsigned long
      received_messages() const noexcept;
      
      unsigned long
      received_messages_size() const noexcept;

      template <typename Functor>
      void
      enumerate_all(Functor& functor) const /*throw(eh::Exception)*/;

    protected:
      static const Generics::StringHashAdapter SENT_MESSAGES_COUNT_;
      static const Generics::StringHashAdapter SENT_MESSAGES_SIZE_;
      static const Generics::StringHashAdapter RECEIVED_MESSAGES_COUNT_;
      static const Generics::StringHashAdapter RECEIVED_MESSAGES_SIZE_;
      static const Generics::StringHashAdapter FILTERED_MESSAGES_COUNT_;
      static const Generics::StringHashAdapter KAFKA_ERRORS_COUNT_;

      ~ProfilingServerStats() noexcept = default;
        
    protected:
      DMPKafkaStreamer_var streamer_;
      Algs::AtomicUInt filtered_messages_count_;
      Algs::AtomicUInt received_messages_count_;
      Algs::AtomicUInt received_messages_size_;
    };

    typedef ReferenceCounting::SmartPtr<ProfilingServerStats> ProfilingServerStats_var;
  }
}

#include "ProfilingServerStats.ipp"
