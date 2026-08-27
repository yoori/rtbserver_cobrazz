#include "FastJsonParserSimd.hpp"

#include <immintrin.h>

namespace AdServer::Commons::FastJsonParserSimd
{
  const char*
  find_non_space_avx2(const char* pos, const char* end) noexcept
  {
    const __m256i space = _mm256_set1_epi8(' ');
    const __m256i tab = _mm256_set1_epi8('\t');
    const __m256i line_feed = _mm256_set1_epi8('\n');
    const __m256i vertical_tab = _mm256_set1_epi8('\v');
    const __m256i form_feed = _mm256_set1_epi8('\f');
    const __m256i carriage_return = _mm256_set1_epi8('\r');

    while (static_cast<std::size_t>(end - pos) >= 32)
    {
      const __m256i bytes = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pos));
      const __m256i blank_matches = _mm256_or_si256(
        _mm256_cmpeq_epi8(bytes, space),
        _mm256_cmpeq_epi8(bytes, tab));
      const __m256i line_matches = _mm256_or_si256(
        _mm256_cmpeq_epi8(bytes, line_feed),
        _mm256_cmpeq_epi8(bytes, carriage_return));
      const __m256i other_matches = _mm256_or_si256(
        _mm256_cmpeq_epi8(bytes, vertical_tab),
        _mm256_cmpeq_epi8(bytes, form_feed));
      const unsigned space_mask = static_cast<unsigned>(
        _mm256_movemask_epi8(
          _mm256_or_si256(blank_matches, _mm256_or_si256(line_matches, other_matches))));
      if (space_mask != 0xFFFFFFFFu)
      {
        return pos + __builtin_ctz(~space_mask);
      }

      pos += 32;
    }

    return find_non_space_sse2(pos, end);
  }

  const char*
  find_string_special_avx2(const char* pos, const char* end) noexcept
  {
    const __m256i quote = _mm256_set1_epi8('"');
    const __m256i backslash = _mm256_set1_epi8('\\');

    while (static_cast<std::size_t>(end - pos) >= 32)
    {
      const __m256i bytes = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pos));
      const __m256i matches = _mm256_or_si256(
        _mm256_cmpeq_epi8(bytes, quote),
        _mm256_cmpeq_epi8(bytes, backslash));
      const unsigned mask = static_cast<unsigned>(_mm256_movemask_epi8(matches));
      if (mask != 0)
      {
        return pos + __builtin_ctz(mask);
      }

      pos += 32;
    }

    return find_string_special_sse2(pos, end);
  }

  const char*
  find_quote_avx2(const char* pos, const char* end) noexcept
  {
    const __m256i quote = _mm256_set1_epi8('"');

    while (static_cast<std::size_t>(end - pos) >= 32)
    {
      const __m256i bytes = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pos));
      const unsigned mask = static_cast<unsigned>(
        _mm256_movemask_epi8(_mm256_cmpeq_epi8(bytes, quote)));
      if (mask != 0)
      {
        return pos + __builtin_ctz(mask);
      }

      pos += 32;
    }

    return find_quote_sse2(pos, end);
  }

  const char*
  find_compound_special_avx2(const char* pos, const char* end) noexcept
  {
    const __m256i quote = _mm256_set1_epi8('"');
    const __m256i object_open = _mm256_set1_epi8('{');
    const __m256i object_close = _mm256_set1_epi8('}');
    const __m256i array_open = _mm256_set1_epi8('[');
    const __m256i array_close = _mm256_set1_epi8(']');

    while (static_cast<std::size_t>(end - pos) >= 32)
    {
      const __m256i bytes = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pos));
      const __m256i object_matches = _mm256_or_si256(
        _mm256_cmpeq_epi8(bytes, object_open),
        _mm256_cmpeq_epi8(bytes, object_close));
      const __m256i array_matches = _mm256_or_si256(
        _mm256_cmpeq_epi8(bytes, array_open),
        _mm256_cmpeq_epi8(bytes, array_close));
      const __m256i matches = _mm256_or_si256(
        _mm256_cmpeq_epi8(bytes, quote),
        _mm256_or_si256(object_matches, array_matches));
      const unsigned mask = static_cast<unsigned>(_mm256_movemask_epi8(matches));
      if (mask != 0)
      {
        return pos + __builtin_ctz(mask);
      }

      pos += 32;
    }

    return find_compound_special_sse2(pos, end);
  }

  const char*
  find_unquoted_delimiter_avx2(const char* pos, const char* end) noexcept
  {
    const __m256i comma = _mm256_set1_epi8(',');
    const __m256i array_close = _mm256_set1_epi8(']');
    const __m256i object_close = _mm256_set1_epi8('}');

    while (static_cast<std::size_t>(end - pos) >= 32)
    {
      const __m256i bytes = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pos));
      const __m256i matches = _mm256_or_si256(
        _mm256_cmpeq_epi8(bytes, comma),
        _mm256_or_si256(
          _mm256_cmpeq_epi8(bytes, array_close),
          _mm256_cmpeq_epi8(bytes, object_close)));
      const unsigned mask = static_cast<unsigned>(_mm256_movemask_epi8(matches));
      if (mask != 0)
      {
        return pos + __builtin_ctz(mask);
      }

      pos += 32;
    }

    return find_unquoted_delimiter_sse2(pos, end);
  }
}
