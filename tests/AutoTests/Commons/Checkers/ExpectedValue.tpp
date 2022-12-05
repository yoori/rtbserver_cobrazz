#ifndef __AUTOTESTS_COMMONS_CHECKER_EXPECTEDVALUE_IPP
#define __AUTOTESTS_COMMONS_CHECKER_EXPECTEDVALUE_IPP

namespace AutoTest
{
  // ExpValue
  template <typename T>
  ExpValue<T>::ExpValue() :
    is_set_(false)
  { }

  template <typename T>
  ExpValue<T>::ExpValue(
    const T& value) :
    value_(value),
    is_set_(true)
  { }

  template <typename T>
  const T&
  ExpValue<T>::operator*() const
  {
    return value_;
  }

  template <typename T>
  const T*
  ExpValue<T>::operator->() const
  {
    return &value_;
  }

  template <typename T>
  T*
  ExpValue<T>::operator->()
  {
    return &value_;
  }

  template <typename T>
  ExpValue<T>&
  ExpValue<T>::operator=(const T& value)
  {
    value_ = value;
    is_set_ = true;
    
    return *this;
  }

  template <typename T>
  bool
  ExpValue<T>::is_set() const
  {
    return is_set_;
  }

  template <typename T>
  void
  ExpValue<T>::is_set(
    bool is_set)
  {
    is_set_ = is_set;
  }

  template <typename T>
  std::istream&
  operator>> (
    std::istream &in,
    ExpValue<T>& value)
  {
    T v;
    in >> v;
    value = v;
    return in;
  }
}
#endif  // __AUTOTESTS_COMMONS_CHECKER_EXPECTEDVALUE_IPP
