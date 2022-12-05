
#ifndef __AUTOTESTS_COMMONS_TRAITS__HPP
#define __AUTOTESTS_COMMONS_TRAITS__HPP

#include <sstream>
#include <string>
#include <eh/Exception.hpp>
#include <Stream/MemoryStream.hpp>

namespace AutoTest
{

  DECLARE_EXCEPTION(ConvertException, eh::DescriptiveException);

  /**
   * @struct TypeTraits
   * @brief Defines 'equal' condition and provides to_string function.
   */
  template <class T> 
  struct TypeTraits
  {
    /**
     * @brief Checks equality of two elements.
     * @param x first element of comparison
     * @param y second element of comparison.
     * @return whether elements are equal.
     */
    static bool equal( const T& x, const T& y )
    {
      return x == y;
    }

    /**
     * @brief Converts some object to string format.
     *
     * This function converts object of some type T
     * to its string representation. Type T must provide '<<' operator.
     * @param x object to be converted to string.
     * @return string representation of object x.
     */
    static std::string to_string( const T& x )
    {
      std::ostringstream ost;
      ost << x;
      return ost.str();
    }
  };

  template<>
  struct TypeTraits<std::string>
  {  
    static bool equal( const std::string& x, const std::string& y )
    {
      return x == y;
    }
  
    static bool equal( const char* x, const std::string& y )
    {
      return y == x;
    }

    static bool equal( const std::string& y, const char* x )
    {
      return y == x;
    }

    template<typename T>
    static bool equal( const std::string& y, const T& x )
    {
      return TypeTraits<T>::equal(x, y);
    }

    template<typename T>
    static bool equal( const char* y, const T& x )
    {
      return TypeTraits<T>::equal(x, y);
    }

    static const std::string& to_string( const std::string& x )
    {
      return x;
    }
  };


  /**
   * @brief Check equality of two objects with type T.
   * @param x first object of comparison.
   * @param y second object of comparison.
   * @return whether objects are equal.
   */
  template <class T> 
  inline
  bool equal( const T& x, const T& y )
  {
    return TypeTraits<T>::equal(x, y);
  }

  /**
   * @brief Check equality of two objects with different types.
   * @note first element of comparison must provide conversion
   * to type of second element.
   * @param x first object of type L.
   * @param y second object of type R.
   * @return whether objects are equal.
   */
  template <class L, class R> 
  inline
  bool equal( const L& x, const R& y )
  {
    return TypeTraits<R>::equal(y, x);
  }

  /**
   * @brief Converts object to string.
   * @note type of object must provide '<<' operator returning string value.
   * @param x object of type T.
   * @return string representation of x object.
   */
  template <class T> 
  inline
  std::string 
  to_string( const T& x )
  {
    return TypeTraits<T>::to_string(x);
  }
}

/**
 * @brief Convert C-string into STL string.
 * @param x C-string
 * @return STL string
 */
inline
std::string 
strof ( const char* x )
{
  return std::string (x);
}
  
/**
 * @brief Convert object of type T into STL string representation.
 * @param x object of type T
 * @return STL string representation of object of type T
 */
template <class T> 
inline
std::string 
strof ( const T& x )
{
  return AutoTest::to_string(x);
}

/**
 * @brief Convert STL string into object of class T.
 *
 * @note must have '>>' stream input operator for Class T.
 * @param str STL string
 * @return object of class T
 */
template <class T> 
inline
T
valueof (const std::string& str) /*throw(eh::Exception)*/
{
  Stream::Parser istrm(str);
  T value;
  istrm >> value;
  if(istrm.fail() || !istrm.eof())
  {
    Stream::Error ostr;
    ostr << "failed conversion of '" << str << "'";
    throw AutoTest::ConvertException(ostr);
  }
  return value;
}

#endif //__AUTOTESTS_COMMONS_TRAITS__HPP

