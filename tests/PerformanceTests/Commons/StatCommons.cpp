
#include "StatCommons.hpp"

// Utils

Generics::Time time_from_str(const char* time_str) /*throw(Generics::Time::Exception, eh::Exception)*/
{
  Generics::Time time;
  std::stringstream ostr;
  ostr << time_str;
  ostr >> time;
  return time;
}

// Advertiser response
AdvertiserResponse::AdvertiserResponse(
  unsigned long ccid_,
  const char* click_url_,
  const char* action_adv_url_,
  const char* passback_url_,
  unsigned long trigger_channels_count_,
  unsigned long history_channels_count_,
  unsigned long ccids_count_,
  const char* trigger_match_time_,
  const char* request_fill_time_,
  const char* history_match_time_,
  const char* creative_selection_time_,
  bool optout_)
  /*throw(Generics::Time::Exception, eh::Exception)*/ :
  ccid(ccid_),
  click_url(click_url_),
  action_adv_url(action_adv_url_),
  passback_url(passback_url_),
  trigger_channels_count(trigger_channels_count_),
  history_channels_count(history_channels_count_),
  ccids_count(ccids_count_),
  trigger_match_time(time_from_str(trigger_match_time_)),
  request_fill_time(time_from_str(request_fill_time_)),
  history_match_time(time_from_str(history_match_time_)),
  creative_selection_time(time_from_str(creative_selection_time_)),
  optout(optout_)
{ }

AdvertiserResponse::~AdvertiserResponse() noexcept
{ }

// Counter class

const double Counter::HUNDRED = 100.0;

Counter::Counter() :
  total_(0),
  diff_(0)
{ }

Counter::~Counter() noexcept
{ }

void Counter::incr(unsigned long increment)
{
  total_+=increment;
  diff_+=increment;
}

void Counter::reset()
{
  diff_ = 0;
}

double Counter::percentage() const
{
  return 0;
}

unsigned long Counter::get() const
{
  return total_;
}

unsigned long Counter::diff() const
{
  return diff_;
}


// SlaveCounter class

SlaveCounter::SlaveCounter(Counter* master) :
  master_(master)
{ }

SlaveCounter::~SlaveCounter() noexcept
{ }

double SlaveCounter::percentage() const
{
  if (master_)
  {
    unsigned long master_total_ = master_->get();
    return master_total_ ? double(total_) * HUNDRED / double(master_total_) : 0;
  }
  return Counter::percentage();
}


// ConstraintableCounter class

ConstraintableCounter::ConstraintableCounter(Counter* master) :
  SlaveCounter(master),
  constraint_(0)
{ }

ConstraintableCounter::~ConstraintableCounter() noexcept
{ }

void ConstraintableCounter::set_constraint(ConstraintElement* constraint)
{
  constraint_ = constraint;
}

void ConstraintableCounter::incr(unsigned long increment)
{
  SlaveCounter::incr(increment);
  if (constraint_)
    {
      constraint_->push(diff_, master_->get());
    }
}

// PerformanceItem class

PerformanceItem::PerformanceItem() :
  max_(Generics::Time::ZERO),
  min_(Generics::Time::ZERO),
  average_()
{ }

PerformanceItem::~PerformanceItem() noexcept
{ }
  
void PerformanceItem::push(const Generics::Time& time)
{
  min_ = (time < min_ || min_ == Generics::Time::ZERO)? time : min_;
  max_ = time > max_ ? time : max_;
  average_.push(time);
}

const Generics::Time& PerformanceItem::max() const
{
  return max_;
}

const Generics::Time& PerformanceItem::min() const
{
  return min_;
}

Generics::Time PerformanceItem::average() const
{
  return average_.get();
}

void PerformanceItem::reset()
{
  max_ = Generics::Time::ZERO;
  min_ = Generics::Time::ZERO;
  average_.reset();
}

//  PerformanceStatisticsBase class

const PerformanceStatisticsBase::ItemList&
  PerformanceStatisticsBase::items() const
{
  return items_;
}

void PerformanceStatisticsBase::reset()
{
  for (ItemList::iterator it = items_.begin();
    it != items_.end(); ++it)
  {
    it->second->reset();
  }
}

PerformanceItem_var PerformanceStatisticsBase::get_item(const char* item_name)
{
  ItemList::iterator it = items_.find(item_name);
  if (it == items_.end())
  {
    PerformanceItem_var item(new PerformanceItem());
    items_[item_name] = item;
    return item;
  }
  return it->second;
}

// AdvPerformanceStats class

AdvPerformanceStats::AdvPerformanceStats()
{
  data_map["Trigger match"] = &AdvertiserResponse::trigger_match_time;
  data_map["Request fill"] = &AdvertiserResponse::request_fill_time;
  data_map["History match"] = &AdvertiserResponse::history_match_time;
  data_map["Creative selection"] = &AdvertiserResponse::creative_selection_time;    
}

// RangeStats class

RangeStats::RangeStats() :
  max_(0),
  min_(0),
  current(0),
  average_(),
  init_flag(true)
{ }

void RangeStats::push(long value)
{
  if (value > max_) max_ = value;
  if (value < min_ || init_flag)
    {
      min_ = value;
      init_flag = false;
    }
  average_.push(value);
  current+=value;
}

void RangeStats::reset()
{
  max_ = 0;
  min_ = 0;
  current = 0;
  average_.reset();
  init_flag = true;
}

long RangeStats::get() const
{
  return current;
}

long RangeStats::min() const
{
  return min_;
}

long RangeStats::max() const
{
  return max_;
}

const Average <long>& RangeStats::average() const
{
  return average_;
}
