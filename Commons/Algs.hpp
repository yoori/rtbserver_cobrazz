#ifndef _COMMONS_ALGS_HPP_
#define _COMMONS_ALGS_HPP_

#include <iterator>
#include <ostream>
#include <set>
#include <Stream/MemoryStream.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>

namespace Algs
{
  DECLARE_EXCEPTION(InvalidValue, eh::DescriptiveException);

  template<typename Type>
  class ConstPointerHashAdapter
  {
  public:
    ConstPointerHashAdapter(const Type* ptr) noexcept
      : ptr_(ptr)
    {}

    size_t hash() const noexcept
    {
      return reinterpret_cast<size_t>(ptr_);
    }

    const Type* value() const noexcept
    {
      return ptr_;
    }

    bool operator==(const ConstPointerHashAdapter& right) const noexcept
    {
      return ptr_ == right.ptr_;
    }

  private:
    const Type* ptr_;
  };

  template<typename TargetType>
  TargetType cast_string(const char* str) /*throw(InvalidValue)*/
  {
    TargetType res;
    Stream::Parser istr(str);
    istr >> res;
    if(!istr.eof() || istr.fail())
    {
      Stream::Error ostr;
      ostr << "Non correct value : '" << str << "'.";
      throw InvalidValue(ostr);
    }
    return res;
  }

  struct PairEqual
  {
    template<typename FirstType, typename SecondType>
    bool
    operator()(
      const std::pair<FirstType, SecondType>& left,
      const std::pair<FirstType, SecondType>& right) const
    {
      return left == right;
    }
  };

  template<typename TargetType>
  struct MemoryInitAdapter
  {
    template<typename SourceType>
    TargetType operator()(const SourceType& src)
    {
      TargetType tgt;
      tgt.unsafe_init(src.get(), src.size());
      return tgt;
    }
  };

  struct FirstArg
  {
    template<typename FirstArgType, typename SecondArgType>
    const FirstArgType&
    operator()(const FirstArgType& left, const SecondArgType& /*right*/) const
    {
      return left;
    }
  };

  struct SecondArg
  {
    template<typename FirstArgType, typename SecondArgType>
    const SecondArgType&
    operator()(const FirstArgType& left, const SecondArgType& right) const
    {
      return right;
    }
  };

  template<typename IteratorType>
  struct IteratorRange: public std::pair<IteratorType, IteratorType>
  {
    typedef typename IteratorType::value_type value_type;
    typedef IteratorType const_iterator;

    IteratorRange() {}

    IteratorRange(const IteratorType& first, const IteratorType& second)
      : std::pair<IteratorType, IteratorType>(first, second)
    {}

    const IteratorType& begin() const
    {
      return this->first;
    }

    const IteratorType& end() const
    {
      return this->second;
    }
  };

  template<typename InsertIteratorType,
    typename FilterOpType>
  class FilterInsertIterator:
    public std::iterator<std::output_iterator_tag, void, void, void, void>
  {
  public:
    FilterInsertIterator(
      InsertIteratorType ins_it,
      FilterOpType filter_op)
      : ins_it_(ins_it), filter_op_(filter_op)
    {}

    template<typename ValueType>
    FilterInsertIterator& operator=(const ValueType& val)
    {
      if(filter_op_(val))
      {
        *(ins_it_++) = val;
      }

      return *this;
    }

    FilterInsertIterator& operator*()
    {
      return *this;
    }

    FilterInsertIterator& operator++()
    {
      return *this;
    }

    FilterInsertIterator operator++(int)
    {
      return *this;
    }

    InsertIteratorType base() const
    {
      return ins_it_;
    }

  private:
    InsertIteratorType ins_it_;
    FilterOpType filter_op_;
  };

  template<typename InsertIteratorType, typename ModifyOpType>
  class ModifyInsertIterator:
    public std::iterator<std::output_iterator_tag, void, void, void, void>
  {
  public:
    ModifyInsertIterator(
      InsertIteratorType ins_it,
      ModifyOpType modify_op)
      : ins_it_(ins_it), modify_op_(modify_op)
    {}

    template<typename ValueType>
    ModifyInsertIterator& operator=(const ValueType& val)
    {
      *(ins_it_++) = modify_op_(val);
      return *this;
    }

    ModifyInsertIterator& operator*()
    {
      return *this;
    }

    ModifyInsertIterator& operator++()
    {
      return *this;
    }

    ModifyInsertIterator& operator++(int)
    {
      return *this;
    }

    InsertIteratorType base() const
    {
      return ins_it_;
    }

  private:
    InsertIteratorType ins_it_;
    ModifyOpType modify_op_;
  };

  template<typename InsertIteratorType, typename FilterOpType>
  FilterInsertIterator<InsertIteratorType, FilterOpType>
  filter_inserter(InsertIteratorType ins_it, FilterOpType filter_op)
  {
    return FilterInsertIterator<InsertIteratorType, FilterOpType>(
      ins_it, filter_op);
  }

  template<typename InsertIteratorType, typename ModifyOpType>
  ModifyInsertIterator<InsertIteratorType, ModifyOpType>
  modify_inserter(InsertIteratorType ins_it, ModifyOpType modify_op)
  {
    return ModifyInsertIterator<InsertIteratorType, ModifyOpType>(
      ins_it, modify_op);
  }

  template<
    typename Iterator,
    typename UnaryFunction>
  class TransformIterator:
    public std::iterator<std::input_iterator_tag, void, void, void, void>
  {
  public:
    typedef decltype(std::declval<UnaryFunction>()(
      std::declval<typename Iterator::value_type>())) value_type;

    TransformIterator(
      const Iterator& iterator,
      UnaryFunction func)
      : iterator_(iterator), func_(func)
    {}

    value_type
    operator* () const
    {
      return func_(*iterator_);
    }

    TransformIterator&
    operator++()
    {
      ++iterator_;
      return *this;
    }

    bool
    operator== (const TransformIterator& arg)
    {
      return (iterator_ == arg.iterator_);
    }

    bool
    operator!= (const TransformIterator& arg)
    {
      return (iterator_ != arg.iterator_);
    }

  private:
    Iterator iterator_;
    UnaryFunction func_;
  };

  template<
    typename FirstInputIteratorType,
    typename SecondInputIteratorType,
    typename OutputIteratorType,
    typename LessOpType,
    typename MergeOpType>
  OutputIteratorType
  merge_unique(
    FirstInputIteratorType first_it, FirstInputIteratorType first_end,
    SecondInputIteratorType second_it, SecondInputIteratorType second_end,
    OutputIteratorType output_it,
    LessOpType less_op,
    MergeOpType merge_op)
  {
    while(first_it != first_end && second_it != second_end)
    {
      if(less_op(*first_it, *second_it))
      {
        *(output_it++) = *first_it;
        ++first_it;
      }
      else if(less_op(*second_it, *first_it))
      {
        *(output_it++) = *second_it;
        ++second_it;
      }
      else
      {
        *(output_it++) = merge_op(*first_it, *second_it);
        ++first_it;
        ++second_it;
      }
    }

    return std::copy(first_it, first_end,
      std::copy(second_it, second_end, output_it));
  }

  template<
    typename FirstInputIteratorType,
    typename SecondInputIteratorType,
    typename OutputIteratorType,
    typename LessOpType>
  OutputIteratorType
  merge_unique(
    FirstInputIteratorType first_it, FirstInputIteratorType first_end,
    SecondInputIteratorType second_it, SecondInputIteratorType second_end,
    OutputIteratorType output_it,
    LessOpType less_op)
  {
    return merge_unique(
      first_it, first_end,
      second_it, second_end,
      output_it,
      less_op,
      FirstArg());
  }

  template<
    typename FirstInputIteratorType,
    typename SecondInputIteratorType,
    typename OutputIteratorType>
  OutputIteratorType
  merge_unique(
    FirstInputIteratorType first_it, FirstInputIteratorType first_end,
    SecondInputIteratorType second_it, SecondInputIteratorType second_end,
    OutputIteratorType output_it)
  {
    return merge_unique(
      first_it, first_end,
      second_it, second_end,
      output_it,
      std::less<typename std::iterator_traits<FirstInputIteratorType>::
        value_type>(),
      FirstArg());
  }

  template<
    typename FirstInputIteratorType,
    typename SecondInputIteratorType,
    typename OutputIteratorType,
    typename LessOpType,
    typename CrossOpType>
  OutputIteratorType
  cross(
    FirstInputIteratorType first_it, FirstInputIteratorType first_end,
    SecondInputIteratorType second_it, SecondInputIteratorType second_end,
    OutputIteratorType output_it,
    LessOpType less_op, CrossOpType cross_op)
  {
    while(first_it != first_end && second_it != second_end)
    {
      if(less_op(*first_it, *second_it))
      {
        ++first_it;
      }
      else if(less_op(*second_it, *first_it))
      {
        ++second_it;
      }
      else
      {
        *(output_it++) = cross_op(*first_it, *second_it);
        ++first_it;
        ++second_it;
      }
    }

    return output_it;
  }

  template<typename RangeIteratorType, typename LessOpType>
  struct _RangeLess
  {
    _RangeLess() {}
    _RangeLess(const LessOpType& less_val): less_op(less_val) {}

    bool operator()(
      const RangeIteratorType& left, const RangeIteratorType& right) const
    {
      return less_op(*(left.first), *(right.first));
    }

    LessOpType less_op;
  };

  template<
    typename RangeInputInteratorType,
    typename OutputIteratorType,
    typename LessOpType>
  OutputIteratorType custom_merge_n(
    RangeInputInteratorType range_begin,
    RangeInputInteratorType range_end,
    OutputIteratorType output_it,
    LessOpType less_op) /*throw(eh::Exception)*/
  {
    typedef typename std::iterator_traits<
      RangeInputInteratorType>::value_type::const_iterator SubIterator;
    typedef IteratorRange<SubIterator> ItRange;
    typedef std::multiset<ItRange, _RangeLess<ItRange, LessOpType> > RangeSet;
    RangeSet range_set(less_op);

    /* init range set */
    for(; range_begin != range_end; ++range_begin)
    {
      if(range_begin->begin() != range_begin->end())
      {
        range_set.insert(ItRange(range_begin->begin(), range_begin->end()));
      }
    }

    /* merge loop */
    while(!range_set.empty())
    {
      typename RangeSet::iterator cur_it = range_set.begin();
      *output_it++ = *cur_it->first;

      /* update range set */
      ItRange cur_range = *cur_it;
      range_set.erase(cur_it++);
      if(++(cur_range.first) != cur_range.second)
      {
        range_set.insert(cur_range);
      }
    }

    return output_it;
  }

  template<typename IteratorType>
  IteratorRange<IteratorType>
  iterator_range(IteratorType begin, IteratorType end)
  {
    return IteratorRange<IteratorType>(begin, end);
  }

  template<typename OStream, typename IteratorType>
  OStream&
  print(OStream& out,
    IteratorType it_begin, IteratorType it_end, const char* delim = ", ")
  {
    for(IteratorType it = it_begin; it != it_end; ++it)
    {
      if(it != it_begin)
      {
        out << delim;
      }

      out << *it;
    }

    return out;
  }

  template<typename ContainerType, typename FieldType1, typename FieldType2>
  std::ostream& print_fields(
    std::ostream& out,
    const ContainerType& container,
    FieldType1 ContainerType::value_type::* field1,
    FieldType2 ContainerType::value_type::* field2,
    const char* delim = ",",
    const char* field_delim = " : ")
    /*throw(eh::Exception)*/
  {
    for(typename ContainerType::const_iterator it = container.begin();
        it != container.end(); ++it)
    {
      if(it != container.begin())
      {
        out << delim;
      }

      out << (*it).*field1 << field_delim << (*it).*field2;
    }

    return out;
  }
  
  inline
  Generics::Time
  round_to_day(const Generics::Time& time) noexcept
  {
    return Generics::Time(time.tv_sec / Generics::Time::ONE_DAY.tv_sec *
      Generics::Time::ONE_DAY.tv_sec);
  }

  inline
  Generics::SmartMemBuf_var
  copy_membuf(
    const Generics::ConstSmartMemBuf* ptr)
    /*throw(eh::Exception)*/
  {
    if(ptr)
    {
      Generics::SmartMemBuf_var res(new Generics::SmartMemBuf());
      res->membuf().assign(ptr->membuf().data(), ptr->membuf().size());
      return res;
    }

    return Generics::SmartMemBuf_var();
  }

  inline
  Generics::SmartMemBuf_var
  copy_membuf(
    const Generics::SmartMemBuf* ptr)
    /*throw(eh::Exception)*/
  {
    Generics::SmartMemBuf_var res(new Generics::SmartMemBuf());
    res->membuf().assign(ptr->membuf().data(), ptr->membuf().size());
    return res;
  }

  template<typename Writer>
  Generics::ConstSmartMemBuf_var
  save_to_membuf(
    const Writer& writer)
    /*throw(eh::Exception)*/
  {
    const unsigned long size = writer.size();
    Generics::SmartMemBuf_var mem_buf(new Generics::SmartMemBuf(size));
    writer.save(mem_buf->membuf().data(), size);
    return Generics::transfer_membuf(mem_buf);
  }
}

#endif /*_COMMONS_ALGS_HPP_*/
