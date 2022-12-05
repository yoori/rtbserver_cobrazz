#include <arpa/inet.h>
#include <netinet/in.h>

#include "../../TestHelpers.hpp"

#include <CampaignSvcs/CampaignManager/SecToken.hpp>

using namespace AdServer::CampaignSvcs;

class DependencyContainerMock : public DependencyContainer
{
public:
  Generics::Time now;
  uint32_t random_value;

public:
  virtual Generics::Time
  get_time_of_day() const noexcept
  {
    return now;
  }

  virtual uint32_t
  safe_rand() const noexcept
  {
    return random_value;
  }

protected:
  virtual
  ~DependencyContainerMock() noexcept
  {}
};

typedef ReferenceCounting::SmartPtr<DependencyContainerMock>
  DependencyContainerMock_var;

TEST(get_text_token)
{
  SecTokenGenerator::Config config;
  config.aes_keys.push_back("MFBOeuplH3LlQqfGSvNiew==");
  config.aes_keys.push_back("gYi+K845gfiUkvx7Se1zKA==");
  config.aes_keys.push_back("mVa0GF8y1uvou3mktkNxqw==");
  config.aes_keys.push_back("mCFIshSwVu4VxRTi73Rf+Q==");
  config.aes_keys.push_back("fCW/qXof04/U/qRTggbJLw==");
  config.aes_keys.push_back("K2UPkfYBFNA+MxHKkV5OUA==");
  config.aes_keys.push_back("emcqWycuiztgJVng4az0Xw==");

  DependencyContainerMock_var dependency_container =
    new DependencyContainerMock();
  config.custom_dependency_container = dependency_container;
  dependency_container->now =
    Generics::ExtendedTime(2014, 1, 27, 13, 55, 58, 0);
  dependency_container->random_value = 16395523;

  SecTokenGenerator_var token_generator = new SecTokenGenerator(config, 0);

  {
    uint32_t addr;
    ASSERT_TRUE (inet_pton(AF_INET, "127.0.0.1", &addr) > 0);
    ASSERT_EQUALS (token_generator->get_text_token(addr), "RCDulVH4ge2UajX2mze8AAAB");

    ASSERT_TRUE (inet_pton(AF_INET, "127.0.0.2", &addr) > 0);
    ASSERT_FALSE (token_generator->get_text_token(addr) == "RCDulVH4ge2UajX2mze8AAAB");
  }
}

RUN_TESTS
