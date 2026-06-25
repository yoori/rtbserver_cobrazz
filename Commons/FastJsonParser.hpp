#pragma once

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <eh/Exception.hpp>

namespace AdServer::Commons
{
  class FastJsonParser
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(ParseError, Exception);
    DECLARE_EXCEPTION(UnexpectedType, Exception);

    struct ValueProcessor
    {
      virtual
      ~ValueProcessor() noexcept = default;

      virtual void
      object_started(std::string_view path, void* context) const;

      virtual void
      array_started(std::string_view path, void* context) const;

      virtual void
      process_integer(
        int64_t value,
        std::string_view path,
        void* context) const;

      virtual void
      process_float(
        double value,
        std::string_view path,
        void* context) const;

      virtual void
      process_string(
        std::string_view value,
        std::string_view path,
        void* context) const;

      virtual void
      process_string(
        std::string&& value,
        std::string_view path,
        void* context) const;

      virtual void
      process_bool(
        bool value,
        std::string_view path,
        void* context) const;

      virtual void
      process_null(std::string_view path, void* context) const;
    };

  private:
    struct JsonTreeProcessor
    {
      JsonTreeProcessor() = default;

      JsonTreeProcessor(JsonTreeProcessor* parent_val, std::string path_val);

      std::list<std::string> key_holder;
      std::unordered_map<
        std::string_view,
        std::unique_ptr<JsonTreeProcessor> > sub_processors;
      std::shared_ptr<ValueProcessor> value_processor;
      JsonTreeProcessor* parent = nullptr;
      std::string path;
    };

  public:
    explicit
    FastJsonParser(bool strict = true);

    FastJsonParser(const FastJsonParser&) = delete;
    FastJsonParser& operator=(const FastJsonParser&) = delete;
    FastJsonParser(FastJsonParser&&) = delete;
    FastJsonParser& operator=(FastJsonParser&&) = delete;

    void
    add_processor(
      std::string_view path,
      std::shared_ptr<ValueProcessor> processor);

    void
    parse(std::string_view json, void* context) const;

  private:
    struct StringToken
    {
      std::string_view value;
      std::string unescaped;
      bool escaped = false;
    };

    struct NumberToken
    {
      std::string_view value;
      bool is_float = false;
    };

    class Cursor
    {
    public:
      explicit Cursor(std::string_view json);

      bool
      eof() const noexcept;

      char
      peek() const;

      char
      get();

      void
      expect(char expected);

      void
      skip_spaces() noexcept;

      void
      consume_literal(std::string_view literal);

      StringToken
      parse_string();

      void
      skip_string();

      void
      skip_string_rough();

      void
      skip_object_rough();

      void
      skip_array_rough();

      NumberToken
      parse_number();

      [[noreturn]] void
      throw_error(const std::string& message) const;

    private:
      static bool
      is_space_(char c) noexcept;

      static bool
      is_delimiter_(char c) noexcept;

      static bool
      is_dec_(char c) noexcept;

      static bool
      is_hex_(char c) noexcept;

      static int
      hex_to_int_(char c) noexcept;

      void
      parse_string_tail_(std::string& out);

      void
      append_escape_(std::string& out);

      void
      skip_string_rough_after_quote_();

      void
      skip_compound_rough_(
        char open,
        const char* error_message);

      void
      skip_escape_();

      uint32_t
      parse_unicode_escape_();

      void
      skip_unicode_escape_();

      void
      append_unicode_escape_(std::string& out);

    private:
      const char* begin_;
      const char* pos_;
      const char* end_;
    };

    template<bool Strict>
    void
    parse_object_(
      Cursor& cursor,
      const JsonTreeProcessor& processor,
      void* context,
      bool notify_started) const;

    template<bool Strict>
    void
    parse_array_(
      Cursor& cursor,
      const JsonTreeProcessor& processor,
      void* context) const;

    template<bool Strict>
    void
    parse_value_(
      Cursor& cursor,
      const JsonTreeProcessor& processor,
      void* context) const;

    template<bool Strict>
    void
    skip_object_(Cursor& cursor) const;

    template<bool Strict>
    void
    skip_array_(Cursor& cursor) const;

    template<bool Strict>
    void
    skip_value_(Cursor& cursor) const;

  private:
    bool strict_;
    JsonTreeProcessor root_processor_;
  };

  inline void
  FastJsonParser::ValueProcessor::object_started(
    std::string_view,
    void*) const
  {}

  inline
  FastJsonParser::FastJsonParser(bool strict)
    : strict_(strict)
  {}

  inline void
  FastJsonParser::ValueProcessor::array_started(
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected array in " + std::string(path));
  }

  inline void
  FastJsonParser::ValueProcessor::process_integer(
    int64_t,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected integer in " + std::string(path));
  }

  inline void
  FastJsonParser::ValueProcessor::process_float(
    double,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected float in " + std::string(path));
  }

  inline void
  FastJsonParser::ValueProcessor::process_string(
    std::string_view value,
    std::string_view path,
    void* context) const
  {
    process_string(std::string(value), path, context);
  }

  inline void
  FastJsonParser::ValueProcessor::process_string(
    std::string&&,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected string in " + std::string(path));
  }

  inline void
  FastJsonParser::ValueProcessor::process_bool(
    bool,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected bool in " + std::string(path));
  }

  inline void
  FastJsonParser::ValueProcessor::process_null(
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected null in " + std::string(path));
  }

  inline
  FastJsonParser::JsonTreeProcessor::JsonTreeProcessor(
    JsonTreeProcessor* parent_val,
    std::string path_val)
    : parent(parent_val),
      path(std::move(path_val))
  {}

  inline void
  FastJsonParser::add_processor(
    std::string_view path,
    std::shared_ptr<ValueProcessor> processor)
  {
    JsonTreeProcessor* current = &root_processor_;

    if(path.empty())
    {
      current->value_processor = std::move(processor);
      return;
    }

    std::size_t pos = 0;
    while(pos <= path.size())
    {
      const std::size_t next_pos = path.find('.', pos);
      const std::string_view key = next_pos == std::string_view::npos ?
        path.substr(pos) : path.substr(pos, next_pos - pos);

      if(key.empty())
      {
        throw ParseError("empty path element in '" + std::string(path) + "'");
      }

      auto it = current->sub_processors.find(key);
      if(it == current->sub_processors.end())
      {
        current->key_holder.emplace_back(key);
        const std::string_view stored_key(current->key_holder.back());
        std::string child_path = current->path.empty() ?
          std::string(stored_key) :
          current->path + "." + std::string(stored_key);

        auto child = std::make_unique<JsonTreeProcessor>(
          current,
          std::move(child_path));
        it = current->sub_processors.emplace(
          stored_key,
          std::move(child)).first;
      }

      current = it->second.get();

      if(next_pos == std::string_view::npos)
      {
        break;
      }
      pos = next_pos + 1;
    }

    current->value_processor = std::move(processor);
  }

  inline void
  FastJsonParser::parse(std::string_view json, void* context) const
  {
    Cursor cursor(json);
    cursor.skip_spaces();

    if(cursor.eof() || cursor.peek() != '{')
    {
      cursor.throw_error("top-level JSON value must be object");
    }

    if(strict_)
    {
      parse_object_<true>(cursor, root_processor_, context, false);
    }
    else
    {
      parse_object_<false>(cursor, root_processor_, context, false);
    }
    cursor.skip_spaces();

    if(!cursor.eof())
    {
      cursor.throw_error("unexpected trailing character");
    }
  }

  inline
  FastJsonParser::Cursor::Cursor(std::string_view json)
    : begin_(json.data()),
      pos_(json.data()),
      end_(json.data() + json.size())
  {}

  inline bool
  FastJsonParser::Cursor::eof() const noexcept
  {
    return pos_ == end_;
  }

  inline char
  FastJsonParser::Cursor::peek() const
  {
    return *pos_;
  }

  inline char
  FastJsonParser::Cursor::get()
  {
    if(eof())
    {
      throw_error("unexpected end of JSON");
    }
    return *pos_++;
  }

  inline void
  FastJsonParser::Cursor::expect(char expected)
  {
    if(eof() || *pos_ != expected)
    {
      throw_error(std::string("expected '") + expected + "'");
    }
    ++pos_;
  }

  inline void
  FastJsonParser::Cursor::skip_spaces() noexcept
  {
    while(pos_ != end_ && is_space_(*pos_))
    {
      ++pos_;
    }
  }

  inline void
  FastJsonParser::Cursor::consume_literal(std::string_view literal)
  {
    if(static_cast<std::size_t>(end_ - pos_) < literal.size() ||
      std::string_view(pos_, literal.size()) != literal)
    {
      throw_error("bad identifier");
    }
    pos_ += literal.size();
    if(pos_ != end_ && !is_delimiter_(*pos_))
    {
      throw_error("bad identifier");
    }
  }

  inline FastJsonParser::StringToken
  FastJsonParser::Cursor::parse_string()
  {
    expect('"');
    const char* const value_begin = pos_;

    while(pos_ != end_)
    {
      const char c = *pos_++;
      if(c == '"')
      {
        return {
          std::string_view(value_begin, pos_ - value_begin - 1),
          {},
          false};
      }

      if(static_cast<unsigned char>(c) < 0x20)
      {
        throw_error("bad string");
      }

      if(c == '\\')
      {
        StringToken token;
        token.escaped = true;
        token.unescaped.assign(value_begin, pos_ - value_begin - 1);
        parse_string_tail_(token.unescaped);
        token.value = token.unescaped;
        return token;
      }
    }

    throw_error("bad string");
  }

  inline void
  FastJsonParser::Cursor::skip_string()
  {
    expect('"');

    while(pos_ != end_)
    {
      const char c = *pos_++;
      if(c == '"')
      {
        return;
      }

      if(static_cast<unsigned char>(c) < 0x20)
      {
        throw_error("bad string");
      }

      if(c == '\\')
      {
        skip_escape_();
      }
    }

    throw_error("bad string");
  }

  inline void
  FastJsonParser::Cursor::skip_string_rough()
  {
    expect('"');
    skip_string_rough_after_quote_();
  }

  inline void
  FastJsonParser::Cursor::skip_string_rough_after_quote_()
  {
    while(pos_ < end_)
    {
      const auto* quote = static_cast<const char*>(
        std::memchr(pos_, '"', end_ - pos_));
      if(!quote)
      {
        break;
      }

      std::size_t backslashes = 0;
      for(const char* it = quote; it > pos_ && *(it - 1) == '\\'; --it)
      {
        ++backslashes;
      }

      pos_ = quote + 1;
      if((backslashes & 1) == 0)
      {
        return;
      }
    }

    throw_error("bad string");
  }

  inline void
  FastJsonParser::Cursor::skip_object_rough()
  {
    skip_compound_rough_('{', "unexpected end of object");
  }

  inline void
  FastJsonParser::Cursor::skip_array_rough()
  {
    skip_compound_rough_('[', "unexpected end of array");
  }

  inline void
  FastJsonParser::Cursor::skip_compound_rough_(
    char open,
    const char* error_message)
  {
    expect(open);

    unsigned object_depth = open == '{' ? 1 : 0;
    unsigned array_depth = open == '[' ? 1 : 0;

    while(pos_ != end_)
    {
      const char c = *pos_++;

      if(c == '"')
      {
        skip_string_rough_after_quote_();
      }
      else if(c == '{')
      {
        ++object_depth;
      }
      else if(c == '[')
      {
        ++array_depth;
      }
      else if(c == '}' && object_depth > 0)
      {
        --object_depth;
      }
      else if(c == ']' && array_depth > 0)
      {
        --array_depth;
      }

      if(object_depth == 0 && array_depth == 0)
      {
        return;
      }
    }

    throw_error(error_message);
  }

  inline FastJsonParser::NumberToken
  FastJsonParser::Cursor::parse_number()
  {
    const char* const number_begin = pos_;
    bool is_float = false;

    if(pos_ != end_ && (*pos_ == '-' || *pos_ == '+'))
    {
      ++pos_;
    }

    bool has_digits = false;
    while(pos_ != end_ && is_dec_(*pos_))
    {
      has_digits = true;
      ++pos_;
    }

    if(pos_ != end_ && *pos_ == '.')
    {
      is_float = true;
      ++pos_;

      while(pos_ != end_ && is_dec_(*pos_))
      {
        has_digits = true;
        ++pos_;
      }
    }

    if(!has_digits)
    {
      throw_error("bad number");
    }

    if(pos_ != end_ && (*pos_ == 'e' || *pos_ == 'E'))
    {
      is_float = true;
      ++pos_;

      if(pos_ != end_ && (*pos_ == '-' || *pos_ == '+'))
      {
        ++pos_;
      }

      const char* const exponent_begin = pos_;
      while(pos_ != end_ && is_dec_(*pos_))
      {
        ++pos_;
      }

      if(exponent_begin == pos_)
      {
        throw_error("bad number");
      }
    }

    if(pos_ != end_ && !is_delimiter_(*pos_))
    {
      throw_error("bad number");
    }

    return {std::string_view(number_begin, pos_ - number_begin), is_float};
  }

  inline void
  FastJsonParser::Cursor::throw_error(const std::string& message) const
  {
    throw ParseError(
      message + " at pos " + std::to_string(pos_ - begin_));
  }

  inline bool
  FastJsonParser::Cursor::is_space_(char c) noexcept
  {
    return c == ' ' || static_cast<unsigned>(c - '\t') <= ('\r' - '\t');
  }

  inline bool
  FastJsonParser::Cursor::is_delimiter_(char c) noexcept
  {
    return c == ',' || c == ':' || c == ']' || c == '}' || is_space_(c);
  }

  inline bool
  FastJsonParser::Cursor::is_dec_(char c) noexcept
  {
    return static_cast<unsigned>(c - '0') <= 9;
  }

  inline bool
  FastJsonParser::Cursor::is_hex_(char c) noexcept
  {
    return is_dec_(c) ||
      static_cast<unsigned>((c | 0x20) - 'a') <= ('f' - 'a');
  }

  inline int
  FastJsonParser::Cursor::hex_to_int_(char c) noexcept
  {
    if(c >= 'a')
    {
      return c - 'a' + 10;
    }
    if(c >= 'A')
    {
      return c - 'A' + 10;
    }
    return c - '0';
  }

  inline void
  FastJsonParser::Cursor::parse_string_tail_(std::string& out)
  {
    append_escape_(out);

    while(pos_ != end_)
    {
      char c = *pos_++;
      if(c == '"')
      {
        return;
      }

      if(static_cast<unsigned char>(c) < 0x20)
      {
        throw_error("bad string");
      }

      if(c == '\\')
      {
        append_escape_(out);
      }
      else
      {
        out += c;
      }
    }

    throw_error("bad string");
  }

  inline void
  FastJsonParser::Cursor::append_escape_(std::string& out)
  {
    if(pos_ == end_)
    {
      throw_error("bad string");
    }

    const char c = *pos_++;
    switch(c)
    {
    case '\\':
    case '"':
    case '/':
      out += c;
      return;
    case 'b':
      out += '\b';
      return;
    case 'f':
      out += '\f';
      return;
    case 'n':
      out += '\n';
      return;
    case 'r':
      out += '\r';
      return;
    case 't':
      out += '\t';
      return;
    case 'u':
      append_unicode_escape_(out);
      return;
    default:
      throw_error("bad string");
    }
  }

  inline void
  FastJsonParser::Cursor::skip_escape_()
  {
    if(pos_ == end_)
    {
      throw_error("bad string");
    }

    const char c = *pos_++;
    switch(c)
    {
    case '\\':
    case '"':
    case '/':
    case 'b':
    case 'f':
    case 'n':
    case 'r':
    case 't':
      return;
    case 'u':
      skip_unicode_escape_();
      return;
    default:
      throw_error("bad string");
    }
  }

  inline uint32_t
  FastJsonParser::Cursor::parse_unicode_escape_()
  {
    uint32_t code = 0;
    for(int i = 0; i < 4; ++i)
    {
      if(pos_ == end_ || !is_hex_(*pos_))
      {
        throw_error("bad string");
      }
      code = code * 16 + hex_to_int_(*pos_++);
    }
    return code;
  }

  inline void
  FastJsonParser::Cursor::skip_unicode_escape_()
  {
    for(int i = 0; i < 4; ++i)
    {
      if(pos_ == end_ || !is_hex_(*pos_))
      {
        throw_error("bad string");
      }
      ++pos_;
    }
  }

  inline void
  FastJsonParser::Cursor::append_unicode_escape_(std::string& out)
  {
    const uint32_t code = parse_unicode_escape_();
    if(code < 0x80)
    {
      out += static_cast<char>(code);
    }
    else if(code < 0x800)
    {
      out += static_cast<char>(0xC0 | (code >> 6));
      out += static_cast<char>(0x80 | (code & 0x3F));
    }
    else
    {
      out += static_cast<char>(0xE0 | (code >> 12));
      out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (code & 0x3F));
    }
  }

  template<bool Strict>
  inline void
  FastJsonParser::parse_object_(
    Cursor& cursor,
    const JsonTreeProcessor& processor,
    void* context,
    bool notify_started) const
  {
    cursor.expect('{');

    if(notify_started && processor.value_processor)
    {
      processor.value_processor->object_started(processor.path, context);
    }

    cursor.skip_spaces();
    if(!cursor.eof() && cursor.peek() == '}')
    {
      cursor.get();
      return;
    }

    for(;;)
    {
      cursor.skip_spaces();
      if(cursor.eof() || cursor.peek() != '"')
      {
        cursor.throw_error("object key expected");
      }

      StringToken key = cursor.parse_string();
      cursor.skip_spaces();
      cursor.expect(':');
      cursor.skip_spaces();

      const auto child_it = processor.sub_processors.find(key.value);
      if(child_it != processor.sub_processors.end())
      {
        parse_value_<Strict>(cursor, *child_it->second, context);
      }
      else
      {
        skip_value_<Strict>(cursor);
      }

      cursor.skip_spaces();
      if(cursor.eof())
      {
        cursor.throw_error("unexpected end of object");
      }

      const char delimiter = cursor.get();
      if(delimiter == '}')
      {
        return;
      }
      if(delimiter != ',')
      {
        cursor.throw_error("expected ',' or '}'");
      }
    }
  }

  template<bool Strict>
  inline void
  FastJsonParser::parse_array_(
    Cursor& cursor,
    const JsonTreeProcessor& processor,
    void* context) const
  {
    cursor.expect('[');

    if(processor.value_processor)
    {
      processor.value_processor->array_started(processor.path, context);
    }

    cursor.skip_spaces();
    if(!cursor.eof() && cursor.peek() == ']')
    {
      cursor.get();
      return;
    }

    for(;;)
    {
      cursor.skip_spaces();
      parse_value_<Strict>(cursor, processor, context);

      cursor.skip_spaces();
      if(cursor.eof())
      {
        cursor.throw_error("unexpected end of array");
      }

      const char delimiter = cursor.get();
      if(delimiter == ']')
      {
        return;
      }
      if(delimiter != ',')
      {
        cursor.throw_error("expected ',' or ']'");
      }
    }
  }

  template<bool Strict>
  inline void
  FastJsonParser::parse_value_(
    Cursor& cursor,
    const JsonTreeProcessor& processor,
    void* context) const
  {
    cursor.skip_spaces();
    if(cursor.eof())
    {
      cursor.throw_error("unexpected end of JSON");
    }

    const char c = cursor.peek();
    if(c == '{')
    {
      parse_object_<Strict>(cursor, processor, context, true);
    }
    else if(c == '[')
    {
      parse_array_<Strict>(cursor, processor, context);
    }
    else if(c == '"')
    {
      if(processor.value_processor)
      {
        StringToken token = cursor.parse_string();
        if(token.escaped)
        {
          processor.value_processor->process_string(
            std::move(token.unescaped),
            processor.path,
            context);
        }
        else
        {
          processor.value_processor->process_string(
            token.value,
            processor.path,
            context);
        }
      }
      else
      {
        cursor.skip_string();
      }
    }
    else if(c == '-' || (c >= '0' && c <= '9'))
    {
      NumberToken number = cursor.parse_number();
      if(processor.value_processor)
      {
        if(number.is_float)
        {
          constexpr std::size_t stack_buffer_size = 128;
          char stack_buffer[stack_buffer_size];
          const char* number_data = nullptr;
          std::size_t number_size = number.value.size();
          std::string number_string;
          if(number_size < stack_buffer_size)
          {
            std::memcpy(stack_buffer, number.value.data(), number_size);
            stack_buffer[number_size] = '\0';
            number_data = stack_buffer;
          }
          else
          {
            number_string.assign(number.value);
            number_data = number_string.c_str();
          }

          char* end = nullptr;
          const double value = std::strtod(number_data, &end);
          if(end != number_data + number_size)
          {
            cursor.throw_error("bad float");
          }
          processor.value_processor->process_float(
            value,
            processor.path,
            context);
        }
        else
        {
          int64_t value = 0;
          const auto result = std::from_chars(
            number.value.data(),
            number.value.data() + number.value.size(),
            value);
          if(result.ec != std::errc() ||
            result.ptr != number.value.data() + number.value.size())
          {
            cursor.throw_error("bad integer");
          }
          processor.value_processor->process_integer(
            value,
            processor.path,
            context);
        }
      }
    }
    else if(c == 't')
    {
      cursor.consume_literal("true");
      if(processor.value_processor)
      {
        processor.value_processor->process_bool(
          true,
          processor.path,
          context);
      }
    }
    else if(c == 'f')
    {
      cursor.consume_literal("false");
      if(processor.value_processor)
      {
        processor.value_processor->process_bool(
          false,
          processor.path,
          context);
      }
    }
    else if(c == 'n')
    {
      cursor.consume_literal("null");
      if(processor.value_processor)
      {
        processor.value_processor->process_null(processor.path, context);
      }
    }
    else
    {
      cursor.throw_error("unexpected character");
    }
  }

  template<bool Strict>
  inline void
  FastJsonParser::skip_object_(Cursor& cursor) const
  {
    if constexpr(!Strict)
    {
      cursor.skip_object_rough();
      return;
    }

    cursor.expect('{');
    if(!cursor.eof() && cursor.peek() == '}')
    {
      cursor.get();
      return;
    }

    for(;;)
    {
      cursor.skip_spaces();
      if(cursor.eof() || cursor.peek() != '"')
      {
        cursor.throw_error("object key expected");
      }

      cursor.skip_string();
      cursor.skip_spaces();
      cursor.expect(':');
      skip_value_<Strict>(cursor);

      cursor.skip_spaces();
      if(cursor.eof())
      {
        cursor.throw_error("unexpected end of object");
      }

      const char delimiter = cursor.get();
      if(delimiter == '}')
      {
        return;
      }
      if(delimiter != ',')
      {
        cursor.throw_error("expected ',' or '}'");
      }
    }
  }

  template<bool Strict>
  inline void
  FastJsonParser::skip_array_(Cursor& cursor) const
  {
    if constexpr(!Strict)
    {
      cursor.skip_array_rough();
      return;
    }

    cursor.expect('[');
    if(!cursor.eof() && cursor.peek() == ']')
    {
      cursor.get();
      return;
    }

    for(;;)
    {
      cursor.skip_spaces();
      skip_value_<Strict>(cursor);

      cursor.skip_spaces();
      if(cursor.eof())
      {
        cursor.throw_error("unexpected end of array");
      }

      const char delimiter = cursor.get();
      if(delimiter == ']')
      {
        return;
      }
      if(delimiter != ',')
      {
        cursor.throw_error("expected ',' or ']'");
      }
    }
  }

  template<bool Strict>
  inline void
  FastJsonParser::skip_value_(Cursor& cursor) const
  {
    cursor.skip_spaces();
    if(cursor.eof())
    {
      cursor.throw_error("unexpected end of JSON");
    }

    const char c = cursor.peek();
    if(c == '{')
    {
      skip_object_<Strict>(cursor);
    }
    else if(c == '[')
    {
      skip_array_<Strict>(cursor);
    }
    else if(c == '"')
    {
      if constexpr(Strict)
      {
        cursor.skip_string();
      }
      else
      {
        cursor.skip_string_rough();
      }
    }
    else if(c == '-' || (c >= '0' && c <= '9'))
    {
      cursor.parse_number();
    }
    else if(c == 't')
    {
      cursor.consume_literal("true");
    }
    else if(c == 'f')
    {
      cursor.consume_literal("false");
    }
    else if(c == 'n')
    {
      cursor.consume_literal("null");
    }
    else
    {
      cursor.throw_error("unexpected character");
    }
  }
}
