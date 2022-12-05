
#include "RedirectChecker.hpp"
#include <String/StringManip.hpp>
#include <Stream/MemoryStream.hpp>

namespace AutoTest
{
  // RedirectChecker class
  RedirectChecker::RedirectChecker(
    AdClient& client,
    const std::string& location) :
    client_(client)
  {
    std::string regex;
    String::StringManip::mark(location.c_str(), regex,
      String::AsciiStringManip::REGEX_META, '\\');
    regex_.set_expression("^" + regex + "$");
  }

  RedirectChecker::RedirectChecker(
    AdClient& client,
    const String::RegEx& regex) :
    client_(client),
    regex_(regex)
  {}

  RedirectChecker::~RedirectChecker() noexcept
  {}

  bool
  RedirectChecker::check(bool throw_error)
    /*throw(eh::Exception)*/
  {
    if (
      client_.find_header_value("Location", location_) &&
      client_.req_status() == 302 &&
      regex_.match(location_))
    {
      return true;
    }
    if (throw_error)
    {
      Stream::Error error;
      error << "Invalid redirect, got: status=" << client_.req_status() <<
        ", location='" << location_ << "' (expected: status=302, location regex='" <<
        regex_.expression().data() << "')";
      throw AutoTest::CheckFailed(error);
    }
    return false;
  }
}
