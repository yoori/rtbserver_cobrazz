/**
 * @file UserCampaignReachContainerMTTest.cpp
 */
#include <vector>

#include <TestCommons/MTTester.hpp>
#include <Generics/Rand.hpp>
#include <Generics/Uuid.hpp>

#include "UserCampaignReachContainerMTTest.hpp"

namespace
{
  /// Amount of actions should be processed
  /// in some threads.
  const std::size_t MT_OPERATIONS_AMOUNT = 10;
}

using namespace AdServer::RequestInfoSvcs;
using AdServer::Commons::UserId;
using AdServer::Commons::RequestId;

/**
 * Test is modeling data race in UserCampaignReachContainer.
 * The essence of the race, we call 4 identical events on shared data.
 * without synchronization, the number of reactions will be >= 1.
 * If container works good in multi-threaded environment, number of
 * reach actions (= appear new unique events) will be equal the number
 * of unique calls.
 * A criterion for the success of the test, the number of
 * process_reach = number of unique events in queue.
 */
class UserCampaignReachContainerTester
{
public:
  UserCampaignReachContainerTester(const TestIt* test_it) noexcept
    : test_it_(test_it)
  {
    test_it_->reach_processor->clear();
    RequestId id(RequestId::create_random_based());
    UserId uid(UserId::create_random_based());
    for (std::size_t i = 0; i <= MT_OPERATIONS_AMOUNT; ++i)
    {
      for (std::size_t j = 0; j < 4; ++j)
      {
        tasks_.push_back(new Task(test_it_, uid, id, i));
      }
    }
  }

  /// Thread function, choice elemental task and do it.
  void
  operator ()() noexcept
  {
    try
    {
      Task_var task;
      {
        Sync::PosixGuard lock(task_select_mutex_);
        if (tasks_.empty())
        {
          return;
        }
        task = ReferenceCounting::add_ref(tasks_.back());
        tasks_.pop_back();
      }
      (*task)();
      sched_yield();
    }
    catch (...)
    {
    }
  }

  std::size_t
  tasks_amount() const noexcept
  {
    return tasks_.size();
  }

  bool
  check() const noexcept
  {
    if (test_it_->reach_processor->result().size() != MT_OPERATIONS_AMOUNT)
    {
      std::cerr << "FAIL, must processed " << MT_OPERATIONS_AMOUNT <<
        " impressions, but really " <<
        test_it_->reach_processor->result().size() << std::endl;
      return false;
    }
    std::cout << "Processed " << test_it_->reach_processor->result().size() <<
      " impressions." << std::endl << "SUCCESS" << std::endl;

    return true;
  }
private:
  class Task : public ReferenceCounting::AtomicImpl
  {
  public:
    Task(const TestIt* test_it,
      const UserId& uid,
      const RequestId& id,
      unsigned long i)
      noexcept
      : test_it_(test_it),
        request_info_(id)
    {
      request_info_.user_id = uid;
      request_info_.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      request_info_.adv_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      request_info_.colo_id = 1;
      request_info_.advertiser_id = i / 8;
      request_info_.campaign_id = i / 4; // 1
      request_info_.ccg_id = i / 2; // 3
      request_info_.cc_id = i; // 4
      request_info_.enabled_action_tracking = true;
      request_info_.test_request = false;
    }

    virtual
    ~Task() noexcept
    {
    }

    virtual void
    operator ()() const
    {
      test_it_->reach_container->process_impression(
        request_info_,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }
  private:
    const TestIt* test_it_;
    RequestInfo request_info_;
  };
  typedef ReferenceCounting::SmartPtr<Task> Task_var;

  typedef std::vector<Task_var> Tasks;
  Tasks tasks_;
  const TestIt* test_it_;
  Sync::PosixMutex task_select_mutex_;
};

bool
multi_thread_test(const TestIt* test_it)
  noexcept
{
  std::cout << "MT test" << std::endl;
  UserCampaignReachContainerTester test_user_campaign_reach_info(test_it);
  TestCommons::MTTester<UserCampaignReachContainerTester&> mt_tester_ucr(
    test_user_campaign_reach_info, 15);

  mt_tester_ucr.run(MT_OPERATIONS_AMOUNT * 4,
    0,
    MT_OPERATIONS_AMOUNT * 4);

  return test_user_campaign_reach_info.check();
}
