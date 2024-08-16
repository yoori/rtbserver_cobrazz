#ifndef AD_SERVER_LOG_PROCESSING_STAT_COLLECTOR_HPP
#define AD_SERVER_LOG_PROCESSING_STAT_COLLECTOR_HPP


#include <iosfwd>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <Generics/GnuHashTable.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <eh/Exception.hpp>

namespace AdServer {
namespace LogProcessing {

template <class T_>
class IsCollector
{
  typedef char True_;

  struct False_ { True_ x[2]; };

  template <class U_>
  static
  True_
  has_collector_tag(typename U_::CollectorTag*);

  template <class U_>
  static
  False_
  has_collector_tag(...);

public:
  static const bool value = sizeof(has_collector_tag<T_>(0)) == sizeof(True_);
};

namespace StatCollectorImplDefs_ {

template <class STAT_COLLECTOR_, bool IS_NESTED_>
struct CopyImpl;

template <class STAT_COLLECTOR_>
struct CopyImpl<STAT_COLLECTOR_, false>
{
  static
  void
  deep_copy(const STAT_COLLECTOR_& src, STAT_COLLECTOR_& dest)
  {
    dest.clear();
    dest.insert_i_(src.begin(), src.end());
  }
};

template <class STAT_COLLECTOR_>
struct CopyImpl<STAT_COLLECTOR_, true>
{
  static
  void
  deep_copy(const STAT_COLLECTOR_& src, STAT_COLLECTOR_& dest)
  {
    dest.clear();
    for (auto it = src.begin(); it != src.end(); ++it)
    {
      typename STAT_COLLECTOR_::DataT data_copy;
      it->second.deep_copy(data_copy);
      dest.add(it->first, data_copy);
    }
  }
};

template <class STAT_COLLECTOR_, bool EXCLUDE_NULL_VALUES_>
struct ValueOps;

template <class STAT_COLLECTOR_>
struct ValueOps<STAT_COLLECTOR_, false>
{
  template <class DATA_>
  static bool excludable(const DATA_&)
  {
    return false;
  }

  static
  void
  remove_if_excludable(STAT_COLLECTOR_&, typename STAT_COLLECTOR_::iterator)
  {
  }

  template <class INPUT_ITERATOR_>
  static
  void
  insert_non_excludables(
    STAT_COLLECTOR_& sc,
    INPUT_ITERATOR_ first,
    INPUT_ITERATOR_ last
  )
  {
    sc.insert_i_(first, last);
  }
};

template <class STAT_COLLECTOR_>
struct ValueOps<STAT_COLLECTOR_, true>
{
  template <class DATA_>
  static bool excludable(const DATA_& data)
  {
    return data.is_null();
  }

  static
  void
  remove_if_excludable(
    STAT_COLLECTOR_& sc,
    typename STAT_COLLECTOR_::iterator it
  )
  {
    if (excludable(it->second))
    {
      sc.erase(it);
    }
  }

  template <class INPUT_ITERATOR_>
  static
  void
  insert_non_excludables(
    STAT_COLLECTOR_& sc,
    INPUT_ITERATOR_ first,
    INPUT_ITERATOR_ last
  )
  {
    for (auto it = first; it != last; ++it)
    {
      if (!excludable(it->second))
      {
        sc.insert_i_(sc.end(), *it);
      }
    }
  }
};

template <class STAT_COLLECTOR_, bool USE_FIXED_BUF_STREAM_>
struct ValueReader;

template <class STAT_COLLECTOR_>
struct ValueReader<STAT_COLLECTOR_, false>
{
  static
  std::istream&
  read(
    std::istream& is,
    typename STAT_COLLECTOR_::ValueT& value,
    unsigned long& line_num
  )
  {
    typedef typename STAT_COLLECTOR_::KeyT KeyT;
    typedef typename STAT_COLLECTOR_::DataT DataT;
    typedef typename STAT_COLLECTOR_::Exception Exception;

    auto line_begin_pos = is.tellg();
    if (!(is >> const_cast<KeyT&>(static_cast<const KeyT&>(value.first))))
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__
         << ": Failed to read key from istream (line number = " << line_num
         << ", position = " << is.tellg() - line_begin_pos << ')';
      throw Exception(es);
    }
    char separator = is.get();
    if (separator != '\t')
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__
         << ": Key and data must be separated by TAB (0x09), and not by '"
         << separator << "' (" << std::showbase << std::hex
         << short(separator) << "). Line number = " << std::dec
         << line_num << ", position = " << is.tellg() - line_begin_pos << '.';
      throw Exception(es);
    }
    if (!(is >> static_cast<DataT&>(value.second)))
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__
         << ": Failed to read data from istream (line number = "
         << line_num << ", position = " << is.tellg() - line_begin_pos << ")";
      throw Exception(es);
    }
    if (!read_eol(is))
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << (is.eof() ?
        ": Malformed file (file must end with an end-of-line character), line "
        : ": Extra data at the end of line ") << line_num;
      throw Exception(es);
    }
    return is;
  }
};

template <class STAT_COLLECTOR_>
struct ValueReader<STAT_COLLECTOR_, true>
{
  ValueReader()
  {
    const std::size_t RESERVE_SIZE = 1024;
    line_.reserve(RESERVE_SIZE);
  }

  std::istream&
  read(
    std::istream& is,
    typename STAT_COLLECTOR_::ValueT& value,
    unsigned long& line_num
  )
  {
    typedef typename STAT_COLLECTOR_::KeyT KeyT;
    typedef typename STAT_COLLECTOR_::DataT DataT;
    typedef typename STAT_COLLECTOR_::Exception Exception;

    line_.clear();
    read_until_eol(is, line_, false);

    if (is.eof())
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ <<
        ": Malformed file (file must end with an end-of-line character), line " <<
        line_num;
      throw Exception(es);
    }

    if (is.good())
    {
      FixedBufStream<TabCategory> fbs(line_);
      if (!(fbs >> const_cast<KeyT&>(static_cast<const KeyT&>(value.first))))
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__
           << ": Failed to read key from istream (line number = " << line_num
           << ")";
        throw Exception(es);
      }
      if (!(fbs >> static_cast<DataT&>(value.second)))
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__
           << ": Failed to read data from istream (line number = " << line_num
           << ")";
        throw Exception(es);
      }
      fbs.transfer_state(is);
    }

    return is;
  }

protected:
  std::string line_;
};

} // namespace StatCollectorImplDefs_

// Random-access collector
// NOTE: This class behaves like a smart pointer and must be used accordingly
template <
  class KEY_,
  class DATA_,
  bool EXCLUDE_NULL_VALUES_ = false,
  bool USE_FIXED_BUF_STREAM_ = false,
  bool STOP_AT_BLANK_LINE_ = true,
  bool ORDERED_ = false
>
class StatCollector
{
  friend
  class StatCollectorImplDefs_::CopyImpl<StatCollector,
    IsCollector<DATA_>::value>;

  typedef class StatCollectorImplDefs_::CopyImpl<StatCollector,
    IsCollector<DATA_>::value>
      CopyImpl_;

  friend
  class StatCollectorImplDefs_::ValueOps<StatCollector, EXCLUDE_NULL_VALUES_>;

  typedef StatCollectorImplDefs_::ValueOps<StatCollector, EXCLUDE_NULL_VALUES_>
    ValueOps_;

  friend
  class StatCollectorImplDefs_::
    ValueReader<StatCollector, USE_FIXED_BUF_STREAM_>;

  typedef StatCollectorImplDefs_::
    ValueReader<StatCollector, USE_FIXED_BUF_STREAM_>
      ValueReader_;

public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef KEY_ KeyT;
  typedef DATA_ DataT;

  struct CollectorTag {};

  StatCollector(): map_impl_(new RefCountableMapImpl_) {}

  template<typename Range>
  StatCollector(const Range& range): map_impl_(new RefCountableMapImpl_)
  {
    *this += range;
  }

  StatCollector(const StatCollector&) = default;
  StatCollector(StatCollector&&) = default;
  StatCollector& operator=(const StatCollector& collector) = default;
  StatCollector& operator=(StatCollector&& collector) = default;

  ~StatCollector() = default;

  bool operator==(const StatCollector& collector) const
  {
    return map_impl_ == collector.map_impl_ ||
      *map_impl_ == *collector.map_impl_;
  }

  template<typename Mediator>
  StatCollector&
  add(
    const KeyT& key,
    const Mediator& data)
  {
    auto ins_res = map_impl_->insert(ValueT(key, DataT()));
    
    if (ins_res.second)
    {
      ins_res.first->second = DataT(data);
    }
    else
    {
      ins_res.first->second += data;
      ValueOps_::remove_if_excludable(*this, ins_res.first);
    }
	
    return *this;
  }

  StatCollector&
  move_data(const KEY_& key, DATA_& data)
  {
    if (ValueOps_::excludable(data))
    {
      return *this;
    }
    auto ins_res = map_impl_->insert(ValueT(key, data));
    if (!ins_res.second)
    {
      ins_res.first->second.merge(data);
      ValueOps_::remove_if_excludable(*this, ins_res.first);
    }
    return *this;
  }

  StatCollector& operator+=(const StatCollector& collector)
  {
    if (map_impl_ == collector.map_impl_)
    {
      for (auto it = begin(); it != end(); )
      {
        it->second += it->second;
        ValueOps_::remove_if_excludable(*this, it++);
      }
      return *this;
    }
    std::for_each(collector.begin(), collector.end(), add_elem(*this));
    return *this;
  }

  template<typename Container>
  StatCollector&
  operator+= (const Container& collector)
  {
    for (auto it = collector.begin(); it != collector.end(); ++it)
    {
      const typename Container::value_type value(*it);
      add(value.first, value.second);
    }

    return *this;
  }

  void
  merge(const StatCollector& collector)
  {
    *this += collector;
  }

  void load(std::istream& is) /*throw(eh::Exception, Exception)*/
  {
    StatCollector tmp;
    load_i_(is, tmp);
    if (empty())
    {
      swap(tmp); // No need to merge
    }
    else
    {
      *this += tmp;
    }
  }

  void dump(std::ostream& os) const
    /*throw(eh::Exception, Exception)*/;

  void dump(const std::string& filename) const
    /*throw(eh::Exception, Exception)*/
  {
    std::ofstream ofs(filename.c_str());
    dump(ofs);
  }

  size_t size() const
  {
    return map_impl_->size();
  }

  bool empty() const
  {
    return map_impl_->empty();
  }

  void clear()
  {
    map_impl_->clear();
  }

  void swap(StatCollector& other)
  {
    map_impl_->swap(*other.map_impl_);
  }

  bool is_null() const
  {
    return empty();
  }

  template <class K_, class D_, bool EX_, bool U_F_, bool S_, bool O_>
  friend
  std::istream&
  operator>>(
    std::istream& is,
    StatCollector<K_, D_, EX_, U_F_, S_, O_>& collector
  )
    /*throw(eh::Exception,
      typename StatCollector<K_, D_, EX_, U_F_, S_, O_>::Exception)*/;

  template <class K_, class D_, bool EX_, bool U_F_, bool S_, bool O_>
  friend
  std::ostream&
  operator<<(
    std::ostream& os,
    const StatCollector<K_, D_, EX_, U_F_, S_, O_>& collector
  )
    /*throw(eh::Exception,
      typename StatCollector<K_, D_, EX_, U_F_, S_, O_>::Exception)*/;

private:
  template <class M_KEY_, class M_DATA_, bool M_ORDERED_>
  struct MapImplTypedefHelper;

  template <class M_KEY_, class M_DATA_>
  struct MapImplTypedefHelper<M_KEY_, M_DATA_, false>
  {
    struct Type: public Generics::GnuHashTable<M_KEY_, M_DATA_>
    {
      void
      prepare_adding(unsigned long add_size)
      {
        this->rehash(this->size() + add_size);
      }
    };
  };

  template <class M_KEY_, class M_DATA_>
  struct MapImplTypedefHelper<M_KEY_, M_DATA_, true>
  {
    struct Type: public std::map<M_KEY_, M_DATA_>
    {
      void
      prepare_adding(unsigned long)
      {}
    };
  };

  typedef typename MapImplTypedefHelper<KEY_, DATA_, ORDERED_>::Type MapImpl_;

public:
  typedef typename MapImpl_::value_type ValueT;
  typedef typename MapImpl_::iterator iterator;
  typedef typename MapImpl_::const_iterator const_iterator;

  template <class INPUT_ITERATOR_>
  void insert(INPUT_ITERATOR_ first, INPUT_ITERATOR_ last)
  {
    ValueOps_::insert_non_excludables(*this, first, last);
  }

  iterator begin()
  {
    return map_impl_->begin();
  }

  const_iterator begin() const
  {
    return map_impl_->begin();
  }

  iterator end()
  {
    return map_impl_->end();
  }

  const_iterator end() const
  {
    return map_impl_->end();
  }

  iterator find(const KeyT& key)
  {
    return map_impl_->find(key);
  }

  const_iterator find(const KeyT& key) const
  {
    return map_impl_->find(key);
  }

  DataT& find_or_insert(const KeyT& key)
  {
    return (*map_impl_)[key];
  }

  void erase(iterator it)
  {
    map_impl_->erase(it);
  }

  void
  prepare_adding(unsigned long add_elements)
  {
    map_impl_->prepare_adding(add_elements);
  }

  StatCollector&
  add(const ValueT& value)
  {
    if (ValueOps_::excludable(value.second))
    {
      return *this;
    }
    auto ins_res = map_impl_->emplace(value);
    if (!ins_res.second)
    {
      ins_res.first->second += value.second;
      ValueOps_::remove_if_excludable(*this, ins_res.first);
    }
    return *this;
  }

  void
  deep_copy(StatCollector& dest) const
  {
    CopyImpl_::deep_copy(*this, dest);
  }

private:
  iterator insert(iterator it, const ValueT& value)
  {
    if (ValueOps_::excludable(value.second))
    {
      return it;
    }
    return insert_i_(it, value);
  }

  iterator insert_i_(iterator it, const ValueT& value)
  {
    return map_impl_->insert(it, value);
  }

  template <class INPUT_ITERATOR_>
  void insert_i_(INPUT_ITERATOR_ first, INPUT_ITERATOR_ last)
  {
    map_impl_->insert(first, last);
  }

  void load_i_(std::istream& is, StatCollector& dest)
    /*throw(eh::Exception, Exception)*/;

  struct add_elem: public std::unary_function<ValueT, void>
  {
    add_elem(StatCollector& collector): collector_(collector) {}

    void operator()(const ValueT& value)
    {
      if (ValueOps_::excludable(value.second))
      {
        return;
      }
      auto ins_res = collector_.map_impl_->insert(value);
      if (!ins_res.second)
      {
        ins_res.first->second += value.second;
        ValueOps_::remove_if_excludable(collector_, ins_res.first);
      }
    }

    StatCollector& collector_;
  };

  struct output_elem: public std::unary_function<ValueT, void>
  {
    output_elem(std::ostream& os): os_(os) {}

    void operator()(const ValueT& value)
    {
      os_ << value.first << '\t' << value.second << '\n';
    }

    std::ostream& os_;
  };

  struct RefCountableMapImpl_:
    public MapImpl_,
    public ReferenceCounting::AtomicImpl
  {
  private:
    virtual ~RefCountableMapImpl_() noexcept {}
  };

  typedef typename ReferenceCounting::AssertPtr<RefCountableMapImpl_>::Ptr
    RefCountableMapImpl_var;

  RefCountableMapImpl_var map_impl_;
};

template <class KEY_, class DATA_, bool EX_, bool U_F_, bool S_, bool O_>
inline
std::istream&
operator>>(
  std::istream& is,
  StatCollector<KEY_, DATA_, EX_, U_F_, S_, O_>& collector
)
{
  collector.load(is);
  return is;
}

template <class KEY_, class DATA_, bool EX_, bool U_F_, bool S_, bool O_>
inline
std::ostream&
operator<<(
  std::ostream& os,
  const StatCollector<KEY_, DATA_, EX_, U_F_, S_, O_>& collector
)
{
  collector.dump(os);
  return os;
}

template <class KEY_, class DATA_, bool EX_, bool FBUF_, bool STOP_, bool ORD_>
void
StatCollector<KEY_, DATA_, EX_, FBUF_, STOP_, ORD_>::load_i_(
  std::istream& is,
  StatCollector& dest
)
  /*throw(eh::Exception, Exception)*/
{
  ValueReader_ value_reader;
  unsigned long line_num = 1;
  for (ValueT value; !is.eof(); ++line_num)
  {
    value_reader.read(is, value, line_num);
    auto ins_res = dest.map_impl_->insert(value);
    if (!ins_res.second)
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__
         << ": Malformed file (duplicate key found), line " << line_num;
      throw Exception(es);
    }
    char peek_sym = is.peek();
    if (STOP_ && peek_sym == '\n')
    {
      break;
    }
  }
}

template <class KEY_, class DATA_, bool EX_, bool U_F_, bool S_, bool O_>
void
StatCollector<KEY_, DATA_, EX_, U_F_, S_, O_>::dump(std::ostream& os) const
  /*throw(eh::Exception, Exception)*/
{
  std::for_each(begin(), end(), output_elem(os));
  if (!os.good())
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Failed to write collector to ostream";
    throw Exception(es);
  }
}

namespace SeqCollectorImplDefs_ {

template <class SEQ_COLLECTOR_, bool IS_NESTED_>
struct CopyImpl;

template <class SEQ_COLLECTOR_>
struct CopyImpl<SEQ_COLLECTOR_, false>
{
  static
  void
  deep_copy(const SEQ_COLLECTOR_& src, SEQ_COLLECTOR_& dest)
  {
    dest.clear();
    dest.insert(dest.end(), src.begin(), src.end());
  }
};

template <class SEQ_COLLECTOR_>
struct CopyImpl<SEQ_COLLECTOR_, true>
{
  static
  void
  deep_copy(const SEQ_COLLECTOR_& src, SEQ_COLLECTOR_& dest)
  {
    dest.clear();
    for (auto it = src.begin(); it != src.end(); ++it)
    {
      typename SEQ_COLLECTOR_::DataT data_copy;
      it->deep_copy(data_copy);
      dest.add(data_copy);
    }
  }
};

template <class SEQ_COLLECTOR_, bool USE_FIXED_BUF_STREAM_>
struct ValueReader;

template <class SEQ_COLLECTOR_>
struct ValueReader<SEQ_COLLECTOR_, false>
{
  static
  std::istream&
  read(
    std::istream& is,
    typename SEQ_COLLECTOR_::DataT& value,
    unsigned long& line_num
  )
  {
    typedef typename SEQ_COLLECTOR_::Exception Exception;

    is >> value;
    if (is.fail())
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__
         << ": Failed to read data from istream (line number = "
         << line_num;
      throw Exception(es);
    }
    if (!read_eol(is))
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ << (is.eof() ?
        ": Malformed file (file must end with an end-of-line character), line "
        : ": Extra data at the end of line ") << line_num;
      throw Exception(es);
    }
    return is;
  }
};

template <class SEQ_COLLECTOR_>
struct ValueReader<SEQ_COLLECTOR_, true>
{
  ValueReader()
  {
    const std::size_t RESERVE_SIZE = 1024;
    line_.reserve(RESERVE_SIZE);
  }

  std::istream&
  read(
    std::istream& is,
    typename SEQ_COLLECTOR_::DataT& value,
    unsigned long& line_num
  )
  {
    typedef typename SEQ_COLLECTOR_::Exception Exception;

    line_.clear();
    read_until_eol(is, line_, false);

    if (is.eof())
    {
      Stream::Error es;
      es << __PRETTY_FUNCTION__ <<
        ": Malformed file (file must end with an end-of-line character), line " <<
        line_num;
      throw Exception(es);
    }

    if (is.good())
    {
      FixedBufStream<TabCategory> fbs(line_);
      if (!(fbs >> value))
      {
        Stream::Error es;
        es << __PRETTY_FUNCTION__
           << ": Failed to read data from istream (FixedBufStream) (line number = " << line_num
           << ")";
        throw Exception(es);
      }
      fbs.transfer_state(is);
    }

    return is;
  }

protected:
  std::string line_;
};
} // namespace SeqCollectorImplDefs_

// Sequential collector
// NOTE: This class behaves like a smart pointer and must be used accordingly
template <class DATA_, bool USE_FIXED_BUF_STREAM_ = false, bool STOP_AT_BLANK_LINE_ = true>
class SeqCollector
{
  typedef std::list<DATA_> Base;

  friend
  class SeqCollectorImplDefs_::CopyImpl<SeqCollector,
    IsCollector<DATA_>::value>;

  typedef class SeqCollectorImplDefs_::CopyImpl<SeqCollector,
    IsCollector<DATA_>::value>
      CopyImpl_;

  friend
  class SeqCollectorImplDefs_::
    ValueReader<SeqCollector, USE_FIXED_BUF_STREAM_>;

  typedef SeqCollectorImplDefs_::
    ValueReader<SeqCollector, USE_FIXED_BUF_STREAM_>
      ValueReader_;

public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  typedef DATA_ DataT;
  typedef typename Base::iterator iterator;
  typedef typename Base::const_iterator const_iterator;

  struct CollectorTag {};

private:
  class Holder: private Base, public ReferenceCounting::AtomicCopyImpl
  {
    friend class SeqCollector;

  public:
    Holder() noexcept: size_() {}

    using Base::begin;
    using Base::end;
    using Base::empty;

    void
    operator+=(const Holder& collector)
    {
      Base::insert(end(), collector.begin(), collector.end());
      size_ += collector.size_;
    }

    template <typename InputIterator>
    void
    insert(iterator position, InputIterator first, InputIterator last)
    {
      for (; first != last; ++first, ++size_)
      {
        Base::insert(position, *first);
      }
    }

  private:
    virtual
    ~Holder() noexcept {}

    void
    splice_(Holder& collector) noexcept
    {
      Base::splice(end(), collector);
      size_ += collector.size_;
      collector.size_ = 0;
    }

    std::size_t size_;
  };

  typedef typename ReferenceCounting::AssertPtr<Holder>::Ptr Holder_var;
  Holder_var holder_;

public:
  SeqCollector() /*throw(eh::Exception)*/: holder_(new Holder) {}

  SeqCollector&
  add(const DATA_& data)
  {
    holder_->push_back(data);
    ++(holder_->size_);
    return *this;
  }

  SeqCollector& operator+=(const SeqCollector& collector)
  {
    if (holder_.in() == collector.holder_.in())
    {
      Holder_var holder = new Holder(*collector.holder_);
      *holder += *collector.holder_;
      holder_ = holder;
    }
    else
    {
      *holder_ += *collector.holder_;
    }
    return *this;
  }

  void
  merge(SeqCollector& collector) noexcept
  {
    assert(holder_.in() != collector.holder_.in());
    holder_->splice_(*collector.holder_);
  }

  void load(std::istream& is) /*throw(eh::Exception, Exception)*/;

  void dump(std::ostream& os) const /*throw(eh::Exception, Exception)*/;

  void dump(const std::string& filename) const
    /*throw(eh::Exception, Exception)*/
  {
    std::ofstream ofs(filename.c_str());
    dump(ofs);
  }

  bool operator==(const SeqCollector& collector) const
  {
    return holder_->size_ == collector.holder_->size_ &&
      static_cast<const Base&>(*holder_) == *collector.holder_;
  }

  iterator
  begin() noexcept
  {
    return holder_->begin();
  }

  const_iterator
  begin() const noexcept
  {
    return holder_->begin();
  }

  iterator
  end() noexcept
  {
    return holder_->end();
  }

  const_iterator
  end() const noexcept
  {
    return holder_->end();
  }

  bool
  empty() const noexcept
  {
    return holder_->empty();
  }

  void
  clear() noexcept
  {
    holder_->Base::clear();
    holder_->size_ = 0;
  }

  std::size_t
  size() const noexcept
  {
    return holder_->size_;
  }

  void swap(SeqCollector& other)
  {
    holder_->Base::swap(*other.holder_);
    std::swap(holder_->size_, other.holder_->size_);
  }

  void
  deep_copy(SeqCollector& dest) const
  {
    CopyImpl_::deep_copy(*this, dest);
  }

  template <typename InputIterator>
  void
  insert(iterator position, InputIterator first, InputIterator last)
  {
    holder_->insert(position, first, last);
  }

  template <class Predicate>
  void
  remove_if(const Predicate& predicate)
  {
    holder_->remove_if(predicate);
  }

  template <class D_, bool USE_FSB_, bool S_>
  friend
  std::istream&
  operator>>(std::istream& is, SeqCollector<D_, USE_FSB_, S_>& collector)
    /*throw(eh::Exception, typename SeqCollector<D_, USE_FSB_, S_>::Exception)*/;

  template <class D_, bool USE_FSB_, bool S_>
  friend
  std::ostream&
  operator<<(std::ostream& os, const SeqCollector<D_, USE_FSB_, S_>& collector)
    /*throw(eh::Exception, typename SeqCollector<D_, USE_FSB_, S_>::Exception)*/;
};

template <typename Key, typename InnerKey, typename InnerData>
struct NestedStatCollector
{
  typedef StatCollector<Key, StatCollector<InnerKey, InnerData> > Type;
};

template <class DATA_, bool USE_FIXED_BUF_STREAM_, bool STOP_>
inline
void
SeqCollector<DATA_, USE_FIXED_BUF_STREAM_, STOP_>::load(std::istream& is)
  /*throw(eh::Exception, Exception)*/
{
  SeqCollector tmp;
  unsigned long line_num = 1;
  ValueReader_ value_reader;

  for (DataT value; is.peek(), !is.eof(); ++line_num)
  {
    value_reader.read(is, value, line_num);

    tmp.add(value);
    if (STOP_ && is.peek() == '\n')
    {
      break;
    }
  }
  holder_->splice_(*tmp.holder_);
}

template <class DATA_, bool USE_FIXED_BUF_STREAM_, bool STOP_>
inline
void
SeqCollector<DATA_, USE_FIXED_BUF_STREAM_, STOP_>::dump(std::ostream& os) const
  /*throw(eh::Exception, Exception)*/
{
  std::copy(begin(), end(), std::ostream_iterator<DATA_>(os, "\n"));
  if (!os.good())
  {
    Stream::Error es;
    es << __PRETTY_FUNCTION__ << ": Failed to write collector to ostream";
    throw Exception(es);
  }
}

template <class DATA_, bool USE_FIXED_BUF_STREAM_, bool STOP_>
inline
std::istream&
operator>>(std::istream& is, SeqCollector<DATA_, USE_FIXED_BUF_STREAM_, STOP_>& collector)
  /*throw(eh::Exception, typename SeqCollector<DATA_, USE_FIXED_BUF_STREAM_, STOP_>::Exception)*/
{
  collector.load(is);
  return is;
}

template <class DATA_, bool USE_FIXED_BUF_STREAM_, bool STOP_>
inline
std::ostream&
operator<<(std::ostream& os, const SeqCollector<DATA_, USE_FIXED_BUF_STREAM_, STOP_>& collector)
  /*throw(eh::Exception, typename SeqCollector<DATA_, USE_FIXED_BUF_STREAM_, STOP_>::Exception)*/
{
  collector.dump(os);
  return os;
}

} // namespace LogProcessing
} // namespace AdServer

#endif /* AD_SERVER_LOG_PROCESSING_STAT_COLLECTOR_HPP */

