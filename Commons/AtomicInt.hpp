#ifndef ADSERVER_COMMONS_ATOMIC_HPP
#define ADSERVER_COMMONS_ATOMIC_HPP

#if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 2
#include <ext/atomicity.h>
#else
#include <bits/atomicity.h>
#endif

#include <stdint.h>
#include <limits>

namespace Algs
{
  class AtomicInt
  {
  public:
    AtomicInt(_Atomic_word val);

    int exchange_and_add(int val);

    AtomicInt& operator+=(int val);

    AtomicInt& operator-=(int val);

    operator int() const;

  private:
    volatile _Atomic_word value_;
  };

  class AtomicUInt
  {
  public:
    AtomicUInt(unsigned int val);

    int exchange_and_add(int val);

    AtomicUInt& operator+=(int val);

    AtomicUInt& operator-=(int val);

    operator unsigned int() const;

  private:
    volatile _Atomic_word value_;
  };
}

namespace Algs
{
  // AtomicInt
  inline
  AtomicInt::AtomicInt(int val)
    : value_(val)
  {}

  inline
  int
  AtomicInt::exchange_and_add(int val)
  {
    return __gnu_cxx::__exchange_and_add(&value_, val);
  }

  inline
  AtomicInt&
  AtomicInt::operator+=(int val)
  {
    __gnu_cxx::__atomic_add(&value_, val);
    return *this;
  }

  inline
  AtomicInt&
  AtomicInt::operator-=(int val)
  {
    *this += -val;
    return *this;
  }

  inline
  AtomicInt::operator int() const
  {
    return value_;
  }

  // AtomicUInt
  inline
  AtomicUInt::AtomicUInt(unsigned int val)
    : value_(static_cast<int64_t>(val) + std::numeric_limits<_Atomic_word>::min())
  {}

  inline
  int
  AtomicUInt::exchange_and_add(int val)
  {
    return __gnu_cxx::__exchange_and_add(&value_, val);
  }

  inline
  AtomicUInt&
  AtomicUInt::operator+=(int val)
  {
    __gnu_cxx::__atomic_add(&value_, val);
    return *this;
  }

  inline
  AtomicUInt&
  AtomicUInt::operator-=(int val)
  {
    *this += -val;
    return *this;
  }

  inline
  AtomicUInt::operator unsigned int() const
  {
    return static_cast<int64_t>(value_) - std::numeric_limits<_Atomic_word>::min();
  }
}

#endif /*ADSERVER_COMMONS_ATOMIC_HPP*/
