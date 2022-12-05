#ifndef PLAIN_CONSTVECTOR_HPP
#define PLAIN_CONSTVECTOR_HPP

namespace PlainTypes
{
  template<typename ObjectType>
  ObjectType ctor_read_cast(const void* buf)
  {
    return ObjectType(buf, 0);
  }

  template<typename ObjectType>
  ObjectType ctor_with_bounds_read_cast(
    const void* buf, const void* end_buf)
  {
    return ObjectType(
      buf,
      static_cast<const char*>(end_buf) - static_cast<const char*>(buf));
  }

  namespace ReadPolicy
  {
    template<
      typename ReturnType,
      ReturnType ReadCastFun(const void*)>
    struct NoCheck: public std::unary_function<const void*, ReturnType>
    {
      NoCheck() {}

      NoCheck(const void* /*buf_end*/) {}

      ReturnType operator()(const void* ptr) const
      {
        return ReadCastFun(ptr);
      }
    };

    template<
      typename ReturnType,
      ReturnType ReadCastFun(const void*, const void*)>
    struct CheckBounds: public std::unary_function<const void*, ReturnType>
    {
      CheckBounds(): buf_end_(0) {}

      CheckBounds(const void* buf_end)
        : buf_end_(buf_end)
      {}

      ReturnType operator()(const void* ptr) const
      {
        return ReadCastFun(ptr, buf_end_);
      }

    private:
      const void* buf_end_;
    };
  }

  template<
    typename ReadPolicyType,
    const unsigned long STEP>
  class ConstVector
  {
  public:
    typedef ReadPolicyType ReadPolicy;
    typedef typename ReadPolicyType::result_type reference;

    class const_iterator
    {
      friend class ConstVector<ReadPolicyType, STEP>;

    private:
      const_iterator(const void* ptr, const ReadPolicy& read_policy);

    public:
      typedef typename ReadPolicyType::result_type value_type;
      typedef value_type reference;
      typedef std::random_access_iterator_tag iterator_category;
      typedef int difference_type;
      struct pointer {};

    public:
      const_iterator();

      const_iterator& operator++();
      const_iterator operator++(int);
      const_iterator& operator--();
      const_iterator operator--(int);

      bool operator==(const const_iterator& right) const;
      bool operator!=(const const_iterator& right) const;
      bool operator<(const const_iterator& right) const;
      bool operator<=(const const_iterator& right) const;
      bool operator>(const const_iterator& right) const;
      bool operator>=(const const_iterator& right) const;
      int operator-(const const_iterator& right) const;

      const_iterator& operator+=(int n);
      const_iterator& operator-=(int n);

      const_iterator operator+(int n) const;
      const_iterator operator-(int n) const;

      reference operator*() const;

      const_iterator advance(int n) const;

    private:
      const char* ptr_;
      ReadPolicy read_policy_;
    };

    class const_reverse_iterator
    {
      friend class ConstVector<ReadPolicyType, STEP>;

    private:
      const_reverse_iterator(const void* ptr, const ReadPolicy& read_policy);

    public:
      typedef typename ReadPolicyType::result_type value_type;
      typedef value_type reference;
      typedef std::random_access_iterator_tag iterator_category;
      typedef int difference_type;
      struct pointer {};

    public:
      const_reverse_iterator();
      const_reverse_iterator(const const_iterator& init);

      const_reverse_iterator& operator++();
      const_reverse_iterator operator++(int);
      const_reverse_iterator& operator--();
      const_reverse_iterator operator--(int);

      bool operator==(const const_reverse_iterator& right) const;
      bool operator!=(const const_reverse_iterator& right) const;
      bool operator<(const const_reverse_iterator& right) const;
      bool operator<=(const const_reverse_iterator& right) const;
      bool operator>(const const_reverse_iterator& right) const;
      bool operator>=(const const_reverse_iterator& right) const;
      int operator-(const const_reverse_iterator& right) const;

      const_reverse_iterator& operator+=(int n);
      const_reverse_iterator& operator-=(int n);

      const_reverse_iterator operator+(int n) const;
      const_reverse_iterator operator-(int n) const;

      reference operator*() const;

      const_reverse_iterator advance(int n) const;

    private:
      const char* ptr_;
      ReadPolicy read_policy_;
    };

    ConstVector();

    ConstVector(
      const void* first,
      const void* last,
      const ReadPolicyType& read_policy);

    const_iterator begin() const;
    const_iterator end() const;

    const_reverse_iterator rbegin() const;
    const_reverse_iterator rend() const;

    reference back() const;

    unsigned long size() const;
    bool empty() const;

  private:
    const void* first_;
    const void* last_;
    ReadPolicyType read_policy_;
  };

  template<typename ConstVectorType>
  ConstVectorType init_const_vector(
    const void* buf,
    unsigned long size)
  {
    static const char* FUN = "PlainTypes::init_const_vector()";

    const unsigned char* buf_ptr = static_cast<const unsigned char*>(buf);
    uint32_t begin_offset = *static_cast<const uint32_t*>(buf);
    uint32_t end_offset = static_cast<const uint32_t*>(buf)[1];

    if(begin_offset > end_offset || end_offset > size)
    {
      Stream::Error ostr;
      ostr << FUN << ": start or end position great than size: "
        "start-offset = " << begin_offset <<
        ", end-offset = " << end_offset <<
        ", size = " << size;
      throw CorruptedStruct(ostr);
    }

    return ConstVectorType(
      buf_ptr + begin_offset,
      buf_ptr + end_offset,
      typename ConstVectorType::ReadPolicy(buf_ptr + size));
  }
}

template<typename ReadPolicyType, const unsigned long STEP>
typename PlainTypes::ConstVector<ReadPolicyType, STEP>::const_iterator
operator+(
  int n,
  typename PlainTypes::ConstVector<ReadPolicyType, STEP>::const_iterator it);

template<typename ReadPolicyType, const unsigned long STEP>
typename PlainTypes::ConstVector<ReadPolicyType, STEP>::const_iterator
operator-(
  typename PlainTypes::ConstVector<ReadPolicyType, STEP>::const_iterator it,
  int n);

namespace PlainTypes
{
  /* ConstVector<ReadPolicyType, STEP>::const_iterator */
  template<typename ReadPolicyType, const unsigned long STEP>
  ConstVector<ReadPolicyType, STEP>::const_iterator::const_iterator()
    : ptr_(0)
  {}

  template<typename ReadPolicyType, const unsigned long STEP>
  ConstVector<ReadPolicyType, STEP>::const_iterator::const_iterator(
    const void* ptr, const ReadPolicyType& read_policy)
    : ptr_((const char*)ptr),
      read_policy_(read_policy)
  {}

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_iterator&
  ConstVector<ReadPolicyType, STEP>::const_iterator::operator++()
  {
    ptr_ += STEP;
    return *this;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_iterator
  ConstVector<ReadPolicyType, STEP>::const_iterator::operator++(int)
  {
    const_iterator ret(ptr_);
    ++*this;
    return ret;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_iterator&
  ConstVector<ReadPolicyType, STEP>::const_iterator::operator--()
  {
    ptr_ -= STEP;
    return *this;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_iterator
  ConstVector<ReadPolicyType, STEP>::const_iterator::operator--(int)
  {
    const_iterator ret(ptr_);
    --*this;
    return ret;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_iterator
  ConstVector<ReadPolicyType, STEP>::const_iterator::advance(int n) const
  {
    return const_iterator(ptr_ + STEP * n, read_policy_);
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_iterator::operator==(
    const const_iterator& right)
    const
  {
    return ptr_ == right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_iterator::operator!=(
    const const_iterator& right)
    const
  {
    return ptr_ != right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_iterator::operator<(
    const const_iterator& right)
    const
  {
    return ptr_ < right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_iterator::operator<=(
    const const_iterator& right)
    const
  {
    return ptr_ <= right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_iterator::operator>(
    const const_iterator& right)
    const
  {
    return ptr_ > right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_iterator::operator>=(
    const const_iterator& right)
    const
  {
    return ptr_ >= right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  int ConstVector<ReadPolicyType, STEP>::const_iterator::operator-(
    const const_iterator& right)
    const
  {
    return (ptr_ - right.ptr_) / STEP;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_iterator&
  ConstVector<ReadPolicyType, STEP>::const_iterator::operator+=(int n)
  {
    ptr_ = (const char*)ptr_ + n * STEP;
    return *this;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_iterator&
  ConstVector<ReadPolicyType, STEP>::const_iterator::operator-=(int n)
  {
    return *this += -n;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_iterator
  ConstVector<ReadPolicyType, STEP>::const_iterator::operator+(int n) const
  {
    return this->advance(n);
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_iterator
  ConstVector<ReadPolicyType, STEP>::const_iterator::operator-(int n) const
  {
    return this->advance(-n);
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ReadPolicyType::result_type
  ConstVector<ReadPolicyType, STEP>::const_iterator::operator*() const
  {
    return read_policy_(ptr_);
  }

  /* ConstVector<ReadPolicyType, STEP>::const_reverse_iterator */
  template<typename ReadPolicyType, const unsigned long STEP>
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::const_reverse_iterator()
    : ptr_(0)
  {}

  template<typename ReadPolicyType, const unsigned long STEP>
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::const_reverse_iterator(
    const void* ptr, const ReadPolicyType& read_policy)
    : ptr_((const char*)ptr),
      read_policy_(read_policy)
  {}

  template<typename ReadPolicyType, const unsigned long STEP>
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::const_reverse_iterator(
    const const_iterator& init)
    : ptr_(init.ptr_),
      read_policy_(init.read_policy_)
  {}

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_reverse_iterator&
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator++()
  {
    ptr_ -= STEP;
    return *this;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_reverse_iterator
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator++(int)
  {
    const_reverse_iterator ret(ptr_);
    ++*this;
    return ret;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_reverse_iterator&
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator--()
  {
    ptr_ += STEP;
    return *this;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_reverse_iterator
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator--(int)
  {
    const_reverse_iterator ret(ptr_);
    --*this;
    return ret;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_reverse_iterator
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::advance(int n) const
  {
    return const_reverse_iterator(ptr_ - STEP * n, read_policy_);
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator==(
    const const_reverse_iterator& right)
    const
  {
    return ptr_ == right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator!=(
    const const_reverse_iterator& right)
    const
  {
    return ptr_ != right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator<(
    const const_reverse_iterator& right)
    const
  {
    return ptr_ > right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator<=(
    const const_reverse_iterator& right)
    const
  {
    return ptr_ >= right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator>(
    const const_reverse_iterator& right)
    const
  {
    return ptr_ < right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator>=(
    const const_reverse_iterator& right)
    const
  {
    return ptr_ <= right.ptr_;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  int ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator-(
    const const_reverse_iterator& right)
    const
  {
    return (right.ptr_ - ptr_) / STEP;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_reverse_iterator&
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator+=(int n)
  {
    ptr_ = (const char*)ptr_ - n * STEP;
    return *this;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_reverse_iterator&
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator-=(int n)
  {
    return *this += n;
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_reverse_iterator
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator+(int n) const
  {
    return this->advance(n);
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_reverse_iterator
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator-(int n) const
  {
    return this->advance(-n);
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ReadPolicyType::result_type
  ConstVector<ReadPolicyType, STEP>::const_reverse_iterator::operator*() const
  {
    return read_policy_(ptr_ - STEP);
  }

  /* ConstVector<ReadPolicyType, STEP> */
  template<typename ReadPolicyType, const unsigned long STEP>
  ConstVector<ReadPolicyType, STEP>::ConstVector()
    : first_(0), last_(0)
  {}

  template<typename ReadPolicyType, const unsigned long STEP>
  ConstVector<ReadPolicyType, STEP>::ConstVector(
    const void* first,
    const void* last,
    const ReadPolicyType& read_policy)
    : first_(first),
      last_(last),
      read_policy_(read_policy)
  {}

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_iterator
  ConstVector<ReadPolicyType, STEP>::begin() const
  {
    return const_iterator(first_, read_policy_);
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_iterator
  ConstVector<ReadPolicyType, STEP>::end() const
  {
    return const_iterator(last_, read_policy_);
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_reverse_iterator
  ConstVector<ReadPolicyType, STEP>::rbegin() const
  {
    return const_reverse_iterator(last_, read_policy_);
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ConstVector<ReadPolicyType, STEP>::const_reverse_iterator
  ConstVector<ReadPolicyType, STEP>::rend() const
  {
    return const_reverse_iterator(first_, read_policy_);
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  typename ReadPolicyType::result_type
  ConstVector<ReadPolicyType, STEP>::back() const
  {
    return read_policy_(static_cast<const char*>(last_) - STEP);
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  unsigned long ConstVector<ReadPolicyType, STEP>::size() const
  {
    return end() - begin();
  }

  template<typename ReadPolicyType, const unsigned long STEP>
  bool ConstVector<ReadPolicyType, STEP>::empty() const
  {
    return size() == 0;
  }
}

template<typename ReadPolicyType, const unsigned long STEP>
inline
typename PlainTypes::ConstVector<ReadPolicyType, STEP>::const_iterator
operator+(
  int n,
  typename PlainTypes::ConstVector<ReadPolicyType, STEP>::const_iterator it)
{
  return it.advance(n);
}

#endif /* PLAIN_CONSTVECTOR_HPP */
