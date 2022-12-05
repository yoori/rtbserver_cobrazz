#include "InvalidRedirectURLTest.hpp"

REFLECT_UNIT(InvalidRedirectURLTest) (
  "Frontend",
  AUTO_TEST_FAST
);

namespace
{
  const char *const invalid_url[] =
  {
    "http://test\r\nSet-Cookie: test=1",
    "http:// ",
    "http://www.google .com/",
  };
}


static
void
test_request(AutoTest::AdClient &client,
             AutoTest::BaseRequest &request,
             int status)
{
  client.process(request, true);

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      !client.has_cookie("test")),
    "cookie has been erroneously set");

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      status,
      client.req_status()).check(),
    "must got expected status");
}


bool
InvalidRedirectURLTest::run_test()
{
  AutoTest::AdClient client(AutoTest::AdClient::create_user(this));

  for (size_t i = 0; i < sizeof(invalid_url) / sizeof(*invalid_url); ++i)
  {
    NOSTOP_FAIL_CONTEXT(
      {
        add_descr_phrase("Sending OO request");

        AutoTest::OptOutRequest oo_request;
        oo_request.op("status");
        oo_request.opted_in_url(invalid_url[i]).
          opted_out_url(invalid_url[i]).
          opt_undef_url(invalid_url[i]);
        test_request(client, oo_request, 200);
      },
      invalid_url[i]);
  }

  return true;
}
