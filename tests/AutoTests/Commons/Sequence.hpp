#ifndef __AUTOTESTS_COMMONS_SEQUENCE__HPP
#define __AUTOTESTS_COMMONS_SEQUENCE__HPP

#include <Commons/Algs.hpp>
#include <tests/AutoTests/Commons/Traits.hpp>
#include <initializer_list>

/**
 * @brief Count number of elements in array.
 * @param array reference to array
 * @return number of elements in array
 */
template <typename T, size_t Count> 
inline
size_t countof ( const T (&) [Count] )
{
  return Count;
}

/**
 * @brief Count number of elements in some container.
 * @param seq object of enumerated type with size method (STL like)
 * @return number of elements
 */
template <typename T> 
inline
size_t countof ( const T& seq )
{
  return seq.size();
}

/**
 * @brief Get start const iterator of array - begin of array.
 * 
 * Returns const pointer to the first element of array
 * so you can't modify this array using result value.
 * You can use result value only for read purposes.
 * @param array const reference to array
 * @return const pointer to the first element of array
 */
template <typename T, size_t Count> 
inline
const T* beginof ( const T (&array) [Count] )
{
  return array;
}

/**
 * @brief Get start nonconst iterator of array - begin of array.
 *
 * Returns pointer to the first element of array
 * and you can modify this array using result value,
 * i.e. you can use it for read and write purposes.
 * @param array pointer to array
 * @return pointer to begin of array
 */
template <typename T, size_t Count> 
inline
T* beginof ( T (&array) [Count] )
{
  return array;
}

/**
 * @brief Get end const iterator of array - end of array.
 *
 * Returns const pointer to the end of array
 * (const pointer for the position after the last element)
 * so you can't use this value
 * to modify array.
 * @param array const pointer to array
 * @return const pointer for the position after the last element
 */
template <typename T, size_t Count> 
inline
const T* endof ( const T (&array) [Count] )
{
  return array + Count;
}

/**
 * @brief Get end nonconst iterator of array - end of array.
 *
 * Returns pointer to the end of array (pointer for the position
 * after the last element) and you can modify array
 * using result value.
 * @param array pointer to array
 * @return pointer for the position after the last element
 */
template <typename T, size_t Count> 
inline
T* endof ( T (&array) [Count] )
{
  return array + Count;
}

/**
 * @brief Get start const iterator of STL like container - begin of container.
 *
 * Returns const_iterator for the first element of container,
 * so you can't modify it using result iterator.
 * You can use result value only for read purposes.
 * @param seq container
 * @return const_iterator for the first element of the container.
 */
template <typename T> 
inline
typename T::const_iterator beginof ( const T& seq )
{
  return seq.begin();
}

template <typename T> 
inline
const T* beginof ( const std::initializer_list<T>& seq )
{
  return seq.begin();
}

/**
 * @brief Get start nonconst iterator of STL like container - begin of container.
 *
 * Returns read/write iterator for the first element of the container.
 * @param seq container
 * @return iterator for the first element of the container
 */
template <typename T> 
inline
typename T::iterator beginof ( T& seq )
{
  return seq.begin();
}

/**
 * @brief Get end const iterator of STL like container - end of container.
 *
 * Returns const_iterator for the position after the last element
 * of the container, so you can't use it for write purposes.
 * @param seq container
 * @return const_iterator for the position after the last element
 * of the container
 */
template <typename T> 
inline
typename T::const_iterator endof ( const T& seq )
{
  return seq.end();
}

template <typename T> 
inline
const T* endof ( const std::initializer_list<T>& seq )
{
  return seq.end();
}

/**
 * @brief Get end nonconst iterator of STL like container - end of container.
 *
 * Returns read/write iterator for the position after the last element
 * of the container. You can use result iterator to modify container elements.
 * @param seq container
 * @return iterator for the position after the last element of the container
 */
template <typename T> 
inline
typename T::iterator endof ( T& seq )
{
  return seq.end();
}

namespace AutoTest
{
  template<typename It1, typename It2>
  bool
  equal_seq(It1 first1, It1 last1, It2 first2)
  {
    for (; first1 != last1; ++first1, ++first2)
      if (!equal(*first1, *first2))
        return false;
    return true;
  }

  template <typename Seq1, typename Seq2>
  bool
  equal_seq(const Seq1& expected, const Seq2& got)
  {
    return countof(expected) == countof(got) &&
      equal_seq(
        beginof(expected),
        endof(expected),
        beginof(got));
  }

  template <typename It, typename T> 
  bool 
  find_in_seq(const T& expected, It it, It end)
    noexcept
  {
    for (; it != end; it++)
    {
      if (equal(expected, *it))
      {
        return true;
      }
    }
    return false;
  }
  
  template <typename Seq, typename T> 
  bool 
  find_in_seq(const T& expected, const Seq& seq)
  {
    return find_in_seq(expected, beginof(seq), endof(seq));
  }

  // test, expected sequence entry in got sequence
  template <typename It, typename Seq> 
  bool 
  entry_in_seq (It  it, const It&  end, const Seq& got)
    noexcept
  {
    for (; it != end; it++)
    {
      if (!find_in_seq(*it, got))
      {
        return false;
      }
    }
    return true;
  }

  template <typename Seq1, typename Seq2> 
  bool 
  entry_in_seq (const Seq1& expected, const Seq2& got)
    noexcept
  {
    return entry_in_seq(beginof(expected), endof(expected), got);
  }

  // test, expected sequence entry in got sequence
  template <typename It, typename Seq>
  bool 
  not_entry_in_seq(
                   It it,
                   const It& end,
                   const Seq& got)
    noexcept
  {
    for (; it != end; it++)
    {
      if (std::find(got.begin(), got.end(), *it) != got.end())
      {
        return false;
      }
    }
    return true;
  }

  template <typename Seq1, typename Seq2>
  bool 
  not_entry_in_seq(
                   const Seq1& expected,
                   const Seq2& got)
    noexcept
  {
    return not_entry_in_seq(beginof(expected), endof(expected), got);
  }

  // test, expected sequence entry in got sequence
  template <typename It, typename Seq>
  unsigned int
  count_entry_in_seq(It it, const It& end, const Seq& got)
    noexcept
  {
    unsigned int ret = 0;
    for (; it != end; it++)
    {
      if (find_in_seq(*it, got))
      {
        ++ret;
      }
    }
    return ret;
  }

  template <typename Seq1, typename Seq2>
  unsigned int 
  count_entry_in_seq(const Seq1& expected, const Seq2& got )
    noexcept
  {
    return count_entry_in_seq(beginof(expected), endof(expected), got);
  }

  template <class Seq> 
  std::string 
  seq_to_str(const Seq& seq)
    noexcept
  {
    std::ostringstream ostr;
    ostr << "[ ";
    Algs::print(ostr, beginof(seq), endof(seq));
    ostr << " ]";
    return ostr.str();
  }
}

#endif //__AUTOTESTS_COMMONS_SEQUENCE__HPP
