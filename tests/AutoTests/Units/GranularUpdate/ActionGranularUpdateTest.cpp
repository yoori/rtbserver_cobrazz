#include "ActionGranularUpdateTest.hpp"

REFLECT_UNIT(ActionGranularUpdateTest) (
  "GranularUpdate",
  AUTO_TEST_SLOW
);

namespace
{
  typedef AutoTest::ActionChecker ActionChecker;
  typedef AutoTest::ActionProfileChecker ActionProfileChecker;
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::ActionRequest ActionRequest;
  typedef AutoTest::NSLookupRequest NSLookupRequest;
}

void
ActionGranularUpdateTest::set_up()
{
  add_descr_phrase("Setup");
}

bool 
ActionGranularUpdateTest::run()
{
  AUTOTEST_CASE(
    add_action(),
    "Add action");
  
  AUTOTEST_CASE(
    unlink_action(),
    "Unlink action");
  
  AUTOTEST_CASE(
    action_for_inactive_ccg(),
    "Inactive CCG");
  
  return true;
}

void
ActionGranularUpdateTest::add_action()
{
  unsigned long ccg = fetch_int("ADDACTION/CCG");

  ORM::ORMRestorer<ORM::PQ::Action>* action =
    create<ORM::PQ::Action>();

  action->account_id = fetch_int("ADDACTION/ACCOUNT");
  action->name = fetch_string("ADDACTION/ACTIONNAME");
  action->url = fetch_string("ADDACTION/URL");
  action->status = "A";
  action->display_status_id = 1;
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      action->insert()),
    "Create new action");

  ORM::ORMRestorer<ORM::PQ::Ccgaction>* ccg_action =
    create<ORM::PQ::Ccgaction>();
  
  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg_action->insert(
        action->action_id(), ccg)),
    "Link action to CCG");

  ADD_WAIT_CHECKER(
    "Check action",
    ActionChecker(
      this,
      action->action_id(),
      ActionChecker::Expected().
        ccg_ids(strof(ccg))));
}

void
ActionGranularUpdateTest::unlink_action()
{
  unsigned long action1 = fetch_int("UNLINKACTION/ACTION1");
  unsigned long action2 = fetch_int("UNLINKACTION/ACTION2");
  unsigned long ccg1 = fetch_int("UNLINKACTION/CCG1");
  unsigned long ccg2 = fetch_int("UNLINKACTION/CCG2");

  FAIL_CONTEXT(
   AutoTest::wait_checker(
     ActionChecker(
       this,
       action1,
       ActionChecker::Expected().
         ccg_ids(strof(ccg1) + ", " + strof(ccg2)))).check(),
   "Initial check action#1");

  FAIL_CONTEXT(
   AutoTest::wait_checker(
     ActionChecker(
       this,
       action2,
       ActionChecker::Expected().
         ccg_ids(strof(ccg1) + ", " + strof(ccg2)))).check(),
   "Initial check action#2");

  ORM::ORMRestorer<ORM::PQ::Ccgaction>* ccg_action =
    create(
      ORM::PQ::Ccgaction(
        pq_conn_, action1, ccg1));

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      ccg_action->delet()),
    "Unlink action#1 from CCG#1");

  ADD_WAIT_CHECKER(
    "Check action#1",
    ActionChecker(
      this,
      action1,
      ActionChecker::Expected().
        ccg_ids(strof(ccg2))));

  ADD_WAIT_CHECKER(
    "Check action#2",
    ActionChecker(
      this,
      action2,
      ActionChecker::Expected().
        ccg_ids(strof(ccg1) + ", " + strof(ccg2))));
}

void
ActionGranularUpdateTest::action_for_inactive_ccg()
{
 
  std::string action  = fetch_string("INACTIVECCG/ACTION");
  std::string ccg1  = fetch_string("INACTIVECCG/CCG1");
  std::string ccg2  = fetch_string("INACTIVECCG/CCG2");
  
  AdClient client(AdClient::create_user(this));

  client.process_request(NSLookupRequest());

  std::string uid =
    "\\" + client.debug_info.uid.value();

  client.process_request(ActionRequest().actionid(action));

  
  {
    std::string expected = "\\[ action_id = " + action + ",.*, ccg_ids = " +
      ccg2 + " \\]";
  
    ADD_WAIT_CHECKER(
      "Active CCG present",
      ActionProfileChecker(
        this,
        uid,
        AutoTest::RequestInfoManager,
        ActionProfileChecker::Expected().
          custom_action_markers(expected)));
  }

  {
    std::string expected = "\\[ .*, ccg_ids = " + ccg1 + " \\]";
    
    ADD_WAIT_CHECKER(
      "Inactive CCG absent",
      ActionProfileChecker(
        this,
        uid,
        AutoTest::RequestInfoManager,
        ActionProfileChecker::Expected().
          custom_action_markers(expected),
        AutoTest::AEC_NOT_EXISTS));
  }
}

void
ActionGranularUpdateTest::tear_down()
{
  add_descr_phrase("Tear down");
}
