#include <iostream>
#include "HashFilter.hpp"

namespace AdServer
{
  namespace Profiling
  {
    struct HashFilter::TimeLess
    {
      bool
      operator()(const TimeInterval* left, const Generics::Time& right)
      {
        return left->time_from < right;
      }

      bool
      operator()(const Generics::Time& left, const TimeInterval* right)
      {
        return left < right->time_from;
      }
    };

    // HashFilter::Mask
    HashFilter::Mask::Mask(unsigned long size)
      noexcept
      : data_(size / 8 + (size % 8 ? 1 : 0), 0)
    {}

    bool
    HashFilter::Mask::set_i(uint64_t hash)
      noexcept
    {
      uint64_t hash_mod = hash % (data_.size() * 8);
      unsigned char& cell = data_[hash_mod / 8];
      unsigned char mask = 1 << (hash_mod % 8);
      unsigned char prev_value = mask & cell;
      cell |= mask;
      return prev_value;
    }

    // HashFilter::Chunk
    HashFilter::Chunk::Chunk(unsigned long size)
      noexcept
      : mask_(size)
    {}

    bool
    HashFilter::Chunk::set(uint64_t hash)
      noexcept
    {
      SyncPolicy::WriteGuard lock(lock_);
      return mask_.set_i(hash);
    }

    // HashFilter::TimeInterval
    HashFilter::TimeInterval::TimeInterval(
      const Generics::Time& time_from_val,
      const Generics::Time& time_to_val,
      unsigned long chunks_num,
      unsigned long chunk_size)
      noexcept
      : time_from(time_from_val),
        time_to(time_to_val),
        chunks_(chunks_num, Chunk_var())
    {
      for(auto chunk_it = chunks_.begin(); chunk_it != chunks_.end(); ++chunk_it)
      {
        *chunk_it = new Chunk(chunk_size);
      }
    }

    bool
    HashFilter::TimeInterval::set(uint64_t hash)
      noexcept
    {
      unsigned long chunk_i = hash % chunks_.size();
      unsigned long internal_hash = hash / chunks_.size();
      return chunks_[chunk_i]->set(internal_hash);
    }

    // HashFilter
    HashFilter::HashFilter(
      unsigned long chunks_count,
      unsigned long chunk_size,
      const Generics::Time& keep_time_period,
      const Generics::Time& keep_time)
      noexcept
      : chunks_count_(chunks_count),
        chunk_size_(chunk_size),
        keep_time_period_(keep_time_period),
        keep_time_(keep_time)
    {}

    bool
    HashFilter::set(
      uint64_t hash,
      const Generics::Time& time,
      const Generics::Time& now)
      noexcept
    {
      const Generics::Time time_round =
        keep_time_period_ * (time / keep_time_period_.tv_sec).tv_sec;

      if(time_round < now - keep_time_)
      {
        // hash already expired
        return false;
      }

      TimeInterval_var time_interval;
      TimeInterval_var expired_time_interval;

      {
        TimeIntervalsSyncPolicy::ReadGuard lock(time_intervals_lock_);

        auto it = std::lower_bound(
          time_intervals_.begin(), time_intervals_.end(), time_round, TimeLess());
        if(it != time_intervals_.end() && (*it)->time_from == time_round)
        {
          time_interval = *it;
        }
      }

      if(!time_interval.in())
      {
        TimeIntervalsSyncPolicy::WriteGuard lock(time_intervals_lock_);

        auto it = std::lower_bound(
          time_intervals_.begin(),
          time_intervals_.end(),
          time_round,
          TimeLess());

        if(it != time_intervals_.end() && (*it)->time_from == time_round)
        {
          time_interval = *it;
        }
        else
        {
          TimeInterval_var new_time_interval = new TimeInterval(
            time_round,
            time_round + keep_time_period_,
            chunks_count_,
            chunk_size_);

          time_intervals_.insert(it, new_time_interval);
          time_interval.swap(new_time_interval);

          // thread that inserted new time interval should check expiration
          TimeInterval_var& last_time_interval = *time_intervals_.begin();
          if(last_time_interval->time_from + keep_time_period_ < now - keep_time_)
          {
            expired_time_interval.swap(last_time_interval);
            time_intervals_.pop_front();
          }
        }
      }

#     ifdef DEBUG_OUTPUT
      {
        std::cout << "intervals:";
        for(auto it = time_intervals_.begin(); it != time_intervals_.end(); ++it)
        {
          std::cout << " [" << (*it)->time_from.gm_ft() << ", " << (*it)->time_to.gm_ft() << " : " << it->in() << "]";
        }
        std::cout << std::endl;
      }
#     endif
      
      if(time_interval.in())
      {
        return time_interval->set(hash);
      }

      return false;
    }
  }
}

