/**
 * @file RequestInfoSvcs/UserActionInfoContainerTest.cpp
 */
#include <Generics/AppUtils.hpp>
#include <Generics/DirSelector.hpp>
#include <Generics/Uuid.hpp>
#include <Logger/StreamLogger.hpp>

#include "UserActionInfoContainerMTTest.hpp"

using namespace AdServer::RequestInfoSvcs;
using AdServer::Commons::UserId;
using AdServer::Commons::RequestId;

namespace
{
  const char DEFAULT_ROOT_PATH[] = "./";
  Generics::AppUtils::Option<std::string> root_path(DEFAULT_ROOT_PATH);

  const char TEST_FOLDER[] = "/UserActionInfoContainerTestDir/Actions/";

  const char USAGE[] =
    "UserActionInfoContainerTest [OPTIONS]\n"
    "OPTIONS:\n"
    "  -p, --path : path to folder with temporary files.\n"
    "  -h, --help : show this message.\n";


  UserId generate_user_id()
  {
    return UserId::create_random_based();
  }

  RequestId
  generate_request_id()
  {
    return RequestId::create_random_based();
  }
}

bool
no_action_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  act_processor->clear();
  
  RequestId request_id(generate_request_id());
  UserId user_id(generate_user_id());

  RequestInfo request_info;
  request_info.time =
    Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
  request_info.ccg_id = 1;
  request_info.cc_id = 1;
  request_info.user_id = user_id;
  request_info.request_id = request_id;
  request_info.enabled_action_tracking = true;
  request_info.has_custom_actions = true;

  act_container->request_processor()->process_click(
    request_info,
    RequestActionProcessor::ProcessingState());

  if (act_processor->simple_actions.find(request_id) !=
     act_processor->simple_actions.end())
  {
    std::cerr << "no_action_test - error." << std::endl;
    return false;
  }

  std::cout << "no_action_test - success." << std::endl;
  return true;
}

bool action_direct_order_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  act_processor->clear();

  RequestId request_id(generate_request_id());
  UserId user_id(generate_user_id());

  RequestInfo request_info;
  request_info.time =
    Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
  request_info.ccg_id = 1;
  request_info.cc_id = 1;
  request_info.user_id = user_id;
  request_info.request_id = request_id;
  request_info.enabled_action_tracking = true;
  request_info.has_custom_actions = true;

  act_container->request_processor()->process_click(
    request_info,
    RequestActionProcessor::ProcessingState());

  AdvActionProcessor::AdvActionInfo act_info;
  act_info.time =
    Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
  act_info.ccg_id = 1;
  act_info.user_id = user_id;

  act_container->process_adv_action(act_info);

  if (act_processor->simple_actions.find(request_id) ==
     act_processor->simple_actions.end())
  {
    std::cerr << "action_direct_order_test - error." << std::endl;
    return false;
  }

  if (act_processor->simple_actions[request_id] != 1)
  {
    std::cerr
      << "action_direct_order_test - error: result actions count("
      << act_processor->simple_actions[request_id] << ") != 1."
      << std::endl;
    return false;
  }

  std::cout << "action_direct_order_test - success." << std::endl;
  return true;
}

bool action_three_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  act_processor->clear();

  RequestId request_id(generate_request_id());
  UserId user_id(generate_user_id());

  AdvActionProcessor::AdvActionInfo act_info;
  act_info.time =
    Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
  act_info.ccg_id = 1;
  act_info.user_id = user_id;

  act_container->process_adv_action(act_info);

  RequestInfo request_info;
  request_info.time =
    Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
  request_info.ccg_id = 1;
  request_info.cc_id = 1;
  request_info.user_id = user_id;
  request_info.request_id = request_id;
  request_info.enabled_action_tracking = true;
  request_info.has_custom_actions = true;

  act_container->request_processor()->process_click(
    request_info,
    RequestActionProcessor::ProcessingState());

  act_container->process_adv_action(act_info);

  if (act_processor->simple_actions.find(request_id) ==
     act_processor->simple_actions.end())
  {
    std::cerr << "action_three_test - error." << std::endl;
    return false;
  }

  if (act_processor->simple_actions[request_id] != 2)
  {
    std::cerr
      << "action_three_test - error: result actions count("
      << act_processor->simple_actions[request_id] << ") != 2."
      << std::endl;
    return false;
  }

  std::cout << "action_three_test - success." << std::endl;
  return true;
}

bool action_before_imp_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  act_processor->clear();

  RequestId request_id(generate_request_id());
  UserId user_id(generate_user_id());

  {
    AdvActionProcessor::AdvActionInfo act_info;
    act_info.time = Generics::Time(String::SubString("2008-01-01 00:00:01"),
      "%Y-%m-%d %H:%M:%S");
    act_info.ccg_id = 1;
    act_info.user_id = user_id;

    act_container->process_adv_action(act_info);
  }

  RequestInfo request_info;
  request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:10"),
    "%Y-%m-%d %H:%M:%S");
  request_info.ccg_id = 1;
  request_info.cc_id = 1;
  request_info.user_id = user_id;
  request_info.request_id = request_id;
  request_info.enabled_action_tracking = true;
  request_info.has_custom_actions = true;

  act_container->request_processor()->process_click(
    request_info,
    RequestActionProcessor::ProcessingState());

  if (act_processor->simple_actions.find(request_id) !=
     act_processor->simple_actions.end())
  {
    std::cerr << "action_before_imp_test - error: found excess action."
      << std::endl;
    return false;
  }

  {
    AdvActionProcessor::AdvActionInfo act_info;
    act_info.time = Generics::Time(String::SubString("2008-01-01 00:00:03"),
      "%Y-%m-%d %H:%M:%S");
    act_info.ccg_id = 1;
    act_info.user_id = user_id;

    act_container->process_adv_action(act_info);
  }

  if (act_processor->simple_actions.find(request_id) ==
     act_processor->simple_actions.end())
  {
    std::cerr << "action_before_imp_test - error: can't find action."
      << std::endl;
    return false;
  }

  if (act_processor->simple_actions[request_id] != 1)
  {
    std::cerr
      << "action_before_imp_test - error: result actions count("
      << act_processor->simple_actions[request_id] << ") != 1."
      << std::endl;
    return false;
  }

  std::cout << "action_before_imp_test - success." << std::endl;
  return true;
}

bool action_before_and_after_imp_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  act_processor->clear();

  RequestId request_id(generate_request_id());
  UserId user_id(generate_user_id());

  {
    AdvActionProcessor::AdvActionInfo act_info;
    act_info.time = Generics::Time(String::SubString("2008-01-01 00:00:00"),
      "%Y-%m-%d %H:%M:%S");
    act_info.ccg_id = 1;
    act_info.user_id = user_id;

    act_container->process_adv_action(act_info);
  }

  {
    AdvActionProcessor::AdvActionInfo act_info;
    act_info.time = Generics::Time(String::SubString("2008-01-01 00:00:06"),
      "%Y-%m-%d %H:%M:%S");
    act_info.ccg_id = 1;
    act_info.user_id = user_id;

    act_container->process_adv_action(act_info);
  }

  RequestInfo request_info;
  request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:04"),
    "%Y-%m-%d %H:%M:%S");
  request_info.ccg_id = 1;
  request_info.cc_id = 1;
  request_info.user_id = user_id;
  request_info.request_id = request_id;
  request_info.enabled_action_tracking = true;
  request_info.has_custom_actions = true;

  act_container->request_processor()->process_click(
    request_info,
    RequestActionProcessor::ProcessingState());

  if (act_processor->simple_actions.find(request_id) ==
     act_processor->simple_actions.end())
  {
    std::cerr << "action_before_and_after_imp_test - error: can't find action."
      << std::endl;
    return false;
  }

  if (act_processor->simple_actions[request_id] != 1)
  {
    std::cerr
      << "action_before_and_after_imp_test - error: result actions count("
      << act_processor->simple_actions[request_id] << ") != 1."
      << std::endl;
    return false;
  }

  std::cout << "action_before_and_after_imp_test - success." << std::endl;
  return true;
}

bool two_action_before_click_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  static const char* FUN = "two_action_before_click_test";
  
  act_processor->clear();

  RequestId request_id(generate_request_id());
  UserId user_id(generate_user_id());

  {
    AdvActionProcessor::AdvActionInfo act_info;
    act_info.time = Generics::Time(String::SubString("2008-01-01 00:00:01"),
      "%Y-%m-%d %H:%M:%S");
    act_info.ccg_id = 1;
    act_info.user_id = user_id;

    act_container->process_adv_action(act_info);
  }

  {
    AdvActionProcessor::AdvActionInfo act_info;
    act_info.time = Generics::Time(String::SubString("2008-01-01 00:00:01"),
      "%Y-%m-%d %H:%M:%S");
    act_info.ccg_id = 1;
    act_info.user_id = user_id;

    act_container->process_adv_action(act_info);
  }

  RequestInfo request_info;
  request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:00"),
    "%Y-%m-%d %H:%M:%S");
  request_info.ccg_id = 1;
  request_info.cc_id = 1;
  request_info.user_id = user_id;
  request_info.request_id = request_id;
  request_info.enabled_action_tracking = true;
  request_info.has_custom_actions = true;

  act_container->request_processor()->process_click(
    request_info,
    RequestActionProcessor::ProcessingState());

  if (act_processor->simple_actions.find(request_id) ==
     act_processor->simple_actions.end())
  {
    std::cerr << FUN << " - error: can't find action." << std::endl;
    return false;
  }

  if (act_processor->simple_actions[request_id] != 2)
  {
    std::cerr << FUN << " - error: result actions count(" <<
      act_processor->simple_actions[request_id] << ") != 2." <<
      std::endl;
    return false;
  }

  std::cout << FUN << " - success." << std::endl;
  return true;
}

bool action_ADSC_3478_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  act_processor->clear();

  RequestId request_id_1(generate_request_id());
  RequestId request_id_2(generate_request_id());
  UserId user_id(generate_user_id());

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:00"),
      "%Y-%m-%d %H:%M:%S");
    request_info.ccg_id = 1;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id_1;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_click(
      request_info,
      RequestActionProcessor::ProcessingState());
  }

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:01"),
      "%Y-%m-%d %H:%M:%S");
    request_info.ccg_id = 1;
    request_info.cc_id = 2;
    request_info.user_id = user_id;
    request_info.request_id = request_id_2;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_click(
      request_info,
      RequestActionProcessor::ProcessingState());
  }

  AdvActionProcessor::AdvActionInfo act_info;
  act_info.time = Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
  act_info.ccg_id = 1;
  act_info.user_id = user_id;

  act_container->process_adv_action(act_info);

  if (act_processor->simple_actions.find(request_id_1) !=
     act_processor->simple_actions.end())
  {
    std::cerr << "action_ADSC_3478_test - error: found excess request." <<
      std::endl;
    return false;
  }

  if (act_processor->simple_actions.find(request_id_2) ==
     act_processor->simple_actions.end())
  {
    std::cerr << "action_ADSC_3478_test - error: not found expected request." <<
      std::endl;
    return false;
  }

  if (act_processor->simple_actions[request_id_2] != 1)
  {
    std::cerr
      << "action_ADSC_3478_test - error: result actions count("
      << act_processor->simple_actions[request_id_2] << ") != 1."
      << std::endl;
    return false;
  }

  std::cout << "action_ADSC_3478_test - success." << std::endl;
  return true;
}

bool action_ADSC_3478_reverse_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  act_processor->clear();

  static const char* TEST = "action_ADSC_3478_reverse_test";

  RequestId request_id_1(generate_request_id());
  RequestId request_id_2(generate_request_id());
  UserId user_id(generate_user_id());

  AdvActionProcessor::AdvActionInfo act_info;
  act_info.time = Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
  act_info.ccg_id = 1;
  act_info.user_id = user_id;

  act_container->process_adv_action(act_info);

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:01"),
      "%Y-%m-%d %H:%M:%S");
    request_info.ccg_id = 1;
    request_info.cc_id = 2;
    request_info.user_id = user_id;
    request_info.request_id = request_id_2;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_click(
      request_info,
      RequestActionProcessor::ProcessingState());
  }

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:00"),
      "%Y-%m-%d %H:%M:%S");
    request_info.ccg_id = 1;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id_1;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_click(
      request_info,
      RequestActionProcessor::ProcessingState());
  }

  if (act_processor->simple_actions.find(request_id_1) !=
     act_processor->simple_actions.end())
  {
    std::cerr << TEST << ": error: found excess request." << std::endl;
    return false;
  }

  if (act_processor->simple_actions.find(request_id_2) ==
     act_processor->simple_actions.end())
  {
    std::cerr << TEST << ": error: not found expected request." << std::endl;
    return false;
  }

  if (act_processor->simple_actions[request_id_2] != 1)
  {
    std::cerr
      << TEST << ": error: result actions count("
      << act_processor->simple_actions[request_id_2] << ") != 1."
      << std::endl;
    return false;
  }

  std::cout << TEST << ": success." << std::endl;
  return true;
}

bool
custom_action_direct_order_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  act_processor->clear();

  static const char* TEST = "custom_action_direct_order_test";
  RequestId request_id;
  UserId user_id = generate_user_id();
  RequestInfo request_info1;

  {
    request_id = RequestId(generate_request_id());

    request_info1.time =
      Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
    request_info1.imp_time = request_info1.time;
    request_info1.ccg_id = 1;
    request_info1.cc_id = 1;
    request_info1.user_id = user_id;
    request_info1.request_id = request_id;
    request_info1.enabled_action_tracking = true;
    request_info1.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info1,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  RequestId request_id_2;

  {
    request_id_2 = RequestId(generate_request_id());

    RequestInfo request_info;
    request_info.time =
      Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 3;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id_2;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  AdvActionProcessor::AdvExActionInfo act_ex_info;
  act_ex_info.time =
    Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
  act_ex_info.action_id = 1;
  act_ex_info.ccg_ids.push_back(1);
  act_ex_info.ccg_ids.push_back(2);
  act_ex_info.action_request_id = user_id;
  act_ex_info.user_id = user_id;
  act_ex_info.referer = "test.com";
  act_ex_info.action_value = RevenueDecimal::ZERO;

  act_container->process_custom_action(act_ex_info);

  TestProcessor::CustomActionKey key(
    request_id,
    act_ex_info.action_id,
    act_ex_info.action_request_id,
    act_ex_info.referer.c_str());

  if (act_processor->custom_actions.find(key) ==
     act_processor->custom_actions.end())
  {
    std::cerr << TEST << ": error." << std::endl;
    return false;
  }

  if (act_processor->custom_actions[key] != 1)
  {
    std::cerr << TEST << ": error: result actions count(" <<
      act_processor->custom_actions[key] << ") != 1." << std::endl;
    return false;
  }

  if (act_processor->custom_actions.size() != 1)
  {
    std::cerr << TEST << ": error: "
      "more then 1 custom action in test container." <<
      std::endl;
    return false;
  }

  {
    request_info1.click_time = request_info1.imp_time;
    act_container->request_processor()->process_click(
      request_info1,
      RequestActionProcessor::ProcessingState());
  }

  if (act_processor->custom_actions[key] != 2) // imp + click
  {
    std::cerr << TEST << ": error: result actions count(" <<
      act_processor->custom_actions[key] << ") != 2 after click." << std::endl;
    return false;
  }

  if (act_processor->custom_actions.size() != 1)
  {
    std::cerr << TEST << ": error: "
      "more then 1 custom action in test container after click." <<
      std::endl;
    return false;
  }

  std::cout << TEST << ": success." << std::endl;
  return true;
}

bool
custom_action_direct_order_with_imp_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  static const char* TEST = "custom_action_direct_order_with_imp_test";

  act_processor->clear();

  RequestId request_id;
  UserId user_id = generate_user_id();

  {
    request_id = RequestId(generate_request_id());

    RequestInfo request_info;
    request_info.time =
      Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 1;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  RequestId request_id_2;

  {
    request_id_2 = RequestId(generate_request_id());

    RequestInfo request_info;
    request_info.time =
      Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 3;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id_2;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_click(
      request_info,
      RequestActionProcessor::ProcessingState());
  }

  AdvActionProcessor::AdvExActionInfo act_ex_info;
  act_ex_info.time =
    Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
  act_ex_info.action_id = 1;
  act_ex_info.ccg_ids.push_back(1);
  act_ex_info.ccg_ids.push_back(2);
  act_ex_info.action_request_id = user_id;
  act_ex_info.user_id = user_id;
  act_ex_info.referer = "test.com";
  act_ex_info.action_value = RevenueDecimal::ZERO;

  act_container->process_custom_action(act_ex_info);

  {
    TestProcessor::CustomActionKey key(
      request_id,
      act_ex_info.action_id,
      act_ex_info.action_request_id,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << TEST << ": error: not found action" << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr << TEST << ": error: result actions count(" <<
        act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  if (act_processor->custom_actions.size() != 1)
  {
    std::cerr << TEST << ": error: "
      "more then 1 custom action in test container." <<
      std::endl;
    return false;
  }

  std::cout << TEST << ": success." << std::endl;
  return true;
}

bool custom_two_action_direct_order_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  static const char* TEST = "custom_two_action_direct_order_test";

  act_processor->clear();

  RequestId request_id;
  UserId user_id;

  {
    request_id = RequestId(generate_request_id());
    user_id = UserId(generate_user_id());

    RequestInfo request_info;
    request_info.time =
      Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 1;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  RequestId request_id_2;

  {
    request_id_2 = RequestId(generate_request_id());

    RequestInfo request_info;
    request_info.time =
      Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 2;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id_2;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  AdvActionProcessor::AdvExActionInfo act_ex_info;
  act_ex_info.time =
    Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
  act_ex_info.action_id = 1;
  act_ex_info.ccg_ids.push_back(1);
  act_ex_info.ccg_ids.push_back(2);
  act_ex_info.action_request_id = user_id;
  act_ex_info.user_id = user_id;
  act_ex_info.referer = "test.com";
  act_ex_info.action_value = RevenueDecimal::ZERO;

  act_container->process_custom_action(act_ex_info);

  {
    TestProcessor::CustomActionKey key(
      request_id,
      act_ex_info.action_id,
      act_ex_info.action_request_id,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << TEST << ": error." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr << TEST << ": error: result actions count(" <<
        act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  {
    TestProcessor::CustomActionKey key(
      request_id_2,
      act_ex_info.action_id,
      act_ex_info.action_request_id,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << TEST << ": error: not found request 2." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr << TEST << ": error: result actions count(" <<
        act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  if (act_processor->custom_actions.size() != 2)
  {
    std::cerr << TEST << ": error: "
      "more then 2 custom action in test container." <<
      std::endl;
    return false;
  }

  std::cout << TEST << ": success." << std::endl;
  return true;
}

bool custom_imp_before_two_action_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  static const char* TEST = "custom_imp_before_two_action_test";
  
  act_processor->clear();

  UserId user_id =
    UserId(generate_user_id());

  AdvActionProcessor::AdvExActionInfo act_ex_info;
  act_ex_info.time = Generics::Time(String::SubString("2008-01-01 00:00:03"),
    "%Y-%m-%d %H:%M:%S");
  act_ex_info.action_id = 1;
  act_ex_info.ccg_ids.push_back(1);
  act_ex_info.ccg_ids.push_back(2);
  act_ex_info.action_request_id = user_id;
  act_ex_info.user_id = user_id;
  act_ex_info.referer = "test.com";
  act_ex_info.action_value = RevenueDecimal::ZERO;

  act_container->process_custom_action(act_ex_info);

  RequestId request_id(generate_request_id());

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:02"),
      "%Y-%m-%d %H:%M:%S");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 1;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  RequestId request_id_2(generate_request_id());

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:01"),
      "%Y-%m-%d %H:%M:%S");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 2;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id_2;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  {
    TestProcessor::CustomActionKey key(
      request_id,
      act_ex_info.action_id,
      act_ex_info.action_request_id,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << TEST << ": error." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr << TEST << ": error: result actions count(" <<
        act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  {
    TestProcessor::CustomActionKey key(
      request_id_2,
      act_ex_info.action_id,
      act_ex_info.action_request_id,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << TEST << ": error: not found request 2." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr << TEST << ": error: result actions count(" <<
        act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  if (act_processor->custom_actions.size() != 2)
  {
    std::cerr << TEST << ": error: "
      "more then 2 custom action in test container." <<
      std::endl;
    return false;
  }

  std::cout << TEST << ": success." << std::endl;
  return true;
}

bool custom_imp_between_two_action_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  static const char* TEST = "custom_imp_between_two_action_test";
  
  act_processor->clear();
  
  UserId user_id(generate_user_id());
  RequestId request_id(generate_request_id());

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:02"),
      "%Y-%m-%d %H:%M:%S");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 1;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  if (!act_processor->custom_actions.empty())
  {
    std::cerr << TEST << ": error: "
      "more then 2 custom action in test container." <<
      std::endl;
    return false;
  }

  AdvActionProcessor::AdvExActionInfo act_ex_info;
  act_ex_info.time = Generics::Time(String::SubString("2008-01-01 00:00:03"),
    "%Y-%m-%d %H:%M:%S");
  act_ex_info.action_id = 1;
  act_ex_info.ccg_ids.push_back(1);
  act_ex_info.ccg_ids.push_back(2);
  act_ex_info.action_request_id = user_id;
  act_ex_info.user_id = user_id;
  act_ex_info.referer = "test.com";
  act_ex_info.action_value = RevenueDecimal::ZERO;

  act_container->process_custom_action(act_ex_info);

  RequestId request_id_2(generate_request_id());

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:01"),
      "%Y-%m-%d %H:%M:%S");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 2;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id_2;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  {
    TestProcessor::CustomActionKey key(
      request_id,
      act_ex_info.action_id,
      act_ex_info.action_request_id,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << TEST << ": error." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr << TEST << ": error: result actions count(" <<
        act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  {
    TestProcessor::CustomActionKey key(
      request_id_2,
      act_ex_info.action_id,
      act_ex_info.action_request_id,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << TEST << ": error: not found request 2." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr << TEST << ": error: result actions count(" <<
        act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  if (act_processor->custom_actions.size() != 2)
  {
    std::cerr << TEST << ": error: "
      "more then 2 custom action in test container." <<
      std::endl;
    return false;
  }

  std::cout << TEST << ": success." << std::endl;
  return true;
}

bool custom_two_ccid_action_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  static const char* FUN = "custom_two_ccid_action_test";

  act_processor->clear();

  UserId user_id(generate_user_id());
  RequestId request_id(generate_request_id());
  RequestId request_id_2(generate_request_id());

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:02"),
      "%Y-%m-%d %H:%M:%S");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 1;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:02"),
      "%Y-%m-%d %H:%M:%S");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 1;
    request_info.cc_id = 2;
    request_info.user_id = user_id;
    request_info.request_id = request_id_2;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  if (!act_processor->custom_actions.empty())
  {
    std::cerr << FUN << " - error: excess custom action in test container." <<
      std::endl;
    return false;
  }

  AdvActionProcessor::AdvExActionInfo act_ex_info;
  act_ex_info.time = Generics::Time(String::SubString("2008-01-01 00:00:03"),
    "%Y-%m-%d %H:%M:%S");
  act_ex_info.action_id = 1;
  act_ex_info.ccg_ids.push_back(1);
  act_ex_info.ccg_ids.push_back(2);
  act_ex_info.action_request_id = user_id;
  act_ex_info.user_id = user_id;
  act_ex_info.referer = "test.com";
  act_ex_info.action_value = RevenueDecimal::ZERO;

  act_container->process_custom_action(act_ex_info);

  {
    TestProcessor::CustomActionKey key(
      request_id,
      act_ex_info.action_id,
      act_ex_info.action_request_id,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << FUN << " - error: not found request 1." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr
        << FUN << " - error: result actions count("
        << act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  {
    TestProcessor::CustomActionKey key(
      request_id_2,
      act_ex_info.action_id,
      act_ex_info.action_request_id,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << FUN << " - error: not found request 2." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr <<
        FUN << " - error: result actions count(" <<
        act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  if (act_processor->custom_actions.size() != 2)
  {
    std::cerr << FUN << " - error: "
      "more then 2 custom action in test container." <<
      std::endl;
    return false;
  }

  std::cout << FUN << ": success." << std::endl;
  return true;
}

bool custom_two_ccid_action_reverse_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  /* ADSC-5749
   * time : ccid1 (ccg_id1)
   * time + 5 : action linked to ccg_id1
   * time + 1 : ccid2 (ccg_id1)
   */
  static const char* FUN = "custom_two_ccid_action_reverse_test";

  act_processor->clear();

  UserId user_id(generate_user_id());
  RequestId request_id(generate_request_id());
  RequestId request_id_2(generate_request_id());
  RequestId act_request_id(generate_request_id());

  const Generics::Time base_time(
    String::SubString("2008-01-01 00:00:02"), "%Y-%m-%d %H:%M:%S");
  const unsigned long ACTION_ID = 1;
  const char REFERER[] = "test.com";

  {
    RequestInfo request_info;
    request_info.time = base_time;
    request_info.imp_time = base_time;
    request_info.ccg_id = 1;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id;
    request_info.enabled_action_tracking = false;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }
  
  {
    AdvActionProcessor::AdvExActionInfo act_ex_info;
    act_ex_info.time = base_time + 5;
    act_ex_info.action_id = ACTION_ID;
    act_ex_info.ccg_ids.push_back(1);
    act_ex_info.action_request_id = act_request_id;
    act_ex_info.user_id = user_id;
    act_ex_info.referer = REFERER;
    act_ex_info.action_value = RevenueDecimal::ZERO;

    act_container->process_custom_action(act_ex_info);
  }

  {
    // expect ccid1 action
    TestProcessor::CustomActionKey key(
      request_id,
      ACTION_ID,
      act_request_id,
      REFERER);

    if(act_processor->custom_actions.size() != 1)
    {
      std::cerr << FUN << " - error: "
        "more then one custom action in test container." <<
        std::endl;
      return false;
    }

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << FUN << " - error: not found request 1." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr <<
        FUN << " - error: result actions count(" <<
        act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  {
    RequestInfo request_info;
    request_info.time = base_time + 1;
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 1;
    request_info.cc_id = 2;
    request_info.user_id = user_id;
    request_info.request_id = request_id_2;
    request_info.enabled_action_tracking = false;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  {
    if(act_processor->custom_actions.size() != 2)
    {
      std::cerr << FUN << " - error: "
        "number of actions in test container != 2(" <<
        act_processor->custom_actions.size() << ")" <<
        std::endl;
      return false;
    }

    // expect ccid2 action
    TestProcessor::CustomActionKey key(
      request_id_2,
      ACTION_ID,
      act_request_id,
      REFERER);

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << FUN << " - error: not found request 1." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr <<
        FUN << " - error: result actions count(" <<
        act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  std::cout << FUN << ": success." << std::endl;
  return true;
}

bool
custom_action_timeout_test(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  static const char* FUN = "custom_action_timeout_test";

  act_processor->clear();

  UserId user_id(generate_user_id());
  RequestId request_id(generate_request_id());
  RequestId act_id_1(generate_request_id());
  RequestId act_id_2(generate_request_id());

  act_processor->custom_actions.clear();

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:02"),
      "%Y-%m-%d %H:%M:%S");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 1;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  if (!act_processor->custom_actions.empty())
  {
    std::cerr <<
      FUN << " - error: excess custom action in test container: " <<
      act_processor->custom_actions.size() << "." <<
      std::endl;
    return false;
  }

  {
    AdvActionProcessor::AdvExActionInfo act_ex_info;
    act_ex_info.time = Generics::Time(String::SubString("2008-01-01 00:00:03"),
      "%Y-%m-%d %H:%M:%S");
    act_ex_info.action_id = 1;
    act_ex_info.ccg_ids.push_back(1);
    act_ex_info.ccg_ids.push_back(2);
    act_ex_info.action_request_id = act_id_1;
    act_ex_info.user_id = user_id;
    act_ex_info.referer = "test.com";
    act_ex_info.action_value = RevenueDecimal::ZERO;

    act_container->process_custom_action(act_ex_info);

    TestProcessor::CustomActionKey key(
      request_id,
      act_ex_info.action_id,
      act_id_1,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << FUN << " - error: not found request 1." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr <<
        FUN << " - error: result actions count(" <<
        act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  {
    AdvActionProcessor::AdvExActionInfo act_ex_info;
    act_ex_info.time = Generics::Time(String::SubString("2008-01-01 00:00:10"),
      "%Y-%m-%d %H:%M:%S");
    act_ex_info.action_id = 1;
    act_ex_info.ccg_ids.push_back(1);
    act_ex_info.ccg_ids.push_back(2);
    act_ex_info.action_request_id = act_id_2;
    act_ex_info.user_id = user_id;
    act_ex_info.referer = "test.com";
    act_ex_info.action_value = RevenueDecimal::ZERO;

    act_container->process_custom_action(act_ex_info);

    TestProcessor::CustomActionKey key(
      request_id,
      act_ex_info.action_id,
      act_id_2,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) !=
       act_processor->custom_actions.end())
    {
      std::cerr << FUN <<
        " - error: found request 2 (it must be ignored)." << std::endl;
      return false;
    }
  }

  if (act_processor->custom_actions.size() != 1)
  {
    std::cerr << FUN << " - error: "
      "more then 2 custom action in test container." <<
      std::endl;
    return false;
  }

  std::cout << FUN << ": success." << std::endl;
  return true;
}

bool
ADSC_2051(
  UserActionInfoContainer* act_container,
  TestProcessor* act_processor)
{
  static const char* FUN = "custom_action_timeout_test(ADSC-2051)";

  act_processor->clear();

  UserId user_id(generate_user_id());
  RequestId request_id(generate_request_id());
  RequestId act_id_1(generate_request_id());
  RequestId act_id_2(generate_request_id());

  act_processor->custom_actions.clear();

  {
    RequestInfo request_info;
    request_info.time = Generics::Time(String::SubString("2008-01-01 00:00:02"),
      "%Y-%m-%d %H:%M:%S");
    request_info.imp_time = request_info.time;
    request_info.ccg_id = 1;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id;
    request_info.enabled_action_tracking = true;
    request_info.has_custom_actions = true;

    act_container->request_processor()->process_impression(
      request_info,
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());
  }

  if (!act_processor->custom_actions.empty())
  {
    std::cerr <<
      FUN << " - error: excess custom action in test container: " <<
      act_processor->custom_actions.size() << "." <<
      std::endl;
    return false;
  }

  {
    AdvActionProcessor::AdvExActionInfo act_ex_info;
    act_ex_info.time = Generics::Time(String::SubString("2008-01-01 00:00:03"),
      "%Y-%m-%d %H:%M:%S");
    act_ex_info.action_id = 1;
    act_ex_info.ccg_ids.push_back(1);
//  act_ex_info.ccg_ids.push_back(2); difference from custom_action_timeout_test
    act_ex_info.action_request_id = act_id_1;
    act_ex_info.user_id = user_id;
    act_ex_info.referer = "test.com";
    act_ex_info.action_value = RevenueDecimal::ZERO;

    act_container->process_custom_action(act_ex_info);

    TestProcessor::CustomActionKey key(
      request_id,
      act_ex_info.action_id,
      act_id_1,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) ==
       act_processor->custom_actions.end())
    {
      std::cerr << FUN << " - error: not found request 1." << std::endl;
      return false;
    }

    if (act_processor->custom_actions[key] != 1)
    {
      std::cerr << FUN << " - error: result actions count("
        << act_processor->custom_actions[key] << ") != 1." << std::endl;
      return false;
    }
  }

  {
    AdvActionProcessor::AdvExActionInfo act_ex_info;
    act_ex_info.time = Generics::Time(String::SubString("2008-01-01 00:00:10"),
      "%Y-%m-%d %H:%M:%S");
    act_ex_info.action_id = 1;
    act_ex_info.ccg_ids.push_back(1);
//  act_ex_info.ccg_ids.push_back(2);
    act_ex_info.action_request_id = act_id_2;
    act_ex_info.user_id = user_id;
    act_ex_info.referer = "test.com";
    act_ex_info.action_value = RevenueDecimal::ZERO;

    act_container->process_custom_action(act_ex_info);

    TestProcessor::CustomActionKey key(
      request_id,
      act_ex_info.action_id,
      act_id_2,
      act_ex_info.referer.c_str());

    if (act_processor->custom_actions.find(key) !=
       act_processor->custom_actions.end())
    {
      std::cerr << FUN
        << " - error: found request 2 (it must be ignored)." << std::endl;
      return false;
    }
  }

  if (act_processor->custom_actions.size() != 1)
  {
    std::cerr << FUN << " - error: "
      "more then 2 custom action in test container."
              << std::endl;
    return false;
  }

  std::cout << FUN << ": success." << std::endl;
  return true;
}

bool
init(int& argc, char**& argv) /*throw(eh::Exception)*/
{
  using namespace Generics::AppUtils;
  Args args;
  CheckOption opt_help;

  args.add(
    equal_name("path") ||
    short_name("p"),
    root_path);
  args.add(
    equal_name("help") ||
    short_name("h"),
    opt_help);

  args.parse(argc - 1, argv + 1);

  if (opt_help.enabled())
  {
    std::cout << USAGE << std::endl;
    return false;
  }
  return true;
}

int
main(int argc, char* argv[]) noexcept
{
  try
  {
    if (!init(argc, argv))
    {
      return 0;
    }

    Logging::Logger_var logger(new Logging::Null::Logger);
    /*
    Logging::Logger_var logger(
      new Logging::OStream::Logger(Logging::OStream::Config(std::cerr, 15)));
    */

    bool result = true;
    std::cout << "UserActionInfoContainer test started.." << std::endl;
    
    system(("rm -r " + *root_path + TEST_FOLDER +
      " 2>/dev/null ; mkdir -p " + *root_path + TEST_FOLDER).c_str());

    {
      TestProcessor_var act_processor(new TestProcessor());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserActionInfoContainer_var act_container(
        new UserActionInfoContainer(
          logger,
          act_processor,
          (*root_path + TEST_FOLDER).c_str(),
          "Actions",
          Generics::Time::ZERO, // action_ignore_time
          cache,
          Generics::Time(10), // expire time (sec)
          Generics::Time(2)));

      result &= no_action_test(act_container, act_processor);
      result &= action_direct_order_test(act_container, act_processor);
      result &= custom_action_direct_order_with_imp_test(act_container, act_processor);
      result &= action_three_test(act_container, act_processor);
      result &= action_before_imp_test(act_container, act_processor);
      result &= action_ADSC_3478_test(act_container, act_processor);
      result &= action_ADSC_3478_reverse_test(act_container, act_processor);
      result &= custom_two_ccid_action_test(act_container, act_processor);
      result &= action_before_and_after_imp_test(act_container, act_processor);
      result &= two_action_before_click_test(act_container, act_processor);
      result &= custom_two_ccid_action_reverse_test(act_container, act_processor);
    }

    {
      TestProcessor_var act_processor(new TestProcessor());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserActionInfoContainer_var act_container(
        new UserActionInfoContainer(
          logger,
          act_processor,
          (*root_path + TEST_FOLDER).c_str(),
          "Actions",
          Generics::Time::ZERO, // action_ignore_time
          cache,
          Generics::Time(10), // expire time (sec)
          Generics::Time(2)));

      result &=
        custom_action_direct_order_test(act_container, act_processor);
    }

    {
      TestProcessor_var act_processor(new TestProcessor());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserActionInfoContainer_var act_container(
        new UserActionInfoContainer(
          logger,
          act_processor,
          (*root_path + TEST_FOLDER).c_str(),
          "Actions",
          Generics::Time::ZERO, // action_ignore_time
          cache,
          Generics::Time(10), // expire time (sec)
          Generics::Time(2)));

      result &=
        custom_two_action_direct_order_test(act_container, act_processor);
    }

    {
      TestProcessor_var act_processor(new TestProcessor());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserActionInfoContainer_var act_container(
        new UserActionInfoContainer(
          logger,
          act_processor,
          (*root_path + TEST_FOLDER).c_str(),
          "Actions",
          Generics::Time::ZERO, // action_ignore_time
          cache,
          Generics::Time(10), // expire time (sec)
          Generics::Time(2)));

      result &=
        custom_imp_before_two_action_test(act_container, act_processor);
    }

    {
      TestProcessor_var act_processor(new TestProcessor());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserActionInfoContainer_var act_container(
        new UserActionInfoContainer(
          logger,
          act_processor,
          (*root_path + TEST_FOLDER).c_str(),
          "Actions",
          Generics::Time::ZERO, // action_ignore_time
          cache,
          Generics::Time(10), // expire time (sec)
          Generics::Time(2)));

      result &=
        custom_imp_between_two_action_test(act_container, act_processor);
    }

    {
      TestProcessor_var act_processor(new TestProcessor());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserActionInfoContainer_var act_container(
        new UserActionInfoContainer(
          logger,
          act_processor,
          (*root_path + TEST_FOLDER).c_str(),
          "Actions",
          Generics::Time(10), // action_ignore_time
          cache,
          Generics::Time(10), // expire time (sec)
          Generics::Time(2)));

      result &= custom_action_timeout_test(act_container, act_processor);
    }

    {
      TestProcessor_var act_processor(new TestProcessor());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserActionInfoContainer_var act_container(
        new UserActionInfoContainer(
          logger,
          act_processor,
          (*root_path + TEST_FOLDER).c_str(),
          "Actions",
          Generics::Time(10), // action_ignore_time
          cache,
          Generics::Time(10), // expire time (sec)
          Generics::Time(2)));

      result &= ADSC_2051(act_container, act_processor);
    }

    {
      TestProcessor_var act_processor(new TestProcessor());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserActionInfoContainer_var act_container(
        new UserActionInfoContainer(
          logger,
          act_processor,
          (*root_path + TEST_FOLDER).c_str(),
          "Actions",
          Generics::Time::ZERO, // action_ignore_time
          cache,
          Generics::Time(10), // expire time (sec)
          Generics::Time(2)));

      TestIt test_it;
      test_it.act_container = act_container.in();
      test_it.act_processor = act_processor.in();
      result &= multi_thread_test(&test_it);
    }

    return result ? 0 : 1;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  return -1;
}
