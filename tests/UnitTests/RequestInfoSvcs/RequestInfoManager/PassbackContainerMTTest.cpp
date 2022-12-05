/**
 * @file PassbackContainerMTTest.cpp
 */
#include <TestCommons/MTTester.hpp>
#include <Generics/Rand.hpp>
#include "PassbackContainerMTTest.hpp"

namespace
{
  /// Amount of  reqs, imps, clicks, actions, should be processed
  /// in some threads. Must be even number
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
class MTPassbackProcessTester
{
public:
  /**
   * Generate a sequence of pairs of tasks
   */
  MTPassbackProcessTester(const TestIt* test_it) noexcept
    : test_it_(test_it)
  {
    test_it_->passback_processor->clear();
    for (std::size_t i = 1; i <= MT_OPERATIONS_AMOUNT / 2; ++i)
    {
      RequestId id(RequestId::create_random_based().to_string().c_str());

      if (Generics::safe_rand(2))
      {
        tasks_.push_back(new Request(id, i, test_it));
        tasks_.push_back(new Process(id, i, test_it));
      }
      else
      {
        tasks_.push_back(new Process(id, i, test_it));
        tasks_.push_back(new Request(id, i, test_it));
      }

      ids_.insert(i);
    }
    std::cout << "Tasks=" << tasks_.size() << std::endl;
  }

  class Task: public ReferenceCounting::DefaultImpl<>
  {
  public:
    Task(const RequestId& id, unsigned long tag_id, const TestIt* test_it) noexcept
      : test_it_(test_it)
    {
      tag_request_info_.request_id = id;
      tag_request_info_.user_status = 'I';
      tag_request_info_.time = Generics::Time(10);
      tag_request_info_.tag_id = tag_id;
    }

    virtual ~Task() noexcept
    {}

    virtual void
    operator ()() const = 0; // /*throw(PassbackProcessor::Exception)*/

  protected:
    const TestIt* test_it_;
    TagRequestInfo tag_request_info_;
  };

  typedef ReferenceCounting::SmartPtr<Task> Task_var;

  struct Request: public Task
  {
    Request(const RequestId& id, unsigned long tag_id, const TestIt* test_it) noexcept
      : Task(id, tag_id, test_it)
    {}

    virtual void
    operator ()() const // /*throw(PassbackProcessor::Exception)*/
    {
      test_it_->passback_container->process_passback_request(
        tag_request_info_.request_id,
        tag_request_info_.time);
    }
  };

  struct Process: public Task
  {
    Process(const RequestId& id, unsigned long tag_id, const TestIt* test_it) noexcept
      : Task(id, tag_id, test_it)
    {}

    virtual void
    operator ()() const // /*throw(PassbackProcessor::Exception)*/
    {
      test_it_->passback_container->process_tag_request(
        tag_request_info_);
    }
  };

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

  bool
  check() noexcept
  {
    const TestPassbackProcessorImpl& passback_processor =
      *(test_it_->passback_processor);
    const TestPassbackProcessorImpl::PassbackInfoList& res_list =
      passback_processor.result();

    Ids results;

    for (TestPassbackProcessorImpl::PassbackInfoList::
           const_iterator cit = res_list.begin();
         cit != res_list.end(); ++cit)
    {
      results.insert(cit->tag_id);
    }

    if (results != ids_)
    {
      std::cerr << "FAIL, standard size=" << ids_.size()
        << ", result size= " << results.size() << std::endl;
      return false;
    }
    else
    {
      std::cout << "SUCCESS" << std::endl;
      return true;
    }
  }

private:
  typedef std::vector<Task_var> Tasks;
  Tasks tasks_;
  typedef std::set<unsigned long> Ids;
  Ids ids_;

  const TestIt* test_it_;
  Sync::PosixMutex task_select_mutex_;
};

bool
multi_thread_test(const TestIt* test_it)
  noexcept
{
  MTPassbackProcessTester test_pb(test_it);
  TestCommons::MTTester<MTPassbackProcessTester&> mt_tester_pb(
    test_pb, 15);

  mt_tester_pb.run(MT_OPERATIONS_AMOUNT,
    0,
    MT_OPERATIONS_AMOUNT);

  if (test_pb.check())
  {
    std::cout << "Process passback mt success" << std::endl;
  }
  else
  {
    std::cerr << "FAIL" << std::endl;
  }

  return true;
}
