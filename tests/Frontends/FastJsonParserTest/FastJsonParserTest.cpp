#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <Commons/FastJsonParser.hpp>

#include "../../UnitTests/TestHelpers.hpp"

namespace
{
  struct Context
  {
    std::vector<std::string> calls;
    std::vector<std::string> strings;
    std::vector<int64_t> integers;
    std::vector<double> floats;
    std::vector<bool> bools;
    std::vector<std::string> nulls;
    std::size_t moved_strings = 0;
    std::size_t viewed_strings = 0;
  };

  struct ObjectArrayProcessor final:
    public AdServer::Commons::FastJsonParser::ValueProcessor
  {
    void
    object_started(std::string_view path, void* context) const override
    {
      static_cast<Context*>(context)->calls.emplace_back(
        "object:" + std::string(path));
    }

    void
    array_started(std::string_view path, void* context) const override
    {
      static_cast<Context*>(context)->calls.emplace_back(
        "array:" + std::string(path));
    }
  };

  struct StringProcessor final:
    public AdServer::Commons::FastJsonParser::ValueProcessor
  {
    void
    array_started(std::string_view, void*) const override
    {}

    void
    process_string(
      std::string_view value,
      std::string_view path,
      void* context) const override
    {
      auto* actual_context = static_cast<Context*>(context);
      actual_context->calls.emplace_back("view:" + std::string(path));
      actual_context->strings.emplace_back(value);
      ++actual_context->viewed_strings;
    }

    void
    process_string(
      std::string&& value,
      std::string_view path,
      void* context) const override
    {
      auto* actual_context = static_cast<Context*>(context);
      actual_context->calls.emplace_back("move:" + std::string(path));
      actual_context->strings.emplace_back(std::move(value));
      ++actual_context->moved_strings;
    }
  };

  struct DefaultStringProcessor final:
    public AdServer::Commons::FastJsonParser::ValueProcessor
  {
    void
    process_string(
      std::string&& value,
      std::string_view path,
      void* context) const override
    {
      auto* actual_context = static_cast<Context*>(context);
      actual_context->calls.emplace_back("default:" + std::string(path));
      actual_context->strings.emplace_back(std::move(value));
      ++actual_context->moved_strings;
    }
  };

  struct IntegerProcessor final:
    public AdServer::Commons::FastJsonParser::ValueProcessor
  {
    void
    process_integer(int64_t value, std::string_view path, void* context)
      const override
    {
      auto* actual_context = static_cast<Context*>(context);
      actual_context->calls.emplace_back("int:" + std::string(path));
      actual_context->integers.emplace_back(value);
    }
  };

  struct FloatProcessor final:
    public AdServer::Commons::FastJsonParser::ValueProcessor
  {
    void
    process_float(double value, std::string_view path, void* context)
      const override
    {
      auto* actual_context = static_cast<Context*>(context);
      actual_context->calls.emplace_back("float:" + std::string(path));
      actual_context->floats.emplace_back(value);
    }
  };

  struct BoolProcessor final:
    public AdServer::Commons::FastJsonParser::ValueProcessor
  {
    void
    process_bool(bool value, std::string_view path, void* context)
      const override
    {
      auto* actual_context = static_cast<Context*>(context);
      actual_context->calls.emplace_back("bool:" + std::string(path));
      actual_context->bools.emplace_back(value);
    }
  };

  struct NullProcessor final:
    public AdServer::Commons::FastJsonParser::ValueProcessor
  {
    void
    process_null(std::string_view path, void* context) const override
    {
      auto* actual_context = static_cast<Context*>(context);
      actual_context->calls.emplace_back("null:" + std::string(path));
      actual_context->nulls.emplace_back(path);
    }
  };

  struct UnexpectedArrayProcessor final:
    public AdServer::Commons::FastJsonParser::ValueProcessor
  {
    void
    object_started(std::string_view, void*) const override
    {}
  };
}

TEST(streaming_path_dispatch)
{
  AdServer::Commons::FastJsonParser parser;
  parser.add_processor("imp", std::make_shared<ObjectArrayProcessor>());
  parser.add_processor("imp.id", std::make_shared<StringProcessor>());
  parser.add_processor("imp.w", std::make_shared<IntegerProcessor>());
  parser.add_processor("imp.price", std::make_shared<FloatProcessor>());
  parser.add_processor("test", std::make_shared<BoolProcessor>());
  parser.add_processor("optional", std::make_shared<NullProcessor>());

  Context context;
  const std::string json =
    R"({"skip":{"value":"a\nb"},"imp":[{"id":"first","w":10,"price":1.25},)"
    R"({"id":"second","w":20}],"test":true,"optional":null})";

  parser.parse(json, &context);

  ASSERT_EQUALS(context.calls[0], std::string("array:imp"));
  ASSERT_EQUALS(context.calls[1], std::string("object:imp"));
  ASSERT_EQUALS(context.calls[2], std::string("view:imp.id"));
  ASSERT_EQUALS(context.calls[3], std::string("int:imp.w"));
  ASSERT_EQUALS(context.calls[4], std::string("float:imp.price"));
  ASSERT_EQUALS(context.calls[5], std::string("object:imp"));

  ASSERT_EQUALS(context.strings.size(), 2UL);
  ASSERT_EQUALS(context.strings[0], std::string("first"));
  ASSERT_EQUALS(context.strings[1], std::string("second"));

  ASSERT_EQUALS(context.integers.size(), 2UL);
  ASSERT_EQUALS(context.integers[0], 10);
  ASSERT_EQUALS(context.integers[1], 20);

  ASSERT_EQUALS(context.floats.size(), 1UL);
  ASSERT_EQUALS(context.floats[0], 1.25);

  ASSERT_EQUALS(context.bools.size(), 1UL);
  ASSERT_TRUE(context.bools[0]);

  ASSERT_EQUALS(context.nulls.size(), 1UL);
  ASSERT_EQUALS(context.nulls[0], std::string("optional"));
}

TEST(array_of_scalars)
{
  AdServer::Commons::FastJsonParser parser;
  parser.add_processor("cur", std::make_shared<StringProcessor>());

  Context context;
  parser.parse(R"({"cur":["RUB","USD"]})", &context);

  ASSERT_EQUALS(context.strings.size(), 2UL);
  ASSERT_EQUALS(context.strings[0], std::string("RUB"));
  ASSERT_EQUALS(context.strings[1], std::string("USD"));
}

TEST(string_unescape_is_lazy)
{
  AdServer::Commons::FastJsonParser parser;
  parser.add_processor("plain", std::make_shared<StringProcessor>());
  parser.add_processor("escaped", std::make_shared<StringProcessor>());

  Context context;
  std::string json =
    R"({"ignored":"this\nmust\u0020not\u0020be\u0020unescaped",)"
    R"("plain":"value","escaped":"a\n\u0042"})";
  const std::string original_json = json;

  parser.parse(json, &context);

  ASSERT_EQUALS(json, original_json);
  ASSERT_EQUALS(context.strings.size(), 2UL);
  ASSERT_EQUALS(context.strings[0], std::string("value"));
  ASSERT_EQUALS(context.strings[1], std::string("a\nB"));
  ASSERT_EQUALS(context.viewed_strings, 1UL);
  ASSERT_EQUALS(context.moved_strings, 1UL);
}

TEST(default_string_processor_copies_view)
{
  AdServer::Commons::FastJsonParser parser;
  parser.add_processor("id", std::make_shared<DefaultStringProcessor>());

  Context context;
  parser.parse(R"({"id":"plain"})", &context);

  ASSERT_EQUALS(context.strings.size(), 1UL);
  ASSERT_EQUALS(context.strings[0], std::string("plain"));
  ASSERT_EQUALS(context.moved_strings, 1UL);
}

TEST(as_string_passes_scalar_literals_without_conversion)
{
  AdServer::Commons::FastJsonParser parser;
  parser.add_processor("price", std::make_shared<StringProcessor>(), true);
  parser.add_processor("enabled", std::make_shared<StringProcessor>(), true);
  parser.add_processor("optional", std::make_shared<StringProcessor>(), true);

  Context context;
  parser.parse(R"({"price":1.2300,"enabled":false,"optional":null})", &context);

  ASSERT_EQUALS(context.strings.size(), 3UL);
  ASSERT_EQUALS(context.strings[0], std::string("1.2300"));
  ASSERT_EQUALS(context.strings[1], std::string("false"));
  ASSERT_EQUALS(context.strings[2], std::string("null"));
  ASSERT_TRUE(context.integers.empty());
  ASSERT_TRUE(context.floats.empty());
  ASSERT_TRUE(context.bools.empty());
  ASSERT_TRUE(context.nulls.empty());
}

TEST(skips_unregistered_subtrees)
{
  AdServer::Commons::FastJsonParser parser;
  parser.add_processor("id", std::make_shared<StringProcessor>());

  Context context;
  parser.parse(
    R"({"ignored":[{"a":"bad\n"},{"b":[1,2,3]}],"id":"ok"})",
    &context);

  ASSERT_EQUALS(context.strings.size(), 1UL);
  ASSERT_EQUALS(context.strings[0], std::string("ok"));
}

TEST(strict_skip_validates_unregistered_object)
{
  AdServer::Commons::FastJsonParser parser;
  parser.add_processor("id", std::make_shared<StringProcessor>());

  try
  {
    Context context;
    parser.parse(R"({"ignored":{"bad":"bad\q"},"id":"ok"})", &context);
    FAIL("strict skip must validate skipped objects");
  }
  catch(const AdServer::Commons::FastJsonParser::ParseError&)
  {}
}

TEST(strict_skip_validates_unregistered_array)
{
  AdServer::Commons::FastJsonParser parser;
  parser.add_processor("id", std::make_shared<StringProcessor>());

  try
  {
    Context context;
    parser.parse(R"({"ignored":["bad\q"],"id":"ok"})", &context);
    FAIL("strict skip must validate skipped arrays");
  }
  catch(const AdServer::Commons::FastJsonParser::ParseError&)
  {}
}

TEST(strict_skip_validates_unregistered_string)
{
  AdServer::Commons::FastJsonParser parser;
  parser.add_processor("id", std::make_shared<StringProcessor>());

  try
  {
    Context context;
    parser.parse(R"({"ignored":"bad\q","id":"ok"})", &context);
    FAIL("strict skip must validate skipped strings");
  }
  catch(const AdServer::Commons::FastJsonParser::ParseError&)
  {}
}

TEST(non_strict_skip_scans_object_to_closing_brace)
{
  AdServer::Commons::FastJsonParser parser(false);
  parser.add_processor("id", std::make_shared<StringProcessor>());

  Context context;
  parser.parse(
    R"({"ignored":{"bad":"bad\q","nested":{"text":"} not close"},)"
    R"("array":[{"v":1}]},"id":"ok"})",
    &context);

  ASSERT_EQUALS(context.strings.size(), 1UL);
  ASSERT_EQUALS(context.strings[0], std::string("ok"));
}

TEST(non_strict_skip_scans_array_to_closing_bracket)
{
  AdServer::Commons::FastJsonParser parser(false);
  parser.add_processor("id", std::make_shared<StringProcessor>());

  Context context;
  parser.parse(
    R"({"ignored":["bad\q", {"text":"] not close"}, [1,2,3]],"id":"ok"})",
    &context);

  ASSERT_EQUALS(context.strings.size(), 1UL);
  ASSERT_EQUALS(context.strings[0], std::string("ok"));
}

TEST(non_strict_skip_scans_string_to_unescaped_quote)
{
  AdServer::Commons::FastJsonParser parser(false);
  parser.add_processor("id", std::make_shared<StringProcessor>());

  Context context;
  parser.parse(R"({"ignored":"bad\q \" not close","id":"ok"})", &context);

  ASSERT_EQUALS(context.strings.size(), 1UL);
  ASSERT_EQUALS(context.strings[0], std::string("ok"));
}

TEST(parse_errors)
{
  AdServer::Commons::FastJsonParser parser;
  parser.add_processor("id", std::make_shared<StringProcessor>());

  try
  {
    Context context;
    parser.parse(R"(["not-object"])", &context);
    FAIL("top-level array must fail");
  }
  catch(const AdServer::Commons::FastJsonParser::ParseError&)
  {}

  try
  {
    Context context;
    parser.parse(R"({"id":"unterminated})", &context);
    FAIL("bad string must fail");
  }
  catch(const AdServer::Commons::FastJsonParser::ParseError&)
  {}
}

TEST(unexpected_type)
{
  AdServer::Commons::FastJsonParser parser;
  parser.add_processor("items", std::make_shared<UnexpectedArrayProcessor>());

  try
  {
    Context context;
    parser.parse(R"({"items":[]})", &context);
    FAIL("unexpected array must fail");
  }
  catch(const AdServer::Commons::FastJsonParser::UnexpectedType&)
  {}
}

RUN_TESTS
