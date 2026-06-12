#include <stdlib.h>
#include "Gason.hpp"

namespace
{
  inline bool
  json_is_space(char c)
  {
    return c == ' ' || static_cast<unsigned>(c - '\t') <= ('\r' - '\t');
  }

  inline bool
  json_is_delim(char c)
  {
    return c == '\0' || c == ',' || c == ':' || c == ']' || c == '}';
  }

  inline bool
  json_is_sign(char c)
  {
    return c == '+' || c == '-';
  }

  inline bool
  json_is_dec(char c)
  {
    return static_cast<unsigned>(c - '0') <= 9;
  }

  inline bool
  json_is_hex(char c)
  {
    return json_is_dec(c) ||
      static_cast<unsigned>((c | 0x20) - 'a') <= ('f' - 'a');
  }

  inline char*
  json_consume_float(char* str)
  {
    if (json_is_sign(*str))
    {
      ++str;
    }

    while (json_is_dec(*str))
    {
      ++str;
    }

    if (*str == '.')
    {
      ++str;

      while (json_is_dec(*str))
      {
        ++str;
      }
    }

    if (*str == 'e' || *str == 'E')
    {
      ++str;

      if (json_is_sign(*str))
      {
        ++str;
      }

      while (json_is_dec(*str))
      {
        ++str;
      }
    }

    return str;
  }
}

bool is_space(char c) { return json_is_space(c); }
bool is_delim(char c) { return json_is_delim(c); }
bool is_sign(char c) { return json_is_sign(c); }
bool is_dec(char c) { return json_is_dec(c); }
bool is_hex(char c) { return json_is_hex(c); }

inline int char2int(char c)
{
  if (c >= 'a') return c - 'a' + 10;
  if (c >= 'A') return c - 'A' + 10;
  return c - '0';
}

void
consume_float(
  char *str,
  char **endptr)
{
  *endptr = json_consume_float(str);
}

double
str2float(const char* str)
{
  double sign = json_is_sign(*str) && *str++ == '-' ? -1 : 1;
  double result = 0;
  while (json_is_dec(*str))
  {
    result = (result * 10) + (*str++ - '0');
  }

  if (*str == '.')
  {
    ++str;
    double fraction = 1;
    while (json_is_dec(*str))
    {
      fraction *= 0.1;
      result += (*str++ - '0') * fraction;
    }
  }
  if (*str == 'e' || *str == 'E')
  {
    ++str;
    double base = json_is_sign(*str) && *str++ == '-' ? 0.1 : 10;
    int exponent = 0;
    while (json_is_dec(*str))
    {
      exponent = (exponent * 10) + (*str++ - '0');
    }
    double power = 1;
    for (; exponent; exponent >>= 1, base *= base)
    {
      if (exponent & 1)
      {
        power *= base;
      }
    }
    result *= power;
  }

  return sign * result;
}

JsonAllocator::~JsonAllocator()
{
  while (head)
  {
    Zone *temp = head->next;
    free(head);
    head = temp;
  }
}

inline void *align_pointer(void *x, size_t align)
{
  return (void *)(((uintptr_t)x + (align - 1)) & ~(align - 1));
}

namespace
{
  inline void*
  json_allocate(JsonAllocator& allocator, size_t n, size_t align)
  {
    JsonAllocator::Zone* head = allocator.head;
    if (head)
    {
      char *p = (char *)align_pointer(head->end, align);
      if (p + n <= (char *)head + JSON_ZONE_SIZE)
      {
        head->end = p + n;
        return p;
      }
    }
    size_t zone_size = sizeof(JsonAllocator::Zone) + n + align;
    JsonAllocator::Zone *z = (JsonAllocator::Zone *)malloc(
      zone_size <= JSON_ZONE_SIZE ? JSON_ZONE_SIZE : zone_size);
    char *p = (char *)align_pointer(z + 1, align);
    z->end = p + n;
    if (zone_size <= JSON_ZONE_SIZE || !head)
    {
      z->next = head;
      allocator.head = z;
    }
    else
    {
      z->next = head->next;
      head->next = z;
    }
    return p;
  }
}

void *JsonAllocator::allocate(size_t n, size_t align)
{
  return json_allocate(*this, n, align);
}

struct JsonList
{
  JsonList()
  {}

  JsonList(JsonTag tag_val, const JsonValue& node_val, char* key_val)
    : tag(tag_val), node(node_val), key(key_val)
  {}

  JsonTag tag;
  JsonValue node;
  char *key;

  void grow_the_tail(JsonNode *p)
  {
    JsonNode *tail = (JsonNode *)node.getPayload();
    if (tail)
    {
      p->next = tail->next;
      tail->next = p;
    }
    else
    {
      p->next = p;
    }
    node = JsonValue(tag, p);
  }

  JsonValue cut_the_head()
  {
    JsonNode *tail = (JsonNode *)node.getPayload();
    if (tail)
    {
      JsonNode *head = tail->next;
      tail->next = 0;
      return JsonValue(tag, head);
    }
    return node;
  }
};

JsonParseStatus
json_parse(char *str, char **endptr, JsonValue *value, JsonAllocator &allocator)
{
  JsonList stack[JSON_STACK_SIZE];
  int top = -1;
  bool separator = true;
  while (*str)
  {
    JsonValue o;
    while (json_is_space(*str)) ++str;
    *endptr = str++;

    switch (**endptr)
    {
      case '\0':
        continue;
      case '"':
        o = JsonValue(JSON_TAG_STRING, str);
        for (;; ++str)
        {
          const char c = *str;
          if (c == '"')
          {
            *str++ = 0;
            break;
          }
          if (c == '\\')
          {
            break;
          }
          if (c == '\0')
          {
            return *endptr = str, JSON_PARSE_BAD_STRING;
          }
        }

        if (*str == '\\')
        {
          char* s = str;
          for (;; ++s, ++str)
          {
            int c = *str;
            if (c == '\\')
            {
              c = *++str;
              switch (c)
              {
                case '\\':
                case '"':
                case '/': *s = c; break;
                case 'b': *s = '\b'; break;
                case 'f': *s = '\f'; break;
                case 'n': *s = '\n'; break;
                case 'r': *s = '\r'; break;
                case 't': *s = '\t'; break;
                case 'u':
                  c = 0;
                  for (int i = 0; i < 4; ++i)
                  {
                    if (!json_is_hex(*++str))
                    {
                      return *endptr = str, JSON_PARSE_BAD_STRING;
                    }
                    c = c * 16 + char2int(*str);
                  }
                  if (c < 0x80)
                  {
                    *s = c;
                  }
                  else if (c < 0x800)
                  {
                    *s++ = 0xC0 | (c >> 6);
                    *s = 0x80 | (c & 0x3F);
                  }
                  else
                  {
                    *s++ = 0xE0 | (c >> 12);
                    *s++ = 0x80 | ((c >> 6) & 0x3F);
                    *s = 0x80 | (c & 0x3F);
                  }
                  break;
                default:
                  return *endptr = str, JSON_PARSE_BAD_STRING;
              }
            }
            else if (c == '"')
            {
              *s = 0;
              ++str;
              break;
            }
            else if (c == '\0')
            {
              return *endptr = str, JSON_PARSE_BAD_STRING;
            }
            else
            {
              *s = c;
            }
          }
        }
        if (!json_is_delim(*str)) return *endptr = str, JSON_PARSE_BAD_STRING;
        break;
      case ',':
        // allow few separators in series and after object or array beginning
        //if (/*separator || */stack[top].key != 0) return JSON_PARSE_UNEXPECTED_CHARACTER;
        separator = true;
        continue;
      case ':':
        if (separator || stack[top].key == 0) return JSON_PARSE_UNEXPECTED_CHARACTER;
        separator = true;
        continue;
      case '{':
        if (++top == JSON_STACK_SIZE) return JSON_PARSE_STACK_OVERFLOW;
        stack[top] = JsonList(JSON_TAG_OBJECT, JsonValue(JSON_TAG_OBJECT, 0), 0);
        continue;
      case '[':
        if (++top == JSON_STACK_SIZE) return JSON_PARSE_STACK_OVERFLOW;
        stack[top] = JsonList(JSON_TAG_ARRAY, JsonValue(JSON_TAG_ARRAY, 0), 0);
        continue;
      case '}':
        if (top == -1) return JSON_PARSE_STACK_UNDERFLOW;
        if (stack[top].tag != JSON_TAG_OBJECT) return JSON_PARSE_MISMATCH_BRACKET;
        o = stack[top--].cut_the_head();
        break;
      case ']':
        if (top == -1) return JSON_PARSE_STACK_UNDERFLOW;
        if (stack[top].tag != JSON_TAG_ARRAY) return JSON_PARSE_MISMATCH_BRACKET;
        o = stack[top--].cut_the_head();
        break;
      case 'f':
        for (const char *s = "alse"; *s; ++s, ++str)
        {
          if (*s != *str) return JSON_PARSE_BAD_IDENTIFIER;
        }
        if (!json_is_delim(*str)) return JSON_PARSE_BAD_IDENTIFIER;
        o = JsonValue(JSON_TAG_BOOL, (void *)false);
        break;
      case 't':
        for (const char *s = "rue"; *s; ++s, ++str)
        {
          if (*s != *str) return JSON_PARSE_BAD_IDENTIFIER;
        }
        if (!json_is_delim(*str)) return JSON_PARSE_BAD_IDENTIFIER;
        o = JsonValue(JSON_TAG_BOOL, (void *)true);
        break;
      case 'n':
        for (const char *s = "ull"; *s; ++s, ++str)
        {
          if (*s != *str) return JSON_PARSE_BAD_IDENTIFIER;
        }
        if (!json_is_delim(*str)) return JSON_PARSE_BAD_IDENTIFIER;
        break;
      default:
        if (**endptr == '-' || json_is_dec(**endptr))
        {
          if (**endptr == '-' && !json_is_dec(*str) && *str != '.')
          {
            return *endptr = str, JSON_PARSE_BAD_NUMBER;
          }

          o = JsonValue(JSON_TAG_NUMBER, *endptr);
          str = json_consume_float(*endptr);
          if (!json_is_delim(*str))
          {
            return *endptr = str, JSON_PARSE_BAD_NUMBER;
          }
          break;
        }

        return JSON_PARSE_UNEXPECTED_CHARACTER;
    }

    separator = false;

    if (top == -1)
    {
      *endptr = str;
      *value = o;
      return JSON_PARSE_OK;
    }

    if (stack[top].tag == JSON_TAG_OBJECT)
    {
      if (!stack[top].key)
      {
        if (o.getTag() != JSON_TAG_STRING) return JSON_PARSE_UNQUOTED_KEY;
        stack[top].key = reinterpret_cast<char*>(o.getPayload());
        continue;
      }
      JsonNode *p = (JsonNode *)json_allocate(
        allocator,
        sizeof(JsonNode),
        8);
      p->value = o;
      p->key = stack[top].key;
      stack[top].key = 0;
      stack[top].grow_the_tail((JsonNode *)p);
      continue;
    }

    JsonNode *p = (JsonNode *)json_allocate(
      allocator,
      sizeof(JsonNode) - sizeof(char *),
      8);
    p->value = o;
    stack[top].grow_the_tail(p);
  }
  return JSON_PARSE_BREAKING_BAD;
}

std::string
json_parse_error(JsonParseStatus status)
{
  switch(status)
  {
  case JSON_PARSE_OK:
    return "ok";

  case JSON_PARSE_BAD_NUMBER :
    return "bad number";

  case JSON_PARSE_BAD_STRING :
    return "bad string";

  case JSON_PARSE_BAD_IDENTIFIER :
    return "bad identifier";

  case JSON_PARSE_STACK_OVERFLOW :
    return "stack overflow";

  case JSON_PARSE_STACK_UNDERFLOW :
    return "stack underflow";

  case JSON_PARSE_MISMATCH_BRACKET :
    return "mismatch bracket";

  case JSON_PARSE_UNEXPECTED_CHARACTER :
    return "unexpected character";

  case JSON_PARSE_UNQUOTED_KEY :
    return "unquoted key";

  case JSON_PARSE_BREAKING_BAD :
    return "breaking bad";

  };

  return "unknown";
}
