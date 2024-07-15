#ifndef USERINFOSVCS_USERBINDSERVER_FETCHABLEHASHTABLE_HPP
#define USERINFOSVCS_USERBINDSERVER_FETCHABLEHASHTABLE_HPP

#include <deque>
#include <bitset>
#include <variant>
#include <Generics/GnuHashTable.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Commons/tsl/sparse_set.h>

namespace AdServer::UserInfoSvcs
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

  template<typename KeyType, typename ValueType>
  class FetcherDelegate : virtual private Generics::Uncopyable
  {
  public:
    FetcherDelegate() = default;

    virtual ~FetcherDelegate() = default;

    virtual void on_hashtable_erase(
      KeyType&& key,
      ValueType&& value) noexcept = 0;
  };

  template<typename KeyType, typename ValueType>
  using FetcherDelegatePtr = std::shared_ptr<FetcherDelegate<KeyType, ValueType>>;

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

    using IndexType = uint32_t;
    using ThisFetchableHashTable = FetchableHashTable<KeyType, ValueType, HashSetType>;
    using FetchArray = std::vector<std::pair<KeyType, ValueType>>;

    class FilterDefault final
    {
    public:
      bool operator()(const ValueType& /*value*/)
      {
        return true;
      }
    };

    template<class Filter = FilterDefault>
    class Fetcher : Generics::Uncopyable
    {
    private:
      friend class FetchableHashTable;
      using Data = std::pair<KeyType, ValueType>;
      using HelperArray = std::vector<Data>;

    public:
      bool get(FetchArray& ret,
        unsigned long actual_fetch_size,
        unsigned long max_fetch_size = 0);

    protected:
      Fetcher(
        FetchableHashTable& owner,
        const Filter& filter = Filter{},
        const FetcherDelegatePtr<KeyType, ValueType>& delegate = {});

    protected:
      FetchableHashTable& owner_;
      IndexType position_;
      Filter filter_;
      FetcherDelegatePtr<KeyType, ValueType> delegate_;
      HelperArray helper_array_;
    };

    FetchableHashTable();

    FetchableHashTable(FetchableHashTable&& other);

    bool get(ValueType& value, KeyType key);

    void set(KeyType key, ValueType value);

    bool erase(KeyType key);

    template<class Filter = FilterDefault>
    Fetcher<Filter> fetcher(
      const Filter& filter = Filter{},
      const FetcherDelegatePtr<KeyType, ValueType>& delegate = {});

  protected:
    struct HashElement final
    {
      explicit HashElement()
      {}

      explicit HashElement(KeyType&& key_val)
        : key(std::move(key_val))
      {}

      explicit HashElement(const KeyType& key_val)
        : key(key_val)
      {}

      explicit HashElement(KeyType&& key_val, ValueType&& value_val)
        : key(std::move(key_val)),
          value(std::move(value_val))
      {}

      HashElement(const KeyType& key_val, const ValueType& value_val)
        : key(key_val),
          value(value_val)
      {}

      HashElement(HashElement&& init)
        : key(std::move(init.key)),
          value(std::move(init.value))
      {}

      HashElement& operator=(HashElement&& init)
      {
        if (this != &init)
        {
          key = std::move(init.key);
          value = std::move(init.value);
        }

        return *this;
      }

      KeyType key;
      ValueType value;
    };

    struct HashElementVariantHashOp
    {
    public:
      explicit HashElementVariantHashOp(
        const FetchableHashTable<KeyType, ValueType, HashSetType>& owner)
        : owner_(&owner)
      {}

      HashElementVariantHashOp(const HashElementVariantHashOp& init)
        : owner_(init.owner_)
      {}

      HashElementVariantHashOp(HashElementVariantHashOp&& init)
        : owner_(std::move(init.owner_))
      {}

      HashElementVariantHashOp& operator=(HashElementVariantHashOp&& init)
      {
        if (this != &init)
        {
          owner_ = std::move(init.owner_);
        }

        return *this;
      }

      auto operator()(const IndexType element_index) const
      {
        const auto& variant = owner_->hash_element_variant_(element_index);
        const auto hash = std::get<HashElement>(variant).key.hash();
        return hash;
      };

    protected:
      const FetchableHashTable<KeyType, ValueType, HashSetType>* owner_;
    };

    struct HashElementVariantEqualOp
    {
    public:
      HashElementVariantEqualOp(
        const FetchableHashTable<KeyType, ValueType, HashSetType>& owner)
        : owner_(&owner)
      {}

      HashElementVariantEqualOp(const HashElementVariantEqualOp& other)
        : owner_(other.owner_)
      {}

      HashElementVariantEqualOp(HashElementVariantEqualOp&& other)
        : owner_(std::move(other.owner_))
      {}

      HashElementVariantEqualOp& operator=(HashElementVariantEqualOp&& other)
      {
        if (this != &other)
        {
          owner_ = std::move(other.owner_);
        }

        return *this;
      }

      bool operator() (
        const IndexType element_index1,
        const IndexType element_index2) const
      {
        const auto& variant1 = owner_->hash_element_variant_(element_index1);
        const auto& variant2 = owner_->hash_element_variant_(element_index2);
        if (variant1.index() != variant2.index())
        {
          return false;
        }

        return std::get<HashElement>(variant1).key == std::get<HashElement>(variant2).key;
      };

    protected:
      const FetchableHashTable<KeyType, ValueType, HashSetType>* owner_;
    };

    using HashElementVariant = std::variant<HashElement, IndexType>;
    using HashElementArray = std::deque<HashElementVariant>;
    using HashElementSet = HashSetType<
      IndexType,
      HashElementVariantHashOp,
      HashElementVariantEqualOp>;
    using SyncPolicy = Sync::Policy::PosixThread;

  protected:
    HashElementVariant& hash_element_variant_(const IndexType index)
    {
      return all_elements_[index];
    }

    const HashElementVariant& hash_element_variant_(const IndexType index) const
    {
      return all_elements_[index];
    }

  private:
    template<typename Key,
             typename = std::enable_if_t<std::is_same_v<std::decay_t<Key>, KeyType>>>
    bool erase_no_guard(Key&& key);

  protected:
    mutable SyncPolicy::Mutex lock_;
    bit_deque actual_elements_;
    HashElementArray all_elements_;
    IndexType head_index_free_ = 0;
    HashElementSet element_map_;
  };

  //
  template<typename KeyType, typename HashOp, typename EqualOp>
  struct UnorderedSet final: public std::unordered_set<KeyType, HashOp, EqualOp>
  {
    UnorderedSet(int bucket_count, HashOp hash_op, EqualOp equal_op)
      : std::unordered_set<KeyType, HashOp, EqualOp>(
        bucket_count,
        hash_op,
        equal_op)
    {}
  };

  template<typename KeyType, typename HashOp, typename EqualOp>
  struct SparseSet final: public tsl::sparse_set<KeyType, HashOp, EqualOp>
  {
    SparseSet(int bucket_count, HashOp hash_op, EqualOp equal_op)
      : tsl::sparse_set<KeyType, HashOp, EqualOp>(
        bucket_count,
        hash_op,
        equal_op)
    {}
  };

  template<typename KeyType, typename ValueType>
  struct USFetchableHashTable final : public FetchableHashTable<KeyType, ValueType, UnorderedSet>
  {};

  template<typename KeyType, typename ValueType>
  struct SparseFetchableHashTable final : public FetchableHashTable<KeyType, ValueType, SparseSet>
  {};
} // namespace AdServer::UserInfoSvcs

#include "FetchableHashTable.tpp"

#endif /*USERINFOSVCS_USERBINDSERVER_FETCHABLEHASHTABLE_HPP*/
