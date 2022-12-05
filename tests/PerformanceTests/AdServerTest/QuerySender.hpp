
#ifndef __QUERYSENDER_HPP
#define __QUERYSENDER_HPP

#include <time.h>

#include <ace/Task.h>
#include <ReferenceCounting/Vector.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/Scheduler.hpp>

#include <HTTP/HttpClient.hpp>

#include <tests/PerformanceTests/Commons/Request.hpp>
#include <tests/PerformanceTests/Commons/QuerySenderBase.hpp>


#include "Configuration.hpp"
#include "Statistics.hpp"

class QuerySender;

/**
 * @class QueryScheduledTask
 * @brief Task for pushing Action & Click requests.
 */
class QueryScheduledTask : public Generics::Goal,
                           public ReferenceCounting::AtomicImpl
{
public:

  /**
   * @brief Constructor.
   *
   * @param owner pointer for main test class - QuerySender.
   * @param request pointer for request.
   */
  QueryScheduledTask(QuerySender* owner,
                     BaseRequest* request);

  /**
   * @brief Destructor.
   */
  ~QueryScheduledTask() noexcept;

  /**
   * @brief Callback function to be called from the scheduler
   *        Override Generics::Goal::deliver(...)
   */
  virtual void
  deliver() noexcept;

private:
  QuerySender* owner_;
  BaseRequest* request_;
};


/**
 * @class ThresholdConstraint
 * @brief Threshold constraint checker.
 */
template <class T> class ThresholdConstraint : public BaseConstraint
{

public:
  /**
   * @brief Constructor.
   *
   * @param name_ threshold name.
   * @param description_ threshold description.
   * @param threshold_value "trap event" value.
   * @param init_value initial value.
   */
  ThresholdConstraint(const char* name_,
                      const char* description_,
                      const T threshold_value,
                      T init_value) :
    BaseConstraint(name_, description_),
    current_value_(init_value),
    threshold_value_(threshold_value)
  { }

  /**
   * @brief Destructor.
   */
  ~ThresholdConstraint() noexcept
  { }

  /**
   * @brief update current value.
   *
   * @param value new value.
   */
  void set_value(T value)
  {
    current_value_ = value;
  }

  /**
   * @brief check threshold on "trap event".
   *
   * @return true if "trap event", otherwise false.
   */
  virtual bool check()
  {
    if (current_value_ > threshold_value_)
      {
        std::ostringstream ostr;
        ostr << "Constraint '" << name << "' (" << description <<
          ") failed, because size (" << current_value_ <<
          ") more than allowable limit (" << threshold_value_ <<
          ")";
        error_ = ostr.str();
        return false;
      }
    return true;
  }

private:
  T current_value_;
  const T threshold_value_;
};


//
/**
 * @class QuerySender
 * @brief HTTP-request sender for performance test.
 *        Main class setting a test logic.
 */
class QuerySender : public QuerySenderBase,
                    public ACE_Task<ACE_MT_SYNCH> // FIXME Get rid of ACE
{

  typedef Sync::PosixRWLock Mutex_;
  typedef Sync::PosixRGuard ReadGuard_;
  typedef Sync::PosixWGuard WriteGuard_;

public:

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:

  /**
   * @brief Constructor.
   *
   * @param cfg test configuration.
   * @param logger
   * @param scheduler
   * @param http_pool HTTP pool for sending requests to server.
   * @param http_policy policy for http_pool.
   */
  QuerySender(
    Configuration& cfg,
    Logging::Logger* logger,
    Generics::Planner_var& scheduler,
    HttpActiveInterface_var& http_pool,
    HttpPoolPolicy* http_policy);

  /**
   * Destructor
   */
  virtual ~QuerySender() noexcept;

  /**
   * @brief Start test.
   */
  void start() /*throw(Exception)*/;

  /**
   * @brief Shutdown test.
   */
  void shutdown();

  /**
   * @brief Callback calling from BaseRequest after getting correct response.
   */
  virtual void on_response(
    unsigned long client_id,
    const HTTP::ResponseInformation& data,
    bool is_opted_out,
    unsigned long ccid = 0,
    const AdvertiserResponse* ad_response = 0) noexcept;

  /**
   * @brief Callback calling from BaseRequest after getting HTTP error.
   */
  virtual void on_error(
    const String::SubString& description,
    const HTTP::ResponseInformation& data,
    bool is_opted_out) noexcept;


  /**
   * @brief test work thread.
   */
  virtual int svc (void) noexcept;

  /**
   * @brief pop request from sender's query
   */
  void enqueue_request(BaseRequest* request);

  /**
   * @brief Dumping statistic to log
   */
  void dump(bool need_full = false);

  /**
   * @brief Dumping statistic in confluence format
   */
  void dump_confluence_report();

  /**
   * @brief get test total duration
   */
  Generics::Time get_total_duration();

private:
  bool _shutdown_check();
  void _shutdown_set();

  void _init_clients();

  void _send_main_request();
  void _process_request(BaseRequest* request);
  unsigned long _get_requests_increment();
  void _log_response(const char* http_request,
                     int response_code,
                     const char* debug_info = 0);
  void _schedule_request(BaseRequest* request,
                         unsigned long request_delay);
  void _schedule_child_requests(unsigned long client_id,
                                bool is_opted_out,
                                const char* click_url,
                                const char* action_adv_url,
                                const char* passback_url);
  unsigned long _send_queued_requests(unsigned long max_requests);

  bool _need_optout();

  Configuration& cfg_;
  Logging::Logger* logger_;
  Generics::Planner_var& scheduler_;
  HttpActiveInterface_var& http_pool_;
  HttpPoolPolicy* http_policy_;
  ConstraintsContainer constraints_;
  Statistics stats_;
  volatile sig_atomic_t deactivated_;
  unsigned long action_count_;
  unsigned long click_count_;
  unsigned long passback_count_;
  time_t full_dump_time_;

  ReferenceCounting::Vector<HttpInterface_var> clients_;
  ReferenceCounting::Vector<CookiePool_var> cookies_;

  mutable Mutex_ lock_;
  mutable Mutex_ shutdown_lock_;

  ThresholdConstraint<unsigned long>* pool_queue_constraint_;
  ThresholdConstraint<unsigned long>* sender_queue_constraint_;
  ThresholdConstraint<Generics::Time>* execution_time_constraint_;

  Generics::Time start_time_;

};

#endif  // __QUERYSENDER_HPP
