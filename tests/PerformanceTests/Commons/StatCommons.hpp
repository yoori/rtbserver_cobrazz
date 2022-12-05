
#ifndef __STATCOMMONS_HPP
#define __STATCOMMONS_HPP

#include <map>
#include "Constraint.hpp"
#include "ConfigCommons.hpp"
#include <Generics/Time.hpp>

/**
 * @struct AdvertiserResponse
 * @brief Contain advertiser request properties
 */
struct AdvertiserResponse
{
  /**
   * @brief CCID
   *
   * CCID containing in the advertising response.
   */
  unsigned long ccid;

  /**
   * @brief Click URL
   *
   * Click URL from advertising response
   */
  std::string click_url;

  /**
   * @brief Action URL
   *
   * Action URL from advertising response
   */
  std::string action_adv_url;

  /**
   * @brief Passback URL
   *
   * Passback URL from advertising response
   */
  std::string passback_url;

  /**
   * @brief Trigger channels size
   *
   * Count of trigger channels matched by
   * advertising request
   */
  unsigned long trigger_channels_count;
  
  /**
   * @brief History channels size
   *
   * Count of history channels matched by
   * advertising request
   */
  unsigned long history_channels_count;

  /**
   * @brief Selected creatives size
   *
   * Count of creatives selected by
   * advertising request
   */
  unsigned long ccids_count;

  /**
   * @brief Time of trigger matching
   *
   * Trigger matching time measured by adserver
   */
  Generics::Time trigger_match_time;

  /**
   * @brief Filling request time
   *
   * Filling request time measured by adserver
   */
  Generics::Time request_fill_time;

  /**
   * @brief History matching time
   *
   * History matching time measured by adserver
   */
  Generics::Time history_match_time;

  /**
   * @brief Creative selection time
   *
   * Creative selection time measured by adserver
   */
  Generics::Time creative_selection_time;

  /**
   * @brief Optout flag
   *
   * true - if request is optout
   */
  bool optout;
  

  /**
   * @brief Constructor.
   * @param CCID
   * @param Click URL
   * @param Action URL
   * @param Passback URL
   * @param trigger channels count
   * @param history channels count
   * @param trigger match time
   * @param request fill time
   * @param history match time
   * @param creative selection time
   */
  AdvertiserResponse(
    unsigned long ccid,
    const char* click_url,
    const char* action_adv_url,
    const char* passback_url,
    unsigned long trigger_channels_count,
    unsigned long history_channels_count,
    unsigned long ccids_count,
    const char* trigger_match_time,
    const char* request_fill_time,
    const char* history_match_time,
    const char* creative_selection_time,
    bool optout)
    /*throw(Generics::Time::Exception, eh::Exception)*/;

  /**
   * @brief Destructor.
   */
  virtual ~AdvertiserResponse() noexcept;

  /**
   * @brief Pointer to class member.
   */  
  typedef AdvertiserResponse AdvertiserResponse::* Member;

};

typedef std::unique_ptr<AdvertiserResponse> AdvertiserResponse_var;


/**
 * @struct Simple counter
 * @brief Implement simple request counter
 */
class Counter 
{
protected:

  static const double HUNDRED;
  static const unsigned long INCREMENT = 1;

public:
  /**
   * @brief Default constructor.
   */  
  Counter();

  /**
   * @brief Destructor
   */  
  virtual ~Counter() noexcept;

  /**
   * @brief Increment counter
   * @param increment value
   */  
  virtual void incr(unsigned long increment = INCREMENT);

  /**
   * @brief Reset counter diff
   *
   * Using when start new second
   */  
  virtual void reset();

  /**
   * @brief Percentage
   *
   * @return 0 in this class
   */  
  virtual double percentage() const;

  /**
   * @brief Total
   *
   * @return total counts ov events
   */  
  virtual unsigned long get() const;

  /**
   * @brief Diffs
   *
   * @return counts ov events between 2 resets
   */  
  virtual unsigned long diff() const;

protected:
  unsigned long total_;
  unsigned long diff_;
};


/**
 * @struct Average
 * @brief Calculate average value of entities
 */
template <class T> class Average
{
public:

  /**
   * @brief Constructor.
   */  
  Average() :
    sum_(0),
    size_(0)
  {
  }

  /**
   * @brief Push new value.
   * @param value
   */  
  void push(const T& val)
  {
    sum_ += val;
    size_++;
  }

  /**
   * @brief Get average value.
   */  
  T get() const
  {
    if (!size_) return static_cast<T>(0);
    return sum_ / size_;
  }

  void reset()
  {
    size_ = 0;
    sum_ = static_cast<T>(0);
  }
  
private:

  T sum_;
  unsigned long size_;
};

/**
 * @struct SlaveCounter
 * @brief Counter depending on master counter
 */
class SlaveCounter : public Counter
{
public:

  /**
   * @brief Constructor.
   * @param master counter
   */  
  SlaveCounter(Counter* master);

  /**
   * @brief Destructor.
   */  
  ~SlaveCounter() noexcept;

  /**
   * @brief Percentage.
   * @return percentage of master counter
   */  
  double percentage() const;

protected:
  Counter* master_;
};

/**
 * @struct ConstraintableCounter
 * @brief Counter with constraint check
 */
class ConstraintableCounter : public SlaveCounter
{
public:

  /**
   * @brief Constructor.
   * @param master counter
   */  
  ConstraintableCounter(Counter* master);

  /**
   * @brief Destructor.
   */  
  ~ConstraintableCounter() noexcept;

  /**
   * @brief Set constraint.
   * @param constraint element
   */
  void set_constraint(ConstraintElement* constraint);

  /**
   * @brief Increment counter
   * @param increment value
   */  
  void incr(unsigned long increment = INCREMENT);

private:
  ConstraintElement* constraint_;
};

/**
 * @struct PerformanceItem
 * @brief Item for storing performance statistic
 */
class PerformanceItem:
  public virtual ReferenceCounting::Interface,
  public virtual ReferenceCounting::AtomicImpl
{
public:

  /**
   * @brief Default constructor.
   */  
  PerformanceItem();

  /**
   * @brief Destructor.
   */  
  ~PerformanceItem() noexcept;

  /**
   * @brief push duration.
   * @param time duration.
   */  
  void push(const Generics::Time& time);

  /**
   * @brief Get max duration.
   * @param max duration.
   */  
  const Generics::Time& max() const;

  /**
   * @brief Get min duration.
   * @param min duration.
   */  
  const Generics::Time& min() const;

  /**
   * @brief Get average duration.
   * @param average duration.
   */  
  Generics::Time average() const;

  /**
   * @brief Reset.
   */  
  void reset();

private:
  Generics::Time max_;
  Generics::Time min_;
  Average <Generics::Time> average_;
  
};

typedef ReferenceCounting::SmartPtr<PerformanceItem> PerformanceItem_var;

/**
 * @struct PerformanceStatisticsBase
 * @brief Base class for performance statistic's containers
 */
class PerformanceStatisticsBase
{

public:
  typedef std::map <std::string, PerformanceItem_var> ItemList;

public:

  /**
   * @brief Get performance items dictionary.
   * @param items dictionary (name => performance item).
   */    
  const ItemList& items() const;

  /**
   * @brief Reset all items.
   */    
  void reset();

protected:
  ItemList items_;

protected:

  /**
   * @brief Get item by name.
   * @param name
   * @return performance item.
   */    
  PerformanceItem_var get_item(const char* item_name);
};


/**
 * @struct PerformanceStatistics template
 * @brief Using for link different request's types (template parameter)
 *        with performance statistic.
 */
template <class T> class PerformanceStatistics : public PerformanceStatisticsBase
{

  typedef Generics::Time T::* ResponseMember;

  typedef std::map <std::string, ResponseMember> DataMapType;

  typedef typename DataMapType::const_iterator dt_const_iterator;
  typedef typename DataMapType::iterator dt_iterator;
  typedef typename DataMapType::value_type dt_value_type;
  typedef typename DataMapType::size_type  dt_size_type;
 
public:

  /**
   * @brief Push response .
   * @param response
   */    
  void  push(const T* response)
  {
    if (response)
    {
      dt_iterator it = data_map.begin(),
                 end = data_map.end();
      for (; it != end;++it)
      {
        get_item(it->first.c_str())->push(response->*(it->second));
      }
    }
  }
 
protected:
  DataMapType data_map;
};

/**
 * @struct AdvPerformanceStats
 * @brief Performance statistics for advertising requests.
 */
class AdvPerformanceStats : public PerformanceStatistics<AdvertiserResponse>
{
public:
  
  /**
   * @brief Default constructor.
   */  
  AdvPerformanceStats();
};

/**
 * @struct RangeStats
 * @brief Range statistics, contain min, max, current & average for observed parameter.
 */
class RangeStats
{
public:
  /**
   * @brief Default constructor.
   */  
  RangeStats();

  /**
   * @brief Push.
   * @param value
   */  
  void push(long value);

  /**
   * @brief Reset.
   */  
  void reset();

  /**
   * @brief Get value.
   * @return current value.
   */  
  long get() const;

  /**
   * @brief Get max value.
   * @return max value.
   */  
  long max() const;

  /**
   * @brief Get min value.
   * @return min value.
   */  
  long min() const;

  /**
   * @brief Get average value.
   * @return average value.
   */  
  const Average <long>& average() const;
 
private:
  long max_;
  long min_;
  long current;
  Average <long> average_;
  bool init_flag;
};

#endif  // __STATCOMMONS_HPP
