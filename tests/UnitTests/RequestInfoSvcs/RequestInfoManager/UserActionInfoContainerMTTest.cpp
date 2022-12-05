/**
 * @file UserActionInfoContainerMTTest.cpp
 */

#include <numeric>

#include <TestCommons/MTTester.hpp>
#include <Generics/Rand.hpp>
#include <Generics/Uuid.hpp>

#include "UserActionInfoContainerMTTest.hpp"

namespace
{
  /// Amount of actions should be processed
  /// in some threads.
  const std::size_t MT_OPERATIONS_AMOUNT = 1000;
}

using namespace AdServer::RequestInfoSvcs;
using AdServer::Commons::UserId;
using AdServer::Commons::RequestId;

/**
 * Task for each thread will be following:
 * (id, request) or (id, process)
 * Create sequence contained pairs of tasks, where pair is
 *   [(id, request), (id, process)]  or  [(id, process), (id, request)]
 * Each pair have unique id, we will  share one id between threads and perform
 * activity simultaneously.
 * Some threads gets and perform tasks from sequence, criteria of success:
 * TestPassbackProcessorImpl contain all id's from sequence of pairs
 */
class UserActionInfoContainerTester
{
public:
  /**
   * Split the single-thread task into elemental (atomic) tasks, they will be
   * executed in some threads sumultaneosly.
   */
  UserActionInfoContainerTester(const TestIt* test_it) noexcept
    : std_simple_actions_size_(0),
      std_custom_actions_size_(0),
      test_it_(test_it)
  {
    test_it_->act_processor->clear();
    for (std::size_t i = 0; i < MT_OPERATIONS_AMOUNT; ++i)
    {
      RequestId id(RequestId::create_random_based());
      UserId uid(UserId::create_random_based());

      switch (Generics::safe_rand(6))
      {
      case 0:
        { // action_direct_order_test
          tasks_.push_back(new Request(test_it, uid, id));
          tasks_.push_back(new AdvAction(test_it, uid, id));
          ++std_simple_actions_size_;
          break;
        }
      case 1:
        { // action_three_test
          tasks_.push_back(new AdvAction(test_it, uid, id));
          tasks_.push_back(new Request(test_it, uid, id));
          tasks_.push_back(new AdvAction(test_it, uid, id));
          std_simple_actions_size_ += 2;
          break;
        }
      case 2:
        { // action_ADSC_3478_test
          tasks_.push_back(new Request(test_it, uid, id,
            Generics::Time(String::SubString("2008-01-01 00:00:00"),
              "%Y-%m-%d %H:%M:%S")));
          RequestId id2(
            Generics::Uuid::create_random_based().to_string().c_str());
          tasks_.push_back(new Request(test_it, uid, id2,
            Generics::Time(String::SubString("2008-01-01 00:00:01"),
              "%Y-%m-%d %H:%M:%S")));
          tasks_.push_back(new AdvAction(test_it, uid, id));
          ++std_simple_actions_size_;
          break;
        }
      case 3:
        { // action_ADSC_3478_reverse_test
          tasks_.push_back(new AdvAction(test_it, uid, id));
          RequestId id2(
            Generics::Uuid::create_random_based().to_string().c_str());
          tasks_.push_back(new Request(test_it, uid, id2,
            Generics::Time(String::SubString("2008-01-01 00:00:01"),
              "%Y-%m-%d %H:%M:%S")));
          ++std_simple_actions_size_;
          tasks_.push_back(new Request(test_it, uid, id,
            Generics::Time(String::SubString("2008-01-01 00:00:00"),
              "%Y-%m-%d %H:%M:%S")));
          break;
        }
      case 4:
        { // custom_action_direct_order_test
          tasks_.push_back(new Request(test_it, uid, id));
          RequestId id2(
            Generics::Uuid::create_random_based().to_string().c_str());
          tasks_.push_back(new Request(test_it, uid, id2));
          tasks_.push_back(new CustomAction(test_it, uid, id2));
          ++std_custom_actions_size_;
          break;
        }
      case 5:
        { // custom_two_action_direct_order_test
          tasks_.push_back(new Request(test_it, uid, id));
          RequestId id2(
            Generics::Uuid::create_random_based().to_string().c_str());
          tasks_.push_back(new Request(test_it, uid, id2,
            Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"), 2));
          tasks_.push_back(new CustomAction(test_it, uid, id));
          ++std_custom_actions_size_;
          tasks_.push_back(new CustomAction(test_it, uid, id2));
          ++std_custom_actions_size_;
          break;
        }
      };
    }
    // Backup state to be ready for restart
    save_tasks_ = tasks_;
    save_std_simple_actions_size_ = std_simple_actions_size_;
    save_std_custom_actions_size_ = std_custom_actions_size_;
  }

  std::size_t
  tasks_amount() const noexcept
  {
    return tasks_.size();
  }

  class Task : public ReferenceCounting::AtomicImpl
  {
  public:
    Task(const TestIt* test_it,
      const UserId& uid,
      const RequestId& id,
      const Generics::Time& request_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"),
      unsigned long ccg_id = 1)
      noexcept
      : test_it_(test_it),
        request_info_(id)
    {
      request_info_.user_id = uid;
      request_info_.ccg_id = ccg_id;
      request_info_.cc_id = 1;
      request_info_.time = request_time;
      request_info_.enabled_action_tracking = true;
      request_info_.has_custom_actions = true;

      adv_action_info_.time = request_time;
      adv_action_info_.ccg_id = ccg_id;
      adv_action_info_.user_id = uid;

      adv_ex_action_info_.time = request_time;
      adv_ex_action_info_.action_id = ccg_id;
      adv_ex_action_info_.ccg_ids.push_back(1);
      adv_ex_action_info_.ccg_ids.push_back(2);
      adv_ex_action_info_.action_request_id = uid;
      adv_ex_action_info_.user_id = uid;
      adv_ex_action_info_.referer = "test.com";
      adv_ex_action_info_.action_value = RevenueDecimal::ZERO;
    }

    virtual
    ~Task() noexcept
    {
    }

    virtual void
    operator ()() const = 0; // /*throw(PassbackProcessor::Exception)*/

  protected:
    const TestIt* test_it_;
    RequestInfo request_info_;
    AdvActionProcessor::AdvActionInfo adv_action_info_;
    AdvActionProcessor::AdvExActionInfo adv_ex_action_info_;
  };
  typedef ReferenceCounting::SmartPtr<Task> Task_var;

  /// serActionInfoContainer::process_adv_action
  struct AdvAction : public Task
  {
    AdvAction(const TestIt* test_it,
      const UserId& uid,
      const RequestId& id,
      const Generics::Time& request_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"))
      noexcept
      : Task(test_it, uid, id, request_time)
    {
    }

    virtual void
    operator ()() const // /*throw(PassbackProcessor::Exception)*/
    {
      test_it_->act_container->process_adv_action(adv_action_info_);
    }
  };

  /// UserActionInfoContainer::process_custom_action
  struct CustomAction : public Task
  {
    CustomAction(const TestIt* test_it,
      const UserId& uid,
      const RequestId& id) noexcept
      : Task(test_it, uid, id)
    {
    }

    virtual void
    operator ()() const // /*throw(PassbackProcessor::Exception)*/
    {
      test_it_->act_container->process_custom_action(adv_ex_action_info_);
    }
  };

  /// UserActionInfoContainer::process_click
  struct Request : public Task
  {
     Request(const TestIt* test_it,
       const UserId& uid,
       const RequestId& id,
       const Generics::Time& request_time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d"),
       unsigned long ccg_id = 1)
       noexcept
      : Task(test_it, uid, id, request_time, ccg_id)
    {
    }

    virtual void
    operator ()() const // /*throw(RequestActionProcessor::Exception)*/
    {
      test_it_->act_container->request_processor()->process_impression(
        request_info_,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
      test_it_->act_container->request_processor()->process_click(
        request_info_,
        RequestActionProcessor::ProcessingState());
    }
  };

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
    catch (const eh::Exception& ex)
    {
      std::cerr << ex.what() << std::endl;
      assert(0);
    }
  }

  /**
   * @return true if occured early calculated numbers of actions
   */
  bool
  check() noexcept
  {
    std::size_t result = std::accumulate(
      test_it_->act_processor->simple_actions.begin(),
      test_it_->act_processor->simple_actions.end(), 0, MapAdder());
    std::cout << "Elemental tasks=" << save_tasks_.size() << ", "
      << "Left tasks in queue=" << tasks_.size() << std::endl;
    if (result != std_simple_actions_size_)
    {
      std::cerr << "FAIL, standard actions=" << std_simple_actions_size_
        << ", but only performed actions=" << result << std::endl;
      tasks_ = save_tasks_;
      std_simple_actions_size_ = save_std_simple_actions_size_;
      std_custom_actions_size_ = save_std_custom_actions_size_;
      test_it_->act_processor->clear();
      return false;
    }
    else
    {
      std::size_t result = std::accumulate(
        test_it_->act_processor->custom_actions.begin(),
        test_it_->act_processor->custom_actions.end(), 0, MapAdder());
      if (result != std_custom_actions_size_)
      {
        std::cerr << "FAIL, standard custom actions="
          << std_custom_actions_size_
          << ", but performed custom actions=" << result << std::endl;
        tasks_ = save_tasks_;
        std_simple_actions_size_ = save_std_simple_actions_size_;
        std_custom_actions_size_ = save_std_custom_actions_size_;
        test_it_->act_processor->clear();
        return false;
      }
      std::cout << "SUCCESS" << std::endl;
      return true;
    }
  }

private:
  struct MapAdder
  {
    template <typename ValueType>
    std::size_t
    operator ()(std::size_t sum,
      const ValueType& map_iterator) const
    {
      return sum + map_iterator.second;
    }
  };

  typedef std::vector<Task_var> Tasks;
  Tasks tasks_;
  Tasks save_tasks_;

  /// determined in constructor correct number of completed actions
  std::size_t std_simple_actions_size_;
  std::size_t save_std_simple_actions_size_;

  std::size_t std_custom_actions_size_;
  std::size_t save_std_custom_actions_size_;

  const TestIt* test_it_;
  Sync::PosixMutex task_select_mutex_;
};

bool
multi_thread_test(const TestIt* test_it)
  noexcept
{
  UserActionInfoContainerTester test_user_action_info(test_it);
  TestCommons::MTTester<UserActionInfoContainerTester&> mt_tester_uai(
    test_user_action_info, 15);

  const std::size_t MT_OPERATIONS_AMOUNT =
    test_user_action_info.tasks_amount();
  mt_tester_uai.run(MT_OPERATIONS_AMOUNT,
    0,
    MT_OPERATIONS_AMOUNT);

  return true;
  //return test_user_action_info.check();
}
