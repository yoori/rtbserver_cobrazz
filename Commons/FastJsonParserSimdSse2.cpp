#include "FastJsonParserSimd.hpp"

#include <emmintrin.h>

namespace AdServer::Commons::FastJsonParserSimd
{
  const char*
  find_non_space_sse2(const char* pos, const char* end) noexcept
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
          _mm_or_si128(
            blank_matches,
            _mm_or_si128(line_matches, other_matches))));
      if(space_mask != 0xFFFFu)
      {
        return pos + __builtin_ctz((~space_mask) & 0xFFFFu);
      }

      pos += 16;
    }

    return find_non_space_scalar(pos, end);
  }

  const char*
  find_string_special_sse2(const char* pos, const char* end) noexcept
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

    return find_string_special_scalar(pos, end);
  }

  const char*
  find_quote_sse2(const char* pos, const char* end) noexcept
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

    return find_quote_scalar(pos, end);
  }

  const char*
  find_compound_special_sse2(const char* pos, const char* end) noexcept
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

    return find_compound_special_scalar(pos, end);
  }

  const char*
  find_unquoted_delimiter_sse2(const char* pos, const char* end) noexcept
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

    return find_unquoted_delimiter_scalar(pos, end);
  }
}
