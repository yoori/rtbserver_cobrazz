#ifndef DECIMALUTILS_HPP
#define DECIMALUTILS_HPP

#include <Generics/CommonDecimal.hpp>
#include <String/SubString.hpp>
#include <Stream/MemoryStream.hpp>

namespace AdServer
{
namespace Commons
{
  const char*
  find_end_of_number(const char* str)
    noexcept;

  /*
   * This function converts string to a DecimalType (Decimal/SimpleDecimal).
   * If str precision is more that DecimalType precision then function rounds number.
   * Function supports format with mantis and exponent.
   * If DecimalType doesn't support str exponent this function will throw DecimalType::Overflow exception.
   * @param str[in] - string to convert;
   * @param round_type[in] - round type;
   * @param check_format[in] - if false then function processes str until first invalid charachter or str end,
   *                           if true then functon will throw DecimalType::NotNumber exception if invalid charachter meets;
   * @return str converted to DecimalType.
   */
  template<typename DecimalType>
  DecimalType
  extract_decimal(
    const String::SubString& str,
    Generics::DecimalMulRemainder round_type = Generics::DMR_ROUND,
    bool check_format = false) /*throw(typename DecimalType::Overflow, typename DecimalType::NotNumber)*/;
}
}

namespace AdServer
{
namespace Commons
{
  inline
  const char*
  find_end_of_number(const char* str)
    noexcept
  {
    if(*str == '-' || *str == '+')
    {
      ++str;
    }

    while (static_cast<unsigned>(*str - '0') < 10)
    {
      ++str;
    }

    if (*str == '.')
    {
      ++str;
    }

    while (static_cast<unsigned>(*str - '0') < 10)
    {
      ++str;
    }

    if (*str == 'e' || *str == 'E')
    {
      ++str;

      if(*str == '-' || *str == '+')
      {
        ++str;
      }

      while (static_cast<unsigned>(*str - '0') < 10)
      {
        ++str;
      }
    }

    return str;
  }

  namespace
  {
    inline
    bool
    check_negative(
      const String::SubString& str,
      String::SubString::SizeType& cur_pos)
    {
      if(cur_pos < str.length())
      {
        const bool res = (str[cur_pos] == '-');
        if(str[cur_pos] == '-' || str[cur_pos] == '+')
        {
          ++cur_pos;
        }
        return res;
      }
      return false;
    }
  }

  template<typename DecimalType>
  DecimalType
  extract_decimal(
    const String::SubString& str,
    Generics::DecimalMulRemainder round_type,
    bool check_format) /*throw(typename DecimalType::Overflow, typename DecimalType::NotNumber)*/
  {
    if (str.empty())
    {
      if (check_format)
      {
        Stream::Error ostr;
        ostr << "extract_decimal() empty string passed";
        throw typename DecimalType::NotNumber(ostr);
      }
      else
      {
        return DecimalType(false, 0, 0);
      }
    }

    typedef typename DecimalType::Base Base;
    String::SubString::SizeType cur_pos = 0;

    const bool negative = check_negative(str, cur_pos);

    Base integer = 0;
    Base fraction = 0;

    while (cur_pos < str.length() && isdigit(str[cur_pos]))
    {
      integer = (integer * 10) + (str[cur_pos++] - '0');
    }

    DecimalType rounding = DecimalType::ZERO;

    if ((cur_pos < str.length()) && (str[cur_pos] == '.'))
    {
      ++cur_pos;
      int digits_left = DecimalType::FRACTION_RANK;

      while (cur_pos < str.length() && isdigit(str[cur_pos]) && digits_left > 0)
      {
        --digits_left;
        fraction = (fraction * 10) + (str[cur_pos++] - '0');
      }

      if ((cur_pos < str.length()) &&
          (digits_left == 0) &&
          (static_cast<unsigned>(str[cur_pos] - '0') < 10))
      {
        // fraction part is longer than FRACTION_RANK
        if (round_type == Generics::DMR_ROUND)
        {
          if ((str[cur_pos] - '0') > 4)
          {
            rounding = DecimalType(negative, 0, 1);
          }
        }
        else if (round_type == Generics::DMR_CEIL)
        {
          if (!negative)
          {
            rounding = DecimalType(false, 0, 1);
          }
        }
        else if (round_type == Generics::DMR_FLOOR)
        {
          if (negative)
          {
            rounding = DecimalType(true, 0, 1);
          }
        }

        // skip extra digits
        do
        {
          ++cur_pos;
        }
        while (cur_pos < str.length() && isdigit(str[cur_pos]));
      }

      if (DecimalType::FRACTION_RANK > 1 && digits_left > 0)
      {
        fraction *= Generics::DecimalHelper::pow10<Base>(digits_left);
      }
    }

    DecimalType result(negative, integer, fraction);
    result += rounding;

    if ((cur_pos < str.length()) &&
        (str[cur_pos] == 'e' || str[cur_pos] == 'E'))
    {
      ++cur_pos;
      const bool negative_order = check_negative(str, cur_pos);
     
      Base exponent = 0;

      while (cur_pos < str.length() && isdigit(str[cur_pos]))
      {
        exponent = (exponent * 10) + (str[cur_pos++] - '0');
      }

      const unsigned pow_rank =
        DecimalType::FRACTION_RANK > 1 ?
        DecimalType::FRACTION_RANK - 1 : 1;
      for(unsigned i = 0; i < exponent / pow_rank; ++i)
      {
        const DecimalType power(1, pow_rank);

        if (negative_order)
        {
          result = DecimalType::mul(result, power, Generics::DMR_ROUND);
        }
        else
        {
          result = DecimalType::div(result, power);
        }
      }

      if(exponent % pow_rank > 0)
      {
        const DecimalType power(1, exponent % pow_rank);

        if (negative_order)
        {
          result = DecimalType::mul(result, power, Generics::DMR_ROUND);
        }
        else
        {
          result = DecimalType::div(result, power);
        }
      }
    }

    if (check_format && cur_pos < str.length())
    {
      Stream::Error ostr;
      ostr << "extract_decimal() string '" << str << "' contains non-digit character";
      throw typename DecimalType::NotNumber(ostr);
    }

    return result;
  }
}
}

#endif /*DECIMALUTILS_HPP*/
