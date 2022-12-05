
#include "TagPassbackChecker.hpp"
#include <tests/AutoTests/Commons/Traits.hpp>
#include <Stream/MemoryStream.hpp>

namespace AutoTest
{
  TagPassbackChecker::Tokens::Tokens() :
    type_(),
    url_(),
    pixel_()
  {}
  
  // Mapping, based on pb.unit-test(-imp) template format
  const TagPassbackChecker::PassbackToken
    TagPassbackChecker::TOKENS[TagPassbackChecker::TOKEN_COUNT] =
  {
    {
      "PASSBACK_TYPE",
      &TagPassbackChecker::Tokens::type,
      &TagPassbackChecker::Tokens::type
    },
    {
      "PASSBACK_URL",
      &TagPassbackChecker::Tokens::url,
      &TagPassbackChecker::Tokens::url
    },
    {
      "PASSBACK_PIXEL",
      &TagPassbackChecker::Tokens::pixel,
      &TagPassbackChecker::Tokens::pixel,
    },
  };
  
  
  TagPassbackChecker::TagPassbackChecker(
    const AdClient& client,
    const std::string& url) : 
    client_(client)
  {
    expected_.url(url);
  }

  TagPassbackChecker::TagPassbackChecker(
    const AdClient& client,
    const Tokens& expected) : 
    client_(client),
    expected_(expected)
  {}

  TagPassbackChecker::~TagPassbackChecker() noexcept
  {}

  bool
  TagPassbackChecker::check(bool throw_error)
    /*throw(eh::Exception)*/
  {
    Stream::Parser body(client_.req_response_data());
    size_t i = 0;
    bool result = true;
    Stream::Error error;
    while( !body.eof())
    {
      char got[MAX_TOKEN_SIZE];
      body.getline(got, MAX_TOKEN_SIZE);

      (got_.*(TOKENS[i].set))(got);

      if (i < TOKEN_COUNT)
      {
        std::string exp((expected_.*(TOKENS[i].get))());
       
        result = exp.empty() ||
          equal(exp, (got_.*(TOKENS[i].get))());
      
        if (!result)
        {
          error << "Invalid passback body (" << TOKENS[i].name <<
            "): expected '" << exp << "', got '" << got << "'";
          break;
        }
      }
      i++;
    }
    if (result && i != TOKEN_COUNT)
    {
      error << "Invalid passback body format '" <<
        client_.req_response_data() << "': expected " <<
        TOKEN_COUNT << " lines, got " << i;
      result = false;
    }
    if (!result && throw_error)
    {
      throw AutoTest::CheckFailed(error);
    }
    return result;
  }
}
