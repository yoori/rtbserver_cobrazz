#ifndef COMMONS_INTERVAL_HPP_
#define COMMONS_INTERVAL_HPP_

#include <set>

namespace AdServer
{
namespace Commons
{
  template<typename ValueType>
  struct Interval
  {
    Interval(
      const ValueType& min_val,
      const ValueType& max_val)
      : min(min_val), max(max_val)
    {}

    bool
    operator<(const Interval& right) const
    {
      return min < right.min || (min == right.min &&
        max < right.max);
    }

    bool
    operator==(const Interval& right) const
    {
      return min == right.min && max == right.max;
    }

    ValueType min;
    ValueType max;
  };

  template<typename ValueType>
  struct IntervalSet: public std::set<Interval<ValueType> >
  {
    void
    normalize(
      const ValueType& min = std::numeric_limits<ValueType>::min(),
      const ValueType& max = std::numeric_limits<ValueType>::max())
      noexcept;

    bool
    contains(const ValueType& val) const noexcept;

    void
    minimize(const ValueType& min) noexcept;

    void
    cross(
      IntervalSet<ValueType>& res,
      const ValueType& min,
      const ValueType& max)
      const
      noexcept;

    void
    cross(const IntervalSet<ValueType>& res)
      noexcept;
  };
}
}

namespace AdServer
{
namespace Commons
{
  template<typename ValueType>
  void
  IntervalSet<ValueType>::normalize(const ValueType& min,
    const ValueType& max)
    noexcept
  {
    std::set<Interval<ValueType> > ints;

    ValueType pmax = min;
    for(typename std::set<Interval<ValueType> >::iterator it =
          this->begin(); it != this->end(); ++it)
    {
      ValueType rmin = std::max(pmax, it->min);
      ValueType rmax = std::min(it->max, max);
      if(rmin <= rmax)
      {
        if(rmin == pmax && !ints.empty())
        {
          // union intervals
          ValueType pmin = ints.rbegin()->min;
          ints.erase(--ints.end());
          ints.insert(Interval<ValueType>(pmin, rmax));
        }
        else
        {
          ints.insert(Interval<ValueType>(rmin, rmax));
        }

        pmax = rmax;
      }
    }

    this->swap(ints);
  }

  template<typename ValueType>
  bool
  IntervalSet<ValueType>::contains(const ValueType& val) const noexcept
  {
    auto lit = this->upper_bound(Interval<ValueType>(val, val));
    return (lit != this->end() && lit->min == val) ||
      (lit != this->begin() && (--lit)->min <= val && val < lit->max);
  }

  template<typename ValueType>
  void
  IntervalSet<ValueType>::cross(
    IntervalSet<ValueType>& res,
    const ValueType& min,
    const ValueType& max)
    const
    noexcept
  {
    typedef typename std::set<Interval<ValueType> >::const_iterator
      ConstIterator;

    if(min < max || min == max)
    {
      ConstIterator min_it = this->lower_bound(Interval<ValueType>(min, 0));
      ConstIterator max_it = this->upper_bound(Interval<ValueType>(max, 0));
      ConstIterator copy_begin_it = min_it;

      if(min_it != this->begin() && min < (--min_it)->max)
      {
        res.insert(Interval<ValueType>(
          min,
          std::min(min_it->max, max)));
      }

      if(copy_begin_it != max_it)
      {
        ConstIterator copy_end_it = max_it;

        if(max < (--max_it)->max)
        {
          res.insert(Interval<ValueType>(max_it->min, max));
          copy_end_it = max_it;
        }

        std::copy(copy_begin_it,
          copy_end_it,
          std::inserter(res, res.begin()));
      }
    }
  }

  template<typename ValueType>
  void
  IntervalSet<ValueType>::cross(const IntervalSet<ValueType>& right)
    noexcept
  {
    IntervalSet<ValueType> res;
    for(typename IntervalSet<ValueType>::const_iterator rit =
          right.begin();
        rit != right.end(); ++rit)
    {
      cross(res, rit->min, rit->max);
    }
    this->swap(res);
  }
}
}
#endif /* COMMONS_INTERVAL_HPP_ */
