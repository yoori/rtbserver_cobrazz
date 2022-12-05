
#include "Statistics.hpp"

// class Statistics

Statistics::Statistics() :
  errors_(0),
  creatives_(0),
  responses_(0),
  start_time_(Generics::Time::get_time_of_day())
{
}

void Statistics::incr_errors()
{
  WriteGuard_ guard(lock_);
  errors_++;
}

void Statistics::add_response(const char* uid,
                              const AdvertiserResponse* ad_response)
{
  WriteGuard_ guard(lock_);
  uids_.insert(uid);
  responses_++;
  if (ad_response)
  {
    trigger_channels_stats_.push(ad_response->trigger_channels_count);
    history_channels_stats_.push(ad_response->history_channels_count);
    creatives_+=ad_response->ccids_count;
    adv_performance_stats_.push(ad_response);
  }
}

unsigned long Statistics::errors() const
{
  ReadGuard_ guard(lock_);
  return errors_;
}

unsigned long Statistics::creatives() const
{
  ReadGuard_ guard(lock_);
  return creatives_;
}

unsigned long Statistics::responses() const
{
  ReadGuard_ guard(lock_);
  return responses_;
}


size_t Statistics::uids_size() const
{
  ReadGuard_ guard(lock_);
  return uids_.size();
}

Generics::Time Statistics::duration() const
{
  ReadGuard_ guard(lock_);
  return Generics::Time::get_time_of_day() - start_time_;
}

const RangeStats& Statistics::trigger_channels_stats() const
{
   return trigger_channels_stats_;
}

const RangeStats& Statistics::history_channels_stats() const
{
   return history_channels_stats_;
}

const AdvPerformanceStats& Statistics::adv_performance_stats() const
{
   return adv_performance_stats_;
}

void Statistics::reset()
{
  ReadGuard_ guard(lock_);
  uids_.clear();
  creatives_ = 0;
  errors_ = 0;
  responses_ = 0;
  trigger_channels_stats_.reset();
  history_channels_stats_.reset();
  adv_performance_stats_.reset();
  start_time_ = Generics::Time::get_time_of_day();
}
