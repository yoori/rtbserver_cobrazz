#include "ForbiddenUserAgentsTest.hpp"

REFLECT_UNIT(ForbiddenUserAgentsTest) (
  "Frontend",
  AUTO_TEST_FAST
);

namespace
{
  typedef AutoTest::NSLookupRequest NSLookupRequest;
  typedef AutoTest::AdClient AdClient;
}

bool 
ForbiddenUserAgentsTest::run_test()
{
  const Locals locals = get_local_params();
  const size_t locals_size = locals.DataElem().size();
  
  AdClient client(AdClient::create_nonoptin_user(this));
  NSLookupRequest request;
  {
    request.user_agent = "Mozilla/5.0 (Windows; U; Windows NT 5.1;)";
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        204,
        client.process(request, true)).check(),
      "must process valid user agent: "
      "Mozilla/5.0 (Windows; U; Windows NT 5.1;)");
  }
  for (size_t i = 2; i < locals_size; ++i)
  {
    const std::string &name  = locals.DataElem()[i].Name();
    const std::string &value = locals.DataElem()[i].Value();

    if (name.find("Agent") != std::string::npos)
    {
      request.user_agent = value;
    FAIL_CONTEXT(
      AutoTest::equal_checker(
        403,
        client.process(request, true)).check(),
      "must got forbidden status for agent: " + value);
    }
  }
  return true;
}
 
