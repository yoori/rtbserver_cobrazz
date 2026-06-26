#include "FastJsonParser.hpp"

#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <list>
#include <memory_resource>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__x86_64__) || defined(__SSE2__)
#include <emmintrin.h>
#endif

#if defined(__GNUC__)
#define AD_FAST_JSON_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define AD_FAST_JSON_ALWAYS_INLINE inline
#endif

namespace AdServer::Commons
{
  namespace
  {
    bool
    simd_supported_() noexcept
    {
#if defined(__x86_64__)
      return true;
#elif defined(__i386__) && defined(__GNUC__)
      __builtin_cpu_init();
      return __builtin_cpu_supports("sse2");
#elif defined(__SSE2__)
      return true;
#else
      return false;
#endif
    }

    bool
    is_space_(char c) noexcept
    {
      return c == ' ' || static_cast<unsigned>(c - '\t') <= ('\r' - '\t');
    }

    bool
    is_string_special_(char c) noexcept
    {
      return c == '"' || c == '\\';
    }

    bool
    is_compound_special_(char c) noexcept
    {
      return c == '"' || c == '{' || c == '}' || c == '[' || c == ']';
    }

    bool
    is_unquoted_delimiter_(char c) noexcept
    {
      return c == ',' || c == ']' || c == '}';
    }

    const char*
    find_non_space_scalar_(const char* pos, const char* end) noexcept
    {
      while(pos != end && is_space_(*pos))
      {
        ++pos;
      }

      return pos;
    }

    const char*
    find_string_special_scalar_(const char* pos, const char* end) noexcept
    {
      while(pos != end && !is_string_special_(*pos))
      {
        ++pos;
      }

      return pos;
    }

    const char*
    find_quote_scalar_(const char* pos, const char* end) noexcept
    {
      while(pos != end && *pos != '"')
      {
        ++pos;
      }

      return pos;
    }

    const char*
    find_unquoted_delimiter_scalar_(const char* pos, const char* end) noexcept
    {
      while(pos != end && !is_unquoted_delimiter_(*pos))
      {
        ++pos;
      }

      return pos;
    }

#if defined(__x86_64__) || defined(__SSE2__)
#if defined(__GNUC__)
    __attribute__((target("sse2")))
#endif
    const char*
    find_non_space_simd_(const char* pos, const char* end) noexcept
    {
      const __m128i space = _mm_set1_epi8(' ');
      const __m128i tab = _mm_set1_epi8('\t');
      const __m128i line_feed = _mm_set1_epi8('\n');
      const __m128i vertical_tab = _mm_set1_epi8('\v');
      const __m128i form_feed = _mm_set1_epi8('\f');
      const __m128i carriage_return = _mm_set1_epi8('\r');

      while(static_cast<std::size_t>(end - pos) >= 16)
      {
        const __m128i bytes = _mm_loadu_si128(
          reinterpret_cast<const __m128i*>(pos));
        const __m128i blank_matches = _mm_or_si128(
          _mm_cmpeq_epi8(bytes, space),
          _mm_cmpeq_epi8(bytes, tab));
        const __m128i line_matches = _mm_or_si128(
          _mm_cmpeq_epi8(bytes, line_feed),
          _mm_cmpeq_epi8(bytes, carriage_return));
        const __m128i other_matches = _mm_or_si128(
          _mm_cmpeq_epi8(bytes, vertical_tab),
          _mm_cmpeq_epi8(bytes, form_feed));
        const unsigned space_mask = static_cast<unsigned>(
          _mm_movemask_epi8(
            _mm_or_si128(blank_matches, _mm_or_si128(line_matches, other_matches))));
        if(space_mask != 0xFFFFu)
        {
          return pos + __builtin_ctz((~space_mask) & 0xFFFFu);
        }

        pos += 16;
      }

      return find_non_space_scalar_(pos, end);
    }

#if defined(__GNUC__)
    __attribute__((target("sse2")))
#endif
    const char*
    find_string_special_simd_(const char* pos, const char* end) noexcept
    {
      const __m128i quote = _mm_set1_epi8('"');
      const __m128i backslash = _mm_set1_epi8('\\');

      while(static_cast<std::size_t>(end - pos) >= 16)
      {
        const __m128i bytes = _mm_loadu_si128(
          reinterpret_cast<const __m128i*>(pos));
        const __m128i matches = _mm_or_si128(
          _mm_cmpeq_epi8(bytes, quote),
          _mm_cmpeq_epi8(bytes, backslash));
        const unsigned mask = static_cast<unsigned>(
          _mm_movemask_epi8(matches));
        if(mask != 0)
        {
          return pos + __builtin_ctz(mask);
        }

        pos += 16;
      }

      return find_string_special_scalar_(pos, end);
    }

#if defined(__GNUC__)
    __attribute__((target("sse2")))
#endif
    const char*
    find_quote_simd_(const char* pos, const char* end) noexcept
    {
      const __m128i quote = _mm_set1_epi8('"');

      while(static_cast<std::size_t>(end - pos) >= 16)
      {
        const __m128i bytes = _mm_loadu_si128(
          reinterpret_cast<const __m128i*>(pos));
        const unsigned mask = static_cast<unsigned>(
          _mm_movemask_epi8(_mm_cmpeq_epi8(bytes, quote)));
        if(mask != 0)
        {
          return pos + __builtin_ctz(mask);
        }

        pos += 16;
      }

      return find_quote_scalar_(pos, end);
    }

#if defined(__GNUC__)
    __attribute__((target("sse2")))
#endif
    const char*
    find_compound_special_simd_(const char* pos, const char* end) noexcept
    {
      const __m128i quote = _mm_set1_epi8('"');
      const __m128i object_open = _mm_set1_epi8('{');
      const __m128i object_close = _mm_set1_epi8('}');
      const __m128i array_open = _mm_set1_epi8('[');
      const __m128i array_close = _mm_set1_epi8(']');

      while(static_cast<std::size_t>(end - pos) >= 16)
      {
        const __m128i bytes = _mm_loadu_si128(
          reinterpret_cast<const __m128i*>(pos));
        const __m128i object_matches = _mm_or_si128(
          _mm_cmpeq_epi8(bytes, object_open),
          _mm_cmpeq_epi8(bytes, object_close));
        const __m128i array_matches = _mm_or_si128(
          _mm_cmpeq_epi8(bytes, array_open),
          _mm_cmpeq_epi8(bytes, array_close));
        const __m128i matches = _mm_or_si128(
          _mm_cmpeq_epi8(bytes, quote),
          _mm_or_si128(object_matches, array_matches));
        const unsigned mask = static_cast<unsigned>(
          _mm_movemask_epi8(matches));
        if(mask != 0)
        {
          return pos + __builtin_ctz(mask);
        }

        pos += 16;
      }

      while(pos != end && !is_compound_special_(*pos))
      {
        ++pos;
      }

      return pos;
    }

#if defined(__GNUC__)
    __attribute__((target("sse2")))
#endif
    const char*
    find_unquoted_delimiter_simd_(const char* pos, const char* end) noexcept
    {
      const __m128i comma = _mm_set1_epi8(',');
      const __m128i array_close = _mm_set1_epi8(']');
      const __m128i object_close = _mm_set1_epi8('}');

      while(static_cast<std::size_t>(end - pos) >= 16)
      {
        const __m128i bytes = _mm_loadu_si128(
          reinterpret_cast<const __m128i*>(pos));
        const __m128i matches = _mm_or_si128(
          _mm_cmpeq_epi8(bytes, comma),
          _mm_or_si128(
            _mm_cmpeq_epi8(bytes, array_close),
            _mm_cmpeq_epi8(bytes, object_close)));
        const unsigned mask = static_cast<unsigned>(
          _mm_movemask_epi8(matches));
        if(mask != 0)
        {
          return pos + __builtin_ctz(mask);
        }

        pos += 16;
      }

      return find_unquoted_delimiter_scalar_(pos, end);
    }

    template<bool UseSimd>
    const char*
    find_non_space_(const char* pos, const char* end) noexcept
    {
      if constexpr(UseSimd)
      {
        return find_non_space_simd_(pos, end);
      }

      return find_non_space_scalar_(pos, end);
    }

    template<bool UseSimd>
    const char*
    find_string_special_(const char* pos, const char* end) noexcept
    {
      if constexpr(UseSimd)
      {
        return find_string_special_simd_(pos, end);
      }

      return find_string_special_scalar_(pos, end);
    }

    template<bool UseSimd>
    const char*
    find_quote_(const char* pos, const char* end) noexcept
    {
      if constexpr(UseSimd)
      {
        return find_quote_simd_(pos, end);
      }

      return find_quote_scalar_(pos, end);
    }

    template<bool UseSimd>
    const char*
    find_compound_special_(const char* pos, const char* end) noexcept
    {
      if constexpr(UseSimd)
      {
        return find_compound_special_simd_(pos, end);
      }

      while(pos != end && !is_compound_special_(*pos))
      {
        ++pos;
      }

      return pos;
    }

    template<bool UseSimd>
    const char*
    find_unquoted_delimiter_(const char* pos, const char* end) noexcept
    {
      if constexpr(UseSimd)
      {
        return find_unquoted_delimiter_simd_(pos, end);
      }

      return find_unquoted_delimiter_scalar_(pos, end);
    }

#else
    template<bool UseSimd>
    const char*
    find_non_space_(const char* pos, const char* end) noexcept
    {
      static_cast<void>(UseSimd);
      return find_non_space_scalar_(pos, end);
    }

    template<bool UseSimd>
    const char*
    find_string_special_(const char* pos, const char* end) noexcept
    {
      static_cast<void>(UseSimd);
      return find_string_special_scalar_(pos, end);
    }

    template<bool UseSimd>
    const char*
    find_quote_(const char* pos, const char* end) noexcept
    {
      static_cast<void>(UseSimd);
      return find_quote_scalar_(pos, end);
    }

    template<bool UseSimd>
    const char*
    find_compound_special_(const char* pos, const char* end) noexcept
    {
      static_cast<void>(UseSimd);
      while(pos != end &&
        *pos != '"' &&
        *pos != '{' &&
        *pos != '}' &&
        *pos != '[' &&
        *pos != ']')
      {
        ++pos;
      }

      return pos;
    }

    template<bool UseSimd>
    const char*
    find_unquoted_delimiter_(const char* pos, const char* end) noexcept
    {
      static_cast<void>(UseSimd);
      return find_unquoted_delimiter_scalar_(pos, end);
    }

#endif
  }

  struct FastJsonParser::Impl
  {
    struct JsonTreeProcessor
    {
      JsonTreeProcessor() = default;

      explicit
      JsonTreeProcessor(std::string path_val);

      static std::uint64_t
      short_key_(std::string_view key) noexcept;

      JsonTreeProcessor*
      find_sub_processor(std::string_view key, bool escaped);

      const JsonTreeProcessor*
      find_sub_processor(std::string_view key, bool escaped) const;

      std::list<std::string> key_holder;
      std::unordered_map<
        std::string_view,
        std::unique_ptr<JsonTreeProcessor> > sub_processors;
      std::unordered_map<std::uint64_t, JsonTreeProcessor*> short_processors;
      std::shared_ptr<ValueProcessor> value_processor;
      std::string path;
      bool as_string = false;
    };

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

    enum class ParseFrameType
    {
      Object,
      Array
    };

    enum class ParseFrameState
    {
      ValueOrEnd,
      DelimiterOrEnd
    };

    struct ParseFrame
    {
      ParseFrameType type;
      ParseFrameState state = ParseFrameState::ValueOrEnd;
      const JsonTreeProcessor* processor = nullptr;
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

      char
      get_unchecked() noexcept;

      void
      skip_char() noexcept;

      void
      expect(char expected);

      template<bool UseSimd>
      void
      skip_spaces() noexcept;

      void
      consume_literal(std::string_view literal);

      template<bool UseSimd>
      StringToken
      parse_string();

      template<bool UseSimd>
      void
      skip_string();

      template<bool UseSimd>
      void
      skip_string_rough();

      template<bool UseSimd>
      void
      skip_unquoted_value_rough();

      template<bool UseSimd>
      void
      skip_object_rough();

      template<bool UseSimd>
      void
      skip_array_rough();

      NumberToken
      parse_number();

      std::string_view
      scan_number_literal();

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
      append_escape_(char*& out);

      template<bool UseSimd>
      void
      skip_string_rough_after_quote_();

      template<bool UseSimd>
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
      append_unicode_escape_(char*& out);

    private:
      const char* begin_;
      const char* pos_;
      const char* end_;
    };

    explicit
    Impl(bool strict_val, bool use_simd_val);

    void
    add_processor(
      std::string_view path,
      std::shared_ptr<ValueProcessor> processor,
      bool as_string);

    void
    parse(std::string_view json, void* context) const;

    template<bool Strict, bool UseSimd>
    static void
    parse_(
      const Impl& impl,
      std::string_view json,
      void* context);

    template<bool Strict, bool UseSimd>
    void
    parse_iterative_(Cursor& cursor, void* context) const;

    template<bool Strict, bool UseSimd>
    void
    parse_value_iterative_(
      Cursor& cursor,
      const JsonTreeProcessor& processor,
      void* context,
      std::pmr::vector<ParseFrame>& frames) const;

    template<bool Strict, bool UseSimd>
    void
    skip_object_(Cursor& cursor) const;

    template<bool Strict, bool UseSimd>
    void
    skip_array_(Cursor& cursor) const;

    template<bool Strict, bool UseSimd>
    void
    skip_value_(Cursor& cursor) const;

    using ParseHandler = void (*)(
      const Impl& impl,
      std::string_view json,
      void* context);

    ParseHandler parse_handler = nullptr;
    JsonTreeProcessor root_processor;
  };

  void
  FastJsonParser::ValueProcessor::object_started(
    std::string_view,
    void*) const
  {}

  void
  FastJsonParser::ValueProcessor::array_started(
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected array in " + std::string(path));
  }

  void
  FastJsonParser::ValueProcessor::process_integer(
    int64_t,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected integer in " + std::string(path));
  }

  void
  FastJsonParser::ValueProcessor::process_float(
    double,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected float in " + std::string(path));
  }

  void
  FastJsonParser::ValueProcessor::process_number(
    std::string_view value,
    bool is_float,
    std::string_view path,
    void* context) const
  {
    if(is_float)
    {
      constexpr std::size_t stack_buffer_size = 128;
      char stack_buffer[stack_buffer_size];
      const char* number_data = nullptr;
      const std::size_t number_size = value.size();
      std::string number_string;
      if(number_size < stack_buffer_size)
      {
        std::memcpy(stack_buffer, value.data(), number_size);
        stack_buffer[number_size] = '\0';
        number_data = stack_buffer;
      }
      else
      {
        number_string.assign(value);
        number_data = number_string.c_str();
      }

      char* end = nullptr;
      const double parsed = std::strtod(number_data, &end);
      if(end != number_data + number_size)
      {
        throw ParseError("bad float");
      }
      process_float(parsed, path, context);
    }
    else
    {
      int64_t parsed = 0;
      const auto result = std::from_chars(
        value.data(),
        value.data() + value.size(),
        parsed);
      if(result.ec != std::errc() ||
        result.ptr != value.data() + value.size())
      {
        throw ParseError("bad integer");
      }
      process_integer(parsed, path, context);
    }
  }

  void
  FastJsonParser::ValueProcessor::process_string(
    std::string_view value,
    std::string_view path,
    void* context) const
  {
    process_string(std::string(value), path, context);
  }

  void
  FastJsonParser::ValueProcessor::process_string(
    std::string&&,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected string in " + std::string(path));
  }

  void
  FastJsonParser::ValueProcessor::process_bool(
    bool,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected bool in " + std::string(path));
  }

  void
  FastJsonParser::ValueProcessor::process_null(
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected null in " + std::string(path));
  }

  FastJsonParser::FastJsonParser(bool strict)
    : impl_(std::make_unique<Impl>(strict, simd_supported_()))
  {}

  FastJsonParser::~FastJsonParser() noexcept = default;

  void
  FastJsonParser::add_processor(
    std::string_view path,
    std::shared_ptr<ValueProcessor> processor,
    bool as_string)
  {
    impl_->add_processor(path, std::move(processor), as_string);
  }

  void
  FastJsonParser::parse(std::string_view json, void* context) const
  {
    impl_->parse(json, context);
  }

  FastJsonParser::Impl::JsonTreeProcessor::JsonTreeProcessor(
    std::string path_val)
    : path(std::move(path_val))
  {}

  inline std::uint64_t
  FastJsonParser::Impl::JsonTreeProcessor::short_key_(
    std::string_view key) noexcept
  {
    const auto* data = reinterpret_cast<const unsigned char*>(key.data());
    switch(key.size())
    {
    case 0:
      return 0;
    case 1:
      return data[0];
    case 2:
      return data[0] |
        (std::uint64_t(data[1]) << 8);
    case 3:
      return data[0] |
        (std::uint64_t(data[1]) << 8) |
        (std::uint64_t(data[2]) << 16);
    case 4:
      return data[0] |
        (std::uint64_t(data[1]) << 8) |
        (std::uint64_t(data[2]) << 16) |
        (std::uint64_t(data[3]) << 24);
    case 5:
      return data[0] |
        (std::uint64_t(data[1]) << 8) |
        (std::uint64_t(data[2]) << 16) |
        (std::uint64_t(data[3]) << 24) |
        (std::uint64_t(data[4]) << 32);
    case 6:
      return data[0] |
        (std::uint64_t(data[1]) << 8) |
        (std::uint64_t(data[2]) << 16) |
        (std::uint64_t(data[3]) << 24) |
        (std::uint64_t(data[4]) << 32) |
        (std::uint64_t(data[5]) << 40);
    case 7:
      return data[0] |
        (std::uint64_t(data[1]) << 8) |
        (std::uint64_t(data[2]) << 16) |
        (std::uint64_t(data[3]) << 24) |
        (std::uint64_t(data[4]) << 32) |
        (std::uint64_t(data[5]) << 40) |
        (std::uint64_t(data[6]) << 48);
    default:
      return data[0] |
        (std::uint64_t(data[1]) << 8) |
        (std::uint64_t(data[2]) << 16) |
        (std::uint64_t(data[3]) << 24) |
        (std::uint64_t(data[4]) << 32) |
        (std::uint64_t(data[5]) << 40) |
        (std::uint64_t(data[6]) << 48) |
        (std::uint64_t(data[7]) << 56);
    }
  }

  FastJsonParser::Impl::JsonTreeProcessor*
  FastJsonParser::Impl::JsonTreeProcessor::find_sub_processor(
    std::string_view key,
    bool escaped)
  {
    if(!escaped && key.size() <= sizeof(std::uint64_t))
    {
      const auto short_it = short_processors.find(short_key_(key));
      if(short_it != short_processors.end())
      {
        return short_it->second;
      }
      return nullptr;
    }

    const auto it = sub_processors.find(key);
    return it != sub_processors.end() ? it->second.get() : nullptr;
  }

  const FastJsonParser::Impl::JsonTreeProcessor*
  FastJsonParser::Impl::JsonTreeProcessor::find_sub_processor(
    std::string_view key,
    bool escaped) const
  {
    if(!escaped && key.size() <= sizeof(std::uint64_t))
    {
      const auto short_it = short_processors.find(short_key_(key));
      if(short_it != short_processors.end())
      {
        return short_it->second;
      }
      return nullptr;
    }

    const auto it = sub_processors.find(key);
    return it != sub_processors.end() ? it->second.get() : nullptr;
  }

  FastJsonParser::Impl::Impl(bool strict_val, bool use_simd_val)
  {
    if(strict_val)
    {
      parse_handler = use_simd_val ?
        &Impl::parse_<true, true> :
        &Impl::parse_<true, false>;
    }
    else
    {
      parse_handler = use_simd_val ?
        &Impl::parse_<false, true> :
        &Impl::parse_<false, false>;
    }
  }

  void
  FastJsonParser::Impl::add_processor(
    std::string_view path,
    std::shared_ptr<ValueProcessor> processor,
    bool as_string)
  {
    JsonTreeProcessor* current = &root_processor;

    if(path.empty())
    {
      current->value_processor = std::move(processor);
      current->as_string = as_string;
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

      JsonTreeProcessor* child_processor =
        current->find_sub_processor(key, false);
      if(child_processor == nullptr)
      {
        current->key_holder.emplace_back(key);
        const std::string_view stored_key(current->key_holder.back());
        std::string child_path = current->path.empty() ?
          std::string(stored_key) :
          current->path + "." + std::string(stored_key);

        auto child = std::make_unique<JsonTreeProcessor>(
          std::move(child_path));
        child_processor = child.get();
        current->sub_processors.emplace(stored_key, std::move(child));
        if(stored_key.size() <= sizeof(std::uint64_t))
        {
          current->short_processors.emplace(
            JsonTreeProcessor::short_key_(stored_key),
            child_processor);
        }
      }

      current = child_processor;

      if(next_pos == std::string_view::npos)
      {
        break;
      }
      pos = next_pos + 1;
    }

    current->value_processor = std::move(processor);
    current->as_string = as_string;
  }

  AD_FAST_JSON_ALWAYS_INLINE
  FastJsonParser::Impl::Cursor::Cursor(std::string_view json)
    : begin_(json.data()),
      pos_(json.data()),
      end_(json.data() + json.size())
  {}

  AD_FAST_JSON_ALWAYS_INLINE bool
  FastJsonParser::Impl::Cursor::eof() const noexcept
  {
    return pos_ == end_;
  }

  AD_FAST_JSON_ALWAYS_INLINE char
  FastJsonParser::Impl::Cursor::peek() const
  {
    return *pos_;
  }

  AD_FAST_JSON_ALWAYS_INLINE char
  FastJsonParser::Impl::Cursor::get()
  {
    if(eof())
    {
      throw_error("unexpected end of JSON");
    }
    return *pos_++;
  }

  AD_FAST_JSON_ALWAYS_INLINE char
  FastJsonParser::Impl::Cursor::get_unchecked() noexcept
  {
    return *pos_++;
  }

  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser::Impl::Cursor::skip_char() noexcept
  {
    ++pos_;
  }

  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser::Impl::Cursor::expect(char expected)
  {
    if(eof() || *pos_ != expected)
    {
      throw_error(std::string("expected '") + expected + "'");
    }
    ++pos_;
  }

  template<bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser::Impl::Cursor::skip_spaces() noexcept
  {
    if(pos_ == end_ || !is_space_(*pos_))
    {
      return;
    }
    pos_ = find_non_space_<UseSimd>(pos_ + 1, end_);
  }

  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser::Impl::Cursor::consume_literal(std::string_view literal)
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

  template<bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE FastJsonParser::Impl::StringToken
  FastJsonParser::Impl::Cursor::parse_string()
  {
    skip_char();
    const char* const value_begin = pos_;
    const char* const special = find_string_special_<UseSimd>(pos_, end_);
    if(special == end_)
    {
      pos_ = end_;
      throw_error("bad string");
    }

    pos_ = special + 1;
    if(*special == '"')
    {
      return {
        std::string_view(value_begin, special - value_begin),
        {},
        false};
    }

    StringToken token;
    token.escaped = true;
    token.unescaped.assign(value_begin, special - value_begin);
    parse_string_tail_(token.unescaped);
    token.value = token.unescaped;
    return token;
  }

  template<bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser::Impl::Cursor::skip_string()
  {
    skip_char();

    for(;;)
    {
      const char* const special = find_string_special_<UseSimd>(pos_, end_);
      if(special == end_)
      {
        pos_ = end_;
        throw_error("bad string");
      }

      pos_ = special + 1;
      if(*special == '"')
      {
        return;
      }

      skip_escape_();
    }
  }

  template<bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser::Impl::Cursor::skip_string_rough()
  {
    skip_char();
    skip_string_rough_after_quote_<UseSimd>();
  }

  template<bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser::Impl::Cursor::skip_unquoted_value_rough()
  {
    pos_ = find_unquoted_delimiter_<UseSimd>(pos_, end_);
  }

  template<bool UseSimd>
  inline void
  FastJsonParser::Impl::Cursor::skip_string_rough_after_quote_()
  {
    while(pos_ < end_)
    {
      const char* const quote = find_quote_<UseSimd>(pos_, end_);
      if(quote == end_)
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

  template<bool UseSimd>
  inline void
  FastJsonParser::Impl::Cursor::skip_object_rough()
  {
    skip_compound_rough_<UseSimd>('{', "unexpected end of object");
  }

  template<bool UseSimd>
  inline void
  FastJsonParser::Impl::Cursor::skip_array_rough()
  {
    skip_compound_rough_<UseSimd>('[', "unexpected end of array");
  }

  template<bool UseSimd>
  inline void
  FastJsonParser::Impl::Cursor::skip_compound_rough_(
    char open,
    const char* error_message)
  {
    skip_char();

    unsigned object_depth = open == '{' ? 1 : 0;
    unsigned array_depth = open == '[' ? 1 : 0;

    while(pos_ != end_)
    {
      const char* const special = find_compound_special_<UseSimd>(pos_, end_);
      if(special == end_)
      {
        pos_ = end_;
        break;
      }

      pos_ = special + 1;
      const char c = *special;

      if(c == '"')
      {
        skip_string_rough_after_quote_<UseSimd>();
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

  AD_FAST_JSON_ALWAYS_INLINE FastJsonParser::Impl::NumberToken
  FastJsonParser::Impl::Cursor::parse_number()
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

  AD_FAST_JSON_ALWAYS_INLINE std::string_view
  FastJsonParser::Impl::Cursor::scan_number_literal()
  {
    const char* const number_begin = pos_;

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

    return std::string_view(number_begin, pos_ - number_begin);
  }

  inline void
  FastJsonParser::Impl::Cursor::throw_error(
    const std::string& message) const
  {
    throw ParseError(
      message + " at pos " + std::to_string(pos_ - begin_));
  }

  AD_FAST_JSON_ALWAYS_INLINE bool
  FastJsonParser::Impl::Cursor::is_space_(char c) noexcept
  {
    return c == ' ' || static_cast<unsigned>(c - '\t') <= ('\r' - '\t');
  }

  AD_FAST_JSON_ALWAYS_INLINE bool
  FastJsonParser::Impl::Cursor::is_delimiter_(char c) noexcept
  {
    return c == ',' || c == ':' || c == ']' || c == '}' || is_space_(c);
  }

  AD_FAST_JSON_ALWAYS_INLINE bool
  FastJsonParser::Impl::Cursor::is_dec_(char c) noexcept
  {
    return static_cast<unsigned>(c - '0') <= 9;
  }

  inline bool
  FastJsonParser::Impl::Cursor::is_hex_(char c) noexcept
  {
    return is_dec_(c) ||
      static_cast<unsigned>((c | 0x20) - 'a') <= ('f' - 'a');
  }

  inline int
  FastJsonParser::Impl::Cursor::hex_to_int_(char c) noexcept
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
  FastJsonParser::Impl::Cursor::parse_string_tail_(std::string& out)
  {
    const std::size_t prefix_size = out.size();
    out.resize(prefix_size + static_cast<std::size_t>(end_ - pos_));
    char* const out_begin = out.data();
    char* out_pos = out_begin + prefix_size;

    append_escape_(out_pos);

    while(pos_ != end_)
    {
      char c = *pos_++;
      if(c == '"')
      {
        out.resize(static_cast<std::size_t>(out_pos - out_begin));
        return;
      }

      if(c == '\\')
      {
        append_escape_(out_pos);
      }
      else
      {
        *out_pos++ = c;
      }
    }

    throw_error("bad string");
  }

  inline void
  FastJsonParser::Impl::Cursor::append_escape_(char*& out)
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
      *out++ = c;
      return;
    case 'b':
      *out++ = '\b';
      return;
    case 'f':
      *out++ = '\f';
      return;
    case 'n':
      *out++ = '\n';
      return;
    case 'r':
      *out++ = '\r';
      return;
    case 't':
      *out++ = '\t';
      return;
    case 'u':
      append_unicode_escape_(out);
      return;
    default:
      throw_error("bad string");
    }
  }

  inline void
  FastJsonParser::Impl::Cursor::skip_escape_()
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
  FastJsonParser::Impl::Cursor::parse_unicode_escape_()
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
  FastJsonParser::Impl::Cursor::skip_unicode_escape_()
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
  FastJsonParser::Impl::Cursor::append_unicode_escape_(char*& out)
  {
    const uint32_t code = parse_unicode_escape_();
    if(code < 0x80)
    {
      *out++ = static_cast<char>(code);
    }
    else if(code < 0x800)
    {
      *out++ = static_cast<char>(0xC0 | (code >> 6));
      *out++ = static_cast<char>(0x80 | (code & 0x3F));
    }
    else
    {
      *out++ = static_cast<char>(0xE0 | (code >> 12));
      *out++ = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
      *out++ = static_cast<char>(0x80 | (code & 0x3F));
    }
  }

  template<bool Strict, bool UseSimd>
  inline void
  FastJsonParser::Impl::skip_object_(Cursor& cursor) const
  {
    if constexpr(!Strict)
    {
      cursor.skip_object_rough<UseSimd>();
      return;
    }

    cursor.skip_char();
    if(!cursor.eof() && cursor.peek() == '}')
    {
      cursor.get();
      return;
    }

    for(;;)
    {
      cursor.skip_spaces<UseSimd>();
      if(cursor.eof() || cursor.peek() != '"')
      {
        cursor.throw_error("object key expected");
      }

      cursor.skip_string<UseSimd>();
      cursor.skip_spaces<UseSimd>();
      cursor.expect(':');
      skip_value_<Strict, UseSimd>(cursor);

      cursor.skip_spaces<UseSimd>();
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

  template<bool Strict, bool UseSimd>
  inline void
  FastJsonParser::Impl::skip_array_(Cursor& cursor) const
  {
    if constexpr(!Strict)
    {
      cursor.skip_array_rough<UseSimd>();
      return;
    }

    cursor.skip_char();
    if(!cursor.eof() && cursor.peek() == ']')
    {
      cursor.get();
      return;
    }

    for(;;)
    {
      cursor.skip_spaces<UseSimd>();
      skip_value_<Strict, UseSimd>(cursor);

      cursor.skip_spaces<UseSimd>();
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

  template<bool Strict, bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser::Impl::skip_value_(Cursor& cursor) const
  {
    cursor.skip_spaces<UseSimd>();
    if(cursor.eof())
    {
      cursor.throw_error("unexpected end of JSON");
    }

    const char c = cursor.peek();
    if(c == '{')
    {
      skip_object_<Strict, UseSimd>(cursor);
    }
    else if(c == '[')
    {
      skip_array_<Strict, UseSimd>(cursor);
    }
    else if(c == '"')
    {
      if constexpr(Strict)
      {
        cursor.skip_string<UseSimd>();
      }
      else
      {
        cursor.skip_string_rough<UseSimd>();
      }
    }
    else if constexpr(!Strict)
    {
      cursor.skip_unquoted_value_rough<UseSimd>();
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

  template<bool Strict, bool UseSimd>
  inline void
  FastJsonParser::Impl::parse_value_iterative_(
    Cursor& cursor,
    const JsonTreeProcessor& processor,
    void* context,
    std::pmr::vector<ParseFrame>& frames) const
  {
    cursor.skip_spaces<UseSimd>();
    if(cursor.eof())
    {
      cursor.throw_error("unexpected end of JSON");
    }

    const char c = cursor.peek();
    if(c == '{')
    {
      cursor.skip_char();
      if(processor.value_processor)
      {
        processor.value_processor->object_started(processor.path, context);
      }

      frames.push_back({
        ParseFrameType::Object,
        ParseFrameState::ValueOrEnd,
        &processor});
    }
    else if(c == '[')
    {
      cursor.skip_char();
      if(processor.value_processor)
      {
        processor.value_processor->array_started(processor.path, context);
      }

      frames.push_back({
        ParseFrameType::Array,
        ParseFrameState::ValueOrEnd,
        &processor});
    }
    else if(c == '"')
    {
      if(processor.value_processor)
      {
        StringToken token = cursor.parse_string<UseSimd>();
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
        cursor.skip_string<UseSimd>();
      }
    }
    else if(c == '-' || (c >= '0' && c <= '9'))
    {
      if(processor.value_processor && processor.as_string)
      {
        processor.value_processor->process_string(
          cursor.scan_number_literal(),
          processor.path,
          context);
      }
      else
      {
        NumberToken number = cursor.parse_number();
        if(processor.value_processor)
        {
          processor.value_processor->process_number(
            number.value,
            number.is_float,
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
        if(processor.as_string)
        {
          processor.value_processor->process_string(
            std::string_view("true", 4),
            processor.path,
            context);
        }
        else
        {
          processor.value_processor->process_bool(
            true,
            processor.path,
            context);
        }
      }
    }
    else if(c == 'f')
    {
      cursor.consume_literal("false");
      if(processor.value_processor)
      {
        if(processor.as_string)
        {
          processor.value_processor->process_string(
            std::string_view("false", 5),
            processor.path,
            context);
        }
        else
        {
          processor.value_processor->process_bool(
            false,
            processor.path,
            context);
        }
      }
    }
    else if(c == 'n')
    {
      cursor.consume_literal("null");
      if(processor.value_processor)
      {
        if(processor.as_string)
        {
          processor.value_processor->process_string(
            std::string_view("null", 4),
            processor.path,
            context);
        }
        else
        {
          processor.value_processor->process_null(processor.path, context);
        }
      }
    }
    else
    {
      cursor.throw_error("unexpected character");
    }
  }

  template<bool Strict, bool UseSimd>
  inline void
  FastJsonParser::Impl::parse_iterative_(
    Cursor& cursor,
    void* context) const
  {
    alignas(std::max_align_t) char frame_buffer[4096];
    std::pmr::monotonic_buffer_resource frame_resource(
      frame_buffer,
      sizeof(frame_buffer));
    std::pmr::vector<ParseFrame> frames(&frame_resource);
    frames.reserve(32);

    cursor.skip_char();
    frames.push_back({
      ParseFrameType::Object,
      ParseFrameState::ValueOrEnd,
      &root_processor});

    while(!frames.empty())
    {
      ParseFrame& frame = frames.back();
      cursor.skip_spaces<UseSimd>();
      if(cursor.eof())
      {
        cursor.throw_error(
          frame.type == ParseFrameType::Object ?
            "unexpected end of object" :
            "unexpected end of array");
      }

      if(frame.state == ParseFrameState::DelimiterOrEnd)
      {
        const char delimiter = cursor.get_unchecked();
        if(frame.type == ParseFrameType::Object)
        {
          if(delimiter == '}')
          {
            frames.pop_back();
            continue;
          }

          if(delimiter != ',')
          {
            cursor.throw_error("expected ',' or '}'");
          }
        }
        else
        {
          if(delimiter == ']')
          {
            frames.pop_back();
            continue;
          }

          if(delimiter != ',')
          {
            cursor.throw_error("expected ',' or ']'");
          }
        }

        frame.state = ParseFrameState::ValueOrEnd;
        continue;
      }

      if(frame.type == ParseFrameType::Object)
      {
        if(cursor.peek() == '}')
        {
          cursor.get_unchecked();
          frames.pop_back();
          continue;
        }

        if(cursor.peek() != '"')
        {
          cursor.throw_error("object key expected");
        }

        StringToken key = cursor.parse_string<UseSimd>();
        cursor.skip_spaces<UseSimd>();
        if(cursor.eof() || cursor.peek() != ':')
        {
          cursor.throw_error("expected ':'");
        }
        cursor.skip_char();
        cursor.skip_spaces<UseSimd>();

        const JsonTreeProcessor* const child_processor =
          frame.processor->find_sub_processor(key.value, key.escaped);
        frame.state = ParseFrameState::DelimiterOrEnd;
        if(child_processor != nullptr)
        {
          parse_value_iterative_<Strict, UseSimd>(
            cursor,
            *child_processor,
            context,
            frames);
        }
        else
        {
          skip_value_<Strict, UseSimd>(cursor);
        }
      }
      else
      {
        if(cursor.peek() == ']')
        {
          cursor.get_unchecked();
          frames.pop_back();
          continue;
        }

        frame.state = ParseFrameState::DelimiterOrEnd;
        parse_value_iterative_<Strict, UseSimd>(
          cursor,
          *frame.processor,
          context,
          frames);
      }
    }
  }

  void
  FastJsonParser::Impl::parse(std::string_view json, void* context) const
  {
    parse_handler(*this, json, context);
  }

  template<bool Strict, bool UseSimd>
  void
  FastJsonParser::Impl::parse_(
    const Impl& impl,
    std::string_view json,
    void* context)
  {
    Cursor cursor(json);
    cursor.skip_spaces<UseSimd>();

    if(cursor.eof() || cursor.peek() != '{')
    {
      cursor.throw_error("top-level JSON value must be object");
    }

    impl.parse_iterative_<Strict, UseSimd>(cursor, context);
    cursor.skip_spaces<UseSimd>();

    if(!cursor.eof())
    {
      cursor.throw_error("unexpected trailing character");
    }
  }
}

#undef AD_FAST_JSON_ALWAYS_INLINE
