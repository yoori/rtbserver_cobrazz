
#include <ace/Reactor.h>

#include "QuerySender.hpp"
#include <HTTP/HttpSync.hpp>
#include "SecondStartWaiter.hpp"
#include "Report.hpp"


namespace
{
  const unsigned long POOL_SIZE_THRESHOLD  = 8000;
  const unsigned long QUEUE_SIZE_THRESHOLD = 10000;
}

// Utils

void
print_cookie(std::ostream& out, const CookieDef& cookie)
{
  out << "    " << cookie.name << "=" << cookie.value << " " <<
      cookie.domain << " " << cookie.path << " " <<
      HTTP::cookie_date(cookie.expires) <<
      (cookie.secure ? " secure" : "") << std::endl;
}

class PrintCookies : public HTTP::ClientCookieFacility
{
public:
  void
  print_cookies(std::ostream& ostr) /*throw(eh::Exception)*/
  {
    ostr << "  Client cookies: " << std::endl;
    for (iterator cookie_it(begin()); cookie_it != end(); ++cookie_it)
    {
      print_cookie(ostr, *cookie_it);
    }
  }
};

bool url_not_empty (const char* url)
{
  if (!url) return false;
  return strlen(url) != 0;
}

bool need_send(unsigned long request_index,
               double percentage)
{
  double prc = percentage >= 100? 100: percentage < 0? 0: percentage;
  int last_index =
    int(double(request_index)*prc) / 100;
  int current_index =
    int(double(request_index + 1)*prc) / 100;
  return (current_index > last_index);
}

// QueryScheduledTask class

QueryScheduledTask::QueryScheduledTask(QuerySender* owner,
                                       BaseRequest* request) :
  owner_(owner),
  request_(request)
{ }

QueryScheduledTask::~QueryScheduledTask() noexcept
{ }

void  QueryScheduledTask::deliver()
  noexcept
{
  owner_->enqueue_request(request_);
}

// QuerySender class

QuerySender::QuerySender(
  Configuration& cfg,
  Logging::Logger* logger,
  Generics::Planner_var& scheduler,
  HttpActiveInterface_var& http_pool,
  HttpPoolPolicy* http_policy) :
  ACE_Task<ACE_MT_SYNCH> (ACE_Thread_Manager::instance ()), // FIXME Get rid of ACE
  cfg_(cfg),
  logger_(logger),
  scheduler_(scheduler),
  http_pool_(http_pool),
  http_policy_(http_policy),
  constraints_("Performance test constraints", "container"),
  stats_(constraints_, cfg_.client_config()),
  deactivated_(false),
  action_count_(0),
  click_count_(0),
  passback_count_(0),
  full_dump_time_(0),
  pool_queue_constraint_(0),
  sender_queue_constraint_(0),
  execution_time_constraint_(0),
  start_time_(Generics::Time::get_time_of_day())
{
  full_dump_time_ = time(NULL);
  pool_queue_constraint_ =
    new ThresholdConstraint <unsigned long>("Size constraint",
                                            "HTTP pool queue",
                                            POOL_SIZE_THRESHOLD,
                                            0);
  sender_queue_constraint_ =
    new ThresholdConstraint<unsigned long>("Size constraint",
                                           "Actions&Clicks queue",
                                           QUEUE_SIZE_THRESHOLD,
                                           0);
  execution_time_constraint_ =
    new ThresholdConstraint<Generics::Time>("Test execution",
                                            "Time exceed",
                                            Generics::Time(
                                              cfg_.execution_time()),
                                            Generics::Time::ZERO);
  Constraint_var pool_queue_constraint_var =
    Constraint_var(pool_queue_constraint_);
  Constraint_var sender_queue_constraint_var =
    Constraint_var(sender_queue_constraint_);
  Constraint_var execution_time_constraint_var =
    Constraint_var(execution_time_constraint_);
  constraints_.register_constraint(pool_queue_constraint_var);
  constraints_.register_constraint(sender_queue_constraint_var);
  constraints_.register_constraint(execution_time_constraint_var);
}

QuerySender::~QuerySender() noexcept
{
}

void QuerySender::start() /*throw(Exception)*/
{
  logger_->stream(Logging::Logger::TRACE) << "Query sender start";
  _init_clients();
  int error = ACE_Task<ACE_MT_SYNCH>::activate (THR_NEW_LWP, 1); // FIXME Get rid of ACE
  if ( error != 0)
    {
      logger_->stream(Logging::Logger::CRITICAL) <<
        "QuerySender thread start failed";
      throw Exception("QuerySender thread start failed");
    }
}

void QuerySender::shutdown()
{
  _shutdown_set();
}


void QuerySender::on_response(
  unsigned long client_id,
  const HTTP::ResponseInformation& data,
  bool is_opted_out,
  unsigned long ccid,
  const AdvertiserResponse* ad_response) noexcept
{

  _log_response(data.http_request(), data.response_code());
  stats_.push_response(data.http_request(), is_opted_out, ccid,
                       ad_response);
  if (ad_response)
  {
    _schedule_child_requests(client_id,
                             is_opted_out,
                             ad_response->click_url.c_str(),
                             ad_response->action_adv_url.c_str(),
                             ad_response->passback_url.c_str());
  }
}

void QuerySender::on_error(
  const String::SubString& description,
  const HTTP::ResponseInformation& data,
  bool is_opted_out) noexcept
{
  logger_->stream(Logging::Logger::TRACE) << description;
  stats_.push_error(data.http_request(),
                    is_opted_out);
}

int QuerySender::svc() noexcept
{
  try
    {
      unsigned long request_per_sec = cfg_.client_config()->count;
      // Main performance test cycle
      while (!_shutdown_check())
      {
        SecondStartWaiter waiter; // need for waiting next second on iteration ending
        // Send queued request
        unsigned long sended_requests =
            _send_queued_requests(request_per_sec);

        // Send 'ns-lookup' request
        for(unsigned int i = 0;
            i < request_per_sec - sended_requests;
            i++)
        {
          _send_main_request();
        }

        // Dump short statistics
        dump();

        // Check constraints
        if (!constraints_.check())
        {
          logger_->log(
            constraints_.error(),
            Logging::Logger::ERROR);
          return 0;
        }

        request_per_sec+=_get_requests_increment();

        ACE_Reactor::instance()->handle_events(); // FIXME Get rid of ACE
      }
      return 0;
    }
  catch (eh::Exception& e)
  {
    logger_->stream( Logging::Logger::CRITICAL) <<
      "Exception:" << e.what();
    return 1;
  }
  catch (...)
  {
    logger_->log(
      String::SubString("Unexpected exception"),
      Logging::Logger::CRITICAL);
    return 1;
  }
}


bool QuerySender::_shutdown_check()
{
  ReadGuard_ guard(shutdown_lock_);
  return deactivated_;
}

void QuerySender::_shutdown_set()
{
  WriteGuard_ guard(shutdown_lock_);
  deactivated_ = true;
}

void QuerySender::_init_clients()
{
  for (unsigned int i = 0; i < cfg_.client_config()->count; i++)
    {
      CookiePool_var cookie(new CookiePoolPtr(new PrintCookies()));
      HttpInterface_var client(
        CreateCookieClient(http_pool_.in(), cookie.in()));
      clients_.push_back(client);
      cookies_.push_back(cookie);
    }
}

void QuerySender::dump(bool need_full)
{
  // Using queues
  pool_queue_constraint_->set_value(http_policy_->get_request_queue_size());
  sender_queue_constraint_->set_value(msg_queue()->message_count());
  execution_time_constraint_->set_value(get_total_duration());

  logger_->stream(Logging::Logger::INFO) <<
    "Resource usage: sender queue=" << msg_queue()->message_count() <<
    ", pool queue=" << http_policy_->get_request_queue_size() <<
    ", pool threads=" << http_policy_->get_threads_count() <<
    ", connections=" << http_policy_->get_connections_count();

  // Statistics
  if (cfg_.client_config()->ns_request().get() != 0)
  {
    std::ostringstream out;
    ChannelsReport(stats_, out).dump();
    logger_->log(out.str(), Logging::Logger::INFO);
  }

  std::ostringstream stat_msg;
  ShortReport(stats_, stat_msg).dump();

  unsigned long time_afer_last_dump = time(NULL) - full_dump_time_;
  if (need_full ||
       time_afer_last_dump >= cfg_.statistics_interval_dump())
    {
      full_dump_time_ = time(NULL);
      stat_msg << std::endl;
      StandardReport(stats_, stat_msg).dump();
    }
  logger_->log(stat_msg.str(), Logging::Logger::INFO);
}

void QuerySender::dump_confluence_report()
{
  if (!cfg_.confluence_report_path().empty())
  {
    ConfluenceReport(stats_, cfg_, constraints_,
                     get_total_duration()).dump();
  }
}

Generics::Time QuerySender::get_total_duration()
{
  return Generics::Time::get_time_of_day() - start_time_;
}

void QuerySender::_send_main_request()
{
  std::string server;
  unsigned long srv_idx = Generics::safe_rand(cfg_.server_urls()->size());
  cfg_.server_urls()->get(srv_idx, server);
  ParamsRequest* request = 0;
  const RequestConfig_var& request_cfg = cfg_.client_config()->ns_request();
  unsigned long client_id = Generics::safe_rand(clients_.size());
  if (request_cfg.get() != 0)
  {
    request = new NSLookupRequest(this,
                                  client_id,
                                  _need_optout(),
                                  request_cfg,
                                  server.c_str(),
                                  cfg_.client_config()->ad_all_optouts);
  }
  _process_request(request);
}

bool QuerySender::_need_optout()
{
  unsigned long optout_rand =
      Generics::safe_rand(100);
  return optout_rand < cfg_.client_config()->optout_rate;
}

void QuerySender::_process_request(BaseRequest* request)
{
  ResponseCallback_var response_cb_(request);
  std::string url = request->url();
  HeaderList headers;
  request->headers(headers);
  unsigned long client_id = request->client_id();
  std::ostringstream ostr;
  ostr << request << std::endl;
  HttpInterface* client = http_pool_.in();
  if (!request->optout())
  {
    client = clients_[client_id].in();
    cookies_[client_id]->as<PrintCookies>()->print_cookies(ostr);
  }
  if (request->isGet())
  {
    client->add_get_request(url.c_str(), response_cb_, HTTP::HttpServer(), headers);
  }
  else
  {
    client->add_post_request(url.c_str(), response_cb_,
                             String::SubString(),
                             HTTP::HttpServer(),
                             headers);
  }
  logger_->log(ostr.str(), Logging::Logger::TRACE);
}

// comment prior and uncomment this block for making synchronous requests

// void QuerySender::_process_request(BaseRequest* request)
// {
//   unsigned long client_id = Generics::safe_rand(clients_.size());
//   std::string url = request->url();
//   int response_code;
//   HeaderList response_headers;
//   ResponseBody response_body;
//   std::string response_error;
//   ExpectedHeaders exp_headers;
//   HeaderList headers;
//   exp_headers.push_back("Debug-Info");
//   request->headers(headers);
//   Generics::Time start_time_ = Generics::Time::get_time_of_day();
//   stats_.push_request(url.c_str());
//   syncronous_get_request(response_code, response_headers, response_body,
//                          response_error, *clients_[client_id],
//                          url.c_str(), exp_headers,
//                          HttpServer(), headers);
//   Generics::Time duration = Generics::Time::get_time_of_day() - start_time_;
//   std::ostringstream ostr;
//   ostr << "Client# " << client_id << " send request: " << url << std::endl;
//   ostr << "  Client cookies: " << std::endl;
//   ClientCookieFacility::iterator cookie_it((*cookies_[client_id])->begin());
//   for (;cookie_it != (*cookies_[client_id])->end(); ++cookie_it)
//     {
//       print_cookie(ostr, *cookie_it);
//     }
//   logger_.log(ostr.str(), Logging::Logger::TRACE);
//   if (response_code < 200 || response_code >= 400)
//     {
//       stats_.push_error(url.c_str());
//     }
//   else
//     {
//       const char* debug_info = 0;
//       for (HeaderList::iterator it = response_headers.begin();
//            it != response_headers.end(); ++it)
//         {
//           if (it->name == "Debug-Info")
//             {
//               debug_info = (it->value).c_str();
//               break;
//             }
//         }
//       if (debug_info)
//         {
//           UnitCommons::DebugInfo::DebugInfo debug_info_;
//           std::string debug_info_string(debug_info);
//           debug_info_.parse(debug_info_string);
//           // TODO: it is not good, may be work with 'ccid' like a string
//           unsigned long ccid = atoi(debug_info_.ccid.value().c_str());
//           on_response(url.c_str(),
//                       response_code,
//                       duration,
//                       ccid, debug_info_.click_url.value().c_str(),
//                       debug_info_.action_pixel_url.value().c_str(),
//                       debug_info_.action_adv_url.value().c_str());
//         }
//       else
//         {
//           typedef std::vector<std::string> result_type;
//           result_type result;
//           unsigned long ccid  = 0;
//           String::RegEx re("ccid\\*eql\\*([\\d]+)");
//           if (re.search(result, url.c_str()))
//             {
//               if (result.size() == 2)
//                 {
//                   ccid = atoi(result[1].c_str());
//                 }
//             }
//           else
//             {
//               String::RegEx re("&ccid=([\\d]+)");
//               if (re.search(result, url.c_str()))
//                 {
//                   if (result.size() == 2)
//                     {
//                       ccid = atoi(result[1].c_str());
//                     }
//                 }
//             }
//           on_response(url.c_str(),
//                       response_code,
//                       duration, ccid);
//         }
//     }
//  delete request;
// }





unsigned long QuerySender::_get_requests_increment()
{
  if (!cfg_.client_config()->step_interval)
    return 0;
  return
    cfg_.client_config()->incr_count_step /
    cfg_.client_config()->step_interval;
}

void QuerySender::_log_response(const char* http_request,
                                int response_code,
                                const char* )
{
  logger_->stream(Logging::Logger::TRACE) <<
    "Receive response on request '" << http_request <<
    "' status=" << response_code;
}

void QuerySender::enqueue_request(BaseRequest* request)
{
  ACE_Message_Block* message = request; // FIXME Get rid of ACE
  if(putq(message) == -1)
    {
      int error = ACE_OS::last_error();
      message->release();

      char buf[256];
      logger_->stream(Logging::Logger::CRITICAL) <<
        "Generics::TaskRunner::enqueue_task: putq() failed. "
        "Errno " << error  << ". : " <<
        strerror_r(error, buf, sizeof(buf));
      _shutdown_set();

    }
}

void QuerySender::_schedule_request(BaseRequest* request,
                                    unsigned long request_delay)
{
  Generics::Time request_time =
    Generics::Time::get_time_of_day() + request_delay;
  Generics::Goal_var goal(new QueryScheduledTask(this, request));
  scheduler_->schedule(goal, request_time);
}

void QuerySender::_schedule_child_requests(unsigned long client_id,
                                           bool is_opted_out,
                                           const char* click_url,
                                           const char* action_adv_url,
                                           const char* passback_url)
{
  std::string server;
  unsigned long srv_idx = Generics::safe_rand(cfg_.server_urls()->size());
  cfg_.server_urls()->get(srv_idx, server);
  if (url_not_empty(click_url) &&
      need_send(click_count_++, cfg_.client_config()->click_rate))
  {
    ClickRequest* request = new ClickRequest(this,
                                             client_id,
                                             is_opted_out,
                                             cfg_.client_config()->click_request(),
                                             click_url);
    _schedule_request(request, 0);
  }
  if (url_not_empty(action_adv_url) &&
      need_send(action_count_++,  cfg_.client_config()->action_rate))
  {
    ActionRequest* request = new ActionRequest(this,
                                               client_id,
                                               is_opted_out,
                                               cfg_.client_config()->action_request(),
                                               action_adv_url);
    _schedule_request(request, 0);
  }
  if (url_not_empty(passback_url) &&
      need_send(passback_count_++,  cfg_.client_config()->passback_rate))
  {
    PassbackRequest* request = new PassbackRequest(this,
                                                   client_id,
                                                   is_opted_out,
                                                   cfg_.client_config()->passback_request(),
                                                   passback_url);
    _schedule_request(request, 0);
  }

}

unsigned long QuerySender::_send_queued_requests(unsigned long max_requests)
{
  for (unsigned int i=0; i < max_requests; i++ )
    {
      if  (!msg_queue()->message_count())
        {
          return i;
        }
      ACE_Message_Block* message = 0;
      int result = getq(message);
      if(result == -1)
        {
          int error = ACE_OS::last_error();
          char buf[256];
          std::ostringstream ostr;
          ostr << "Generics::TaskRunner::svc: internal error. : " << "getq() failed, errno "
               << error << ", " << strerror_r(error, buf, sizeof(buf));
          logger_->log(ostr.str(), Logging::Logger::CRITICAL);
          _shutdown_set();
          return 0;
        }
      if(message)
        {
          BaseRequest* request = dynamic_cast<BaseRequest*>(message);
          _process_request(request);
        }
    }
  return max_requests;
}





