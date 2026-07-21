#include "FastJsonParser.hpp"

#include <Commons/MonoAllocator.hpp>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <type_traits>
#include <utility>

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
    template<typename StringType>
    struct HasDefaultStringCreator :
      std::bool_constant<std::is_default_constructible_v<StringType>>
    {};

    template<>
    struct HasDefaultStringCreator<MonoString> : std::false_type
    {};

    template<typename KeyType, typename ValueType>
    class MapHelper
    {
    public:
      enum Mode
      {
        SINGLE_ELEMENT_STORAGE,
        VECTOR_STORAGE,
        HASH_STORAGE
      };

      explicit
      MapHelper(MonoAllocatorArena* arena)
        : vector_storage_(arena),
          hash_storage_(arena)
      {}

      void
      emplace(KeyType&& key, ValueType&& value)
      {
        if(size_ == 0)
        {
          single_element_storage_.emplace(
            std::move(key),
            std::move(value));
          mode_ = SINGLE_ELEMENT_STORAGE;
          size_ = 1;
          return;
        }

        if(mode_ == SINGLE_ELEMENT_STORAGE)
        {
          if(single_element_storage_->first == key)
          {
            return;
          }

          vector_storage_.emplace_back(
            std::move(single_element_storage_->first),
            std::move(single_element_storage_->second));
          single_element_storage_.reset();
          mode_ = VECTOR_STORAGE;
        }

        if(mode_ == VECTOR_STORAGE)
        {
          if(vector_storage_.size() + 1 < HASH_STORAGE_MIN_SIZE)
          {
            auto it = lower_bound_(key);
            if(it != vector_storage_.end() && it->first == key)
            {
              return;
            }

            vector_storage_.emplace(
              it,
              std::move(key),
              std::move(value));
            ++size_;
            return;
          }

          hash_storage_.reserve(vector_storage_.size() + 1);
          for(auto& item : vector_storage_)
          {
            hash_storage_.emplace(
              std::move(item.first),
              std::move(item.second));
          }
          vector_storage_.clear();
          mode_ = HASH_STORAGE;
        }

        const auto inserted = hash_storage_.emplace(
          std::move(key),
          std::move(value));
        if(inserted.second)
        {
          ++size_;
        }
      }

      ValueType*
      find(const KeyType& key)
      {
        switch(mode_)
        {
        case SINGLE_ELEMENT_STORAGE:
          return single_element_storage_ && single_element_storage_->first == key ?
            &single_element_storage_->second :
            nullptr;
        case VECTOR_STORAGE:
          {
            auto it = lower_bound_(key);
            return it != vector_storage_.end() && it->first == key ?
              &it->second :
              nullptr;
          }
        case HASH_STORAGE:
          {
            auto it = hash_storage_.find(key);
            return it != hash_storage_.end() ? &it->second : nullptr;
          }
        }

        return nullptr;
      }

      const ValueType*
      find(const KeyType& key) const
      {
        switch(mode_)
        {
        case SINGLE_ELEMENT_STORAGE:
          return single_element_storage_ && single_element_storage_->first == key ?
            &single_element_storage_->second :
            nullptr;
        case VECTOR_STORAGE:
          {
            auto it = lower_bound_(key);
            return it != vector_storage_.end() && it->first == key ?
              &it->second :
              nullptr;
          }
        case HASH_STORAGE:
          {
            auto it = hash_storage_.find(key);
            return it != hash_storage_.end() ? &it->second : nullptr;
          }
        }

        return nullptr;
      }

    private:
      using Pair = std::pair<KeyType, ValueType>;
      using Vector = MonoVector<Pair>;

      static constexpr std::size_t HASH_STORAGE_MIN_SIZE = 8;

      typename Vector::iterator
      lower_bound_(const KeyType& key)
      {
        return std::lower_bound(
          vector_storage_.begin(),
          vector_storage_.end(),
          key,
          [] (const Pair& left, const KeyType& right)
          {
            return left.first < right;
          });
      }

      typename Vector::const_iterator
      lower_bound_(const KeyType& key) const
      {
        return std::lower_bound(
          vector_storage_.begin(),
          vector_storage_.end(),
          key,
          [] (const Pair& left, const KeyType& right)
          {
            return left.first < right;
          });
      }

      Mode mode_ = SINGLE_ELEMENT_STORAGE;
      std::size_t size_ = 0;
      std::optional<Pair> single_element_storage_;
      Vector vector_storage_;
      MonoUnorderedMap<KeyType, ValueType> hash_storage_;
    };

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

  template<typename StringType>
  struct FastJsonParser<StringType>::Impl
  {
    using StringCreator = StringType (*)(void*);

    struct JsonTreeProcessor
    {
      explicit
      JsonTreeProcessor(MonoAllocatorArena* arena);

      JsonTreeProcessor(
        std::string path_val,
        MonoAllocatorArena* arena);

      static std::uint64_t
      short_key_(std::string_view key) noexcept;

      JsonTreeProcessor*
      get_or_add_sub_processor(std::string_view key);

      JsonTreeProcessor*
      find_sub_processor(std::string_view key, bool escaped);

      const JsonTreeProcessor*
      find_sub_processor(std::string_view key, bool escaped) const;

      struct ChildProcessor
      {
        ChildProcessor(
          std::string_view key_val,
          std::unique_ptr<JsonTreeProcessor>&& processor_val,
          MonoAllocatorArena* arena);

        MonoString key;
        std::unique_ptr<JsonTreeProcessor> processor;
      };

      void
      build_sub_processors_();

      MonoAllocatorArena* arena;
      MonoVector<ChildProcessor> child_processors;
      MapHelper<std::string_view, JsonTreeProcessor*> sub_processors;
      MapHelper<std::uint64_t, JsonTreeProcessor*> short_processors;
      std::shared_ptr<ValueProcessor> value_processor;
      std::string path;
      bool as_string = false;
    };

    struct StringToken
    {
      std::string_view value;
      std::optional<StringType> unescaped;
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
      Cursor(
        std::string_view json,
        StringCreator string_creator,
        void* string_creator_context);

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
      parse_string_tail_(StringType& out);

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
      StringCreator string_creator_;
      void* string_creator_context_;
    };

    explicit
    Impl(
      typename FastJsonParser<StringType>::ProcessorSet&& processors,
      bool strict_val,
      bool use_simd_val);

    void
    add_processor_(
      std::string_view path,
      std::shared_ptr<ValueProcessor> processor,
      bool as_string);

    void
    parse(
      std::string_view json,
      void* context,
      StringCreator string_creator,
      void* string_creator_context) const;

    template<bool Strict, bool UseSimd>
    static void
    parse_(
      const Impl& impl,
      std::string_view json,
      void* context,
      StringCreator string_creator,
      void* string_creator_context);

    template<bool Strict, bool UseSimd>
    void
    parse_iterative_(Cursor& cursor, void* context) const;

    template<bool Strict, bool UseSimd>
    void
    parse_value_iterative_(
      Cursor& cursor,
      const JsonTreeProcessor& processor,
      void* context,
      MonoVector<ParseFrame>& frames) const;

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
      void* context,
      StringCreator string_creator,
      void* string_creator_context);

    MonoAllocatorArena processor_arena;
    JsonTreeProcessor root_processor;
    ParseHandler parse_handler = nullptr;
  };

  template<typename StringType>
  void
  FastJsonParser<StringType>::ProcessorSet::add_processor(
    std::string_view path,
    std::shared_ptr<ValueProcessor> processor,
    bool as_string)
  {
    entries_.push_back({
      std::string(path),
      std::move(processor),
      as_string});
  }

  template<typename StringType>
  void
  FastJsonParser<StringType>::ValueProcessor::object_started(
    std::string_view,
    void*) const
  {}

  template<typename StringType>
  void
  FastJsonParser<StringType>::ValueProcessor::array_started(
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected array in " + std::string(path));
  }

  template<typename StringType>
  void
  FastJsonParser<StringType>::ValueProcessor::process_integer(
    int64_t,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected integer in " + std::string(path));
  }

  template<typename StringType>
  void
  FastJsonParser<StringType>::ValueProcessor::process_float(
    double,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected float in " + std::string(path));
  }

  template<typename StringType>
  void
  FastJsonParser<StringType>::ValueProcessor::process_number(
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

  template<typename StringType>
  void
  FastJsonParser<StringType>::ValueProcessor::process_string(
    std::string_view value,
    std::string_view path,
    void* context) const
  {
    if constexpr(HasDefaultStringCreator<StringType>::value)
    {
      StringType string_value;
      string_value.assign(value.data(), value.size());
      process_string(std::move(string_value), path, context);
    }
    else
    {
      throw UnexpectedType("unexpected string in " + std::string(path));
    }
  }

  template<typename StringType>
  void
  FastJsonParser<StringType>::ValueProcessor::process_string(
    StringType&&,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected string in " + std::string(path));
  }

  template<typename StringType>
  void
  FastJsonParser<StringType>::ValueProcessor::process_bool(
    bool,
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected bool in " + std::string(path));
  }

  template<typename StringType>
  void
  FastJsonParser<StringType>::ValueProcessor::process_null(
    std::string_view path,
    void*) const
  {
    throw UnexpectedType("unexpected null in " + std::string(path));
  }

  template<typename StringType>
  FastJsonParser<StringType>::FastJsonParser(bool strict)
    : FastJsonParser(ProcessorSet(), strict)
  {}

  template<typename StringType>
  FastJsonParser<StringType>::FastJsonParser(
    ProcessorSet&& processors,
    bool strict)
    : impl_(std::make_unique<Impl>(
        std::move(processors),
        strict,
        simd_supported_()))
  {}

  template<typename StringType>
  FastJsonParser<StringType>::~FastJsonParser() noexcept = default;

  template<typename StringType>
  void
  FastJsonParser<StringType>::parse(std::string_view json, void* context) const
  {
    if constexpr(HasDefaultStringCreator<StringType>::value)
    {
      parse(
        json,
        context,
        []()
        {
          return StringType();
        });
    }
    else
    {
      throw Exception(
        "FastJsonParser::parse without string creator requires "
        "default constructible string type");
    }
  }

  template<typename StringType>
  void
  FastJsonParser<StringType>::parse_(
    std::string_view json,
    void* context,
    typename FastJsonParser<StringType>::StringCreator string_creator,
    void* string_creator_context) const
  {
    impl_->parse(
      json,
      context,
      string_creator,
      string_creator_context);
  }

  template<typename StringType>
  FastJsonParser<StringType>::Impl::JsonTreeProcessor::JsonTreeProcessor(
    MonoAllocatorArena* arena_val)
    : arena(arena_val),
      child_processors(arena),
      sub_processors(arena),
      short_processors(arena)
  {}

  template<typename StringType>
  FastJsonParser<StringType>::Impl::JsonTreeProcessor::JsonTreeProcessor(
    std::string path_val,
    MonoAllocatorArena* arena_val)
    : JsonTreeProcessor(arena_val)
  {
    path = std::move(path_val);
  }

  template<typename StringType>
  FastJsonParser<StringType>::Impl::JsonTreeProcessor::ChildProcessor::ChildProcessor(
    std::string_view key_val,
    std::unique_ptr<JsonTreeProcessor>&& processor_val,
    MonoAllocatorArena* arena)
    : key(key_val.data(), key_val.size(), arena),
      processor(std::move(processor_val))
  {}

  template<typename StringType>
  inline std::uint64_t
  FastJsonParser<StringType>::Impl::JsonTreeProcessor::short_key_(
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

  template<typename StringType>
  typename FastJsonParser<StringType>::Impl::JsonTreeProcessor*
  FastJsonParser<StringType>::Impl::JsonTreeProcessor::get_or_add_sub_processor(
    std::string_view key)
  {
    for(auto& child : child_processors)
    {
      if(std::string_view(child.key.data(), child.key.size()) == key)
      {
        return child.processor.get();
      }
    }

    std::string child_path = path.empty() ?
      std::string(key) :
      path + "." + std::string(key);

    auto child = std::make_unique<JsonTreeProcessor>(
      std::move(child_path),
      arena);
    JsonTreeProcessor* const child_processor = child.get();
    child_processors.emplace_back(
      key,
      std::move(child),
      arena);
    return child_processor;
  }

  template<typename StringType>
  void
  FastJsonParser<StringType>::Impl::JsonTreeProcessor::build_sub_processors_()
  {
    for(auto& child : child_processors)
    {
      const std::string_view key(child.key.data(), child.key.size());
      child.processor->build_sub_processors_();
      sub_processors.emplace(
        std::string_view(key),
        child.processor.get());

      if(key.size() <= sizeof(std::uint64_t))
      {
        short_processors.emplace(
          short_key_(key),
          child.processor.get());
      }
    }
  }

  template<typename StringType>
  typename FastJsonParser<StringType>::Impl::JsonTreeProcessor*
  FastJsonParser<StringType>::Impl::JsonTreeProcessor::find_sub_processor(
    std::string_view key,
    bool escaped)
  {
    if(!escaped && key.size() <= sizeof(std::uint64_t))
    {
      JsonTreeProcessor** const processor =
        short_processors.find(short_key_(key));
      if(processor != nullptr)
      {
        return *processor;
      }
      return nullptr;
    }

    JsonTreeProcessor** const processor = sub_processors.find(key);
    return processor != nullptr ? *processor : nullptr;
  }

  template<typename StringType>
  const typename FastJsonParser<StringType>::Impl::JsonTreeProcessor*
  FastJsonParser<StringType>::Impl::JsonTreeProcessor::find_sub_processor(
    std::string_view key,
    bool escaped) const
  {
    if(!escaped && key.size() <= sizeof(std::uint64_t))
    {
      JsonTreeProcessor* const* const processor =
        short_processors.find(short_key_(key));
      if(processor != nullptr)
      {
        return *processor;
      }
      return nullptr;
    }

    JsonTreeProcessor* const* const processor = sub_processors.find(key);
    return processor != nullptr ? *processor : nullptr;
  }

  template<typename StringType>
  FastJsonParser<StringType>::Impl::Impl(
    typename FastJsonParser<StringType>::ProcessorSet&& processors,
    bool strict_val,
    bool use_simd_val)
    : root_processor(&processor_arena)
  {
    for(auto& entry : processors.entries_)
    {
      add_processor_(
        entry.path,
        std::move(entry.processor),
        entry.as_string);
    }
    root_processor.build_sub_processors_();

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

  template<typename StringType>
  void
  FastJsonParser<StringType>::Impl::add_processor_(
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

      current = current->get_or_add_sub_processor(key);

      if(next_pos == std::string_view::npos)
      {
        break;
      }
      pos = next_pos + 1;
    }

    current->value_processor = std::move(processor);
    current->as_string = as_string;
  }

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE
  FastJsonParser<StringType>::Impl::Cursor::Cursor(
    std::string_view json,
    typename FastJsonParser<StringType>::Impl::StringCreator string_creator,
    void* string_creator_context)
    : begin_(json.data()),
      pos_(json.data()),
      end_(json.data() + json.size()),
      string_creator_(string_creator),
      string_creator_context_(string_creator_context)
  {}

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE bool
  FastJsonParser<StringType>::Impl::Cursor::eof() const noexcept
  {
    return pos_ == end_;
  }

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE char
  FastJsonParser<StringType>::Impl::Cursor::peek() const
  {
    return *pos_;
  }

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE char
  FastJsonParser<StringType>::Impl::Cursor::get()
  {
    if(eof())
    {
      throw_error("unexpected end of JSON");
    }
    return *pos_++;
  }

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE char
  FastJsonParser<StringType>::Impl::Cursor::get_unchecked() noexcept
  {
    return *pos_++;
  }

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser<StringType>::Impl::Cursor::skip_char() noexcept
  {
    ++pos_;
  }

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser<StringType>::Impl::Cursor::expect(char expected)
  {
    if(eof() || *pos_ != expected)
    {
      throw_error(std::string("expected '") + expected + "'");
    }
    ++pos_;
  }

  template<typename StringType>
  template<bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser<StringType>::Impl::Cursor::skip_spaces() noexcept
  {
    if(pos_ == end_ || !is_space_(*pos_))
    {
      return;
    }
    pos_ = find_non_space_<UseSimd>(pos_ + 1, end_);
  }

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser<StringType>::Impl::Cursor::consume_literal(std::string_view literal)
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

  template<typename StringType>
  template<bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE typename FastJsonParser<StringType>::Impl::StringToken
  FastJsonParser<StringType>::Impl::Cursor::parse_string()
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

    StringToken token{
      {},
      std::optional<StringType>(
        std::in_place,
        string_creator_(string_creator_context_)),
      true};
    token.unescaped->assign(value_begin, special - value_begin);
    parse_string_tail_(*token.unescaped);
    token.value = *token.unescaped;
    return token;
  }

  template<typename StringType>
  template<bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser<StringType>::Impl::Cursor::skip_string()
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

  template<typename StringType>
  template<bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser<StringType>::Impl::Cursor::skip_string_rough()
  {
    skip_char();
    skip_string_rough_after_quote_<UseSimd>();
  }

  template<typename StringType>
  template<bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser<StringType>::Impl::Cursor::skip_unquoted_value_rough()
  {
    pos_ = find_unquoted_delimiter_<UseSimd>(pos_, end_);
  }

  template<typename StringType>
  template<bool UseSimd>
  inline void
  FastJsonParser<StringType>::Impl::Cursor::skip_string_rough_after_quote_()
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

  template<typename StringType>
  template<bool UseSimd>
  inline void
  FastJsonParser<StringType>::Impl::Cursor::skip_object_rough()
  {
    skip_compound_rough_<UseSimd>('{', "unexpected end of object");
  }

  template<typename StringType>
  template<bool UseSimd>
  inline void
  FastJsonParser<StringType>::Impl::Cursor::skip_array_rough()
  {
    skip_compound_rough_<UseSimd>('[', "unexpected end of array");
  }

  template<typename StringType>
  template<bool UseSimd>
  inline void
  FastJsonParser<StringType>::Impl::Cursor::skip_compound_rough_(
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

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE typename FastJsonParser<StringType>::Impl::NumberToken
  FastJsonParser<StringType>::Impl::Cursor::parse_number()
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

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE std::string_view
  FastJsonParser<StringType>::Impl::Cursor::scan_number_literal()
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

  template<typename StringType>
  inline void
  FastJsonParser<StringType>::Impl::Cursor::throw_error(
    const std::string& message) const
  {
    throw ParseError(
      message + " at pos " + std::to_string(pos_ - begin_));
  }

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE bool
  FastJsonParser<StringType>::Impl::Cursor::is_space_(char c) noexcept
  {
    return c == ' ' || static_cast<unsigned>(c - '\t') <= ('\r' - '\t');
  }

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE bool
  FastJsonParser<StringType>::Impl::Cursor::is_delimiter_(char c) noexcept
  {
    return c == ',' || c == ':' || c == ']' || c == '}' || is_space_(c);
  }

  template<typename StringType>
  AD_FAST_JSON_ALWAYS_INLINE bool
  FastJsonParser<StringType>::Impl::Cursor::is_dec_(char c) noexcept
  {
    return static_cast<unsigned>(c - '0') <= 9;
  }

  template<typename StringType>
  inline bool
  FastJsonParser<StringType>::Impl::Cursor::is_hex_(char c) noexcept
  {
    return is_dec_(c) ||
      static_cast<unsigned>((c | 0x20) - 'a') <= ('f' - 'a');
  }

  template<typename StringType>
  inline int
  FastJsonParser<StringType>::Impl::Cursor::hex_to_int_(char c) noexcept
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

  template<typename StringType>
  inline void
  FastJsonParser<StringType>::Impl::Cursor::parse_string_tail_(StringType& out)
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

  template<typename StringType>
  inline void
  FastJsonParser<StringType>::Impl::Cursor::append_escape_(char*& out)
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

  template<typename StringType>
  inline void
  FastJsonParser<StringType>::Impl::Cursor::skip_escape_()
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

  template<typename StringType>
  inline uint32_t
  FastJsonParser<StringType>::Impl::Cursor::parse_unicode_escape_()
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

  template<typename StringType>
  inline void
  FastJsonParser<StringType>::Impl::Cursor::skip_unicode_escape_()
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

  template<typename StringType>
  inline void
  FastJsonParser<StringType>::Impl::Cursor::append_unicode_escape_(char*& out)
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

  template<typename StringType>
  template<bool Strict, bool UseSimd>
  inline void
  FastJsonParser<StringType>::Impl::skip_object_(Cursor& cursor) const
  {
    if constexpr(!Strict)
    {
      cursor.template skip_object_rough<UseSimd>();
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
      cursor.template skip_spaces<UseSimd>();
      if(cursor.eof() || cursor.peek() != '"')
      {
        cursor.throw_error("object key expected");
      }

      cursor.template skip_string<UseSimd>();
      cursor.template skip_spaces<UseSimd>();
      cursor.expect(':');
      skip_value_<Strict, UseSimd>(cursor);

      cursor.template skip_spaces<UseSimd>();
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

  template<typename StringType>
  template<bool Strict, bool UseSimd>
  inline void
  FastJsonParser<StringType>::Impl::skip_array_(Cursor& cursor) const
  {
    if constexpr(!Strict)
    {
      cursor.template skip_array_rough<UseSimd>();
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
      cursor.template skip_spaces<UseSimd>();
      skip_value_<Strict, UseSimd>(cursor);

      cursor.template skip_spaces<UseSimd>();
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

  template<typename StringType>
  template<bool Strict, bool UseSimd>
  AD_FAST_JSON_ALWAYS_INLINE void
  FastJsonParser<StringType>::Impl::skip_value_(Cursor& cursor) const
  {
    cursor.template skip_spaces<UseSimd>();
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
        cursor.template skip_string<UseSimd>();
      }
      else
      {
        cursor.template skip_string_rough<UseSimd>();
      }
    }
    else if constexpr(!Strict)
    {
      cursor.template skip_unquoted_value_rough<UseSimd>();
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

  template<typename StringType>
  template<bool Strict, bool UseSimd>
  inline void
  FastJsonParser<StringType>::Impl::parse_value_iterative_(
    Cursor& cursor,
    const JsonTreeProcessor& processor,
    void* context,
    MonoVector<ParseFrame>& frames) const
  {
    cursor.template skip_spaces<UseSimd>();
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
        StringToken token = cursor.template parse_string<UseSimd>();
        if(token.escaped)
        {
          processor.value_processor->process_string(
            std::move(*token.unescaped),
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
        cursor.template skip_string<UseSimd>();
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

  template<typename StringType>
  template<bool Strict, bool UseSimd>
  inline void
  FastJsonParser<StringType>::Impl::parse_iterative_(
    Cursor& cursor,
    void* context) const
  {
    alignas(std::max_align_t) char frame_buffer[4096];
    MonoAllocatorArena frame_arena(
      frame_buffer,
      sizeof(frame_buffer));
    MonoVector<ParseFrame> frames(&frame_arena);
    frames.reserve(32);

    cursor.skip_char();
    frames.push_back({
      ParseFrameType::Object,
      ParseFrameState::ValueOrEnd,
      &root_processor});

    while(!frames.empty())
    {
      ParseFrame& frame = frames.back();
      cursor.template skip_spaces<UseSimd>();
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

        StringToken key = cursor.template parse_string<UseSimd>();
        cursor.template skip_spaces<UseSimd>();
        if(cursor.eof() || cursor.peek() != ':')
        {
          cursor.throw_error("expected ':'");
        }
        cursor.skip_char();
        cursor.template skip_spaces<UseSimd>();

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

  template<typename StringType>
  void
  FastJsonParser<StringType>::Impl::parse(
    std::string_view json,
    void* context,
    typename FastJsonParser<StringType>::Impl::StringCreator string_creator,
    void* string_creator_context) const
  {
    parse_handler(
      *this,
      json,
      context,
      string_creator,
      string_creator_context);
  }

  template<typename StringType>
  template<bool Strict, bool UseSimd>
  void
  FastJsonParser<StringType>::Impl::parse_(
    const typename FastJsonParser<StringType>::Impl& impl,
    std::string_view json,
    void* context,
    typename FastJsonParser<StringType>::Impl::StringCreator string_creator,
    void* string_creator_context)
  {
    Cursor cursor(json, string_creator, string_creator_context);
    cursor.template skip_spaces<UseSimd>();

    if(cursor.eof() || cursor.peek() != '{')
    {
      cursor.throw_error("top-level JSON value must be object");
    }

    impl.parse_iterative_<Strict, UseSimd>(cursor, context);
    cursor.template skip_spaces<UseSimd>();

    if(!cursor.eof())
    {
      cursor.throw_error("unexpected trailing character");
    }
  }

  template class FastJsonParser<std::string>;
  template class FastJsonParser<MonoString>;
}

#undef AD_FAST_JSON_ALWAYS_INLINE
