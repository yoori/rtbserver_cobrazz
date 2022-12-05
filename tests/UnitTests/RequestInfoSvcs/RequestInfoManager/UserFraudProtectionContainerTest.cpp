#include <Generics/AppUtils.hpp>
#include <Generics/Uuid.hpp>

#include <RequestInfoSvcs/RequestInfoManager/UserFraudProtectionContainer.hpp>

using namespace AdServer::RequestInfoSvcs;
using AdServer::Commons::UserId;
using AdServer::Commons::RequestId;

namespace
{
  const char DEFAULT_ROOT_PATH[] = "./";

  const char TEST_FOLDER[] = "/UserFraudProtectionContainerTestDir/Users/";

  const char USAGE[] =
    "UserFraudProtectionContainerTest [OPTIONS]\n"
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

  RequestInfo
  fill_simple_request_info(
    const UserId& user_id,
    const Generics::Time& time)
  {
    RequestId request_id(generate_request_id());

    RequestInfo request_info;
    request_info.time = time;
    request_info.click_time = time;
    request_info.ccg_id = 1;
    request_info.cc_id = 1;
    request_info.user_id = user_id;
    request_info.request_id = request_id;
    request_info.position = 1;
    request_info.enabled_action_tracking = true;

    return request_info;
  }
}

typedef std::vector<RequestId> RequestIdArray;

struct TestProcessor:
  public AdServer::RequestInfoSvcs::RequestContainerProcessor,
  public virtual ReferenceCounting::AtomicImpl
{
  typedef std::set<RequestId> RequestIdSet;

  virtual void
  process_request(const AdServer::RequestInfoSvcs::RequestInfo& /*ri*/)
    noexcept
  {}

  virtual void
  process_impression(const ImpressionInfo& /*imp_info*/)
    noexcept
  {}

  virtual void
  process_action(
    ActionType action_type,
    const Generics::Time& /*time*/,
    const AdServer::Commons::RequestId& request_id)
    /*throw(RequestContainerProcessor::Exception)*/
  {
    if(action_type == RequestContainerProcessor::AT_FRAUD_ROLLBACK)
    {
      RequestIdSet::iterator it = fraud_imps.find(request_id);
      if(it != fraud_imps.end())
      {
        /*
        throw RequestContainerProcessor::Exception(
          "double fraud rollback");
        */
      }
      else
      {
//      std::cerr << "rollback: " << time.gm_ft() << std::endl;
        fraud_imps.insert(request_id);
      }
    }
  }

  virtual void
  process_custom_action(
    const AdServer::Commons::RequestId&,
    const AdServer::RequestInfoSvcs::AdvCustomActionInfo&)
    noexcept
  {}

  virtual void
  process_impression_post_action(
    const AdServer::Commons::RequestId&,
    const RequestPostActionInfo&)
    /*throw(Exception)*/
  {}

  void
  clear() noexcept
  {
    fraud_imps.clear();
  }

  RequestIdSet fraud_imps;

protected:
  virtual ~TestProcessor() noexcept
  {}
};

typedef ReferenceCounting::SmartPtr<TestProcessor> TestProcessor_var;

struct TestCallback:
  public AdServer::RequestInfoSvcs::UserFraudProtectionContainer::Callback,
  public virtual ReferenceCounting::AtomicImpl
{
  virtual void
  detected_fraud_user(
    const UserId& /*user_id*/,
    const Generics::Time& deactivate_time)
    noexcept
  {
//  std::cerr << "detected_fraud_user(): " << deactivate_time.gm_ft() << std::endl;
    last_deactivate_time = deactivate_time;
    max_deactivate_time = std::max(max_deactivate_time, deactivate_time);
  }

  Generics::Time last_deactivate_time;
  Generics::Time max_deactivate_time;
};

typedef ReferenceCounting::SmartPtr<TestCallback> TestCallback_var;

template<typename Iterator>
void
print_time_seq(std::ostream& out, Iterator begin, Iterator end)
{
  for(Iterator it = begin; it != end; ++it)
  {
    if(it != begin)
    {
      out << ", ";
    }

    out << it->get_gm_time();
  }
}

struct RequestInfoWrap: public RequestInfo
{
  enum Type
  {
    IMPRESSION,
    CLICK
  };

  RequestInfoWrap(
    const RequestInfo& request_info,
    unsigned long id_val,
    const RequestIdArray& request_ids_val,
    Type type_val = IMPRESSION)
    : RequestInfo(request_info),
      id(id_val),
      request_ids(request_ids_val),
      type(type_val)
  {}

  bool operator<(const RequestInfoWrap& right) const
  {
    return id < right.id;
  }

  unsigned long id;
  RequestIdArray request_ids;
  Type type;
};

typedef std::vector<RequestInfoWrap> RequestInfoWrapArray;

void
print_request_array(
  std::ostream& out,
  const RequestInfoWrapArray& requests,
  const char* offset)
{
  for(RequestInfoWrapArray::const_iterator it = requests.begin();
      it != requests.end(); ++it)
  {
    out << offset << "[ " << it->time.gm_ft() << "(" <<
      (it->type == RequestInfoWrap::IMPRESSION ? "imp" : "click") <<
      "): " << it->request_ids.size() << " ]" << std::endl;
  }
}

bool
simple_test(
  UserFraudProtectionContainer* fraud_container,
  TestProcessor* test_processor)
{
  static const char* TEST_NAME = "simple_test";

  try
  {
    test_processor->clear();

    UserFraudProtectionContainer::Config_var config(
      new UserFraudProtectionContainer::Config());
    UserFraudProtectionContainer::Config::FraudRule rule;
    rule.limit = 1000;
    rule.period = Generics::Time::ONE_DAY;
    config->imp_rules.add_rule(rule);
    fraud_container->config(config);

    UserId user_id(generate_user_id());

    for(unsigned long i = 0; i < 999; ++i)
    {
      RequestId request_id(generate_request_id());

      RequestInfo request_info;
      request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      request_info.ccg_id = 1;
      request_info.cc_id = 1;
      request_info.user_id = user_id;
      request_info.request_id = request_id;
      request_info.enabled_action_tracking = true;

      fraud_container->process_impression(
        request_info,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (!test_processor->fraud_imps.empty())
    {
      std::cerr << TEST_NAME << ": error: non empty fraud imps of clicks after 999 imps" << std::endl;
      return false;
    }

    for(unsigned long i = 0; i < 1; ++i)
    {
      RequestId request_id(generate_request_id());

      RequestInfo request_info;
      request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") + 1000;
      request_info.ccg_id = 1;
      request_info.cc_id = 1;
      request_info.user_id = user_id;
      request_info.request_id = request_id;
      request_info.enabled_action_tracking = true;

      fraud_container->process_impression(
        request_info,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (test_processor->fraud_imps.size() != 1000)
    {
      std::cerr << TEST_NAME << ": error: fraud imps number " <<
        test_processor->fraud_imps.size() << " non equal 1000" << std::endl;
      return false;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << TEST_NAME << ": caught eh::Exception: " << ex.what() << std::endl;
    return false;
  }

  std::cout << TEST_NAME << ": success." << std::endl;
  return true;
}

bool
reverse_order_test(
  UserFraudProtectionContainer* fraud_container,
  TestProcessor* test_processor)
{
  static const char* TEST_NAME = "reverse_order_test";

  try
  {
    test_processor->clear();

    UserFraudProtectionContainer::Config_var config(
      new UserFraudProtectionContainer::Config());
    UserFraudProtectionContainer::Config::FraudRule rule;
    rule.limit = 1000;
    rule.period = Generics::Time::ONE_DAY;
    config->imp_rules.add_rule(rule);
    fraud_container->config(config);

    UserId user_id(generate_user_id());

    for(unsigned long i = 999; i > 0; --i)
    {
      RequestId request_id(generate_request_id());

      RequestInfo request_info;
      request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d");
      request_info.ccg_id = 1;
      request_info.cc_id = 1;
      request_info.user_id = user_id;
      request_info.request_id = request_id;
      request_info.enabled_action_tracking = true;

      fraud_container->process_impression(
        request_info,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (!test_processor->fraud_imps.empty())
    {
      std::cerr << TEST_NAME << ": error: non empty fraud imps after 999 imps" << std::endl;
      return false;
    }

    for(unsigned long i = 0; i < 1; ++i)
    {
      RequestId request_id(generate_request_id());

      RequestInfo request_info;
      request_info.time =
        Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") + i;
      request_info.ccg_id = 1;
      request_info.cc_id = 1;
      request_info.user_id = user_id;
      request_info.request_id = request_id;
      request_info.enabled_action_tracking = true;

      fraud_container->process_impression(
        request_info,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (test_processor->fraud_imps.size() != 1000)
    {
      std::cerr << TEST_NAME << ": error: fraud imps number " <<
        test_processor->fraud_imps.size() << " non equal 1000" << std::endl;
      return false;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << TEST_NAME << ": caught eh::Exception: " << ex.what() << std::endl;
    return false;
  }

  std::cout << TEST_NAME << ": success." << std::endl;
  return true;
}

bool
strange_order_test(
  UserFraudProtectionContainer* fraud_container,
  TestProcessor* test_processor)
{
  static const char* TEST_NAME = "strange_order_test";

  try
  {
    test_processor->clear();

    UserFraudProtectionContainer::Config_var config(
      new UserFraudProtectionContainer::Config());

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = 10;
      rule.period = Generics::Time(10);
      config->imp_rules.add_rule(rule);
    }

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = 1000;
      rule.period = Generics::Time(1000);
      config->imp_rules.add_rule(rule);
    }

    fraud_container->config(config);

    UserId user_id(generate_user_id());

    for(unsigned long i = 0; i < 9; ++i)
    {
      fraud_container->process_impression(
        fill_simple_request_info(
          user_id,
          Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") + i),
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (!test_processor->fraud_imps.empty())
    {
      std::cerr << TEST_NAME << ": error: non empty fraud imps after 9 imps" << std::endl;
      return false;
    }

    for(unsigned long i = 11; i < 20; ++i)
    {
      fraud_container->process_impression(
        fill_simple_request_info(
          user_id,
          Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") + i),
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (!test_processor->fraud_imps.empty())
    {
      std::cerr << TEST_NAME << ": error: non empty = " <<
        test_processor->fraud_imps.size() << " fraud imps after 9 imps" << std::endl;
      return false;
    }

    for(unsigned long i = 9; i < 10; ++i)
    {
      fraud_container->process_impression(
        fill_simple_request_info(
          user_id,
          Generics::Time(String::SubString("2008-01-01"), "%Y-%m-%d") + i),
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (test_processor->fraud_imps.size() != 19)
    {
      std::cerr << TEST_NAME << ": error: fraud imps number " <<
        test_processor->fraud_imps.size() << " non equal 19" << std::endl;
      return false;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << TEST_NAME << ": caught eh::Exception: " << ex.what() << std::endl;
    return false;
  }

  std::cout << TEST_NAME << ": success." << std::endl;
  return true;
}

bool
double_fraud_test(
  UserFraudProtectionContainer* fraud_container,
  TestProcessor* test_processor)
{
  static const char* TEST_NAME = "double_fraud_test";

  try
  {
    test_processor->clear();

    UserFraudProtectionContainer::Config_var config(
      new UserFraudProtectionContainer::Config());

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = 60;
      rule.period = Generics::Time::ONE_MINUTE;
      config->imp_rules.add_rule(rule);
    }

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = 120;
      rule.period = Generics::Time::ONE_MINUTE * 3;
      config->imp_rules.add_rule(rule);
    }

    fraud_container->config(config);

    UserId user_id(generate_user_id());

    Generics::Time base_time(String::SubString("2008-01-01"), "%Y-%m-%d");

    for(unsigned long i = 0; i < 60; ++i)
    {
      RequestId request_id(generate_request_id());

      RequestInfo request_info;
      request_info.time = base_time + i;
      request_info.ccg_id = 1;
      request_info.cc_id = 1;
      request_info.user_id = user_id;
      request_info.request_id = request_id;
      request_info.enabled_action_tracking = true;

      fraud_container->process_impression(
        request_info,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (test_processor->fraud_imps.size() != 60)
    {
      std::cerr << TEST_NAME << ": error: fraud imps after 60 imps != 60" << std::endl;
      return false;
    }

    for(unsigned long i = 0; i < 60; ++i)
    {
      RequestId request_id(generate_request_id());

      RequestInfo request_info;
      request_info.time = base_time + 60 + i*2;
      request_info.ccg_id = 1;
      request_info.cc_id = 1;
      request_info.user_id = user_id;
      request_info.request_id = request_id;
      request_info.enabled_action_tracking = true;

      fraud_container->process_impression(
        request_info,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (test_processor->fraud_imps.size() != 120)
    {
      std::cerr << TEST_NAME << ": error: fraud imps after 120 imps != 120: " <<
        test_processor->fraud_imps.size() << std::endl;
      return false;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << TEST_NAME << ": caught eh::Exception: " << ex.what() << std::endl;
    return false;
  }

  std::cout << TEST_NAME << ": success." << std::endl;
  return true;
}

bool
fraud_time_test(
  UserFraudProtectionContainer* fraud_container,
  TestProcessor* test_processor,
  TestCallback* test_callback)
{
  // ADSC-3919 regression
  static const char* TEST_NAME = "fraud_time_test";

  try
  {
    test_processor->clear();

    UserFraudProtectionContainer::Config_var config(
      new UserFraudProtectionContainer::Config());

    config->deactivate_period = Generics::Time(60);

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = 40;
      rule.period = Generics::Time::ONE_MINUTE;
      config->imp_rules.add_rule(rule);
    }

    fraud_container->config(config);

    UserId user_id(generate_user_id());

    Generics::Time base_time(String::SubString("2008-01-01"), "%Y-%m-%d");

    for(unsigned long i = 0; i < 41; ++i)
    {
      RequestId request_id(generate_request_id());

      RequestInfo request_info;
      request_info.time = base_time;
      request_info.ccg_id = 1;
      request_info.cc_id = 1;
      request_info.user_id = user_id;
      request_info.request_id = request_id;
      request_info.enabled_action_tracking = false;

      fraud_container->process_impression(
        request_info,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if(test_callback->last_deactivate_time != base_time + Generics::Time::ONE_MINUTE*2)
    {
      std::cerr << TEST_NAME << ": error: incorrect deactivate time: " <<
        test_callback->last_deactivate_time.get_gm_time() <<
        " instead " << (base_time + Generics::Time::ONE_MINUTE*2
          ).get_gm_time() << std::endl;
      return false;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << TEST_NAME << ": caught eh::Exception: " << ex.what() << std::endl;
    return false;
  }

  std::cout << TEST_NAME << ": success." << std::endl;
  return true;
}

bool
fraud_non_first_pos_test(
  UserFraudProtectionContainer* fraud_container,
  TestProcessor* test_processor)
{
  static const char* TEST_NAME = "fraud_non_first_pos_test";

  try
  {
    test_processor->clear();

    UserFraudProtectionContainer::Config_var config(
      new UserFraudProtectionContainer::Config());

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = 10;
      rule.period = Generics::Time::ONE_MINUTE;
      config->imp_rules.add_rule(rule);
    }

    fraud_container->config(config);

    UserId user_id(generate_user_id());

    Generics::Time base_time(String::SubString("2008-01-01"), "%Y-%m-%d");

    for(unsigned long i = 0; i < 10; ++i)
    {
      RequestId request_id(generate_request_id());

      RequestInfo request_info;
      request_info.time = base_time;
      request_info.ccg_id = 1;
      request_info.cc_id = 1;
      request_info.user_id = user_id;
      request_info.request_id = request_id;
      request_info.position = 1;
      request_info.enabled_action_tracking = true;

      fraud_container->process_impression(
        request_info,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    for(unsigned long i = 0; i < 10; ++i)
    {
      RequestId request_id(generate_request_id());

      RequestInfo request_info;
      request_info.time = base_time;
      request_info.ccg_id = 1;
      request_info.cc_id = 1;
      request_info.user_id = user_id;
      request_info.request_id = request_id;
      request_info.position = 2;
      request_info.enabled_action_tracking = true;
      fraud_container->process_impression(
        request_info,
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (test_processor->fraud_imps.size() != 20)
    {
      std::cerr << TEST_NAME << ": error: fraud imps after 10*2 imps: " <<
        test_processor->fraud_imps.size() << " != 20" <<
        std::endl;
      return false;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << TEST_NAME << ": caught eh::Exception: " << ex.what() << std::endl;
    return false;
  }

  std::cout << TEST_NAME << ": success." << std::endl;
  return true;
}

bool
fraud_move_test(
  UserFraudProtectionContainer* fraud_container,
  TestProcessor* test_processor,
  TestCallback* test_callback)
{
  static const char* TEST_NAME = "fraud_move_test";

  const unsigned long REQUESTS_PER_TIME = 10;
  const Generics::Time DEACTIVATE_PERIOD = Generics::Time::ONE_MINUTE;
  const Generics::Time RULE_PERIOD = Generics::Time::ONE_MINUTE;

  try
  {
    test_processor->clear();

    UserFraudProtectionContainer::Config_var config(
      new UserFraudProtectionContainer::Config());

    config->deactivate_period = DEACTIVATE_PERIOD;

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = REQUESTS_PER_TIME;
      rule.period = RULE_PERIOD;
      config->imp_rules.add_rule(rule);
    }

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = 5 * REQUESTS_PER_TIME;
      rule.period = RULE_PERIOD * 10;
      config->imp_rules.add_rule(rule);
    }

    fraud_container->config(config);

    Generics::Time base_time(String::SubString("2008-01-01"), "%Y-%m-%d");

    Generics::Time TIMES[4] = {
      base_time,
      base_time + Generics::Time::ONE_MINUTE + 1,
      base_time + Generics::Time::ONE_MINUTE * 2 + 1,
      base_time + Generics::Time::ONE_MINUTE * 3
    };

    while(std::next_permutation(TIMES, TIMES + sizeof(TIMES) / sizeof(TIMES[0])))
    {
      test_processor->clear();
      test_callback->max_deactivate_time = Generics::Time::ZERO;

      UserId user_id(generate_user_id());

      for(unsigned long j = 0; j < 4; ++j)
      {
//      std::cerr << "IT #" << j << std::endl;

        for(unsigned long i = 0; i < REQUESTS_PER_TIME; ++i)
        {
          RequestId request_id(generate_request_id());

          RequestInfo request_info;
          request_info.time = TIMES[j];
          request_info.ccg_id = 1;
          request_info.cc_id = 1;
          request_info.user_id = user_id;
          request_info.request_id = request_id;
          request_info.position = 1;
          request_info.enabled_action_tracking = true;

          fraud_container->process_impression(
            request_info,
            ImpressionInfo(),
            RequestActionProcessor::ProcessingState());
        }
      }

      if (test_processor->fraud_imps.size() !=
          sizeof(TIMES) / sizeof(TIMES[0]) * REQUESTS_PER_TIME)
      {
        std::cerr << TEST_NAME << ": error: fraud imps after all imps: " <<
          test_processor->fraud_imps.size() << " != 40: by " <<
          REQUESTS_PER_TIME << " requests for [";
        print_time_seq(std::cerr, TIMES, TIMES + sizeof(TIMES) / sizeof(TIMES[0]));
        std::cerr << "]" << std::endl;
        return false;
      }

      const Generics::Time expected_deactivate_time =
        base_time +
        Generics::Time::ONE_MINUTE * 3 +
        RULE_PERIOD +
        DEACTIVATE_PERIOD
        ;

      if(test_callback->max_deactivate_time != expected_deactivate_time)
      {
        std::cerr << TEST_NAME << ": error: incorrect user max deactivation time: " <<
          test_callback->max_deactivate_time.get_gm_time() <<
          " != " << expected_deactivate_time.get_gm_time() <<
          ": by " << REQUESTS_PER_TIME << " requests for [";
        print_time_seq(std::cerr, TIMES, TIMES + sizeof(TIMES) / sizeof(TIMES[0]));
        std::cerr << "]" << std::endl;
        return false;
      }
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << TEST_NAME << ": caught eh::Exception: " << ex.what() << std::endl;
    return false;
  }

  std::cout << TEST_NAME << ": success." << std::endl;
  return true;
}

bool
fraud_deactivate_period_test(
  UserFraudProtectionContainer* fraud_container,
  TestProcessor* test_processor,
  TestCallback* test_callback)
{
  static const char* TEST_NAME = "fraud_deactivate_period_test";

  const unsigned long REQUESTS_PER_TIME = 10;
  const Generics::Time DEACTIVATE_PERIOD = Generics::Time::ONE_HOUR;
  const Generics::Time RULE_PERIOD = Generics::Time::ONE_MINUTE;

  try
  {
    test_processor->clear();

    UserFraudProtectionContainer::Config_var config(
      new UserFraudProtectionContainer::Config());

    config->deactivate_period = DEACTIVATE_PERIOD;

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = REQUESTS_PER_TIME;
      rule.period = RULE_PERIOD;
      config->imp_rules.add_rule(rule);
    }

    fraud_container->config(config);

    Generics::Time base_time(String::SubString("2008-01-01"), "%Y-%m-%d");
    UserId user_id(generate_user_id());

    for(unsigned long i = 0; i < REQUESTS_PER_TIME; ++i)
    {
      fraud_container->process_impression(
        fill_simple_request_info(user_id, base_time),
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (test_processor->fraud_imps.size() != REQUESTS_PER_TIME)
    {
      std::cerr << TEST_NAME << ": error: fraud imps after all imps: " <<
        test_processor->fraud_imps.size() << " != " <<
        REQUESTS_PER_TIME << ": by " <<
        REQUESTS_PER_TIME << " requests for " <<
        base_time.get_gm_time() << std::endl;
      return false;
    }

    const Generics::Time expected_deactivate_time =
      base_time +
      RULE_PERIOD +
      DEACTIVATE_PERIOD
      ;

    if(test_callback->max_deactivate_time != expected_deactivate_time)
    {
      std::cerr << TEST_NAME << ": error: incorrect user max deactivation time: " <<
        test_callback->max_deactivate_time.get_gm_time() <<
        " != " << expected_deactivate_time.get_gm_time() <<
        ": by " << REQUESTS_PER_TIME << " requests for " <<
        base_time.get_gm_time() << std::endl;
      return false;
    }

    // send additional request for up deactivate period bound
    fraud_container->process_impression(
      fill_simple_request_info(
        user_id,
        expected_deactivate_time - Generics::Time::ONE_SECOND),
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());

    if (test_processor->fraud_imps.size() != REQUESTS_PER_TIME + 1)
    {
      std::cerr << TEST_NAME << ": error: fraud imps after additional imp on deactivate bound: " <<
        test_processor->fraud_imps.size() << " != " <<
        (REQUESTS_PER_TIME + 1) << std::endl;
      return false;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << TEST_NAME << ": caught eh::Exception: " << ex.what() << std::endl;
    return false;
  }

  std::cout << TEST_NAME << ": success." << std::endl;
  return true;
}

bool
fraud_reverse_deactivate_period_test(
  UserFraudProtectionContainer* fraud_container,
  TestProcessor* test_processor,
  TestCallback* test_callback)
{
  // regression for case when latest request activate fraud condition
  // and this request time is less then time of other requests,
  // that activate condition
  static const char* TEST_NAME = "fraud_reverse_deactivate_period_test";

  const unsigned long REQUESTS_PER_TIME = 15;
  const Generics::Time DEACTIVATE_PERIOD = Generics::Time::ONE_MINUTE;
  const Generics::Time RULE_PERIOD = Generics::Time::ONE_MINUTE;

  try
  {
    test_processor->clear();

    UserFraudProtectionContainer::Config_var config(
      new UserFraudProtectionContainer::Config());

    config->deactivate_period = DEACTIVATE_PERIOD;

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = REQUESTS_PER_TIME;
      rule.period = RULE_PERIOD;
      config->imp_rules.add_rule(rule);
    }

    fraud_container->config(config);

    UserId user_id(generate_user_id());
    Generics::Time base_time(
      String::SubString("2008-01-01 01:00:40"),
      "%Y-%m-%d %H:%M:%S");

    for(unsigned long i = 0; i < REQUESTS_PER_TIME - 1; ++i)
    {
      fraud_container->process_impression(
        fill_simple_request_info(user_id, base_time + 20),
        ImpressionInfo(),
        RequestActionProcessor::ProcessingState());
    }

    if (!test_processor->fraud_imps.empty())
    {
      std::cerr << TEST_NAME << ": error: fraud imps when unexpected: " <<
        test_processor->fraud_imps.size() << std::endl;
      return false;
    }

    fraud_container->process_impression(
      fill_simple_request_info(user_id, base_time),
      ImpressionInfo(),
      RequestActionProcessor::ProcessingState());

    const Generics::Time expected_deactivate_time =
      base_time +
      RULE_PERIOD +
      DEACTIVATE_PERIOD
      ;

    if(test_callback->max_deactivate_time != expected_deactivate_time)
    {
      std::cerr << TEST_NAME << ": error: incorrect user max deactivation time: " <<
        test_callback->max_deactivate_time.get_gm_time() <<
        " != " << expected_deactivate_time.get_gm_time() <<
        "(expected): by " << REQUESTS_PER_TIME << " requests for " <<
        base_time.get_gm_time() << std::endl;
      return false;
    }

    if (test_processor->fraud_imps.size() != REQUESTS_PER_TIME)
    {
      std::cerr << TEST_NAME << ": error: number of fraud imps is incorrect: " <<
        test_processor->fraud_imps.size() << " != " <<
        (REQUESTS_PER_TIME + 1) << std::endl;
      return false;
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << TEST_NAME << ": caught eh::Exception: " << ex.what() << std::endl;
    return false;
  }

  std::cout << TEST_NAME << ": success." << std::endl;
  return true;
}

bool
complex_deactivate_period_test(
  UserFraudProtectionContainer* fraud_container,
  TestProcessor* test_processor,
  TestCallback* test_callback)
{
  struct TimeToCounter
  {
    Generics::Time time;
    unsigned long count;
  };

  // regression
  static const char* TEST_NAME = "complex_deactivate_period_test";

  const Generics::Time DEACTIVATE_PERIOD = Generics::Time::ONE_MINUTE;

  try
  {
    UserFraudProtectionContainer::Config_var config(
      new UserFraudProtectionContainer::Config());

    config->deactivate_period = DEACTIVATE_PERIOD;

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = 10000;
      rule.period = Generics::Time::ONE_DAY;
      config->imp_rules.add_rule(rule);
      config->click_rules.add_rule(rule);
    }

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = 15;
      rule.period = Generics::Time::ONE_MINUTE;
      config->imp_rules.add_rule(rule);
    }

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = 10;
      rule.period = Generics::Time::ONE_SECOND;
      config->imp_rules.add_rule(rule);
//    config->click_rules.add_rule(rule);
    }

    {
      UserFraudProtectionContainer::Config::FraudRule rule;
      rule.limit = 40;
      rule.period = Generics::Time::ONE_MINUTE * 3;
      config->imp_rules.add_rule(rule);
//    config->click_rules.add_rule(rule);
    }

    fraud_container->config(config);

    Generics::Time base_time(
    String::SubString("2008-01-01 12:00:40"),
      "%Y-%m-%d %H:%M:%S");

    const TimeToCounter TIMES[] = {
      { base_time + 1, 10 },
      { base_time + Generics::Time::ONE_MINUTE, 3 },
      { base_time + Generics::Time::ONE_MINUTE + 2, 9 },
      { base_time + Generics::Time::ONE_MINUTE * 2 + 1, 9 },
      { base_time + Generics::Time::ONE_MINUTE * 3, 9 },
    };

    unsigned long request_id = 0;
    RequestInfoWrapArray requests;

    for(unsigned long i = 0; i < sizeof(TIMES) / sizeof(TIMES[0]); ++i)
    {
      RequestIdArray request_ids;
      for(unsigned long request_id_i = 0;
          request_id_i < TIMES[i].count; ++request_id_i)
      {
        request_ids.push_back(generate_request_id());
      }

      requests.push_back(RequestInfoWrap(
        fill_simple_request_info(UserId(), TIMES[i].time),
        ++request_id,
        request_ids));

      /*
      requests.push_back(RequestInfoWrap(
        fill_simple_request_info(UserId(), TIMES[i].time),
        ++request_id,
        request_ids,
        RequestInfoWrap::CLICK));
      */
    }

    const Generics::Time expected_deactivate_time =
      base_time + Generics::Time::ONE_MINUTE * 2 + 1 +
      Generics::Time::ONE_MINUTE +
      DEACTIVATE_PERIOD
      ;

    std::cout << "REQUESTS.size() = " << requests.size() << std::endl;

    unsigned long perm_i = 0;

    while(std::next_permutation(requests.begin(), requests.end()))
    {
      ++perm_i;

      if(perm_i % 10000 == 0)
      {
        std::cout << perm_i << "/3628800" << std::endl;
      }

//    std::cerr << "===================================================================" << std::endl;
      test_processor->clear();

      UserId user_id(generate_user_id());

      for(RequestInfoWrapArray::const_iterator req_it = requests.begin();
          req_it != requests.end(); ++req_it)
      {
        RequestInfo request_info(*req_it);
        request_info.user_id = user_id;

        for(RequestIdArray::const_iterator
              request_id_it = req_it->request_ids.begin();
            request_id_it != req_it->request_ids.end();
            ++request_id_it)
        {
          request_info.request_id = *request_id_it;

          if(req_it->type == RequestInfoWrap::IMPRESSION)
          {
            fraud_container->process_impression(
              request_info,
              ImpressionInfo(),
              RequestActionProcessor::ProcessingState());
          }
          else
          {
            fraud_container->process_click(
              request_info,
              RequestActionProcessor::ProcessingState());
          }
        }
      }

      if(test_callback->max_deactivate_time != expected_deactivate_time)
      {
        std::cerr << TEST_NAME << ": error: incorrect user max deactivation time: " <<
          test_callback->max_deactivate_time.get_gm_time() <<
          " != " << expected_deactivate_time.get_gm_time() <<
          "(expected), at sequence: " << std::endl;
        print_request_array(std::cerr, requests, "  ");
        std::cerr << std::endl;
        return false;
      }

      if (test_processor->fraud_imps.size() != 40)
      {
        std::cerr << TEST_NAME << ": error: number of fraud imps is incorrect: " <<
          test_processor->fraud_imps.size() <<
          " != 40, at sequence: " << std::endl;
        print_request_array(std::cerr, requests, "  ");
        std::cerr << std::endl;
        return false;
      }
    }
  }
  catch(const eh::Exception& ex)
  {
    std::cerr << TEST_NAME << ": caught eh::Exception: " << ex.what() << std::endl;
    return false;
  }

  std::cout << TEST_NAME << ": success." << std::endl;
  return true;
}

int
main(int argc, char* argv[]) noexcept
{
  try
  {
    using namespace Generics::AppUtils;
    Args args;
    CheckOption opt_help;
    Generics::AppUtils::StringOption root_path(DEFAULT_ROOT_PATH);

    args.add(equal_name("path") || short_name("p"), root_path);
    args.add(equal_name("help") || short_name("h"), opt_help);

    args.parse(argc - 1, argv + 1);

    if (opt_help.enabled())
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    Logging::Logger_var logger(new Logging::Null::Logger);
    bool result = true;
    std::cout << "UserFraudProtectionContainerTest test started.." << std::endl;

    {
      system(("rm -r " + *root_path + TEST_FOLDER +
        " 2>/dev/null ; mkdir -p " + *root_path + TEST_FOLDER).c_str());

      TestProcessor_var test_processor(new TestProcessor());
      TestCallback_var test_callback(new TestCallback());

      AdServer::ProfilingCommons::ProfileMapFactory::Cache_var cache(
        new AdServer::ProfilingCommons::ProfileMapFactory::Cache(3));

      UserFraudProtectionContainer_var fraud_container(
        new UserFraudProtectionContainer(
          logger,
          test_processor,
          test_callback,
          (*root_path + TEST_FOLDER).c_str(),
          "Actions",
          cache,
          Generics::Time(10), // expire time (sec)
          Generics::Time(2)));

      /*
      result &= simple_test(fraud_container, test_processor);
      result &= reverse_order_test(fraud_container, test_processor);
      result &= strange_order_test(fraud_container, test_processor);
      result &= double_fraud_test(fraud_container, test_processor);
      result &= fraud_time_test(fraud_container, test_processor, test_callback);
      result &= fraud_non_first_pos_test(fraud_container, test_processor);
      result &= fraud_move_test(fraud_container, test_processor, test_callback);

      result &= fraud_deactivate_period_test(
        fraud_container, test_processor, test_callback);
      result &= fraud_reverse_deactivate_period_test(
        fraud_container, test_processor, test_callback);
      */
      result &= complex_deactivate_period_test(
        fraud_container, test_processor, test_callback);
    }

    return result ? 0 : 1;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }

  return -1;
}
