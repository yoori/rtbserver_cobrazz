/**
 * @file UserActionInfoContainerMTTest.hpp
 */
#ifndef USER_ACTION_INFO_CONTAINER_MT_TEST_HPP_INCLUDED
#define USER_ACTION_INFO_CONTAINER_MT_TEST_HPP_INCLUDED

#include <map>

#include <Sync/PosixLock.hpp>
#include <RequestInfoSvcs/RequestInfoManager/UserActionInfoContainer.hpp>

struct TestProcessor :
  public AdServer::RequestInfoSvcs::RequestContainerProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
  struct CustomActionKey
  {
    CustomActionKey(
      const AdServer::Commons::RequestId& request_id_val,
      unsigned long action_id_val,
      const AdServer::Commons::UserId& action_request_id_val,
      const char* referer_val)
      : request_id(request_id_val),
        action_id(action_id_val),
        action_request_id(action_request_id_val),
        referer(referer_val)
    {}

    bool
    operator <(const CustomActionKey& right) const
    {
      return
        request_id < right.request_id ||
        (request_id == right.request_id &&
        (action_id < right.action_id ||
         (action_id == right.action_id &&
          (action_request_id < right.action_request_id ||
           (action_request_id == right.action_request_id &&
           referer < right.referer)))));
    }

    AdServer::Commons::RequestId request_id;
    unsigned long action_id;
    AdServer::Commons::UserId action_request_id;
    std::string referer;
  };

  virtual void
  process_request(const AdServer::RequestInfoSvcs::RequestInfo& /*ri*/)
    noexcept
  {}

  virtual void
  process_impression(const AdServer::RequestInfoSvcs::ImpressionInfo& imp_info)
    noexcept
  {
    Guard lock(simple_actions_lock_);
    if (simple_actions.find(imp_info.request_id) == simple_actions.end())
    {
      simple_actions[imp_info.request_id] = 1;
    }
    else
    {
      ++simple_actions[imp_info.request_id];
    }
  }

  virtual void
  process_action(
    ActionType /*action_type*/,
    const Generics::Time&,
    const AdServer::Commons::RequestId& request_id)
    noexcept
  {
    Guard lock(simple_actions_lock_);
    if (simple_actions.find(request_id) == simple_actions.end())
    {
      simple_actions[request_id] = 1;
    }
    else
    {
      ++simple_actions[request_id];
    }
  }

  virtual void
  process_custom_action(
    const AdServer::Commons::RequestId& request_id,
    const AdServer::RequestInfoSvcs::AdvCustomActionInfo&
      adv_custom_action_info)
    /*throw(Exception)*/
  {
    CustomActionKey key(
      request_id,
      adv_custom_action_info.action_id,
      adv_custom_action_info.action_request_id,
      adv_custom_action_info.referer.c_str());

    Guard lock(custom_actions_lock_);
    if (custom_actions.find(key) == custom_actions.end())
    {
      custom_actions[key] = 1;
    }
    else
    {
      ++custom_actions[key];
    }
  }

  virtual void
  process_impression_post_action(
    const AdServer::Commons::RequestId&,
    const AdServer::RequestInfoSvcs::RequestPostActionInfo&)
    /*throw(Exception)*/
  {}

  void
  clear() noexcept
  {
    simple_actions.clear();
    custom_actions.clear();
  }

  typedef std::map<AdServer::Commons::RequestId, std::size_t> SimpleActionMap;
  SimpleActionMap simple_actions;

  typedef std::map<CustomActionKey, std::size_t> CustomActionMap;
  CustomActionMap custom_actions;

protected:
  virtual ~TestProcessor() noexcept
  {}

private:
  typedef Sync::PosixMutex Mutex;
  typedef Sync::PosixGuard Guard;

  // Need synch for multi-threading test
  Mutex simple_actions_lock_;
  Mutex custom_actions_lock_;
};

typedef ReferenceCounting::SmartPtr<TestProcessor> TestProcessor_var;

struct TestIt
{
  AdServer::RequestInfoSvcs::UserActionInfoContainer* act_container;
  TestProcessor* act_processor;
};

/**
 * Multithreading test
 */
bool
multi_thread_test(const TestIt* test_it) noexcept;

#endif // USER_ACTION_INFO_CONTAINER_MT_TEST_HPP_INCLUDED
