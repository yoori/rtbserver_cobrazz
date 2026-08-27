#include "FastJsonParserSimd.hpp"

#include <immintrin.h>

namespace AdServer::Commons::FastJsonParserSimd
{
  const char*
  find_non_space_avx512bw(const char* pos, const char* end) noexcept
  {
    const __m512i space = _mm512_set1_epi8(' ');
    const __m512i tab = _mm512_set1_epi8('\t');
    const __m512i line_feed = _mm512_set1_epi8('\n');
    const __m512i vertical_tab = _mm512_set1_epi8('\v');
    const __m512i form_feed = _mm512_set1_epi8('\f');
    const __m512i carriage_return = _mm512_set1_epi8('\r');

    while (static_cast<std::size_t>(end - pos) >= 64)
    {
      const __m512i bytes = _mm512_loadu_si512(pos);
      const std::uint64_t space_mask =
        _mm512_cmpeq_epi8_mask(bytes, space) |
        _mm512_cmpeq_epi8_mask(bytes, tab) |
        _mm512_cmpeq_epi8_mask(bytes, line_feed) |
        _mm512_cmpeq_epi8_mask(bytes, vertical_tab) |
        _mm512_cmpeq_epi8_mask(bytes, form_feed) |
        _mm512_cmpeq_epi8_mask(bytes, carriage_return);

      if (space_mask != ~std::uint64_t(0))
      {
        return pos + __builtin_ctzll(~space_mask);
      }

      pos += 64;
    }

    return find_non_space_avx2(pos, end);
  }

  const char*
  find_string_special_avx512bw(const char* pos, const char* end) noexcept
  {
    const __m512i quote = _mm512_set1_epi8('"');
    const __m512i backslash = _mm512_set1_epi8('\\');

    while (static_cast<std::size_t>(end - pos) >= 64)
    {
      const __m512i bytes = _mm512_loadu_si512(pos);
      const std::uint64_t mask =
        _mm512_cmpeq_epi8_mask(bytes, quote) |
        _mm512_cmpeq_epi8_mask(bytes, backslash);
      if (mask != 0)
      {
        return pos + __builtin_ctzll(mask);
      }

      pos += 64;
    }

    return find_string_special_avx2(pos, end);
  }

  const char*
  find_quote_avx512bw(const char* pos, const char* end) noexcept
  {
    const __m512i quote = _mm512_set1_epi8('"');

    while (static_cast<std::size_t>(end - pos) >= 64)
    {
      const __m512i bytes = _mm512_loadu_si512(pos);
      const std::uint64_t mask = _mm512_cmpeq_epi8_mask(bytes, quote);
      if (mask != 0)
      {
        return pos + __builtin_ctzll(mask);
      }

      pos += 64;
    }

    return find_quote_avx2(pos, end);
  }

  const char*
  find_compound_special_avx512bw(const char* pos, const char* end) noexcept
  {
    const __m512i quote = _mm512_set1_epi8('"');
    const __m512i object_open = _mm512_set1_epi8('{');
    const __m512i object_close = _mm512_set1_epi8('}');
    const __m512i array_open = _mm512_set1_epi8('[');
    const __m512i array_close = _mm512_set1_epi8(']');

    while (static_cast<std::size_t>(end - pos) >= 64)
    {
      const __m512i bytes = _mm512_loadu_si512(pos);
      const std::uint64_t mask =
        _mm512_cmpeq_epi8_mask(bytes, quote) |
        _mm512_cmpeq_epi8_mask(bytes, object_open) |
        _mm512_cmpeq_epi8_mask(bytes, object_close) |
        _mm512_cmpeq_epi8_mask(bytes, array_open) |
        _mm512_cmpeq_epi8_mask(bytes, array_close);
      if (mask != 0)
      {
        return pos + __builtin_ctzll(mask);
      }

      pos += 64;
    }

    return find_compound_special_avx2(pos, end);
  }

  const char*
  find_unquoted_delimiter_avx512bw(const char* pos, const char* end) noexcept
  {
    const __m512i comma = _mm512_set1_epi8(',');
    const __m512i array_close = _mm512_set1_epi8(']');
    const __m512i object_close = _mm512_set1_epi8('}');

    while (static_cast<std::size_t>(end - pos) >= 64)
    {
      const __m512i bytes = _mm512_loadu_si512(pos);
      const std::uint64_t mask =
        _mm512_cmpeq_epi8_mask(bytes, comma) |
        _mm512_cmpeq_epi8_mask(bytes, array_close) |
        _mm512_cmpeq_epi8_mask(bytes, object_close);
      if (mask != 0)
      {
        return pos + __builtin_ctzll(mask);
      }

      pos += 64;
    }

    return find_unquoted_delimiter_avx2(pos, end);
  }
}
