#include "SessionSyncTest.hpp"
#include <memory>
#include <sstream>
#include <Logger/ActiveObjectCallback.hpp>

REFLECT_UNIT(SessionSyncTest) (
  "UserProfiling",
  AUTO_TEST_QUIET
);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::Logger Logger;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::BaseRequest BaseRequest;
  const char ASPECT[] = "SessionSyncTest";

  class SesSyncTestThread : public virtual Generics::TaskImpl
  {
    AdClient           client_;
    Logger&            logger_;
    int num_;
    const BaseRequest* request_;
  public:

    virtual ~SesSyncTestThread() noexcept {}

    //construct this
    SesSyncTestThread(
      const AdClient& client,
      Logger& logger,
      int num,
      const BaseRequest* request) :
      client_(client),
      logger_(logger),
      num_(num),
      request_(request)
    { }
    
    //execute this
    virtual void
    execute() noexcept
    {
      AutoTest::Logger::thlog(logger_);
      try
      {
        logger_.stream(Logging::Logger::DEBUG, ASPECT) <<
          "Thread#" << num_ << " start";
        client_.process_request(*request_);
        logger_.stream(Logging::Logger::DEBUG, ASPECT) <<
          "Thread#" << num_ << " stop";
      }
      catch(const eh::Exception& exc)
      {
        logger_.stream(Logging::Logger::ERROR, ASPECT)
          << "SesSyncTestThread::execute(). Thread#"
          << num_ << exc.what() << ". Unknown exception caught.";
      }
    }
  };

}

SessionSyncTest::SessionSyncTest(
  UnitStat& stat_var, 
  const char* task_name, 
  XsdParams params_var)
  : BaseUnit(stat_var, task_name, params_var),
    callback_(
      new Logging::ActiveObjectCallbackImpl(
        &AutoTest::Logger::thlog()))
  
{ }

bool
SessionSyncTest::run_test_element(int count,
                                  unsigned int minimum_visits,
                                  const std::string& keyword)
{
  add_descr_phrase("repeat test element: N" + strof(count));
  NSLookupRequest validating_request;
  validating_request.referer_kw = keyword;

  AdClient client(AdClient::create_user(this));
  
  //Request for real uid
  client.process_request(validating_request);
  {
    Generics::TaskRunner_var task_runner(
      new Generics::TaskRunner(callback_,
                                 minimum_visits - 2,
                                 1024 * 1024));
    //Creating of <minimum_visits - 2> threads, which will send equal requests concurrently;
    for (size_t ind = 0; ind < minimum_visits - 2; ++ind)
    {
      task_runner->
        enqueue_task(Generics::Task_var(
          new SesSyncTestThread(
            client,
            AutoTest::Logger::thlog(),
            ind, &validating_request)), 0);
    }
    task_runner->activate_object();
    task_runner->wait_for_queue_exhausting();
    task_runner->deactivate_object();
    task_runner->wait_object();
  }
  client.process_request(validating_request, "final request");
  // Fix for ADSC-3319
  // check HTTP status
  // Status != 200 and != 204, if race occur
  FAIL_CONTEXT(
    AutoTest::or_checker(
      AutoTest::equal_checker(
        204,
        client.req_status()),
      AutoTest::equal_checker(
        200,
        client.req_status())),      
    "Invalid HTTP status");
  return true;
}

bool
SessionSyncTest::run_test()
{
  std::string exp_channel_id = fetch_string("SessionSyncTest/01");
  std::string keyword = fetch_string("SessionSyncTest/KeyWord");
  unsigned int minimum_visits = fetch_int("SessionSyncTest/MinimumVisits");
  int repeat_count = (int)fetch_int("SessionSyncTest/RepeatCount");
  int count  = repeat_count;

  bool result = false;
  while(count-- && (result = run_test_element(repeat_count-count, minimum_visits, keyword)));
  return result;
}

