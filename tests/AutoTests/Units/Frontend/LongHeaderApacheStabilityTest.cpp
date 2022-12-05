#include "LongHeaderApacheStabilityTest.hpp"
#include <Generics/Rand.hpp>
#include <string>
#include <stdlib.h>
#include <time.h>

REFLECT_UNIT(LongHeaderApacheStabilityTest) (
  "Frontend",
  AUTO_TEST_QUIET
);

typedef AutoTest::NSLookupRequest NSLookupRequest;
typedef AutoTest::AdClient AdClient;


namespace
{
  const int NAME_SIZE = 100;
  const int VALUE_SIZE = 8000;
  const int HEADER_COUNT = 95;
  const int CLIENT_TIMEOUT = 10;
  const int PROBE_CLIENT_TIMEOUT = 10;
  static const char ALLOWED[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";

  class HeaderData
  {
  public:
    virtual ~HeaderData() {}
    virtual const char *name() = 0;
    virtual const char *value() = 0;

  protected:
    static void fill_random(std::string &str, size_t size);
  };

  void
  HeaderData::fill_random(std::string &str, size_t count)
  {
    str.reserve(str.size() + count);
    for (size_t i = 0; i < count; ++i)
    {
      // -1 to exclude tailing zero.
      size_t index = Generics::safe_rand(sizeof(ALLOWED) - 1);
      str.push_back(ALLOWED[index]);
    }
  }


  class RandomValueHeader : public HeaderData
  {
  public:
    RandomValueHeader(size_t value_size)
      : value_size_(value_size)
    {}

    const char *value()
    {
      value_.clear();
      fill_random(value_, value_size_);

      return value_.c_str();
    }

  private:
    size_t value_size_;
    std::string value_;
  };


  class RandomHeader : public RandomValueHeader
  {
  public:
    RandomHeader(size_t name_size, size_t value_size)
      : RandomValueHeader(value_size), name_size_(name_size)
    {}

    const char *name()
    {
      name_ = "X-";
      fill_random(name_, name_size_ - 2);

      return name_.c_str();
    }

  private:
    size_t name_size_;
    std::string name_;
  };


  class SameNameHeader : public RandomValueHeader
  {
  public:
    SameNameHeader(size_t value_size)
      : RandomValueHeader(value_size)
    {}

    const char *name()
    {
      return "X-SameName";
    }
  };


  class CookieHeader : public HeaderData
  {
  public:
    CookieHeader(size_t cookie_name_size, size_t value_size)
      : cookie_name_size_(cookie_name_size), value_size_(value_size)
    {}

    const char *name()
    {
      return "Cookie";
    }

    const char *value()
    {
      value_.clear();
      fill_random(value_, cookie_name_size_);
      value_ += "=";
      fill_random(value_, value_size_ - cookie_name_size_ - 1);

      return value_.c_str();
    }

  private:
    size_t cookie_name_size_;
    size_t value_size_;
    std::string value_;
  };


  class PatternHeader : public HeaderData
  {
  public:
    PatternHeader(const char *name, const char *pattern)
      : name_(name)
    {
      value_.reserve(VALUE_SIZE);
      const size_t length_threshold = VALUE_SIZE - strlen(pattern);
      while (value_.size() < length_threshold)
      {
        value_.append(pattern);
      }
    }

    const char *name()
    {
      return name_.c_str();
    }

    const char *value()
    {
      return value_.c_str();
    }

  private:
    std::string name_;
    std::string value_;
  };
}


static
void
scenario(AdClient &probe_client, AdClient &client, int expected_status)
{
  client.process(NSLookupRequest(), true);
  FAIL_CONTEXT(
    AutoTest::equal_checker(
      expected_status,
      client.req_status()).check(),
    "got unexpected status on request");

  if (client.req_time().tv_sec > CLIENT_TIMEOUT)
  {
    Stream::Error ostr;
    ostr << "request with long headers timeout="
      << CLIENT_TIMEOUT << ", actual request duration="
      << client.req_time();
    throw AutoTest::Exception(ostr.str());
  }

  probe_client.process(NSLookupRequest(), false);

  if (probe_client.req_time().tv_sec > PROBE_CLIENT_TIMEOUT)
  {
    Stream::Error ostr;
    ostr << "request after request with long headers timeout="
      << PROBE_CLIENT_TIMEOUT << ", actual request duration="
      << probe_client.req_time();
    throw AutoTest::Exception(ostr.str());
  }
}

bool
LongHeaderApacheStabilityTest::run_test()
{
  srand(time(0));

  add_descr_phrase("start test");

  AdClient probe_client(AdClient::create_user(this));

  NOSTOP_FAIL_CONTEXT(
    {
      add_descr_phrase("send long random header");

      AdClient client(AdClient::create_user(this));

      for (int i = 1; i <= 200; i += 10)
      {
        RandomHeader header(NAME_SIZE, 20 * 1024 * i);
        client.add_http_header(header.name(), header.value());
        scenario(probe_client, client, 400);
      }
    },
    "send long random header");

  NOSTOP_FAIL_CONTEXT(
    {
      add_descr_phrase("send many long random headers");

      AdClient client(AdClient::create_user(this));

      RandomHeader header(NAME_SIZE, VALUE_SIZE);

      for(int i = 1; i <= HEADER_COUNT; ++i)
      {
        for (int j = 0; j < i; ++j)
        {
          client.add_http_header(header.name(), header.value());
        }
        scenario(probe_client, client, 204);
      }
    },
    "send many long random headers");

  NOSTOP_FAIL_CONTEXT(
    {
      add_descr_phrase("send many headers with the same name");

      AdClient client(AdClient::create_user(this));
      
      SameNameHeader header(VALUE_SIZE);
      for(int i = 1; i <= HEADER_COUNT; ++i)
      {
        for (int j = 0; j < i; ++j)
        {
          client.add_http_header(header.name(), header.value());
        }
        scenario(probe_client, client, 204);
      }
    },
    "send many headers with the same name");

  NOSTOP_FAIL_CONTEXT(
    {
      add_descr_phrase("send many cookie headers");

      AdClient client(AdClient::create_user(this));

      CookieHeader header(NAME_SIZE, VALUE_SIZE);
      for(int i = 1; i <= HEADER_COUNT; ++i)
      {
        for (int j = 0; j < i; ++j)
        {
          client.add_http_header(header.name(), header.value());
        }
        scenario(probe_client, client, 204);
      }
    },
    "send many cookie headers");

  NOSTOP_FAIL_CONTEXT(
    {
      add_descr_phrase("send headers with ' ::'+ pattern");

      AdClient client(AdClient::create_user(this));

      PatternHeader header("X-Header", " ::");
      for(int i = 1; i <= HEADER_COUNT; ++i)
      {
        client.add_http_header(header.name(), header.value());
      }
      scenario(probe_client, client, 204);
    },
    "send headers with ' ::'+ pattern");

  NOSTOP_FAIL_CONTEXT(
    {
      add_descr_phrase("send referer-kw headers with ' a'+ pattern");


      AdClient client(AdClient::create_user(this));

      PatternHeader header("Referer-KW", " a");
      for(int i = 1; i <= HEADER_COUNT; ++i)
      {
        client.add_http_header(header.name(), header.value());
      }
      scenario(probe_client, client, 204);
    },
    "send referer-kw headers with ' a'+ pattern");

  NOSTOP_FAIL_CONTEXT(
    {
      add_descr_phrase("send context-kw headers with ' a'+ pattern");

      AdClient client(AdClient::create_user(this));

      PatternHeader header("Context-KW", " a");
      for(int i = 1; i <= HEADER_COUNT; ++i)
      {
        client.add_http_header(header.name(), header.value());
      }
      scenario(probe_client, client, 204);
    },
    "send context-kw headers with ' a'+ pattern");

  return true;
}
