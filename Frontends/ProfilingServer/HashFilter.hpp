#ifndef HASHFILTER_HPP_
#define HASHFILTER_HPP_

#include <vector>
#include <deque>

#include <Generics/Time.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Sync/SyncPolicy.hpp>

namespace AdServer
{
  namespace Profiling
  {
    class HashFilter: public ReferenceCounting::AtomicImpl
    {
    public:
      HashFilter(
        unsigned long chunks_count,
        unsigned long chunk_size,
        const Generics::Time& keep_time_period,
        const Generics::Time& keep_time)
        noexcept;

      bool
      set(
        uint64_t hash,
        const Generics::Time& time,
        const Generics::Time& now)
        noexcept;

    protected:
      class Mask
      {
      public:
        Mask(unsigned long size)
          noexcept;

        bool
        set_i(uint64_t hash) noexcept;

      protected:
        std::deque<unsigned char> data_;
      };

      struct Chunk: public ReferenceCounting::AtomicImpl
      {
      public:
        Chunk(unsigned long size)
          noexcept;

        bool
        set(uint64_t hash) noexcept;

      protected:
        typedef Sync::Policy::PosixSpinThread SyncPolicy;

      protected:
        virtual
        ~Chunk() noexcept 
        {}

      protected:
        SyncPolicy::Mutex lock_;
        Mask mask_;
      };

      typedef ReferenceCounting::SmartPtr<Chunk> Chunk_var;

      struct TimeInterval: public ReferenceCounting::AtomicImpl
      {
      public:
        TimeInterval(
          const Generics::Time& time_from_val,
          const Generics::Time& time_to_val,
          unsigned long chunks,
          unsigned long chunk_size)
          noexcept;

        bool
        set(uint64_t hash) noexcept;

        typedef std::vector<Chunk_var> ChunkArray;

      public:
        const Generics::Time time_from;
        const Generics::Time time_to;

      protected:
        virtual
        ~TimeInterval() noexcept 
        {}

      protected:
        ChunkArray chunks_;
      };

      struct TimeLess;

      typedef ReferenceCounting::SmartPtr<TimeInterval> TimeInterval_var;
      typedef std::deque<TimeInterval_var> TimeIntervalArray;

      typedef Sync::Policy::PosixThreadRW TimeIntervalsSyncPolicy;

    protected:
      virtual
      ~HashFilter() noexcept
      {}

    protected:
      const unsigned long chunks_count_;
      const unsigned long chunk_size_;
      const Generics::Time keep_time_period_;
      const Generics::Time keep_time_;

      TimeIntervalsSyncPolicy::Mutex time_intervals_lock_;
      TimeIntervalArray time_intervals_;
    };

    typedef ReferenceCounting::SmartPtr<HashFilter> HashFilter_var;
  }
}

#endif /*HASHFILTER_HPP_*/
