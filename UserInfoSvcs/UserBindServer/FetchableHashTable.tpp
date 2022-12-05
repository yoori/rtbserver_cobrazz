namespace AdServer
{
namespace UserInfoSvcs
{
  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  FetchableHashTable<KeyType, ValueType, HashSetType>::Fetcher::Fetcher(
    const FetchableHashTable<KeyType, ValueType, HashSetType>& owner)
    : owner_(owner),
      position_(1)
  {}

  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  bool
  FetchableHashTable<KeyType, ValueType, HashSetType>::Fetcher::get(
    FetchArray& ret,
    unsigned long fetch_actual_size,
    unsigned long max_fetch_size)
  {
    ret.clear();
    ret.reserve(fetch_actual_size);
    unsigned long local_pos = 0;
    unsigned long local_full_pos = 0;

    FetchableHashTable<KeyType, ValueType, HashSetType>::
      SyncPolicy::WriteGuard lock(owner_.lock_);
    auto el_it = owner_.all_elements_.begin() + position_;
    auto actual_it = owner_.actual_elements_.begin() + position_;

    while(el_it != owner_.all_elements_.end() &&
      local_pos < fetch_actual_size &&
      (max_fetch_size == 0 || local_full_pos < max_fetch_size))
    {
      if(*actual_it)
      {
        //std::cout << "next actual(" << local_pos << ")" << std::endl;
        ret.push_back(std::make_pair(el_it->key, el_it->value));
        ++local_pos;
      }
      else
      {
        //std::cout << "next non actual(" << local_pos << ")" << std::endl;
      }

      ++el_it;
      ++actual_it;
      ++local_full_pos;
    }

    position_ += local_pos;
    return el_it != owner_.all_elements_.end();
  }

  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  FetchableHashTable<KeyType, ValueType, HashSetType>::FetchableHashTable()
    : element_map_(
        static_cast<typename HashElementSet::size_type>(1024),
        HashElementHashOp(*this),
        HashElementEqualOp(*this))
  {
    all_elements_.push_back(HashElement());
    actual_elements_.push_back(true);
  }

  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  FetchableHashTable<KeyType, ValueType, HashSetType>::FetchableHashTable(
    FetchableHashTable<KeyType, ValueType, HashSetType>&& init)
    : element_map_(
        static_cast<typename HashElementSet::size_type>(1024),
        HashElementHashOp(*this),
        HashElementEqualOp(*this))
  {
    SyncPolicy::WriteGuard lock(init.lock_);
    actual_elements_.swap(init.actual_elements_);
    all_elements_.swap(init.all_elements_);
    element_map_.swap(init.element_map_);
  }

  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  bool
  FetchableHashTable<KeyType, ValueType, HashSetType>::get(
    ValueType& value, KeyType key)
  {
    KeyType del_key;

    SyncPolicy::WriteGuard lock(lock_);
    del_key = std::move(all_elements_.begin()->key);
    all_elements_.begin()->key = std::move(key);

    auto it = element_map_.find(0);
    if(it != element_map_.end() && actual_elements_[*it])
    {
      value = all_elements_[*it].value;
      return true;
    }

    return false;
  }

  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  void
  FetchableHashTable<KeyType, ValueType, HashSetType>::set(
    KeyType key, ValueType value)
    /*throw(MaxIndexReached)*/
  {
    KeyType del_key;

    SyncPolicy::WriteGuard lock(lock_);
    del_key = std::move(all_elements_.begin()->key);
    all_elements_.begin()->key = std::move(key);

    auto it = element_map_.find(0);
    if(it != element_map_.end())
    {
      all_elements_[*it] = HashElement(
        std::move(all_elements_.begin()->key),
        std::move(value));
      actual_elements_[*it] = true;
    }
    else
    {
      if(all_elements_.size() + 1 > std::numeric_limits<IndexType>::max())
      {
        throw MaxIndexReached("");
      }

      all_elements_.push_back(HashElement(
         std::move(all_elements_.begin()->key),
         std::move(value)));
      actual_elements_.push_back(true);
      unsigned long new_index = all_elements_.size() - 1;
      element_map_.insert(new_index);
    }
  }

  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  bool
  FetchableHashTable<KeyType, ValueType, HashSetType>::erase(
    KeyType key)
  {
    KeyType del_key;
    HashElement del_hash_element;

    SyncPolicy::WriteGuard lock(lock_);
    del_key = std::move(all_elements_.begin()->key);
    all_elements_.begin()->key = std::move(key);

    auto it = element_map_.find(0);
    if(it != element_map_.end())
    {
      del_hash_element = std::move(all_elements_[*it]);
      all_elements_[*it] = HashElement();
      actual_elements_[*it] = false;
      element_map_.erase(it);

      return true;
    }

    return false;
  }

  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  typename FetchableHashTable<KeyType, ValueType, HashSetType>::Fetcher
  FetchableHashTable<KeyType, ValueType, HashSetType>::fetcher() const
  {
    return typename FetchableHashTable<
      KeyType, ValueType, HashSetType>::Fetcher(*this);
  }
}
}
