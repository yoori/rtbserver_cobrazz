#ifndef GASON_HPP
#define GASON_HPP

#include <stdint.h>
#include <assert.h>
#include <String/StringManip.hpp>
#include "DecimalUtils.hpp"

#define JSON_ZONE_SIZE 4096
#define JSON_STACK_SIZE 32

struct JsonAllocator
{
  struct Zone
  {
    Zone *next;
    char *end;
  };

  Zone *head;

  JsonAllocator(): head() {};

  ~JsonAllocator();

  void *allocate(size_t n, size_t align = 8);
};

#define JSON_VALUE_PAYLOAD_MASK 0x00007FFFFFFFFFFFULL
#define JSON_VALUE_NAN_MASK 0x7FF8000000000000ULL
#define JSON_VALUE_NULL 0x7FFF800000000000ULL
#define JSON_VALUE_TAG_MASK 0xF
#define JSON_VALUE_TAG_SHIFT 47

enum JsonTag
{
  JSON_TAG_NUMBER,
  JSON_TAG_STRING,
  JSON_TAG_BOOL,
  JSON_TAG_ARRAY,
  JSON_TAG_OBJECT,
  JSON_TAG_NULL = 0xF
};

double
str2float(const char *str);

struct JsonNode;

struct JsonValue
{
  union
  {
    uint64_t i;
    double f;
  } data;

  JsonValue()
  {
    data.i = JSON_VALUE_NULL;
  }

  JsonValue(JsonTag tag, const void *p)
  {
    uint64_t x = (uint64_t)p;
    assert((int)tag <= JSON_VALUE_TAG_MASK);
    assert(x <= JSON_VALUE_PAYLOAD_MASK);
    data.i = JSON_VALUE_NAN_MASK | ((uint64_t)tag << JSON_VALUE_TAG_SHIFT) | x;
  }

  explicit JsonValue(double x)
  {
    data.f = x;
  }

  bool isDouble() const
  {
    return (int64_t)data.i <= (int64_t)JSON_VALUE_NAN_MASK;
  }

  JsonTag getTag() const
  {
    return isDouble() ? JSON_TAG_NUMBER : JsonTag((data.i >> JSON_VALUE_TAG_SHIFT) & JSON_VALUE_TAG_MASK);
  }

  uint64_t getPayload() const
  {
    assert(!isDouble());
    return data.i & JSON_VALUE_PAYLOAD_MASK;
  }

  double toNumber() const
  {
    assert(getTag() == JSON_TAG_NUMBER);
    return str2float(
      reinterpret_cast<const char*>(getPayload()));
  }

  template<typename DecimalType>
  DecimalType
  toDecimal(Generics::DecimalMulRemainder round_type = Generics::DMR_ROUND) const
  {
    assert(getTag() == JSON_TAG_NUMBER);
    return AdServer::Commons::extract_decimal<DecimalType>(
      String::SubString(reinterpret_cast<const char*>(getPayload())), round_type);
  }

  bool toBool() const
  {
    assert(getTag() == JSON_TAG_BOOL);
    return (bool)getPayload();
  }

  std::string toString() const
  {
    std::string res;
    toString(res);
    return res;
  }

  void toString(
    std::string& str,
    bool normalize_integer_number = false)
    const
  {
    assert(getTag() == JSON_TAG_STRING || getTag() == JSON_TAG_NUMBER);
    if(getTag() == JSON_TAG_STRING)
    {
      str = reinterpret_cast<const char*>(getPayload());
    }
    else
    {
      const char* payload = reinterpret_cast<const char*>(getPayload());
      const char* payload_end = AdServer::Commons::find_end_of_number(payload);

      if(!normalize_integer_number)
      {
        str.assign(payload, payload_end);
      }
      else
      {
        float val = toNumber();
        float round_val = ::roundf(val);
        bool parsed = false;

        if(round_val == val)
        {
          if(round_val >= std::numeric_limits<unsigned long long>::min() &&
            round_val < std::numeric_limits<unsigned long long>::max())
          {
            char value_str[32];
            size_t value_str_size = String::StringManip::int_to_str(
              static_cast<unsigned long long>(round_val),
              value_str,
              sizeof(value_str));
            str.assign(value_str, value_str_size);

            parsed = true;
          }
          else if(round_val >= std::numeric_limits<long long>::min() &&
            round_val < std::numeric_limits<long long>::max())
          {
            char value_str[32];
            size_t value_str_size = String::StringManip::int_to_str(
              static_cast<long long>(round_val),
              value_str,
              sizeof(value_str));
            str.assign(value_str, value_str_size);

            parsed = true;
          }
        }

        if(!parsed)
        {
          str.assign(payload, payload_end);
        }
      }
    }
  }

  JsonNode *toNode() const
  {
    assert(getTag() == JSON_TAG_ARRAY || getTag() == JSON_TAG_OBJECT);
    return (JsonNode *)getPayload();
  }

  // FIXME: ensure toSubString safety
  String::SubString toSubString() const
  {
    String::SubString res;
    toSubString(res);
    return res;
  }

  void toSubString(String::SubString& str) const
  {
    assert(getTag() == JSON_TAG_STRING || getTag() == JSON_TAG_NUMBER);
    if(getTag() == JSON_TAG_STRING)
    {
      str = String::SubString(reinterpret_cast<const char *>(getPayload()));
    }
    else if (getTag() == JSON_TAG_NUMBER)
    {
      const char* payload = reinterpret_cast<const char*>(getPayload());
      str = String::SubString(payload, AdServer::Commons::find_end_of_number(payload));
    }
    // boolean?
  }

};

struct JsonNode
{
  JsonValue value;
  JsonNode *next;
  char *key;
};

struct JsonIterator
{
  JsonIterator(JsonNode* node)
    : p(node)
  {}

  JsonNode *p;

  void operator++() { p = p->next; }

  bool operator!=(const JsonIterator &x) const { return p != x.p; }

  JsonNode *operator*() const { return p; }

  JsonNode *operator->() const { return p; }
};

inline JsonIterator begin(JsonValue o) { return JsonIterator(o.toNode()); }
inline JsonIterator end(JsonValue) { return JsonIterator(0); }

enum JsonParseStatus
{
  JSON_PARSE_OK,
  JSON_PARSE_BAD_NUMBER,
  JSON_PARSE_BAD_STRING,
  JSON_PARSE_BAD_IDENTIFIER,
  JSON_PARSE_STACK_OVERFLOW,
  JSON_PARSE_STACK_UNDERFLOW,
  JSON_PARSE_MISMATCH_BRACKET,
  JSON_PARSE_UNEXPECTED_CHARACTER,
  JSON_PARSE_UNQUOTED_KEY,
  JSON_PARSE_BREAKING_BAD
};

JsonParseStatus
json_parse(char *str, char **endptr, JsonValue *value, JsonAllocator &allocator);

std::string
json_parse_error(JsonParseStatus status);

bool is_space(char c);
bool is_delim(char c);
bool is_sign(char c);
bool is_dec(char c);
bool is_hex(char c);

#endif /*GASON_HPP*/
