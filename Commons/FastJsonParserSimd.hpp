#pragma once

#include <cstddef>
#include <cstdint>

namespace AdServer::Commons::FastJsonParserSimd
{
  enum class Level
  {
    SCALAR,
    SSE2,
    AVX2,
    AVX512BW
  };

  inline bool
  is_space(char c) noexcept
  {
    return c == ' ' || static_cast<unsigned>(c - '\t') <= ('\r' - '\t');
  }

  inline bool
  is_string_special(char c) noexcept
  {
    return c == '"' || c == '\\';
  }

  inline bool
  is_compound_special(char c) noexcept
  {
    return c == '"' || c == '{' || c == '}' || c == '[' || c == ']';
  }

  inline bool
  is_unquoted_delimiter(char c) noexcept
  {
    return c == ',' || c == ']' || c == '}';
  }

  inline const char*
  find_non_space_scalar(const char* pos, const char* end) noexcept
  {
    while(pos != end && is_space(*pos))
    {
      ++pos;
    }

    return pos;
  }

  inline const char*
  find_string_special_scalar(const char* pos, const char* end) noexcept
  {
    while(pos != end && !is_string_special(*pos))
    {
      ++pos;
    }

    return pos;
  }

  inline const char*
  find_quote_scalar(const char* pos, const char* end) noexcept
  {
    while(pos != end && *pos != '"')
    {
      ++pos;
    }

    return pos;
  }

  inline const char*
  find_compound_special_scalar(const char* pos, const char* end) noexcept
  {
    while(pos != end && !is_compound_special(*pos))
    {
      ++pos;
    }

    return pos;
  }

  inline const char*
  find_unquoted_delimiter_scalar(const char* pos, const char* end) noexcept
  {
    while(pos != end && !is_unquoted_delimiter(*pos))
    {
      ++pos;
    }

    return pos;
  }

  const char*
  find_non_space_sse2(const char* pos, const char* end) noexcept;

  const char*
  find_string_special_sse2(const char* pos, const char* end) noexcept;

  const char*
  find_quote_sse2(const char* pos, const char* end) noexcept;

  const char*
  find_compound_special_sse2(const char* pos, const char* end) noexcept;

  const char*
  find_unquoted_delimiter_sse2(const char* pos, const char* end) noexcept;

#if defined(AD_FAST_JSON_PARSER_HAS_AVX2)
  const char*
  find_non_space_avx2(const char* pos, const char* end) noexcept;

  const char*
  find_string_special_avx2(const char* pos, const char* end) noexcept;

  const char*
  find_quote_avx2(const char* pos, const char* end) noexcept;

  const char*
  find_compound_special_avx2(const char* pos, const char* end) noexcept;

  const char*
  find_unquoted_delimiter_avx2(const char* pos, const char* end) noexcept;
#endif

#if defined(AD_FAST_JSON_PARSER_HAS_AVX512BW)
  const char*
  find_non_space_avx512bw(const char* pos, const char* end) noexcept;

  const char*
  find_string_special_avx512bw(const char* pos, const char* end) noexcept;

  const char*
  find_quote_avx512bw(const char* pos, const char* end) noexcept;

  const char*
  find_compound_special_avx512bw(const char* pos, const char* end) noexcept;

  const char*
  find_unquoted_delimiter_avx512bw(const char* pos, const char* end) noexcept;
#endif
}
