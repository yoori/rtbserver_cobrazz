#ifndef __AUTOTESTS_COMMONS_CHECKER_EXPECTEDVALUE_HPP
#define __AUTOTESTS_COMMONS_CHECKER_EXPECTEDVALUE_HPP

#include <istream>

namespace AutoTest
{
  /**
   * @class ExpValue
   * @brief Expected value storage.
   *
   * Some value, that holds 'not set' state.
   * Useful in checkers. Allows it to check only set values.
   */
  template <typename T>
  class ExpValue
  {
  public:
    
    /**
     * @brief Default constructor.
     */
    ExpValue();

    /**
     * @brief Copy constructor.
     */
    explicit ExpValue(const T& value);
    
    /**
     * @brief Access to original value.
     */
    const T& operator*() const;

    /**
     * @brief Access to original value.
     */
    const T* operator->() const;
    
    /**
     * @brief Access to original value.
     */
    T* operator->();
    
    /**
     * @brief Assignment operator.
     */
    ExpValue& operator=(
      const T& value);
   
    /**
     * @brief Check value set.
     */
    bool is_set() const;
    
    /**
     * @brief Mark value set.
     */
    void is_set(
      bool is_set);
    
  private:
    T value_;
    bool is_set_;
  };
  
  /**
   * @brief Assignment from stream.
   * @param stream
   * @param value
   */
  template <typename T>
  std::istream&
  operator >>(std::istream& istr, ExpValue<T>& value);
  
} //namespace AutoTest

#include "ExpectedValue.tpp"

#endif  // __AUTOTESTS_COMMONS_CHECKER_EXPECTEDVALUE_HPP
