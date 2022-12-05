#ifndef USERINFOSVCS_USERBINDSERVER_FETCHABLEHASHTABLE_HPP
#define USERINFOSVCS_USERBINDSERVER_FETCHABLEHASHTABLE_HPP

#include <deque>
#include <bitset>
#include <Generics/GnuHashTable.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Commons/tsl/sparse_set.h>

namespace AdServer
{
namespace UserInfoSvcs
{
  struct bit_deque
  {
    typedef bool const_reference;

    class reference
    {
    public:
      reference(bit_deque& owner, int pos)
        : owner_(owner),
          pos_(pos)
      {}

      operator bool() const
      {
        return owner_.get(pos_);
      }
      
      reference&
      operator=(bool value)
      {
        owner_.set(pos_, value);
        return *this;
      }

    protected:
      bit_deque& owner_;
      int pos_;
    };

    class iterator;

    class const_iterator: public std::iterator<
      std::random_access_iterator_tag,
      bool,
      int,
      const bool*,
      const_reference>
    {
      friend class bit_deque;
      friend class iterator;

    public:
      const_iterator()
        : owner_(nullptr),
          pos_(0)
      {}
      
      const_iterator&
      operator++() 
      {
        ++pos_;
        return *this;
      }

      const_iterator
      operator++(int) 
      {
        const_iterator retval = *this;
        ++pos_;
        return retval;
      }

      const_iterator&
      operator--() 
      {
        ++pos_;
        return *this;
      }

      const_iterator
      operator--(int) 
      {
        const_iterator retval = *this;
        ++pos_;
        return retval;
      }

      const_iterator&
      operator+=(int diff) 
      {
        pos_ += diff;
        return *this;
      }

      const_iterator&
      operator-=(int diff) 
      {
        pos_ -= diff;
        return *this;
      }

      const_iterator
      operator+(int diff) 
      {
        const_iterator retval = *this;
        retval.pos_ += diff;
        return retval;
      }

      const_iterator
      operator-(int diff) 
      {
        const_iterator retval = *this;
        retval.pos_ -= diff;
        return retval;
      }

      bool
      operator==(const_iterator other) const 
      {
        return pos_ == other.pos_;
      }

      bool
      operator!=(const_iterator other) const 
      {
        return pos_ != other.pos_;
      }

      const_reference
      operator*() const 
      {
        return owner_->get(pos_);
      }

    protected:
      const_iterator(const bit_deque* owner, int pos) 
        : owner_(owner),
          pos_(pos)
      {}

    protected:
      const bit_deque* owner_;
      int pos_;
    };

    class iterator: public std::iterator<
      std::random_access_iterator_tag,
      reference,
      int,
      reference*,
      reference>
    {
      friend class bit_deque;

    public:
      iterator()
        : owner_(nullptr),
          pos_(0)
      {}
      
      iterator&
      operator++() 
      {
        ++pos_;
        return *this;
      }

      iterator
      operator++(int) 
      {
        iterator retval = *this;
        ++pos_;
        return retval;
      }

      iterator&
      operator--() 
      {
        ++pos_;
        return *this;
      }

      iterator
      operator--(int) 
      {
        iterator retval = *this;
        ++pos_;
        return retval;
      }

      iterator&
      operator+=(int diff) 
      {
        pos_ += diff;
        return *this;
      }

      iterator&
      operator-=(int diff) 
      {
        pos_ -= diff;
        return *this;
      }

      iterator
      operator+(int diff) 
      {
        iterator retval = *this;
        retval.pos_ += diff;
        return retval;
      }

      iterator
      operator-(int diff) 
      {
        iterator retval = *this;
        retval.pos_ -= diff;
        return retval;
      }

      bool
      operator==(iterator other) const 
      {
        return pos_ == other.pos_;
      }

      bool
      operator!=(iterator other) const 
      {
        return pos_ != other.pos_;
      }

      reference
      operator*() const 
      {
        reference ret(*owner_, pos_);
        return ret;
      }

      operator const_iterator()
      {
        return const_iterator(owner_, pos_);
      }

    protected:
      iterator(bit_deque* owner, int pos) 
        : owner_(owner),
          pos_(pos)
      {}

    protected:
      bit_deque* owner_;
      int pos_;
    };

    bit_deque()
      : size_(0)
    {}

    reference
    operator[](int pos)
    {
      return reference(*this, pos);
    }

    const_reference
    operator[](int pos) const
    {
      return get(pos);
    }

    iterator
    begin()
    {
      return iterator(this, 0);
    }

    iterator
    end()
    {
      return iterator(this, size_);
    }

    const_iterator
    begin() const
    {
      return const_iterator(this, 0);
    }

    const_iterator
    end() const
    {
      return const_iterator(this, size_);
    }

    int
    size() const
    {
      return size_;
    }

    void
    push_back(bool value)
    {
      if(size_ % 8 == 0)
      {
        array_.push_back(value ? 1 : 0);
      }
      else
      {
        set(size_, value);
      }

      ++size_;
    }

    void
    resize(int size, bool value)
    {
      int prev_size = size_;
      array_.resize(size / 8 + (size % 8 ? 1 : 0), value ? 0xFF : 0);
      size_ = size;
      if(prev_size < size_)
      {
        for(int p = prev_size; p < std::min((prev_size / 8 + 1) * 8, size_); ++p)
        {
          set(p, value);
        }
      }
      
      if(size_ % 8 == 0)
      {
        array_.push_back(value ? 1 : 0);
        ++size_;
      }
      else
      {
        set(size_, value);
      }
    }

    void
    swap(bit_deque& other)
    {
      std::swap(size_, other.size_);
      array_.swap(other.array_);
    }
    
    bool
    get(int pos) const
    {
      return array_[pos / 8] & (1 << (pos % 8));
    }
    
    void
    set(int pos, bool value)
    {
      if(value)
      {
        array_[pos / 8] |= (1 << (pos % 8));
      }
      else
      {
        array_[pos / 8] &= ~(1 << (pos % 8));
      }
    }

  protected:
    int size_;
    std::deque<unsigned char> array_;
  };

  //
  // specific container wrappers that allow:
  // safe fetch of containers by portions
  //
  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  class FetchableHashTable
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(MaxIndexReached, Exception);

    typedef uint32_t IndexType;
    typedef FetchableHashTable<KeyType, ValueType, HashSetType>
      ThisFetchableHashTable;
    typedef std::vector<std::pair<KeyType, ValueType> > FetchArray;

    class Fetcher
    {
      friend class FetchableHashTable;

    public:
      Fetcher(const Fetcher& init)
        : owner_(init.owner_),
          position_(init.position_)
      {}

      bool
      get(FetchArray& ret,
        unsigned long actual_fetch_size,
        unsigned long max_fetch_size = 0);

    protected:
      Fetcher(const FetchableHashTable& owner);

    protected:
      const FetchableHashTable& owner_;
      unsigned long position_;
    };

    FetchableHashTable();

    FetchableHashTable(FetchableHashTable&& init);

    bool
    get(ValueType& value, KeyType key);

    void
    set(KeyType key, ValueType value) /*throw(MaxIndexReached)*/;

    bool
    erase(KeyType key);

    Fetcher
    fetcher() const;

  protected:
    struct HashElement
    {
      HashElement()
      {}

      HashElement(KeyType key_val)
        : key(std::move(key_val))
      {}

      HashElement(KeyType key_val, ValueType value_val)
        : key(std::move(key_val)),
          value(std::move(value_val))
      {}

      HashElement(HashElement&& init)
        : key(std::move(init.key)),
          value(std::move(init.value))
      {}

      HashElement&
      operator=(HashElement&& init)
      {
        key = std::move(init.key);
        value = std::move(init.value);
        return *this;
      }

      KeyType key;
      ValueType value;
    };

    struct HashElementHashOp
    {
    public:
      HashElementHashOp(
        const FetchableHashTable<KeyType, ValueType, HashSetType>& owner)
        : owner_(&owner)
      {}

      HashElementHashOp(const HashElementHashOp& init)
        : owner_(init.owner_)
      {}

      HashElementHashOp(HashElementHashOp&& init)
        : owner_(init.owner_)
      {}

      HashElementHashOp&
      operator=(HashElementHashOp&& init)
      {
        owner_ = init.owner_;
        return *this;
      }

      size_t
      operator()(uint32_t element_index) const
      {
        size_t ret_hash = owner_->get_hash_element_(element_index).key.hash();
        return ret_hash;
      };

      /*
      size_t
      operator()(const KeyType& key) const
      {
        return key.hash();
      };
      */

    protected:
      const FetchableHashTable<KeyType, ValueType, HashSetType>* owner_;
    };

    struct HashElementEqualOp
    {
    public:
      HashElementEqualOp(
        const FetchableHashTable<KeyType, ValueType, HashSetType>& owner)
        : owner_(&owner)
      {}

      HashElementEqualOp(const HashElementEqualOp& init)
        : owner_(init.owner_)
      {}

      HashElementEqualOp(HashElementEqualOp&& init)
        : owner_(init.owner_)
      {}

      HashElementEqualOp&
      operator=(HashElementEqualOp&& init)
      {
        owner_ = init.owner_;
        return *this;
      }

      size_t
      operator()(uint32_t element_index1, uint32_t element_index2) const
      {
        return owner_->get_hash_element_(element_index1).key ==
          owner_->get_hash_element_(element_index2).key;
      };

    protected:
      const FetchableHashTable<KeyType, ValueType, HashSetType>* owner_;
    };

    typedef std::deque<HashElement> HashElementArray;

    typedef HashSetType<
      IndexType,
      HashElementHashOp,
      HashElementEqualOp>
      HashElementSet;

    typedef Sync::Policy::PosixThread SyncPolicy;

  protected:
    HashElement&
    get_hash_element_(unsigned long index)
    {
      return all_elements_[index];
    }

    const HashElement&
    get_hash_element_(unsigned long index) const
    {
      return all_elements_[index];
    }

  protected:
    mutable SyncPolicy::Mutex lock_;
    bit_deque actual_elements_;
    HashElementArray all_elements_;
    HashElementSet element_map_;
  };

  //
  template<typename KeyType, typename HashOp, typename EqualOp>
  struct UnorderedSet: public std::unordered_set<KeyType, HashOp, EqualOp>
  {
    UnorderedSet(int bucket_count, HashOp hash_op, EqualOp equal_op)
      : std::unordered_set<KeyType, HashOp, EqualOp>(
        bucket_count,
        hash_op,
        equal_op)
    {}
  };

  template<typename KeyType, typename HashOp, typename EqualOp>
  struct SparseSet: public tsl::sparse_set<KeyType, HashOp, EqualOp>
  {
    SparseSet(int bucket_count, HashOp hash_op, EqualOp equal_op)
      : tsl::sparse_set<KeyType, HashOp, EqualOp>(
        bucket_count,
        hash_op,
        equal_op)
    {}
  };

  template<typename KeyType, typename ValueType>
  struct USFetchableHashTable:
    public FetchableHashTable<KeyType, ValueType, UnorderedSet>
  {};

  template<typename KeyType, typename ValueType>
  struct SparseFetchableHashTable:
    public FetchableHashTable<KeyType, ValueType, SparseSet>
  {};
}
}

#include "FetchableHashTable.tpp"

#endif /*USERINFOSVCS_USERBINDSERVER_FETCHABLEHASHTABLE_HPP*/
