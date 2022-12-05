/**
 * @file RequestInfoContainerMTTest.cpp
 * Multi-threading test for RequestInfoContainer
 */

#include <tr1/array>
#include <TestCommons/MTTester.hpp>
#include <Generics/Rand.hpp>
#include <Generics/Uuid.hpp>

#include <RequestInfoSvcs/RequestInfoManager/RequestInfoContainer.hpp>

#include "RequestInfoContainerMTTest.hpp"

using namespace AdServer::RequestInfoSvcs;

namespace
{
  /// Amount of  reqs, imps, clicks, actions, should be processed in some threads
  const std::size_t MT_OPERATIONS_AMOUNT = 1000;
}


/**
 * Contain random set of operations with RequestInfoContaine for specified
 * request_id (random generated to). When thr functor is called, the number
 * of operations decreases.
 */
class RequestIdOperations : public virtual ReferenceCounting::AtomicImpl
{
public:
  RequestIdOperations() noexcept
    : custom_action_info_(),
      complete_counter_(0)
  {
    imp_track_ = Generics::safe_rand(1);
    action_track_ = Generics::safe_rand(1);
    std::generate(ops_.begin(), ops_.end(), rand_operation_);

    request_info_.request_id = AdServer::Commons::UserId::create_random_based();
    request_info_.enabled_action_tracking = action_track_;
    request_info_.enabled_impression_tracking = imp_track_;
    request_info_.expression = "TEST";

    request_info_.adv_revenue.impression = RevenueDecimal("0.1");
    request_info_.adv_revenue.click = RevenueDecimal("0.1");
    request_info_.adv_revenue.action = RevenueDecimal("0.1");
    request_info_.adv_revenue.currency_rate = RevenueDecimal(1);
    request_info_.pub_revenue.impression = RevenueDecimal("0.2");
    request_info_.pub_revenue.click = RevenueDecimal("0.1");
    request_info_.pub_revenue.action = RevenueDecimal("0.1");
    request_info_.pub_revenue.currency_rate = RevenueDecimal(1);
    request_info_.isp_revenue.impression = RevenueDecimal("0.1");
    request_info_.isp_revenue.click = RevenueDecimal("0.1");
    request_info_.isp_revenue.action = RevenueDecimal("0.1");
  }

  /**
   * @return True if operation was performed, False mean that functor exhaused
   * and done all operations
   */
  bool
  do_it(RequestInfoContainer* request_info_container)
  {
    std::size_t index;
    {
      Guard lock(operation_selector_);
      if (complete_counter_ == ops_.size())
      {
        return false;
      }
      index = complete_counter_++;
    }

    if (ops_[index] == -1)
    {
      request_info_container->process_request(RequestInfo(request_info_));
    }
    else if (ops_[index] == -2)
    {
      request_info_container->process_custom_action(
        request_info_.request_id,
        custom_action_info_);
    }
    else
    {
      request_info_container->process_action(
        static_cast<RequestContainerProcessor::ActionType>(ops_[index]),
        Generics::Time::get_time_of_day(),
        request_info_.request_id);
    }
    return index != ops_.size();
  }

  bool
  empty() const noexcept
  {
    Guard lock(operation_selector_);
    return complete_counter_ == ops_.size();
  }
protected:
  virtual
  ~RequestIdOperations() noexcept
  {
  }

/*  void
  print(std::ostream& ostr) const
  {
    for (int i = 0; i < sz; ++i)
    {
      if (i != 0) ostr << "->";

      if (ops[i] == -1)
      {
        ostr << "REQ";
      }
      else if (ops[i] == -2)
      {
        ostr << "CUSTOM ACTION";
      }
      else
      {
        switch(ops[i])
        {
        case RequestContainerProcessor::AT_IMPRESSION:
          ostr << "IMP";
          break;
        case RequestContainerProcessor::AT_CLICK:
          ostr << "CLICK";
          break;
        case RequestContainerProcessor::AT_ACTION:
          ostr << "ACT";
          break;
        };
      }
    }
  }*/

private:
  static int
  rand_operation_() noexcept
  {
    return Generics::safe_rand(RequestContainerProcessor::AT_ACTION + 2) - 2;
  }
  typedef Sync::PosixMutex Mutex;
  typedef Sync::PosixGuard Guard;

  bool imp_track_;
  bool action_track_;
  typedef std::tr1::array<int, 8> OperationsSet;
  OperationsSet ops_;
  AdvCustomActionInfo custom_action_info_;

  RequestInfo request_info_;

  mutable Mutex operation_selector_;
  std::size_t complete_counter_;
};

  typedef ReferenceCounting::SmartPtr<RequestIdOperations>
    RequestIdOperations_var;


class Tester
{
public:
  /**
   * Generate random sequence - queue of task: Impression or Click or Action.
   * Threads will sork out tasks from queue and do it.
   * Here, form queue of tasks.
   */
  Tester(RequestInfoContainer* request_info_container)
    : request_info_container_(
        ReferenceCounting::add_ref(request_info_container))
  {
    for (std::size_t i = 0; i < MT_OPERATIONS_AMOUNT; ++i)
    {
      tasks_.push_front(new RequestIdOperations);
    }
  }

  void
  operator ()()
  {
    RequestIdOperations_var operation;
    { // Choice key for transaction
      Guard lock(task_selection_);
      operation = tasks_.front();
      if (operation->empty())
      {
        tasks_.pop_front();
      }
    }
    operation->do_it(request_info_container_);
  }
private:
  typedef Sync::PosixMutex Mutex;
  typedef Sync::PosixGuard Guard;
  Mutex task_selection_;
  typedef std::list<RequestIdOperations_var> Tasks;
  Tasks tasks_;
  RequestInfoContainer_var request_info_container_;
};

bool
multi_thread_test(
  AdServer::RequestInfoSvcs::RequestInfoContainer* request_info_container)
  noexcept
{
  Tester test(request_info_container);
  TestCommons::MTTester<Tester&> mt_tester(
    test, 15);

  mt_tester.run(MT_OPERATIONS_AMOUNT * 8,
    0,
    MT_OPERATIONS_AMOUNT * 8);
  return true;
}
