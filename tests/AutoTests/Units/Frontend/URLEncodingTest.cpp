#include "URLEncodingTest.hpp"

REFLECT_UNIT(URLEncodingTest) (
  "Frontend",
  AUTO_TEST_FAST);

namespace
{
  typedef AutoTest::AdClient AdClient;
  typedef AutoTest::OptOutRequest OptOutRequest;
  typedef AutoTest::TagPassbackChecker TagPassbackChecker;
  typedef AutoTest::RedirectChecker RedirectChecker;
  typedef AutoTest::ClickResponseChecker ClickResponseChecker;

  struct OORequest
  {
    const char* op;
    const std::string url;
    const std::string expected_url;
    unsigned int status;
  };
}

bool
URLEncodingTest::run_test()
{
  add_descr_phrase("Passback frontend.");
  
  NOSTOP_FAIL_CONTEXT(
    passback_encoding(
      "SIMPLE_TAG",
      "PASSBACK_URL1",
      &NSLookupRequest::passback,
      true,
      "PASSBACK_URL1_EXP"));

  // It isn't valid business case, may be.
  // Passback URL come from DC and
  // should have valid encoding (IDNA 2003).
  NOSTOP_FAIL_CONTEXT(
    passback_encoding(
      "SIMPLE_TAG",
      "PASSBACK_URL2",
      &NSLookupRequest::passback,
      true,
      "PASSBACK_URL2_IDNA_EXP"));

  NOSTOP_FAIL_CONTEXT(
    passback_encoding(
      "SIMPLE_TAG",
      "PASSBACK_URL2",
      &NSLookupRequest::orig,
      false,
      "PASSBACK_URL2_IDNA_EXP"));

  NOSTOP_FAIL_CONTEXT(
    tag_passback_encoding(
      "ENPASSBACK_TAG",
      "PASSBACK_URL1_EXP"));

  NOSTOP_FAIL_CONTEXT(
    tag_passback_encoding(
      "RUPASSBACK_TAG",
      "PASSBACK_URL2_IDNA_EXP"));

  NOSTOP_FAIL_CONTEXT(clickurl_encoding());
  NOSTOP_FAIL_CONTEXT(clickurl_relocate_encoding());
  
  add_descr_phrase("Click frontend - preclick.");

  NOSTOP_FAIL_CONTEXT(
    clickurl_preclick_encoding(
      "PASSBACK_URL1",
      "PASSBACK_URL1_EXP"));

  NOSTOP_FAIL_CONTEXT(
    clickurl_preclick_encoding(
      "PASSBACK_URL2",
      "PASSBACK_URL2_IDNA_EXP"));
      
  NOSTOP_FAIL_CONTEXT(optout_redirect());

  return true;
}

void
URLEncodingTest::passback_encoding(
  const char* tag,
  const char* passback,
  NSLookupRequest::Member param,
  bool with_frontend,
  const char* expected_passback)
{
  NSLookupRequest request;
  request.tid = fetch_string(tag);
  request.pt = "redir";
  if (passback)
  {
    param(request, fetch_string(passback));
  }

  AdClient client(AdClient::create_user(this));
  client.process_request(request);

  std::string passback_request;

  FAIL_CONTEXT(
    AutoTest::predicate_checker(
      client.find_header_value("Location", passback_request)),
    "Client must be redirected to passback frontend");

  if (with_frontend)
  {
    client.process_request(passback_request);

    FAIL_CONTEXT(
      AutoTest::predicate_checker(
        client.find_header_value("Location", passback_request)),
        "Client must be redirected to passback");
  }

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      fetch_string(expected_passback),
      passback_request).check(),
    "Client must be redirected to original passback URL");
}

void
URLEncodingTest::tag_passback_encoding(
  const char* tag,
  const char* expected_passback)
{
  NSLookupRequest request;
  request.tid = fetch_string(tag);

  AdClient client(AdClient::create_user(this));
  client.process_request(request);

  FAIL_CONTEXT(
    AutoTest::equal_checker(
      "0",
      client.debug_info.ccid).check(),
    "Check ccid empty");

  FAIL_CONTEXT(
    TagPassbackChecker(
      client,
      fetch_string(expected_passback)).check(),
    "Check passback");
}

void
URLEncodingTest::clickurl_encoding()
{
  add_descr_phrase("Click url encoding.");
  std::string keyword   = fetch_string("KEYWORD1");
  std::string idna_click_url = fetch_string("PASSBACK_URL2_IDNA_EXP");

  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("CLICK1_TAG");

  request.referer_kw = keyword;
  client.process_request(request);
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      1,
      client.debug_info.selected_creatives.size()).check(),
    "must select creative");
  std::string url_to_click = client.debug_info.selected_creatives.first().click_url;
  client.process_request(url_to_click, "click on keyword click_url");

  FAIL_CONTEXT(
    ClickResponseChecker(
      client,
      "",
      idna_click_url).check(),
    "Check click response");
}

void
URLEncodingTest::clickurl_relocate_encoding()
{
  add_descr_phrase("Click frontend - relocate.");
  std::string keyword   = fetch_string("KEYWORD2");

  AdClient client(AdClient::create_user(this));
  
  NSLookupRequest request;
  request.tid         = fetch_string("CLICK2_TAG");

  request.referer_kw = keyword;
  client.process_request(request);
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      1,
      client.debug_info.selected_creatives.size()).check(),
    "must select creative");
  std::string url_to_click = client.debug_info.selected_creatives.first().click_url;
  {
    client.process_request(url_to_click +
    "*amp*relocate*eql*" + fetch_string("PASSBACK_URL1"));

    FAIL_CONTEXT(
    ClickResponseChecker(
      client,
      "",
      fetch_string("PASSBACK_URL1_EXP")).check(),
    "Check click response");
  }
  {

    std::string url;
    String::StringManip::mime_url_encode(
      fetch_string("PASSBACK_URL2"), url);

    
    client.process_request(url_to_click +
    "*amp*relocate*eql*" + url);

    FAIL_CONTEXT(
    ClickResponseChecker(
      client,
      "",
      fetch_string("PASSBACK_URL2_IDNA_EXP")).check(),
    "Check click response");
  }
  {
    // Deprecated behavior
    // Server doesn't decode URLs without 'http://' prefix

    std::string url;
    String::StringManip::mime_url_encode(
      fetch_string("PASSBACK_URL3"), url);
    
    client.process_request(url_to_click +
    "*amp*relocate*eql*" + url);

    FAIL_CONTEXT(
      ClickResponseChecker(
        client,
        "",
        fetch_string("CRCLICK_DEFAULT")).check(),
    "Check click response");

  }

  {
    std::string url;
    String::StringManip::mime_url_encode(
      fetch_string("PASSBACK_URL4"), url);

    client.process_request(url_to_click +
    "*amp*relocate*eql*" + url);

    FAIL_CONTEXT(
      ClickResponseChecker(
        client,
        "",
        fetch_string("PASSBACK_URL4_EXP")).check(),
    "Check click response");

  }
  
}

void
URLEncodingTest::clickurl_preclick_encoding(
  const char* preclick,
  const char* expected_preclick)
{
  std::string keyword   = fetch_string("KEYWORD2");

  AdClient client(AdClient::create_user(this));

  NSLookupRequest request;
  request.tid         = fetch_string("CLICK2_TAG");
  request.referer_kw = keyword;
  request.preclick = fetch_string(preclick);
    
  client.process_request(request);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      1,
      client.debug_info.selected_creatives.size()).check(),
    "must select creative");
  std::string url_to_click = client.debug_info.selected_creatives.first().click_url;
  client.process_request(url_to_click);

  FAIL_CONTEXT(
    ClickResponseChecker(
      client,
      fetch_string(expected_preclick),
      fetch_string("CRCLICK_DEFAULT")).check(),
    "Check click response");
}

void
URLEncodingTest::optout_redirect()
{
  add_descr_phrase("Optout frontend - redirect URLs.");
  AdClient client(AdClient::create_nonoptin_user(this));

  std::string idna_url = fetch_string("PASSBACK_URL2_IDNA_EXP");
  std::string non_ascii_url = fetch_string("PASSBACK_URL2");
  std::string path_url(
    "http://cs.ocslab.com/cgi-bin/attest.cgi?actionid=123&cong=opt-out frontend");
  std::string path_url_mime(
    "http://cs.ocslab.com/cgi-bin/attest.cgi?actionid=123&cong=opt-out%20frontend");
    
  const OORequest REQUESTS[] =
  {
    { "in", path_url, path_url_mime, 302},
    { "in", non_ascii_url, idna_url, 302},
    { "status", idna_url, idna_url, 302},
    { "abc", "NOT_AN_URL", "", 400},
    { "in", "about:blank", "", 200}
  };

  for (size_t i = 0; i < countof(REQUESTS); ++i)
  {

    client.process(
      OptOutRequest().
        op(REQUESTS[i].op).
        opted_in_url(REQUESTS[i].url).
        opted_out_url(REQUESTS[i].url).
        opt_undef_url(REQUESTS[i].url).
        success_url(REQUESTS[i].url).
        fail_url(REQUESTS[i].url).
        already_url(REQUESTS[i].url), true);

    FAIL_CONTEXT(
        AutoTest::equal_checker(
          REQUESTS[i].status,
          client.req_status()).check(),
        "Check status#" + strof(i+1));

    if (!REQUESTS[i].expected_url.empty())
    {
      FAIL_CONTEXT(
        RedirectChecker(
          client,
          REQUESTS[i].expected_url).check(),
        "Check redirect#" + strof(i+1));
    }
  }

}
