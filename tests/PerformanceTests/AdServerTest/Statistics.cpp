
#include "Statistics.hpp"
#include "Report.hpp"

// constants

namespace
{
  // Request paths
  const char NSLOOKUP_PATH[] = "/services/nslookup";
  const char CLICK_PATH[] = "/services/AdClickServer";
  const char ACTION_PATH[] = "/services/ActionServer";
  const char PASSBACK_PATH[] = "/services/passback";
  const char OPTOUT_PATH[] = "/services/OO";

  // Request aliases
  const char NSLOOKUP_ALIAS[] = "Nslookup";
  const char CLICK_ALIAS[] = "Click";
  const char ACTION_ALIAS[] = "Action";
  const char PASSBACK_ALIAS[] = "Passback";
  const char OPTOUT_ALIAS[] = "Optout";

  const char ADV_SIGN[] = "tid=";

  const unsigned long FRONTEND_ARRAY_SIZE = 5;
  const char* FRONTEND_NAMES[FRONTEND_ARRAY_SIZE] = { NSLOOKUP_PATH, CLICK_PATH,
                                                      ACTION_PATH, PASSBACK_PATH,
                                                      OPTOUT_PATH};
  const char* FRONTEND_ALIASES[FRONTEND_ARRAY_SIZE] = { NSLOOKUP_ALIAS, CLICK_ALIAS,
                                                        ACTION_ALIAS, PASSBACK_ALIAS,
                                                        OPTOUT_ALIAS};
}

// RequestCounterContainer class

RequestCounterContainer::RequestCounterContainer(Counter* master) :
  total_(master),
  opted_in_(master? master: &total_),
  opted_out_(master? master: &total_)
{ }


void RequestCounterContainer::incr(bool is_optedout)
{
  total_.incr();
  if (is_optedout)
  {
    opted_out_.incr();
  }
  else
  {
    opted_in_.incr();
  }
}

void RequestCounterContainer::reset()
{
  total_.reset();
  opted_out_.reset();
  opted_in_.reset();
}

const ConstraintableCounter& RequestCounterContainer::total() const
{
  return total_;
}

const SlaveCounter& RequestCounterContainer::opted_in() const
{
  return opted_in_;
}

const SlaveCounter& RequestCounterContainer::opted_out() const
{
  return opted_out_;
}

// AdvertisingStatistics class

AdvertisingStatistics::AdvertisingStatistics(const char* frontend_name) :
  frontend_name_(frontend_name),
  total_responses_(0),
  creative_responses_(&total_responses_.total_)
{}

AdvertisingStatistics::~AdvertisingStatistics() noexcept
{}

void AdvertisingStatistics::push_response(unsigned long ccid,
                                          bool optout)
{
  total_responses_.incr(optout);
  if (ccid)
  {
    creative_responses_.incr(optout);
  }
  RequestCounterContainerList::iterator it = ccid_stats_.find(ccid);
  if (it == ccid_stats_.end())
  {
    RequestCounterContainer_var
        counter(new  RequestCounterContainer(&total_responses_.total_));
    counter->incr(optout);
    ccid_stats_[ccid] = counter;
  }
  else
  {
    (it->second)->incr(optout);
  }
}

const std::string& AdvertisingStatistics::frontend_name() const
{
  return frontend_name_;
}

const RequestCounterContainer&
  AdvertisingStatistics::total_responses() const
{
  return total_responses_;
}

const RequestCounterContainer& AdvertisingStatistics::creative_responses() const
{
  return creative_responses_;
}

const AdvertisingStatistics::RequestCounterContainerList&
  AdvertisingStatistics::ccid_stats() const
{
  return ccid_stats_;
}

// FrontendStatistics class

FrontendStatistics::FrontendStatistics(const char* frontend_name,
                                       const ClientConfig_var& config,
                                       RequestCounterContainer& total_requests,
                                       ConstraintsContainer& constraints) :
  constraints_(constraints),
  total_requests_(total_requests),
  frontend_name_(frontend_name),
  constraint_(_get_constraint_by_name(config, frontend_name)),
  error_constraint_(),
  requests_(&total_requests_.total_),
  approved_(&requests_.total_),
  errors_(&requests_.total_),
  start_time_(0)
{
  start_time_ = time(NULL);
  if (constraint_.in())
  {
    ConstraintElement* error_constraint_ptr =
      new ConstraintElement("Failed requests",
                            frontend_name,
                            constraint_->sampling_size,
                            constraint_->failed_percentage);
    error_constraint_ = Constraint_var(error_constraint_ptr);
    errors_.total_.set_constraint(error_constraint_ptr);
    constraints.register_constraint(error_constraint_);
  }
}

FrontendStatistics::~FrontendStatistics() noexcept
{}


bool FrontendStatistics::push_response(bool is_opted_out)
{
  requests_.incr(is_opted_out);
  approved_.incr(is_opted_out);
  return true;
}

void FrontendStatistics::push_error(bool is_opted_out)
{
  requests_.incr(is_opted_out);
  errors_.incr(is_opted_out);
}

const RequestCounterContainer& FrontendStatistics::requests() const
{
  return requests_;
}

const RequestCounterContainer& FrontendStatistics::approved() const
{
  return approved_;
}

const RequestCounterContainer& FrontendStatistics::errors() const
{
  return errors_;
}

const std::string& FrontendStatistics::frontend_name() const
{
  return frontend_name_;
}

const ConstraintConfig_var&
FrontendStatistics::_get_constraint_by_name(const ClientConfig_var& config,
                                            const char* frontend_name)
{
  if (strcmp(frontend_name, NSLOOKUP_ALIAS) == 0 )
  {
    return config->ns_request()->constraint;
  }
  if (strcmp(frontend_name, CLICK_ALIAS) == 0 )
  {
    return config->click_request()->constraint;
  }
  if (strcmp(frontend_name, ACTION_ALIAS) == 0 )
  {
    return config->action_request()->constraint;
  }
  if (strcmp(frontend_name, OPTOUT_ALIAS) == 0 )
  {
    return config->optout_request()->constraint;
  }
  if (strcmp(frontend_name, PASSBACK_ALIAS) == 0 )
  {
    return config->passback_request()->constraint;
  }
  return config->ns_request()->constraint;
}


// Statistics class

Statistics::Statistics(ConstraintsContainer& constraints,
                       const ClientConfig_var& config) :
  constraints_(constraints),
  config_(config),
  total_requests_(0),
  total_approved_(&total_requests_.total_),
  total_errors_(&total_requests_.total_),
  profiling_requests_(&total_requests_.total_),
  request_channels_stats_()
{}


void Statistics::push_response(const char* url,
                               bool is_opted_out,
                               unsigned long ccid,
                               const AdvertiserResponse* ad_response)
{
  WriteGuard_ guard(lock_);
  total_requests_.incr(is_opted_out);
  total_approved_.incr(is_opted_out);
  std::string frontend_name;
  _get_frontend_name(url, frontend_name);
  _get_frontend_stat_by_name(frontend_name.c_str())->push_response(is_opted_out);
  if (ad_response)
  {
    request_channels_stats_.push(ad_response->history_channels_count);
    adv_performance_stats_.push(ad_response);
    if (_is_profiling_req(url))
    {
      profiling_requests_.incr(is_opted_out);
    }
  }
  _get_advertising_stat_by_name(frontend_name.c_str())->push_response(ccid, is_opted_out);
}

void Statistics::refresh(Report* report,
                         Statistics::ResetEnum reset)
{
  ReadGuard_ guard(lock_);
  report->publish();
  switch(reset)
  {
  case RST_COUNTERS:
    total_errors_.reset();
    total_approved_.reset();
    total_requests_.reset();
    break;
  case RST_CHANNELS:
    request_channels_stats_.reset();
    break;
  default:
    break;
  }
}

const RequestCounterContainer& Statistics::total_requests() const
{
   return total_requests_;
}

const RequestCounterContainer& Statistics::total_approved() const
{
  return total_approved_;
}

const RequestCounterContainer& Statistics::total_errors() const
{
  return total_errors_;
}

const RequestCounterContainer& Statistics::profiling_requests() const
{
   return profiling_requests_;
}

const RangeStats& Statistics::request_channels_stats() const
{
  return request_channels_stats_;
}

const AdvPerformanceStats& Statistics::adv_performance_stats() const
{
  return adv_performance_stats_;
}

const AdvertisingStatList& Statistics::adv_stats() const
{
  return adv_stats_;
}


const FrontendStatList& Statistics::frontend_stats() const
{
  return frontend_stats_;
}

AdvertisingStatistics* Statistics::get_nslookup_stats()
{
  AdvertisingStatList::iterator it = adv_stats_.find(NSLOOKUP_ALIAS);
  if (it == adv_stats_.end())
  {
    return 0;
  }
  return it->second;
}

void Statistics::push_error(const char* url,
                            bool is_opted_out)
{
  WriteGuard_ guard(lock_);
  total_requests_.incr(is_opted_out);
  total_errors_.incr(is_opted_out);
  std::string frontend_name;
  _get_frontend_name(url, frontend_name);
  _get_frontend_stat_by_name(frontend_name.c_str())->push_error(is_opted_out);
}

void Statistics::_get_frontend_name(const char* url, std::string& frontend_name)
{
  std::string _url(url);
  frontend_name = _url;
  for (unsigned int i = 0; i < FRONTEND_ARRAY_SIZE; i++)
    {
      if(_url.find(FRONTEND_NAMES[i]) != std::string::npos)
        {
          frontend_name = FRONTEND_ALIASES[i];
          return;
        }
    }
}

FrontendStatistics_var Statistics::_get_frontend_stat_by_name(const char* frontend_name)
{
  FrontendStatList::iterator it = frontend_stats_.find(frontend_name);
  if (it == frontend_stats_.end())
    {
      FrontendStatistics_var stat(new FrontendStatistics(frontend_name, config_,
                                                         total_requests_, constraints_));
      frontend_stats_[frontend_name] = stat;
      return stat;
    }
  return it->second;
}

AdvertisingStatistics_var Statistics::_get_advertising_stat_by_name(const char* frontend_name)
{
  AdvertisingStatList::iterator it = adv_stats_.find(frontend_name);
  if (it == adv_stats_.end())
    {
      AdvertisingStatistics_var stat(new AdvertisingStatistics(frontend_name));
      adv_stats_[frontend_name] = stat;
      return stat;
    }
  return it->second;
}

bool  Statistics::_is_profiling_req(const char* url)
{
  return strstr(url, NSLOOKUP_PATH) && !strstr(url, ADV_SIGN);
}





