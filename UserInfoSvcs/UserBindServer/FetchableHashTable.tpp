namespace AdServer::UserInfoSvcs
{
  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  template<class Filter>
  FetchableHashTable<KeyType, ValueType, HashSetType>::Fetcher<Filter>::Fetcher(
    FetchableHashTable& owner,
    const Filter& filter,
    const FetcherDelegatePtr<KeyType, ValueType>& delegate)
    : owner_(owner),
      position_(1),
      filter_(filter),
      delegate_(delegate)
  {}

  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  template<class Filter>
  bool FetchableHashTable<KeyType, ValueType, HashSetType>::Fetcher<Filter>::get(
    FetchArray& ret,
    unsigned long fetch_actual_size,
    unsigned long max_fetch_size)
  {
    ret.clear();
    ret.reserve(fetch_actual_size);
    helper_array_.clear();
    helper_array_.reserve(fetch_actual_size);
    bool need_continue = false;

    {
      unsigned long local_pos = 0;
      unsigned long local_full_pos = 0;

      FetchableHashTable::SyncPolicy::WriteGuard lock(owner_.lock_);
      auto el_it = owner_.all_elements_.begin() + position_;
      auto actual_it = owner_.actual_elements_.begin() + position_;

      while(el_it != owner_.all_elements_.end() &&
        local_pos < fetch_actual_size &&
        (max_fetch_size == 0 || local_full_pos < max_fetch_size))
      {
        if(*actual_it)
        {
          auto& element = std::get<HashElement>(*el_it);
          if (filter_(element.value))
          {
            ret.emplace_back(element.key, element.value);
            ++local_pos;
          }
          else
          {
            helper_array_.emplace_back(
              element.key,
              std::move(element.value));
          }
        }

        ++el_it;
        ++actual_it;
        ++local_full_pos;
      }

      position_ += local_full_pos;
      need_continue =  el_it != owner_.all_elements_.end();

      for (const auto& [key, _] : helper_array_)
      {
        owner_.erase_no_guard(key);
      }
    }

    if (delegate_)
    {
      for (auto& [key, value] : helper_array_)
      {
        delegate_->on_hashtable_erase(
          std::move(key),
          std::move(value));
      }
    }

    return need_continue;
  }

  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  FetchableHashTable<KeyType, ValueType, HashSetType>::FetchableHashTable()
    : element_map_(
        static_cast<typename HashElementSet::size_type>(1024),
        HashElementVariantHashOp(*this),
        HashElementVariantEqualOp(*this))
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
        HashElementVariantHashOp(*this),
        HashElementVariantEqualOp(*this))
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
    SyncPolicy::WriteGuard lock(lock_);
    std::get<HashElement>(all_elements_.front()).key = std::move(key);

    const auto it = element_map_.find(0);
    if(it != element_map_.end() && actual_elements_[*it])
    {
      value = std::get<HashElement>(all_elements_[*it]).value;
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
  {
    SyncPolicy::WriteGuard lock(lock_);
    auto& first_element = std::get<HashElement>(all_elements_.front());
    first_element.key = std::move(key);

    auto it = element_map_.find(0);
    if(it != element_map_.end())
    {
      all_elements_[*it] = HashElement(
        std::move(first_element.key),
        std::move(value));
    }
    else
    {
      if(head_index_free_ == 0
      && all_elements_.size() + 1 > std::numeric_limits<IndexType>::max())
      {
        throw MaxIndexReached("");
      }

      if (head_index_free_ == 0)
      {
        all_elements_.push_back(HashElement(
           std::move(first_element.key),
           std::move(value)));
        actual_elements_.push_back(true);
        unsigned long new_index = all_elements_.size() - 1;
        element_map_.insert(new_index);
      }
      else
      {
        const auto index = head_index_free_;
        auto& index_element = all_elements_[index];
        head_index_free_ = std::get<IndexType>(index_element);
        actual_elements_[index] = true;
        index_element = HashElement(
          std::move(first_element.key),
          std::move(value));
        element_map_.insert(index);
      }
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
    SyncPolicy::WriteGuard lock(lock_);
    return erase_no_guard(std::move(key));
  }

  template<
    typename KeyType,
    typename ValueType,
    template<typename, typename, typename> class HashSetType>
  template<typename Key, typename>
  bool FetchableHashTable<KeyType, ValueType, HashSetType>::erase_no_guard(Key&& key)
  {
    std::get<HashElement>(all_elements_.front()).key = std::forward<Key>(key);
    auto it = element_map_.find(0);
    if(it != element_map_.end())
    {
      all_elements_[*it] = head_index_free_;
      head_index_free_ = *it;
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
  template<class Filter>
  typename FetchableHashTable<KeyType, ValueType, HashSetType>::Fetcher<Filter>
  FetchableHashTable<KeyType, ValueType, HashSetType>::fetcher(
    const Filter& filter,
    const FetcherDelegatePtr<KeyType, ValueType>& delegate)
  {
    return typename FetchableHashTable<KeyType, ValueType, HashSetType>::Fetcher<Filter>(
      *this,
      filter,
      delegate);
  }
} // namespace AdServer::UserInfoSvcs