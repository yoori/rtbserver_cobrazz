// @file RequestInfoSvcs/RequestInfoManagerStormTest.cpp

#include <map>

#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>
#include <Generics/ActiveObject.hpp>
#include <Sync/Condition.hpp>
#include <TestCommons/ActiveObjectCallback.hpp>

#include <RequestInfoSvcs/RequestInfoManager/CompositeRequestActionProcessor.hpp>
#include <RequestInfoSvcs/RequestInfoManager/RequestInfoContainer.hpp>
#include <RequestInfoSvcs/RequestInfoManager/UserCampaignReachContainer.hpp>
#include <RequestInfoSvcs/RequestInfoManager/UserActionInfoContainer.hpp>
#include <RequestInfoSvcs/RequestInfoManager/PassbackContainer.hpp>

using namespace AdServer::RequestInfoSvcs;
using AdServer::Commons::UserId;
using AdServer::Commons::RequestId;

/**
 * This test don't test logic - It test only containers stability &
 * its performance
 */

Generics::AppUtils::Option<unsigned long> opt_cache_blocks(0);

namespace
{
  const char USAGE[] =
    "RequestInfoManagerStormTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -i, --iterations : number of scenario iterations.\n"
    "  -u, --users : count of users.\n"
    "  -h, --help : show this message.\n";

  const char USER_CAMPAIGN_REACH_CHUNKS_ROOT[] = "/UserCampaignReach/";
  const char USER_CAMPAIGN_REACH_CHUNKS_PREFIX[] = "UserCampaignReach";
  const char REQUEST_CHUNKS_ROOT[] = "/Request/";
  const char REQUEST_CHUNKS_PREFIX[] = "Request";
  const char USER_ACTION_INFO_CHUNKS_ROOT[] = "/UserActionInfo/";
  const char USER_ACTION_INFO_CHUNKS_PREFIX[] = "UserActionInfo";
  const char PASSBACK_CHUNKS_ROOT[] = "/Passback/";
  const char PASSBACK_CHUNKS_PREFIX[] = "Passback";

  const char DEFAULT_ROOT_PATH[] = "./";
  const unsigned long DEFAULT_USERS_COUNT = 10;
  const unsigned long DEFAULT_NUMBER_OF_SCENARIOUS = 1000;

  struct TestRequestActionProcessorImpl :
    public virtual RequestActionProcessor,
    public virtual CampaignReachProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
    TestRequestActionProcessorImpl(bool print)
      : print_(print)
    {}

    virtual void
    process_request(const RequestInfo& /*ri*/,
      const ProcessingState&) noexcept
    { if(print_) std::cout << "(R)"; }

    virtual void
    process_impression(const RequestInfo& /*ri*/,
      const ImpressionInfo&,
      const ProcessingState&) noexcept
    { if(print_) std::cout << "(I)"; }

    virtual void
    process_click(const RequestInfo& /*ri*/,
      const ProcessingState&) noexcept
    { if(print_) std::cout << "(C)"; }

    virtual void
    process_action(const RequestInfo& /*ri*/) noexcept
    { if(print_) std::cout << "(A)"; }

    virtual void
    process_custom_action(
      const RequestInfo& /*ri*/,
      const AdvCustomActionInfo& /*adv_custom_action_info*/) noexcept {}

    virtual void
    process_reach(const ReachInfo& /*reach_info*/) noexcept {}

  private:
    bool print_;
  };

  typedef
    ReferenceCounting::SmartPtr<TestRequestActionProcessorImpl>
    TestRequestActionProcessorImpl_var;

  struct NullPassbackProcessorImpl :
    public virtual PassbackProcessor,
    public virtual ReferenceCounting::AtomicImpl
  {
    NullPassbackProcessorImpl()
    {}

    virtual void
    process_passback(const PassbackInfo& /*passback_info*/)
      /*throw(Exception)*/
    {}
  };

  typedef ReferenceCounting::SmartPtr<NullPassbackProcessorImpl>
    NullPassbackProcessorImpl_var;

  template<typename FunctorType>
  class MTRunner: public Generics::ActiveObjectCommonImpl
  {
  public:
    struct ProcessingState: public ReferenceCounting::AtomicImpl
    {
      ProcessingState(unsigned long threads_count)
        : threads_in_progress(threads_count)
      {}

      Sync::Condition cond;
      unsigned long threads_in_progress;
      Generics::Time processing_time;
    };

    typedef ReferenceCounting::SmartPtr<ProcessingState>
      ProcessingState_var;

    MTRunner(
      Generics::ActiveObjectCallback* callback,
      unsigned long thread_count,
      ProcessingState* processing_state,
      unsigned long iter_count,
      FunctorType fun = FunctorType())
      /*throw(eh::Exception)*/;

  private:
    class Job: public SingleJob
    {
    public:
      Job(Generics::ActiveObjectCallback* callback,
        ProcessingState* processing_state,
        unsigned long iter_count,
        FunctorType fun)
        /*throw(eh::Exception)*/;

      virtual void
      work() noexcept;

      virtual void
      terminate() noexcept;

    protected:
      virtual ~Job() noexcept
      {}

    protected:
      ProcessingState_var processing_state_;
      const unsigned long ITER_COUNT_;
      FunctorType fun_;
    };
  };

  template<typename FunctorType>
  MTRunner<FunctorType>::MTRunner(
    Generics::ActiveObjectCallback* callback,
    unsigned long thread_count,
    ProcessingState* processing_state,
    unsigned long iter_count,
    FunctorType fun)
    /*throw(eh::Exception)*/
    : Generics::ActiveObjectCommonImpl(
        SingleJob_var(new Job(callback, processing_state, iter_count, fun)),
        thread_count)
  {}

  template<typename FunctorType>
  MTRunner<FunctorType>::Job::Job(
    Generics::ActiveObjectCallback* callback,
    ProcessingState* processing_state,
    unsigned long iter_count,
    FunctorType fun)
    /*throw(eh::Exception)*/
    : SingleJob(callback),
      processing_state_(ReferenceCounting::add_ref(processing_state)),
      ITER_COUNT_(iter_count),
      fun_(fun)
  {}

  template<typename FunctorType>
  void
  MTRunner<FunctorType>::Job::work() noexcept
  {
    Generics::Timer timer;
    timer.start();

    for(unsigned long i = 0; i < ITER_COUNT_; ++i)
    {
      fun_();
    }

    timer.stop();

    Sync::ConditionalGuard lock(processing_state_->cond);
    --processing_state_->threads_in_progress;
    processing_state_->processing_time += timer.elapsed_time();
    processing_state_->cond.signal();
  }

  template<typename FunctorType>
  void
  MTRunner<FunctorType>::Job::terminate() noexcept
  {}
}

void generate_op_seq(std::list<int>& op_seq, unsigned long op_seq_len)
{
  for (unsigned long i = 0; i < op_seq_len; ++i)
  {
    int op = ::rand() % 4 - 1;
    op_seq.push_back(op);
  }
}

void print_op_seq(
  std::ostream& out,
  const UserId& user_id,
  const std::list<int>& op_seq)
{
  out << user_id << ": ";
  for (std::list<int>::const_iterator it = op_seq.begin();
       it != op_seq.end(); ++it)
  {
    if (it != op_seq.begin()) out << " -> ";
    switch (*it)
    {
    case -1: out << "R"; break;
    case -3: out << "I"; break;
    case RequestContainerProcessor::AT_CLICK: out << "C"; break;
    case RequestContainerProcessor::AT_ACTION: out << "A"; break;
    };
  }
}

struct ProcessingTraits
{
  ProcessingTraits()
    : action_info_container(true),
      ccg_reach_container(true)
  {}

  ProcessingTraits(
    bool action_info_container_val,
    bool ccg_reach_container_val)
    : action_info_container(action_info_container_val),
      ccg_reach_container(ccg_reach_container_val)
  {}

  bool action_info_container;
  bool ccg_reach_container;
};

int
request_info_manager_storm_test(
  const char* fun,
  const char* root_path,
  unsigned long users_count,
  unsigned long iter_count,
  const std::list<int>* op_seq = 0,
  const ProcessingTraits& processing_traits = ProcessingTraits(),
  bool print_ops = false)
{
  {
    /* create full sequence of containers like RequestInfoManager */
    Logging::Logger_var logger(new Logging::Null::Logger);

    ::system(
      ( std::string("mkdir -p ") + root_path +
        USER_CAMPAIGN_REACH_CHUNKS_ROOT +
        "; mkdir -p " + root_path +
        REQUEST_CHUNKS_ROOT +
        "; mkdir -p " + root_path +
        USER_ACTION_INFO_CHUNKS_ROOT).c_str());

    TestRequestActionProcessorImpl_var test_processor(
      new TestRequestActionProcessorImpl(print_ops));

    CompositeRequestActionProcessor_var processing_distributor =
      new CompositeRequestActionProcessor();

    if(processing_traits.ccg_reach_container)
    {
      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(*opt_cache_blocks));

      UserCampaignReachContainer_var user_campaign_reach_container =
        new UserCampaignReachContainer(
          logger,
          test_processor,
          (std::string(root_path) + USER_CAMPAIGN_REACH_CHUNKS_ROOT).c_str(),
          USER_CAMPAIGN_REACH_CHUNKS_PREFIX,
          cache,
          Generics::Time(24*60*60));

      processing_distributor->add_child_processor(
        user_campaign_reach_container);
    }

    processing_distributor->add_child_processor(test_processor);

    AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
      new AdServer::ProfilingCommons::ProfileMapFactory::Cache(*opt_cache_blocks));

    RequestInfoContainer_var request_info_container =
      new RequestInfoContainer(
        logger,
        processing_distributor,
        0,
        (std::string(root_path) + REQUEST_CHUNKS_ROOT).c_str(),
        REQUEST_CHUNKS_PREFIX,
        (std::string(root_path) + REQUEST_CHUNKS_ROOT),
        String::SubString("Bid"),
        cache,
        Generics::Time(60*60*4));

    UserActionInfoContainer_var user_action_info_container;

    if(processing_traits.action_info_container)
    {
      user_action_info_container =
        new UserActionInfoContainer(
          logger,
          request_info_container,
          (std::string(root_path) + USER_ACTION_INFO_CHUNKS_ROOT).c_str(),
          USER_ACTION_INFO_CHUNKS_PREFIX,
          Generics::Time::ZERO,
          0, // cache
          Generics::Time(10),
          Generics::Time(24*60*60));

      processing_distributor->add_child_processor(
        user_action_info_container->request_processor());
    }

    /* create users pool */
    typedef std::vector<UserId> UserIdVector;
    UserIdVector users;

    for (std::size_t i = 0; i < users_count; ++i)
    {
      users.push_back(UserId(UserId::create_random_based()));
    }

    /* start loop */
    Generics::Timer timer;
    timer.start();

    for (std::size_t i = 0; i < iter_count; ++i)
    {
      int j = ::rand() % users_count;
      const UserId& user_id = users[j];

      Generics::Time stime(
        Generics::Time::get_time_of_day() - 24 * 60 * 60);
      RequestInfo request_info;
      request_info.time = stime + ::rand() % (24 * 60 * 60);
      request_info.user_id = user_id;
      request_info.request_id = RequestId(RequestId::create_random_based());
      request_info.enabled_action_tracking = (::rand() % 2 == 0);
      request_info.enabled_impression_tracking = (::rand() % 2 == 0);
      request_info.expression = "TEST";
      request_info.test_request = false;
      request_info.ccg_id = ::rand() % 1000;
      request_info.campaign_id = ::rand() % 1000;
      request_info.cc_id = ::rand() % 10;

      request_info.adv_revenue.impression = RevenueDecimal("0.1");
      request_info.adv_revenue.click = RevenueDecimal("0.1");
      request_info.adv_revenue.action = RevenueDecimal("0.1");
      request_info.adv_revenue.currency_rate = RevenueDecimal(1);
      request_info.pub_revenue.impression = RevenueDecimal("0.1");
      request_info.pub_revenue.click = RevenueDecimal("0.1");
      request_info.pub_revenue.action = RevenueDecimal("0.1");
      request_info.pub_revenue.currency_rate = RevenueDecimal(1);
      request_info.isp_revenue.impression = RevenueDecimal("0.1");
      request_info.isp_revenue.click = RevenueDecimal("0.1");
      request_info.isp_revenue.action = RevenueDecimal("0.1");
      request_info.isp_revenue.currency_rate = RevenueDecimal(1);
      request_info.advertiser_id = ::rand() % 1000;
      request_info.pub_floor_cost = RevenueDecimal::ZERO;
      request_info.pub_bid_cost = RevenueDecimal::ZERO;
      request_info.pub_commission = RevenueDecimal::ZERO;
      request_info.isp_revenue_share = RevenueDecimal::ZERO;
      request_info.ctr = RevenueDecimal::ZERO;

      /* generate sequence */
      std::list<int> gen_op_seq;
      const std::list<int>* use_op_seq = op_seq;

      if(!use_op_seq)
      {
        generate_op_seq(gen_op_seq, ::rand() % 20 + 1);
        use_op_seq = &gen_op_seq;
      }

      if(print_ops)
      {
        print_op_seq(std::cout, user_id, *use_op_seq);
        std::cout << ": ";
      }

      for (std::list<int>::const_iterator op_it = use_op_seq->begin();
        op_it != use_op_seq->end(); ++op_it)
      {
        if (*op_it == -1)
        {
          request_info_container->process_request(RequestInfo(request_info));
        }
        else
        {
          request_info_container->process_action(
            static_cast<RequestContainerProcessor::ActionType>(*op_it),
            Generics::Time::get_time_of_day(),
            request_info.request_id);
        }
      }

      if(print_ops)
      {
        std::cout << std::endl;
      }
    }

    timer.stop();

    std::cout << fun << ": " << iter_count <<
      " iterations, avg time = " <<
      (timer.elapsed_time() / iter_count) << std::endl;
  }

  return 0;
}

struct TagRequestFun
{
  TagRequestFun(
    PassbackContainer* passback_container,
    const Generics::Time& base_time)
    : passback_container_(ReferenceCounting::add_ref(passback_container)),
      base_time_(base_time)
  {}

  void
  operator()() const
  {
    TagRequestInfo ri;
    ri.time = base_time_ + ::rand() % (2 * 60 * 60);
    ri.colo_id = 0;
    ri.tag_id = 1;
    ri.user_status = 'I';
    ri.referer = "";
    ri.isp_time = ri.time;
    ri.country = "ru";
    ri.site_id = 1;
    ri.user_id = UserId::create_random_based();
    ri.page_load_id = ::rand() % 1000000;
    ri.ad_shown = false;
    ri.request_id = RequestId::create_random_based();

    passback_container_->process_tag_request(ri);
  }

private:
  PassbackContainer_var passback_container_;
  const Generics::Time base_time_;
};

int
request_info_manager_passback_storm_test(
  const char* fun,
  const char* root_path,
  unsigned long /*users_count*/,
  unsigned long iter_count,
  unsigned long threads_count,
  bool remove_old_data)
{
  {
    /* create full sequence of containers like RequestInfoManager */
    Logging::Logger_var logger(new Logging::Null::Logger);

    if(remove_old_data)
    {
      ::system((
        std::string("rm -r ") + root_path + PASSBACK_CHUNKS_ROOT +
        "; mkdir -p " + root_path + PASSBACK_CHUNKS_ROOT).c_str());
    }
    else
    {
      ::system((
        std::string("mkdir -p ") + root_path + PASSBACK_CHUNKS_ROOT).c_str());
    }

    NullPassbackProcessorImpl_var test_processor(
      new NullPassbackProcessorImpl());

    AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache;
    if(*opt_cache_blocks > 0)
    {
      cache = new AdServer::ProfilingCommons::ProfileMapFactory::Cache(*opt_cache_blocks);
    }

    PassbackContainer_var passback_container =
      new PassbackContainer(
        logger,
        test_processor,
        (std::string(root_path) + PASSBACK_CHUNKS_ROOT).c_str(),
        PASSBACK_CHUNKS_PREFIX,
        cache,
        Generics::Time::ONE_HOUR * 2 * 10);

    /* start loop */
    Generics::Time base_time(
      Generics::Time::get_time_of_day() - Generics::Time::ONE_DAY);

    MTRunner<TagRequestFun>::ProcessingState_var processing_state =
      new MTRunner<TagRequestFun>::ProcessingState(threads_count);

    Generics::ActiveObjectCallback_var active_object_callback(
      new TestCommons::ActiveObjectCallbackStreamImpl(std::cerr, fun));

    Generics::ActiveObject_var mt_runner(new MTRunner<TagRequestFun>(
      active_object_callback,
      threads_count,
      processing_state,
      iter_count,
      TagRequestFun(passback_container, base_time)));

    mt_runner->activate_object();

    while(true)
    {
      Sync::ConditionalGuard lock(processing_state->cond);
      lock.wait();
      if(processing_state->threads_in_progress == 0)
      {
        break;
      }
    }

    mt_runner->deactivate_object();
    mt_runner->wait_object();

    std::cout << fun << ": " << iter_count <<
      " iterations"
      ", sum time = " << processing_state->processing_time <<
      ", time per thread = " << (processing_state->processing_time / threads_count) <<
      std::endl;
  }

  return 0;
}

int
main(int argc, char* argv[]) noexcept
{
  int result = 0;

  try
  {
    using namespace Generics::AppUtils;
    Args args;
    CheckOption opt_help;
    Generics::AppUtils::StringOption root_path(DEFAULT_ROOT_PATH);
    Generics::AppUtils::Option<unsigned long> users_count(DEFAULT_USERS_COUNT);
    Generics::AppUtils::Option<unsigned long> iter_count(DEFAULT_NUMBER_OF_SCENARIOUS);
    Generics::AppUtils::Option<unsigned long> run_count(1);
    Generics::AppUtils::Option<unsigned long> opt_threads_count(1);

    args.add(equal_name("path") || short_name("p"), root_path);
    args.add(equal_name("iterations") || short_name("i"), iter_count);
    args.add(equal_name("runs") || short_name("r"), run_count);
    args.add(equal_name("users") || short_name("u"), users_count);
    args.add(equal_name("help") || short_name("h"), opt_help);
    args.add(equal_name("cache") || short_name("c"), opt_cache_blocks);
    args.add(equal_name("threads") || short_name("t"), opt_threads_count);

    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    {
      std::list<int> one_request_op_seq;
      one_request_op_seq.push_back(-1);

      result += request_info_manager_storm_test(
        "only one request test (RequestInfoContainer)",
        (*root_path + "/Storm/1").c_str(),
        *users_count,
        *iter_count,
        &one_request_op_seq,
        ProcessingTraits(false, false));

      result += request_info_manager_storm_test(
        "only one request test (RequestInfoContainer, ActionInfoContainer)",
        (*root_path + "/Storm/2").c_str(),
        *users_count,
        *iter_count,
        &one_request_op_seq,
        ProcessingTraits(true, false));

      result += request_info_manager_storm_test(
        "only one request test",
        (*root_path + "/Storm/3").c_str(),
        *users_count,
        *iter_count,
        &one_request_op_seq);
    }

    result += request_info_manager_storm_test(
      "random ops sequence (avg size = 10)",
      (*root_path + "/Storm/4").c_str(),
      *users_count,
      *iter_count);

    for(unsigned long run_i = 0; run_i < *run_count; ++run_i)
    {
      result += request_info_manager_passback_storm_test(
        "passback request test",
        (*root_path + "/Storm/5").c_str(),
        *users_count,
        *iter_count,
        *opt_threads_count,
        run_i != 0);
    }
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
  catch (...)
  {
    std::cerr << "unknown exception" << std::endl;
    return 1;
  }

  return result;
}
