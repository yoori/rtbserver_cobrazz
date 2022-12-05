#include <algorithm>

namespace AutoTest
{
namespace ORM
{
  template<typename StatsContainerType>
  StatsContainerWrapper<StatsContainerType>::StatsContainerWrapper(
    const StatsContainerWrapper<StatsContainerType>& init)
    : StatsContainerType(init)
  {}

  template<typename StatsContainerType>
  StatsContainerWrapper<StatsContainerType>&
  StatsContainerWrapper<StatsContainerType>::operator=(
    const StatsContainerWrapper<StatsContainerType>& init)
  {
    StatsContainerType::operator=(init.values_);
    return *this;
  }

  template<typename StatsContainerType>
  void
  StatsContainerWrapper<StatsContainerType>::print_idname(
    std::ostream& out) const
  {
    for(typename StatsContainerType::const_iterator it = this->begin();
        it != this->end(); ++it)
    {
      it->print_idname(out);
    }
  }

  template<typename StatsContainerType>
  bool
  StatsContainerWrapper<StatsContainerType>::select(
    StatsDB::IConn& connection,
    bool initial)
  {
    // Must handle all values
    bool ret = true;
    for(typename StatsContainerType::iterator it = this->begin();
        it != this->end(); ++it)
    {
      ret &= it->select(connection, initial);
    }
    return ret;
  }

  template<typename StatsContainerType>
  bool
  StatsContainerWrapper<StatsContainerType>::operator==(
    const StatsContainerType& right) const
  {
    return this->size() == right.size() &&
      std::equal(this->begin(), this->end(), right.begin());
  }

  template<typename StatsContainerType>
  bool
  StatsContainerWrapper<StatsContainerType>::operator!=(
    const StatsContainerType& stats) const
  {
    return !(*this == stats);
  }

  template<typename StatsContainerType>
  template<typename DiffType>
  void
  StatsContainerWrapper<StatsContainerType>::print_each_diff(
    std::ostream& out,
    const DiffType& diff)
  {
    StatsContainerType::value_type::print_diff(out, diff);
  }

  template<typename StatsContainerType>
  template<typename DiffIteratorType>
  void
  StatsContainerWrapper<StatsContainerType>::print_diff_array_(
    std::ostream& out,
    DiffIteratorType diff_it,
    DiffIteratorType diff_end_it)
  {
    bool first = true;
    for(; diff_it != diff_end_it; ++diff_it)
    {
      if(!first)
      {
        out << ",";
        first = false;
      }
      out << "{";
      StatsContainerType::value_type::print_diff(out, *diff_it);
      out << "}";
    }
  }

  template<typename StatsContainerType>
  template<typename IteratorType, typename DiffIteratorType>
  void
  StatsContainerWrapper<StatsContainerType>::print_diff_(
    std::ostream& out,
    IteratorType real_it,
    IteratorType real_end_it,
    DiffIteratorType diff_it,
    DiffIteratorType diff_end_it) const
  {
    for(typename StatsContainerType::const_iterator it = this->begin();
        it != this->end() &&
          real_it != real_end_it &&
          diff_it != diff_end_it;
        ++it, ++real_it, ++diff_it)
    {
      // Print only failed
      if (check_difference(*real_it, *it, *diff_it) != -1)
      {
        it->print_diff(out, *diff_it, *real_it);
      }
    }
  }

  template<typename StatsContainerType>
  template<typename IteratorType, typename DiffType>
  void
  StatsContainerWrapper<StatsContainerType>::print_each_diff_(
    std::ostream& out,
    IteratorType real_it,
    IteratorType real_end_it,
    const DiffType& diff) const
  {
    typename StatsContainerType::const_iterator it = this->begin();

    for(; it != this->end() && real_it != real_end_it;
        ++it, ++real_it)
    {
      // Print only failed
      if (check_difference(*real_it, *it, diff) != -1)
      {
        it->print_diff(out, diff, *real_it);
      }
    }

    if(it != this->end() || real_it != real_end_it)
    {
      Stream::Error ostr;
      ostr << "incorrect diff array size";
      throw Exception(ostr);
    }
  }

  template<typename StatsContainerType>
  std::ostream& operator<<(
    std::ostream& out,
    const StatsContainerWrapper<StatsContainerType>& stats)
  {
    for(typename StatsContainerWrapper<StatsContainerType>::
          const_iterator it = stats.begin();
        it != stats.end(); ++it)
    {
      out << *it;
    }
    return out;
  }

  template<typename ValueType, std::size_t SIZE>
  template<typename DiffType>
  void
  StatsArray<ValueType, SIZE>::print_diff(
    std::ostream& out,
    const DiffType (&diff)[SIZE],
    const StatsArray<ValueType, SIZE>& real)
    const
  {
    return this->print_diff_(
      out,
      real.begin(),
      real.end(),
      diff,
      diff + SIZE);
  } 

  template<typename ValueType, std::size_t SIZE>
  template<typename DiffType>
  void
  StatsArray<ValueType, SIZE>::print_diff(
    std::ostream& out,
    const DiffType (&diff)[SIZE])
  {
    StatsContainerWrapper<FixedArray<ValueType, SIZE> >::
      print_diff_array_(out, diff, diff + SIZE);
  }

  template<typename ValueType, std::size_t SIZE>
  template<typename DiffType>
  void
  StatsArray<ValueType, SIZE>::print_each_diff(
    std::ostream& out,
    const DiffType& diff,
    const StatsArray<ValueType, SIZE>& real)
    const
  {
    this->print_each_diff_(
      out,
      real.begin(),
      real.end(),
      diff);
  } 

  template<typename ValueType>
    template<typename StatsDiffContainerType, typename StatsContainerType>
  void StatsList<ValueType>::print_diff(
    std::ostream& out,
    const StatsDiffContainerType& diffs,
    const StatsContainerType& real)
    const
  {
    this->print_diff_(
      out,
      real.begin(),
      real.end(),
      diffs.begin(),
      diffs.end());
  }

  template<typename ValueType>
    template<typename StatsDiffContainerType>
  void StatsList<ValueType>::print_diff(
    std::ostream& out,
    const StatsDiffContainerType& diffs)
  {
    StatsContainerWrapper<std::list<ValueType> >::print_diff_array_(
      out,
      diffs.begin(),
      diffs.end());
  }

  template<typename ValueType>
  template<typename DiffType, typename StatsContainerType>
  void StatsList<ValueType>::print_each_diff(
    std::ostream& out,
    const DiffType& diff,
    const StatsContainerType& real)
    const
  {
    this->print_each_diff_(
      out,
      real.begin(),
      real.end(),
      diff);
  }
}
}
