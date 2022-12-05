#ifndef __AUTOTESTS_COMMONS_UTILS__HPP
#define __AUTOTESTS_COMMONS_UTILS__HPP

#include <sstream>
#include <math.h>
#include <iomanip>
#include <iostream>
#include <list>
#include <vector>

#include <eh/Exception.hpp>
#include <Stream/MemoryStream.hpp>
#include <Generics/Time.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

#include <tests/AutoTests/Commons/Traits.hpp>
#include <tests/AutoTests/Commons/Sequence.hpp>

/**
 * @brief Converts characters in string to lower case.
 * @param size number of characters in string
 * @param str source string for lower case operation
 * @return STL lower case string
 */
std::string tolower(size_t size, const char* str);

/**
 * @brief Converts all characters in C-string to lower case.
 * @param str source C-string
 * @return STL lower case string
 */
inline std::string tolower(const char* str)
{
  return tolower(strlen(str), str);
}

/**
 * @brief Converts all characters in STL-string to lower case.
 * @param str source STL-string
 * @return STL lower case string
 */
inline std::string tolower(const std::string& str)
{
  return tolower(str.size(), str.c_str());
}

/**
 * @brief Converts characters in string to upper case.
 * @param size number of characters in string
 * @param str source string for upper case operation
 * @return STL upper case string
 */
std::string toupper(size_t size, const char* str);

/**
 * @brief Converts all characters in C-string to upper case.
 * @param str source C-string
 * @return STL upper case string
 */
inline std::string toupper(const char* str)
{
  return toupper(strlen(str), str);
}

/**
 * @brief Converts all characters in STL-string to upper case.
 * @param str source STL-string
 * @return STL upper case string
 */
inline std::string toupper(const std::string& str)
{
  return toupper(str.size(), str.c_str());
}

namespace AutoTest
{

  inline time_t days(const Generics::Time& time)
  {
    return (time / Generics::Time::ONE_DAY.tv_sec).tv_sec;
  }

  /**
   * @brief Converts time to GMT and returns its string representation.
   * @param time source local time.
   * @return string representation of converted to GMT time.
   */
  std::string time_to_gmt_str(time_t time);

  /**
   * @brief Converts string to time_t format.
   * @param gmt_str time well formatted string
   * @return converted from string time in time_t format.
   */
  time_t gmt_str_to_time(const char* gmt_str) /*throw(eh::Exception)*/;

  /**
   * @class slice_iterator
   * @brief Iterator for SliceSeq class.
   *
   * slice_terator points to some element of SliceSeq
   * with accent on any member (slice member) of this element.
   * For SliceSeq important only one field of element of sequence
   * (any field of struct for example).
   */
  template<typename iterator, typename slice_type>
  class slice_iterator: public std::iterator<
    typename iterator::iterator_category,
    typename iterator::value_type,
    typename iterator::difference_type>
  {
  public:
    typedef std::iterator<
    typename iterator::iterator_category,
    typename iterator::value_type,
    typename iterator::difference_type>
    BaseIteratorType;

    typedef slice_type value_type;
    typedef typename iterator::value_type iterator_value_type;
    typedef slice_type iterator_value_type::* slice;
  protected:
    iterator iter_;   //!< Base iterator
    slice    member_; //!< Slice member
  public:

    /**
     * @brief Constructor.
     *
     * Default Constructor. Initialize values with 0.
     */
    slice_iterator ()
      :member_(0)
    {}

    /**
     * @brief Constructor.
     *
     * Construct slice_iterator object
     * and initialize slice member with some value.
     * @param member initial value for slice member.
     */
    slice_iterator (slice member)
      :member_(member)
    {}

    /**
     * @brief Constructor.
     *
     * Construct slice_iterator object
     * and initialize base iterator with some value.
     * @param iter initial value for base iterator.
     */
    slice_iterator (const iterator& iter)
      :iter_(iter)
    {}

    /**
     * @brief Constructor.
     *
     * Construct slice_iterator object
     * and initialize slice member and base iteartor with some values.
     * @param iter initial value for base iterator.
     * @param member initial value for slice member.
     */
    slice_iterator (const iterator& iter, slice member)
      :iter_(iter), member_(member)
    {}

    /**
     * @brief Constructor.
     *
     * Construct slice_iterator object
     * and initialize base iterator with iterator of another type.
     * @param iter initial value for base iterator.
     * @note Base iterator must be compatible with initial iterator.
     */
    template<typename other_iterator>
    slice_iterator (const other_iterator& iter)
      :iter_(iter)
    {}

    /**
     * @brief Constructor.
     *
     * Construct slice_iterator object
     * and initialize base iterator with iterator of another type
     * and slice member with some value.
     * @param iter initial value for base iterator.
     * @param member initial value for slice member.
     * @note Base iterator must be compatible with initial iterator.
     */
    template<typename other_iterator>
    slice_iterator (const other_iterator& iter, slice member)
      :iter_(iter), member_(member)
    {}

    /**
     * @brief Constructor.
     *
     * Copy constructor.
     * @param iter initial slice_iterator.
     */
    slice_iterator (const slice_iterator& iter)
      :iter_(iter.iter_), member_(iter.member_)
    {}

    /**
     * @brief Assignment operator.
     *
     * Allows assign base iterator with new value using '=' operator.
     * @param iter new value for base iterator.
     */
    slice_iterator& operator=(const iterator& iter)
    { iter_= iter; return *this; }

    /**
     * @brief Assignment operator.
     *
     * Allows assign slice member with new value using '=' operator.
     * @param member new value for slice member.
     */
    slice_iterator& operator=(slice member)
    { member_ = member; return *this; }

    /**
     * @brief Assignment operator.
     *
     * Allows assign slice_iterator with new value using '=' operator.
     * @param from new value for slice_iterator.
     * @note Copy only base iterator.
     */
    slice_iterator& operator=(const slice_iterator& from)
    { iter_= from.iter_; return *this; }

    /**
     * @brief Comparison on equality.
     *
     * Check equal of iterators.
     * @param iter iterator to compare with slice iterator.
     * @return true if iterators occupy the same position, false otherwise.
     */
    bool operator== (const iterator& iter) const { return iter_ == iter; }

    /**
     * @brief Comparison on equality.
     *
     * Check equal of iteartors.
     * @param iter slice_iterator to compare with another slice_iterator.
     * @return true if iterators occupy the same position, false otherwise.
     */
    bool operator== (const slice_iterator& iter) const { return iter_ == iter.iter_; }

    /**
     * @brief Comparison on inequality.
     *
     * Checking inequality of slice_iterator and other iterator.
     * @param iter iterator to compare with slice iterator.
     * @return true if iterators occupy different positions, false otherwise.
     */
    bool operator!= (const iterator& iter) const { return iter_ != iter; }

    /**
     * @brief Comparison on inequality.
     *
     * Checking inequality of two slice_iterators.
     * @param iter slice_iterator to compare with another slice_iterator.
     * @return true if iterators occupy different positions, false otherwise.
     */
    bool operator!= (const slice_iterator& iter) const { return iter_ != iter.iter_; }

    /**
     * @brief Comparison on less.
     *
     * Compare positions of slice_iterator and other iterator.
     * @param iter iterator to compare with slice iterator.
     * @return whether slice_iterator is before iter.
     */
    bool operator< (const iterator& iter) const { return iter_ < iter; }

    /**
     * @brief Comparison on less.
     *
     * Compare positions of two slice_iterators.
     * @param iter slice_iterator to compare with another slice_iterator.
     * @return whether slice_iterator is before iter.
     */
    bool operator< (const slice_iterator& iter) const { return iter_ < iter.iter_; }

    /**
     * @brief Comparison on greater.
     *
     * Compare positions of slice_iterator and other iterator.
     * @param iter iterator to compare with slice iterator.
     * @return whether slice_iterator is after iter.
     */
    bool operator> (const iterator& iter) const { return iter_ > iter; }

    /**
     * @brief Comparison on greater.
     *
     * Compare positions of two slice_iterators.
     * @param iter slice_iterator to compare with another slice_iterator.
     * @return whether slice_iterator is after iter.
     */
    bool operator> (const slice_iterator& iter) const { return iter_ > iter.iter_; }

    /**
     * @brief Increment operator.
     *
     * Steps forward.
     * @return new position of iterator (increased by 1).
     */
    slice_iterator& operator++ () { ++iter_; return *this; }
    slice_iterator operator++ (int) { slice_iterator ret(*this); ++iter_; return ret; }

    /**
     * @brief Decrement operator.
     *
     * Steps backward.
     * @return new position of thr iterator (decreased by 1).
     */
    slice_iterator& operator-- () { --iter_; return *this; }
    slice_iterator operator-- (int) { slice_iterator ret(*this); --iter_; return ret; }

    /**
     * @brief Plus operator.
     *
     * Steps forward on several positions.
     * @param shift shift size.
     * @return new position of iterator (moved on shift positions forward).
     */
    slice_iterator operator+ (int shift) const { return slice_iterator(iter_+shift, member_); }

    /**
     * @brief Minus operator.
     *
     * Steps backward on several positions.
     * @param shift shift size.
     * @return new position of iterator (moved on shift positions backward).
     */
    slice_iterator operator- (int shift) const { return slice_iterator(iter_-shift, member_); }

    typename BaseIteratorType::difference_type
    operator- (slice_iterator it) const
    {
      return it.iter_ - this->iter_;
    }

    /**
     * @brief Dereference.
     *
     * Provides read/write access to slice member
     * of the actual element.
     * @return value of slice member.
     */
    value_type& operator*  () { return (*iter_).*member_; }

    /**
     * @brief Dereference.
     *
     * Provides read only access to slice member
     * of the actual element.
     * @return value of slice member.
     */
    const value_type& operator*  () const { return (*iter_).*member_; }

    /**
     * @brief Member by pointer.
     *
     * Provides pointer to member value
     * of "slice member" of the actual element.
     * @return pointer to value of some member of "slice member".
     */
    value_type* operator-> () { return &((*iter_).*member_); }

    /**
     * @brief Member by pointer.
     *
     * Provides pointer to read only member value
     * of "slice member" of the actual element.
     * @return pointer to const value of member of "slice member".
     */
    const value_type* operator-> () const { return &((*iter_).*member_); }
  };

  template<typename iterator, typename slice_type>
  inline 
  slice_iterator<iterator, slice_type>
  operator+ (int shift, const slice_iterator<iterator, slice_type>& iter)
  { 
    return iter + shift; 
  }

  template<typename iterator, typename slice_type>
  inline
  slice_iterator<iterator, slice_type>
  operator- (int shift, const slice_iterator<iterator, slice_type>& iter)
  { 
    return iter - shift; 
  }

  template<typename Seq, typename Value>
  class SliceSeq
  {
  public:
    typedef slice_iterator<typename Seq::const_iterator, Value> const_iterator;
    typedef typename Seq::size_type size_type;
  protected:
    const Seq&        seq_;
    const_iterator    begin_;
    const_iterator    end_;
  public:
    template<typename Member>
    SliceSeq (const Seq& seq, Member member)
      :seq_(seq),
       begin_(seq.begin(), member),
       end_  (seq.end(),   member)
    {}
    const_iterator begin () const { return begin_;}
    const_iterator end   () const { return end_;}
    size_type      size  () const { return seq_.size();}
  };

  inline double round (double value, double precisely)
  {
    return ::round(value/precisely)*precisely;
  }

  inline int digits (double value, double precisely)
  {
    return (int)(::ceil(log10(value)) - ::floor(log10(precisely))) + 1;
  }

  /**
   * @class Comparable
   * @brief Comparator for values (interface).
   */
  class Comparable
  {
  public:
    /**
     * @brief Destructor.
     */
    virtual ~Comparable() noexcept = 0;

    /**
     * @brief Compare values
     * @param other value.
     */
    virtual bool compare(const char* other) const = 0;

    /**
     * @brief Convert values to string
     * @return string representation of value
     */
    virtual std::string str() const = 0;

    /**
     * @brief Compare values (std::string alias)
     * @param other value.
     */
    bool compare(const std::string& other) const
    {
      return compare(other.c_str());
    }
    
  };

  template<>
  struct TypeTraits<Comparable>
  {  
    static bool equal( const Comparable& x, const Comparable& y )
    {
      return x.compare(y.str());
    }
  
    static bool equal( const Comparable& x, const std::string& y )
    {
      return x.compare(y.c_str());
    }

    static bool equal( const std::string& y, const Comparable& x )
    {
      return x.compare(y.c_str());
    }

    static const std::string to_string( const Comparable& x )
    {
      return x.str();
    }
  };

  /**
   * @class ComparableString.
   * @brief Comparator for string values.
   */
  class ComparableString : 
    public virtual Comparable
  {
  protected:

    std::string value_; 

  public:

    /**
     * @brief Default constructor.
     */
    ComparableString()
    { }

    /**
     * @brief Constructor.
     * @param value
     */
    ComparableString(
      const char* v) :
      value_(v)
    { }

    /**
     * @brief Constructor.
     * @param value
     */
    ComparableString(
      const std::string& v) :
      value_(v)
    { }

    /**
     * @brief Destructor.
     */    
    virtual ~ComparableString() noexcept
    { }

    /**
     * @brief Compare values
     * @param other value.
     */
    virtual
    bool
    compare(
      const char* other) const
    { 
      return String::SubString(value_) == String::SubString(other);
    }

    /**
     * @brief Convert values to string
     * @return string representation of value
     */
    virtual
    std::string str() const
    {
      return value_;
    }
  };

  
  template<>
  struct TypeTraits<ComparableString>: public TypeTraits<Comparable>
  {  
  };

  class ComparableRegExp : 
    public virtual Comparable
  {
  protected:
    std::string value_;
  public:
    ComparableRegExp() {}
    ComparableRegExp(const char* v):value_(v) {}
    ComparableRegExp(const std::string& v):value_(v) {}
    virtual ~ComparableRegExp() noexcept {}

    virtual bool compare(const char* other) const;
    virtual std::string str() const { return value_;}
  };
  template<>
  struct TypeTraits<ComparableRegExp>: public TypeTraits<Comparable>
  {  
  };

  typedef std::list<std::string> StringList;

  StringList parse_list (const char* str);

  inline
  StringList parse_list (const std::string& str)
  {
    return parse_list(str.c_str());
  }

  /**
   * @class ComparableSeq.
   * @brief Comparator for sequences.
   */
  template<class Item>
  class ComparableSeq :
    public virtual Comparable
  {
  public:

    typedef std::list<Item> List;
    typedef typename List::iterator iterator;

    /**
     * @brief Default constructor.
     */
    ComparableSeq()
    { }

    /**
     * @brief Constructor (from list).
     * @param sequence
     * @param entry or full compare, flag
     */
    ComparableSeq(
      const List& v,
      bool check_is_entry = false) :
      value_(v),
      check_is_entry_(check_is_entry)
    { }

    /**
     * @brief Constructor (from initializer list).
     * @param sequence
     * @param equal or not, flag
     */
    template <typename T>
    ComparableSeq(
      const std::initializer_list<T>& v,
      bool check_is_entry = false) :
      value_(v.begin(), v.end()),
      check_is_entry_(check_is_entry)
    { }

    /**
     * @brief Constructor (from array or other container).
     * @param sequence
     * @param equal or not, flag
     */
    template<typename Sequence>
    ComparableSeq(Sequence v,
      bool check_is_entry = false) :
      value_(beginof(v), endof(v)),
      check_is_entry_(check_is_entry)
    { }

    /**
     * @brief Destructor.
     */
    virtual ~ComparableSeq() noexcept
    { }

    /**
     * @brief Compare values
     * @param other value (string).
     */
    virtual
    bool
    compare(
      const char* other) const
    { 
      if (check_is_entry_)
      {
        return equal_seq(value_, parse_list(other));
      }
      return entry_in_seq(value_, parse_list(other));
    }

    /**
     * @brief Convert sequence to string
     * @return string representation of sequence
     */
    virtual
    std::string
    str() const
    {
      std::ostringstream ost;
      if (!value_.empty())
      {
        typename List::const_iterator i = value_.begin();
        ost << (i++)->str();
        while (i != value_.end())
        {
          ost << ',' << (i++)->str();
        }
      }
      return ost.str();
    }

  protected:

    List value_;
    bool check_is_entry_;
    
  };

  typedef ComparableSeq<ComparableString> ComparableStringList;

  /**
   * @class precisely_number
   * @brief Base class for number with given precision.
   *
   * Using for comparing floating-point numbers.
   */
  class precisely_number : 
    public virtual Comparable
  {
  protected:

    double value_;
    double precisely_;

    /**
     * @brief 'Put to stream' operator.
     * Puts precisely_number value to ostream.
     * @param out output number.
     * @param svalue SimpleValue value.
     * @return out stream;
     */
    friend std::ostream& operator<< (std::ostream&, const precisely_number&);

  public:

    /**
     * @brief Setup number.
     * @param value.
     * @param precision.
     */
    void set_up (double v, double precisely = 0.0)
    {
      value_ = v;
      precisely_ = precisely;
    }

    /**
     * @brief Get value.
     * @return raw value.
     */
    double value() const
    {
      return value_;
    }    

    /**
     * @brief Defaul constructor
     */
    precisely_number ()
      :Comparable()
    {}

    /**
     * @brief Constructor
     * @param double value
     * @param precision
     */
    precisely_number (double v, double precisely = 0.0)
      :value_(v),
       precisely_(precisely)
    {}

    /**
     * @brief Constructor
     * @param string value
     * @param precision
     */
    precisely_number (const char* v, double precisely = 0.0)
      :precisely_(precisely)
    { value_ = ::atof(v);}

    /**
     * @brief Constructor
     * @param string value
     * @param precision
     */
    precisely_number (const std::string& v, double precisely = 0.0)
      :precisely_(precisely)
    { value_ = ::atof(v.c_str());}

    /**
     * @brief Copy constructor
     */
    precisely_number (const precisely_number& v)
      :Comparable(),
       value_(v.value_),
       precisely_(v.precisely_)
    {}

    virtual ~precisely_number() noexcept {}

    /**
     * @brief Get digits
     * @return count of number digits
     */
    int digits () const
    {
      return (int)(ceil(log10(value_)) - floor(log10(precisely_))) + 1;
    }

    /**
     * @brief Assignment operator.
     * @param double value.
     */
    precisely_number& operator= (double v)
    {
      value_ = v;
      return *this;
    }

    /**
     * @brief Assignment operator.
     * @param other precisely_number.
     */
    precisely_number& operator= (const precisely_number& v)
    {
      value_ = v.value_;
      return *this;
    }

    /**
     * @brief Assignment operator.
     * @param string value.
     */
    precisely_number& operator= (const char* v)
    {
      value_ = ::atof(v);
      return *this;
    }

    /**
     * @brief Assignment operator.
     * @param string value.
     */
    precisely_number& operator= (const std::string& v)
    {
      value_ = ::atof(v.c_str());
      return *this;
    }

    /**
     * @brief Comparison on greater.
     *
     * Compare precisely_numbers.
     * @param other precisely_number.
     * @return true when greater than other.
     */
    bool operator> (const precisely_number& v) const
    {
      return (value_ - v.value_) > precisely_;
    }

    /**
     * @brief Comparison less.
     *
     * Compare precisely_numbers.
     * @param other precisely_number.
     * @return true when less than other.
     */
    bool operator< (const precisely_number& v) const
    {
      return (v.value_ - value_) > precisely_;
    }
  
    /**
     * @brief Increment.
     *
     * Increment precisely_numbers.
     * @param other precisely_number.
     * @return incremented precisely_number.
     */
    precisely_number& operator+= (const precisely_number& v)
    {
      value_ += v.value_;
      return *this;
    }

    /**
     * @brief Division.
     *
     * Divide precisely_numbers.
     * @param other precisely_number.
     * @return divided precisely_number.
     */
    precisely_number& operator/= (const precisely_number& v)
    {
      value_ /= v.value_;
      return *this;
    }

    /**
     * @brief Multiplication         .
     *
     * Multiply precisely_numbers.
     * @param other precisely_number.
     * @return  multiplied precisely_number.
     */
    precisely_number& operator*= (const precisely_number& v)
    {
      value_ *= v.value_;
      return *this;
    }

    /**
     * @brief Subtraction         .
     *
     * Substract precisely_numbers.
     * @param other precisely_number.
     * @return  substracted precisely_number.
     */
    precisely_number& operator-= (const precisely_number& v)
    {
      value_ -= v.value_;
      return *this;
    }

    /**
     * @brief Increment.
     *
     * Increment precisely_numbers.
     * @param other precisely_number.
     * @return sum.
     */
    precisely_number operator+(const precisely_number& v) const
    {
      precisely_number val(*this);
      val+=v;
      return val;
    }
  
    /**
     * @brief Division.
     *
     * Divide precisely_numbers.
     * @param other precisely_number.
     * @return quotient.
     */  
    precisely_number operator/(const precisely_number& v) const
    {
      precisely_number val(*this);
      val/=v;
      return val;
    }

    /**
     * @brief Multiplication         .
     *
     * Multiply precisely_numbers.
     * @param other precisely_number.
     * @return product precisely_number.
     */
    precisely_number operator*(const precisely_number& v) const
    {
      precisely_number val(*this);
      val*=v;
      return val;
    }

    /**
     * @brief Subtraction         .
     *
     * Substract precisely_numbers.
     * @param other precisely_number.
     * @return residual precisely_number.
     */
    precisely_number operator-(const precisely_number& v) const
    {
      precisely_number val(*this);
      val-=v;
      return val;
    }

    /**
     * @brief Comparison on equality.
     *
     * Check equal of precisely_numbers.
     * @param other precisely_number.
     * @return true if numbers is equal, false otherwise.
     */
    bool operator== (const precisely_number& v) const
    {
      return fabs(value_ - v.value_) <= precisely_;
    }

    /**
     * @brief Comparison on inequality.
     *
     * Checking inequality of two precisely_numbers.
     * @param other precisely_number.
     * @return false if numbers is equal, true otherwise.
     */
    bool operator!= (const precisely_number& v) const
    {
      return !(*this == v);
    }

    virtual bool compare(const char* other) const;
    virtual std::string str() const;

  };

  inline
  std::ostream&
  operator<< (std::ostream& out, const precisely_number& number)
  {
    out << std::setprecision(number.digits()) << number.value_;// << '%' << number.precisely_;
    return out;
  }

  static const double money_delta = 0.01;


  /**
   * @class Money.
   * @brief Class for comparing moneys (such as revenue, eCPM, actualCPC etc) .
   */
  class Money :
    public precisely_number
  {
  public:

    /**
     * @brief Default constructor.
     */
    Money ()
      :precisely_number(0.0, money_delta)
    {}
  
    /**
     * @brief Constructor.
     * @param double value.
     */
    Money (double v)
      :precisely_number(v, money_delta)
    {}

    /**
     * @brief Constructor.
     * @param string value.
     */
    Money (const char* v)
      :precisely_number(v, money_delta)
    {}

    /**
     * @brief Constructor
     * @param string value.
     */
    Money (const std::string& v)
      :precisely_number(v, money_delta)
    {}

    /**
     * @brief Constructor
     * @param precisely_number.
     */
    Money (const precisely_number& v)
      :precisely_number(v)
    {}

  };

  template<>
  struct TypeTraits<Money>
  {  
    static bool equal( const Money& x, const Money& y )
    {
      return x == y;
    }
  
    static bool equal( const double& x, const Money& y )
    {
      return y == x;
    }
  
    static bool equal( const Money& y, const double& x )
    {
      return y == x;
    }
  
    static const std::string to_string( const Money& x )
    {
      std::ostringstream ost;
      ost << x.value();
      return ost.str();
    }
  };

  template <class ValueInterface>
  class ValueWrapperVar
  {
  protected:

    class VarValue :
      public virtual ValueInterface,
      public virtual ReferenceCounting::AtomicImpl
    {
    protected:
      virtual ~VarValue() noexcept {}
    };
    typedef ReferenceCounting::SmartPtr<VarValue> Var;

    template<class Value>
    class ValueVar :
      public virtual VarValue,
      public virtual Value
    {
    public:
      ValueVar(const ValueVar& val)
        : Value(val) {}
      ValueVar(const Value& val)
        : Value(val) {}
    protected:
      virtual ~ValueVar() noexcept {}
    };
  
    Var var_;

  public:

    ValueWrapperVar() {}
    template<class Value>
    ValueWrapperVar(const Value& value):var_(new ValueVar<Value>(value)) {}

    template<class Value>
    ValueWrapperVar& set(const Value& value)
    {
      var_ = new ValueVar<Value>(value);
      return *this;
    }

    // conversions
    operator ValueInterface*() const noexcept
    { return var_.in();}
    // accessors
    ValueInterface* operator ->() const
    { return var_.in();}
    ValueInterface& operator *() const
    { return *var_.in();}
    ValueInterface* in() const noexcept
    { return var_.in();}
    ValueInterface* retn() noexcept
    { return var_.retn();}
  };

  typedef ValueWrapperVar<Comparable> ComparableVar;
  typedef std::vector<ComparableVar> ComparableList;
}

#endif//__AUTOTESTS_COMMONS_UTILS__HPP
